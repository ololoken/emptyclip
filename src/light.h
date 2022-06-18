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

#include <ae/opengl.h>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

// Light
struct _Light {
	_Light() : PositionID(-1), ColorID(-1), RadiusID(-1), Position(0.0f, 0.0f, 0.0f), Color(1.0f), Radius(1.0f) { }

	GLint PositionID;
	GLint ColorID;
	GLint RadiusID;
	glm::vec3 Position;
	glm::vec4 Color;
	float Radius;
};
