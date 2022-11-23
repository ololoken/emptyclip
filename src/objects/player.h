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
#include <objects/inventory.h>
#include <map>

// Forward Declarations
class _Item;
class _Weapon;
class _Inventory;
struct _Bag;
struct _Slot;

// Enumerations
enum PlayerAnimationTypes {
	PLAYER_ANIMATIONWALKINGONEHAND,
	PLAYER_ANIMATIONWALKINGTWOHAND,
	PLAYER_ANIMATIONMELEE,
	PLAYER_ANIMATIONSHOOTONEHAND,
	PLAYER_ANIMATIONSHOOTTWOHAND,
	PLAYER_ANIMATIONDYING
};

enum AddResultTypes {
	ADD_FULL,
	ADD_QUIETFULL,
	ADD_REMOVE,
	ADD_DELETE,
};

// Classes
class _Player : public _Entity {

	friend class _Save;

	public:

		_Player(const _ObjectTemplate &PlayerTemplate);
		~_Player() override;

		void ResetAchievementTracking();

		bool IsDead() const { return Action == ACTION_DYING && !Active; }
		bool IsMelee() const;
		const char *GetWeaponID(int AttackType) override;

		void Render(double BlendFactor) const override;
		void Render2D(const glm::ivec2 &DrawPosition);

		void Update(double FrameTime) override;
		void UpdateAnimation(double FrameTime, bool PlaySound=true) override;
		void UpdateExperience(int64_t ExperienceGained) override;
		void UpdateReloading();
		void UpdateWeaponSwitch();
		void UpdateOutfitSwitch();
		void UpdateSpeed(float Factor) override;
		void UpdateKillCount(int Value) override { TotalKills += Value; }
		void UpdateSkill(int Index, int Value);

		void StartReloading();
		void CancelReloading();
		void StartWeaponSwitch(const _Slot &SlotFrom, const _Slot &SlotTo);
		void StartOutfitSwitch(size_t Outfit);
		int SpentSkillPoints() const;
		void ResetAccuracy(bool CompleteReset);
		void RecalculateStats(bool SoftReset=false) override;
		void Respawn();
		void WarpPosition(const glm::vec2 &NewPosition);

		int AddItem(_Item *Item, int &AmountAdded, bool UseOnFull);
		void DropItem(const _Slot &Slot, const glm::vec2 &DropPosition=glm::vec2(-1.0f));
		void SortInventory();
		void SwapInventory(const _Slot &SwapFrom, const _Slot &SwapTo);
		bool CanEquipItem(const _Item *Item, size_t Slot) const;
		int AddItemToBackpack(_Item *Item, size_t StartingBagIndex, bool OneBagOnly=false);
		int CombineItems(_Item *FromItem, _Item *ToItem);
		bool AddMod(const _Slot &SlotFrom, const _Slot &SlotTo);
		bool ApplyUsable(const _Slot &SlotFrom, const _Slot &SlotTo);
		void ConsumeInventory(const _Slot &Slot, bool Delete=true);
		int GetPickupAmount(const _Item *Item) const;
		int ReduceAmmo(int Amount) override;
		bool WeaponHasAmmo(int AttackType) const override;
		int GetWeaponAmmo() const override;
		_Bag &GetActiveOutfitBag() const;
		_Bag &GetActiveBackpackBag() const;
		void GetEquippedCompareSlot(const _Item *Item, bool Offhand, _Slot &Slot);
		void UpdateAmmoNeeded();
		void AddMissingBackpacks();
		bool HasAmmoForMain() const;

		bool CanAttack(int AttackType) const override { return !IsMeleeAttacking() && !Reloading && !IsSwitching() && !IsDying(); }
		bool CanPickup() const { return !IsDying() && CanUse(); }
		bool CanUse() const { return UseTimer > UsePeriod; }
		bool CanDropItems() const { return !Reloading && !IsSwitching(); }
		bool CanDragItems() const { return !Reloading && !IsSwitching(); }
		bool CanEquipItems() const { return !Reloading && !IsSwitching(); }
		bool CanSwitchWeapons() const { return !IsSwitching() && !IsMeleeAttacking() && !IsDying(); }
		bool CanSwitchOutfits() const { return !IsSwitching() && !IsMeleeAttacking() && !IsDying(); }
		bool CanReload() const;
		bool IsSteady() const override { return GetMainHand() && CurrentAccuracy <= MinAccuracy; }
		bool IsSwitching() const { return SwitchingWeapons || SwitchingOutfits; }
		bool InCombat() const { return CombatTimer < GAME_COMBAT_TIMER; }
		bool ApplyUse() const { return UseRequested && CanUse(); }
		void RequestAttack(int RequestType);

