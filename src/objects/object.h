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
struct _Hit;
struct _Unique;
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
	AI_GHOST,
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
			CONSUMABLE,
			USABLE,
			PROP,
			PROJECTILE,
			COUNT
		};

		_Object(const _ObjectTemplate &ObjectTemplate);
		virtual ~_Object() {}

		virtual void Update(double FrameTime);
		virtual void Render(double BlendFactor) const;
		virtual void RenderLights(double BlendFactor, float Alpha);
		virtual void Serialize(ae::_Buffer &Buffer) {}

		bool IsDying() const { return Action == ACTION_DYING || Action == ACTION_STARTDEATH; }
		virtual bool IsInvulnerable() const { return false; }
		bool IsCrate() const { return AIType == AI_NONE; }
		bool CanFreePath() const { return FreePathing || FreePathingTimer > 0.0; }
		bool CanStack() const { return false; }
		bool CanMove() const { return Type == _Object::WEAPON || Type == _Object::ARMOR || Type == _Object::MOD || Type == _Object::USABLE; }
		bool CanEquip() const { return Type == _Object::WEAPON || Type == _Object::ARMOR; }
		bool CanMod() const { return Type == _Object::WEAPON || Type == _Object::ARMOR; }
		bool CanDynamite() const { return Type == _Object::WEAPON || Type == _Object::ARMOR || Type == _Object::MOD; }
		bool CanIncreaseQuality(bool CheckQuality) const;
		bool CanIncreaseLevel(bool CheckLevel) const;
		bool CanQuality() const { return Type == _Object::WEAPON || Type == _Object::ARMOR || Type == _Object::MOD || Type == _Object::USABLE; }
		bool CanLevel() const { return Type == _Object::WEAPON || Type == _Object::ARMOR; }
		bool CanShowMoreInfo() const { return Type == _Object::WEAPON || Type == _Object::ARMOR || Type == _Object::MOD || Type == _Object::USABLE; }
		bool CanUnique() const { return Type == _Object::WEAPON || Type == _Object::ARMOR || Type == _Object::MOD || Type == _Object::AMMO || Type == _Object::CONSUMABLE || Type == _Object::USABLE; }
		bool CanAutoPickup() const { return Type == _Object::AMMO || Type == _Object::KEY || Type == _Object::CONSUMABLE; }
		bool CanHide() const { return Type == _Object::AMMO || Type == _Object::CONSUMABLE; }

		virtual const _ParticleTemplate *GetParticle(int ParticleType) const { return nullptr; }
		const ae::_Sound *GetSound(int SoundType) const;

		void UpdateBounds();
		void FacePosition(const glm::vec2 &Target);
		void SetPosition(const glm::vec2 &NewPosition);
		void GetDrawPosition(glm::vec2 &DrawPosition, double BlendFactor) const { DrawPosition = Position * (float)BlendFactor + LastPosition * (float)(1.0 - BlendFactor); }
		glm::vec2 GetDirectionVector(float RotationOffset = 0.0f) const;
		float RayIntersection(const glm::vec2 &Origin, const glm::vec2 &Direction) const;
		bool IsTouchingCircle(const glm::vec2 &CircleCenter, float CircleRadius, float &DistanceSquared) const;

		virtual std::string GetTypeAsString() const { return "Object"; }
		void SetAttributeRange(const std::string &AttributeName, float Multiplier);
		void SetAttributeLevel(const std::string &AttributeName, float Multiplier);
		float GetAttributeLevel(const std::string &AttributeName, float Multiplier, int MaxLevel=0);
		void GetAttributeRange(const std::string &AttributeName, float Multiplier, int &Min, int &Max);

		void CreateAmmoPickup(float SpawnPositionZ);
		void CheckProjectileCollisions();
		void ApplyDamage(const _Hit &Hit);
		void ApplyForce(const glm::vec2 &ForceDirection, float Amount, bool LimitForce=false);

		// Template
		const _ObjectTemplate &Template;

		// Attributes
		std::unordered_map<std::string, _Value> Attributes;
		_Object *Owner{nullptr};
		const _Unique *Unique{nullptr};
		std::string Name;
		std::string ID;
		float ExtraMods{0.0f};
		int Type{NONE};
		int Quality{0};
		int Level{1};
		int FilterType{-1};
		bool Active{true};

		// Projectiles
		std::unordered_map<_Object *, int> HitObjects;
		const _ObjectTemplate *ProjectileWeaponTemplate{nullptr};
		float ProjectilePenetrationDamage{0.0f};
		float ProjectileExplosionSize{0.0f};
		float ProjectileForce{0.0f};
		int ProjectileMinDamage{0};
		int ProjectileMaxDamage{0};
		int ProjectileCritChance{0};
		int ProjectileCritDamage{0};
		int Bounces{0};
		int Depth{0};

		// Character
		ActionType Action{ACTION_IDLE};

		// Monsters
		int AIType{AI_NONE};

		// Map
		_Map *Map{nullptr};
		bool TileChanged{false};

		// Physics
		glm::vec2 Position{0.0f};
		glm::vec2 LastPosition{0.0f};
		glm::vec2 Direction{0.0f, 1.0f};
		glm::vec2 LastDirection{0.0f};
		glm::vec2 Velocity{0.0f};
		std::vector<int> GridTypes;
		double FreePathingTimer{0.0};
		float Radius{0.25f};
		float Mass{0.0f};
		bool Circle{true};
		bool FreePathing{false};
		bool Carry{true};

		// Graphics
		const ae::_Texture *Texture{nullptr};
		const ae::_Texture *LightTexture{nullptr};
		const ae::_Mesh *Mesh{nullptr};
		glm::vec4 Bounds{0.0f};
		glm::vec4 Color{1.0f};
		glm::vec4 LightColor{1.0f};
		glm::vec4 LightBounds{0.0f};
		glm::vec2 LightScale{1.0f};
		float Rotation{0.0f};
		float RotationSpeed{0.0f};
		float Scale{1.0f};
		float PositionZ{0.0f};
		bool Visible{true};
		bool AmmoPickup{false};
		bool Filtered{false};

};
