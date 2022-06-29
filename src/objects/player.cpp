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
#include <objects/player.h>
#include <objects/monster.h>
#include <objects/weapon.h>
#include <ae/texture.h>
#include <ae/graphics.h>
#include <ae/assets.h>
#include <ae/animation.h>
#include <ae/program.h>
#include <ae/audio.h>
#include <ae/ui.h>
#include <gameassets.h>
#include <stats.h>
#include <map.h>
#include <iostream>
#include <stdexcept>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

// Constructor
_Player::_Player() {
	Type = _Object::PLAYER;

	// Set up animations
	LegAnimation = new ae::_Animation(nullptr);
	LegAnimation->Reels.push_back(ae::Assets.Reels["player_legs"]);

	WalkingAnimation = PLAYER_ANIMATIONWALKINGONEHAND;
	MeleeAnimation = PLAYER_ANIMATIONMELEE;
	ShootingOnehandAnimation = PLAYER_ANIMATIONSHOOTONEHAND;
	ShootingTwohandAnimation = PLAYER_ANIMATIONSHOOTTWOHAND;
	DyingAnimation = PLAYER_ANIMATIONDYING;

	// Weapon offsets
	WeaponParticleOffset[0] = glm::vec2(0, 0);
	WeaponParticleOffset[1] = PLAYER_PISTOLOFFSET;
	for(int i = 2; i < WEAPON_TYPES; i++)
		WeaponParticleOffset[i] = PLAYER_WEAPONOFFSET;

	// Inventory
	for(int i = 0; i < INVENTORY_SIZE; i++)
		Inventory[i] = nullptr;

	// Set animation
	Animation->Reels = ae::Assets.Animations["player"];

	// Set sounds
	_SoundGroup *SoundGroup = GameAssets.GetSoundGroupTemplate("player");
	for(int i = 0; i < SOUND_TYPES; i++) {
		if(SoundGroup)
			Sounds[i] = SoundGroup->SoundID[i];
	}

	Reset();
}

// Destructor
_Player::~_Player() {
	delete LegAnimation;

	DeleteItems();
}

// Resets the player state
void _Player::Reset() {

	// Set stats
	MonsterKills = 0;
	TimePlayed = 0;
	Radius = PLAYER_RADIUS;
	Name = "test";
	ColorID = "white";
	Level = 1;
	Gold = 0;
	Experience = 0;
	ExperienceCurrentLevel = 0;
	ExperienceNextLevel = 0;
	LevelPercentage = 0.0f;
	SkillPointsRemaining = 0;

	for(int i = 0; i < SKILL_COUNT; i++)
		Skills[i] = 0;

	DeleteItems();
	Ammo.clear();

	// Reset state
	MapID = GAME_STARTLEVEL;
	CheckpointIndex = 0;
	Progression = 0;
	Active = true;
	Action = ACTION_IDLE;
	Reloading = false;
	SwitchingWeapons = false;
	Crouching = false;
	Sprinting = false;
	AttackRequested = false;
	UseRequested = false;
	MedkitRequested = false;
	for(int i = 0; i < WEAPONATTACK_COUNT; i++)
		AttackAllowed[i] = true;

	UsePeriod = PLAYER_USEPERIOD;
	ZoomScale = PLAYER_ZOOMSCALE;
	LegDirection = 0.0f;
	MovementSpeed = 0.0f;
	MoveState = MOVE_NONE;
	WeaponSwitchTimer = 0.0;
	ReloadTimer = 0.0;
	UseTimer = 0.0;
	MedkitTimer = 0.0;
	WeaponSwitchFrom = -1;
	WeaponSwitchTo = -1;
	TimePlayed = 0;
	PlayingTimer = 0;
	Stamina = 100.0f;

	CalculateExperienceStats();
	CalculateLevelPercentage();
	CalculateSkillsRemaining();
	UpdateColor();

	Animation->Play(0);
	Animation->Stop();
	LegAnimation->Stop();

	RecalculateStats();
	ResetWeaponAnimation();
	StopAudio();

	Health = MaxHealth;
}

// Deletes the item objects
void _Player::DeleteItems() {
	for(int i = 0; i < INVENTORY_SIZE; i++) {
		delete Inventory[i];
		Inventory[i] = nullptr;
	}
}

