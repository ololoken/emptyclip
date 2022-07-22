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
#include <value.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <glm/vec2.hpp>

// Forward Declarations
class _Map;
struct _ObjectTemplate;
struct _ParticleTemplate;
namespace ae {
	class _Buffer;
	class _Texture;
	class _Mesh;
}

// Used for animation
enum ActionType {
	ACTION_IDLE,
	ACTION_MOVING,
	ACTION_STARTMELEE,
	ACTION_MELEE,
	ACTION_STARTSHOOT,
	ACTION_SHOOT,
	ACTION_STARTDEATH,
	ACTION_DYING,
};

// Used to determine what direction an entity wants to go
enum MoveType {
	MOVE_NONE,
	MOVE_FORWARD,
	MOVE_LEFT,
	MOVE_RIGHT,
	MOVE_BACKWARD,
	MOVE_FORWARDRIGHT,
	MOVE_FORWARDLEFT,
	MOVE_BACKWARDRIGHT,
	MOVE_BACKWARDLEFT,
	MOVE_TARGET,
};

enum WeaponAttackType {
	WEAPONATTACK_MAIN,
	WEAPONATTACK_MELEE,
	WEAPONATTACK_COUNT,
};

// Indices into the animation array
enum EntityAnimationTypes {
	ANIMATION_MOVE,
	ANIMATION_ATTACK,
	ANIMATION_DIE,
};

// Types of AI
enum AITypes {
	AI_NONE,
	AI_SIMPLE,
	AI_BOSS,
	AI_BASIC,
	AI_HITANDRUN,
	AI_COUNT
};

// Object class
class _Object {

	public:

		// Types of objects in the game
		enum ObjectType {
			NONE,
			PLAYER,
			MONSTER,
			WEAPON,
			ARMOR,
			JEWELRY,
			MOD,
			KEY,
			AMMO,
			MEDKIT,
			PROP,
			COUNT
		};

		_Object(const _ObjectTemplate &ObjectTemplate);
		virtual ~_Object() { }

		virtual void Update(double FrameTime) { }
		virtual void Render(double BlendFactor);
		virtual void Serialize(ae::_Buffer &Buffer) { }
		bool IsDying() const { return Action == ACTION_DYING || Action == ACTION_STARTDEATH; }

		virtual const _ParticleTemplate *GetParticle(int ParticleType) const { return nullptr; }

		void GetRenderBounds(glm::vec4 &Bounds);
		void FacePosition(const glm::vec2 &Target);
		void SetPosition(const glm::vec2 &NewPosition);
		glm::vec2 GetDirectionVector(float RotationOffset = 0.0f) const;
		float RayIntersection(const glm::vec2 &Origin, const glm::vec2 &Direction) const;
		bool IsTouchingCircle(const glm::vec2 &CircleCenter, float CircleRadius, float &DistanceSquared) const;

		virtual std::string GetTypeAsString() const { return "Object"; }
		void SetAttributeRange(const std::string &AttributeName, float Multiplier);
		void SetAttributeLevel(const std::string &AttributeName, float Multiplier);
		void SetAttributeSpread(const std::string &AttributeName, float Multiplier);
		float GetAttributeLevel(const std::string &AttributeName, float Multiplier);
		void GetAttributeRange(const std::string &AttributeName, float Multiplier, int &Min, int &Max);
		void SetMaxMods(bool RandomStats=false);

		// Template
		const _ObjectTemplate &Template;

		// Attributes
		std::unordered_map<std::string, _Value> Attributes;
		std::string Name;
		std::string ID;
		int Type;
		int Level;
		bool Active;

		// Character
		ActionType Action;

		// Map
		_Map *Map;
		bool TileChanged;

		// Physics
		glm::vec2 Position;
		glm::vec2 LastPosition;
		glm::vec2 Direction;
		float Radius;
		bool Circle;
		bool FreePathing;

		// Graphics
		const ae::_Texture *Texture;
		const ae::_Mesh *Mesh;
		glm::vec4 Color;
		float Rotation;
		float Scale;
		float PositionZ;

};
