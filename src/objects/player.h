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

		void Reset();

		bool IsMelee() const;

		void Render(double BlendFactor) override;
		void Render2D(const glm::ivec2 &Position);

		void Update(double FrameTime) override;
		void UpdateAnimation(double FrameTime, bool PlaySound=true) override;
		void UpdateExperience(int64_t ExperienceGained) override;
		void UpdateReloading();
		void UpdateWeaponSwitch();
		void UpdateSpeed(float Factor) override;
		void UpdateKillCount(int Value) override { MonsterKills += Value; }
		void UpdateSkill(int Index, int Value);
		void StartReloading();
		void CancelReloading();
		void StartWeaponSwitch(int SlotFrom, int SlotTo);
		int SpentSkillPoints() const;
		void ResetAccuracy(bool CompleteReset);
		void RecalculateStats();
		void Respawn();

		int AddItem(_Item *Item, int &AmountAdded);
		void DropItem(int Slot);
		void SwapInventory(int SwapFrom, int SwapTo);
		bool CanEquipItem(_Item *Item, int Slot);

		int AddInventory(_Item *Item);
		int CombineItems(_Item *FromItem, _Item *ToItem);
		bool AddMod(int FromIndex, int ToIndex);
		bool UseItem(int Index, bool Event);
		bool UseMedkit(int Index);
		int FindItem(int Index);
		int FindItem(const std::string &ID);
		void ResetUseTimer() { UseTimer = 0; }
		void ConsumeInventory(int Index, bool Delete=true);
		int ReduceAmmo(int Amount) override;
		bool WeaponHasAmmo(int AttackType) const override;
		bool HasAmmoForMain() const;
		bool HasMainHand() const { return GetMainHand() != nullptr; }
		bool HasOffHand() const { return GetOffHand() != nullptr; }
		bool HasMelee() const { return GetMelee() != nullptr; }
		bool HasArmor() const { return GetArmor() != nullptr; }
		bool HasInventory(int Index) const { return Index >= 0 && Inventory[Index] != nullptr; }
		bool CanAttack(int AttackType) const override { return AttackAllowed[AttackType] && !IsMeleeAttacking() && !Reloading && !SwitchingWeapons && !IsDying(); }
		bool CanPickup() const { return !IsDying() && CanUse(); }
		bool CanUse() const { return UseTimer > UsePeriod; }
		bool CanDropItem() const { return !Reloading && !SwitchingWeapons; }
		bool CanSwitchWeapons() const { return !SwitchingWeapons && !Reloading && !IsMeleeAttacking() && !IsDying(); }
		bool CanReload() const;

		void SetColorID(const std::string &ColorID) { this->ColorID = ColorID; UpdateColor(); }
		void SetCrouching(bool State);
		void SetSprinting(bool State);

		double GetReloadPercent() const { return std::min(1.0, ReloadTimer / ReloadPeriod); }
		double GetWeaponSwitchPercent() const { return std::min(1.0, WeaponSwitchTimer / WeaponSwitchPeriod); }
		float GetCrosshairRadius(const glm::vec2 &Cursor);
		const _ParticleTemplate *GetWeaponParticle(int Index) const override;
		_Item *GetMainHand() const { return Inventory[INVENTORY_MAINHAND]; }
		_Item *GetOffHand() const { return Inventory[INVENTORY_OFFHAND]; }
		_Item *GetMelee() const { return Inventory[INVENTORY_MELEE]; }
		_Item *GetArmor() const  { return Inventory[INVENTORY_ARMOR]; }
		int GetFireRate(int AttackType) const { return FireRate[AttackType]; }
		int GetInventoryMaxStack() const;
		const std::string &GetSound(int SoundType, int AttackType) const override;

		void AdjustLegDirection(float Destination);
		void SetLegAnimationPlayMode(int Mode) override;

		// Map
		std::string MapID;
		int CheckpointIndex;
		int Progression;

		// Saves
		std::string SavePath;

		// Animation
		ae::_Animation *LegAnimation;
		std::string ColorID;
		float LegDirection;
		bool Crouching;
		bool Sprinting;

		// Inventory
		_Item *Inventory[INVENTORY_SIZE];
		std::unordered_map<std::string, int> Ammo;
		std::unordered_map<std::string, int> AmmoMax;
		bool UseRequested;
		bool MedkitRequested;
		int WeaponSwitchFrom;
		int WeaponSwitchTo;

		// Character information
		double PlayingTimer;
		int MonsterKills;
		int TimePlayed;
		int64_t Gold;
		int64_t Experience;
		int64_t ExperienceNextLevel;
		int64_t ExperienceNeeded;

		// Skills
		int Skills[SKILL_COUNT];
		int SkillPointsRemaining;

		// Attacking
		float CurrentAccuracyNormal;
		float MinAccuracyNormal;
		float MaxAccuracyNormal;
		float ZoomScale;
		double WeaponSwitchTimer;
		double ReloadTimer;
		double UseTimer;
		double MedkitTimer;
		double WeaponSwitchPeriod;
		double ReloadPeriod;
		double UsePeriod;
		int FireRate[WEAPONATTACK_COUNT];
		bool Reloading;
		bool SwitchingWeapons;

		// Sounds
		const ae::_AudioSource *ReloadSound;

	private:

		bool IsBagIndex(int Index) { return Index >= INVENTORY_BAGSTART && Index < INVENTORY_BAGEND; }
		bool IsEquipmentIndex(int Index) { return Index <= INVENTORY_BAGSTART; }
		bool IsHandIndex(int Index) { return Index == INVENTORY_MAINHAND || Index == INVENTORY_OFFHAND; }

		void SetAnimationPlaybackSpeedFactor() override;
		void CalculateExperienceStats();
		void CalculateSkillsRemaining();
		void UpdateColor();

		void IncurDeathPenalty() override;
		void ResetWeaponAnimation();

		bool CanUseMedkit() const;
		void DeleteItems();

};
