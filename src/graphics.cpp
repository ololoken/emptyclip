/******************************************************************************
* Empty Clip
* Copyright (C) 2022  Alan Witkowski
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/
#include <graphics.h>
#include <ae/texture.h>
#include <ae/texture_array.h>
#include <assets.h>
#include <program.h>
#include <color.h>
#include <stdexcept>
#include <constants.h>
#include <ui/ui.h>
#include <SDL_mouse.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>

_Graphics Graphics;

// Initialize
void _Graphics::Init(const _WindowSettings &WindowSettings) {

	// Initialize
	CircleVertices = 32;
	Anisotropy = 0.0f;
	FramesPerSecond = 0;
	FrameCount = 0;
	FrameRateTimer = 0;
	Context = nullptr;
	Window = nullptr;
	VertexArrayID = 0;
	Element = nullptr;

	// Set sizes
	SDL_DisplayMode DisplayMode;
	WindowSize = WindowSettings.Size;
	FullscreenSize = glm::ivec2(0);
	if(SDL_GetDesktopDisplayMode(0, &DisplayMode) == 0)
		FullscreenSize = glm::ivec2(DisplayMode.w, DisplayMode.h);

	// Set video flags
	Uint32 VideoFlags = SDL_WINDOW_OPENGL;
	if(WindowSettings.Fullscreen) {
		VideoFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		CurrentSize = FullscreenSize;
	}
	else
		CurrentSize = WindowSize;

	// Set opengl attributes
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	if(WindowSettings.MSAA > 0) {
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, WindowSettings.MSAA);
	}

	// Load cursors
	Cursors[CURSOR_NONE] = nullptr;
	Cursors[CURSOR_MAIN] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	Cursors[CURSOR_CROSS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
	SDL_SetCursor(Cursors[CURSOR_MAIN]);

	// Create window
	Window = SDL_CreateWindow(WindowSettings.WindowTitle.c_str(), WindowSettings.Position.x, WindowSettings.Position.y, CurrentSize.x, CurrentSize.y, VideoFlags);
	if(Window == nullptr)
		throw std::runtime_error("SDL_CreateWindow failed");

	// Set up opengl context
	Context = SDL_GL_CreateContext(Window);
	if(Context == nullptr)
		throw std::runtime_error("SDL_GL_CreateContext failed");

	InitGLFunctions();

	int MajorVersion, MinorVersion;
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &MajorVersion);
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &MinorVersion);

	// Set vsync
	SDL_GL_SetSwapInterval(WindowSettings.Vsync);

	// Set up OpenGL
	SetupOpenGL();

	// Setup viewport
	SetViewport(CurrentSize);
}

// Sets up OpenGL
void _Graphics::SetupOpenGL() {

	// Anisotropic filtering
	if(SDL_GL_ExtensionSupported("GL_EXT_texture_filter_anisotropic")) {
		GLfloat MaxAnisotropy;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &MaxAnisotropy);

		if(Anisotropy > MaxAnisotropy)
			Anisotropy = MaxAnisotropy;
	}

	// Default state
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Create vertex array
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);
	glEnableVertexAttribArray(0);

	// Set ortho matrix
	Ortho = glm::ortho(0.0f, (float)CurrentSize.x, (float)CurrentSize.y, 0.0f, -1.0f, 1.0f);

	// Build vertex buffers
	BuildVertexBuffers();

	// Reset internal state
	ResetState();

	// Clear screen
	ClearScreen();
	Flip(0);
}

// Shutdown
void _Graphics::Close() {
	delete Element;
	Element = nullptr;

	// Close opengl context
	if(Context) {
		for(int i = 1; i < VBO_COUNT; i++)
			glDeleteBuffers(1, &VertexBuffer[i]);

		glDeleteVertexArrays(1, &VertexArrayID);

		SDL_GL_DeleteContext(Context);
		Context = nullptr;
	}

	// Close cursors
	for(int i = 1; i < CURSOR_COUNT; i++)
		SDL_FreeCursor(Cursors[i]);

	// Close window
	if(Window) {
		SDL_DestroyWindow(Window);
		Window = nullptr;
	}
}

// Builds the vertex buffer objects
void _Graphics::BuildVertexBuffers() {
	VertexBuffer[VBO_NONE] = 0;

	// Line
	{
		float Vertices[] = {
			0.0f, 0.0f,
			1.0f, 1.0f,
		};

		VertexBuffer[VBO_LINE] = CreateVBO(Vertices, sizeof(Vertices), GL_STATIC_DRAW);
	}

	// Circle
	{
		float *Vertices = new float[CircleVertices * 2];

		// Get vertices
		for(int i = 0; i < CircleVertices; i++) {
			float Radians = ((float)i / CircleVertices) * (glm::pi<float>() * 2.0f);
			Vertices[i * 2] = std::cos(Radians);
			Vertices[i * 2 + 1] = std::sin(Radians);
		}

		VertexBuffer[VBO_CIRCLE] = CreateVBO(Vertices, sizeof(float) * (unsigned)CircleVertices * 2, GL_STATIC_DRAW);
		delete[] Vertices;
	}

	// Quad
	{
		float Vertices[] = {
			1.0f, 0.0f,
			0.0f, 0.0f,
			1.0f, 1.0f,
			0.0f, 1.0f,
		};

		VertexBuffer[VBO_QUAD] = CreateVBO(Vertices, sizeof(Vertices), GL_STATIC_DRAW);
	}

	// Rectangle
	{
		float Vertices[] = {
			0.0f, 0.0f,
			1.0f, 0.0f,
			1.0f, 1.0f,
			0.0f, 1.0f,
		};

		VertexBuffer[VBO_RECT] = CreateVBO(Vertices, sizeof(Vertices), GL_STATIC_DRAW);
	}

	// Centered textured quad
	{
		float Vertices[] = {
			-0.5f,  0.5f,
			 0.5f,  0.5f,
			-0.5f, -0.5f,
			 0.5f, -0.5f,
			 0.0f,  1.0f,
			 1.0f,  1.0f,
			 0.0f,  0.0f,
			 1.0f,  0.0f,
		};

		VertexBuffer[VBO_SPRITE] = CreateVBO(Vertices, sizeof(Vertices), GL_STATIC_DRAW);
	}

	// Dynamic vbo for drawing animations
	{
		float Vertices[] = {
			-0.5f,  0.5f,
			 0.5f,  0.5f,
			-0.5f, -0.5f,
			 0.5f, -0.5f,
			 1.0f,  0.0f,
			 0.0f,  0.0f,
			 1.0f,  1.0f,
			 0.0f,  1.0f,
		};

		VertexBuffer[VBO_ATLAS] = CreateVBO(Vertices, sizeof(Vertices), GL_DYNAMIC_DRAW);
	}

	// Textured quad
	{
		float Vertices[] = {
			1.0f, 0.0f,
			0.0f, 0.0f,
			1.0f, 1.0f,
			0.0f, 1.0f,
			1.0f, 0.0f,
			0.0f, 0.0f,
			1.0f, 1.0f,
			0.0f, 1.0f,
		};

		VertexBuffer[VBO_QUAD_UV] = CreateVBO(Vertices, sizeof(Vertices), GL_STATIC_DRAW);
	}

	// Cube
	{
		float Vertices[] = {

			// Top
			1.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
			0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,
			1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,
			0.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,

			// Front
			1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
			0.0f,  1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f,
			1.0f,  1.0f,  0.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,
			0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,

			// Left
			0.0f,  1.0f,  1.0f,  1.0f,  0.0f, -1.0f,  0.0f,  0.0f,
			0.0f,  0.0f,  1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
			0.0f,  1.0f,  0.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f,
			0.0f,  0.0f,  0.0f,  0.0f,  1.0f, -1.0f,  0.0f,  0.0f,

			// Back
			0.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f, -1.0f,  0.0f,
			1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f, -1.0f,  0.0f,
			0.0f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f, -1.0f,  0.0f,
			1.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f, -1.0f,  0.0f,

			// Right
			1.0f,  0.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,
			1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
			1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f,
			1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,
		};

		VertexBuffer[VBO_CUBE] = CreateVBO(Vertices, sizeof(Vertices), GL_STATIC_DRAW);
	}
}

// Create vertex buffer and return id
GLuint _Graphics::CreateVBO(float *Vertices, GLsizeiptr Size, GLenum Type) {

	// Create buffer
	GLuint BufferID;
	glGenBuffers(1, &BufferID);
	glBindBuffer(GL_ARRAY_BUFFER, BufferID);
	glBufferData(GL_ARRAY_BUFFER, Size, Vertices, Type);

	return BufferID;
}

// Resets all the last used variables
void _Graphics::ResetState() {
	SetAttribLevel(0);
	glUseProgram(0);
	glActiveTexture(GL_TEXTURE0);
	LastVertexBufferID = (GLuint)-1;
	LastTextureID = (GLuint)-1;
	LastAttribLevel = (GLuint)-1;
	LastProgram = nullptr;
	LastDepthTest = false;
}

// Throw opengl error
void _Graphics::CheckError() {
	GLenum Error = glGetError();
	if(Error)
		throw std::runtime_error("glGetError returned " + std::to_string(Error));
}

// Sets up the projection matrix for drawing 2D objects
void _Graphics::Setup2D() {
	glViewport(0, 0, CurrentSize.x, CurrentSize.y);
}

// Set up modelview matrix
void _Graphics::Setup3D() {
	glViewport(0, CurrentSize.y - ViewportSize.y, ViewportSize.x, ViewportSize.y);
}

// Fade the screen
void _Graphics::FadeScreen(const _Program *Program, float Amount) {
	Graphics.SetProgram(Program);
	Graphics.SetColor(glm::vec4(0.0f, 0.0f, 0.0f, Amount));
	DrawRectangle(glm::vec2(0, 0), CurrentSize, true);
}

// Clears the screen
void _Graphics::ClearScreen() {
	SetDepthMask(true);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

// Draws the frame
void _Graphics::Flip(double FrameTime) {

	// Swap buffers
	SDL_GL_SwapWindow(Window);

	// Clear screen
	ClearScreen();

	// Update frame counter
	FrameCount++;
	FrameRateTimer += FrameTime;
	if(FrameRateTimer >= 1.0) {
		FramesPerSecond = FrameCount;
		FrameCount = 0;
		FrameRateTimer -= 1.0;
	}

	// Check for errors
	#ifndef NDEBUG
		CheckError();
	#endif
}

// Change the viewport
void _Graphics::SetViewport(const glm::ivec2 &Size) {
	ViewportSize = Size;

	// Calculate aspect ratio
	AspectRatio = (float)ViewportSize.x / ViewportSize.y;
}

// Change window and viewport size
void _Graphics::SetWindowSize(const glm::ivec2 &Size) {

	// Keep viewport difference the same
	glm::ivec2 ViewportDifference = CurrentSize - ViewportSize;

	// Change viewport size
	CurrentSize = Size;
	SetViewport(Size - ViewportDifference);

	// Update shaders
	Ortho = glm::ortho(0.0f, (float)CurrentSize.x, (float)CurrentSize.y, 0.0f, -1.0f, 1.0f);
	SetStaticUniforms();

	// Update UI elements
	Element->Size = Size;
	Element->CalculateBounds(false);

	// Update actual window
	SDL_SetWindowSize(Window, Size.x, Size.y);
}

// Toggle fullscreen
bool _Graphics::SetFullscreen(bool Fullscreen) {
	if(FullscreenSize == glm::ivec2(0))
		return false;

	if(Fullscreen)
		Graphics.SetWindowSize(FullscreenSize);
	else
		Graphics.SetWindowSize(WindowSize);

	if(SDL_SetWindowFullscreen(Window, SDL_GetWindowFlags(Window) ^ SDL_WINDOW_FULLSCREEN_DESKTOP) != 0)
		return false;

	return true;
}

// Assign uniform values in program
void _Graphics::SetStaticUniforms() {
	SetProgram(Assets.Programs["ortho_pos"]);
	glUniformMatrix4fv(Assets.Programs["ortho_pos"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Ortho));
	SetProgram(Assets.Programs["ortho_pos_uv"]);
	glUniformMatrix4fv(Assets.Programs["ortho_pos_uv"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Ortho));
	SetProgram(Assets.Programs["text"]);
	glUniformMatrix4fv(Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Ortho));
}

// Set Vsync
bool _Graphics::SetVsync(bool Vsync) {
	return SDL_GL_SetSwapInterval(Vsync) == 0;
}

// Get Vsync value
bool _Graphics::GetVsync() {
	return SDL_GL_GetSwapInterval();
}

// Set mouse cursor icon
void _Graphics::SetCursor(int Type) {
	if(Type == CURSOR_NONE) {
		SDL_ShowCursor(false);
	}
	else {
		SDL_ShowCursor(true);
		SDL_SetCursor(Cursors[Type]);
	}
}

// Enable state for VBO
void _Graphics::SetVBO(GLuint VBO) {
	if(LastVertexBufferID == VBO)
		return;

	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer[VBO]);

	switch(VBO) {
		case VBO_CUBE:
			SetAttribLevel(3);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, nullptr);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid *)(sizeof(glm::vec3)));
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid *)(sizeof(glm::vec3) + sizeof(glm::vec2)));
		break;
		case VBO_SPRITE:
		case VBO_ATLAS:
		case VBO_QUAD_UV:
			SetAttribLevel(2);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (GLvoid *)(sizeof(float) * 8));
		break;
		case VBO_LINE:
		case VBO_RECT:
		case VBO_QUAD:
		case VBO_CIRCLE:
			SetAttribLevel(1);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
		break;
	}

	LastVertexBufferID = VBO;
}

// Enable vertex attrib arrays
void _Graphics::SetAttribLevel(GLuint AttribLevel) {
	if(AttribLevel == LastAttribLevel)
		return;

	if(AttribLevel < LastAttribLevel && LastAttribLevel != (GLuint)-1) {
		for(GLuint i = 1; i < LastAttribLevel; i++)
			glDisableVertexAttribArray(i);
	}

	for(GLuint i = 1; i < AttribLevel; i++)
		glEnableVertexAttribArray(i);

	LastAttribLevel = AttribLevel;
}

// Set opengl color
void _Graphics::SetColor(const glm::vec4 &Color) {
	glUniform4fv(LastProgram->ColorID, 1, &Color[0]);
}

// Set texture id
void _Graphics::SetTextureID(GLuint TextureID, GLenum Type) {
	if(TextureID == LastTextureID)
		return;

	glBindTexture(Type, TextureID);

	LastTextureID = TextureID;
}

// Set vertex buffer id
void _Graphics::SetVertexBufferID(GLuint VertexBufferID) {
	if(VertexBufferID == LastVertexBufferID)
		return;

	glBindBuffer(GL_ARRAY_BUFFER, VertexBufferID);
	LastVertexBufferID = VertexBufferID;
}

// Enable a program
void _Graphics::SetProgram(const _Program *Program) {
	if(Program == LastProgram)
		return;

	SetAttribLevel(Program->Attribs);
	Program->Use();
	LastProgram = Program;
}

// Enable/disable depth test
void _Graphics::SetDepthTest(bool DepthTest) {
	if(DepthTest == LastDepthTest)
		return;

	if(DepthTest)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);

	LastDepthTest = DepthTest;
}

// Set cull face
void _Graphics::SetCullFace(bool Value) {
	if(Value)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);
}

// Set scissor region
void _Graphics::SetScissor(const _Bounds &Bounds) {
	glScissor((GLint)Bounds.Start.x, (GLint)(CurrentSize.y - Bounds.End.y), (GLsizei)(Bounds.End.x - Bounds.Start.x), (GLsizei)(Bounds.End.y - Bounds.Start.y));
}

// Set depth mask
void _Graphics::SetDepthMask(bool Value) {
	glDepthMask(Value);
}

// Enable stencil test
void _Graphics::EnableStencilTest() {
	glEnable(GL_STENCIL_TEST);
}

// Disable stencil tests
void _Graphics::DisableStencilTest() {
	glDisable(GL_STENCIL_TEST);
}

// Enable scissor test
void _Graphics::EnableScissorTest() {
	glEnable(GL_SCISSOR_TEST);
}

// Disable scissor test
void _Graphics::DisableScissorTest() {
	glDisable(GL_SCISSOR_TEST);
}

// Enable blending mode for particles
void _Graphics::EnableParticleBlending() {
	glBlendFunc(GL_SRC_ALPHA, 1);
}

// Disable blending mode for particles
void _Graphics::DisableParticleBlending() {
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// Draw line
void _Graphics::DrawLine(const glm::vec2 &Start, const glm::vec2 &End) {
	SetVBO(VBO_LINE);
	glm::vec2 Size = End - Start;

	glm::mat4 Transform(1.0f);
	Transform[3][0] = Start.x;
	Transform[3][1] = Start.y;
	Transform[0][0] = Size.x;
	Transform[1][1] = Size.y;
	glUniformMatrix4fv(LastProgram->ModelTransformID, 1, GL_FALSE, glm::value_ptr(Transform));
	glDrawArrays(GL_LINES, 0, 2);
}

// Draw rectangle in screen space
void _Graphics::DrawRectangle(const _Bounds &Bounds, bool Filled) {
	DrawRectangle(glm::vec2(Bounds.Start.x, Bounds.Start.y), glm::vec2(Bounds.End.x, Bounds.End.y), Filled);
}

// Draw rectangle
void _Graphics::DrawRectangle(const glm::vec2 &Start, const glm::vec2 &End, bool Filled) {

	// Get transform
	glm::mat4 Transform(1.0f);
	if(Filled) {
		SetVBO(VBO_QUAD);
		Transform = glm::translate(Transform, glm::vec3(Start, 0.0f));
		Transform = glm::scale(Transform, glm::vec3(End - Start, 0.0f));
	}
	else {
		SetVBO(VBO_RECT);
		Transform = glm::translate(Transform, glm::vec3(Start.x + 0.5f, Start.y + 0.5f, 0.0f));
		Transform = glm::scale(Transform, glm::vec3(End - Start - glm::vec2(1.0f), 0.0f));
	}

	glUniformMatrix4fv(LastProgram->ModelTransformID, 1, GL_FALSE, glm::value_ptr(Transform));
	if(Filled)
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	else
		glDrawArrays(GL_LINE_LOOP, 0, 4);
}

// Draw rectangle in 3D space
void _Graphics::DrawRectangle3D(const glm::vec2 &Start, const glm::vec2 &End, bool Filled) {

	// Get transform
	glm::mat4 Transform(1.0f);
	if(Filled) {
		SetVBO(VBO_QUAD);
		Transform = glm::translate(Transform, glm::vec3(Start, 0.0f));
		Transform = glm::scale(Transform, glm::vec3(End - Start, 0.0f));
	}
	else {
		SetVBO(VBO_RECT);
		Transform = glm::translate(Transform, glm::vec3(Start.x, Start.y, 0.0f));
		Transform = glm::scale(Transform, glm::vec3(End - Start, 0.0f));
	}

	glUniformMatrix4fv(LastProgram->ModelTransformID, 1, GL_FALSE, glm::value_ptr(Transform));
	if(Filled)
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	else
		glDrawArrays(GL_LINE_LOOP, 0, 4);
}

// Draw circle
void _Graphics::DrawCircle(const glm::vec3 &Position, float Radius) {
	Graphics.SetVBO(VBO_CIRCLE);

	glm::mat4 ModelTransform;
	ModelTransform = glm::translate(glm::mat4(1.0f), Position);
	ModelTransform = glm::scale(ModelTransform, glm::vec3(Radius, Radius, 0.0f));
	glUniformMatrix4fv(LastProgram->ModelTransformID, 1, GL_FALSE, glm::value_ptr(ModelTransform));

	glDrawArrays(GL_LINE_LOOP, 0, (GLsizei)CircleVertices);
}

// Draw stencil mask
void _Graphics::DrawMask(const _Bounds &Bounds) {

	// Enable stencil
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glStencilMask(0x01);

	// Write 1 to stencil buffer
	glStencilFunc(GL_ALWAYS, 0x01, 0x01);
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

	// Draw region
	DrawRectangle(Bounds.Start, Bounds.End, true);

	// Then draw element only where stencil is 1
	glStencilFunc(GL_EQUAL, 0x01, 0x01);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glStencilMask(0x00);
}

// Draw image in screen space
void _Graphics::DrawImage(const _Bounds &Bounds, const ae::_Texture *Texture, bool Stretch) {
	SetVBO(VBO_QUAD_UV);
	SetTextureID(Texture->ID);

	// Get texture coordinates
	float S = Stretch ? 1.0f : (Bounds.End.x - Bounds.Start.x) / (float)(Texture->Size.x);
	float T = Stretch ? 1.0f : (Bounds.End.y - Bounds.Start.y) / (float)(Texture->Size.y);

	// Get size
	glm::vec2 Size = Bounds.End - Bounds.Start;

	// Model transform
	glm::mat4 Transform(1.0f);
	Transform[3][0] = Bounds.Start.x;
	Transform[3][1] = Bounds.Start.y;
	Transform[0][0] = Size.x;
	Transform[1][1] = Size.y;
	glUniformMatrix4fv(LastProgram->ModelTransformID, 1, GL_FALSE, glm::value_ptr(Transform));

	// Texture transform
	glm::mat4 TextureTransform(1.0f);
	TextureTransform[0][0] = S;
	TextureTransform[1][1] = T;
	glUniformMatrix4fv(LastProgram->TextureTransformID, 1, GL_FALSE, glm::value_ptr(TextureTransform));
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// Draw image centered given a size
void _Graphics::DrawScaledImage(const glm::vec2 &Position, const ae::_Texture *Texture, const glm::vec2 &Size, const glm::vec4 &Color) {
	Graphics.SetColor(Color);

	// Scale texture by UI scale
	glm::vec2 TextureSize = Size * 0.5f * _Element::GetUIScale();

	// Draw image
	_Bounds Bounds(Position - TextureSize, Position + TextureSize);
	DrawImage(Bounds, Texture, true);
}

// Draw 3d sprite
void _Graphics::DrawSprite(const glm::vec3 &Position, const ae::_Texture *Texture, float Rotation, const glm::vec2 &Scale) {
	SetVBO(VBO_SPRITE);
	SetTextureID(Texture->ID);

	Rotation = glm::radians(Rotation);

	glm::mat4 ModelTransform;
	ModelTransform = glm::translate(glm::mat4(1.0f), Position);
	if(Rotation != 0.0f)
		ModelTransform = glm::rotate(ModelTransform, Rotation, glm::vec3(0, 0, 1));

	ModelTransform = glm::scale(ModelTransform, glm::vec3(Scale, 0.0f));

	glUniformMatrix4fv(LastProgram->ModelTransformID, 1, GL_FALSE, glm::value_ptr(ModelTransform));

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// Draw frame from an animation
void _Graphics::DrawAnimationFrame(const glm::vec3 &Position, const ae::_Texture *Texture, const glm::vec4 &TextureCoords, float Rotation, const glm::vec2 &Scale) {
	SetVBO(VBO_ATLAS);
	SetTextureID(Texture->ID);

	// Set transform
	glm::mat4 ModelTransform;
	ModelTransform = glm::translate(glm::mat4(1.0f), Position);
	Rotation = glm::radians(Rotation);
	if(Rotation != 0.0f)
		ModelTransform = glm::rotate(ModelTransform, Rotation, glm::vec3(0, 0, 1));
	ModelTransform = glm::scale(ModelTransform, glm::vec3(Scale, 0.0f));
	glUniformMatrix4fv(LastProgram->ModelTransformID, 1, GL_FALSE, glm::value_ptr(ModelTransform));

	// Texture transform
	glm::mat4 TextureTransform(1.0f);
	TextureTransform[3][0] = TextureCoords[0];
	TextureTransform[3][1] = TextureCoords[1];
	TextureTransform[0][0] = TextureCoords[2] - TextureCoords[0];
	TextureTransform[1][1] = TextureCoords[3] - TextureCoords[1];
	glUniformMatrix4fv(LastProgram->TextureTransformID, 1, GL_FALSE, glm::value_ptr(TextureTransform));

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// Draw quad with repeated textures
void _Graphics::DrawRepeatable(const glm::vec3 &Start, const glm::vec3 &End, const ae::_Texture *Texture, float Rotation, float ScaleX) {
	SetVBO(VBO_QUAD_UV);
	SetTextureID(Texture->ID);

	// Get size
	glm::vec3 Size = End - Start;

	// Model transform
	glm::mat4 Transform(1.0f);
	Transform[3][0] = Start.x;
	Transform[3][1] = Start.y;
	Transform[3][2] = Start.z;
	Transform[0][0] = Size.x;
	Transform[1][1] = Size.y;
	Transform[2][2] = Size.z;
	glUniformMatrix4fv(LastProgram->ModelTransformID, 1, GL_FALSE, glm::value_ptr(Transform));

	// Texture transform
	glm::mat4 TextureTransform(1.0f);
	if(Rotation != 0.0f)
		TextureTransform = glm::rotate(TextureTransform, glm::radians(Rotation), glm::vec3(0, 0, -1));
	TextureTransform = glm::scale(TextureTransform, glm::vec3(Size.x * ScaleX, Size.y, 1.0f));

	glUniformMatrix4fv(LastProgram->TextureTransformID, 1, GL_FALSE, glm::value_ptr(TextureTransform));
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// Draw image from a texture atlas
void _Graphics::DrawAtlasTexture(const _Bounds &Bounds, const ae::_Texture *Texture, const glm::vec4 &TextureCoords) {
	SetVBO(VBO_QUAD_UV);
	SetTextureID(Texture->ID);

	// Get size
	glm::vec2 Size = Bounds.End - Bounds.Start;

	// Model transform
	glm::mat4 Transform(1.0f);
	Transform[3][0] = Bounds.Start.x;
	Transform[3][1] = Bounds.Start.y;
	Transform[0][0] = Size.x;
	Transform[1][1] = Size.y;
	glUniformMatrix4fv(LastProgram->ModelTransformID, 1, GL_FALSE, glm::value_ptr(Transform));

	// Texture transform
	glm::mat4 TextureTransform(1.0f);
	TextureTransform[3][0] = TextureCoords[0];
	TextureTransform[3][1] = TextureCoords[1];
	TextureTransform[0][0] = TextureCoords[2] - TextureCoords[0];
	TextureTransform[1][1] = TextureCoords[3] - TextureCoords[1];
	glUniformMatrix4fv(LastProgram->TextureTransformID, 1, GL_FALSE, glm::value_ptr(TextureTransform));
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// Draw image from texture array
void _Graphics::DrawTextureArray(const _Bounds &Bounds, const ae::_TextureArray *Texture, uint32_t Index) {
	SetVBO(VBO_QUAD_UV);
	SetTextureID(Texture->ID, GL_TEXTURE_2D_ARRAY);
	LastProgram->SetUniformFloat("texture_index", Index);

	// Get size
	glm::vec2 Size = Bounds.End - Bounds.Start;

	// Model transform
	glm::mat4 Transform(1.0f);
	Transform[3][0] = Bounds.Start.x;
	Transform[3][1] = Bounds.Start.y;
	Transform[0][0] = Size.x;
	Transform[1][1] = Size.y;
	glUniformMatrix4fv(LastProgram->ModelTransformID, 1, GL_FALSE, glm::value_ptr(Transform));
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// Draw double-sided flat wall
void _Graphics::DrawWall(const glm::vec3 &Position, const glm::vec3 &Scale, float Rotation, const ae::_Texture *Texture) {
	SetVBO(VBO_CUBE);
	SetTextureID(Texture->ID);

	glm::mat4 ModelTransform(1.0f);
	glm::mat4 TextureTransform(1.0f);
	int Offset;
	if(Rotation == 0) {
		ModelTransform = glm::translate(ModelTransform, glm::vec3(Position.x, Position.y + 0.5f, Position.z));
		TextureTransform[0][0] = Scale.x;
		TextureTransform[1][1] = Scale.z;
		Offset = 12;
	}
	else {
		ModelTransform = glm::translate(ModelTransform, glm::vec3(Position.x + 0.5f, Position.y, Position.z));
		TextureTransform[0][0] = Scale.y;
		TextureTransform[1][1] = Scale.z;
		Offset = 8;
	}
	ModelTransform = glm::scale(ModelTransform, Scale);

	glUniformMatrix4fv(LastProgram->ModelTransformID, 1, GL_FALSE, glm::value_ptr(ModelTransform));
	glUniformMatrix4fv(LastProgram->TextureTransformID, 1, GL_FALSE, glm::value_ptr(TextureTransform));

	glDrawArrays(GL_TRIANGLE_STRIP, Offset, 4);
}

// Draw 3d wall
void _Graphics::DrawCube(const glm::vec3 &Start, const glm::vec3 &Scale, const ae::_Texture *Texture) {
	SetVBO(VBO_CUBE);
	SetTextureID(Texture->ID);

	glm::mat4 ModelTransform(1.0f);
	ModelTransform = glm::translate(ModelTransform, Start);
	ModelTransform = glm::scale(ModelTransform, Scale);
	glUniformMatrix4fv(LastProgram->ModelTransformID, 1, GL_FALSE, glm::value_ptr(ModelTransform));

	glm::mat4 TextureTransform(1.0f);

	// Draw top
	TextureTransform[0][0] = Scale.x;
	TextureTransform[1][1] = Scale.y;
	glUniformMatrix4fv(LastProgram->TextureTransformID, 1, GL_FALSE, glm::value_ptr(TextureTransform));
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// Draw front
	TextureTransform[0][0] = Scale.x;
	TextureTransform[1][1] = Scale.z;
	glUniformMatrix4fv(LastProgram->TextureTransformID, 1, GL_FALSE, glm::value_ptr(TextureTransform));
	glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);

	// Draw left
	TextureTransform[0][0] = Scale.y;
	TextureTransform[1][1] = Scale.z;
	glUniformMatrix4fv(LastProgram->TextureTransformID, 1, GL_FALSE, glm::value_ptr(TextureTransform));
	glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);

	// Draw back
	TextureTransform[0][0] = Scale.x;
	TextureTransform[1][1] = Scale.z;
	glUniformMatrix4fv(LastProgram->TextureTransformID, 1, GL_FALSE, glm::value_ptr(TextureTransform));
	glDrawArrays(GL_TRIANGLE_STRIP, 12, 4);

	// Draw right
	TextureTransform[0][0] = Scale.y;
	TextureTransform[1][1] = Scale.z;
	glUniformMatrix4fv(LastProgram->TextureTransformID, 1, GL_FALSE, glm::value_ptr(TextureTransform));
	glDrawArrays(GL_TRIANGLE_STRIP, 16, 4);
}

// Vertical texture
void _Graphics::DrawWallDecal(const glm::vec3 &Position, const ae::_Texture *Texture, float Rotation, const glm::vec2 &Scale) {
	SetVBO(VBO_SPRITE);
	SetTextureID(Texture->ID);

	glm::mat4 ModelTransform;
	ModelTransform = glm::translate(glm::mat4(1.0f), Position);
	if(Rotation != 0.0f)
		ModelTransform = glm::rotate(ModelTransform, glm::radians(Rotation), glm::vec3(0, 0, 1));

	ModelTransform = glm::rotate(ModelTransform, glm::radians(90.0f), glm::vec3(1, 0, 0));
	ModelTransform = glm::scale(ModelTransform, glm::vec3(Scale.x, Scale.y, 0.0f));

	glUniformMatrix4fv(LastProgram->ModelTransformID, 1, GL_FALSE, glm::value_ptr(ModelTransform));

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
