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

// Used for level information
struct _Level {
	int64_t Experience;
	int HealthBonus;
	int DamageBlockBonus;
	int SkillPoints;
};

// Holds skill information
struct _Skill {
	float Data[SKILL_COUNT];
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
	_ItemGroup() { }

	std::vector<_ItemGroupEntry> Entries;
	int Quantity;
	float Total;
};

// Item template
struct _ItemTemplate {

	_ItemTemplate() : Type(-1) { }
	_ItemTemplate(int Type) : Type(Type) { }

	std::string Name;
	std::string IconIdentifier;
	glm::vec4 Color;
	int Type;

	std::unordered_map<std::string, _Value> Attributes;
};

// Classes
class _Stats {

	public:

		void Init();
		void Close();

		void LoadLevels(const std::string &Path);
		void LoadSkills(const std::string &Path);
		void LoadAmmoTable(const std::string &Path);
		void LoadArmorTable(const std::string &Path);
		void LoadMiscItemTable(const std::string &Path);
		void LoadUpgradeTable(const std::string &Path);
		void LoadWeaponTable(const std::string &Path);
		void LoadItemDrops(const std::string &Path);

		_Item *CreateAmmoItem(const std::string &Identifier, int Count, const glm::vec2 &Position);
		_Item *CreateArmor(const std::string &Identifier, int Count, const glm::vec2 &Position);
		_Item *CreateMiscItem(const std::string &Identifier, int Count, const glm::vec2 &Position);
		_Item *CreateUpgradeItem(const std::string &Identifier, int Count, const glm::vec2 &Position);
		_Weapon *CreateWeapon(const std::string &Identifier, int Count, const glm::vec2 &Position, bool Generate);

		int GetLevel(int64_t Experience);
		int64_t GetValidExperience(int64_t Experience);
		int64_t GetExperienceForLevel(int Level);
		int GetLevelHealth(int Level) { return Levels[Level-1].HealthBonus; }
		int GetLevelDamageBlock(int Level) { return Levels[Level-1].DamageBlockBonus; }
		int GetSkillPointsRemaining(int Level) { return Levels[Level-1].SkillPoints; }

		int GetValidSkillLevel(int Level);
		float GetSkill(int Level, int Type) const { return Skills[Level].Data[Type]; }
		float GetSkillPercentImprovement(int Level, int Type) const { return (Skills[Level].Data[Type] - 1.0f) * 100.0f; }

		_ItemGroup *GetItemGroup(const std::string &Identifier);
		void GetRandomDrop(const _ItemGroup *ItemGroup, _ObjectSpawn *ObjectSpawn);

		std::unordered_map<std::string, _ItemTemplate> Items;
		std::unordered_map<std::string, _MiscItemTemplate> MiscItems;
		std::unordered_map<std::string, _WeaponTemplate> Weapons;

		std::vector<std::string> AmmoNames;

		std::unordered_map<std::string, _ItemGroup> ItemGroupTable;

	private:

		std::vector<_Level> Levels;
		std::vector<_Skill> Skills;
};

extern _Stats Stats;
