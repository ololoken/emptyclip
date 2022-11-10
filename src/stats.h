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
	float Data[SKILL_COUNT][2];
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

struct _Progression {
	int Progression;
	int Level;
	int Spawn;
	int SpecialChance;
	float Health;
	float Damage;
	float Experience;
};

struct _Special {
	std::string Name;
	glm::vec4 Color{1.0f};
	float DamageFactor{1.0f};
	float AttackSpeedFactor{1.0f};
	float MoveSpeedFactor{1.0f};
	float ExperienceModifier{1.0f};
	float DamageResist{0.0f};
	int FreePathing{0};
};

struct _Unique {
	std::string Name;
	const ae::_Texture *Texture;
	glm::vec4 Color;
	int Chance;
	int Quality;
	int Progression;
	int Mods;
	int HammerValue;
	int WhetstoneValue;
};

struct _Achievement {
	std::string ID;
	std::string Name;
	std::string Text;
};

struct _Ammo {
	std::string ID;
	std::string Name;
	std::string IconID;
	int Max;
	int Type;
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
	std::string PickupID;
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
	int DoorColorType{0};
	int Type{0};
	int RenderListType{-1};
	int AmmoTypeID{-1};

	std::unordered_map<std::string, _Value> Attributes;
};

// Classes
class _Stats {

	public:

		void Init();
		void Close();

		void LoadText();
		void LoadLevels();
		void LoadSkills();
		void LoadAmmo();
		void LoadAmmoTypes();
		void LoadWeapons();
		void LoadArmor();
		void LoadKeys();
		void LoadConsumables();
		void LoadUsables();
		void LoadMods();
		void LoadItemDrops();
		void LoadMonsters();
		void LoadProps();
		void LoadProjectiles();
		void LoadProgression();
		void LoadSpecials();
		void LoadUniques();
		void LoadAchievements();

		void CreateTransformedText();

		_Item *CreateItem(const std::string &ID, int Level, int Quality, int Count, const glm::vec2 &Position, bool RandomStats, int Progression=0);
		_Monster *CreateMonster(const std::string &ID, int Level, int Progression, const glm::vec2 &Position, size_t SpecialType=0);
		_Object *CreateProp(const std::string &ID, const glm::vec2 &Position, float Rotation, float Scale) const;
		_Object *CreateProjectile(const _ObjectTemplate &Template, const glm::vec2 &Position) const;

		const _Level &FindLevel(int64_t Experience);
		int GetLevelHealth(int Level) { return Levels[(size_t)Level-1].HealthBonus; }
		int GetSkillPointsRemaining(int Level) { return Levels[(size_t)Level-1].SkillPoints; }
		int GetSkillLevels() const { return (int)Skills.size() - 1; }
		int GetMaxSkillLevel(int PlayerLevel) const;
		int GetMaxLevel() const { return (int)Levels.size(); }

		int GetValidSkillLevel(int Level);
		float GetSkill(int Level, int Type, int Index=0) const { return Skills[(size_t)Level].Data[Type][Index]; }
		float GetSkillBonusMultiplier(int Level, int Type, int Index=0) const { return (100.0f + Skills[(size_t)Level].Data[Type][Index]) * 0.01f; }

		void GetRandomDrop(const _ItemDrop *ItemDrop, _ObjectSpawn *ObjectSpawn);

		const _Unique *GetUnique(int Quality) const;

		ae::_Database *Database;

		std::unordered_map<std::string, std::string> TransformedText;
		std::unordered_map<std::string, _ObjectTemplate> Objects;
		std::unordered_map<std::string, _ItemDrop> ItemDrops;
		std::unordered_map<std::string, _Ammo> Ammo;
		std::vector<_Progression> Progressions;
		std::vector<_Special> Specials;
		std::vector<_Unique *> Uniques;
		std::vector<_Level> Levels;
		std::vector<_Achievement> Achievements;

		std::vector<std::string> AmmoNames;
		std::vector<std::string> ModNames;
		std::vector<_Skill> Skills;

		_Item *WeaponFists{nullptr};

	private:

		void SetColor(glm::vec4 &Color, const std::string &ColorID);
		_ParticleGroup BlankWeaponParticle;

		std::unordered_map<std::string, std::string> Text;
};

extern _Stats Stats;
