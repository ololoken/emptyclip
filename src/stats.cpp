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
#include <objects/object.h>
#include <objects/weapon.h>
#include <objects/monster.h>
#include <ae/random.h>
#include <ae/assets.h>
#include <ae/animation.h>
#include <gameassets.h>
#include <constants.h>
#include <map.h>
#include <fstream>
#include <sstream>
#include <stdexcept>

_Stats Stats;

// Initialize
void _Stats::Init() {
	BlankWeaponParticle = _WeaponParticleTemplate();

	LoadStrings("tables/strings.tsv");
	LoadLevels("tables/levels.tsv");
	LoadSkills("tables/skills.tsv");
	LoadAmmo("tables/ammo.tsv");
	LoadArmor("tables/armor.tsv");
	LoadKeys("tables/keys.tsv");
	LoadMedkits("tables/medkits.tsv");
	LoadUpgrades("tables/upgrades.tsv");
	LoadWeapons("tables/weapons.tsv");
	LoadItemDrops("tables/itemdrops.tsv");
	LoadMonsters("tables/monsters.tsv");
	WeaponFists = Stats.CreateWeapon("weapon_fists", glm::vec2(0), false);
}

// Shutdown
void _Stats::Close() {
	delete WeaponFists;
	Levels.clear();
	Skills.clear();
	Items.clear();
	Weapons.clear();
	ItemGroups.clear();
	Monsters.clear();
}

// Load strings
void _Stats::LoadStrings(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Ignore the first line
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		std::string ID;
		std::string Text;
		std::getline(File, ID, '\t');
		std::getline(File, Text, '\n');

		// Check for duplicates
		if(Strings.find(ID) != Strings.end())
			throw std::runtime_error(std::string(__func__) + " - Duplicate entry: " + ID);

		Strings[ID] = Text;
	}

	File.close();
}

// Load level stats
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

// Load skill stats
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

// Load ammo stats
void _Stats::LoadAmmo(const std::string &Path) {

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
		std::getline(File, Name, '\t');
		std::getline(File, Template.Name, '\t');
		std::getline(File, Template.IconID, '\t');

		File >> Template.Attributes["amount"].Int >> Template.Attributes["amount_max"].Int;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for loaded textures
		if(!ae::Assets.Textures[Template.IconID])
			throw std::runtime_error(std::string(__func__) + " - Texture not found: " + Template.IconID);

		// Check for duplicates
		if(Items.find(Name) != Items.end())
			throw std::runtime_error(std::string(__func__) + " - Duplicate entry: " + Name);

		Items[Name] = Template;
		AmmoNames.push_back(Name);
	}

	File.close();
}

// Load armor stats
void _Stats::LoadArmor(const std::string &Path) {

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
		std::getline(File, Name, '\t');
		std::getline(File, Template.Name, '\t');
		std::getline(File, Template.IconID, '\t');

		File
			>> Template.Attributes["damage_block"].Float
			>> Template.Attributes["damage_block_level"].Float
			>> Template.Attributes["damage_resist"].Float
			>> Template.Attributes["damage_resist_level"].Float
			>> Template.Attributes["max_ammo"].Float
			>> Template.Attributes["max_ammo_level"].Float
			>> Template.Attributes["move_speed"].Float
			>> Template.Attributes["move_speed_level"].Float;

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for loaded textures
		if(!ae::Assets.Textures[Template.IconID])
			throw std::runtime_error(std::string(__func__) + " - Texture not found: " + Template.IconID);

		// Check for duplicates
		if(Items.find(Name) != Items.end())
			throw std::runtime_error(std::string(__func__) + " - Duplicate entry: " + Name);

		Items[Name] = Template;
	}

	File.close();
}

// Load key stats
void _Stats::LoadKeys(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		_ItemTemplate Template(_Object::KEY);
		std::string ID;
		std::string ColorID;
		std::getline(File, ID, '\t');
		std::getline(File, Template.Name, '\t');
		std::getline(File, Template.IconID, '\t');
		std::getline(File, ColorID, '\n');

		// Check for loaded textures
		if(!ae::Assets.Textures[Template.IconID])
			throw std::runtime_error(std::string(__func__) + " - Cannot find texture: " + Template.IconID);

		// Set color
		SetColor(Template.Color, ColorID);

		// Check for duplicates
		if(Items.find(ID) != Items.end())
			throw std::runtime_error(std::string(__func__) + " - Duplicate entry: " + ID);

		Items[ID] = Template;
	}

	File.close();
}

