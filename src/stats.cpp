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
#include <stats.h>
#include <constants.h>
#include <fstream>
#include <sstream>
#include <assets.h>
#include <stdexcept>
#include <random.h>
#include <objects/object.h>
#include <objects/weapon.h>

_Stats Stats;

// Initialize
void _Stats::Init() {
	LoadLevels("tables/levels.tsv");
	LoadSkills("tables/skills.tsv");
	LoadAmmoTable("tables/ammo.tsv");
	LoadArmorTable("tables/armor.tsv");
	LoadMiscItemTable("tables/items.tsv");
	LoadUpgradeTable("tables/upgrades.tsv");
	LoadWeaponTable("tables/weapons.tsv");
	LoadItemDrops("tables/itemdrops.tsv");
}

// Shutdown
void _Stats::Close() {
	Levels.clear();
	Skills.clear();
	Items.clear();
	MiscItems.clear();
	Weapons.clear();
	ItemGroupTable.clear();
}

// Loads the level table
void _Stats::LoadLevels(const std::string &Path) {

	// Open file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Load data
	Levels.clear();
	for(int i = 0; i < GAME_MAX_LEVEL; i++) {
		if(File.eof())
			throw std::runtime_error("LoadLevels - Premature end of file");

		_Level Level;
		File >> Level.Experience >> Level.HealthBonus >> Level.DamageBlockBonus >> Level.SkillPoints;

		Levels.push_back(Level);
	}
}

// Loads the skill table
void _Stats::LoadSkills(const std::string &Path) {
	_Skill Skill;

	// Load file
	std::ifstream InputFile(Path, std::ios::in);
	if(!InputFile)
		throw std::runtime_error("LoadSkills: Cannot open " + Path);

	Skills.clear();

	// Load the data
	InputFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	for(int i = 0; i < GAME_SKILLLEVELS+1; i++) {
		if(InputFile.eof())
			throw std::runtime_error("Premature end of file" + Path);

		for(int i = 0; i < SKILL_COUNT; i++)
			InputFile >> Skill.Data[i];

		Skills.push_back(Skill);
	}
}

// Loads the ammo table
void _Stats::LoadAmmoTable(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	AmmoNames.push_back("None");

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		_ItemTemplate Template(_Object::AMMO);
		std::string Name;
		std::string ColorName;
		std::getline(File, Name, '\t');
		std::getline(File, Template.Name, '\t');
		std::getline(File, Template.IconIdentifier, '\t');
		std::getline(File, ColorName, '\n');

		// Check for loaded textures
		if(!Assets.IsTextureLoaded(Template.IconIdentifier))
			throw std::runtime_error(std::string(__FUNCTION__) + " - Texture not found: " + Template.IconIdentifier);

		// Set color
		if(ColorName != "") {
			if(!Assets.IsColorLoaded(ColorName))
				throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find color: " + ColorName);

			Template.Color = Assets.Colors[ColorName];
		}
		else
			Template.Color = COLOR_WHITE;

		// Check for duplicates
		if(Items.find(Name) != Items.end())
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Name);

		Template.Attributes["ammo_type"].Int = AmmoNames.size();
		Items[Name] = Template;
		AmmoNames.push_back(Name);
	}

	File.close();
}

// Loads the armor table
void _Stats::LoadArmorTable(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		_ItemTemplate Template(_Object::ARMOR);
		std::string Name;
		std::string ColorName;
		std::getline(File, Name, '\t');
		std::getline(File, Template.Name, '\t');
		std::getline(File, Template.IconIdentifier, '\t');
		std::getline(File, ColorName, '\t');

		File
			>> Template.Attributes["strength_required"].Int
			>> Template.Attributes["damage_block"].Int
			>> Template.Attributes["damage_resist"].Float
			>> Template.Attributes["move_speed"].Float;

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for loaded textures
		if(!Assets.IsTextureLoaded(Template.IconIdentifier))
			throw std::runtime_error(std::string(__FUNCTION__) + " - Texture not found: " + Template.IconIdentifier);

		// Set color
		if(ColorName != "") {
			if(!Assets.IsColorLoaded(ColorName))
				throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find color: " + ColorName);

			Template.Color = Assets.Colors[ColorName];
		}
		else
			Template.Color = COLOR_WHITE;

		// Check for duplicates
		if(Items.find(Name) != Items.end())
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Name);

		Items[Name] = Template;
	}

	File.close();
}