// Updates the entity's states
void _Player::Update(double FrameTime) {
	_Entity::Update(FrameTime);

	PlayingTimer += FrameTime;
	WeaponSwitchTimer += FrameTime;
	ReloadTimer += FrameTime;
	UseTimer += FrameTime;
	MedkitTimer += FrameTime;
	if(PlayingTimer > 1.0) {
		TimePlayed++;
		PlayingTimer -= 1.0;
	}

	// Update stamina
	if(!IsDying() && !Sprinting)
		Stamina += PLAYER_STAMINAREGEN * StaminaRegenModifier * FrameTime;

	if(Stamina > MaxStamina)
	   Stamina = MaxStamina;
	if(Tired && Stamina > PLAYER_TIREDTHRESHOLD)
		Tired = false;

	// Update states
	UpdateAnimation(FrameTime);
	UpdateReloading();
	UpdateWeaponSwitch();

	// Stop trigger down audio
	if(TriggerDownAudio && (!AttackRequested || !WeaponHasAmmo() || IsDying() || SwitchingWeapons || Reloading)) {
		StopAudio();
	}

	// Make an attack
	if(AttackRequested) {
		StartAttack();
		AttackRequested = false;
	}

	// Use a medkit
	if(MedkitRequested) {
		UseMedkit(FindItem(_Object::MEDKIT));
		MedkitRequested = false;
	}

	Move(FrameTime);

	if(Stamina > 0.0f && PositionChanged && Sprinting) {
		Stamina -= PLAYER_SPRINTSTAMINA * FrameTime;
		if(Stamina < 0.0f) {
			Stamina = 0.0f;
			SetSprinting(false);
			Tired = true;
		}
	}
}

// Updates the leg's animation and direction
void _Player::UpdateAnimation(double FrameTime, bool PlaySound) {
	_Entity::UpdateAnimation(FrameTime, false);

	int LastFrame = LegAnimation->Frame;
	LegAnimation->Update(FrameTime);

	// Play move sound on first and last frame of leg animation
	if(PlaySound && LastFrame != LegAnimation->Frame && (LegAnimation->Frame == 0 || LegAnimation->Frame == LegAnimation->Reels[LegAnimation->Reel]->EndFrame))
		ae::Audio.PlaySound(ae::Assets.Sounds[GetSound(SOUND_MOVE)]);

	switch(MoveState) {
		case MOVE_FORWARD:
			AdjustLegDirection(0);
		break;
		case MOVE_BACKWARD:
			AdjustLegDirection(180);
		break;
		case MOVE_LEFT:
			AdjustLegDirection(270);
		break;
		case MOVE_RIGHT:
			AdjustLegDirection(90);
		break;
		case MOVE_FORWARDLEFT:
			AdjustLegDirection(315);
		break;
		case MOVE_FORWARDRIGHT:
			AdjustLegDirection(45);
		break;
		case MOVE_BACKWARDLEFT:
			AdjustLegDirection(225);
		break;
		case MOVE_BACKWARDRIGHT:
			AdjustLegDirection(135);
		break;
		default:
		break;
	}

}

// Determines how much to move the leg direction
void _Player::AdjustLegDirection(float Destination) {
	float Distance, Adjust;

	Distance = Destination - LegDirection;

	// Get deltas
	if(Distance < -180.0f)
		Adjust = -(Distance + 180.0f) * PLAYER_LEGCHANGEFACTOR;
	else if(Distance > 180.0f)
		Adjust = -(Distance - 180.0f) * PLAYER_LEGCHANGEFACTOR;
	else
		Adjust = Distance * PLAYER_LEGCHANGEFACTOR;

	// Update leg
	if(std::abs(Adjust) < 0.1f)
		LegDirection = Destination;
	else
		LegDirection += Adjust;

	// Cap direction
	if(LegDirection < 0.0f)
		LegDirection += 360.0f;
	else if(LegDirection >= 360.0f)
		LegDirection -= 360.0f;
}

// Draws the player
void _Player::Render(double BlendFactor) {
	glm::vec2 DrawPosition(Position * (float)BlendFactor + LastPosition * (float)(1.0 - BlendFactor));

	// Draw legs
	ae::Graphics.SetColor(Color);
	ae::Graphics.DrawAnimationFrame(
		glm::vec3(DrawPosition, PositionZ),
		LegAnimation->Reels[LegAnimation->Reel]->Texture,
		glm::vec4(LegAnimation->TextureCoords),
		LegDirection,
		glm::vec2(Scale)
	);

	// Draw torso
	ae::Graphics.SetColor(COLOR_WHITE);
	ae::Graphics.DrawAnimationFrame(
		glm::vec3(DrawPosition, PositionZ + 0.01f),
		Animation->Reels[Animation->Reel]->Texture,
		glm::vec4(Animation->TextureCoords),
		Rotation,
		glm::vec2(Scale)
	);

	//ae::Assets.Fonts["hud_large"]->DrawText(std::to_string(NewLegAnimation->Timer), glm::vec3(DrawPosition, PositionZ), ae::LEFT_BASELINE, glm::vec4(1.0f), 1/64.0f);
}

