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
#include <ae/database.h>
#include <gameassets.h>
#include <constants.h>
#include <map.h>
#include <stdexcept>

_Stats Stats;

// Initialize
void _Stats::Init() {
	Database = new ae::_Database("data/stats.db", true);

	BlankWeaponParticle = _ParticleGroup();

	LoadStrings("tables/strings.tsv");
	LoadLevels("tables/levels.tsv");
	LoadSkills("tables/skills.tsv");
	LoadAmmo("tables/ammo.tsv");
	LoadArmor("tables/armor.tsv");
	LoadKeys("tables/keys.tsv");
	LoadMedkits("tables/medkits.tsv");
	LoadMods("tables/mods.tsv");
	LoadProjectiles("tables/projectiles.tsv");
	LoadWeapons("tables/weapons.tsv");
	LoadItemDrops("tables/itemdrops.tsv");
	LoadMonsters("tables/monsters.tsv");
	LoadProps("tables/props.tsv");

	_ObjectTemplate PlayerTemplate(_Object::PLAYER);
	Objects.insert(std::make_pair("player", PlayerTemplate));

	WeaponFists = Stats.CreateItem("weapon_fists", 1, 0, 1, glm::vec2(0), false);
}

// Shutdown
void _Stats::Close() {
	delete WeaponFists;
	Levels.clear();
	Skills.clear();
	Objects.clear();
	ItemDrops.clear();
	delete Database;
}

// Load strings
void _Stats::LoadStrings(const std::string &Path) {

	// Run query
	Database->PrepareQuery("SELECT * FROM strings");

	// Get data
	while(Database->FetchRow()) {
		std::string ID = Database->GetString("id");
		std::string Text = Database->GetString("text");
		Strings[ID] = Text;
	}

	Database->CloseQuery();
}

// Load level stats
void _Stats::LoadLevels(const std::string &Path) {

	// Run query
	Database->PrepareQuery("SELECT * FROM levels");

	// Get data
	while(Database->FetchRow()) {
		_Level Level;
		Level.Level = (int)Levels.size() + 1;
		Level.Experience = Database->GetInt64("experience");
		Level.HealthBonus = Database->GetInt<int>("health");
		Level.SkillPoints = Database->GetInt<int>("skill_points");

		Levels.push_back(Level);
	}

	Database->CloseQuery();

	// Calculate next level
	for(size_t i = 1; i < Levels.size(); i++)
		Levels[i - 1].NextLevel = Levels[i].Experience - Levels[i - 1].Experience;

	// Cap next level
	Levels[Levels.size() - 1].NextLevel = 0;
}

// Load skill stats
void _Stats::LoadSkills(const std::string &Path) {

	// Run query
	Database->PrepareQuery("SELECT * FROM skills");

	// Get data
	while(Database->FetchRow()) {
		_Skill Skill;
		Skill.Data[SKILL_STRENGTH][0] = Database->GetInt<int>("strength0");
		Skill.Data[SKILL_STRENGTH][1] = Database->GetInt<int>("strength1");
		Skill.Data[SKILL_DEXTERITY][0] = Database->GetInt<int>("dexterity0");
		Skill.Data[SKILL_DEXTERITY][1] = Database->GetInt<int>("dexterity1");
		Skill.Data[SKILL_FORTITUDE][0] = Database->GetInt<int>("fortitude0");
		Skill.Data[SKILL_FORTITUDE][1] = Database->GetInt<int>("fortitude1");
		Skill.Data[SKILL_VITALITY][0] = Database->GetInt<int>("vitality0");
		Skill.Data[SKILL_VITALITY][1] = Database->GetInt<int>("vitality1");
		Skill.Data[SKILL_AGILITY][0] = Database->GetInt<int>("agility0");
		Skill.Data[SKILL_AGILITY][1] = Database->GetInt<int>("agility1");
		Skill.Data[SKILL_CUNNING][0] = Database->GetInt<int>("cunning0");
		Skill.Data[SKILL_CUNNING][1] = Database->GetInt<int>("cunning1");
		Skill.Data[SKILL_ENDURANCE][0] = Database->GetInt<int>("endurance0");
		Skill.Data[SKILL_ENDURANCE][1] = Database->GetInt<int>("endurance1");
		Skill.Data[SKILL_PERCEPTION][0] = Database->GetInt<int>("perception0");
		Skill.Data[SKILL_PERCEPTION][1] = Database->GetInt<int>("perception1");
		Skill.Data[SKILL_LUCK][0] = Database->GetInt<int>("luck0");
		Skill.Data[SKILL_LUCK][1] = Database->GetInt<int>("luck1");
		Skills.push_back(Skill);
	}

	Database->CloseQuery();
}