// Load medkit stats
void _Stats::LoadMedkits(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		_ItemTemplate Template(_Object::MEDKIT);
		std::string ID;
		std::string ColorID;
		std::getline(File, ID, '\t');
		std::getline(File, Template.Name, '\t');
		std::getline(File, Template.IconID, '\t');
		std::getline(File, ColorID, '\t');

		File >> Template.Attributes["health_restored"].Int;

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for loaded textures
		if(!ae::Assets.Textures[Template.IconID])
			throw std::runtime_error(std::string(__func__) + " - Cannot find texture: " + Template.IconID);

		// Set color
		SetColor(Template.Color, ColorID);

		// Check for duplicates
		if(Items.find(ID) != Items.end())
			throw std::runtime_error(std::string(__func__) + " - Duplicate entry: " + ID);

		Items[ID] = Template;
	}

	File.close();
}

// Load upgrade stats
void _Stats::LoadUpgrades(const std::string &Path) {

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
		std::getline(File, Template.IconID, '\t');
		std::getline(File, ColorName, '\t');

		File
			>> Template.Attributes["upgrade_type"].Int
			>> Template.Attributes["weapon_type"].Int
			>> Template.Attributes["bonus"].Int;

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for loaded textures
		if(!ae::Assets.Textures[Template.IconID])
			throw std::runtime_error(std::string(__func__) + " - Cannot find texture: " + Template.IconID);

		// Set color
		if(ColorName != "") {
			if(ae::Assets.Colors.find(ColorName) == ae::Assets.Colors.end())
				throw std::runtime_error(std::string(__func__) + " - Cannot find color: " + ColorName);

			Template.Color = ae::Assets.Colors[ColorName];
		}
		else
			Template.Color = COLOR_WHITE;

		// Check for duplicates
		if(Items.find(Name) != Items.end())
			throw std::runtime_error(std::string(__func__) + " - Duplicate entry: " + Name);

		Items[Name] = Template;
	}

	File.close();
}

// Load weapon stats
void _Stats::LoadWeapons(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {
		_WeaponTemplate WeaponTemplate;
		_SoundGroup *SoundGroupTemplate;

		std::string ID;
		std::string ColorName;
		std::string SoundGroupID;
		std::string WeaponParticlesID;
		std::getline(File, ID, '\t');
		std::getline(File, WeaponTemplate.Name, '\t');
		std::getline(File, WeaponTemplate.IconID, '\t');
		std::getline(File, SoundGroupID, '\t');
		std::getline(File, WeaponParticlesID, '\t');
		std::getline(File, WeaponTemplate.AmmoID, '\t');

		File
			>> WeaponTemplate.Attributes["weapon_type"].Int
			>> WeaponTemplate.Attributes["damage"].Float
			>> WeaponTemplate.Attributes["damage_level"].Float
			>> WeaponTemplate.Attributes["damage_spread"].Float
			>> WeaponTemplate.Attributes["zoom_scale"].Float
			>> WeaponTemplate.Attributes["accuracy"].Float
			>> WeaponTemplate.Attributes["accuracy_spread"].Float
			>> WeaponTemplate.Attributes["recoil"].Float
			>> WeaponTemplate.Attributes["recoil_regen"].Float
			>> WeaponTemplate.Attributes["range"].Float
			>> WeaponTemplate.Attributes["fire_rate"].Int
			>> WeaponTemplate.Attributes["fire_period"].Double
			>> WeaponTemplate.Attributes["reload_rounds"].Int
			>> WeaponTemplate.Attributes["reload_period"].Double
			>> WeaponTemplate.Attributes["components"].Float
			>> WeaponTemplate.Attributes["components_level"].Float
			>> WeaponTemplate.Attributes["attack_count"].Int
			>> WeaponTemplate.Attributes["rounds"].Int
			>> WeaponTemplate.Attributes["penetration"].Int;

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for loaded textures
		if(WeaponTemplate.IconID != "" && !ae::Assets.Textures[WeaponTemplate.IconID])
			throw std::runtime_error(std::string(__func__) + " - Texture not found: " + WeaponTemplate.IconID);

		// Set color
		if(ColorName != "") {
			if(ae::Assets.Colors.find(ColorName) == ae::Assets.Colors.end())
				throw std::runtime_error(std::string(__func__) + " - Cannot find color: " + ColorName);

			WeaponTemplate.Color = ae::Assets.Colors[ColorName];
		}
		else
			WeaponTemplate.Color = COLOR_WHITE;

		// Check for attack sample
		if(!GameAssets.IsSoundGroupLoaded(SoundGroupID))
			throw std::runtime_error(std::string(__func__) + " - Cannot find sample: " + SoundGroupID);

		// Set sound ids
		SoundGroupTemplate = GameAssets.GetSoundGroupTemplate(SoundGroupID);
		if(SoundGroupTemplate) {
			for(int i = 0; i < SOUND_TYPES; i++)
				WeaponTemplate.SoundID[i] = SoundGroupTemplate->SoundID[i];
		}

		// Set particles
		if(GameAssets.IsWeaponParticleTemplateLoaded(WeaponParticlesID))
			WeaponTemplate.WeaponParticles = GameAssets.GetWeaponParticleTemplate(WeaponParticlesID);
		else
			WeaponTemplate.WeaponParticles = &BlankWeaponParticle;

		// Check for duplicates
		if(Weapons.find(ID) != Weapons.end())
			throw std::runtime_error(std::string(__func__) + " - Duplicate entry: " + ID);

		Weapons[ID] = WeaponTemplate;
	}

	File.close();
}

