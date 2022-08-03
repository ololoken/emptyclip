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
#include <string>
#include <glm/vec2.hpp>

// Forward Declarations
namespace ae {
	class _Camera;
	class _Texture;
	class _Font;
}
struct _ParticleSpawn;

// Classes
class _Particle {

	public:

		_Particle(const _ParticleSpawn &Spawn);
		~_Particle();

		void Update(double FrameTime);
		void Render(const ae::_Camera *Camera);

		// Attributes
		int Type;
		double Lifetime;
		bool Deleted = false;

		// Graphics
		const ae::_Texture *Texture;
		const ae::_Font *Font;
		std::string Text;
		glm::vec4 Color;
		glm::vec2 Scale;
		float Rotation;
		float AlphaSpeed;
		float PositionZ;
		float ScaleAspect;

		// Physics
		glm::vec2 Position;
		glm::vec2 Velocity;
		glm::vec2 Acceleration;
		float TurnSpeed;

};