// Load ammo stats
void _Stats::LoadAmmo(const std::string &Path) {

	// Run query
	Database->PrepareQuery("SELECT * FROM ammo");

	// Get data
	while(Database->FetchRow()) {
		_ObjectTemplate Template(_Object::AMMO);
		Template.ID = Database->GetString("id");
		Template.Name = Database->GetString("name");
		Template.IconID = Database->GetString("icon_id");
		Template.Attributes["amount"].Int = Database->GetInt<int>("amount");
		Template.Attributes["amount_max"].Int = Database->GetInt<int>("max");

		// Check for loaded textures
		if(!ae::Assets.Textures[Template.IconID])
			throw std::runtime_error(std::string(__func__) + " unknown texture '" + Template.IconID + "'");

		Objects.insert(std::make_pair(Template.ID, Template));
		AmmoNames.push_back(Template.ID);
	}

	Database->CloseQuery();
}

// Load weapon stats
void _Stats::LoadWeapons(const std::string &Path) {

	// Run query
	Database->PrepareQuery("SELECT * FROM weapons");

	// Get data
	while(Database->FetchRow()) {
		_ObjectTemplate Template(_Object::WEAPON);
		std::string WeaponParticlesID;
		Template.ID = Database->GetString("id");
		Template.Name = Database->GetString("name");
		Template.IconID = Database->GetString("icon_id");
		Template.MeleeID = Database->GetString("melee_id");
		Template.SoundGroupID = Database->GetString("soundgroup_id");
		Template.ProjectileID = Database->GetString("projectile_id");
		Template.AmmoID = Database->GetString("ammo_id");
		WeaponParticlesID = Database->GetString("particlegroup_id");

		Template.Attributes["weapon_type"].Int = Database->GetInt<int>("weapon_type");
		Template.Attributes["damage"].Float = Database->GetReal("damage");
		Template.Attributes["damage_level"].Float = Database->GetReal("damage_level");
		Template.Attributes["damage_spread"].Float = Database->GetReal("damage_spread");
		Template.Attributes["zoom_scale"].Float = Database->GetReal("zoom_scale");
		Template.Attributes["accuracy"].Float = Database->GetReal("accuracy");
		Template.Attributes["accuracy_spread"].Float = Database->GetReal("accuracy_spread");
		Template.Attributes["recoil"].Float = Database->GetReal("recoil");
		Template.Attributes["recoil_regen"].Float = Database->GetReal("recoil_regen");
		Template.Attributes["move_recoil"].Float = Database->GetReal("move_recoil");
		Template.Attributes["range"].Float = Database->GetReal("range");
		Template.Attributes["fire_rate"].Int = Database->GetInt<int>("fire_rate");
		Template.Attributes["fire_period"].Double = Database->GetReal("fire_period");
		Template.Attributes["fire_allrounds"].Int = Database->GetInt<int>("fire_allrounds");
		Template.Attributes["burst_rounds"].Int = Database->GetInt<int>("burst_rounds");
		Template.Attributes["burst_period"].Double = Database->GetReal("burst_period");
		Template.Attributes["reload_amount"].Int = Database->GetInt<int>("reload_amount");
		Template.Attributes["reload_period"].Double = Database->GetReal("reload_period");
		Template.Attributes["mods"].Float = Database->GetReal("mods");
		Template.Attributes["mods_level"].Float = Database->GetReal("mods_level");
		Template.Attributes["attack_count"].Int = Database->GetInt<int>("attack_count");
		Template.Attributes["rounds"].Int = Database->GetInt<int>("rounds");
		Template.Attributes["penetration"].Int = Database->GetInt<int>("penetration");
		Template.Attributes["penetration_damage"].Float = Database->GetReal("penetration_damage");
		Template.Attributes["crit_chance"].Int = Database->GetInt<int>("crit_chance");
		Template.Attributes["attack_movespeed"].Float = Database->GetReal("attack_movespeed");
		Template.Attributes["melee_width"].Float = Database->GetReal("melee_width");
		Template.Attributes["scale_x"].Float = Database->GetReal("scale_x");
		Template.Attributes["scale_y"].Float = Database->GetReal("scale_y");
		Template.Attributes["projectile_speed"].Float = Database->GetReal("projectile_speed");
		Template.Attributes["explosion_size"].Float = Database->GetReal("explosion_size");
		Template.Attributes["flash"].Int = Database->GetInt<int>("flash");

		// Check for loaded textures
		if(Template.IconID != "" && !ae::Assets.Textures[Template.IconID])
			throw std::runtime_error(std::string(__func__) + " unknown texture '" + Template.IconID + "'");

		// Check for projectile
		if(Template.ProjectileID != "" && Objects.find(Template.ProjectileID) == Objects.end())
			throw std::runtime_error(std::string(__func__) + " unknown projectile '" + Template.ProjectileID + "'");

		// Check for ammo
		if(Template.AmmoID != "" && Objects.find(Template.AmmoID) == Objects.end())
			throw std::runtime_error(std::string(__func__) + " unknown ammo '" + Template.AmmoID + "'");

		// Check for attack sound
		if(!Template.SoundGroupID.empty()) {
			if(GameAssets.SoundGroups.find(Template.SoundGroupID) == GameAssets.SoundGroups.end())
				throw std::runtime_error(std::string(__func__) + " unknown sound group '" + Template.SoundGroupID + "'");

			// Set sound ids
			_SoundGroup &SoundGroupTemplate = GameAssets.SoundGroups.at(Template.SoundGroupID);
			for(int i = 0; i < SOUND_COUNT; i++)
				Template.SoundID[i] = SoundGroupTemplate.SoundID[i];
		}

		// Set particles
		if(GameAssets.ParticleGroups.find(WeaponParticlesID) != GameAssets.ParticleGroups.end())
			Template.ParticleGroup = &GameAssets.ParticleGroups.at(WeaponParticlesID);
		else
			Template.ParticleGroup = &BlankWeaponParticle;

		Objects.insert(std::make_pair(Template.ID, Template));
	}

	Database->CloseQuery();
}

