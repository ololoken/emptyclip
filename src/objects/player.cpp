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
#include <ae/buffer.h>
#include <ae/texture.h>
#include <ae/graphics.h>
#include <ae/assets.h>
#include <ae/animation.h>
#include <ae/program.h>
#include <audio.h>
#include <gameassets.h>
#include <stats.h>
#include <map.h>
#include <constants.h>
#include <ae/ui.h>
#include <objects/monster.h>
#include <objects/weapon.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

enum SaveChunkTypes {
	CHUNK_SAVEVERSION,
	CHUNK_PLAYERNAME,
	CHUNK_COLOR,
	CHUNK_MAP,
	CHUNK_PROGRESSION,
	CHUNK_CHECKPOINT,
	CHUNK_EXPERIENCE,
	CHUNK_GOLD,
	CHUNK_HEALTH,
	CHUNK_TIME_PLAYED,
	CHUNK_MONSTER_KILLS,
	CHUNK_SKILLS,
	CHUNK_ITEMS,
};

// Write a chunk to a stream
static void WriteChunk(std::ofstream &File, int Type, const char *Data, size_t Size) {
	File.write((char *)&Type, sizeof(Type));
	File.write((char *)&Size, sizeof(Size));
	File.write(Data, Size);
}

// Constructor
_Player::_Player(const std::string &SavePath) {
	this->SavePath = SavePath;
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

	// Set samples
	AttackSampleTemplateStruct *AttackSample = GameAssets.GetAttackSampleTemplate("player0");
	for(int i = 0; i < SAMPLE_TYPES; i++) {
		if(AttackSample)
			Samples[i] = AttackSample->Samples[i];
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
	MonsterKills = TimePlayed = 0;
	Radius = PLAYER_RADIUS;
	Name = "test";
	ColorIdentifier = "white";
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

	// Reset state
	MapIdentifier = GAME_STARTLEVEL;
	CheckpointIndex = 0;
	Progression = 0;
	Active = true;
	Action = ACTION_IDLE;
	Reloading = SwitchingWeapons = Crouching = Sprinting = AttackRequested = UseRequested = MedkitRequested = false;
	for(int i = 0; i < WEAPONATTACK_COUNT; i++) {
		AttackAllowed[i] = true;
	}
	UsePeriod = PLAYER_USEPERIOD;
	ZoomScale = PLAYER_ZOOMSCALE;
	LegDirection = 0.0f;
	MovementSpeed = 0.0f;
	MoveState = MOVE_NONE;
	WeaponSwitchTimer = ReloadTimer = UseTimer = MedkitTimer = 0;
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

// Loads information from a file
void _Player::Load() {
	Reset();

	// Open file
	std::ifstream File(SavePath.c_str(), std::ios::in | std::ios::binary);
	if(!File)
		throw std::runtime_error("Cannot load save file: " + SavePath);

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		// Get chunk type
		int Type;
		File.read((char *)&Type, sizeof(Type));

		// Get chunk size
		size_t Size;
		File.read((char *)&Size, sizeof(Size));

		switch(Type) {
			case CHUNK_SAVEVERSION: {
				int SaveVersion;
				File.read((char *)&SaveVersion, sizeof(SaveVersion));

				if(SaveVersion != PLAYER_SAVEVERSION)
					throw std::runtime_error("Save version mismatch");
			} break;
			case CHUNK_PLAYERNAME: {
				char Buffer[1024];
				File.read(Buffer, Size);
				Buffer[Size] = 0;
				Name = Buffer;
				//std::cout << "Name: " << Name << std::endl;
			} break;
			case CHUNK_COLOR: {
				char Buffer[1024];
				File.read(Buffer, Size);
				Buffer[Size] = 0;
				ColorIdentifier = Buffer;
				//std::cout << "ColorIdentifier: " << ColorIdentifier << std::endl;
			} break;
			case CHUNK_MAP: {
				char Buffer[1024];
				File.read(Buffer, Size);
				Buffer[Size] = 0;
				MapIdentifier = Buffer;
				//std::cout << "MapIdentifier: " << MapIdentifier << std::endl;
			} break;
			case CHUNK_CHECKPOINT:
				File.read((char *)&CheckpointIndex, sizeof(CheckpointIndex));
				//std::cout << "CheckpointIndex: " << CheckpointIndex << std::endl;
			break;
			case CHUNK_PROGRESSION:
				File.read((char *)&Progression, sizeof(Progression));
				//std::cout << "Progression: " << Progression << std::endl;
			break;
			case CHUNK_GOLD:
				File.read((char *)&Gold, sizeof(Gold));
				//std::cout << "Gold: " << Gold << std::endl;
			break;
			case CHUNK_EXPERIENCE:
				File.read((char *)&Experience, sizeof(Experience));
				//std::cout << "Experience: " << Experience << std::endl;
			break;
			case CHUNK_HEALTH:
				File.read((char *)&Health, sizeof(Health));
				if(Health <= 0)
					Health = 1;
				//std::cout << "CurrentHealth: " << CurrentHealth << std::endl;
			break;
			case CHUNK_TIME_PLAYED: {
				File.read((char *)&TimePlayed, sizeof(TimePlayed));
				//std::cout << "TimePlayed: " << TimePlayed << std::endl;
			} break;
			case CHUNK_MONSTER_KILLS:
				File.read((char *)&MonsterKills, sizeof(MonsterKills));
				//std::cout << "MonsterKills: " << MonsterKills << std::endl;
			break;
			case CHUNK_SKILLS:
				File.read((char *)&Skills, sizeof(Skills));
				//for(int i = 0; i < SKILL_COUNT; i++) Skills[i] = 0;
				//std::cout << "Skills: " << Skills[0] << " " << Skills[1] << " " << Skills[2] << " " << Skills[3] << " " << Skills[4] << std::endl;
			break;
			case CHUNK_ITEMS: {
				ae::_Buffer Buffer(Size);
				File.read(&Buffer[0], Size);
				LoadItems(Buffer);
			} break;
			default:
				//std::cout << "Unknown chunk: " << Type << " size: " << Size << std::endl;
				File.ignore(Size);
			break;
		}
	}

	File.close();
	//std::cout << std::endl;

	CalculateExperienceStats();
	CalculateLevelPercentage();
	CalculateSkillsRemaining();
	UpdateColor();
	RecalculateStats();
	ResetWeaponAnimation();
	UpdateHealth(0);
}

// Saves information to a file
void _Player::Save() {

	// Open file
	std::ofstream File(SavePath.c_str(), std::ios::out | std::ios::binary);
	if(!File.is_open()) {
		throw std::runtime_error("Cannot create save file: " + SavePath);
	}

	WriteChunk(File, CHUNK_SAVEVERSION, (char *)&PLAYER_SAVEVERSION, sizeof(PLAYER_SAVEVERSION));
	WriteChunk(File, CHUNK_PLAYERNAME, Name.c_str(), Name.length());
	WriteChunk(File, CHUNK_COLOR, ColorIdentifier.c_str(), ColorIdentifier.length());
	if(Map) {
		WriteChunk(File, CHUNK_MAP, MapIdentifier.c_str(), MapIdentifier.length());
		WriteChunk(File, CHUNK_CHECKPOINT, (char *)&CheckpointIndex, sizeof(CheckpointIndex));
	}
	WriteChunk(File, CHUNK_PROGRESSION, (char *)&Progression, sizeof(Progression));
	WriteChunk(File, CHUNK_EXPERIENCE, (char *)&Experience, sizeof(Experience));
	WriteChunk(File, CHUNK_GOLD, (char *)&Gold, sizeof(Gold));
	WriteChunk(File, CHUNK_HEALTH, (char *)&Health, sizeof(Health));
	WriteChunk(File, CHUNK_TIME_PLAYED, (char *)&TimePlayed, sizeof(TimePlayed));
	WriteChunk(File, CHUNK_MONSTER_KILLS, (char *)&MonsterKills, sizeof(MonsterKills));
	WriteChunk(File, CHUNK_SKILLS, (char *)&Skills, sizeof(Skills));

	SaveItems(File);

	File.close();
}

// Loads items from a stream
void _Player::LoadItems(ae::_Buffer &Buffer) {

	// Get inventory size
	int ItemCount = Buffer.Read<int>();
	if(ItemCount > INVENTORY_SIZE) {
		throw std::runtime_error("Too many items");
	}

	// Get items
	for(int i = 0; i < ItemCount; i++) {
		int Slot = Buffer.Read<int>();
		int Type = Buffer.Read<int>();
		//int Quality = Buffer.Read<int>();
		Buffer.Read<int>();
		int Count = Buffer.Read<int>();
		std::string Identifier;

		// Create items
		switch(Type) {
			case _Object::KEY:
			case _Object::AMMO:
			case _Object::UPGRADE:
			case _Object::ARMOR:
			case _Object::MEDKIT:
				Identifier = Buffer.ReadString();
				Inventory[Slot] = Stats.CreateItem(Identifier, Count, glm::vec2(0, 0));
			break;
			case _Object::WEAPON:
				LoadWeapon(Buffer, Count, Slot);
			break;
		}
	}
}

// Loads weapons from a stream
void _Player::LoadWeapon(ae::_Buffer &Buffer, int Count, int InventoryIndex) {

	// Get weapons
	std::string Identifier = Buffer.ReadString();
	int Ammo = Buffer.Read<int>();
	int MaxComponents = Buffer.Read<int>();

	// Create weapon
	_Weapon *Weapon = Stats.CreateWeapon(Identifier, Count, glm::vec2(0, 0), false);
	Weapon->Attributes["max_components"].Int = MaxComponents;
	LoadUpgrades(Buffer, Weapon);
	Weapon->RecalculateStats();
	Weapon->SetAmmo(Ammo);

	Inventory[InventoryIndex] = Weapon;
}

// Loads upgrade components from a stream
void _Player::LoadUpgrades(ae::_Buffer &Buffer, _Weapon *Weapon) {

	// Get size header
	int Components = Buffer.Read<int>();

	// Read data
	for(int i = 0; i < Components; i++) {
		std::string Identifier = Buffer.ReadString();
		_Item *Item = Stats.CreateItem(Identifier, 1, glm::vec2(0, 0));
		if(!Weapon->AddComponent(Item))
			delete Item;
	}
}

// Saves items to a stream
void _Player::SaveItems(std::ofstream &File) {

	// Buffer
	int ItemCount = 0;
	for(int i = 0; i < INVENTORY_SIZE; i++) {
		if(HasInventory(i))
			ItemCount++;
	}

	// Write item count
	ae::_Buffer Buffer;
	Buffer.Write<int>(ItemCount);

	// Write items
	for(int i = 0; i < INVENTORY_SIZE; i++) {
		if(HasInventory(i)) {
			Buffer.Write(i);
			Buffer.Write(Inventory[i]->Type);
			Buffer.Write(Inventory[i]->Quality);
			Buffer.Write(Inventory[i]->Count);
			Inventory[i]->Serialize(Buffer);
		}
	}

	// Write chunk
	WriteChunk(File, CHUNK_ITEMS, &Buffer[0], Buffer.GetCurrentSize());
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

	// Check timer to see if the object can attack
	for(int i = 0; i < WEAPONATTACK_COUNT; i++) {
		if(!AttackAllowed[i] && FireTimer[i] >= FirePeriod[i])
			AttackAllowed[i] = true;
	}

	// Update states
	UpdateAnimation(FrameTime);
	UpdateRecoil();
	UpdateReloading();
	UpdateWeaponSwitch();

	// Stop trigger down audio
	if(TriggerDownAudio && (!AttackRequested || !HasAmmo() || IsDying() || SwitchingWeapons || Reloading)) {
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

	if(TriggerDownAudio)
		TriggerDownAudio->SetPosition(Position);
}

// Updates the leg's animation and direction
void _Player::UpdateAnimation(double FrameTime, bool PlaySound) {
	::_Entity::UpdateAnimation(FrameTime, false);

	int LastFrame = LegAnimation->Frame;
	LegAnimation->Update(FrameTime);

	// Play move sound on first and last frame of leg animation
	if(PlaySound && LastFrame != LegAnimation->Frame && (LegAnimation->Frame == 0 || LegAnimation->Frame == LegAnimation->Reels[LegAnimation->Reel]->EndFrame)) {
		Audio.Play(new _AudioSource(Audio.GetBuffer(GetSample(SAMPLE_MOVE)), true));
	}

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

// Adds an item to the player's possession, returns 0 on full, return 2 on combine
int _Player::AddItem(_Item *Item) {

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
			if(!HasArmor() && Stats.GetSkill(Skills[SKILL_STRENGTH], SKILL_STRENGTH) >= Item->Attributes.at("strength_required").Int) {
				SetArmor(Item);
				RecalculateStats();
				return 1;
			}
			else {
				return AddInventory(Item);
			}
		} break;
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

			return Stats.GetSkill(Skills[SKILL_STRENGTH], SKILL_STRENGTH) >= Item->Attributes.at("strength_required").Int;
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
	_Weapon *Weapon = nullptr;

	if(!HasInventory(FromIndex) || Inventory[FromIndex]->Type != _Object::UPGRADE)
		return false;

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
	float Distance, Accuracy;

	// Check bounds
	Accuracy = CurrentAccuracy * AccuracyModifier;
	if(Accuracy < 0.0f)
		Accuracy = 0.0f;
	else if(Accuracy > PLAYER_MAXACCURACY)
		Accuracy = PLAYER_MAXACCURACY;

	// Get distance to cursor
	Distance = glm::length(Cursor - Position);

	return tan(glm::radians(Accuracy * 0.5f)) * Distance;
}

// Determines what type of ammo is required by the weapon the player is using
int _Player::GetWeaponAmmoType() const {
	if(!HasMainHand())
		return 0;

	return GetMainHand()->Attributes.at("ammo_type").Int;
}

// Determines what type of ammo an item in the inventory is
int _Player::GetInventoryAmmoType(int Index) const {
	if(HasInventory(Index) && Inventory[Index]->Type == _Object::AMMO)
		return Inventory[Index]->Attributes.at("ammo_type").Int;

	return -1;
}

// Checks if the item is the right ammo for the player's mainhand weapon
bool _Player::IsRightClip(const _Item *Item) const {
	if(Item->Type == _Object::AMMO) {
		if(Item->Attributes.at("ammo_type").Int == GetWeaponAmmoType())
			return true;
	}

	return false;
}

// Checks if the player's weapon has ammo
bool _Player::HasAmmo() const {

	if(AttackRequestType == WEAPONATTACK_MAIN) {
		if(!HasMainHand() || GetMainHand()->Attributes.at("ammo_type").Int == 0)
			return true;

		return GetMainHand()->Attributes.at("ammo").Int > 0;
	}
	else if(AttackRequestType == WEAPONATTACK_MELEE) {
		return true;
	}

	return false;
}

// Checks if the player has ammo for the current weapon
bool _Player::HasClips() const {

	// Search inventory for clip
	for(int i = INVENTORY_BAGSTART; i < INVENTORY_BAGEND; i++) {
		if(HasInventory(i) && IsRightClip(Inventory[i])) {
			return true;
		}
	}

	return false;
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
int _Player::FindItem(const std::string &Identifier) {

	for(int i = INVENTORY_BAGSTART; i < INVENTORY_BAGEND; i++) {
		if(HasInventory(i) && Inventory[i]->ID == Identifier) {
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

	Audio.Play(new _AudioSource(Audio.GetBuffer(GetSample(SAMPLE_RELOAD)), true));

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

		// Search inventory for clip
		for(int i = INVENTORY_BAGSTART; i < INVENTORY_BAGEND; i++) {
			if(HasInventory(i) && IsRightClip(Inventory[i])) {
				GetMainHand()->SetAmmo(GetMainHand()->Attributes.at("rounds").Int);
				ConsumeInventory(i);

				// Update accuracy
				ResetAccuracy(true);
				ResetWeaponAnimation();
				return;
			}
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
	_WeaponTemplate Weapon[WEAPONATTACK_COUNT];
	for(int i = 0; i < WEAPONATTACK_COUNT; i++)
		Weapon[i] = Stats.Weapons["fists"];

	// See if the player is using a weapon
	if(HasMainHand()) {
		for(const auto &Attribute : GetMainHand()->Attributes)
			Weapon[WEAPONATTACK_MAIN].Attributes[Attribute.first] = Attribute.second;

		MainWeaponType = GetMainHand()->Attributes.at("weapon_type").Int;
	}
	else
		MainWeaponType = WEAPON_MELEE;

	// Get stats of melee weapon
	if(HasMelee()) {
		for(const auto &Attribute : GetMelee()->Attributes)
			Weapon[WEAPONATTACK_MELEE].Attributes[Attribute.first] = Attribute.second;
	}

	// Set up main stats based on weapon
	Recoil = 0;
	RecoilRegen = 0;
	AttackRange[WEAPONATTACK_MAIN] = Weapon[WEAPONATTACK_MAIN].Attributes["range"].Float;
	AttackRange[WEAPONATTACK_MELEE] = Weapon[WEAPONATTACK_MELEE].Attributes["range"].Float;
	if(MainWeaponType == WEAPON_MELEE) {
		CurrentAccuracyNormal = MinAccuracyNormal = Weapon[WEAPONATTACK_MAIN].Attributes.at("min_accuracy").Float;
		MaxAccuracyNormal = Weapon[WEAPONATTACK_MAIN].Attributes.at("max_accuracy").Float;
	}
	else {
		float AccuracySkillMultiplier = 1.0f / Stats.GetSkill(Skills[SKILL_ACCURACY], SKILL_ACCURACY);
		CurrentAccuracyNormal = MinAccuracyNormal = Weapon[WEAPONATTACK_MAIN].Attributes.at("min_accuracy").Float * AccuracySkillMultiplier;
		MaxAccuracyNormal = Weapon[WEAPONATTACK_MAIN].Attributes.at("max_accuracy").Float * AccuracySkillMultiplier;
		Recoil = Weapon[WEAPONATTACK_MAIN].Attributes["recoil"].Float;
		RecoilRegen = Weapon[WEAPONATTACK_MAIN].Attributes["recoil_regen"].Float;
	}

	MaxAccuracy[WEAPONATTACK_MELEE] = Weapon[WEAPONATTACK_MELEE].Attributes.at("max_accuracy").Float;

	// Set accuracy
	ResetAccuracy(true);

	// Attacking
	for(int i = 0; i < WEAPONATTACK_COUNT; i++) {
		FireRate[i] = Weapon[i].Attributes["fire_rate"].Int;
		FirePeriod[i] = Weapon[i].Attributes["fire_period"].Double / Stats.GetSkill(Skills[SKILL_ATTACKSPEED], SKILL_ATTACKSPEED);
		MinDamage[i] = Weapon[i].Attributes["min_damage"].Int;
		MaxDamage[i] = Weapon[i].Attributes["max_damage"].Int;
	}
	ReloadPeriod = Weapon[WEAPONATTACK_MAIN].Attributes["reload_period"].Double / Stats.GetSkill(Skills[SKILL_RELOADSPEED], SKILL_RELOADSPEED);
	WeaponSwitchPeriod = PLAYER_WEAPONSWITCHPERIOD * 1;
	AttackCount = Weapon[WEAPONATTACK_MAIN].Attributes["attack_count"].Int;
	ZoomScale = Weapon[WEAPONATTACK_MAIN].Attributes["zoom_scale"].Float;

	// Cap fire period
	if(FirePeriod[WEAPONATTACK_MAIN] < WEAPON_MINFIREPERIOD)
		FirePeriod[WEAPONATTACK_MAIN] = WEAPON_MINFIREPERIOD;

	MovementSpeed = Stats.GetSkill(Skills[SKILL_MOVESPEED], SKILL_MOVESPEED);
	DamageResist = Stats.GetSkill(Skills[SKILL_DAMAGERESIST], SKILL_DAMAGERESIST) - 1.0f;
	MaxHealth = (int)(Stats.GetLevelHealth(Level) * Stats.GetSkill(Skills[SKILL_HEALTH], SKILL_HEALTH));
	MaxStamina = 1.0f * Stats.GetSkill(Skills[SKILL_MAXSTAMINA], SKILL_MAXSTAMINA);
	StaminaRegenModifier = 1.0f * Stats.GetSkill(Skills[SKILL_MAXSTAMINA], SKILL_MAXSTAMINA);

	// Armor
	DamageBlock = Stats.GetLevelDamageBlock(Level);
	if(GetArmor()) {
		DamageBlock += GetArmor()->Attributes.at("damage_block").Int;
		DamageResist += GetArmor()->Attributes.at("damage_resist").Float;
		MovementSpeed += GetArmor()->Attributes.at("move_speed").Float;
	}

	// Get final speed
	MovementSpeed *= PLAYER_MOVEMENTSPEED;
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

// Returns a sample index
const std::string &_Player::GetSample(int SampleType) const {

	if(AttackRequestType == 0 && SampleType <= SAMPLE_HIT && HasMainHand())
		return GetMainHand()->GetSample(SampleType);
	else if(AttackRequestType == 1 && SampleType <= SAMPLE_HIT && HasMelee())
		return GetMelee()->GetSample(SampleType);
	else
		return Samples[SampleType];
}

// Returns the weapon's particle template
const _ParticleTemplate *_Player::GetWeaponParticle(int Index) const {
	if(HasMainHand())
		return Stats.Weapons[GetMainHand()->ID].WeaponParticles->ParticleTemplates[Index];

	return nullptr;
}

// Sets the color string and color of the player
void _Player::UpdateColor() {
	if(ae::Assets.Colors.find(ColorIdentifier) != ae::Assets.Colors.end())
		Color = ae::Assets.Colors[ColorIdentifier];
	else
		Color = COLOR_WHITE;
}

int _Player::GetInventoryMaxStack() const {
	return Stats.GetSkill(Skills[SKILL_MAXINVENTORY], SKILL_MAXINVENTORY) + 1;
}

bool _Player::CanUseMedkit() const {
	return (MedkitTimer > PLAYER_MEDKITPERIOD) && Health < MaxHealth;
}

bool _Player::CanReload() const {
	return HasMainHand() && !Reloading && !SwitchingWeapons && !IsMeleeAttacking() && GetMainHand()->Attributes.at("ammo").Int != GetMainHand()->Attributes.at("rounds").Int && HasClips();
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
