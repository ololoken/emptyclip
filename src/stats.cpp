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
#include <objects/armor.h>
#include <objects/ammo.h>
#include <objects/misc.h>
#include <objects/upgrade.h>
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
	AmmoTable.clear();
	ArmorTable.clear();
	MiscItemTable.clear();
	UpgradeTable.clear();
	WeaponTable.clear();
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

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		_AmmoTemplate Ammo;
		std::string Name;
		std::string ColorName;
		std::getline(File, Name, '\t');
		std::getline(File, Ammo.Name, '\t');
		std::getline(File, Ammo.IconIdentifier, '\t');
		std::getline(File, ColorName, '\t');

		File >> Ammo.AmmoType;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for loaded textures
		if(!Assets.IsTextureLoaded(Ammo.IconIdentifier))
			throw std::runtime_error(std::string(__FUNCTION__) + " - Texture not found: " + Ammo.IconIdentifier);

		// Set color
		if(ColorName != "") {
			if(!Assets.IsColorLoaded(ColorName))
				throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find color: " + ColorName);

			Ammo.Color = Assets.Colors[ColorName];
		}
		else
			Ammo.Color = COLOR_WHITE;

		// Check for duplicates
		if(AmmoTable.find(Name) != AmmoTable.end())
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Name);

		AmmoTable[Name] = Ammo;
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

		_ArmorTemplate Armor;
		std::string Name;
		std::string ColorName;
		std::getline(File, Name, '\t');
		std::getline(File, Armor.Name, '\t');
		std::getline(File, Armor.IconIdentifier, '\t');
		std::getline(File, ColorName, '\t');

		File >> Armor.StrengthRequirement >> Armor.DamageBlock >> Armor.DamageResist >> Armor.MovementSpeed;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for loaded textures
		if(!Assets.IsTextureLoaded(Armor.IconIdentifier))
			throw std::runtime_error(std::string(__FUNCTION__) + " - Texture not found: " + Armor.IconIdentifier);

		// Set color
		if(ColorName != "") {
			if(!Assets.IsColorLoaded(ColorName))
				throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find color: " + ColorName);

			Armor.Color = Assets.Colors[ColorName];
		}
		else
			Armor.Color = COLOR_WHITE;

		// Check for duplicates
		if(ArmorTable.find(Name) != ArmorTable.end())
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Name);

		ArmorTable[Name] = Armor;
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

		_MiscItemTemplate MiscItem;
		std::string Identifier;
		std::string ColorName;
		std::getline(File, Identifier, '\t');
		std::getline(File, MiscItem.Name, '\t');
		std::getline(File, MiscItem.IconIdentifier, '\t');
		std::getline(File, ColorName, '\t');
		File >> MiscItem.Type >> MiscItem.Level;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for loaded textures
		if(!Assets.IsTextureLoaded(MiscItem.IconIdentifier))
			throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find texture: " + MiscItem.IconIdentifier);

		// Set color
		if(ColorName != "") {
			if(!Assets.IsColorLoaded(ColorName))
				throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find color: " + ColorName);

			MiscItem.Color = Assets.Colors[ColorName];
		}
		else
			MiscItem.Color = COLOR_WHITE;

		// Check for duplicates
		if(MiscItemTable.find(Identifier) != MiscItemTable.end())
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Identifier);

		MiscItemTable[Identifier] = MiscItem;
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

		_UpgradeTemplate Upgrade;
		std::string Name;
		std::string ColorName;
		std::getline(File, Name, '\t');
		std::getline(File, Upgrade.Name, '\t');
		std::getline(File, Upgrade.IconIdentifier, '\t');
		std::getline(File, ColorName, '\t');
		File >> Upgrade.UpgradeType >> Upgrade.WeaponType >> Upgrade.Bonus;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for loaded textures
		if(!Assets.IsTextureLoaded(Upgrade.IconIdentifier))
			throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find texture: " + Upgrade.IconIdentifier);

		// Set color
		if(ColorName != "") {
			if(!Assets.IsColorLoaded(ColorName))
				throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find color: " + ColorName);

			Upgrade.Color = Assets.Colors[ColorName];
		}
		else
			Upgrade.Color = COLOR_WHITE;

		// Check for duplicates
		if(UpgradeTable.find(Name) != UpgradeTable.end())
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Name);

		UpgradeTable[Name] = Upgrade;
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
		_WeaponTemplate Weapon;
		AttackSampleTemplateStruct *AttackSample;

		std::string Name;
		std::string ColorName;
		std::string SamplesIdentifier;
		std::string WeaponParticlesIdentifier;
		std::getline(File, Name, '\t');
		std::getline(File, Weapon.Name, '\t');
		std::getline(File, Weapon.IconIdentifier, '\t');
		std::getline(File, SamplesIdentifier, '\t');
		std::getline(File, WeaponParticlesIdentifier, '\t');
		std::getline(File, ColorName, '\t');

		File >> Weapon.Type >> Weapon.ZoomScale >> Weapon.MinAccuracy >> Weapon.MaxAccuracy >> Weapon.Recoil >> Weapon.RecoilRegen >> Weapon.Range
				>> Weapon.FireRate >> Weapon.FirePeriod >> Weapon.ReloadPeriod >> Weapon.MinComponents >> Weapon.MaxComponents
				>> Weapon.MinDamage >> Weapon.MaxDamage	>> Weapon.BulletsShot >> Weapon.RoundSize >> Weapon.AmmoType;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for loaded textures
		if(!Assets.IsTextureLoaded(Weapon.IconIdentifier))
			throw std::runtime_error(std::string(__FUNCTION__) + " - Texture not found: " + Weapon.IconIdentifier);

		// Set color
		if(ColorName != "") {
			if(!Assets.IsColorLoaded(ColorName))
				throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find color: " + ColorName);

			Weapon.Color = Assets.Colors[ColorName];
		}
		else
			Weapon.Color = COLOR_WHITE;

		// Check for attack sample
		if(!Assets.IsAttackSampleLoaded(SamplesIdentifier))
			throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find sample: " + SamplesIdentifier);

		// Set samples
		AttackSample = Assets.GetAttackSampleTemplate(SamplesIdentifier);
		for(int i = 0; i < SAMPLE_TYPES; i++) {
			if(AttackSample)
				Weapon.Samples[i] = AttackSample->Samples[i];
		}

		// Set particles
		if(Assets.IsWeaponParticleTemplateLoaded(WeaponParticlesIdentifier))
			Weapon.WeaponParticles = Assets.GetWeaponParticleTemplate(WeaponParticlesIdentifier);
		else
			Weapon.WeaponParticles = &Assets.BlankWeaponParticle;

		// Check for duplicates
		if(WeaponTable.find(Name) != WeaponTable.end())
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Name);

		WeaponTable[Name] = Weapon;
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

		ItemGroupEntryStruct ItemGroupEntry;
		File >> ItemGroupEntry.Type;
		File.ignore(1, '\t');
		std::getline(File, ItemGroupEntry.ItemIdentifier, '\t');

		// See if items exist
		switch(ItemGroupEntry.Type) {
			case -1:
			break;
			case _Object::MEDKIT:
				if(MiscItemTable.find(ItemGroupEntry.ItemIdentifier) == MiscItemTable.end())
					throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find: " + ItemGroupEntry.ItemIdentifier + " in " + Path);
			break;
			case _Object::AMMO:
				if(AmmoTable.find(ItemGroupEntry.ItemIdentifier) == AmmoTable.end())
					throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find: " + ItemGroupEntry.ItemIdentifier + " in " + Path);
			break;
			case _Object::UPGRADE:
				if(UpgradeTable.find(ItemGroupEntry.ItemIdentifier) == UpgradeTable.end())
					throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find: " + ItemGroupEntry.ItemIdentifier + " in " + Path);
			break;
			case _Object::WEAPON:
				if(WeaponTable.find(ItemGroupEntry.ItemIdentifier) == WeaponTable.end())
					throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find: " + ItemGroupEntry.ItemIdentifier + " in " + Path);
			break;
			case _Object::ARMOR:
				if(ArmorTable.find(ItemGroupEntry.ItemIdentifier) == ArmorTable.end())
					throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find: " + ItemGroupEntry.ItemIdentifier + " in " + Path);
			break;
			case _Object::KEY:
				if(MiscItemTable.find(ItemGroupEntry.ItemIdentifier) == MiscItemTable.end())
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
_Ammo *_Stats::CreateAmmoItem(const std::string &Identifier, int Count, const glm::vec2 &Position) {
	_AmmoTemplate &AmmoTemplate = AmmoTable[Identifier];

	return new _Ammo(Identifier, Count, Position, AmmoTemplate,  Assets.Textures[AmmoTemplate.IconIdentifier]);
}

// Creates armor
_Armor *_Stats::CreateArmor(const std::string &Identifier, int Count, const glm::vec2 &Position) {
	_ArmorTemplate &ArmorTemplate = ArmorTable[Identifier];

	return new _Armor(Identifier, Count, Position, ArmorTemplate, Assets.Textures[ArmorTemplate.IconIdentifier]);
}

// Creates a misc item
_MiscItem *_Stats::CreateMiscItem(const std::string &Identifier, int Count, const glm::vec2 &Position) {
	_MiscItemTemplate &MiscItemTemplate = MiscItemTable[Identifier];

	return new _MiscItem(Identifier, Count, Position, MiscItemTemplate, Assets.Textures[MiscItemTemplate.IconIdentifier]);
}

// Creates a weapon
_Weapon *_Stats::CreateWeapon(const std::string &Identifier, int Count, const glm::vec2 &Position, bool Generate) {
	_WeaponTemplate &WeaponTemplate = WeaponTable[Identifier];
	_Weapon *Weapon = new _Weapon(Identifier, Count, Position, WeaponTemplate, Assets.Textures[WeaponTemplate.IconIdentifier], Generate);

	return Weapon;
}

// Creates an upgrade item
_Upgrade *_Stats::CreateUpgradeItem(const std::string &Identifier, int Count, const glm::vec2 &Position) {
	_UpgradeTemplate &UpgradeTemplate = UpgradeTable[Identifier];

	return new _Upgrade(Identifier, Count, Position, UpgradeTemplate, Assets.Textures[UpgradeTemplate.IconIdentifier]);
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
int _Stats::GetValidSkill(int Level) {
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
	float RandomNumber = Random.GenerateRange(0.0f, ItemGroup->Total);

	// Get item
	for(size_t i = 0; i < ItemGroupSize; i++) {
		if(RandomNumber <= ItemGroup->Entries[i].Count) {
			ObjectSpawn->Type = ItemGroup->Entries[i].Type;
			ObjectSpawn->Identifier = ItemGroup->Entries[i].ItemIdentifier;
			return;
		}
	}
}