		void SetColorID(const std::string &ColorID) { this->ColorID = ColorID; UpdateColor(); }
		void SetAiming(bool State);
		void SetSprinting(bool State);

		double GetReloadPercent() const { return std::min(1.0, ReloadTimer / ReloadPeriod); }
		double GetWeaponSwitchPercent() const { return std::min(1.0, WeaponSwitchTimer / WeaponSwitchPeriod); }
		double GetOutfitSwitchPercent() const { return std::min(1.0, OutfitSwitchTimer / OutfitSwitchPeriod); }
		float GetCrosshairRadius(const glm::vec2 &Cursor);
		const _ParticleTemplate *GetParticle(int ParticleType) const override;
		_Item *GetMainHand() const;
		_Item *GetOffHand() const;
		_Item *GetMelee() const;
		_Item *GetArmor() const;
		int GetInventoryMaxStack() const;
		const ae::_Sound *GetSound(int SoundType, int AttackType) const override;

		void AdjustLegDirection(float Destination);
		void SetLegAnimationPlayMode(int Mode) override;
		void OnHit(_Entity *Attacker, const _Hit &Hit, bool PlaySound) override;
		bool PlayEquipSound(size_t Slot) const;

		// Map
		std::string MapID{GAME_FIRSTLEVEL};
		glm::ivec2 LastGoodCoord{0};
		int CheckpointIndex{0};
		int Progression{1};
		double Clock{GAME_DEFAULT_CLOCK};

		// Saves
		std::string SavePath;
		bool TestSave{false};

		// Animation
		ae::_Animation *LegAnimation;
		const ae::_Texture *MeleeTexture{nullptr};
		glm::vec2 MeleeScale[WEAPONATTACK_COUNT];
		std::string ColorID{"white"};
		float LegDirection{0.0f};
		bool Aiming{false};
		bool Sprinting{false};

		// UI
		int Filters[FILTER_COUNT];
		int LastFilters[FILTER_COUNT];

		// Inventory
		_Inventory *Inventory;
		std::unordered_map<std::string, int> Ammo;
		std::unordered_map<std::string, int> AmmoMax;
		std::vector<bool> AmmoNeeded;
		std::map<std::string, int> Keys;
		bool UseRequested{false};
		_Slot WeaponSwitchFrom;
		_Slot WeaponSwitchTo;
		size_t OutfitSwitchTo{0};
		bool Flashlight{false};
		uint64_t ActiveOutfit{0};
		uint64_t ActiveBackpack{0};

		// Character information
		double LevelTime{0.0};
		double ProgressionTime{0.0};
		double PlayTime{0.0};
		int TotalDeaths{0};
		int64_t TotalKills{0};
		int64_t ProgressionKills{0};
		int ProgressionDeaths{0};
		int ProgressionCrates{0};
		int ProgressionSecrets{0};
		int LavaTouches{0};
		bool Hardcore{false};
		bool Stat100Percent{false};
		bool StatLoneWolf{false};
		bool StatFistsOnly{false};
		int64_t Experience{0};
		int64_t ExperienceNextLevel{0};
		int64_t ExperienceNeeded{0};
		int64_t ExperienceLost{0};

		// Skills
		int Skills[SKILL_COUNT];
		int SkillPointsRemaining{0};
		int DropRate{100};
		double SelfHealStartTime;
		double SelfHealPeriod;
		double SelfHealTimer{PLAYER_HEAL_STARTTIME};
		float SelfHealPercent;
		float HealModifier{1.0f};
		float PickupModifier{1.0f};
		float ExperienceModifier{1.0f};

		// Attacking
		float CurrentAccuracyNormal;
		float MinAccuracyNormal;
		float MaxAccuracyNormal;
		float ZoomScale{PLAYER_ZOOMSCALE};
		double WeaponSwitchTimer{0.0};
		double OutfitSwitchTimer{0.0};
		double ReloadTimer{0.0};
		double UseTimer{0.0};
		double WeaponSwitchPeriod;
		double OutfitSwitchPeriod;
		double ReloadPeriod;
		double UsePeriod{PLAYER_USEPERIOD};
		int FireRateType[WEAPONATTACK_COUNT];
		bool Reloading{false};
		bool SwitchingWeapons{false};
		bool SwitchingOutfits{false};

		// Sounds
		const ae::_AudioSource *ReloadSound{nullptr};
		double TakeDamageSoundTimer{0.0};

	private:

		void SetAnimationPlaybackSpeedFactor() override;
		void CalculateExperienceStats();
		void CalculateSkillsRemaining();
		void UpdateColor();

		void ApplyDeathPenalty() override;
		void ResetWeaponAnimation();

};