// Load armor stats
void _Stats::LoadArmor(const std::string &Path) {

	// Run query
	Database->PrepareQuery("SELECT * FROM armor");

	// Get data
	while(Database->FetchRow()) {
		_ObjectTemplate Template(_Object::ARMOR);
		Template.ID = Database->GetString("id");
		Template.Name = Database->GetString("name");
		Template.IconID = Database->GetString("icon_id");
		Template.Attributes["damage_block"].Float = Database->GetReal("damage_block");
		Template.Attributes["damage_block_level"].Float = Database->GetReal("damage_block_level");
		Template.Attributes["damage_resist"].Float = Database->GetReal("damage_resist");
		Template.Attributes["damage_resist_level"].Float = Database->GetReal("damage_resist_level");
		Template.Attributes["max_ammo"].Float = Database->GetReal("max_ammo");
		Template.Attributes["max_ammo_level"].Float = Database->GetReal("max_ammo_level");
		Template.Attributes["move_speed"].Float = Database->GetReal("move_speed");
		Template.Attributes["move_speed_level"].Float = Database->GetReal("move_speed_level");
		Template.Attributes["mods"].Float = Database->GetReal("mods");
		Template.Attributes["mods_level"].Float = Database->GetReal("mods_level");

		// Check for loaded textures
		if(!ae::Assets.Textures[Template.IconID])
			throw std::runtime_error(std::string(__func__) + " unknown texture '" + Template.IconID + "'");

		Objects.insert(std::make_pair(Template.ID, Template));
	}

	Database->CloseQuery();
}