// Draws the player in screen space
void _Player::Render2D(const glm::ivec2 &Position) {
	ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);

	// Draw legs
	const ae::_Reel *LegTemplate = LegAnimation->Reels[LegAnimation->Reel];
	ae::Graphics.SetColor(Color);
	ae::Graphics.DrawAnimationFrame(
		glm::vec3(Position, 0),
		LegTemplate->Texture,
		glm::vec4(LegAnimation->TextureCoords),
		Rotation,
		glm::vec2(LegTemplate->FrameSize)
	);

	// Draw torso
	const ae::_Reel *WalkTemplate = Animation->Reels[Animation->Reel];
	ae::Graphics.SetColor(COLOR_WHITE);
	ae::Graphics.DrawAnimationFrame(
		glm::vec3(Position, 0.01f),
		WalkTemplate->Texture,
		glm::vec4(Animation->TextureCoords),
		Rotation,
		glm::vec2(WalkTemplate->FrameSize)
	);
}

// Updates the player's experience, leveling up if needed
void _Player::UpdateExperience(int64_t ExperienceGained) {
	Experience = Stats.GetValidExperience(Experience + ExperienceGained);

	// Check if enough experience has been reached for a new level.
	if(Experience >= ExperienceNextLevel)
		UpdateLevel();

	CalculateLevelPercentage();
}

// Advance the player levels
void _Player::UpdateLevel() {
	int OldLevel = Level;

	// Get new level
	CalculateExperienceStats();

	// Get the number of skill points to spend
	CalculateSkillsRemaining();

	// Reset stats
	RecalculateStats();

	// Update current health
	UpdateHealth(Stats.GetLevelHealth(Level) - Stats.GetLevelHealth(OldLevel));
}

// Updates a skill
void _Player::UpdateSkill(int Index, int Value) {
	int TentativeSum = Value;

	for(int i = 0; i < SKILL_COUNT; i++)
		TentativeSum += Skills[i];

	// Check to make sure skill points don't exceed level
	if(TentativeSum > Stats.GetSkillPointsRemaining(Level))
		return;

	Skills[Index] = Stats.GetValidSkillLevel(Skills[Index] + Value);
	CalculateSkillsRemaining();

	// Update player stats
	RecalculateStats();
}

// Calculates the level and experience variables
void _Player::CalculateExperienceStats() {
	Level = Stats.GetLevel(Experience);
	ExperienceCurrentLevel = Stats.GetExperienceForLevel(Level);
	ExperienceNextLevel = Stats.GetExperienceForLevel(Level + 1);
}

// Calculates the number of skills points remaining
void _Player::CalculateSkillsRemaining() {
	SkillPointsRemaining = Stats.GetSkillPointsRemaining(Level) - SpentSkillPoints();
}

// Calculates the percentage to the player's next level
void _Player::CalculateLevelPercentage() {
	LevelPercentage = (float)(Experience - ExperienceCurrentLevel) / (float)(ExperienceNextLevel - ExperienceCurrentLevel);
}

// Returns the number of skill points the player has spent
int _Player::SpentSkillPoints() const {
	int Sum = 0;
	for(int i = 0; i < SKILL_COUNT; i++)
		Sum += Skills[i];

	return Sum;
}

// Adds an item to the player's possession, returns 0 on full, return 1 on delete, return 2 on combine
int _Player::AddItem(_Item *Item, int &AmountAdded) {

	switch(Item->Type) {
		case _Object::WEAPON: {
			_Weapon *WeaponItem = (_Weapon *)(Item);
			if(WeaponItem->IsMelee()) {
				if(!HasMelee()) {
					SetMelee(WeaponItem);
					RecalculateStats();
					ResetWeaponAnimation();
					return 1;
				}
				else
					return AddInventory(Item);
			}
			else {
				if(!HasMainHand()) {
					SetMainHand(WeaponItem);
					RecalculateStats();
					ResetWeaponAnimation();
					return 1;
				}
				else if(!HasOffHand()) {
					SetOffHand(WeaponItem);
					return 1;
				}
				else
					return AddInventory(Item);
			}
		} break;
		case _Object::ARMOR: {
			if(!HasArmor()) {
				SetArmor(Item);
				RecalculateStats();
				return 1;
			}
			else {
				return AddInventory(Item);
			}
		} break;
		case _Object::AMMO: {
			if(Ammo[Item->ID] == AmmoMax[Item->ID])
				return 0;

			int AmountToMax = AmmoMax[Item->ID] - Ammo[Item->ID];
			AmountAdded = std::min(AmountToMax, Item->Attributes["amount"].Int);
			Ammo[Item->ID] += AmountAdded;

			return 1;
		}
		default:
			return AddInventory(Item);
		break;
	}

	return 0;
}

