/******************************************************************************
* Empty Clip
* Copyright (C) 2025 Alan Witkowski
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
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <string>

// Forward Declarations
struct _ParticleSpawn;
namespace ae {
	class _Animation;
	class _Camera;
	class _Font;
	class _Texture;
}

// Classes
class _Particle {

	public:

		_Particle(const _ParticleSpawn &Spawn);
		~_Particle();

		void Update(double FrameTime);
		void Render(const ae::_Camera *Camera, double BlendFactor);

		// Attributes
		int Type{0};
		double Lifetime{0.0};
		bool Deleted{false};

		// Graphics
		const ae::_Texture *Texture{nullptr};
		const ae::_Font *Font{nullptr};
		ae::_Animation *Animation{nullptr};
		std::string Text;
		glm::vec4 Color{1.0f};
		glm::vec2 Scale{1.0f, 1.0f};
		float Rotation{0.0f};
		float AlphaSpeed{0.0f};
		float PositionZ{0.0f};
		float ScaleAspect{1.0f};
		int Side{0};

		// Physics
		glm::vec2 LastPosition{0.0f};
		glm::vec2 Position{0.0f};
		glm::vec2 Velocity{0.0f};
		glm::vec2 Acceleration{0.0f};
		float TurnSpeed{0.0f};

};
