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
#include <program.h>
#include <graphics.h>
#include <light.h>
#include <utils.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>

// Load a program from two shaders
_Program::_Program(const std::string &Name, const _Shader *VertexShader, const _Shader *FragmentShader, GLuint Attribs, int MaxLights) :
	Name(Name),
	ViewProjectionTransformID(-1),
	ModelTransformID(-1),
	TextureTransformID(-1),
	ColorID(-1),
	AmbientLightID(-1),
	LightCountID(-1),
	Attribs(Attribs),
	MaxLights(MaxLights),
	LightCount(0),
	Lights(nullptr),
	AmbientLight(1.0f) {

	// Create program
	ID = glCreateProgram();
	glAttachShader(ID, VertexShader->ID);
	glAttachShader(ID, FragmentShader->ID);
	glLinkProgram(ID);

	// Check the program
	GLint Result = GL_FALSE;
	glGetProgramiv(ID, GL_LINK_STATUS, &Result);
	if(!Result) {

		// Get error message length
		GLint ResultLength;
		glGetProgramiv(ID, GL_INFO_LOG_LENGTH, &ResultLength);

		// Get message
		std::string ErrorMessage((std::size_t)ResultLength, 0);
		glGetProgramInfoLog(ID, ResultLength, nullptr, (GLchar *)&ErrorMessage[0]);

		throw std::runtime_error(ErrorMessage);
	}

	// Setup lights
	if(MaxLights)
		Lights = new _Light[MaxLights]();

	// Set attrib locations
	glBindAttribLocation(ID, 0, "vertex_pos");
	glBindAttribLocation(ID, 1, "vertex_uv");
	glBindAttribLocation(ID, 2, "vertex_norm");

	// Get uniforms
	for(int i = 0; i < SAMPLER_COUNT; i++)
		SamplerIDs[i] = glGetUniformLocation(ID, std::string("sampler" + std::to_string(i)).c_str());

	ViewProjectionTransformID = glGetUniformLocation(ID, "view_projection_transform");
	ModelTransformID = glGetUniformLocation(ID, "model_transform");
	TextureTransformID = glGetUniformLocation(ID, "texture_transform");
	ColorID = glGetUniformLocation(ID, "color");
	AmbientLightID = glGetUniformLocation(ID, "ambient_light");
	LightCountID = glGetUniformLocation(ID, "light_count");

	for(int i = 0; i < MaxLights; i++) {
		std::string LightPositionName = "lights[" + std::to_string(i) + "].position";
		std::string LightColorName = "lights[" + std::to_string(i) + "].color";
		std::string LightRadiusName = "lights[" + std::to_string(i) + "].radius";
		Lights[i].PositionID = glGetUniformLocation(ID, LightPositionName.c_str());
		Lights[i].ColorID = glGetUniformLocation(ID, LightColorName.c_str());
		Lights[i].RadiusID = glGetUniformLocation(ID, LightRadiusName.c_str());
	}
}

// Destructor
_Program::~_Program() {
	delete[] Lights;
	glDeleteProgram(ID);
}

// Enable the program
void _Program::Use() const {
	glUseProgram(ID);

	// Set uniforms
	for(int i = 0; i < SAMPLER_COUNT; i++) {
		if(SamplerIDs[i] != -1)
			glUniform1i(SamplerIDs[i], i);
	}

	if(AmbientLightID != -1)
		glUniform4fv(AmbientLightID, 1, &AmbientLight[0]);

	if(LightCountID != -1)
		glUniform1i(LightCountID, LightCount);

	for(int i = 0; i < LightCount; i++) {
		glUniform3fv(Lights[i].PositionID, 1, &Lights[i].Position[0]);
		glUniform4fv(Lights[i].ColorID, 1, &Lights[i].Color[0]);
		glUniform1fv(Lights[i].RadiusID, 1, &Lights[i].Radius);
	}

	if(TextureTransformID)
		glUniformMatrix4fv(TextureTransformID, 1, GL_FALSE, glm::value_ptr(glm::mat4(1)));
}

// Set the value of a float uniform
void _Program::SetUniformFloat(const std::string &Name, float Value) const {
	glUniform1f(glGetUniformLocation(ID, Name.c_str()), Value);
}

// Set the value of a vec2 uniform
void _Program::SetUniformVec2(const std::string &Name, const glm::vec2 &Value) const {
	glUniform2fv(glGetUniformLocation(ID, Name.c_str()), 1, &Value[0]);
}

// Set the value of a vec4 uniform
void _Program::SetUniformVec4(const std::string &Name, const glm::vec4 &Value) const {
	glUniform4fv(glGetUniformLocation(ID, Name.c_str()), 1, &Value[0]);
}

// Set the value of a mat4 uniform
void _Program::SetUniformMat4(const std::string &Name, const glm::mat4 &Value) const {
	glUniformMatrix4fv(glGetUniformLocation(ID, Name.c_str()), 1, GL_FALSE, glm::value_ptr(Value[0]));
}

// Reset texture transform
void _Program::ResetTextureTransform() {
	if(TextureTransformID)
		glUniformMatrix4fv(TextureTransformID, 1, GL_FALSE, glm::value_ptr(glm::mat4(1)));
}

// Loads a shader
_Shader::_Shader(const std::string &Path, GLenum ProgramType) {

	// Load program from file
	const char *ShaderSource = LoadFileIntoMemory(Path.c_str());
	if(!ShaderSource)
		throw std::runtime_error("Failed to load shader file: " + Path);

	// Create the shader
	ID = glCreateShader(ProgramType);

	// Compile shader
	glShaderSource(ID, 1, &ShaderSource, nullptr);
	delete[] ShaderSource;

	glCompileShader(ID);

	// Check for errors
	GLint Result = GL_FALSE;
	glGetShaderiv(ID, GL_COMPILE_STATUS, &Result);
	if(!Result) {

		// Get error message length
		GLint ResultLength;
		glGetShaderiv(ID, GL_INFO_LOG_LENGTH, &ResultLength);

		// Get message
		std::string ErrorMessage((std::size_t)ResultLength, 0);
		glGetShaderInfoLog(ID, ResultLength, nullptr, (GLchar *)&ErrorMessage[0]);

		throw std::runtime_error("Error in " + Path + '\n' + ErrorMessage);
	}
}

// Destructor
_Shader::~_Shader() {
	glDeleteShader(ID);
}