// Drops an item from the player's inventory
void _Player::DropItem(int Slot) {
	if(!CanDropItem() || Slot < 0 || Slot >= INVENTORY_SIZE)
		return;

	// Get item
	_Item *Item = Inventory[Slot];
	if(!Item)
		return;

	// Remove item from inventory
	Inventory[Slot] = nullptr;

	// Check if the item was equipped
	if(Slot < INVENTORY_BAGSTART) {
		RecalculateStats();
		ResetWeaponAnimation();
	}

	// Add item to map
	Item->SetPosition(Position + _Map::GenerateRandomPointInCircle(PLAYER_RADIUS));
	Map->AddItem(Item);
}

// Equip an item
bool _Player::CanEquipItem(_Item *Item, int Slot) {
	if(!Item)
		return true;

	switch(Slot) {
		case INVENTORY_ARMOR: {
			if(Item->Type != _Object::ARMOR)
				return false;

			return true;
		} break;
		case INVENTORY_MAINHAND:
		case INVENTORY_OFFHAND:
		case INVENTORY_MELEE: {
			if(Item->Type == _Object::WEAPON) {
				_Weapon *Weapon = (_Weapon *)Item;
				if(Slot == INVENTORY_MELEE) {
					if(Weapon->IsMelee())
						return true;
				}
				else
					return !Weapon->IsMelee();
			}
		} break;
	}

	return false;
}

// Swap inventory
void _Player::SwapInventory(int SlotFrom, int SlotTo) {
	if(SlotFrom == SlotTo)
		return;

	bool CanSwap = false;

	// Check for simple swap
	if(IsBagIndex(SlotFrom) && IsBagIndex(SlotTo)) {
		CanSwap = true;
	}
	// Equipment swap
	else if((IsEquipmentIndex(SlotFrom) && IsBagIndex(SlotTo)) || (IsEquipmentIndex(SlotTo) && IsBagIndex(SlotFrom)) || (IsEquipmentIndex(SlotTo) && IsEquipmentIndex(SlotFrom))) {
		if(IsEquipmentIndex(SlotTo)) {

			// Try component
			if(!AddComponent(SlotFrom, SlotTo)) {
				CanSwap = CanEquipItem(Inventory[SlotFrom], SlotTo);
			}
		}
		else
			CanSwap = CanEquipItem(Inventory[SlotTo], SlotFrom);
	}

	if(CanSwap) {

		if(SlotTo == INVENTORY_MAINHAND || (IsHandIndex(SlotFrom) && IsHandIndex(SlotTo))) {
			StartWeaponSwitch(SlotFrom, SlotTo);
		}
		else {

			// Try to combine items
			int CombineResult = CombineItems(Inventory[SlotFrom], Inventory[SlotTo]);
			if(CombineResult == 0) {
				_Item *Temp = Inventory[SlotFrom];
				Inventory[SlotFrom] = Inventory[SlotTo];
				Inventory[SlotTo] = Temp;
			}
			else if(CombineResult == 2) {
				delete Inventory[SlotFrom];
				Inventory[SlotFrom] = nullptr;
			}
		}

		RecalculateStats();
		ResetWeaponAnimation();
	}
}

// Attempts to combine two items and deletes FromItem if successful
// Return 0 when item can't be combined
// Return 1 when item was combined but still has count left
// Return 2 when item was combined and fromitem needs deletion
int _Player::CombineItems(_Item *FromItem, _Item *ToItem) {

	if(FromItem && ToItem && FromItem->CanStack() && ToItem->CanStack() && FromItem->ID == ToItem->ID) {
		ToItem->UpdateCount(FromItem->Count);
		if(ToItem->Count > GetInventoryMaxStack()) {
			FromItem->Count = ToItem->Count - GetInventoryMaxStack();
			ToItem->Count = GetInventoryMaxStack();

			return 1;
		}
		else
			return 2;
	}

	return 0;
}

