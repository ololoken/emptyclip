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
class _Animation;
class _Item;
class _Weapon;
class _Armor;
class _Buffer;
struct _Point;

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
	INVENTORY_ARMOR,
	INVENTORY_MAINHAND,
	INVENTORY_OFFHAND,
	INVENTORY_MELEE,
	INVENTORY_BAGSTART,
	INVENTORY_BAGEND = INVENTORY_BAGSTART + INVENTORY_BAGSIZE,
	INVENTORY_SIZE = INVENTORY_BAGEND,
};

// Classes
class _Player : public _Entity {

	public:

		_Player(const std::string &SavePath);
		~_Player() override;

		void Reset();
		void Load();
		void Save();

		bool IsMelee() const;
		bool IsSwitchingWeapons() const { return SwitchingWeapons; }
		bool IsReloading() const { return Reloading; }

		void Render(double BlendFactor) override;
		void Render2D(const _Point &Position);

		void Update(double FrameTime) override;
		void UpdateAnimation(double FrameTime) override;
		void UpdateExperience(int64_t ExperienceGained) override;
		void UpdateReloading();
		void UpdateWeaponSwitch();
		void UpdateSpeed(float Factor) override;
		void UpdateLevel();
		void UpdateKillCount(int Value) override { MonsterKills += Value; }
		void UpdateSkill(int Index, int Value);
		void StartReloading();
		void CancelReloading();
		void StartWeaponSwitch(int SlotFrom, int SlotTo);
		int SpentSkillPoints() const;
		void ResetAccuracy(bool CompleteReset);
		void RecalculateStats();

		int AddItem(_Item *Item);
		void DropItem(int Slot);
		void SwapInventory(int SwapFrom, int SwapTo);
		bool CanEquipItem(_Item *Item, int Slot);

		int AddInventory(_Item *Item);
		int CombineItems(_Item *FromItem, _Item *ToItem);
		bool AddComponent(int FromIndex, int ToIndex);
		bool UseItem(int Index, bool Event);
		bool UseMedkit(int Index);
		int FindMiscItem(int Index);
		int FindItem(const std::string &Identifier);
		void ResetUseTimer() { UseTimer = 0; }
		void ConsumeInventory(int Index, bool Delete=true);
		void ReduceAmmo() override;
		bool HasAmmo() const override;
		bool HasClips() const;
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

		void SetColorIdentifier(const std::string &ColorIdentifier) { this->ColorIdentifier = ColorIdentifier; UpdateColor(); }
		void SetTorsoAnimation(const _Animation *Animation);
		void SetLegAnimation(const _Animation *Animation);
		void SetCrouching(bool State);
		void SetSprinting(bool State);
		void SetUseRequested(bool Use) { UseRequested = Use; }
		void SetMedkitRequested(bool Medkit) { MedkitRequested = Medkit; }
		void SetExprience(int64_t Experience) { this->Experience = Experience; }
		void SetSkill(int Index, int Value) { Skills[Index] = Value; }

		double GetReloadPercent() const { return std::min(1.0, ReloadTimer / ReloadPeriod); }
		double GetWeaponSwitchPercent() const { return std::min(1.0, WeaponSwitchTimer / WeaponSwitchPeriod); }
		int GetMedkitHealAmount(int MedkitLevel) const;
		float GetCrosshairRadius(const Vector2 &Cursor);
		int GetWeaponAmmoType() const;
		int GetInventoryAmmoType(int Index) const;
		const _ParticleTemplate *GetWeaponParticle(int Index) const override;
		void SetMainHand(_Weapon *Weapon);
		void SetOffHand(_Weapon *Weapon);
		void SetMelee(_Weapon *Weapon);
		void SetArmor(_Armor *Armor);
		_Weapon *GetMainHand() const { return (_Weapon *)Inventory[INVENTORY_MAINHAND]; }
		_Weapon *GetOffHand() const { return (_Weapon *)Inventory[INVENTORY_OFFHAND]; }
		_Weapon *GetMelee() const { return (_Weapon *)Inventory[INVENTORY_MELEE]; }
		_Armor *GetArmor() const  { return (_Armor *)Inventory[INVENTORY_ARMOR]; }
		_Item *GetInventory(int Index)  { return Inventory[Index]; }
		float GetZoomScale() const { return ZoomScale; }
		int GetFireRate(int AttackType) const { return FireRate[AttackType]; }
		int64_t GetExperience() const { return Experience; }
		int64_t GetExperienceNextLevel() const { return ExperienceNextLevel; }
		int GetMonsterKills() const { return MonsterKills; }
		int GetTimePlayed() const { return TimePlayed; }
		float GetLevelPercentage() const { return LevelPercentage; }
		int GetSkill(int Index) const { return Skills[Index]; }
		int GetSkillPointsRemaining() const { return SkillPointsRemaining; }
		int GetInventoryMaxStack() const;
		bool IsCrouching() const { return Crouching; }
		bool IsSprinting() const { return Sprinting; }
		bool GetUseRequested() const { return UseRequested; }
		bool GetMedkitRequested() const { return MedkitRequested; }
		const std::string &GetSample(int Type) const override;

		void AdjustLegDirection(float Destination);
		void SetLegAnimationPlayMode(int Mode) override;
		void SetLegDirection(float Rotation) { LegDirection = Rotation; }

		void SetCheckpointIndex(int CheckpointIndex) { this->CheckpointIndex = CheckpointIndex; }
		int GetCheckpointIndex() const { return CheckpointIndex; }

		void SetMapIdentifier(const std::string &MapIdentifier) { this->MapIdentifier = MapIdentifier; }
		const std::string &GetMapIdentifier() const { return MapIdentifier; }

	private:

		bool IsBagIndex(int Index) { return Index >= INVENTORY_BAGSTART && Index < INVENTORY_BAGEND; }
		bool IsEquipmentIndex(int Index) { return Index <= INVENTORY_BAGSTART; }
		bool IsHandIndex(int Index) { return Index == INVENTORY_MAINHAND || Index == INVENTORY_OFFHAND; }

		void LoadItems(_Buffer &Buffer);
		void LoadWeapon(_Buffer &Buffer, int Count, int InventoryIndex);
		void LoadUpgrades(_Buffer &Buffer, _Weapon *Weapon);
		void SaveItems(std::ofstream &File);

		void SetAnimationPlaybackSpeedFactor() override;
		void CalculateLevelPercentage();
		void CalculateExperienceStats();
		void CalculateSkillsRemaining();
		void UpdateColor();

		void IncurDeathPenalty() override;
		void ResetWeaponAnimation();

		bool CanUseMedkit() const;
		bool IsRightClip(const _Item *Item) const;
		void DeleteItems();

		// Map
		std::string MapIdentifier;
		int CheckpointIndex;
		int Progression;

		// Engine
		std::string SavePath;
		int DebugLevel;

		// Animation
		_Animation *LegAnimation;
		std::string ColorIdentifier;
		float LegDirection;
		bool Crouching;
		bool Sprinting;

		// Inventory
		_Item *Inventory[INVENTORY_SIZE];
		bool UseRequested;
		bool MedkitRequested;
		bool Used;
		bool MedkitUsed;
		int WeaponSwitchFrom;
		int WeaponSwitchTo;

		// Character information
		float LevelPercentage;
		double PlayingTimer;
		int MonsterKills;
		int TimePlayed;
		int64_t Gold;
		int64_t Experience;
		int64_t ExperienceNextLevel;
		int64_t ExperienceCurrentLevel;

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
};
