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
	LoadMods("tables/mods.tsv");
	LoadWeapons("tables/weapons.tsv");
	LoadItemDrops("tables/itemdrops.tsv");
	LoadMonsters("tables/monsters.tsv");
	Objects.insert(std::make_pair("player", _ObjectTemplate(_Object::PLAYER)));
	WeaponFists = Stats.CreateItem("weapon_fists", 1, 0, 1, glm::vec2(0), false);
}

// Shutdown
void _Stats::Close() {
	delete WeaponFists;
	Levels.clear();
	Skills.clear();
	Objects.clear();
	ItemDrops.clear();
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

	// Read file
	while(!File.eof() && File.peek() != EOF) {
		_Level Level;
		Level.Level = (int)Levels.size() + 1;
		File >> Level.Experience >> Level.HealthBonus >> Level.SkillPoints;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		Levels.push_back(Level);
	}

	// Calculate next level
	for(std::size_t i = 1; i < Levels.size(); i++)
		Levels[i - 1].NextLevel = Levels[i].Experience - Levels[i - 1].Experience;

	// Cap next level
	Levels[Levels.size() - 1].NextLevel = 0;
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

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		_ObjectTemplate Template(_Object::AMMO);
		std::string ID;
		std::getline(File, ID, '\t');
		std::getline(File, Template.Name, '\t');
		std::getline(File, Template.IconID, '\t');

		File >> Template.Attributes["amount"].Int >> Template.Attributes["amount_max"].Int;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for loaded textures
		if(!ae::Assets.Textures[Template.IconID])
			throw std::runtime_error(std::string(__func__) + " - Texture not found: " + Template.IconID);

		// Check for duplicates
		if(Objects.find(ID) != Objects.end())
			throw std::runtime_error(std::string(__func__) + " - Duplicate entry: " + ID);

		Objects.insert(std::make_pair(ID, Template));
		AmmoNames.push_back(ID);
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

		_ObjectTemplate Template(_Object::WEAPON);
		std::string ID;
		std::string ColorID;
		std::string SoundGroupID;
		std::string WeaponParticlesID;
		std::getline(File, ID, '\t');
		std::getline(File, Template.Name, '\t');
		std::getline(File, Template.IconID, '\t');
		std::getline(File, SoundGroupID, '\t');
		std::getline(File, WeaponParticlesID, '\t');
		std::getline(File, Template.AmmoID, '\t');

		File
			>> Template.Attributes["weapon_type"].Int
			>> Template.Attributes["damage"].Float
			>> Template.Attributes["damage_level"].Float
			>> Template.Attributes["damage_spread"].Float
			>> Template.Attributes["zoom_scale"].Float
			>> Template.Attributes["accuracy"].Float
			>> Template.Attributes["accuracy_spread"].Float
			>> Template.Attributes["recoil"].Float
			>> Template.Attributes["recoil_regen"].Float
			>> Template.Attributes["move_recoil"].Float
			>> Template.Attributes["range"].Float
			>> Template.Attributes["fire_rate"].Int
			>> Template.Attributes["fire_period"].Double
			>> Template.Attributes["reload_amount"].Int
			>> Template.Attributes["reload_period"].Double
			>> Template.Attributes["mods"].Float
			>> Template.Attributes["mods_level"].Float
			>> Template.Attributes["attack_count"].Int
			>> Template.Attributes["rounds"].Int
			>> Template.Attributes["penetration"].Int
			>> Template.Attributes["attack_movespeed"].Float;

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for loaded textures
		if(Template.IconID != "" && !ae::Assets.Textures[Template.IconID])
			throw std::runtime_error(std::string(__func__) + " - Texture not found: " + Template.IconID);

		// Set color
		SetColor(Template.Color, ColorID);

		// Check for attack sound
		if(!GameAssets.IsSoundGroupLoaded(SoundGroupID))
			throw std::runtime_error(std::string(__func__) + " Unknown sound group: " + SoundGroupID);

		// Set sound ids
		_SoundGroup *SoundGroupTemplate = GameAssets.GetSoundGroupTemplate(SoundGroupID);
		if(SoundGroupTemplate) {
			for(int i = 0; i < SOUND_TYPES; i++)
				Template.SoundID[i] = SoundGroupTemplate->SoundID[i];
		}

		// Set particles
		if(GameAssets.IsWeaponParticleTemplateLoaded(WeaponParticlesID))
			Template.WeaponParticles = GameAssets.GetWeaponParticleTemplate(WeaponParticlesID);
		else
			Template.WeaponParticles = &BlankWeaponParticle;

		// Check for duplicates
		if(Objects.find(ID) != Objects.end())
			throw std::runtime_error(std::string(__func__) + " - Duplicate entry: " + ID);

		Objects.insert(std::make_pair(ID, Template));
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

		_ObjectTemplate Template(_Object::ARMOR);
		std::string ID;
		std::getline(File, ID, '\t');
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
			>> Template.Attributes["move_speed_level"].Float
			>> Template.Attributes["mods"].Float
			>> Template.Attributes["mods_level"].Float;

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for loaded textures
		if(!ae::Assets.Textures[Template.IconID])
			throw std::runtime_error(std::string(__func__) + " - Texture not found: " + Template.IconID);

		// Check for duplicates
		if(Objects.find(ID) != Objects.end())
			throw std::runtime_error(std::string(__func__) + " - Duplicate entry: " + ID);

		Objects.insert(std::make_pair(ID, Template));
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

		_ObjectTemplate Template(_Object::KEY);
		std::string ID;
		std::string DoorColorID;
		std::getline(File, ID, '\t');
		std::getline(File, Template.Name, '\t');
		std::getline(File, Template.IconID, '\t');
		std::getline(File, DoorColorID, '\n');

		// Check for loaded textures
		if(!ae::Assets.Textures[Template.IconID])
			throw std::runtime_error(std::string(__func__) + " - Cannot find texture: " + Template.IconID);

		// Set color
		SetColor(Template.DoorColor, DoorColorID);

		// Check for duplicates
		if(Objects.find(ID) != Objects.end())
			throw std::runtime_error(std::string(__func__) + " - Duplicate entry: " + ID);

		Objects.insert(std::make_pair(ID, Template));
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

		_ObjectTemplate Template(_Object::MEDKIT);
		std::string ID;
		std::string ColorID;
		std::getline(File, ID, '\t');
		std::getline(File, Template.Name, '\t');
		std::getline(File, Template.IconID, '\t');

		File
			>> Template.Attributes["health_restored"].Float
			>> Template.Attributes["health_restored_level"].Float;

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for loaded textures
		if(!ae::Assets.Textures[Template.IconID])
			throw std::runtime_error(std::string(__func__) + " - Cannot find texture: " + Template.IconID);

		// Check for duplicates
		if(Objects.find(ID) != Objects.end())
			throw std::runtime_error(std::string(__func__) + " - Duplicate entry: " + ID);

		Objects.insert(std::make_pair(ID, Template));
	}

	File.close();
}