// Add an item to the inventory
// return 0 on inventory full
// return 1 on added item
// return 2 on added item and combined
int _Player::AddInventory(_Item *Item) {

	// Search for an existing item or empty slot
	int EmptySlot = -1;
	for(int i = INVENTORY_BAGSTART; i < INVENTORY_BAGEND; i++) {
		if(CombineItems(Item, Inventory[i]) == 2) {
			return 2;
		}

		if(Inventory[i] == nullptr && EmptySlot == -1)
			EmptySlot = i;
	}

	// Add item to empty slot
	if(EmptySlot != -1) {
		Inventory[EmptySlot] = Item;
		return 1;
	}

	return 0;
}

// Add an upgrade to a weapon
bool _Player::AddComponent(int FromIndex, int ToIndex) {
	if(!HasInventory(FromIndex) || Inventory[FromIndex]->Type != _Object::UPGRADE)
		return false;

	_Weapon *Weapon = nullptr;
	if(ToIndex == INVENTORY_MAINHAND && HasMainHand())
		Weapon = GetMainHand();
	else if(ToIndex == INVENTORY_OFFHAND && HasOffHand())
		Weapon = GetOffHand();
	else if(ToIndex == INVENTORY_MELEE && HasOffHand())
		Weapon = GetOffHand();
	else
		return false;

	if(Weapon->AddComponent(Inventory[FromIndex])) {
		ConsumeInventory(FromIndex, false);
		RecalculateStats();
		return true;
	}

	return false;
}

// Calculates the radius of the crosshair
float _Player::GetCrosshairRadius(const glm::vec2 &Cursor) {

	// Check bounds
	float Accuracy = CurrentAccuracy * AccuracyModifier;
	if(Accuracy < 0.0f)
		Accuracy = 0.0f;
	else if(Accuracy > PLAYER_MAXACCURACY)
		Accuracy = PLAYER_MAXACCURACY;

	// Get distance to cursor
	float Distance = glm::length(Cursor - Position);

	return tan(glm::radians(Accuracy * 0.5f)) * Distance;
}

// Checks if the player's weapon has ammo
bool _Player::WeaponHasAmmo() const {
	if(AttackRequestType == WEAPONATTACK_MAIN) {
		if(!HasMainHand() || Stats.Weapons.at(GetMainHand()->ID).AmmoID == "")
			return true;

		return GetMainHand()->Attributes.at("ammo").Int > 0;
	}
	else if(AttackRequestType == WEAPONATTACK_MELEE) {
		return true;
	}

	return false;
}

// Checks if the player has ammo for the main weapon
bool _Player::HasAmmoForMain() const {
	if(!HasMainHand())
		return false;

	const std::string &AmmoType = Stats.Weapons.at(GetMainHand()->ID).AmmoID;
	if(Ammo.find(AmmoType) == Ammo.end())
		return false;

	return Ammo.at(AmmoType) > 0;
}

// Reduces the weapons ammo by one
void _Player::ReduceAmmo() {
	if(HasMainHand() && AttackRequestType == WEAPONATTACK_MAIN) {
		GetMainHand()->Attributes["ammo"].Int--;
		if(GetMainHand()->Attributes["ammo"].Int < 0)
			GetMainHand()->Attributes["ammo"].Int = 0;
	}
}

// Uses an item from the player's inventory, return true if a key was used
bool _Player::UseItem(int Index, bool Event) {
	if(Index >= INVENTORY_BAGSTART && Index < INVENTORY_BAGEND && HasInventory(Index)) {
		switch(Inventory[Index]->Type) {
			case _Object::MEDKIT:
				UseMedkit(Index);
			break;
			case _Object::KEY:
				if(Event) {
					ConsumeInventory(Index);
					return true;
				}
			break;
		}
	}

	return false;
}

// Uses a medkit if one is available
bool _Player::UseMedkit(int Index) {
	if(CanUseMedkit() && HasInventory(Index) && Inventory[Index]->Type == _Object::MEDKIT) {
		UpdateHealth(Inventory[Index]->Attributes.at("health_restored").Int);
		ConsumeInventory(Index);
		MedkitTimer = 0;

		return true;
	}

	return false;
}

// Searches for an item by type and returns the index
int _Player::FindItem(int ItemType) {
	for(int i = INVENTORY_BAGSTART; i < INVENTORY_BAGEND; i++) {
		if(HasInventory(i) && Inventory[i]->Type == ItemType)
			return i;
	}

	return -1;
}

// Searchs the inventory for a certain item
int _Player::FindItem(const std::string &ID) {
	for(int i = INVENTORY_BAGSTART; i < INVENTORY_BAGEND; i++) {
		if(HasInventory(i) && Inventory[i]->ID == ID) {
			return i;
		}
	}

	return -1;
}