// Load key stats
void _Stats::LoadKeys(const std::string &Path) {

	// Run query
	Database->PrepareQuery("SELECT * FROM keys");

	// Get data
	while(Database->FetchRow()) {
		_ObjectTemplate Template(_Object::KEY);
		Template.ID = Database->GetString("id");
		Template.Name = Database->GetString("name");
		Template.IconID = Database->GetString("icon_id");
		std::string DoorColorID = Database->GetString("doorcolor_id");

		// Check for loaded textures
		if(!ae::Assets.Textures[Template.IconID])
			throw std::runtime_error(std::string(__func__) + " unknown texture '" + Template.IconID + "'");

		// Set color
		SetColor(Template.DoorColor, DoorColorID);

		Objects.insert(std::make_pair(Template.ID, Template));
	}

	Database->CloseQuery();
}

// Load medkit stats
void _Stats::LoadMedkits(const std::string &Path) {

	// Run query
	Database->PrepareQuery("SELECT * FROM medkits");

	// Get data
	while(Database->FetchRow()) {
		_ObjectTemplate Template(_Object::MEDKIT);
		Template.ID = Database->GetString("id");
		Template.Name = Database->GetString("name");
		Template.IconID = Database->GetString("icon_id");

		// Check for loaded textures
		if(!ae::Assets.Textures[Template.IconID])
			throw std::runtime_error(std::string(__func__) + " unknown texture '" + Template.IconID + "'");

		Objects.insert(std::make_pair(Template.ID, Template));
	}

	Database->CloseQuery();
}

// Load mod stats
void _Stats::LoadMods(const std::string &Path) {
	ModNames.push_back("");

	// Run query
	Database->PrepareQuery("SELECT * FROM mods");

	// Get data
	while(Database->FetchRow()) {
		_ObjectTemplate Template(_Object::MOD);
		Template.ID = Database->GetString("id");
		Template.Name = Database->GetString("name");
		Template.IconID = Database->GetString("icon_id");
		Template.Attributes["mod_type"].Int = Database->GetInt<int>("mod_type");
		Template.Attributes["object_type"].Int = Database->GetInt<int>("object_type");
		Template.Attributes["weapon_type"].Int = Database->GetInt<int>("weapon_type");
		Template.Attributes["bonus"].Float = Database->GetReal("bonus");
		Template.Attributes["bonus_level"].Float = Database->GetReal("bonus_level");
		Template.Attributes["percent_sign"].Int = Database->GetInt<int>("percent_sign");

		// Check for loaded textures
		if(!ae::Assets.Textures[Template.IconID])
			throw std::runtime_error(std::string(__func__) + " unknown texture '" + Template.IconID + "'");

		Objects.insert(std::make_pair(Template.ID, Template));
		ModNames.push_back(Template.ID);
	}

	Database->CloseQuery();
}

