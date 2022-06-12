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
		void DrawImage(const glm::ivec2 &CenterPoint, const _Texture *Texture, const _Color &Color);
		void DrawImage(const _Bounds &Bounds, const _Texture *Texture, const _Color &Color, bool Stretch=false);
		void DrawRectangle(const _Bounds &Bounds, const _Color &Color, bool Filled=false);
		void DrawMask(const _Bounds &Bounds);

		void DrawTexture(float X, float Y, float Z, const _Texture *Texture, const _Color &Color, float Rotation=0.0f, float ScaleX=1.0f, float ScaleY=1.0f);
		void DrawRepeatable(float StartX, float StartY, float StartZ, float EndX, float EndY, float EndZ, const _Texture *Texture, float Rotation, float ScaleX);
		void DrawCube(float StartX, float StartY, float StartZ, float ScaleX, float ScaleY, float ScaleZ, const _Texture *Texture);
		void DrawWall(float StartX, float StartY, float StartZ, float ScaleX, float ScaleY, float ScaleZ, float Rotation, const _Texture *Texture);
		void DrawRectangle(float StartX, float StartY, float EndX, float EndY, const _Color &Color, bool Filled=false);
		void DrawLine(float StartX, float StartY, float EndX, float EndY, const _Color &Color, float Z=0.0f);
		void DrawCircle(float X, float Y, float Z, float Radius, const _Color &Color);

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

		void SetColor(const _Color &Color);
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
		_Color LastColor;

		// Benchmarking
		int TriangleCount;
		double FrameRateTimer;
		int FrameCount;
};

extern _Graphics Graphics;
