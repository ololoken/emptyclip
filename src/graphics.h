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
#pragma once

// Libraries
#include <ae/opengl.h>
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>
#include <SDL_video.h>
#include <string>

// Forward Declarations
namespace ae {
	class _Texture;
	class _TextureArray;
}
struct SDL_Cursor;
class _Program;
class _Element;
struct _Bounds;

enum VertexBufferType {
	VBO_NONE,
	VBO_LINE,
	VBO_CIRCLE,
	VBO_QUAD,
	VBO_RECT,
	VBO_SPRITE,
	VBO_ATLAS,
	VBO_QUAD_UV,
	VBO_CUBE,
	VBO_COUNT
};

struct _WindowSettings {
	_WindowSettings() : Size(0), Position(0), MSAA(0), Fullscreen(false), Vsync(false) { }
	std::string WindowTitle;
	glm::ivec2 Size;
	glm::ivec2 Position;
	int MSAA;
	bool Fullscreen;
	bool Vsync;
};

enum CursorType {
	CURSOR_NONE,
	CURSOR_MAIN,
	CURSOR_CROSS,
	CURSOR_COUNT,
};

// Classes
class _Graphics {

	public:

		void Init(const _WindowSettings &WindowSettings);
		void Close();

		GLuint CreateVBO(float *Vertices, GLsizeiptr Size, GLenum Type);

		void ResetState();
		void CheckError();
		void Setup2D();
		void Setup3D();

		void FadeScreen(const _Program *Program, float Amount);
		void ClearScreen();
		void Flip(double FrameTime);

		void SetViewport(const glm::ivec2 &Size);
		void SetWindowSize(const glm::ivec2 &Size);
		bool SetFullscreen(bool Fullscreen);
		void SetStaticUniforms();
		bool SetVsync(bool Vsync);
		bool GetVsync();
		void SetCursor(int Type);
		void SetVBO(GLuint Type);
		void SetAttribLevel(GLuint AttribLevel);
		void SetColor(const glm::vec4 &Color);
		void SetTextureID(GLuint TextureID, GLenum Type=GL_TEXTURE_2D);
		void SetVertexBufferID(GLuint VertexBufferID);
		void SetProgram(const _Program *Program);
		void SetDepthTest(bool DepthTest);
		void SetCullFace(bool Value);
		void SetScissor(const _Bounds &Bounds);
		void SetDepthMask(bool Value);
		void EnableStencilTest();
		void DisableStencilTest();
		void EnableScissorTest();
		void DisableScissorTest();
		void EnableParticleBlending();
		void DisableParticleBlending();

		void DrawLine(const glm::vec2 &Start, const glm::vec2 &End);
		void DrawRectangle(const _Bounds &Bounds, bool Filled=false);
		void DrawRectangle(const glm::vec2 &Start, const glm::vec2 &End, bool Filled=false);
		void DrawRectangle3D(const glm::vec2 &Start, const glm::vec2 &End, bool Filled);
		void DrawCircle(const glm::vec3 &Position, float Radius);
		void DrawMask(const _Bounds &Bounds);

		void DrawImage(const _Bounds &Bounds, const ae::_Texture *Texture, bool Stretch=false);
		void DrawScaledImage(const glm::vec2 &Position, const ae::_Texture *Texture, const glm::vec2 &Size, const glm::vec4 &Color=glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
		void DrawSprite(const glm::vec3 &Position, const ae::_Texture *Texture, float Rotation=0.0f, const glm::vec2 &Scale=glm::vec2(1.0f));
		void DrawAnimationFrame(const glm::vec3 &Position, const ae::_Texture *Texture, const glm::vec4 &TextureCoords, float Rotation=0.0f, const glm::vec2 &Scale=glm::vec2(1.0f));
		void DrawAtlasTexture(const _Bounds &Bounds, const ae::_Texture *Texture, const glm::vec4 &TextureCoords);
		void DrawTextureArray(const _Bounds &Bounds, const ae::_TextureArray *Texture, uint32_t Index);
		void DrawRepeatable(const glm::vec3 &Start, const glm::vec3 &End, const ae::_Texture *Texture, float Rotation, float ScaleX);
		void DrawWall(const glm::vec3 &Position, const glm::vec3 &Scale, float Rotation, const ae::_Texture *Texture);
		void DrawCube(const glm::vec3 &Start, const glm::vec3 &Scale, const ae::_Texture *Texture);
		void DrawWallDecal(const glm::vec3 &Position, const ae::_Texture *Texture, float Rotation=0.0f, const glm::vec2 &Scale=glm::vec2(1.0f));

		_Element *Element;
		glm::ivec2 CurrentSize;
		glm::ivec2 ViewportSize;
		glm::mat4 Ortho;
		float AspectRatio;
		GLfloat Anisotropy;
		int FramesPerSecond;

	private:

		void BuildVertexBuffers();
		void SetupOpenGL();

		// Vertex buffers
		int CircleVertices;
		GLuint VertexArrayID;
		GLuint VertexBuffer[VBO_COUNT];

		// Data structures
		SDL_Window *Window;
		SDL_GLContext Context;
		SDL_Cursor *Cursors[CURSOR_COUNT];

		// Sizes
		glm::ivec2 WindowSize;
		glm::ivec2 FullscreenSize;

		// State changes
		GLuint LastVertexBufferID;
		GLuint LastTextureID;
		GLuint LastAttribLevel;
		const _Program *LastProgram;
		bool LastDepthTest;

		// Benchmarking
		double FrameRateTimer;
		int FrameCount;
};

extern _Graphics Graphics;