// Begins the reloading process
void _Player::StartReloading() {

	// Test conditions
	if(!CanReload())
		return;

	// Play sound
	ae::Audio.PlaySound(ae::Assets.Sounds[GetSound(SOUND_RELOAD)]);

	// Start timer
	ReloadTimer = 0;
	Reloading = true;
}

// Cancel the reload process
void _Player::CancelReloading() {
	Reloading = false;
}

// Begins the weapon switch process
void _Player::StartWeaponSwitch(int SlotFrom, int SlotTo) {

	// Test conditions
	if(!CanSwitchWeapons())
		return;

	// Test for empty hands
	if(IsHandIndex(SlotFrom) && IsHandIndex(SlotTo) && !GetMainHand() && !GetOffHand())
		return;

	// Start timer
	WeaponSwitchFrom = SlotFrom;
	WeaponSwitchTo = SlotTo;
	WeaponSwitchTimer = 0;
	SwitchingWeapons = true;
}

// Reloads the weapon when the timer goes off
void _Player::UpdateReloading() {

	// Check the timer
	if(Reloading && (ReloadTimer > ReloadPeriod)) {
		Reloading = false;

		// Check weapon type
		if(!CanReload())
			return;

		// Check for ammo
		if(HasAmmoForMain()) {
			const std::string &AmmoType = Stats.Weapons.at(GetMainHand()->ID).AmmoID;
			int AmountNeeded = GetMainHand()->Attributes["rounds"].Int - GetMainHand()->Attributes["ammo"].Int;
			int AmmoLoadAmount = std::min(Ammo[AmmoType], AmountNeeded);
			GetMainHand()->Attributes["ammo"].Int += AmmoLoadAmount;
			Ammo[AmmoType] -= AmmoLoadAmount;

			// Update accuracy
			ResetAccuracy(true);
			ResetWeaponAnimation();
		}
	}
}

// Switches the weapon when the timer goes off
void _Player::UpdateWeaponSwitch() {

	// Check the timer
	if(SwitchingWeapons && (WeaponSwitchTimer > WeaponSwitchPeriod)) {
		SwitchingWeapons = false;

		// Check weapon type
		if(CanSwitchWeapons()) {
			_Item *Temp = Inventory[WeaponSwitchFrom];
			Inventory[WeaponSwitchFrom] = Inventory[WeaponSwitchTo];
			Inventory[WeaponSwitchTo] = Temp;

			RecalculateStats();
			ResetWeaponAnimation();
		}
	}
}

// Updates the states for crouching and running
void _Player::UpdateSpeed(float Factor) {

	if(Crouching)
		MovementModifier = PLAYER_CROUCHINGSPEEDFACTOR;
	else if(Sprinting)
		MovementModifier = PLAYER_SPRINTINGSPEEDFACTOR;
	else
		MovementModifier = 1.0f;

	MovementModifier *= Factor;

	LegAnimation->FramePeriod = LegAnimation->Reels[0]->FramePeriod / MovementModifier;
	if(Animation->Reel == PLAYER_ANIMATIONWALKINGONEHAND || Animation->Reel == PLAYER_ANIMATIONWALKINGTWOHAND)
		SetAnimationPlaybackSpeedFactor();
}

// Updates the states for crouching
void _Player::SetCrouching(bool State) {

	// Update state
	if(Crouching != State) {
		Crouching = State;
		ResetAccuracy(false);
		UpdateSpeed(1.0f);
	}

	if(Crouching)
		SetSprinting(false);
}

// Update state for sprinting
void _Player::SetSprinting(bool State) {
	if(State && Tired)
		return;

	// Update state
	if(Sprinting != State) {
		Sprinting = State;
		ResetAccuracy(false);
		UpdateSpeed(1.0f);
	}

	if(Sprinting)
		SetCrouching(false);
}

// Resets the accuracy depending on crouching states
void _Player::ResetAccuracy(bool CompleteReset) {

	if(Crouching && !IsMelee()) {
		AccuracyModifier = 0.5f;
	}
	else if(Sprinting && !IsMelee()) {
		AccuracyModifier = 2.0f;
	}
	else {
		AccuracyModifier = 1.0f;

	}
	if(CompleteReset)
		CurrentAccuracy = MinAccuracyNormal;

	MinAccuracy = MinAccuracyNormal;
	MaxAccuracy[WEAPONATTACK_MAIN] = MaxAccuracyNormal;
}

// Consume an item from the inventory
void _Player::ConsumeInventory(int Index, bool Delete) {
	if(Inventory[Index] == nullptr)
		return;

	if(Index < INVENTORY_BAGSTART || Index >= INVENTORY_BAGEND)
		return;

	if(Inventory[Index]->UpdateCount(-1) <= 0) {
		if(Delete)
			delete Inventory[Index];
		Inventory[Index] = nullptr;
	}
}

