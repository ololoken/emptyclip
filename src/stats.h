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
#include <objects/templates.h>
#include <unordered_map>
#include <vector>
#include <string>

// Forward Declarations
class _Object;
class _Item;
class _Weapon;
class _Monster;
struct _ObjectSpawn;
namespace ae {
	class _Sound;
	class _Database;
}

// Used for level information
struct _Level {
	int64_t Experience;
	int64_t NextLevel;
	int Level;
	int HealthBonus;
	int SkillPoints;
};

// Holds skill information
struct _Skill {
	int Data[SKILL_COUNT][2];
};

// A single entry for an item drop
struct _ItemDropEntry {
	std::string ItemID;
	int Odds;
	int Type;
};

// Item drop information
struct _ItemDrop {
	std::vector<_ItemDropEntry> Entries;
	int OddsSum;
};

// Object template
struct _ObjectTemplate {

	_ObjectTemplate(int Type) : Type(Type) {}
	bool IsItem() const;

	std::string ID;
	std::string Name;
	std::string IconID;
	std::string MeleeID;
	std::string AmmoID;
	std::string ProjectileID;
	std::string AnimationID;
	std::string SoundGroupID;
	std::string ItemDropID;
	std::string MeshID;
	std::vector<const ae::_Sound *> SoundID[SOUND_COUNT];
	const _ParticleGroup *ParticleGroup{nullptr};
	const _ParticleTemplate *ParticleTemplate{nullptr};
	const ae::_Texture *LightTexture{nullptr};
	glm::vec4 Color{1.0f};
	glm::vec4 LightColor{1.0f};
	glm::vec4 DoorColor{1.0f};
	int Type{0};

	std::unordered_map<std::string, _Value> Attributes;
};

// Classes
class _Stats {

	public:

		void Init();
		void Close();

		void LoadStrings();
		void LoadLevels();
		void LoadSkills();
		void LoadAmmo();
		void LoadWeapons();
		void LoadArmor();
		void LoadKeys();
		void LoadMedkits();
		void LoadMods();
		void LoadItemDrops();
		void LoadMonsters();
		void LoadProps();
		void LoadProjectiles();

		_Item *CreateItem(const std::string &ID, int Level, int Quality, int Count, const glm::vec2 &Position, bool RandomStats);
		_Monster *CreateMonster(const std::string &ID, int Level, const glm::vec2 &Position);
		_Object *CreateProp(const std::string &ID, const glm::vec2 &Position, float Rotation, float Scale) const;
		_Object *CreateProjectile(const _ObjectTemplate &Template, const glm::vec2 &Position) const;

		const _Level &FindLevel(int64_t Experience);
		int GetLevelHealth(int Level) { return Levels[(size_t)Level-1].HealthBonus; }
		int GetSkillPointsRemaining(int Level) { return Levels[(size_t)Level-1].SkillPoints; }
		int GetMaxSkillLevel(int PlayerLevel) const;
		int GetMaxLevel() const { return (int)Levels.size(); }

		int GetValidSkillLevel(int Level);
		int GetSkill(int Level, int Type, int Index=0) const { return Skills[(size_t)Level].Data[Type][Index]; }
		float GetSkillBonusMultiplier(int Level, int Type, int Index=0) const { return (100 + Skills[(size_t)Level].Data[Type][Index]) * 0.01f; }

		void GetRandomDrop(const _ItemDrop *ItemDrop, _ObjectSpawn *ObjectSpawn);

		std::unordered_map<std::string, std::string> Strings;
		std::unordered_map<std::string, _ObjectTemplate> Objects;
		std::unordered_map<std::string, _ItemDrop> ItemDrops;
		std::vector<_Level> Levels;
		_Item *WeaponFists{nullptr};

		std::vector<std::string> AmmoNames;
		std::vector<std::string> ModNames;

		ae::_Database *Database;

	private:

		void SetColor(glm::vec4 &Color, const std::string &ColorID);
		_ParticleGroup BlankWeaponParticle;

		std::vector<_Skill> Skills;
};

extern _Stats Stats;
