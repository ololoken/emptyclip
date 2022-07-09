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
#include <algorithm>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtx/rotate_vector.hpp>

// Constructor
_Player::_Player(const _ObjectTemplate &PlayerTemplate) :
	_Entity(PlayerTemplate) {

	// Set up animations
	LegAnimation = new ae::_Animation(nullptr);
	LegAnimation->Reels.push_back(ae::Assets.Reels["player_legs"]);

	WalkingAnimation = PLAYER_ANIMATIONWALKINGONEHAND;
	MeleeAnimation = PLAYER_ANIMATIONMELEE;
	ShootingOnehandAnimation = PLAYER_ANIMATIONSHOOTONEHAND;
	ShootingTwohandAnimation = PLAYER_ANIMATIONSHOOTTWOHAND;
	DyingAnimation = PLAYER_ANIMATIONDYING;

	// Weapon offsets
	WeaponOffset[0] = glm::vec2(0, 0);
	WeaponOffset[1] = PLAYER_PISTOLOFFSET;
	for(int i = 2; i < WEAPON_COUNT; i++)
		WeaponOffset[i] = PLAYER_WEAPONOFFSET;

	// Inventory
	for(int i = 0; i < INVENTORY_SIZE; i++)
		Inventory[i] = nullptr;

	// Set animation
	Animation->Reels = ae::Assets.Animations["player"];

	// Set sounds
	_SoundGroup &SoundGroup = GameAssets.SoundGroups.at("player");
	for(int i = 0; i < SOUND_COUNT; i++)
		Sounds[i] = SoundGroup.SoundID[i];

	Reset();
}

// Destructor
_Player::~_Player() {
	delete LegAnimation;

	DeleteItems();
}

// Deletes the item objects
void _Player::DeleteItems() {
	for(int i = 0; i < INVENTORY_SIZE; i++) {
		delete Inventory[i];
		Inventory[i] = nullptr;
	}
}

// Resets the player state
void _Player::Reset() {
	Kills = 0;
	Deaths = 0;
	PlayTime = 0;
	Radius = PLAYER_RADIUS;
	Name = "test";
	ColorID = "white";
	Color = glm::vec4(1.0f);
	Level = 1;
	Gold = 0;
	Experience = 0;
	ExperienceNeeded = 0;
	ExperienceNextLevel = 0;
	ExperienceLost = 0;
	SkillPointsRemaining = 0;
	DropRate = 100;
	PickupModifier = 1.0f;
	HealModifier = 1.0f;
	MapID = GAME_STARTLEVEL;
	CheckpointIndex = 0;
	Progression = 0;
	Active = true;
	Action = ACTION_IDLE;
	Reloading = false;
	ReloadSound = nullptr;
	SwitchingWeapons = false;
	Aiming = false;
	Sprinting = false;
	AttackRequested = false;
	UseRequested = false;
	MedkitRequested = false;
	MeleeTexture = nullptr;
	UsePeriod = PLAYER_USEPERIOD;
	ZoomScale = PLAYER_ZOOMSCALE;
	LegDirection = 0.0f;
	MoveSpeed = 0.0f;
	MoveState = MOVE_NONE;
	WeaponSwitchTimer = 0.0;
	ReloadTimer = 0.0;
	UseTimer = 0.0;
	MedkitTimer = 0.0;
	WeaponSwitchFrom = -1;
	WeaponSwitchTo = -1;
	Stamina = 100.0f;
	InvulnerableTimer = 0.0;
	for(int i = 0; i < WEAPONATTACK_COUNT; i++)
		AttackAllowed[i] = true;
	for(int i = 0; i < SKILL_COUNT; i++)
		Skills[i] = 0;

	DeleteItems();
	Ammo.clear();
	Keys.clear();

	CalculateExperienceStats();
	CalculateSkillsRemaining();
	RecalculateStats();
	ResetWeaponAnimation();
	StopAudio();

	Animation->Play(0);
	Animation->Stop();
	LegAnimation->Stop();

	Health = MaxHealth;
}

