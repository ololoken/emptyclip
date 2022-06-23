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

// A single entry for an item group
struct _ItemGroupEntry {
	_ItemGroupEntry() { }
	_ItemGroupEntry(const std::string &ItemIdentifier, float Count, int Type) :
		ItemIdentifier(ItemIdentifier),
		Count(Count),
		Type(Type) { }

	std::string ItemIdentifier;
	float Count;
	int Type;
};

// Item group information
struct _ItemGroup {
	std::vector<_ItemGroupEntry> Entries;
	int Quantity;
	float Total;
};

// Monster template
struct _MonsterTemplate {
	glm::vec4 Color;
	_WeaponParticleTemplate *WeaponParticles;
	std::string Name, AnimationIdentifier, SamplesIdentifier, ItemGroupIdentifier;
	float Radius, Scale, MovementSpeed, Accuracy, ViewRange, AttackRange;
	int Level, Health, DamageBlock, MinDamage, MaxDamage, BehaviorType, WeaponType;
	int64_t ExperienceGiven;
	double FirePeriod;
	std::string FireSample, MissSample, RicochetSample, EmptySample, ReloadSample, HitSample, DeathSample;
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
	std::string IconIdentifier;
	std::string AmmoType;
	std::string Samples[SOUND_TYPES];

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

		_Item *CreateItem(const std::string &Identifier, int Count, const glm::vec2 &Position);
		_Weapon *CreateWeapon(const std::string &Identifier, int Count, const glm::vec2 &Position, bool Generate);
		_Monster *CreateMonster(const std::string &Identifier, const glm::vec2 &Position);

		int GetLevel(int64_t Experience);
		int64_t GetValidExperience(int64_t Experience);
		int64_t GetExperienceForLevel(int Level);
		int GetLevelHealth(int Level) { return Levels[Level-1].HealthBonus; }
		int GetLevelDamageBlock(int Level) { return Levels[Level-1].DamageBlockBonus; }
		int GetSkillPointsRemaining(int Level) { return Levels[Level-1].SkillPoints; }

		int GetValidSkillLevel(int Level);
		int GetSkill(int Level, int Type) const { return Skills[Level].Data[Type]; }
		float GetSkillBonusMultiplier(int Level, int Type) const { return (100 + Skills[Level].Data[Type]) * 0.01f; }

		_ItemGroup *GetItemGroup(const std::string &Identifier);
		void GetRandomDrop(const _ItemGroup *ItemGroup, _ObjectSpawn *ObjectSpawn);

		std::unordered_map<std::string, std::string> Strings;
		std::unordered_map<std::string, _ItemTemplate> Items;
		std::unordered_map<std::string, _WeaponTemplate> Weapons;
		std::unordered_map<std::string, _ItemGroup> ItemGroups;
		std::unordered_map<std::string, _MonsterTemplate> Monsters;

		std::vector<std::string> AmmoNames;

	private:

		_WeaponParticleTemplate BlankWeaponParticle;

		std::vector<_Level> Levels;
		std::vector<_Skill> Skills;
};

extern _Stats Stats;