// Loads the misc item table
void _Stats::LoadMiscItemTable(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		_MiscItemTemplate MiscItemTemplate;
		std::string Identifier;
		std::string ColorName;
		std::getline(File, Identifier, '\t');
		std::getline(File, MiscItemTemplate.Name, '\t');
		std::getline(File, MiscItemTemplate.IconIdentifier, '\t');
		std::getline(File, ColorName, '\t');

		File >> MiscItemTemplate.Type >> MiscItemTemplate.Level;

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for loaded textures
		if(!Assets.IsTextureLoaded(MiscItemTemplate.IconIdentifier))
			throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find texture: " + MiscItemTemplate.IconIdentifier);

		// Set color
		if(ColorName != "") {
			if(!Assets.IsColorLoaded(ColorName))
				throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find color: " + ColorName);

			MiscItemTemplate.Color = Assets.Colors[ColorName];
		}
		else
			MiscItemTemplate.Color = COLOR_WHITE;

		// Check for duplicates
		if(MiscItems.find(Identifier) != MiscItems.end())
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Identifier);

		MiscItems[Identifier] = MiscItemTemplate;
	}

	File.close();
}

// Loads the upgrade table
void _Stats::LoadUpgradeTable(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		_ItemTemplate Template(_Object::UPGRADE);
		std::string Name;
		std::string ColorName;
		std::getline(File, Name, '\t');
		std::getline(File, Template.Name, '\t');
		std::getline(File, Template.IconIdentifier, '\t');
		std::getline(File, ColorName, '\t');

		File
			>> Template.Attributes["upgrade_type"].Int
			>> Template.Attributes["weapon_type"].Int
			>> Template.Attributes["bonus"].Float;

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for loaded textures
		if(!Assets.IsTextureLoaded(Template.IconIdentifier))
			throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find texture: " + Template.IconIdentifier);

		// Set color
		if(ColorName != "") {
			if(!Assets.IsColorLoaded(ColorName))
				throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find color: " + ColorName);

			Template.Color = Assets.Colors[ColorName];
		}
		else
			Template.Color = COLOR_WHITE;

		// Check for duplicates
		if(Items.find(Name) != Items.end())
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Name);

		Items[Name] = Template;
	}

	File.close();
}

// Loads the weapon table
void _Stats::LoadWeaponTable(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {
		_WeaponTemplate WeaponTemplate;
		AttackSampleTemplateStruct *AttackSample;

		std::string Name;
		std::string ColorName;
		std::string SamplesIdentifier;
		std::string WeaponParticlesIdentifier;
		std::getline(File, Name, '\t');
		std::getline(File, WeaponTemplate.Name, '\t');
		std::getline(File, WeaponTemplate.IconIdentifier, '\t');
		std::getline(File, SamplesIdentifier, '\t');
		std::getline(File, WeaponParticlesIdentifier, '\t');
		std::getline(File, ColorName, '\t');

		File	>> WeaponTemplate.Attributes["weapon_type"].Int
				>> WeaponTemplate.Attributes["zoom_scale"].Float
				>> WeaponTemplate.Attributes["min_accuracy"].Float
				>> WeaponTemplate.Attributes["max_accuracy"].Float
				>> WeaponTemplate.Attributes["recoil"].Float
				>> WeaponTemplate.Attributes["recoil_regen"].Float
				>> WeaponTemplate.Attributes["range"].Float
				>> WeaponTemplate.Attributes["fire_rate"].Int
				>> WeaponTemplate.Attributes["fire_period"].Double
				>> WeaponTemplate.Attributes["reload_period"].Double
				>> WeaponTemplate.Attributes["min_components"].Int
				>> WeaponTemplate.Attributes["max_components"].Int
				>> WeaponTemplate.Attributes["min_damage"].Int
				>> WeaponTemplate.Attributes["max_damage"].Int
				>> WeaponTemplate.Attributes["attack_count"].Int
				>> WeaponTemplate.Attributes["rounds"].Int
				>> WeaponTemplate.Attributes["ammo_type"].Int;

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for loaded textures
		if(!Assets.IsTextureLoaded(WeaponTemplate.IconIdentifier))
			throw std::runtime_error(std::string(__FUNCTION__) + " - Texture not found: " + WeaponTemplate.IconIdentifier);

		// Set color
		if(ColorName != "") {
			if(!Assets.IsColorLoaded(ColorName))
				throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find color: " + ColorName);

			WeaponTemplate.Color = Assets.Colors[ColorName];
		}
		else
			WeaponTemplate.Color = COLOR_WHITE;

		// Check for attack sample
		if(!Assets.IsAttackSampleLoaded(SamplesIdentifier))
			throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find sample: " + SamplesIdentifier);

		// Set samples
		AttackSample = Assets.GetAttackSampleTemplate(SamplesIdentifier);
		for(int i = 0; i < SAMPLE_TYPES; i++) {
			if(AttackSample)
				WeaponTemplate.Samples[i] = AttackSample->Samples[i];
		}

		// Set particles
		if(Assets.IsWeaponParticleTemplateLoaded(WeaponParticlesIdentifier))
			WeaponTemplate.WeaponParticles = Assets.GetWeaponParticleTemplate(WeaponParticlesIdentifier);
		else
			WeaponTemplate.WeaponParticles = &Assets.BlankWeaponParticle;

		// Check for duplicates
		if(Weapons.find(Name) != Weapons.end())
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Name);

		Weapons[Name] = WeaponTemplate;
	}

	File.close();
}

