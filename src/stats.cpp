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
#include <ae/actions.h>
#include <actiontype.h>
#include <gameassets.h>
#include <constants.h>
#include <map.h>
#include <stdexcept>

_Stats Stats;

// Initialize
void _Stats::Init() {
	Database = new ae::_Database("data/stats.db", true);

	BlankWeaponParticle = _ParticleGroup();

	LoadText();
	LoadLevels();
	LoadSkills();
	LoadAmmoTypes();
	LoadAmmo();
	LoadArmor();
	LoadKeys();
	LoadMedkits();
	LoadMods();
	LoadProjectiles();
	LoadWeapons();
	LoadItemDrops();
	LoadMonsters();
	LoadProps();
	LoadProgression();
	LoadSpecials();
	LoadUniques();
	LoadAchievements();

	CreateTransformedText();

	_ObjectTemplate PlayerTemplate(_Object::PLAYER);
	Objects.insert(std::make_pair("player", PlayerTemplate));

	WeaponFists = Stats.CreateItem("weapon_fists", 1, 0, 1, glm::vec2(0), false);
}

// Shutdown
void _Stats::Close() {

	// Free memory
	for(const auto &Unique : Uniques)
		delete Unique;

	delete WeaponFists;
	delete Database;

	Text.clear();
	Levels.clear();
	Skills.clear();
	Objects.clear();
	ItemDrops.clear();
	Progressions.clear();
	Specials.clear();
	Uniques.clear();
	UniquesByQuality.clear();
	Achievements.clear();
	Ammo.clear();
	AmmoNames.clear();
}

// Load strings
void _Stats::LoadText() {

	// Run query
	Database->PrepareQuery("SELECT * FROM text");

	// Get data
	while(Database->FetchRow()) {
		std::string ID = Database->GetString("id");
		std::string Value = Database->GetString("text");
		Text[ID] = Value;
	}

	Database->CloseQuery();
}

