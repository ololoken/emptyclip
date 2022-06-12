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
#include <vector2.h>
#include <color.h>
#include <string>

// Forward Declarations
class _Font;
class _Texture;
struct _ParticleSpawn;

// Classes
class _Particle {

	public:

		_Particle(const _ParticleSpawn &Spawn);
		~_Particle();

		void Update(double FrameTime);
		void Render();

		// Attributes
		int Type;
		double Lifetime;
		bool Deleted;

		// Graphics
		const _Texture *Texture;
		const _Font *Font;
		std::string Text;
		_Color Color;
		Vector2 Scale;
		float Rotation;
		float AlphaSpeed;
		float PositionZ;
		float ScaleAspect;

		// Physics
		Vector2 Position;
		Vector2 Velocity;
		Vector2 Acceleration;
		float TurnSpeed;

};