// Load item drop table
void _Stats::LoadItemDrops(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip first two fields
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\t');
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\t');

	// Read rest of line into buffer
	std::string Line;
	std::getline(File, Line, '\n');
	std::stringstream Buffer(Line);

	// Get item drop names first
	int ItemDrops = 0;
	std::vector<std::string> ItemDropNames;
	std::string DropName;
	while(std::getline(Buffer, DropName, '\t')) {
		if(DropName == "")
			continue;

		ItemDropNames.push_back(DropName);

		auto ItemGroupTableIterator = ItemGroupTable.find(DropName);
		if(ItemGroupTableIterator == ItemGroupTable.end()) {
			_ItemGroup ItemGroup;
			ItemGroup.Total = 0;
			ItemGroup.Quantity = 1;
			ItemGroupTable[DropName] = ItemGroup;
		}

		ItemDrops++;
	}

	// Read rest of data
	while(!File.eof() && File.peek() != EOF) {

		_ItemGroupEntry ItemGroupEntry;
		File >> ItemGroupEntry.Type;
		File.ignore(1, '\t');
		std::getline(File, ItemGroupEntry.ItemIdentifier, '\t');

		// See if items exist
		switch(ItemGroupEntry.Type) {
			case -1:
			break;
			case _Object::MEDKIT:
				if(MiscItems.find(ItemGroupEntry.ItemIdentifier) == MiscItems.end())
					throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find: " + ItemGroupEntry.ItemIdentifier + " in " + Path);
			break;
			case _Object::AMMO:
				if(Items.find(ItemGroupEntry.ItemIdentifier) == Items.end())
					throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find: " + ItemGroupEntry.ItemIdentifier + " in " + Path);
			break;
			case _Object::UPGRADE:
				if(Items.find(ItemGroupEntry.ItemIdentifier) == Items.end())
					throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find: " + ItemGroupEntry.ItemIdentifier + " in " + Path);
			break;
			case _Object::WEAPON:
				if(Weapons.find(ItemGroupEntry.ItemIdentifier) == Weapons.end())
					throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find: " + ItemGroupEntry.ItemIdentifier + " in " + Path);
			break;
			case _Object::ARMOR:
				if(Items.find(ItemGroupEntry.ItemIdentifier) == Items.end())
					throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find: " + ItemGroupEntry.ItemIdentifier + " in " + Path);
			break;
			case _Object::KEY:
				if(MiscItems.find(ItemGroupEntry.ItemIdentifier) == MiscItems.end())
					throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find: " + ItemGroupEntry.ItemIdentifier + " in " + Path);
			break;
			default:
				throw std::runtime_error(std::string(__FUNCTION__) + " - Bad item type: " + ItemGroupEntry.ItemIdentifier + " in " + Path);
			break;
		}

		// Add counts to item groups
		for(int i = 0; i < ItemDrops; i++) {
			File >> ItemGroupEntry.Count;
			if(ItemGroupEntry.Count <= 0)
				continue;

			ItemGroupTable[ItemDropNames[i]].Total += ItemGroupEntry.Count;
			ItemGroupEntry.Count = ItemGroupTable[ItemDropNames[i]].Total;
			ItemGroupTable[ItemDropNames[i]].Entries.push_back(ItemGroupEntry);
		}

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}

	File.close();
}

// Creates ammo
_Item *_Stats::CreateAmmoItem(const std::string &Identifier, int Count, const glm::vec2 &Position) {
	_ItemTemplate &Template = Items[Identifier];

	_Item *Item = new _Item();
	Item->Attributes = Template.Attributes;
	Item->Type = _Object::AMMO;
	Item->Name = Template.Name;
	Item->ID = Identifier;
	Item->Count = Count;
	Item->Position = Position;
	Item->Texture = Assets.Textures[Template.IconIdentifier];
	Item->Color = Template.Color;

	return Item;
}