// Load level stats
void _Stats::LoadLevels() {

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
void _Stats::LoadSkills() {

	// Run query
	Database->PrepareQuery("SELECT * FROM skills");

	// Get data
	while(Database->FetchRow()) {
		_Skill Skill;
		Skill.Data[SKILL_STRENGTH][0] = Database->GetReal("strength0");
		Skill.Data[SKILL_STRENGTH][1] = Database->GetReal("strength1");
		Skill.Data[SKILL_DEXTERITY][0] = Database->GetReal("dexterity0");
		Skill.Data[SKILL_DEXTERITY][1] = Database->GetReal("dexterity1");
		Skill.Data[SKILL_FORTITUDE][0] = Database->GetReal("fortitude0");
		Skill.Data[SKILL_FORTITUDE][1] = Database->GetReal("fortitude1");
		Skill.Data[SKILL_VITALITY][0] = Database->GetReal("vitality0");
		Skill.Data[SKILL_VITALITY][1] = Database->GetReal("vitality1");
		Skill.Data[SKILL_AGILITY][0] = Database->GetReal("agility0");
		Skill.Data[SKILL_AGILITY][1] = Database->GetReal("agility1");
		Skill.Data[SKILL_CUNNING][0] = Database->GetReal("cunning0");
		Skill.Data[SKILL_CUNNING][1] = Database->GetReal("cunning1");
		Skill.Data[SKILL_ENDURANCE][0] = Database->GetReal("endurance0");
		Skill.Data[SKILL_ENDURANCE][1] = Database->GetReal("endurance1");
		Skill.Data[SKILL_PERCEPTION][0] = Database->GetReal("perception0");
		Skill.Data[SKILL_PERCEPTION][1] = Database->GetReal("perception1");
		Skill.Data[SKILL_LUCK][0] = Database->GetReal("luck0");
		Skill.Data[SKILL_LUCK][1] = Database->GetReal("luck1");
		Skill.Data[SKILL_INTELLIGENCE][0] = Database->GetReal("intelligence0");
		Skill.Data[SKILL_INTELLIGENCE][1] = Database->GetReal("intelligence1");
		Skills.push_back(Skill);
	}

	Database->CloseQuery();
}

// Load ammo pickups
void _Stats::LoadAmmo() {

	// Run query
	Database->PrepareQuery("SELECT * FROM ammo");

	// Get data
	while(Database->FetchRow()) {
		_ObjectTemplate Template(_Object::AMMO);
		Template.ID = Database->GetString("id");
		Template.Name = Database->GetString("name");
		Template.IconID = Database->GetString("icon_id");
		Template.AmmoID = Database->GetString("type_id");
		Template.RenderListType = Database->GetInt<int>("renderlist");
		Template.Attributes["amount"].Int = Database->GetInt<int>("amount");
		Template.Attributes["pickup_bonus"].Int = Database->GetInt<int>("pickup_bonus");

		// Check for loaded textures
		if(!ae::Assets.Textures[Template.IconID])
			throw std::runtime_error(std::string(__func__) + " unknown texture '" + Template.IconID + "'");

		Objects.insert(std::make_pair(Template.ID, Template));
	}

	Database->CloseQuery();
}

// Load types of ammo
void _Stats::LoadAmmoTypes() {

	// Run query
	Database->PrepareQuery("SELECT * FROM ammotypes");

	// Get data
	while(Database->FetchRow()) {
		_Ammo AmmoType;
		AmmoType.ID = Database->GetString("id");
		AmmoType.Name = Database->GetString("name");
		AmmoType.IconID = Database->GetString("icon_id");
		AmmoType.Max = Database->GetInt<int>("max");

		// Check for loaded textures
		if(!ae::Assets.Textures[AmmoType.IconID])
			throw std::runtime_error(std::string(__func__) + " unknown texture '" + AmmoType.IconID + "'");

		Ammo.insert(std::make_pair(AmmoType.ID, AmmoType));
		AmmoNames.push_back(AmmoType.ID);
	}

	Database->CloseQuery();
}

// Load weapon stats
void _Stats::LoadWeapons() {

	// Run query
	Database->PrepareQuery("SELECT * FROM weapons");

	// Get data
	while(Database->FetchRow()) {
		_ObjectTemplate Template(_Object::WEAPON);
		Template.ID = Database->GetString("id");
		Template.Name = Database->GetString("name");
		Template.IconID = Database->GetString("icon_id");
		Template.MeleeID = Database->GetString("melee_id");
		Template.SoundGroupID = Database->GetString("soundgroup_id");
		Template.ProjectileID = Database->GetString("projectile_id");
		Template.AmmoID = Database->GetString("ammo_id");
		Template.PickupID = Database->GetString("pickup_id");
		std::string WeaponParticlesID = Database->GetString("particlegroup_id");

		Template.Attributes["weapon_type"].Int = Database->GetInt<int>("weapon_type");
		Template.Attributes["damage"].Float = Database->GetReal("damage");
		Template.Attributes["damage_level"].Float = Database->GetReal("damage_level");
		Template.Attributes["damage_spread"].Float = Database->GetReal("damage_spread");
		Template.Attributes["zoom_scale"].Float = Database->GetReal("zoom_scale");
		Template.Attributes["accuracy_min"].Float = Database->GetReal("accuracy_min");
		Template.Attributes["accuracy_max"].Float = Database->GetReal("accuracy_max");
		Template.Attributes["recoil"].Float = Database->GetReal("recoil");
		Template.Attributes["accuracy_regen"].Float = Database->GetReal("accuracy_regen");
		Template.Attributes["move_recoil"].Float = Database->GetReal("move_recoil");
		Template.Attributes["range"].Float = Database->GetReal("range");
		Template.Attributes["fire_rate"].Int = Database->GetInt<int>("fire_rate");
		Template.Attributes["fire_period"].Double = Database->GetReal("fire_period");
		Template.Attributes["fire_allrounds"].Int = Database->GetInt<int>("fire_allrounds");
		Template.Attributes["shoot_period"].Double = Database->GetReal("shoot_period");
		Template.Attributes["burst_rounds"].Float = Database->GetReal("burst_rounds");
		Template.Attributes["burst_period"].Double = Database->GetReal("burst_period");
		Template.Attributes["reload_amount"].Float = Database->GetReal("reload_amount");
		Template.Attributes["reload_period"].Double = Database->GetReal("reload_period");
		Template.Attributes["mods"].Float = Database->GetReal("mods");
		Template.Attributes["mods_level"].Float = Database->GetReal("mods_level");
		Template.Attributes["attack_count"].Float = Database->GetReal("attack_count");
		Template.Attributes["rounds"].Float = Database->GetReal("rounds");
		Template.Attributes["penetration"].Float = Database->GetReal("penetration");
		Template.Attributes["penetration_damage"].Float = Database->GetReal("penetration_damage");
		Template.Attributes["crit_chance"].Float = Database->GetReal("crit_chance");
		Template.Attributes["attack_movespeed"].Float = Database->GetReal("attack_movespeed");
		Template.Attributes["melee_width"].Float = Database->GetReal("melee_width");
		Template.Attributes["melee_offset"].Float = Database->GetReal("melee_offset");
		Template.Attributes["melee_switch"].Int = Database->GetInt<int>("melee_switch");
		Template.Attributes["scale_x"].Float = Database->GetReal("scale_x");
		Template.Attributes["scale_y"].Float = Database->GetReal("scale_y");
		Template.Attributes["projectile_speed"].Float = Database->GetReal("projectile_speed");
		Template.Attributes["explosion_size"].Float = Database->GetReal("explosion_size");
		Template.Attributes["flash"].Int = Database->GetInt<int>("flash");
		Template.Attributes["push"].Float = Database->GetReal("push");
		Template.Attributes["force"].Float = Database->GetReal("force");

		// Check for loaded textures
		if(Template.IconID != "" && !ae::Assets.Textures[Template.IconID])
			throw std::runtime_error(std::string(__func__) + " unknown texture '" + Template.IconID + "'");

		// Check for projectile
		if(Template.ProjectileID != "" && Objects.find(Template.ProjectileID) == Objects.end())
			throw std::runtime_error(std::string(__func__) + " unknown projectile '" + Template.ProjectileID + "'");

		// Check for pickup item
		if(Template.PickupID != "" && Objects.find(Template.PickupID) == Objects.end())
			throw std::runtime_error(std::string(__func__) + " unknown pickup_id '" + Template.PickupID + "'");

		// Check for ammo
		if(Template.AmmoID != "" && Ammo.find(Template.AmmoID) == Ammo.end())
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
void _Stats::LoadArmor() {

	// Run query
	Database->PrepareQuery("SELECT * FROM armor");

	// Get data
	while(Database->FetchRow()) {
		_ObjectTemplate Template(_Object::ARMOR);
		Template.ID = Database->GetString("id");
		Template.Name = Database->GetString("name");
		Template.IconID = Database->GetString("icon_id");
		Template.Attributes["mass"].Float = Database->GetReal("mass");
		Template.Attributes["damage_block"].Float = Database->GetReal("damage_block");
		Template.Attributes["damage_block_level"].Float = Database->GetReal("damage_block_level");
		Template.Attributes["damage_resist"].Float = Database->GetReal("damage_resist");
		Template.Attributes["damage_resist_level"].Float = Database->GetReal("damage_resist_level");
		Template.Attributes["max_stamina"].Float = Database->GetReal("max_stamina");
		Template.Attributes["max_stamina_level"].Float = Database->GetReal("max_stamina_level");
		Template.Attributes["max_ammo"].Float = Database->GetReal("max_ammo");
		Template.Attributes["max_ammo_level"].Float = Database->GetReal("max_ammo_level");
		Template.Attributes["move_speed"].Float = Database->GetReal("move_speed");
		Template.Attributes["move_speed_level"].Float = Database->GetReal("move_speed_level");
		Template.Attributes["max_health"].Float = Database->GetReal("max_health");
		Template.Attributes["max_health_level"].Float = Database->GetReal("max_health_level");
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
void _Stats::LoadKeys() {

	// Run query
	Database->PrepareQuery("SELECT * FROM keys");

	// Get data
	while(Database->FetchRow()) {
		_ObjectTemplate Template(_Object::KEY);
		Template.ID = Database->GetString("id");
		Template.Name = Database->GetString("name");
		Template.IconID = Database->GetString("icon_id");
		Template.DoorColorType = Database->GetInt<int>("doorcolor_type");

		// Check for loaded textures
		if(!ae::Assets.Textures[Template.IconID])
			throw std::runtime_error(std::string(__func__) + " unknown texture '" + Template.IconID + "'");

		Objects.insert(std::make_pair(Template.ID, Template));
	}

	Database->CloseQuery();
}

// Load medkit stats
void _Stats::LoadMedkits() {

	// Run query
	Database->PrepareQuery("SELECT * FROM medkits");

	// Get data
	while(Database->FetchRow()) {
		_ObjectTemplate Template(_Object::MEDKIT);
		Template.ID = Database->GetString("id");
		Template.Name = Database->GetString("name");
		Template.IconID = Database->GetString("icon_id");
		Template.RenderListType = Database->GetInt<int>("renderlist");

		// Check for loaded textures
		if(!ae::Assets.Textures[Template.IconID])
			throw std::runtime_error(std::string(__func__) + " unknown texture '" + Template.IconID + "'");

		Objects.insert(std::make_pair(Template.ID, Template));
	}

	Database->CloseQuery();
}

// Load mod stats
void _Stats::LoadMods() {
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
		Template.Attributes["negative"].Int = Database->GetInt<int>("negative");
		Template.Attributes["max"].Int = Database->GetInt<int>("max");

		// Check for loaded textures
		if(!ae::Assets.Textures[Template.IconID])
			throw std::runtime_error(std::string(__func__) + " unknown texture '" + Template.IconID + "'");

		Objects.insert(std::make_pair(Template.ID, Template));
		ModNames.push_back(Template.ID);
	}

	Database->CloseQuery();
}

// Load item drops
void _Stats::LoadItemDrops() {

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
			throw std::runtime_error(std::string(__func__) + " duplicate itemdrop_id '" + ItemDropID);

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
				throw std::runtime_error(std::string(__func__) + " unknown item_id '" + ItemDropEntry.ItemID);

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
void _Stats::LoadMonsters() {

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
		Template.Attributes["poison"].Float = Database->GetReal("poison");
		Template.Attributes["projectile_speed"].Float = Database->GetReal("projectile_speed");
		Template.Attributes["mass"].Float = Database->GetReal("mass");
		Template.Attributes["force"].Float = Database->GetReal("force");

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
void _Stats::LoadProps() {

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
void _Stats::LoadProjectiles() {

	// Run query
	Database->PrepareQuery("SELECT * FROM projectiles");

	// Get data
	while(Database->FetchRow()) {
		_ObjectTemplate Template(_Object::PROJECTILE);
		Template.ID = Database->GetString("id");
		Template.IconID = Database->GetString("icon_id");
		std::string SoundGroupID = Database->GetString("soundgroup_id");
		std::string ParticleID = Database->GetString("particle_id");
		std::string LightID = Database->GetString("light_id");
		std::string LightColorID = Database->GetString("lightcolor_id");
		Template.Attributes["radius"].Float = Database->GetReal("radius");
		Template.Attributes["scale"].Float = Database->GetReal("scale");
		Template.Attributes["light_scale"].Float = Database->GetReal("light_scale");
		Template.Attributes["rotation_speed"].Float = Database->GetReal("rotation_speed");

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

		// Set light texture
		if(!LightID.empty()) {
			if(ae::Assets.Textures.find(LightID) == ae::Assets.Textures.end())
				throw std::runtime_error(std::string(__func__) + " unknown light_id '" + LightID + "'");

			Template.LightTexture = ae::Assets.Textures.at(LightID);
		}

		// Set light color
		SetColor(Template.LightColor, LightColorID);

		Objects.insert(std::make_pair(Template.ID, Template));
	}

	Database->CloseQuery();
}

// Load progression stats
void _Stats::LoadProgression() {
	Progressions.reserve(GAME_MAX_PROGRESSION + 1);

	// Run query
	Database->PrepareQuery("SELECT * FROM progression");

	// Get data
	while(Database->FetchRow()) {
		_Progression Progression;
		Progression.Progression = Database->GetInt<int>("progression");
		Progression.Level = Database->GetInt<int>("level");
		Progression.Spawn = Database->GetInt<int>("spawn");
		Progression.SpecialChance = Database->GetInt<int>("special_chance");
		Progression.Health = Database->GetReal("health");
		Progression.Damage = Database->GetReal("damage");
		Progression.Experience = Database->GetReal("experience");

		Progressions.push_back(Progression);
	}

	Database->CloseQuery();
}

// Load special enemy modifiers
void _Stats::LoadSpecials() {

	// Run query
	Database->PrepareQuery("SELECT * FROM specials");

	// Get data
	while(Database->FetchRow()) {
		_Special Special;
		Special.Name = Database->GetString("name");
		Special.DamageResist = Database->GetReal("damage_resist");
		Special.DamageFactor = Database->GetReal("damage");
		Special.AttackSpeedFactor = Database->GetReal("attack_speed");
		Special.MoveSpeedFactor = Database->GetReal("move_speed");
		Special.FreePathing = Database->GetInt<int>("freepathing");
		Special.ExperienceModifier = Database->GetReal("xp");
		SetColor(Special.Color, Database->GetString("color_id"));

		Specials.push_back(Special);
	}

	Database->CloseQuery();
}

// Load unique modifiers
void _Stats::LoadUniques() {

	// Run query
	Database->PrepareQuery("SELECT * FROM uniques");

	// Get data
	while(Database->FetchRow()) {
		_Unique *Unique = new _Unique;
		Unique->Name = Database->GetString("name");
		Unique->Chance = std::max(1, Database->GetInt<int>("chance"));
		Unique->Quality = Database->GetInt<int>("quality");
		Unique->Progression = Database->GetInt<int>("progression");
		Unique->Mods = Database->GetInt<int>("mods");
		Unique->Texture = ae::Assets.Textures["textures/lights/circle.png"];
		SetColor(Unique->Color, Database->GetString("color_id"));

		Uniques.push_back(Unique);
		UniquesByQuality[Unique->Quality] = Unique;
	}

	Database->CloseQuery();
}

// Load achievement stats
void _Stats::LoadAchievements() {

	// Run query
	Database->PrepareQuery("SELECT * FROM achievements");

	// Get data
	while(Database->FetchRow()) {
		_Achievement Achievement;
		Achievement.ID = Database->GetString("id");
		Achievement.Name = Database->GetString("name");
		Achievement.Text = Database->GetString("text");

		Achievements.push_back(Achievement);
	}

	Database->CloseQuery();
}

// Convert {game_*} values to real button names
void _Stats::CreateTransformedText() {
	TransformedText.clear();
	for(const auto &String : Text) {
		TransformedText[String.first] = String.second;
		for(int i = 0; i < Action::COUNT; i++) {
			std::string Search = "{" + ae::Actions.State[i].Name + "}";
			size_t Position = TransformedText[String.first].find(Search);
			if(Position == std::string::npos)
				continue;

			// Replace text
			TransformedText[String.first].replace(Position, Search.size(), ae::Actions.GetInputNameForAction(i));
		}
	}
}

// Create item
_Item *_Stats::CreateItem(const std::string &ID, int Level, int Quality, int Count, const glm::vec2 &Position, bool RandomStats, int Progression) {
	_ObjectTemplate &Template = Objects.at(ID);

	// Create item
	_Item *Item = new _Item(Template);
	Item->ID = ID;
	Item->Level = Level;
	Item->Quality = Quality;
	Item->Count = Count;
	Item->SetPosition(Position);
	Item->Texture = ae::Assets.Textures[Template.IconID];

	// Generate random quality
	if(RandomStats && Item->CanUnique()) {
		Item->ExtraMods = ae::GetRandomReal(0.0, 1.5);
		Item->Quality = ae::GetRandomInt(-ITEM_QUALITY_RANGE, ITEM_QUALITY_RANGE);
		if(Item->Quality == ITEM_QUALITY_RANGE) {
			for(const auto &Unique : Stats.Uniques) {
				if(Progression >= Unique->Progression && ae::GetRandomInt(1, Unique->Chance) == 1) {
					Item->Quality = Unique->Quality;
					break;
				}
			}
		}
	}

	// Set unique stats
	if(Item->IsUnique() && UniquesByQuality.find(Item->Quality) != UniquesByQuality.end()) {
		_Unique *Unique = UniquesByQuality[Item->Quality];
		Item->LightTexture = Unique->Texture;
		if(Item->IsAutoPickup()) {
			Item->LightColor = COLOR_GOLD;
			Item->Name = Item->Name + " Bundle";
		}
		else {
			Item->LightColor = Unique->Color;
			Item->Name = Unique->Name + " " + Item->Name;
		}
	}

	// Get quality factor
	float QualityFactor = 1.0f + Item->Quality * 0.01f;

	// Set attributes based off type and item level
	switch(Template.Type) {
		case _Object::WEAPON: {
			Item->Attributes["zoom_scale"].Float = Template.Attributes["zoom_scale"].Float;
			Item->Attributes["range"].Float = Template.Attributes["range"].Float;
			Item->Attributes["fire_rate"].Int = Template.Attributes["fire_rate"].Int;
			Item->Attributes["burst_rounds"].Float = Template.Attributes["burst_rounds"].Float;
			Item->Attributes["burst_period"].Double = Template.Attributes["burst_period"].Double;
			Item->Attributes["attack_movespeed"].Float = Template.Attributes["attack_movespeed"].Float;
			Item->Attributes["fire_allrounds"].Int = Template.Attributes["fire_allrounds"].Int;
			Item->Attributes["shoot_period"].Double = Template.Attributes["shoot_period"].Double;
		} break;
		case _Object::MOD:
			Item->SetAttributeLevel("bonus", QualityFactor);
			if(Template.Attributes.at("negative").Int)
				Item->Attributes.at("bonus").Float = -Item->Attributes.at("bonus").Float;
		break;
		default:
			Item->Attributes = Template.Attributes;
		break;
	}

	Item->RecalculateStats();

	if(Template.Type == _Object::WEAPON)
		Item->Attributes["ammo"].Int = std::round(Item->Attributes.at("rounds").Float);

	Item->SetMaxMods();

	return Item;
}

// Create monster
_Monster *_Stats::CreateMonster(const std::string &ID, int Level, int Progression, const glm::vec2 &Position, size_t SpecialType) {
	const _ObjectTemplate &Template = Objects.at(ID);

	// Create object
	_Monster *Monster = new _Monster(Template);
	Monster->SpawnPosition = Position;
	Monster->SetPosition(Position);
	if(!Template.MeshID.empty())
		Monster->Mesh = ae::Assets.Meshes.at(Template.MeshID);
	if(!Template.ProjectileID.empty()) {
		Monster->Projectiles[WEAPONATTACK_MAIN] = &Stats.Objects.at(Template.ProjectileID);
		Monster->ProjectileSpeed[WEAPONATTACK_MAIN] = Template.Attributes.at("projectile_speed").Float;
		Monster->ExplosionSize[WEAPONATTACK_MAIN] = 0.0f;
	}
	Monster->Animation->Reels = ae::Assets.Animations[Template.AnimationID];
	Monster->Animation->Frame = Monster->Animation->Reels[Monster->WalkingAnimation]->DefaultFrame;
	Monster->Animation->CalculateTextureCoords();
	Monster->Level = Level;
	Monster->Mass = Template.Attributes.at("mass").Float;
	if(Template.ItemDropID != "")
		Monster->ItemDrop = &ItemDrops[Template.ItemDropID];

	// Set stats
	Monster->FreePathing = Template.Attributes.at("freepathing").Int;
	Monster->Recoil = 0;
	Monster->AccuracyRegen = 0;
	Monster->DamageBlock = 0;
	Monster->DamageResist = 0;
	Monster->MoveSpeed = Monster->GetAttributeLevel("move_speed", 1.0f, ENTITY_MAX_MOVESPEED_LEVEL);
	Monster->Radius = Template.Attributes.at("radius").Float;
	Monster->Scale = Template.Attributes.at("scale").Float;
	Monster->Health = Monster->MaxHealth = Monster->GetAttributeLevel("health", Stats.Progressions[Progression].Health);
	Monster->ExperienceGiven = Monster->GetAttributeLevel("xp", Stats.Progressions[Progression].Experience);
	Monster->MinAccuracy = Template.Attributes.at("accuracy").Int;
	Monster->PoisonPower = Template.Attributes.at("poison").Float;
	for(int i = 0; i < WEAPONATTACK_COUNT; i++) {
		Monster->GetAttributeRange("damage", Stats.Progressions[Progression].Damage, Monster->MinDamage[i], Monster->MaxDamage[i]);
		Monster->AttackTimer[i] = Monster->AttackPeriod[i] = Template.Attributes.at("attack_period").Double;
		Monster->ShootPeriod[i] = AI_SHOOT_PERIOD;
		Monster->MaxAccuracy[i] = Template.Attributes.at("accuracy").Int;
		Monster->AttackRange[i] = Template.Attributes.at("attack_range").Float;
		Monster->AttackMoveSpeed[i] = Template.Attributes.at("attack_movespeed").Float;
		Monster->Force[i] = Template.Attributes.at("force").Float;
	}
	if(Monster->IsCrate()) {
		Monster->Texture = Monster->Animation->Reels[0]->Texture;
		Monster->PositionZ = 0.0f;
	}

	// Create special monster variation
	if(SpecialType && !Monster->IsCrate()) {
		_Special *Special = &Stats.Specials[SpecialType];
		for(int i = 0; i < WEAPONATTACK_COUNT; i++) {
			Monster->MinDamage[i] = std::round(Monster->MinDamage[i] * Special->DamageFactor);
			Monster->MaxDamage[i] = std::round(Monster->MaxDamage[i] * Special->DamageFactor);
			Monster->AttackPeriod[i] /= Special->AttackSpeedFactor;
		}
		if(Special->FreePathing) {
			Monster->FreePathing = true;
			Monster->AIType = AI_GHOST;
		}
		Monster->Name = Special->Name + " " + Monster->Name;
		Monster->Color = Special->Color;
		Monster->MoveSpeed *= Special->MoveSpeedFactor;
		Monster->DamageResist += Special->DamageResist;
		Monster->ExperienceGiven *= Special->ExperienceModifier;
	}

	Monster->DamageResist = std::min(Monster->DamageResist, ENTITY_MAX_DAMAGE_RESIST);
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
	Projectile->RotationSpeed = Template.Attributes.at("rotation_speed").Float;
	Projectile->LightTexture = Template.LightTexture;
	if(Projectile->LightTexture) {
		Projectile->LightColor = Template.LightColor;
		Projectile->LightScale = glm::vec2(Template.Attributes.at("light_scale").Float);
	}

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
	if(PlayerLevel <= GAME_PLAYERLEVEL_SOFTCAP)
		return std::min(GAME_SKILL_SOFTCAP, (PlayerLevel - 1) * GAME_MAX_SKILL_PERLEVEL);
	else
		return (PlayerLevel - GAME_PLAYERLEVEL_SOFTCAP) * GAME_MAX_SKILL_PERLEVEL + GAME_SKILL_SOFTCAP;
}

// Returns a skill value in a valid range
int _Stats::GetValidSkillLevel(int Level) {
	if(Level < 0)
		return 0;
	else if(Level >= Stats.GetSkillLevels())
		return Stats.GetSkillLevels();

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