// Load item drops
void _Stats::LoadItemDrops(const std::string &Path) {

	// Get item drop names first
	Database->PrepareQuery("PRAGMA table_info(itemdrops)");

	// Skip first column
	Database->FetchRow();

	// Get data
	size_t Drops = 0;
	std::vector<std::string> ItemDropIDs;
	while(Database->FetchRow()) {
		std::string ItemDropID = Database->GetString(1);
		ItemDropIDs.push_back(ItemDropID);

		auto ItemDropIterator = ItemDrops.find(ItemDropID);
		if(ItemDropIterator != ItemDrops.end())
			throw std::runtime_error(std::string(__func__) + " duplicate itemdrop_id '" + ItemDropID + "' in " + Path);

		_ItemDrop ItemDrop;
		ItemDrop.OddsSum = 0;
		ItemDrops[ItemDropID] = ItemDrop;

		Drops++;
	}
	Database->CloseQuery();

	// Run query
	Database->PrepareQuery("SELECT * FROM itemdrops");

	// Get data
	while(Database->FetchRow()) {
		_ItemDropEntry ItemDropEntry;
		ItemDropEntry.ItemID = Database->GetString("id");

		// Check for object
		if(ItemDropEntry.ItemID == "none") {
			ItemDropEntry.Type = 0;
		}
		else {
			if(Objects.find(ItemDropEntry.ItemID) == Objects.end())
				throw std::runtime_error(std::string(__func__) + " unknown item_id '" + ItemDropEntry.ItemID + "' in " + Path);

			ItemDropEntry.Type = Objects.at(ItemDropEntry.ItemID).Type;
		}

		// Add counts to item drops
		for(size_t i = 0; i < Drops; i++) {
			ItemDropEntry.Odds = Database->GetInt<int>((int)(i + 1));
			if(ItemDropEntry.Odds <= 0)
				continue;

			ItemDrops[ItemDropIDs[i]].OddsSum += ItemDropEntry.Odds;
			ItemDropEntry.Odds = ItemDrops[ItemDropIDs[i]].OddsSum;
			ItemDrops[ItemDropIDs[i]].Entries.push_back(ItemDropEntry);
		}
	}

	Database->CloseQuery();
}

// Load monsters
void _Stats::LoadMonsters(const std::string &Path) {

	// Run query
	Database->PrepareQuery("SELECT * FROM monsters");

	// Get data
	while(Database->FetchRow()) {
		std::string WeaponParticlesID;
		_ObjectTemplate Template(_Object::MONSTER);
		Template.ID = Database->GetString("id");
		Template.Name = Database->GetString("name");
		Template.AnimationID = Database->GetString("animation_id");
		Template.MeshID = Database->GetString("mesh_id");
		Template.ProjectileID = Database->GetString("projectile_id");
		SetColor(Template.Color, Database->GetString("color_id"));
		WeaponParticlesID = Database->GetString("particlegroup_id");
		Template.SoundGroupID = Database->GetString("soundgroup_id");
		Template.ItemDropID = Database->GetString("itemdrop_id");

		Template.Attributes["drop_count"].Int = Database->GetInt<int>("drop_count");
		Template.Attributes["health"].Float = Database->GetReal("health");
		Template.Attributes["health_level"].Float = Database->GetReal("health_level");
		Template.Attributes["ai_type"].Int = Database->GetInt<int>("ai_type");
		Template.Attributes["ai_attacks"].Int = Database->GetInt<int>("ai_attacks");
		Template.Attributes["view_range"].Float = Database->GetReal("view_range");
		Template.Attributes["xp"].Float = Database->GetReal("xp");
		Template.Attributes["xp_level"].Float = Database->GetReal("xp_level");
		Template.Attributes["freepathing"].Int = Database->GetInt<int>("freepathing");
		Template.Attributes["move_speed"].Float = Database->GetReal("move_speed");
		Template.Attributes["move_speed_level"].Float = Database->GetReal("move_speed_level");
		Template.Attributes["radius"].Float = Database->GetReal("radius");
		Template.Attributes["scale"].Float = Database->GetReal("scale");
		Template.Attributes["accuracy"].Int = Database->GetInt<int>("accuracy");
		Template.Attributes["attack_range"].Float = Database->GetReal("attack_range");
		Template.Attributes["damage"].Float = Database->GetReal("damage");
		Template.Attributes["damage_level"].Float = Database->GetReal("damage_level");
		Template.Attributes["damage_spread"].Float = Database->GetReal("damage_spread");
		Template.Attributes["attack_period"].Double = Database->GetReal("attack_period");
		Template.Attributes["weapon_type"].Int = Database->GetInt<int>("weapon_type");
		Template.Attributes["attack_movespeed"].Float = Database->GetReal("attack_movespeed");
		Template.Attributes["projectile_speed"].Float = Database->GetReal("projectile_speed");

		// Check for animation
		if(ae::Assets.Animations.find(Template.AnimationID) == ae::Assets.Animations.end())
			throw std::runtime_error(std::string(__func__) + " unknown animation_id '" + Template.AnimationID + "' for '" + Template.ID + "'");

		// Set particles
		if(GameAssets.ParticleGroups.find(WeaponParticlesID) != GameAssets.ParticleGroups.end())
			Template.ParticleGroup = &GameAssets.ParticleGroups.at(WeaponParticlesID);
		else
			Template.ParticleGroup = &BlankWeaponParticle;

		// Check for sound group
		if(GameAssets.SoundGroups.find(Template.SoundGroupID) == GameAssets.SoundGroups.end())
			throw std::runtime_error(std::string(__func__) + " unknown soundgroup_id '" + Template.SoundGroupID + "' for '" + Template.ID + "'");

		// Check for item group
		if(Template.ItemDropID != "" && ItemDrops.find(Template.ItemDropID) == ItemDrops.end())
			throw std::runtime_error(std::string(__func__) + " unknown itemdrop_id '" + Template.ItemDropID + "' for '" + Template.ID + "'");

		// Check for mesh
		if(Template.MeshID != "" && ae::Assets.Meshes.find(Template.MeshID) == ae::Assets.Meshes.end())
			throw std::runtime_error(std::string(__func__) + " unknown mesh_id '" + Template.MeshID + "' for '" + Template.ID + "'");

		Objects.insert(std::make_pair(Template.ID, Template));
	}

	Database->CloseQuery();
}

