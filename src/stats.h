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

// Used for level information
struct _Level {
	int64_t Experience;
	int HealthBonus;
	int DamageBlockBonus;
	int SkillPoints;
};

// Holds skill information
struct _Skill {
	int Data[SKILL_COUNT];
};

// A single entry for an item drop
struct _ItemDropEntry {
	_ItemDropEntry() { }
	_ItemDropEntry(const std::string &ItemID, float Count, int Type) :
		ItemID(ItemID),
		Count(Count),
		Type(Type) { }

	std::string ItemID;
	float Count;
	int Type;
};

// Item drop information
struct _ItemDrop {
	std::vector<_ItemDropEntry> Entries;
	float Total;
};

// Monster template
struct _MonsterTemplate {
	_MonsterTemplate() : Color(1.0f) { }

	std::string Name;
	std::string AnimationID;
	std::string SoundGroupID;
	std::string ItemDropID;
	_WeaponParticleTemplate *WeaponParticles;
	glm::vec4 Color;

	std::unordered_map<std::string, _Value> Attributes;
};

// Item template
struct _ItemTemplate {
	_ItemTemplate() : Color(1.0f), Type(-1) { }
	_ItemTemplate(int Type) : Color(1.0f), Type(Type) { }

	std::string Name;
	std::string IconID;
	glm::vec4 Color;
	int Type;

	std::unordered_map<std::string, _Value> Attributes;
};

// Weapon template
struct _WeaponTemplate {
	_WeaponTemplate() :	WeaponParticles(nullptr) { }

	_WeaponParticleTemplate *WeaponParticles;
	glm::vec4 Color;
	std::string Name;
	std::string IconID;
	std::string AmmoID;
	std::string SoundID[SOUND_TYPES];

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
		void LoadArmor(const std::string &Path);
		void LoadKeys(const std::string &Path);
		void LoadMedkits(const std::string &Path);
		void LoadUpgrades(const std::string &Path);
		void LoadWeapons(const std::string &Path);
		void LoadItemDrops(const std::string &Path);
		void LoadMonsters(const std::string &Path);

		_Item *CreateItem(const std::string &ID, int Level, int Quality, int Count, const glm::vec2 &Position, bool RandomStats);
		_Weapon *CreateWeapon(const std::string &ID, int Level, int Quality, const glm::vec2 &Position, bool RandomStats);
		_Monster *CreateMonster(const std::string &ID, const glm::vec2 &Position);

		int GetLevel(int64_t Experience);
		int64_t GetValidExperience(int64_t Experience);
		int64_t GetExperienceForLevel(int Level);
		int GetLevelHealth(int Level) { return Levels[Level-1].HealthBonus; }
		int GetLevelDamageBlock(int Level) { return Levels[Level-1].DamageBlockBonus; }
		int GetSkillPointsRemaining(int Level) { return Levels[Level-1].SkillPoints; }

		int GetValidSkillLevel(int Level);
		int GetSkill(int Level, int Type) const { return Skills[Level].Data[Type]; }
		float GetSkillBonusMultiplier(int Level, int Type) const { return (100 + Skills[Level].Data[Type]) * 0.01f; }

		void GetRandomDrop(const _ItemDrop *ItemDrop, _ObjectSpawn *ObjectSpawn);

		std::unordered_map<std::string, std::string> Strings;
		std::unordered_map<std::string, _ItemTemplate> Items;
		std::unordered_map<std::string, _WeaponTemplate> Weapons;
		std::unordered_map<std::string, _ItemDrop> ItemDrops;
		std::unordered_map<std::string, _MonsterTemplate> Monsters;
		_Weapon *WeaponFists;

		std::vector<std::string> AmmoNames;

	private:

		void SetColor(glm::vec4 &Color, const std::string &ColorID);
		_WeaponParticleTemplate BlankWeaponParticle;

		std::vector<_Level> Levels;
		std::vector<_Skill> Skills;
};

extern _Stats Stats;
