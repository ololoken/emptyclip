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
#include <objects/entity.h>
#include <constants.h>
#include <map>

// Forward Declarations
class _Item;
class _Weapon;

// Enumerations
enum PlayerAnimationTypes {
	PLAYER_ANIMATIONWALKINGONEHAND,
	PLAYER_ANIMATIONWALKINGTWOHAND,
	PLAYER_ANIMATIONMELEE,
	PLAYER_ANIMATIONSHOOTONEHAND,
	PLAYER_ANIMATIONSHOOTTWOHAND,
	PLAYER_ANIMATIONDYING
};

enum InventoryTypes {
	INVENTORY_MAINHAND,
	INVENTORY_OFFHAND,
	INVENTORY_MELEE,
	INVENTORY_ARMOR,
	INVENTORY_BAGSTART,
	INVENTORY_BAGEND = INVENTORY_BAGSTART + INVENTORY_BAGSIZE,
	INVENTORY_SIZE = INVENTORY_BAGEND,
};

// Classes
class _Player : public _Entity {

	friend class _Save;

	public:

		_Player(const _ObjectTemplate &PlayerTemplate);
		~_Player() override;

		void Reset(bool Recalculate=false);

		bool IsMelee() const;

		void Render(double BlendFactor) override;
		void Render2D(const glm::ivec2 &DrawPosition);

		void Update(double FrameTime) override;
		void UpdateAnimation(double FrameTime, bool PlaySound=true) override;
		void UpdateExperience(int64_t ExperienceGained) override;
		void UpdateReloading();
		void UpdateWeaponSwitch();
		void UpdateSpeed(float Factor) override;
		void UpdateKillCount(int Value) override { TotalKills += Value; }
		void UpdateSkill(int Index, int Value);
		void StartReloading();
		void CancelReloading();
		void StartWeaponSwitch(int SlotFrom, int SlotTo);
		int SpentSkillPoints() const;
		void ResetAccuracy(bool CompleteReset);
		void RecalculateStats() override;
		void Respawn();

		int AddItem(_Item *Item, int &AmountAdded);
		void DropItem(int Slot);
		void SortInventory();
		void SwapInventory(int SwapFrom, int SwapTo);
		bool CanEquipItem(_Item *Item, int Slot);

		int AddInventory(_Item *Item);
		int CombineItems(_Item *FromItem, _Item *ToItem);
		bool AddMod(int FromIndex, int ToIndex);
		bool UseItem(int Index, bool Event);
		int FindItem(int Index);
		int FindItem(const std::string &ID);
		void ConsumeInventory(int Index, bool Delete=true);
		int ReduceAmmo(int Amount) override;
		bool WeaponHasAmmo(int AttackType) const override;
		int GetWeaponAmmo() const override;
		bool HasAmmoForMain() const;
		bool HasMainHand() const { return GetMainHand() != nullptr; }
		bool HasOffHand() const { return GetOffHand() != nullptr; }
		bool HasMelee() const { return GetMelee() != nullptr; }
		bool HasArmor() const { return GetArmor() != nullptr; }
		bool HasInventory(int Index) const { return Index >= 0 && Inventory[Index] != nullptr; }
		bool CanAttack(int AttackType) const override { return !IsMeleeAttacking() && !Reloading && !SwitchingWeapons && !IsDying(); }
		bool CanPickup() const { return !IsDying() && CanUse(); }
		bool CanUse() const { return UseTimer > UsePeriod; }
		bool CanDropItem() const { return !Reloading && !SwitchingWeapons; }
		bool CanSwitchWeapons() const { return !SwitchingWeapons && !Reloading && !IsMeleeAttacking() && !IsDying(); }
		bool CanReload() const;
		bool IsSteady() const override { return HasMainHand() && CurrentAccuracy <= MinAccuracy; }

		void SetColorID(const std::string &ColorID) { this->ColorID = ColorID; UpdateColor(); }
		void SetAiming(bool State);
		void SetSprinting(bool State);

		double GetReloadPercent() const { return std::min(1.0, ReloadTimer / ReloadPeriod); }
		double GetWeaponSwitchPercent() const { return std::min(1.0, WeaponSwitchTimer / WeaponSwitchPeriod); }
		float GetCrosshairRadius(const glm::vec2 &Cursor);
		const _ParticleTemplate *GetParticle(int ParticleType) const override;
		_Item *GetMainHand() const { return Inventory[INVENTORY_MAINHAND]; }
		_Item *GetOffHand() const { return Inventory[INVENTORY_OFFHAND]; }
		_Item *GetMelee() const { return Inventory[INVENTORY_MELEE]; }
		_Item *GetArmor() const  { return Inventory[INVENTORY_ARMOR]; }
		int GetInventoryMaxStack() const;
		const ae::_Sound *GetSound(int SoundType, int AttackType) const override;

		void AdjustLegDirection(float Destination);
		void SetLegAnimationPlayMode(int Mode) override;
		void OnHit(_Entity *Attacker, const _Hit &Hit) override;

		static bool IsBagIndex(int Index) { return Index >= INVENTORY_BAGSTART && Index < INVENTORY_BAGEND; }
		static bool IsEquipmentIndex(int Index) { return Index <= INVENTORY_BAGSTART; }
		static bool IsHandIndex(int Index) { return Index == INVENTORY_MAINHAND || Index == INVENTORY_OFFHAND; }

		// Map
		std::string MapID;
		int CheckpointIndex;
		int Progression;
		double Clock;

		// Saves
		std::string SavePath;

		// Animation
		ae::_Animation *LegAnimation;
		const ae::_Texture *MeleeTexture;
		glm::vec2 MeleeScale[WEAPONATTACK_COUNT];
		std::string ColorID;
		float LegDirection;
		bool Aiming;
		bool Sprinting;

		// Inventory
		_Item *Inventory[INVENTORY_SIZE];
		std::unordered_map<std::string, int> Ammo;
		std::unordered_map<std::string, int> AmmoMax;
		std::map<std::string, int> Keys;
		bool UseRequested;
		int WeaponSwitchFrom;
		int WeaponSwitchTo;
		bool Flashlight;

		// Character information
		double LevelTime;
		double ProgressionTime;
		double PlayTime;
		int TotalDeaths;
		int TotalKills;
		int ProgressionKills;
		int ProgressionDeaths;
		int ProgressionCrates;
		int ProgressionSecrets;
		int64_t Gold;
		int64_t Experience;
		int64_t ExperienceNextLevel;
		int64_t ExperienceNeeded;
		int64_t ExperienceLost;

		// Skills
		int Skills[SKILL_COUNT];
		int SkillPointsRemaining;
		int DropRate;
		double SelfHealStartTime;
		double SelfHealPeriod;
		double SelfHealTimer;
		float HealModifier;
		float PickupModifier;

		// Attacking
		float CurrentAccuracyNormal;
		float MinAccuracyNormal;
		float MaxAccuracyNormal;
		float ZoomScale;
		double WeaponSwitchTimer;
		double ReloadTimer;
		double UseTimer;
		double WeaponSwitchPeriod;
		double ReloadPeriod;
		double UsePeriod;
		int FireRateType[WEAPONATTACK_COUNT];
		bool Reloading;
		bool SwitchingWeapons;

		// Sounds
		const ae::_AudioSource *ReloadSound;

	private:

		void SetAnimationPlaybackSpeedFactor() override;
		void CalculateExperienceStats();
		void CalculateSkillsRemaining();
		void UpdateColor();

		void ApplyDeathPenalty() override;
		void ResetWeaponAnimation();

		void DeleteItems();

};
