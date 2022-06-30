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
#include <objects/object.h>
#include <objects/templates.h>
#include <objects/weapon.h>
#include <list>

// Forward Declarations
namespace ae {
	class _Animation;
	class _AudioSource;
}
struct _ParticleTemplate;
class _Map;

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

// Indices into the animation array
enum EntityAnimationTypes {
	ENTITY_ANIMATIONWALKING,
	ENTITY_ANIMATIONATTACK,
	ENTITY_ANIMATIONDYING,
};

// Classes
class _Entity : public _Object {

	public:

		_Entity(const _ObjectTemplate &EntityTemplate);
		~_Entity() override;

		virtual void Update(double FrameTime) override;
		virtual void Render(double BlendFactor) override;

		void Move(double FrameTime);

		bool StartAttack();
		float GenerateShotDirection();
		void ResetAttackAllowed(int AttackType) { AttackAllowed[AttackType] = false; FireTimer[AttackType] = 0; }
		bool IsMeleeAttacking() const { return Action == ACTION_MELEE || Action == ACTION_STARTMELEE; }

		virtual bool CanAttack(int AttackType) const { return AttackAllowed[AttackType] && !IsMeleeAttacking() && !IsDying() && GetWeaponType() != WEAPON_NONE; }
		virtual void ReduceAmmo() { }
		virtual bool WeaponHasAmmo() const { return true; }

		virtual void UpdateExperience(int64_t ExperienceGained) { }
		virtual void UpdateKillCount(int Value) { }
		virtual void UpdateAnimation(double FrameTime, bool PlaySound=true);

		void UpdateMaxHealth(int Adjust);
		void UpdateHealth(int Adjust);
		virtual void UpdateSpeed(float Factor) {}
		int GenerateDamage(int AttackType, int DamageBlock, int DamageResist);
		bool IsDying() const { return Action == ACTION_DYING || Action == ACTION_STARTDEATH; }
		bool IsDead() const { return Action == ACTION_DYING && !Active; }

		void SetMoveState(MoveType State);

		float GetHealthPercentage() const { return (float)Health / MaxHealth; }
		float GetStaminaPercentage() const { return Stamina / MaxStamina; }
		int GetWeaponType() const { return MainWeaponType; }
		glm::vec2 GetWeaponOffset(int WeaponType) const { return WeaponParticleOffset[WeaponType]; }
		float GetWeaponRange(int AttackType) const { return AttackRange[AttackType]; }
		float GetMaxAccuracy(int AttackType) const { return MaxAccuracy[AttackType]; }
		int GetMinDamage(int Type) const { return MinDamage[Type]; }
		int GetMaxDamage(int Type) const { return MaxDamage[Type]; }

		virtual const _ParticleTemplate *GetWeaponParticle(int Index) const { return nullptr; }
		virtual const std::string &GetSound(int Type, int AttackType) const { return Sounds[Type]; }

		void StartTriggerDownAudio();
		void StopAudio();

		// Graphics
		ae::_Animation *Animation;
		glm::vec2 WeaponParticleOffset[WEAPON_TYPES];

		// Audio
		std::string Sounds[SOUND_TYPES];
		ae::_AudioSource *TriggerDownAudio;

		// Movement
		MoveType MoveState;
		float MovementSpeed;
		float MovementModifier;
		bool PositionChanged;
		float Stamina;
		float MaxStamina;
		float StaminaRegenModifier;
		bool Tired;

		// Stats
		int Health;
		int MaxHealth;
		int DamageBlock;
		int DamageResist;

		// States
		ActionType Action;
		int WalkingAnimation;
		int MeleeAnimation;
		int ShootingOnehandAnimation;
		int ShootingTwohandAnimation;
		int DyingAnimation;

		// Attacking attributes
		float CurrentAccuracy;
		float MinAccuracy;
		float MaxAccuracy[WEAPONATTACK_COUNT];
		float AccuracyModifier;
		float Recoil;
		float RecoilRegen;
		float AttackRange[WEAPONATTACK_COUNT];
		double FireTimer[WEAPONATTACK_COUNT];
		double FirePeriod[WEAPONATTACK_COUNT];
		int MinDamage[WEAPONATTACK_COUNT];
		int MaxDamage[WEAPONATTACK_COUNT];
		int AttackCount;
		int MainWeaponType;
		bool AttackRequested;
		bool AttackAllowed[WEAPONATTACK_COUNT];
		bool AttackMade;
		int AttackRequestType;

		// Monsters
		int64_t ExperienceGiven;
		glm::vec2 TargetPosition;

	protected:

		virtual void IncurDeathPenalty() { }
		virtual void SetLegAnimationPlayMode(int Mode) { }
		virtual void SetAnimationPlaybackSpeedFactor() { }
		void UpdateRecoil(double FrameTime);

};
