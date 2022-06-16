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
#include <coord.h>
#include <color.h>
#include <texture.h>
#include <value.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <glm/vec2.hpp>

// Forward Declarations
class _Buffer;
class _Map;

// Object class
class _Object {

	public:

		// Types of objects in the game
		enum ObjectType {
			UNDEFINED=-1,
			PLAYER,
			MONSTER,
			KEY,
			AMMO,
			UPGRADE,
			WEAPON,
			ARMOR,
			MEDKIT,
			COUNT
		};

		_Object();
		virtual ~_Object();

		virtual void Update(double FrameTime) { }
		virtual void Render(double BlendFactor) { }
		virtual void Serialize(_Buffer &Buffer) { }
		void FacePosition(const glm::vec2 &Cursor);

		virtual const std::string &GetName() const { return Name; }

		void SetPosition(const glm::vec2 &NewPosition);
		glm::vec2 GetDirectionVector(float RotationOffset = 0.0f) const;

		virtual std::string GetTypeAsString() const { return "Object"; }

		// Attributes
		std::string Name;
		std::string ID;
		bool Active;
		int Type;

		// Map
		_Map *Map;
		bool TileChanged;

		// Physics
		glm::vec2 Position;
		glm::vec2 LastPosition;
		glm::vec2 Direction;
		float Radius;
		int WallState;

		// Graphics
		const _Texture *Texture;
		glm::vec4 Color;
		float Rotation;
		float Scale;
		float PositionZ;

		std::unordered_map<std::string, _Value> Attributes;
};
