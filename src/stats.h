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
class _Item;
class _Weapon;
class _Monster;
struct _ObjectSpawn;
namespace ae {
	class _Sound;
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
	int Data[SKILL_COUNT];
};

// A single entry for an item drop
struct _ItemDropEntry {

	_ItemDropEntry() { }
	_ItemDropEntry(const std::string &ItemID, float Count, int Type) : ItemID(ItemID), Count(Count), Type(Type) { }

	std::string ItemID;
	float Count;
	int Type;
};

// Item drop information
struct _ItemDrop {
	std::vector<_ItemDropEntry> Entries;
	float Total;
};

// Object template
struct _ObjectTemplate {

	_ObjectTemplate(int Type) : ParticleGroup(nullptr), Color(1.0f), DoorColor(1.0f), Type(Type) { }
	bool IsItem() const;

	std::string Name;
	std::string IconID;
	std::string AmmoID;
	std::string AnimationID;
	std::string SoundGroupID;
	std::string ItemDropID;
	const ae::_Sound *SoundID[SOUND_COUNT];
	const _ParticleGroup *ParticleGroup;
	glm::vec4 Color;
	glm::vec4 DoorColor;
	int Type;

	std::unordered_map<std::string, _Value> Attributes;
};

// Classes
class _Stats {

	public:

		void Init();
		void Close();

		void LoadStrings(const std::string &Path);
		void LoadLevels(const std::string &Path);
		void LoadSkills(const std::string &Path);
		void LoadAmmo(const std::string &Path);
		void LoadWeapons(const std::string &Path);
		void LoadArmor(const std::string &Path);
		void LoadKeys(const std::string &Path);
		void LoadMedkits(const std::string &Path);
		void LoadMods(const std::string &Path);
		void LoadItemDrops(const std::string &Path);
		void LoadMonsters(const std::string &Path);

		_Item *CreateItem(const std::string &ID, int Level, int Quality, int Count, const glm::vec2 &Position, bool RandomStats);
		_Monster *CreateMonster(const std::string &ID, int Level, const glm::vec2 &Position);

		const _Level &FindLevel(int64_t Experience);
		int GetLevelHealth(int Level) { return Levels[Level-1].HealthBonus; }
		int GetSkillPointsRemaining(int Level) { return Levels[Level-1].SkillPoints; }
		int GetMaxLevel() const { return (int)Levels.size(); }

		int GetValidSkillLevel(int Level);
		int GetSkill(int Level, int Type) const { return Skills[Level].Data[Type]; }
		float GetSkillBonusMultiplier(int Level, int Type, float Multiplier=1.0f) const { return (100 + Skills[Level].Data[Type] * Multiplier) * 0.01f; }

		void GetRandomDrop(const _ItemDrop *ItemDrop, _ObjectSpawn *ObjectSpawn);

		std::unordered_map<std::string, std::string> Strings;
		std::unordered_map<std::string, _ObjectTemplate> Objects;
		std::unordered_map<std::string, _ItemDrop> ItemDrops;
		std::vector<_Level> Levels;
		_Item *WeaponFists;

		std::vector<std::string> AmmoNames;
		std::vector<std::string> ModNames;

	private:

		void SetColor(glm::vec4 &Color, const std::string &ColorID);
		_ParticleGroup BlankWeaponParticle;

		std::vector<_Skill> Skills;
};

extern _Stats Stats;
