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
#include <objects/item.h>
#include <objects/templates.h>
#include <list>

// Forward Declarations
struct _ParticleTemplate;
class _Map;
namespace ae {
	class _Animation;
	class _AudioSource;
	class _Sound;
}

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
		virtual void RecalculateStats() { }

		void Move(double FrameTime);

		bool StartAttack();
		float GenerateShotDirection();
		void ResetAttackAllowed(int AttackType) { AttackAllowed[AttackType] = false; AttackTimer[AttackType] = 0; }
		bool IsMeleeAttacking() const { return Action == ACTION_MELEE || Action == ACTION_STARTMELEE; }

		virtual bool CanAttack(int AttackType) const { return AttackAllowed[AttackType] && !IsMeleeAttacking() && !IsDying() && MainWeaponType != WEAPON_NONE; }
		virtual int ReduceAmmo(int Amount) { return Amount; }
		virtual bool WeaponHasAmmo(int AttackType) const { return true; }

		virtual void UpdateExperience(int64_t ExperienceGained) { }
		virtual void UpdateKillCount(int Value) { }
		virtual void UpdateAnimation(double FrameTime, bool PlaySound=true);

		void UpdateHealth(int Adjust);
		virtual void UpdateSpeed(float Factor) {}
		int GenerateDamage(int AttackType, int DamageBlock, int DamageResist);
		bool IsDying() const { return Action == ACTION_DYING || Action == ACTION_STARTDEATH; }
		bool IsDead() const { return Action == ACTION_DYING && !Active; }
		bool IsInvulnerable() const { return InvulnerableTimer > 0.0; }

		float GetHealthPercentage() const { return (float)Health / MaxHealth; }
		float GetStaminaPercentage() const { return Stamina / MaxStamina; }

		virtual const _ParticleTemplate *GetParticle(int ParticleType) const { return nullptr; }
		virtual const ae::_Sound *GetSound(int Type, int AttackType) const { return Sounds[Type]; }

		void StartTriggerDownAudio();
		void StopAudio();

		// Graphics
		ae::_Animation *Animation;
		glm::vec2 WeaponOffset[WEAPON_COUNT];

		// Audio
		const ae::_Sound *Sounds[SOUND_COUNT];
		ae::_AudioSource *TriggerDownAudio;

		// Movement
		MoveType MoveState;
		int BaseMoveSpeed;
		float MoveSpeed;
		float MoveModifier;
		bool PositionChanged;
		float Stamina;
		float MaxStamina;
		float StaminaRegenModifier;
		int WallState;
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
		double InvulnerableTimer;

		// Attacking attributes
		float CurrentAccuracy;
		float MinAccuracy;
		float MaxAccuracy[WEAPONATTACK_COUNT];
		float Recoil;
		float RecoilRegen;
		float RecoilModifier;
		float MoveRecoil;
		float AttackRange[WEAPONATTACK_COUNT];
		double AttackTimer[WEAPONATTACK_COUNT];
		double AttackPeriod[WEAPONATTACK_COUNT];
		float AttackWidth[WEAPONATTACK_COUNT];
		int MinDamage[WEAPONATTACK_COUNT];
		int MaxDamage[WEAPONATTACK_COUNT];
		int Penetration[WEAPONATTACK_COUNT];
		float AttackMoveSpeed[WEAPONATTACK_COUNT];
		int AttackCount[WEAPONATTACK_COUNT];
		int MainWeaponType;
		int AttackRequestType;
		bool AttackRequested;
		bool AttackAllowed[WEAPONATTACK_COUNT];
		bool AttackMade;

		// Monsters
		int64_t ExperienceGiven;
		glm::vec2 TargetPosition;

	protected:

		virtual void ApplyDeathPenalty() { }
		virtual void SetLegAnimationPlayMode(int Mode) { }
		virtual void SetAnimationPlaybackSpeedFactor() { }
		void UpdateRecoil(double FrameTime);

};