// Load 3d props
void _Stats::LoadProps(const std::string &Path) {

	// Run query
	Database->PrepareQuery("SELECT * FROM props");

	// Get data
	while(Database->FetchRow()) {
		_ObjectTemplate Template(_Object::PROP);
		Template.ID = Database->GetString("id");
		Template.Name = Database->GetString("name");
		Template.IconID = Database->GetString("texture_id");
		Template.MeshID = Database->GetString("mesh_id");
		Template.Attributes["halfsize_x"].Float = Database->GetReal("halfsize_x");
		Template.Attributes["halfsize_y"].Float = Database->GetReal("halfsize_y");
		Template.Attributes["scale"].Float = Database->GetReal("scale");

		// Check for loaded textures
		if(!ae::Assets.Textures[Template.IconID])
			throw std::runtime_error(std::string(__func__) + " unknown texture '" + Template.IconID + "'");

		// Check for loaded mesh
		if(!ae::Assets.Meshes[Template.MeshID])
			throw std::runtime_error(std::string(__func__) + " unknown mesh '" + Template.MeshID + "'");

		Objects.insert(std::make_pair(Template.ID, Template));
	}

	Database->CloseQuery();
}

// Load projectiles
void _Stats::LoadProjectiles(const std::string &Path) {

	// Run query
	Database->PrepareQuery("SELECT * FROM projectiles");

	// Get data
	while(Database->FetchRow()) {
		_ObjectTemplate Template(_Object::PROJECTILE);
		std::string SoundGroupID;
		std::string ParticleID;
		Template.ID = Database->GetString("id");
		Template.IconID = Database->GetString("icon_id");
		SoundGroupID = Database->GetString("soundgroup_id");
		ParticleID = Database->GetString("particle_id");
		Template.Attributes["radius"].Float = Database->GetReal("radius");
		Template.Attributes["scale"].Float = Database->GetReal("scale");

		// Check for loaded textures
		if(!ae::Assets.Textures[Template.IconID])
			throw std::runtime_error(std::string(__func__) + " unknown texture '" + Template.IconID + "'");

		// Set sound ids
		if(!SoundGroupID.empty()) {
			if(GameAssets.SoundGroups.find(SoundGroupID) == GameAssets.SoundGroups.end())
				throw std::runtime_error(std::string(__func__) + " unknown sound group '" + SoundGroupID + "'");

			_SoundGroup &SoundGroupTemplate = GameAssets.SoundGroups.at(SoundGroupID);
			for(int i = 0; i < SOUND_COUNT; i++)
				Template.SoundID[i] = SoundGroupTemplate.SoundID[i];
		}

		// Set particle
		if(!ParticleID.empty()) {
			if(GameAssets.Particles.find(ParticleID) == GameAssets.Particles.end())
				throw std::runtime_error(std::string(__func__) + " unknown particle_id '" + ParticleID + "'");

			Template.ParticleTemplate = &GameAssets.Particles.at(ParticleID);
		}

		Objects.insert(std::make_pair(Template.ID, Template));
	}

	Database->CloseQuery();
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
			Item->Attributes["range"].Float = Template.Attributes["range"].Float;
			Item->Attributes["fire_rate"].Int = Template.Attributes["fire_rate"].Int;
			Item->Attributes["burst_rounds"].Int = Template.Attributes["burst_rounds"].Int;
			Item->Attributes["burst_period"].Double = Template.Attributes["burst_period"].Double;
			Item->Attributes["attack_movespeed"].Float = Template.Attributes["attack_movespeed"].Float;
			Item->Attributes["attack_width"].Float = Template.Attributes["melee_width"].Float;
			Item->Attributes["scale_x"].Float = Template.Attributes["scale_x"].Float;
			Item->Attributes["scale_y"].Float = Template.Attributes["scale_y"].Float;
			Item->Attributes["fire_allrounds"].Int = Template.Attributes["fire_allrounds"].Int;
			Item->SetMaxMods(RandomStats);
		} break;
		case _Object::ARMOR:
			Item->SetMaxMods(RandomStats);
		break;
		case _Object::MOD:
			Item->Attributes["mod_type"].Int = Template.Attributes["mod_type"].Int;
			Item->Attributes["weapon_type"].Int = Template.Attributes["weapon_type"].Int;
			Item->SetAttributeLevel("bonus", 1.0f + Item->Quality * 0.01f);
		break;
		default:
			Item->Attributes = Template.Attributes;
		break;
	}

	Item->RecalculateStats();

	if(Template.Type == _Object::WEAPON)
		Item->Attributes["ammo"].Int = Item->Attributes.at("rounds").Int;

	return Item;
}