// Load item drops
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

		auto ItemGroupTableIterator = ItemGroups.find(DropName);
		if(ItemGroupTableIterator == ItemGroups.end()) {
			_ItemGroup ItemGroup;
			ItemGroup.Total = 0;
			ItemGroup.Quantity = 1;
			ItemGroups[DropName] = ItemGroup;
		}

		ItemDrops++;
	}

	// Read rest of data
	while(!File.eof() && File.peek() != EOF) {

		_ItemGroupEntry ItemGroupEntry;
		File >> ItemGroupEntry.Type;
		File.ignore(1, '\t');
		std::getline(File, ItemGroupEntry.ItemID, '\t');

		// See if items exist
		switch(ItemGroupEntry.Type) {
			case -1:
			break;
			case _Object::KEY:
			case _Object::AMMO:
			case _Object::UPGRADE:
			case _Object::ARMOR:
			case _Object::MEDKIT:
				if(Items.find(ItemGroupEntry.ItemID) == Items.end())
					throw std::runtime_error(std::string(__func__) + " - Cannot find: " + ItemGroupEntry.ItemID + " in " + Path);
			break;
			case _Object::WEAPON:
				if(Weapons.find(ItemGroupEntry.ItemID) == Weapons.end())
					throw std::runtime_error(std::string(__func__) + " - Cannot find: " + ItemGroupEntry.ItemID + " in " + Path);
			break;
			default:
				throw std::runtime_error(std::string(__func__) + " - Bad item type: " + ItemGroupEntry.ItemID + " in " + Path);
			break;
		}

		// Add counts to item groups
		for(int i = 0; i < ItemDrops; i++) {
			File >> ItemGroupEntry.Count;
			if(ItemGroupEntry.Count <= 0)
				continue;

			ItemGroups[ItemDropNames[i]].Total += ItemGroupEntry.Count;
			ItemGroupEntry.Count = ItemGroups[ItemDropNames[i]].Total;
			ItemGroups[ItemDropNames[i]].Entries.push_back(ItemGroupEntry);
		}

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}

	File.close();
}

// Load monsters
void _Stats::LoadMonsters(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		_MonsterTemplate MonsterTemplate;
		std::string ID;
		std::string WeaponParticlesID;
		std::string ColorID;
		std::getline(File, ID, '\t');
		std::getline(File, MonsterTemplate.Name, '\t');
		std::getline(File, MonsterTemplate.AnimationID, '\t');
		std::getline(File, ColorID, '\t');
		std::getline(File, WeaponParticlesID, '\t');
		std::getline(File, MonsterTemplate.SoundGroupID, '\t');
		std::getline(File, MonsterTemplate.ItemGroupID, '\t');

		File
			>> MonsterTemplate.Attributes["max_health"].Int
			>> MonsterTemplate.Attributes["max_health_level"].Int
			>> MonsterTemplate.Attributes["damage_block"].Int
			>> MonsterTemplate.Attributes["ai_type"].Int
			>> MonsterTemplate.Attributes["view_range"].Int
			>> MonsterTemplate.Attributes["experience"].Int
			>> MonsterTemplate.Attributes["experience_level"].Int
			>> MonsterTemplate.Attributes["movement_speed"].Float
			>> MonsterTemplate.Attributes["movement_speed_level"].Float
			>> MonsterTemplate.Attributes["radius"].Float
			>> MonsterTemplate.Attributes["scale"].Float
			>> MonsterTemplate.Attributes["accuracy"].Int
			>> MonsterTemplate.Attributes["attack_range"].Float
			>> MonsterTemplate.Attributes["damage"].Int
			>> MonsterTemplate.Attributes["damage_level"].Int
			>> MonsterTemplate.Attributes["damage_spread"].Float
			>> MonsterTemplate.Attributes["attack_period"].Double
			>> MonsterTemplate.Attributes["weapon_type"].Int;

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for item group
		if(MonsterTemplate.ItemGroupID != "" && ItemGroups.find(MonsterTemplate.ItemGroupID) == ItemGroups.end())
			throw std::runtime_error(std::string(__func__) + " - Cannot find item group: '" + MonsterTemplate.ItemGroupID + "' in " + ID);

		// Check for animation
		if(ae::Assets.Animations.find(MonsterTemplate.AnimationID) == ae::Assets.Animations.end())
			throw std::runtime_error(std::string(__func__) + " - Cannot find animation: '" + MonsterTemplate.AnimationID + "' in " + ID);

		// Set color
		SetColor(MonsterTemplate.Color, ColorID);

		// Check for samples
		if(!GameAssets.IsSoundGroupLoaded(MonsterTemplate.SoundGroupID))
			throw std::runtime_error(std::string(__func__) + " - Cannot find sample: '" + MonsterTemplate.SoundGroupID + "' in " + ID);

		// Set particles
		if(GameAssets.IsWeaponParticleTemplateLoaded(WeaponParticlesID))
			MonsterTemplate.WeaponParticles = GameAssets.GetWeaponParticleTemplate(WeaponParticlesID);
		else
			MonsterTemplate.WeaponParticles = &BlankWeaponParticle;

		// Check for duplicates
		if(Stats.Monsters.find(ID) != Stats.Monsters.end())
			throw std::runtime_error(std::string(__func__) + " - Duplicate entry: " + ID);

		Monsters[ID] = MonsterTemplate;
	}

	File.close();
}

