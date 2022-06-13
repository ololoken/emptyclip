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

#include <opengl.h>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <string>

class _Shader;
struct _Light;

// Program
class _Program {

	static const int SAMPLER_COUNT = 16;

	public:

		_Program(const std::string &Name, const _Shader *VertexShader, const _Shader *FragmentShader, GLuint Attribs, int MaxLights);
		~_Program();

		void Use() const;
		void SetUniformFloat(const std::string &Name, float Value) const;
		void SetUniformVec2(const std::string &Name, const glm::vec2 &Value) const;
		void SetUniformVec4(const std::string &Name, const glm::vec4 &Value) const;
		void SetUniformMat4(const std::string &Name, const glm::mat4 &Value) const;
		void ResetTextureTransform();

		std::string Name;

		GLuint ID;
		GLint ViewProjectionTransformID;
		GLint ModelTransformID;
		GLint TextureTransformID;
		GLint ColorID;
		GLint AmbientLightID;
		GLint LightCountID;
		GLuint Attribs;

		int MaxLights;
		int LightCount;
		_Light *Lights;
		glm::vec4 AmbientLight;

	private:

		GLint SamplerIDs[SAMPLER_COUNT];

};

// Shader
class _Shader {

	public:

		_Shader(const std::string &Path, GLenum ProgramType);
		~_Shader();

		GLuint ID;

	private:

};
