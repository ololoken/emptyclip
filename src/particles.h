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
#include <vector>

// Forward Declarations
class _Map;
class _Particle;
struct _ParticleTemplate;
namespace ae {
	class _Camera;
}

struct _ParticleSpawn {
	_ParticleSpawn(const _ParticleTemplate *Template, const glm::vec4 &Color, const glm::vec2 &Normal, const glm::vec2 &Position, float PositionZ, float RotationAdjust) :
		Template(Template),
		Color(Color),
		Normal(Normal),
		Position(Position),
		PositionZ(PositionZ),
		RotationAdjust(RotationAdjust) {}

	const _ParticleTemplate *Template;
	std::string Text;
	glm::vec4 Color;
	glm::vec2 Normal;
	glm::vec2 Position;
	float PositionZ;
	float RotationAdjust;
};

// Manages particles
class _Particles {

	public:

		enum ParticleTypes {
			NORMAL,
			EMISSIVE,
			FLOOR_DECALS,
			WALL_DECALS,
			TEXT,
			EMISSIVE_ANIMATION,
			COUNT,
		};

		_Particles();
		~_Particles();

		// Updates
		void Update(double FrameTime);
		void Render(int Type, double BlendFactor);

		// Management
		void Add(_Particle *Particle);
		bool Create(const _ParticleSpawn &Spawn);
		void Clear();

		// Objects
		std::vector<_Particle *> Particles;
		const ae::_Camera *Camera{nullptr};
		_Map *Map{nullptr};

	private:

		// Rendering
		std::vector<_Particle *> RenderList[COUNT];

};