// Create monster
_Monster *_Stats::CreateMonster(const std::string &ID, int Level, const glm::vec2 &Position) {
	const _ObjectTemplate &Template = Objects.at(ID);

	// Create object
	_Monster *Monster = new _Monster(Template);
	Monster->SetPosition(Position);
	if(!Template.MeshID.empty())
		Monster->Mesh = ae::Assets.Meshes.at(Template.MeshID);
	if(!Template.ProjectileID.empty()) {
		Monster->Projectiles[WEAPONATTACK_MAIN] = &Stats.Objects.at(Template.ProjectileID);
		Monster->ProjectileSpeed[WEAPONATTACK_MAIN] = Template.Attributes.at("projectile_speed").Float;
		Monster->ExplosionSize[WEAPONATTACK_MAIN] = 0.0f;
	}
	Monster->Animation->Reels = ae::Assets.Animations[Template.AnimationID];
	Monster->Animation->CalculateTextureCoords();
	Monster->Level = Level;
	if(Template.ItemDropID != "")
		Monster->ItemDrop = &ItemDrops[Template.ItemDropID];

	// Set stats
	Monster->FreePathing = Template.Attributes.at("freepathing").Int;
	Monster->Recoil = 0;
	Monster->RecoilRegen = 0;
	Monster->DamageBlock = 0;
	Monster->MoveSpeed = Monster->GetAttributeLevel("move_speed", 1.0f, ENTITY_MAX_MOVESPEED_LEVEL);
	Monster->Radius = Template.Attributes.at("radius").Float;
	Monster->Scale = Template.Attributes.at("scale").Float;
	Monster->Health = Monster->MaxHealth = Monster->GetAttributeLevel("health", 1.0f);
	Monster->ExperienceGiven = Monster->GetAttributeLevel("xp", 1.0f);
	Monster->MinAccuracy = Template.Attributes.at("accuracy").Int;
	for(int i = 0; i < WEAPONATTACK_COUNT; i++) {
		Monster->GetAttributeRange("damage", 1.0f, Monster->MinDamage[i], Monster->MaxDamage[i]);
		Monster->AttackTimer[i] = Monster->ShootPeriod[i] = Monster->AttackPeriod[i] = Template.Attributes.at("attack_period").Double;
		Monster->MaxAccuracy[i] = Template.Attributes.at("accuracy").Int;
		Monster->AttackRange[i] = Template.Attributes.at("attack_range").Float;
		Monster->AttackMoveSpeed[i] = Template.Attributes.at("attack_movespeed").Float;
	}
	if(Monster->IsCrate()) {
		Monster->Texture = Monster->Animation->Reels[0]->Texture;
		Monster->PositionZ = 0.0f;
	}

	Monster->RecalculateStats();

	return Monster;
}