// Calculates the player's stats from weapons and skills
void _Player::RecalculateStats() {
	CalculateExperienceStats();
	CalculateSkillsRemaining();

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
	if(HasMelee()) {
		Weapon[WEAPONATTACK_MELEE].Attributes = GetMelee()->Attributes;
		MeleeTexture = ae::Assets.Textures[GetMelee()->Template.MeleeID];
	}
	else
		MeleeTexture =  ae::Assets.Textures[Stats.WeaponFists->Template.MeleeID];

	// Set up main stats based on weapon
	Recoil = 0;
	RecoilRegen = 0;
	MoveRecoil = 0.0f;
	AttackRange[WEAPONATTACK_MAIN] = Weapon[WEAPONATTACK_MAIN].Attributes["range"].Float;
	AttackRange[WEAPONATTACK_MELEE] = Weapon[WEAPONATTACK_MELEE].Attributes["range"].Float;
	CurrentAccuracyNormal = 0;
	MinAccuracyNormal = 0;
	MaxAccuracyNormal = 0;
	if(MainWeaponType != WEAPON_MELEE) {
		float StrengthSkillMultiplier = Stats.GetSkillBonusMultiplier(Skills[SKILL_STRENGTH], SKILL_STRENGTH);
		float AccuracySkillMultiplier = 1.0f / Stats.GetSkillBonusMultiplier(Skills[SKILL_PERCEPTION], SKILL_PERCEPTION, 1);
		CurrentAccuracyNormal = MinAccuracyNormal = Weapon[WEAPONATTACK_MAIN].Attributes.at("min_accuracy").Float * AccuracySkillMultiplier;
		MaxAccuracyNormal = Weapon[WEAPONATTACK_MAIN].Attributes.at("max_accuracy").Float * AccuracySkillMultiplier;
		Recoil = Weapon[WEAPONATTACK_MAIN].Attributes["recoil"].Float / StrengthSkillMultiplier;
		RecoilRegen = Weapon[WEAPONATTACK_MAIN].Attributes["recoil_regen"].Float * StrengthSkillMultiplier;
		MoveRecoil = Weapon[WEAPONATTACK_MAIN].Attributes["move_recoil"].Float / StrengthSkillMultiplier;
	}

	// Set accuracy
	ResetAccuracy(true);

	// Attacking
	for(int i = 0; i < WEAPONATTACK_COUNT; i++) {
		float MeleeDamageModifier = 1.0f;
		if((i == WEAPONATTACK_MAIN && MainWeaponType == WEAPON_MELEE) || i == WEAPONATTACK_MELEE)
			MeleeDamageModifier = Stats.GetSkillBonusMultiplier(Skills[SKILL_STRENGTH], SKILL_STRENGTH);

		FireRateType[i] = Weapon[i].Attributes["fire_rate"].Int;
		AttackPeriod[i] = std::max(Weapon[i].Attributes["fire_period"].Double / Stats.GetSkillBonusMultiplier(Skills[SKILL_AGILITY], SKILL_AGILITY), WEAPON_MINFIREPERIOD);
		MinDamage[i] = std::ceil(Weapon[i].Attributes["min_damage"].Int * MeleeDamageModifier);
		MaxDamage[i] = std::ceil(Weapon[i].Attributes["max_damage"].Int * MeleeDamageModifier);
		AttackMoveSpeed[i] = Weapon[i].Attributes["attack_movespeed"].Float;
		Penetration[i] = Weapon[i].Attributes["penetration"].Int;
		AttackCount[i] = Weapon[i].Attributes["attack_count"].Int;
		AttackWidth[i] = Weapon[i].Attributes["attack_width"].Float;
		MeleeScale[i].x = Weapon[i].Attributes["scale_x"].Float;
		MeleeScale[i].y = Weapon[i].Attributes["scale_y"].Float;
	}
	ReloadPeriod = Weapon[WEAPONATTACK_MAIN].Attributes["reload_period"].Double / Stats.GetSkillBonusMultiplier(Skills[SKILL_DEXTERITY], SKILL_DEXTERITY);
	WeaponSwitchPeriod = PLAYER_WEAPONSWITCHPERIOD / Stats.GetSkillBonusMultiplier(Skills[SKILL_DEXTERITY], SKILL_DEXTERITY);
	ZoomScale = Weapon[WEAPONATTACK_MAIN].Attributes["zoom_scale"].Float;

	BaseMoveSpeed = 100 + Stats.GetSkill(Skills[SKILL_CUNNING], SKILL_CUNNING);
	MaxHealth = (int)(Stats.GetLevelHealth(Level) * Stats.GetSkillBonusMultiplier(Skills[SKILL_VITALITY], SKILL_VITALITY));
	MaxStamina = Stats.GetSkillBonusMultiplier(Skills[SKILL_ENDURANCE], SKILL_ENDURANCE);
	StaminaRegenModifier = Stats.GetSkillBonusMultiplier(Skills[SKILL_ENDURANCE], SKILL_ENDURANCE);
	Health = std::clamp(Health, 0, MaxHealth);
	HealModifier = Stats.GetSkillBonusMultiplier(Skills[SKILL_VITALITY], SKILL_VITALITY, 1);

	// Armor
	DamageBlock = Stats.GetSkill(Skills[SKILL_FORTITUDE], SKILL_FORTITUDE);
	DamageResist = std::min((int)Stats.GetSkill(Skills[SKILL_FORTITUDE], SKILL_FORTITUDE, 1), ENTITY_MAX_DAMAGE_RESIST);
	Attributes["max_ammo"].Int = 100;
	if(GetArmor()) {
		DamageBlock += GetArmor()->Attributes.at("damage_block").Int;
		DamageResist += GetArmor()->Attributes.at("damage_resist").Int;
		BaseMoveSpeed += GetArmor()->Attributes.at("move_speed").Int;
		Attributes["max_ammo"].Int += GetArmor()->Attributes.at("max_ammo").Int;
	}

	// Get final speed
	MoveSpeed = BaseMoveSpeed * 0.01f * PLAYER_MOVESPEED;

	// Handle max ammo
	AmmoMax.clear();
	for(const auto &AmmoType : Stats.AmmoNames) {
		AmmoMax[AmmoType] = Stats.Objects.at(AmmoType).Attributes["amount_max"].Int * Attributes["max_ammo"].Mult();
		if(Ammo.find(AmmoType) != Ammo.end())
			Ammo[AmmoType] = std::min(Ammo[AmmoType], AmmoMax[AmmoType]);
	}

	// Drop Rate
	DropRate = 100 + Skills[SKILL_LUCK];
	PickupModifier = Stats.GetSkillBonusMultiplier(Skills[SKILL_LUCK], SKILL_LUCK, 1);
}