// Creates armor
_Item *_Stats::CreateArmor(const std::string &Identifier, int Count, const glm::vec2 &Position) {
	_ItemTemplate &Template = Items[Identifier];

	_Item *Item = new _Item();
	Item->Attributes = Template.Attributes;
	Item->Type = _Object::ARMOR;
	Item->Name = Template.Name;
	Item->ID = Identifier;
	Item->Count = Count;
	Item->Position = Position;
	Item->Texture = Assets.Textures[Template.IconIdentifier];
	Item->Color = Template.Color;

	return Item;
}

// Creates a misc item
_Item *_Stats::CreateMiscItem(const std::string &Identifier, int Count, const glm::vec2 &Position) {
	_MiscItemTemplate &MiscItemTemplate = MiscItems[Identifier];

	_Item *Item = new _Item();
	Item->Type = MiscItemTemplate.Type;
	Item->Name = MiscItemTemplate.Name;
	Item->ID = Identifier;
	Item->Count = Count;
	Item->Level = MiscItemTemplate.Level;
	Item->Position = Position;
	Item->Texture = Assets.Textures[MiscItemTemplate.IconIdentifier];
	Item->Color = MiscItemTemplate.Color;

	return Item;
}

// Creates a weapon
_Weapon *_Stats::CreateWeapon(const std::string &Identifier, int Count, const glm::vec2 &Position, bool Generate) {
	_WeaponTemplate &WeaponTemplate = Weapons[Identifier];
	_Weapon *Weapon = new _Weapon(Identifier, Count, Position, WeaponTemplate, Assets.Textures[WeaponTemplate.IconIdentifier], Generate);

	return Weapon;
}

// Creates an upgrade item
_Item *_Stats::CreateUpgradeItem(const std::string &Identifier, int Count, const glm::vec2 &Position) {
	_ItemTemplate &Template = Items[Identifier];

	_Item *Item = new _Item();
	Item->Attributes = Template.Attributes;
	Item->Type = _Object::UPGRADE;
	Item->Name = Template.Name;
	Item->ID = Identifier;
	Item->Count = Count;
	Item->Position = Position;
	Item->Texture = Assets.Textures[Template.IconIdentifier];
	Item->Color = Template.Color;

	return Item;
}

// Returns a valid amount of experience
int64_t _Stats::GetValidExperience(int64_t Experience) {

	if(Experience < 0)
		return 0;
	else if(Experience > Levels[GAME_MAX_LEVEL-1].Experience)
		return Levels[GAME_MAX_LEVEL-1].Experience;

	return Experience;
}

// Returns the level given the experience number
int _Stats::GetLevel(int64_t Experience) {

	// Degenerate case
	if(Experience <= 0)
		return 1;
	else if(Experience >= Levels[GAME_MAX_LEVEL-1].Experience)
		return GAME_MAX_LEVEL;

	// Perform linear search through array
	for(int i = 1; i < GAME_MAX_LEVEL; i++) {
		if(Experience < Levels[i].Experience)
			return i;
	}

	return 1;
}

// Returns the total experience required for a level
int64_t _Stats::GetExperienceForLevel(int Level) {

	// Degenerate case
	if(Level <= 0)
		return Levels[0].Experience;
	else if(Level > GAME_MAX_LEVEL)
		return 0;

	return  Levels[Level-1].Experience;
}

// Returns a skill value in a valid range
int _Stats::GetValidSkillLevel(int Level) {
	if(Level < 0)
		return 0;
	else if(Level >= GAME_SKILLLEVELS)
		return GAME_SKILLLEVELS;

	return Level;
}

// Returns a random item identifier from an item group
void _Stats::GetRandomDrop(const _ItemGroup *ItemGroup, _ObjectSpawn *ObjectSpawn) {
	ObjectSpawn->Type = -1;

	// Get item group
	size_t ItemGroupSize = ItemGroup->Entries.size();
	if(ItemGroupSize == 0)
		return;

	// Get total
	if(ItemGroup->Total <= 0.0f)
		return;

	// Generate roll
	float RandomNumber = GetRandomReal(0.0, ItemGroup->Total);

	// Get item
	for(size_t i = 0; i < ItemGroupSize; i++) {
		if(RandomNumber <= ItemGroup->Entries[i].Count) {
			ObjectSpawn->Type = ItemGroup->Entries[i].Type;
			ObjectSpawn->Identifier = ItemGroup->Entries[i].ItemIdentifier;
			return;
		}
	}
}
