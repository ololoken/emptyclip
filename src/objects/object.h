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
	class _Sound;
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
			PROJECTILE,
			COUNT
		};

		_Object(const _ObjectTemplate &ObjectTemplate);
		virtual ~_Object() {}

		virtual void Update(double FrameTime);
		virtual void Render(double BlendFactor);
		virtual void Serialize(ae::_Buffer &Buffer) {}
		bool IsDying() const { return Action == ACTION_DYING || Action == ACTION_STARTDEATH; }
		bool CanFreePath() const { return FreePathing || FreePathingTimer > 0.0; }

		virtual const _ParticleTemplate *GetParticle(int ParticleType) const { return nullptr; }
		const ae::_Sound *GetSound(int SoundType) const;

		void GetRenderBounds(glm::vec4 &Bounds);
		void FacePosition(const glm::vec2 &Target);
		void SetPosition(const glm::vec2 &NewPosition);
		void GetDrawPosition(glm::vec2 &DrawPosition, double BlendFactor) { DrawPosition = Position * (float)BlendFactor + LastPosition * (float)(1.0 - BlendFactor); }
		glm::vec2 GetDirectionVector(float RotationOffset = 0.0f) const;
		float RayIntersection(const glm::vec2 &Origin, const glm::vec2 &Direction) const;
		bool IsTouchingCircle(const glm::vec2 &CircleCenter, float CircleRadius, float &DistanceSquared) const;
		void CheckProjectileCollisions();

		virtual std::string GetTypeAsString() const { return "Object"; }
		void SetAttributeRange(const std::string &AttributeName, float Multiplier);
		void SetAttributeLevel(const std::string &AttributeName, float Multiplier);
		void SetAttributeSpread(const std::string &AttributeName, float Multiplier);
		float GetAttributeLevel(const std::string &AttributeName, float Multiplier, int MaxLevel=0);
		void GetAttributeRange(const std::string &AttributeName, float Multiplier, int &Min, int &Max);
		void SetMaxMods(bool RandomStats=false);

		// Template
		const _ObjectTemplate &Template;

		// Attributes
		std::unordered_map<std::string, _Value> Attributes;
		_Object *Owner{nullptr};
		std::string Name;
		std::string ID;
		int Type{NONE};
		int Level{1};
		bool Active{true};

		// Projectiles
		std::unordered_map<_Object *, int> HitObjects;
		float ProjectilePenetrationDamage{0.0f};
		float ProjectileExplosionSize{0.0f};
		int ProjectileMinDamage{0};
		int ProjectileMaxDamage{0};
		int ProjectileCritChance{0};
		int ProjectileCritDamage{0};
		int Depth{0};

		// Character
		ActionType Action{ACTION_IDLE};

		// Map
		_Map *Map{nullptr};
		bool TileChanged{false};

		// Physics
		glm::vec2 Position{0.0f};
		glm::vec2 LastPosition{0.0f};
		glm::vec2 Direction{0.0f, 1.0f};
		glm::vec2 Velocity{0.0f};
		double FreePathingTimer{0.0};
		float Radius{0.25f};
		bool Circle{true};
		bool FreePathing{false};

		// Graphics
		const ae::_Texture *Texture{nullptr};
		const ae::_Mesh *Mesh{nullptr};
		glm::vec4 Color{1.0f};
		float Rotation{0.0f};
		float Scale{1.0f};
		float PositionZ{0.0f};

};
