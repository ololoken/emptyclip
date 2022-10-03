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

		virtual void Update(double FrameTime) override;
		virtual void Render(double BlendFactor) override;
		virtual void RecalculateStats() {}

		void Move(double FrameTime);

		bool StartAttack();
		float GenerateShotDirection();
		void ApplyRecoil();
		bool IsRanged() const;
		bool IsMeleeAttacking() const { return Action == ACTION_MELEE || Action == ACTION_STARTMELEE; }
		bool IsCrate() const { return AIType == AI_NONE; }

		bool CheckBurstTimer(int AttackType) const { return AttackTimer[AttackType] >= BurstPeriod[AttackType]; }
		bool CheckAttackTimer(int AttackType) const { return AttackTimer[AttackType] >= AttackPeriod[AttackType]; }
		virtual bool CanAttack(int AttackType) const { return CheckAttackTimer(AttackType) && !IsMeleeAttacking() && !IsDying() && MainWeaponType != WEAPON_NONE; }
		virtual int ReduceAmmo(int Amount) { return Amount; }
		virtual bool WeaponHasAmmo(int AttackType) const { return true; }
		virtual int GetWeaponAmmo() const { return 0; }

		virtual void UpdateExperience(int64_t ExperienceGained) {}
		virtual void UpdateKillCount(int Value) {}
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
		virtual const char *GetWeaponID(int AttackType) { return nullptr; }

		float GetHealthPercentage() const { return (float)Health / MaxHealth; }
		float GetStaminaPercentage() const { return Stamina / MaxStamina; }
		float GetPoisonIntensity() const { return std::min(PoisonTimer, 1.0) / 2.0f; }

		virtual const ae::_Sound *GetSound(int SoundType, int AttackType) const;

		void StartTriggerDownAudio();
		void StopAudio();

		// Graphics
		ae::_Animation *Animation;
		glm::vec2 WeaponOffset[WEAPON_COUNT]{{0.0f, 0.0f}, {0.0f, 0.0f}};

		// Audio
		std::vector<const ae::_Sound *> Sounds[SOUND_COUNT];
		ae::_AudioSource *TriggerDownAudio{nullptr};
		double FireSoundTimer{0.0};

		// Movement
		glm::vec2 SpawnPosition{0.0f};
		MoveType MoveState{MOVE_NONE};
		float MoveSpeed{0.0f};
		float MoveModifier{1.0f};
		float Stamina{1.0f};
		float MaxStamina{1.0f};
		float BaseMoveSpeed{0.0f};
		int WallState{0};
		bool Tired{false};
		bool PositionChanged{false};

		// Stats
		int Health{0};
		int MaxHealth{0};
		int DamageBlock{0};
		float DamageResist{0.0f};

		// States
		int WalkingAnimation{ANIMATION_MOVE};
		int MeleeAnimation{ANIMATION_ATTACK};
		int ShootingOnehandAnimation{ANIMATION_ATTACK};
		int ShootingTwohandAnimation{ANIMATION_ATTACK};
		int DyingAnimation{ANIMATION_DIE};
		double InvulnerableTimer{0.0};
		double LastHitTimer{0.0};
		double PoisonTimer{0.0};

		// Attacking attributes
		float CurrentAccuracy{0.0f};
		float MinAccuracy{0.0f};
		float MaxAccuracy[WEAPONATTACK_COUNT]{0.0f, 0.0f};
		float Recoil{0.0f};
		float AccuracyRegen{0.0f};
		float RecoilModifier{1.0f};
		float MoveRecoil{0.0f};
		double ReloadDelay{0.0};
		float AttackRange[WEAPONATTACK_COUNT]{0.0f};
		double AttackTimer[WEAPONATTACK_COUNT]{0.0};
		double AttackPeriod[WEAPONATTACK_COUNT]{0.0};
		double ShootPeriod[WEAPONATTACK_COUNT]{0.0};
		float AttackWidth[WEAPONATTACK_COUNT]{0.0f};
		int MinDamage[WEAPONATTACK_COUNT]{0};
		int MaxDamage[WEAPONATTACK_COUNT]{0};
		int Penetration[WEAPONATTACK_COUNT]{1, 1};
		float PenetrationDamage[WEAPONATTACK_COUNT]{0.0f};
		float AttackMoveSpeed[WEAPONATTACK_COUNT]{0.0f};
		int AttackCount[WEAPONATTACK_COUNT]{1, 1};
		int FireAllRounds[WEAPONATTACK_COUNT]{0, 0};
		int CritChance[WEAPONATTACK_COUNT]{0};
		int CritDamage[WEAPONATTACK_COUNT]{100, 100};
		int BurstRounds[WEAPONATTACK_COUNT]{0};
		double BurstPeriod[WEAPONATTACK_COUNT]{0.0};
		const _ObjectTemplate *Projectiles[WEAPONATTACK_COUNT]{nullptr};
		float ProjectileSpeed[WEAPONATTACK_COUNT]{0.0f};
		float ExplosionSize[WEAPONATTACK_COUNT]{0.0f};
		float MeleeOffset[WEAPONATTACK_COUNT]{0.0f};
		float Push[WEAPONATTACK_COUNT]{0.0f};
		float Force[WEAPONATTACK_COUNT]{0.0f};
		bool MeleeSwitch[WEAPONATTACK_COUNT]{false};
		float PoisonPower{0.0f};
		int MainWeaponType{0};
		int AttackRequestType{0};
		int BurstRoundsShot{0};
		bool AttackRequested{false};
		bool AttackMade{false};

		// Monsters
		int64_t ExperienceGiven{0};
		glm::vec2 TargetPosition{0.0f};
		float TargetRadius{0.0f};

	protected:

		virtual void ApplyDeathPenalty() {}
		virtual void SetLegAnimationPlayMode(int Mode) {}
		virtual void SetAnimationPlaybackSpeedFactor() {}
		void RenderRadius(const glm::vec2 &DrawPosition);
		void UpdateRecoil(double FrameTime);

};