// Calculates the player's stats from weapons and skills
void _Player::RecalculateStats() {
	_ObjectTemplate Weapon[WEAPONATTACK_COUNT] = { _Object::WEAPON, _Object::WEAPON };
	for(int i = 0; i < WEAPONATTACK_COUNT; i++)
		Weapon[i].Attributes = Stats.WeaponFists->Attributes;

	// See if the player is using a weapon
	if(HasMainHand()) {
		Weapon[WEAPONATTACK_MAIN].Attributes = GetMainHand()->Attributes;
		MainWeaponType = GetMainHand()->Attributes.at("weapon_type").Int;
	}
	else
		MainWeaponType = WEAPON_MELEE;

	// Get stats of melee weapon
	if(HasMelee())
		Weapon[WEAPONATTACK_MELEE].Attributes = GetMelee()->Attributes;

	// Set up main stats based on weapon
	Recoil = 0;
	RecoilRegen = 0;
	AttackRange[WEAPONATTACK_MAIN] = Weapon[WEAPONATTACK_MAIN].Attributes["range"].Float;
	AttackRange[WEAPONATTACK_MELEE] = Weapon[WEAPONATTACK_MELEE].Attributes["range"].Float;
	if(MainWeaponType == WEAPON_MELEE) {
		CurrentAccuracyNormal = MinAccuracyNormal = Weapon[WEAPONATTACK_MAIN].Attributes.at("min_accuracy").Int;
		MaxAccuracyNormal = Weapon[WEAPONATTACK_MAIN].Attributes.at("max_accuracy").Int;
	}
	else {
		float AccuracySkillMultiplier = 1.0f / Stats.GetSkillBonusMultiplier(Skills[SKILL_ACCURACY], SKILL_ACCURACY);
		CurrentAccuracyNormal = MinAccuracyNormal = Weapon[WEAPONATTACK_MAIN].Attributes.at("min_accuracy").Int * AccuracySkillMultiplier;
		MaxAccuracyNormal = Weapon[WEAPONATTACK_MAIN].Attributes.at("max_accuracy").Int * AccuracySkillMultiplier;
		Recoil = Weapon[WEAPONATTACK_MAIN].Attributes["recoil"].Float;
		RecoilRegen = Weapon[WEAPONATTACK_MAIN].Attributes["recoil_regen"].Float;
	}

	MaxAccuracy[WEAPONATTACK_MELEE] = Weapon[WEAPONATTACK_MELEE].Attributes.at("max_accuracy").Int;

	// Set accuracy
	ResetAccuracy(true);

	// Attacking
	for(int i = 0; i < WEAPONATTACK_COUNT; i++) {
		FireRate[i] = Weapon[i].Attributes["fire_rate"].Int;
		FirePeriod[i] = Weapon[i].Attributes["fire_period"].Double / Stats.GetSkillBonusMultiplier(Skills[SKILL_ATTACKSPEED], SKILL_ATTACKSPEED);
		MinDamage[i] = Weapon[i].Attributes["min_damage"].Int;
		MaxDamage[i] = Weapon[i].Attributes["max_damage"].Int;
	}
	ReloadPeriod = Weapon[WEAPONATTACK_MAIN].Attributes["reload_period"].Double / Stats.GetSkillBonusMultiplier(Skills[SKILL_RELOADSPEED], SKILL_RELOADSPEED);
	WeaponSwitchPeriod = PLAYER_WEAPONSWITCHPERIOD * 1;
	AttackCount = Weapon[WEAPONATTACK_MAIN].Attributes["attack_count"].Int;
	ZoomScale = Weapon[WEAPONATTACK_MAIN].Attributes["zoom_scale"].Float;

	// Cap fire period
	if(FirePeriod[WEAPONATTACK_MAIN] < WEAPON_MINFIREPERIOD)
		FirePeriod[WEAPONATTACK_MAIN] = WEAPON_MINFIREPERIOD;

	int BaseMovementSpeed = 100 + Stats.GetSkill(Skills[SKILL_MOVESPEED], SKILL_MOVESPEED);
	DamageResist = Stats.GetSkill(Skills[SKILL_DAMAGERESIST], SKILL_DAMAGERESIST);
	MaxHealth = (int)(Stats.GetLevelHealth(Level) * Stats.GetSkillBonusMultiplier(Skills[SKILL_HEALTH], SKILL_HEALTH));
	MaxStamina = Stats.GetSkillBonusMultiplier(Skills[SKILL_MAXSTAMINA], SKILL_MAXSTAMINA);
	StaminaRegenModifier = Stats.GetSkillBonusMultiplier(Skills[SKILL_MAXSTAMINA], SKILL_MAXSTAMINA);

	// Armor
	DamageBlock = Stats.GetLevelDamageBlock(Level);
	Attributes["max_ammo"].Int = 100;
	if(GetArmor()) {
		DamageBlock += GetArmor()->Attributes.at("damage_block").Int;
		DamageResist += GetArmor()->Attributes.at("damage_resist").Int;
		BaseMovementSpeed += GetArmor()->Attributes.at("move_speed").Int;
		Attributes["max_ammo"].Int += GetArmor()->Attributes.at("max_ammo").Int;
	}

	// Get final speed
	MovementSpeed = BaseMovementSpeed * 0.01f * PLAYER_MOVEMENTSPEED;

	// Handle max ammo
	AmmoMax.clear();
	for(const auto &AmmoType : Stats.AmmoNames) {
		AmmoMax[AmmoType] = Stats.Items.at(AmmoType).Attributes["amount_max"].Int * Attributes["max_ammo"].Mult();
		if(Ammo.find(AmmoType) != Ammo.end())
			Ammo[AmmoType] = std::min(Ammo[AmmoType], AmmoMax[AmmoType]);
	}
}