// Updates the entity's states
void _Player::Update(double FrameTime) {
	_Entity::Update(FrameTime);

	PlayTime += FrameTime;
	WeaponSwitchTimer += FrameTime;
	ReloadTimer += FrameTime;
	UseTimer += FrameTime;
	MedkitTimer += FrameTime;

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
	if(TriggerDownAudio && (!AttackRequested || !WeaponHasAmmo(WEAPONATTACK_MAIN) || IsDying() || SwitchingWeapons || Reloading)) {
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
		ae::Audio.PlaySound(GetSound(SOUND_MOVE, -1));

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

// Draws the player
void _Player::Render(double BlendFactor) {
	glm::vec2 DrawPosition(Position * (float)BlendFactor + LastPosition * (float)(1.0 - BlendFactor));
	float Alpha = 1.0f;
	if(IsInvulnerable())
		Alpha = std::abs(std::fmod(InvulnerableTimer * 5, 1.0)) >= 0.5 ? 0.25f : 0.75f;

	// Draw legs
	ae::Graphics.SetColor(glm::vec4(Color.r, Color.g, Color.b, Color.a * Alpha));
	ae::Graphics.DrawAnimationFrame(
		glm::vec3(DrawPosition, PositionZ),
		LegAnimation->Reels[LegAnimation->Reel]->Texture,
		glm::vec4(LegAnimation->TextureCoords),
		LegDirection,
		glm::vec2(Scale)
	);

	// Draw melee thrust animation
	ae::Graphics.SetColor(glm::vec4(1.0f, 1.0f, 1.0f, Alpha));
	if(Action == ACTION_MELEE && MeleeTexture) {
		float MeleePercent = std::clamp(AttackTimer[AttackRequestType] / AttackPeriod[AttackRequestType], 0.0, 1.0);
		float MeleeMagnitude = std::sin(MeleePercent * glm::pi<double>());

		//TODO fix when range is < 0.5

		// Start position of melee frame behind the player and shift position proportional to magnitude if range is bigger than the melee texture
		glm::vec2 MeleePosition = DrawPosition + Direction * (MeleeScale[AttackRequestType].y * 0.5f + (AttackRange[AttackRequestType] - MeleeScale[AttackRequestType].y) * MeleeMagnitude);
		ae::Graphics.DrawAnimationFrame(
			glm::vec3(MeleePosition, PositionZ + 0.005f),
			MeleeTexture,
			glm::vec4(0.0f, MeleeMagnitude - 1.0f, 1.0f, MeleeMagnitude),
			Rotation,
			MeleeScale[AttackRequestType]
		);
	}

	/*
	// Draw melee swing
	if(Action == ACTION_MELEE) {
		double MeleePercent = std::clamp(AttackTimer[1] / AttackPeriod[1], 0.0, 1.0);
		float MeleeRotation = Rotation + (0.5 - MeleePercent) * MaxAccuracy[WEAPONATTACK_MELEE];
		if(MeleeRotation < 0)
			MeleeRotation += 360;
		else if(MeleeRotation > 360)
			MeleeRotation -= 360;

		//std::cout << MeleeRotation << " " << AttackTimer[1] << " " << AttackPeriod[1] << std::endl;
		glm::vec2 MeleeDirection = glm::rotate(glm::vec2(0, -AttackRange[WEAPONATTACK_MELEE]), glm::radians(MeleeRotation));
		ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
		ae::Graphics.SetDepthMask(false);
		ae::Graphics.SetDepthTest(false);
		ae::Graphics.SetColor(COLOR_WHITE);
		ae::Graphics.DrawLine(Position, Position + MeleeDirection);
		ae::Graphics.SetDepthTest(true);
	}
	*/

	// Draw torso
	ae::Graphics.DrawAnimationFrame(
		glm::vec3(DrawPosition, PositionZ + 0.01f),
		Animation->Reels[Animation->Reel]->Texture,
		glm::vec4(Animation->TextureCoords),
		Rotation,
		glm::vec2(Scale)
	);
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
		glm::vec2(LegTemplate->FrameSize) * ae::_Element::GetUIScale()
	);

	// Draw torso
	const ae::_Reel *WalkTemplate = Animation->Reels[Animation->Reel];
	ae::Graphics.SetColor(COLOR_WHITE);
	ae::Graphics.DrawAnimationFrame(
		glm::vec3(Position, 0.01f),
		WalkTemplate->Texture,
		glm::vec4(Animation->TextureCoords),
		Rotation,
		glm::vec2(WalkTemplate->FrameSize) * ae::_Element::GetUIScale()
	);
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

// Updates the player's experience, leveling up if needed
void _Player::UpdateExperience(int64_t ExperienceGained) {
	Experience = std::clamp(Experience + ExperienceGained, (int64_t)0, Stats.Levels.back().Experience);

	int OldLevel = Level;
	CalculateExperienceStats();

	// Check for new level
	if(Level > OldLevel) {
		CalculateSkillsRemaining();
		RecalculateStats();
		Health = MaxHealth;
		ae::Audio.PlaySound(ae::Assets.Sounds["ui_levelup0"]);
	}
}

// Updates a skill
void _Player::UpdateSkill(int Index, int Value) {

	Value = std::min(Value, SkillPointsRemaining);
	Value = std::min(Value, Stats.GetMaxSkillLevel(Level) - Skills[Index]);
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
	const _Level &LevelStat = Stats.FindLevel(Experience);
	Level = LevelStat.Level;
	ExperienceNextLevel = LevelStat.NextLevel;

	int64_t ExperienceThisLevel = Experience - LevelStat.Experience;
	ExperienceNeeded = (Level == Stats.GetMaxLevel()) ? 0 : LevelStat.NextLevel - ExperienceThisLevel;
	ExperienceLost = std::min(ExperienceThisLevel, (int64_t)(LevelStat.NextLevel * GAME_EXPERIENCE_LOST));
}

// Calculates the number of skills points remaining
void _Player::CalculateSkillsRemaining() {
	SkillPointsRemaining = Stats.GetSkillPointsRemaining(Level) - SpentSkillPoints();
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
			if(Item->IsMelee()) {
				if(!HasMelee()) {
					Inventory[INVENTORY_MELEE] = Item;
					RecalculateStats();
					ResetWeaponAnimation();
					return 1;
				}
				else
					return AddInventory(Item);
			}
			else {
				if(!HasMainHand()) {
					Inventory[INVENTORY_MAINHAND] = Item;
					RecalculateStats();
					ResetWeaponAnimation();
					return 1;
				}
				else if(!HasOffHand()) {
					Inventory[INVENTORY_OFFHAND] = Item;
					return 1;
				}
				else
					return AddInventory(Item);
			}
		} break;
		case _Object::ARMOR: {
			if(!HasArmor()) {
				Inventory[INVENTORY_ARMOR] = Item;
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

			// Add pickup bonus
			int PickupAmount = Item->Attributes["amount"].Int * PickupModifier;

			int AmountToMax = AmmoMax[Item->ID] - Ammo[Item->ID];
			AmountAdded = std::min(AmountToMax, PickupAmount);
			Ammo[Item->ID] += AmountAdded;

			return 2;
		}
		case _Object::KEY: {
			Keys[Item->ID] = 1;
			AmountAdded = 1;
			return 2;
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
				if(Slot == INVENTORY_MELEE) {
					if(Item->IsMelee())
						return true;
				}
				else
					return !Item->IsMelee();
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

			// Try mod
			if(!AddMod(SlotFrom, SlotTo)) {
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

	if(FromItem && ToItem && FromItem->CanStack() && ToItem->CanStack() && FromItem->ID == ToItem->ID && FromItem->Level == ToItem->Level) {
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
	if(!Item)
		return 0;

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

// Add a mod to a weapon
bool _Player::AddMod(int FromIndex, int ToIndex) {
	if(!HasInventory(FromIndex) || Inventory[FromIndex]->Type != _Object::MOD)
		return false;

	_Item *Item = Inventory[ToIndex];
	if(!Item)
		return false;

	if(Item->AddMod(Inventory[FromIndex])) {
		ConsumeInventory(FromIndex, false);
		RecalculateStats();
		return true;
	}

	return false;
}

// Calculates the radius of the crosshair
float _Player::GetCrosshairRadius(const glm::vec2 &Cursor) {

	// Check bounds
	float Accuracy = CurrentAccuracy;
	if(Accuracy < 0.0f)
		Accuracy = 0.0f;
	else if(Accuracy > PLAYER_MAXACCURACY)
		Accuracy = PLAYER_MAXACCURACY;

	// Get distance to cursor
	float Distance = glm::length(Cursor - Position);

	return tan(glm::radians(Accuracy * 0.5f)) * Distance;
}

// Checks if the player's weapon has ammo
bool _Player::WeaponHasAmmo(int AttackType) const {
	if(AttackType == WEAPONATTACK_MAIN) {
		if(!HasMainHand() || Stats.Objects.at(GetMainHand()->ID).AmmoID == "")
			return true;

		return GetMainHand()->Attributes.at("ammo").Int > 0;
	}
	else if(AttackType == WEAPONATTACK_MELEE) {
		return true;
	}

	return false;
}

// Checks if the player has ammo for the main weapon
bool _Player::HasAmmoForMain() const {
	if(!HasMainHand())
		return false;

	const std::string &AmmoType = Stats.Objects.at(GetMainHand()->ID).AmmoID;
	if(Ammo.find(AmmoType) == Ammo.end())
		return false;

	return Ammo.at(AmmoType) > 0;
}

// Reduces the player's mainhand weapon ammo and returns the amount reduced
int _Player::ReduceAmmo(int Amount) {
	if(HasMainHand() && AttackRequestType == WEAPONATTACK_MAIN) {
		Amount = std::min(GetMainHand()->Attributes["ammo"].Int, Amount);
		GetMainHand()->Attributes["ammo"].Int = std::max(GetMainHand()->Attributes["ammo"].Int - Amount, 0);
	}

	return Amount;
}

// Uses an item from the player's inventory, return true if a key was used
bool _Player::UseItem(int Index, bool Event) {
	if(Index < INVENTORY_BAGSTART || Index >= INVENTORY_BAGEND)
		return false;

	if(!HasInventory(Index))
		return false;

	switch(Inventory[Index]->Type) {
		case _Object::MEDKIT:
			UseMedkit(Index);
		break;
		case _Object::KEY:
			if(Event) {
				if(Map->MapType != MAPTYPE_CAMPAIGN)
					ConsumeInventory(Index);
				return true;
			}
		break;
	}

	return false;
}

// Uses a medkit if one is available
bool _Player::UseMedkit(int Index) {
	if(CanUseMedkit() && HasInventory(Index) && Inventory[Index]->Type == _Object::MEDKIT) {
		int HealAmount = Inventory[Index]->Attributes.at("health_restored").Int * HealModifier;
		UpdateHealth(HealAmount);
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
		if(HasInventory(i) && Inventory[i]->ID == ID)
			return i;
	}

	return -1;
}

// Begins the reloading process
void _Player::StartReloading() {

	// Test conditions
	if(!CanReload())
		return;

	// Stop reloading sound
	CancelReloading();

	// Play sound
	ReloadSound = ae::Audio.PlaySound(GetSound(SOUND_RELOAD, WEAPONATTACK_MAIN));

	// Start timer
	ReloadTimer = 0;
	Reloading = true;
}

// Cancel the reload process
void _Player::CancelReloading() {

	// Stop existing sound
	if(ReloadSound && ReloadSound->IsPlaying())
		ReloadSound->Stop();

	ReloadSound = nullptr;
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
	if(!Reloading || ReloadTimer <= ReloadPeriod)
		return;

	Reloading = false;

	// Check weapon type
	if(!CanReload())
		return;

	// Get amounts
	const std::string &AmmoType = Stats.Objects.at(GetMainHand()->ID).AmmoID;
	int AmountNeeded = GetMainHand()->Attributes["rounds"].Int - GetMainHand()->Attributes["ammo"].Int;
	int AmmoLoadAmount = std::min(Ammo[AmmoType], AmountNeeded);

	// Handle different reload amounts
	bool ReloadAgain = false;
	if(GetMainHand()->Attributes["reload_amount"].Int) {
		AmmoLoadAmount = std::min(GetMainHand()->Attributes["reload_amount"].Int, AmmoLoadAmount);
		ReloadAgain = true;
	}

	// Update ammo
	GetMainHand()->Attributes["ammo"].Int += AmmoLoadAmount;
	Ammo[AmmoType] -= AmmoLoadAmount;

	// Reset player
	ResetAccuracy(false);
	ResetWeaponAnimation();
	if(ReloadAgain)
		StartReloading();
}

// Switches weapons when the timer goes off
void _Player::UpdateWeaponSwitch() {
	if(!SwitchingWeapons || WeaponSwitchTimer <= WeaponSwitchPeriod)
		return;

	SwitchingWeapons = false;

	// Check weapon type
	if(!CanSwitchWeapons())
	   return;

	_Item *Temp = Inventory[WeaponSwitchFrom];
	Inventory[WeaponSwitchFrom] = Inventory[WeaponSwitchTo];
	Inventory[WeaponSwitchTo] = Temp;

	RecalculateStats();
	ResetWeaponAnimation();
}

// Updates the move speed modifier for aiming and running
void _Player::UpdateSpeed(float Factor) {

	if(Aiming)
		MoveModifier = PLAYER_AIM_MOVESPEEDFACTOR;
	else if(Sprinting)
		MoveModifier = PLAYER_SPRINT_SPEEDFACTOR;
	else
		MoveModifier = 1.0f;

	MoveModifier *= Factor;

	if(Action == ACTION_SHOOT || Action == ACTION_MELEE)
		MoveModifier *= AttackMoveSpeed[AttackRequestType];

	LegAnimation->FramePeriod = LegAnimation->Reels[0]->FramePeriod / MoveModifier;
	if(Animation->Reel == PLAYER_ANIMATIONWALKINGONEHAND || Animation->Reel == PLAYER_ANIMATIONWALKINGTWOHAND)
		SetAnimationPlaybackSpeedFactor();
}

// Updates the states for aiming
void _Player::SetAiming(bool State) {

	// Update state
	if(Aiming != State) {
		Aiming = State;
		ResetAccuracy(false);
		UpdateSpeed(1.0f);
	}

	// Disable sprint
	if(Aiming)
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
		SetAiming(false);
}

// Resets the accuracy depending on aiming state
void _Player::ResetAccuracy(bool CompleteReset) {

	if(Aiming && !IsMelee())
		RecoilModifier = PLAYER_AIM_RECOIL_MODIFIER;
	else if(Sprinting && !IsMelee())
		RecoilModifier = PLAYER_SPRINT_RECOIL_MODIFIER;
	else
		RecoilModifier = 1.0f;

	if(CompleteReset) {
		CurrentAccuracy = 0;
		if(!IsMelee())
			CurrentAccuracy = MaxAccuracyNormal;
	}

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

// Reset after death
void _Player::Respawn() {
	Health = MaxHealth;
	Action = ACTION_IDLE;
	Animation->Play(0);
	Animation->Stop();
	LegAnimation->Stop();

	RecalculateStats();
	ResetWeaponAnimation();
	StopAudio();
	SetPosition(Map->GetStartingPositionByCheckpoint(CheckpointIndex));

	InvulnerableTimer = GAME_INVULNERABLE_TIME;
}

// Sets the weapon animation for the player
void _Player::ResetWeaponAnimation() {
	if(IsDying())
		return;

	// Get walking animation
	switch(MainWeaponType) {
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
	Animation->Play(WalkingAnimation, MoveSpeed);
	Animation->CalculateTextureCoords();
	SetAnimationPlaybackSpeedFactor();
}

// Applies the death penalty
void _Player::ApplyDeathPenalty() {
	Reloading = false;
	SwitchingWeapons = false;
	Deaths++;

	Experience -= ExperienceLost;
	CalculateExperienceStats();
}

// Returns a sound index
const ae::_Sound *_Player::GetSound(int SoundType, int AttackType) const {
	if(AttackType < 0)
		return Sounds[SoundType];

	if(AttackType == WEAPONATTACK_MAIN && HasMainHand())
		return GetMainHand()->Template.SoundID[SoundType];
	else if(AttackType == WEAPONATTACK_MELEE && HasMelee())
		return GetMelee()->Template.SoundID[SoundType];

	return Sounds[SoundType];
}

// Get a particle from either the main weapon or player group
const _ParticleTemplate *_Player::GetParticle(int Index) const {
	if(Index == PARTICLE_HIT || Index == PARTICLE_FLOORDECAL)
		return GameAssets.ParticleGroups.at("player").ParticleTemplates[Index];

	if(HasMainHand())
		return Stats.Objects.at(GetMainHand()->ID).ParticleGroup->ParticleTemplates[Index];

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
	return HasMainHand() && AttackAllowed[WEAPONATTACK_MAIN] && !Reloading && !SwitchingWeapons && !IsMeleeAttacking() && GetMainHand()->Attributes.at("ammo").Int != GetMainHand()->Attributes.at("rounds").Int && HasAmmoForMain();
}

bool _Player::IsMelee() const { return GetMainHand() == nullptr || GetMainHand()->IsMelee(); }

void _Player::SetLegAnimationPlayMode(int Mode) {
	if(Mode == ae::_Animation::PLAYING)
		LegAnimation->Play(0);
	else if(Mode == ae::_Animation::STOPPED)
		LegAnimation->Stop();
}

void _Player::SetAnimationPlaybackSpeedFactor() {
	Animation->FramePeriod = Animation->Reels[Animation->Reel]->FramePeriod / MoveModifier;
}