// Load mod stats
void _Stats::LoadMods(const std::string &Path) {
	ModNames.push_back("");

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		_ObjectTemplate Template(_Object::MOD);
		std::string ID;
		std::getline(File, ID, '\t');
		std::getline(File, Template.Name, '\t');
		std::getline(File, Template.IconID, '\t');

		File
			>> Template.Attributes["mod_type"].Int
			>> Template.Attributes["object_type"].Int
			>> Template.Attributes["weapon_type"].Int
			>> Template.Attributes["bonus"].Float
			>> Template.Attributes["bonus_level"].Float
			>> Template.Attributes["percent_sign"].Int;

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for loaded textures
		if(!ae::Assets.Textures[Template.IconID])
			throw std::runtime_error(std::string(__func__) + " - Cannot find texture: " + Template.IconID);

		// Check for duplicates
		if(Objects.find(ID) != Objects.end())
			throw std::runtime_error(std::string(__func__) + " - Duplicate entry: " + ID);

		Objects.insert(std::make_pair(ID, Template));
		ModNames.push_back(ID);
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
	int Drops = 0;
	std::vector<std::string> ItemDropNames;
	std::string DropName;
	while(std::getline(Buffer, DropName, '\t')) {
		if(DropName == "")
			continue;

		ItemDropNames.push_back(DropName);

		auto ItemDropIterator = ItemDrops.find(DropName);
		if(ItemDropIterator == ItemDrops.end()) {
			_ItemDrop ItemDrop;
			ItemDrop.Total = 0;
			ItemDrops[DropName] = ItemDrop;
		}

		Drops++;
	}

	// Read rest of data
	while(!File.eof() && File.peek() != EOF) {

		_ItemDropEntry ItemDropEntry;
		File >> ItemDropEntry.Type;
		File.ignore(1, '\t');
		std::getline(File, ItemDropEntry.ItemID, '\t');

		// See if items exist
		switch(ItemDropEntry.Type) {
			case _Object::NONE:
			break;
			case _Object::KEY:
			case _Object::AMMO:
			case _Object::MOD:
			case _Object::ARMOR:
			case _Object::WEAPON:
			case _Object::MEDKIT:
				if(Objects.find(ItemDropEntry.ItemID) == Objects.end())
					throw std::runtime_error(std::string(__func__) + " - Cannot find: " + ItemDropEntry.ItemID + " in " + Path);
			break;
			default:
				throw std::runtime_error(std::string(__func__) + " - Bad item type: " + ItemDropEntry.ItemID + " in " + Path);
			break;
		}

		// Add counts to item drops
		for(int i = 0; i < Drops; i++) {
			File >> ItemDropEntry.Count;
			if(ItemDropEntry.Count <= 0)
				continue;

			ItemDrops[ItemDropNames[i]].Total += ItemDropEntry.Count;
			ItemDropEntry.Count = ItemDrops[ItemDropNames[i]].Total;
			ItemDrops[ItemDropNames[i]].Entries.push_back(ItemDropEntry);
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

		_ObjectTemplate Template(_Object::MONSTER);
		std::string ID;
		std::string WeaponParticlesID;
		std::string ColorID;
		std::getline(File, ID, '\t');
		std::getline(File, Template.Name, '\t');
		std::getline(File, Template.AnimationID, '\t');
		std::getline(File, ColorID, '\t');
		std::getline(File, WeaponParticlesID, '\t');
		std::getline(File, Template.SoundGroupID, '\t');
		std::getline(File, Template.ItemDropID, '\t');

		File
			>> Template.Attributes["drop_count"].Int
			>> Template.Attributes["health"].Float
			>> Template.Attributes["health_level"].Float
			>> Template.Attributes["ai_type"].Int
			>> Template.Attributes["view_range"].Float
			>> Template.Attributes["xp"].Float
			>> Template.Attributes["xp_level"].Float
			>> Template.Attributes["move_speed"].Float
			>> Template.Attributes["move_speed_level"].Float
			>> Template.Attributes["radius"].Float
			>> Template.Attributes["scale"].Float
			>> Template.Attributes["accuracy"].Int
			>> Template.Attributes["attack_range"].Float
			>> Template.Attributes["damage"].Float
			>> Template.Attributes["damage_level"].Float
			>> Template.Attributes["damage_spread"].Float
			>> Template.Attributes["attack_period"].Double
			>> Template.Attributes["weapon_type"].Int
			>> Template.Attributes["attack_movespeed"].Float;

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for animation
		if(ae::Assets.Animations.find(Template.AnimationID) == ae::Assets.Animations.end())
			throw std::runtime_error(std::string(__func__) + " - Unknown animation_id: '" + Template.AnimationID + "' for " + ID);

		// Set color
		SetColor(Template.Color, ColorID);

		// Set particles
		if(GameAssets.IsWeaponParticleTemplateLoaded(WeaponParticlesID))
			Template.WeaponParticles = GameAssets.GetWeaponParticleTemplate(WeaponParticlesID);
		else
			Template.WeaponParticles = &BlankWeaponParticle;

		// Check for sound group
		if(!GameAssets.IsSoundGroupLoaded(Template.SoundGroupID))
			throw std::runtime_error(std::string(__func__) + " - Unknown sound_group_id: '" + Template.SoundGroupID + "' for " + ID);

		// Check for item group
		if(Template.ItemDropID != "" && ItemDrops.find(Template.ItemDropID) == ItemDrops.end())
			throw std::runtime_error(std::string(__func__) + " - Unknown itemdrop_id: '" + Template.ItemDropID + "' for " + ID);

		// Check for duplicates
		if(Stats.Objects.find(ID) != Stats.Objects.end())
			throw std::runtime_error(std::string(__func__) + " - Duplicate id: '" + ID + "'");

		Objects.insert(std::make_pair(ID, Template));
	}

	File.close();
}

// Create item
_Item *_Stats::CreateItem(const std::string &ID, int Level, int Quality, int Count, const glm::vec2 &Position, bool RandomStats) {
	_ObjectTemplate &Template = Objects.at(ID);

	// Create item
	_Item *Item = new _Item(Template);
	Item->ID = ID;
	Item->Level = Level;
	Item->Quality = Quality;
	Item->Count = Count;
	Item->Position = Position;
	Item->Texture = ae::Assets.Textures[Template.IconID];

	// Generate random quality
	if(RandomStats)
		Item->Quality = ae::GetRandomInt(-ITEM_QUALITY_RANGE, ITEM_QUALITY_RANGE);

	// Set attributes based off type and item level
	switch(Template.Type) {
		case _Object::WEAPON: {
			Item->Attributes["weapon_type"].Int = Template.Attributes["weapon_type"].Int;
			Item->Attributes["zoom_scale"].Float = Template.Attributes["zoom_scale"].Float;
			Item->Attributes["recoil"].Float = Template.Attributes["recoil"].Float;
			Item->Attributes["recoil_regen"].Float = Template.Attributes["recoil_regen"].Float;
			Item->Attributes["range"].Float = Template.Attributes["range"].Float;
			Item->Attributes["fire_rate"].Int = Template.Attributes["fire_rate"].Int;
			Item->Attributes["attack_movespeed"].Float = Template.Attributes["attack_movespeed"].Float;
			Item->SetMaxMods(RandomStats);

			Item->Attributes["ammo"].Int = Template.Attributes.at("rounds").Int;
		} break;
		case _Object::ARMOR:
			Item->SetMaxMods(RandomStats);
		break;
		case _Object::MOD:
			Item->Attributes["mod_type"].Int = Template.Attributes["mod_type"].Int;
			Item->Attributes["weapon_type"].Int = Template.Attributes["weapon_type"].Int;
			Item->SetAttributeLevel("bonus", 1.0f + Item->Quality * 0.01f);
		break;
		case _Object::MEDKIT:
			Item->SetAttributeLevel("health_restored", 1.0f);
		break;
		default:
			Item->Attributes = Template.Attributes;
		break;
	}

	Item->RecalculateStats();

	return Item;
}

// Create monster
_Monster *_Stats::CreateMonster(const std::string &ID, int Level, const glm::vec2 &Position) {
	_ObjectTemplate &Template = Objects.at(ID);

	// Create object
	_Monster *Monster = new _Monster(Template);
	Monster->SetPosition(Position);
	Monster->Animation->Reels = ae::Assets.Animations[Template.AnimationID];
	Monster->Level = Level;
	if(Template.ItemDropID != "")
		Monster->ItemDrop = &ItemDrops[Template.ItemDropID];

	// Set stats
	Monster->Recoil = 0;
	Monster->RecoilRegen = 0;
	Monster->DamageBlock = 0;
	Monster->MovementSpeed = Monster->GetAttributeLevel("move_speed", 1.0f);
	Monster->Radius = Template.Attributes.at("radius").Float;
	Monster->Scale = Template.Attributes.at("scale").Float;
	Monster->Health = std::ceil(Monster->MaxHealth = Monster->GetAttributeLevel("health", 1.0f));
	Monster->ExperienceGiven = std::ceil(Monster->GetAttributeLevel("xp", 1.0f));
	Monster->MinAccuracy = Template.Attributes.at("accuracy").Int;
	for(int i = 0; i < WEAPONATTACK_COUNT; i++) {
		Monster->GetAttributeRange("damage", 1.0f, Monster->MinDamage[i], Monster->MaxDamage[i]);
		Monster->FirePeriod[i] = Template.Attributes.at("attack_period").Double;
		Monster->MaxAccuracy[i] = Template.Attributes.at("accuracy").Int;
		Monster->AttackRange[i] = Template.Attributes.at("attack_range").Float;
		Monster->AttackMoveSpeed[i] = Template.Attributes.at("attack_movespeed").Float;
	}

	return Monster;
}

// Returns the level given the experience number
const _Level &_Stats::FindLevel(int64_t Experience) {

	// Search through levels
	for(std::size_t i = 1; i < Levels.size(); i++) {
		if(Levels[i].Experience > Experience)
			return Levels[i-1];
	}

	return Levels[Levels.size()-1];
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
void _Stats::GetRandomDrop(const _ItemDrop *ItemDrop, _ObjectSpawn *ObjectSpawn) {
	ObjectSpawn->Type = 0;

	// Get item group
	size_t ItemDropSize = ItemDrop->Entries.size();
	if(ItemDropSize == 0)
		return;

	// Get total
	if(ItemDrop->Total <= 0.0f)
		return;

	// Generate roll
	float RandomNumber = ae::GetRandomReal(0.0, ItemDrop->Total);

	// Get item
	for(size_t i = 0; i < ItemDropSize; i++) {
		if(RandomNumber <= ItemDrop->Entries[i].Count) {
			ObjectSpawn->Type = ItemDrop->Entries[i].Type;
			ObjectSpawn->ID = ItemDrop->Entries[i].ItemID;
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

// Determine if template is an item
bool _ObjectTemplate::IsItem() const {
	return Type > _Object::MONSTER;
}