// Create prop
_Object *_Stats::CreateProp(const std::string &ID, const glm::vec2 &Position, float Rotation, float Scale) const {
	const _ObjectTemplate &Template = Objects.at(ID);

	// Create object
	_Object *Prop = new _Object(Template);
	Prop->SetPosition(Position);
	Prop->Mesh = ae::Assets.Meshes.at(Template.MeshID);
	Prop->Texture = ae::Assets.Textures.at(Template.IconID);
	Prop->Radius = Template.Attributes.at("halfsize_x").Float * Scale;
	if(Template.Attributes.at("halfsize_y").Float != 0.0f)
		Prop->Circle = false;
	Prop->Rotation = Rotation;
	Prop->Scale = Template.Attributes.at("scale").Float * Scale;

	return Prop;
}

// Create projectile
_Object *_Stats::CreateProjectile(const _ObjectTemplate &Template, const glm::vec2 &Position) const {

	// Create object
	_Object *Projectile = new _Object(Template);
	Projectile->SetPosition(Position);
	Projectile->Texture = ae::Assets.Textures.at(Template.IconID);
	Projectile->Radius = Template.Attributes.at("radius").Float;
	Projectile->Scale = Template.Attributes.at("scale").Float;
	Projectile->PositionZ = OBJECT_Z;

	return Projectile;
}

// Returns the level given the experience number
const _Level &_Stats::FindLevel(int64_t Experience) {

	// Search through levels
	for(size_t i = 1; i < Levels.size(); i++) {
		if(Levels[i].Experience > Experience)
			return Levels[i-1];
	}

	return Levels[Levels.size()-1];
}

// Get max level for any skill given a player level
int _Stats::GetMaxSkillLevel(int PlayerLevel) const {
	return (PlayerLevel - 1) * GAME_MAX_SKILL_PERLEVEL;
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
	if(ItemDrop->OddsSum <= 0)
		return;

	// Generate roll
	int RandomNumber = ae::GetRandomInt(1, ItemDrop->OddsSum);

	// Get item
	for(size_t i = 0; i < ItemDropSize; i++) {
		if(RandomNumber <= ItemDrop->Entries[i].Odds) {
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
	return Type >= _Object::WEAPON && Type <= _Object::MEDKIT;
}