// Sets the weapon animation for the player
void _Player::ResetWeaponAnimation() {

	if(!IsDying()) {

		// Get walking animation
		switch(GetWeaponType()) {
			case WEAPON_MELEE:
			case WEAPON_PISTOL:
				WalkingAnimation = PLAYER_ANIMATIONWALKINGONEHAND;
			break;
			default:
				WalkingAnimation = PLAYER_ANIMATIONWALKINGTWOHAND;
			break;
		}

		Action = ACTION_IDLE;
		Animation->Stop();
		Animation->Play(WalkingAnimation, MovementSpeed);
		Animation->CalculateTextureCoords();
		SetAnimationPlaybackSpeedFactor();
	}
}

// Applies the death penalty
void _Player::IncurDeathPenalty() {
	Reloading = SwitchingWeapons = false;
}

// Returns a sound index
const std::string &_Player::GetSound(int SoundType) const {

	if(AttackRequestType == 0 && SoundType <= SOUND_HIT && HasMainHand())
		return GetMainHand()->GetSound(SoundType);
	else if(AttackRequestType == 1 && SoundType <= SOUND_HIT && HasMelee())
		return GetMelee()->GetSound(SoundType);
	else
		return Sounds[SoundType];
}

// Returns the weapon's particle template
const _ParticleTemplate *_Player::GetWeaponParticle(int Index) const {
	if(HasMainHand())
		return Stats.Weapons.at(GetMainHand()->ID).WeaponParticles->ParticleTemplates[Index];

	return nullptr;
}

// Sets the color string and color of the player
void _Player::UpdateColor() {
	Color = ae::Assets.Colors[ColorID];
}

int _Player::GetInventoryMaxStack() const {
	return INVENTORY_MAX_STACK;
}

bool _Player::CanUseMedkit() const {
	return (MedkitTimer > PLAYER_MEDKITPERIOD) && Health < MaxHealth;
}

bool _Player::CanReload() const {
	return HasMainHand() && !Reloading && !SwitchingWeapons && !IsMeleeAttacking() && GetMainHand()->Attributes.at("ammo").Int != GetMainHand()->Attributes.at("rounds").Int && HasAmmoForMain();
}

bool _Player::IsMelee() const { return GetMainHand() == nullptr || GetMainHand()->IsMelee(); }

void _Player::SetMainHand(_Weapon *Weapon) { Inventory[INVENTORY_MAINHAND] = Weapon; }
void _Player::SetOffHand(_Weapon *Weapon) { Inventory[INVENTORY_OFFHAND] = Weapon; }
void _Player::SetMelee(_Weapon *Weapon) { Inventory[INVENTORY_MELEE] = Weapon; }
void _Player::SetArmor(_Item *Armor) { Inventory[INVENTORY_ARMOR] = Armor; }

void _Player::SetLegAnimationPlayMode(int Mode) {
	if(Mode == ae::_Animation::PLAYING)
		LegAnimation->Play(0);
	else if(Mode == ae::_Animation::STOPPED)
		LegAnimation->Stop();
}

void _Player::SetAnimationPlaybackSpeedFactor() {
	Animation->FramePeriod = Animation->Reels[Animation->Reel]->FramePeriod / MovementModifier;
}
