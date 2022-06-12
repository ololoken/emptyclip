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
#include <color.h>
#include <SDL_video.h>
#include <SDL_opengl.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

// Forward Declarations
class _Texture;
class _Element;
class _Bounds;

enum VertexBufferType {
	VBO_CIRCLE,
	VBO_QUAD,
	VBO_CUBE,
	VBO_COUNT
};

// Classes
class _Graphics {

	public:

		_Graphics() { Enabled = false; }

		void Init(int WindowWidth, int WindowHeight, int Vsync, int MSAA, bool Fullscreen);
		void Close();

		void ToggleFullScreen();
		void ShowCursor(bool Show);
		void BuildVertexBuffers();

		void ChangeViewport(const glm::ivec2 &Size);
		void Setup2DProjectionMatrix();
		void Setup3DViewport();

		void FadeScreen(float Amount);
		void DrawImage(const glm::ivec2 &CenterPoint, const _Texture *Texture, const glm::vec4 &Color);
		void DrawImage(const _Bounds &Bounds, const _Texture *Texture, const glm::vec4 &Color, bool Stretch=false);
		void DrawRectangle(const _Bounds &Bounds, const glm::vec4 &Color, bool Filled=false);
		void DrawMask(const _Bounds &Bounds);

		void DrawTexture(const glm::vec3 &Position, const _Texture *Texture, const glm::vec4 &Color, float Rotation=0.0f, const glm::vec2 &Scale=glm::vec2(1.0f));
		void DrawRepeatable(const glm::vec3 &Start, const glm::vec3 &End, const _Texture *Texture, float Rotation, float ScaleX);
		void DrawCube(const glm::vec3 &Position, const glm::vec3 &Scale, const _Texture *Texture);
		void DrawWall(const glm::vec3 &Position, const glm::vec3 &Scale, float Rotation, const _Texture *Texture);
		void DrawRectangle(const glm::vec2 &Start, const glm::vec2 &End, const glm::vec4 &Color, bool Filled=false);
		void DrawLine(const glm::vec2 &Start, const glm::vec2 &End);
		void DrawCircle(const glm::vec3 &Position, float Radius);

		_Element *GetElement();

		void SetDepthMask(bool Value);
		void EnableDepthTest();
		void DisableDepthTest();
		void EnableStencilTest();
		void DisableStencilTest();
		void EnableParticleBlending();
		void DisableParticleBlending();
		void ClearScreen();
		void Flip(double FrameTime);

		GLuint CreateVBO(float *Triangles, GLuint Size);
		void EnableVBO(int Type);
		void DisableVBO(int Type);

		void SetColor(const glm::vec4 &Color);
		void SetTextureEnabled(bool Value);
		void SetTextureID(GLuint TextureID);

		// Viewport
		glm::ivec2 CurrentSize;
		glm::ivec2 ViewportSize;
		float AspectRatio;

		// Benchmarking
		int FramesPerSecond;

	private:

		void SetupOpenGL();

		// Data structures
		bool Enabled;
		SDL_Window *Window;
		SDL_GLContext Context;
		_Element *Element;

		// Vertex buffers
		GLuint VertexBuffer[VBO_COUNT];

		// State changes
		bool LastTextureEnabled;
		GLuint LastTextureID;
		glm::vec4 LastColor;

		// Benchmarking
		int TriangleCount;
		double FrameRateTimer;
		int FrameCount;
};

extern _Graphics Graphics;