// Create item
_Item *_Stats::CreateItem(const std::string &ID, int Count, const glm::vec2 &Position) {
	_ItemTemplate &Template = Items[ID];

	_Item *Item = new _Item(Template.Attributes);
	Item->Type = Template.Type;
	Item->Name = Template.Name;
	Item->ID = ID;
	Item->Count = Count;
	Item->Position = Position;
	Item->Texture = ae::Assets.Textures[Template.IconID];
	Item->Color = Template.Color;

	switch(Template.Type) {
		case _Object::ARMOR: {
			Item->SetAttributeLevel("damage_block", Item->Level, 1.0f);
			Item->SetAttributeLevel("damage_resist", Item->Level, 1.0f);
			Item->SetAttributeLevel("max_ammo", Item->Level, 1.0f);
			Item->SetAttributeLevel("move_speed", Item->Level, 1.0f);
		} break;
		default:
			Item->Attributes = Template.Attributes;
		break;
	}

	return Item;
}

// Creates a weapon
_Weapon *_Stats::CreateWeapon(const std::string &ID, const glm::vec2 &Position, bool Generate) {
	_WeaponTemplate &WeaponTemplate = Weapons[ID];
	_Weapon *Weapon = new _Weapon(ID, 1, Position, WeaponTemplate, ae::Assets.Textures[WeaponTemplate.IconID], Generate);

	return Weapon;
}

// Creates a monster
_Monster *_Stats::CreateMonster(const std::string &ID, const glm::vec2 &Position) {
	_MonsterTemplate &MonsterTemplate = Monsters[ID];
	_SoundGroup *AttackSample = GameAssets.GetSoundGroupTemplate(MonsterTemplate.SoundGroupID);

	// Creates a monster
	_Monster *Monster = new _Monster(MonsterTemplate, Position);
	Monster->Animation->Reels = ae::Assets.Animations[MonsterTemplate.AnimationID];
	for(int i = 0; i < SOUND_TYPES; i++)
		Monster->Samples[i] = AttackSample->SoundID[i];

	return Monster;
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
	float RandomNumber = ae::GetRandomReal(0.0, ItemGroup->Total);

	// Get item
	for(size_t i = 0; i < ItemGroupSize; i++) {
		if(RandomNumber <= ItemGroup->Entries[i].Count) {
			ObjectSpawn->Type = ItemGroup->Entries[i].Type;
			ObjectSpawn->ID = ItemGroup->Entries[i].ItemID;
			return;
		}
	}
}

// Set optional color from id
void _Stats::SetColor(glm::vec4 &Color, const std::string &ColorID) {

	// Default to white
	if(ColorID.empty()) {
		Color = COLOR_WHITE;
		return;
	}

	// Check for color
	if(ae::Assets.Colors.find(ColorID) == ae::Assets.Colors.end())
		throw std::runtime_error("Unknown color '" + ColorID + "'");

	// Set color
	Color = ae::Assets.Colors[ColorID];
}
