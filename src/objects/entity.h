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
class _Map;
struct _Hit;
namespace ae {
	class _Animation;
	class _AudioSource;
	class _Sound;
}

// Classes
class _Entity : public _Object {

	public:

		_Entity(const _ObjectTemplate &EntityTemplate);
		~_Entity() override;

		virtual void Render(double BlendFactor) override;
		virtual void RecalculateStats() { }

		void Move(double FrameTime);

		bool StartAttack();
		float GenerateShotDirection();
		bool IsMeleeAttacking() const { return Action == ACTION_MELEE || Action == ACTION_STARTMELEE; }
		bool IsCrate() const { return AIType == AI_NONE; }

		bool CheckBurstTimer(int AttackType) const { return AttackTimer[AttackType] >= BurstPeriod[AttackType]; }
		bool CheckAttackTimer(int AttackType) const { return AttackTimer[AttackType] >= AttackPeriod[AttackType]; }
		virtual bool CanAttack(int AttackType) const { return CheckAttackTimer(AttackType) && !IsMeleeAttacking() && !IsDying() && MainWeaponType != WEAPON_NONE; }
		virtual int ReduceAmmo(int Amount) { return Amount; }
		virtual bool WeaponHasAmmo(int AttackType) const { return true; }

		virtual void UpdateExperience(int64_t ExperienceGained) { }
		virtual void UpdateKillCount(int Value) { }
		virtual void UpdateAnimation(double FrameTime, bool PlaySound=true);

		void UpdateHealth(int Adjust);
		virtual void OnAttack(_Entity *Victim, const _Hit &Hit);
		virtual void OnHit(_Entity *Attacker, const _Hit &Hit);
		virtual void UpdateSpeed(float Factor);
		int GenerateDamage(int AttackType, float DamageModifier, bool Steady, bool &Crit);
		int ReduceDamage(int Damage);
		virtual bool IsSteady() const { return false; }
		bool IsDead() const { return Action == ACTION_DYING && !Active; }
		bool IsInvulnerable() const { return InvulnerableTimer > 0.0; }

		float GetHealthPercentage() const { return (float)Health / MaxHealth; }
		float GetStaminaPercentage() const { return Stamina / MaxStamina; }

		virtual const ae::_Sound *GetSound(int SoundType, int AttackType) const;

		void StartTriggerDownAudio();
		void StopAudio();

		// Graphics
		ae::_Animation *Animation;
		glm::vec2 WeaponOffset[WEAPON_COUNT];

		// Audio
		std::vector<const ae::_Sound *> Sounds[SOUND_COUNT];
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
		int WalkingAnimation;
		int MeleeAnimation;
		int ShootingOnehandAnimation;
		int ShootingTwohandAnimation;
		int DyingAnimation;
		double InvulnerableTimer;
		double LastHitTimer;

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
		float PenetrationDamage[WEAPONATTACK_COUNT];
		float AttackMoveSpeed[WEAPONATTACK_COUNT];
		int AttackCount[WEAPONATTACK_COUNT];
		int CritChance[WEAPONATTACK_COUNT];
		int CritDamage[WEAPONATTACK_COUNT];
		int BurstRounds[WEAPONATTACK_COUNT];
		double BurstPeriod[WEAPONATTACK_COUNT];
		const _ObjectTemplate *Projectiles[WEAPONATTACK_COUNT];
		float ProjectileSpeed[WEAPONATTACK_COUNT];
		int MainWeaponType;
		int AttackRequestType;
		int BurstRoundsShot;
		bool AttackRequested;
		bool AttackMade;

		// Monsters
		int64_t ExperienceGiven;
		glm::vec2 TargetPosition;
		int AIType;

	protected:

		virtual void ApplyDeathPenalty() { }
		virtual void SetLegAnimationPlayMode(int Mode) { }
		virtual void SetAnimationPlaybackSpeedFactor() { }
		void UpdateRecoil(double FrameTime);

};
