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
#include <glm/vec2.hpp>
#include <objects/templates.h>
#include <string>
#include <unordered_map>
#include <vector>

// Forward Declarations
class _Style;
class _Font;
class _Element;
class _Label;
class _Image;
class _Button;
class _TextBox;
class _Texture;
class _Animation;
class _Particle;
class _Entity;
class _Player;
class _Monster;
class _Weapon;
class _Armor;
class _MiscItem;
class _Upgrade;
class _Ammo;
class _Program;
class _Shader;
struct _Reel;
struct _ReelTemplate;
struct _ParticleTemplate;
struct _MonsterTemplate;
struct _MiscItemTemplate;
struct _UpgradeTemplate;
struct _ArmorTemplate;

// Stores information about a collection of reel identifiers
struct AnimationTemplateStruct {
	AnimationTemplateStruct() { }

	std::vector<std::string> Identifiers;
};

// Stores information about a collection of sound samples used for attacking
struct AttackSampleTemplateStruct {
	AttackSampleTemplateStruct() { }

	std::string Samples[SAMPLE_TYPES];
};

// Used for the map editor
struct _Brush {
	_Brush() { }
	_Brush(const std::string &Identifier, const std::string &Text, const _Texture *Texture, const glm::vec4 &Color, int ObjectType=-1) :
		Identifier(Identifier),
		Text(Text),
		Texture(Texture),
		Color(Color),
		ObjectType(ObjectType) { }

	std::string Identifier;
	std::string Text;
	const _Texture *Texture;
	glm::vec4 Color;
	int ObjectType;
};

// Used for level information
struct LevelStruct {
	int64_t Experience;
	int HealthBonus;
	int DamageBlockBonus;
	int SkillPoints;
};

// Holds skill information
struct SkillStruct {
	float Data[SKILL_COUNT];
};

// A single entry for an item group
struct ItemGroupEntryStruct {
	ItemGroupEntryStruct() { }
	ItemGroupEntryStruct(const std::string &ItemIdentifier, float Count, int Type)
		:	ItemIdentifier(ItemIdentifier),
		    Count(Count),
		    Type(Type) { }

	std::string ItemIdentifier;
	float Count;
	int Type;
};

// Item group information
struct _ItemGroup {
	_ItemGroup() { }

	std::vector<ItemGroupEntryStruct> Entries;
	int Quantity;
	float Total;
};

// Classes
class _Assets {

	public:

		void Init();
		void Close();

		void LoadStrings(const std::string &Path);
		void LoadColors(const std::string &Path);
		void LoadReelTable(const std::string &Path);
		void LoadAnimationTable(const std::string &Path);
		void LoadSoundGroups(const std::string &Path);
		void LoadMonsterTable(const std::string &Path);
		void LoadParticles(const std::string &Path);
		void LoadPrograms(const std::string &Path);
		void LoadTextures(const std::string &Path);
		void LoadSounds(const std::string &Path, const std::string &SamplePath);
		void LoadMiscItemTable(const std::string &Path);
		void LoadUpgradeTable(const std::string &Path);
		void LoadAmmoTable(const std::string &Path);
		void LoadWeaponTable(const std::string &Path);
		void LoadArmorTable(const std::string &Path);
		void LoadItemDrops(const std::string &Path);

		void LoadFonts(const std::string &Path, bool LoadFonts=true);
		void LoadMonsterSet(const std::string &Path);
		void LoadReel(const std::string &Identifier, const std::string &Path);
		void LoadAnimation(const std::string &Identifier, const std::string &Path);
		void LoadWeaponParticles(const std::string &Path);
		void LoadMonsterAnimation();
		void LoadStyles(const std::string &Path);
		void LoadElements(const std::string &Path);
		void LoadLabels(const std::string &Path);
		void LoadImages(const std::string &Path);
		void LoadButtons(const std::string &Path);
		void LoadTextBoxes(const std::string &Path);

		bool IsColorLoaded(const std::string &Identifier);
		bool IsTextureLoaded(const std::string &Identifier);
		bool IsAttackSampleLoaded(const std::string &Identifier);
		bool IsParticleLoaded(const std::string &Identifier);
		bool IsWeaponParticleTemplateLoaded(const std::string &Identifier);
		bool IsReelLoaded(const std::string &Identifier);
		bool IsAnimationLoaded(const std::string &Identifier);
		bool IsMonsterLoaded(const std::string &Identifier);
		bool IsMiscItemLoaded(const std::string &Identifier);
		bool IsUpgradeLoaded(const std::string &Identifier);
		bool IsAmmoLoaded(const std::string &Identifier);
		bool IsWeaponLoaded(const std::string &Identifier);
		bool IsArmorLoaded(const std::string &Identifier);
		bool IsItemGroupLoaded(const std::string &Identifier);

		void UnloadReel(const std::string &Identifier);
		void UnloadAnimation(const std::string &Identifier);

		int GetLevel(int64_t Experience);
		int64_t GetValidExperience(int64_t Experience);
		int64_t GetExperienceForLevel(int Level);
		int GetLevelHealth(int Level) { return Levels[Level-1].HealthBonus; }
		int GetLevelDamageBlock(int Level) { return Levels[Level-1].DamageBlockBonus; }
		int GetSkillPointsRemaining(int Level) { return Levels[Level-1].SkillPoints; }
		int GetValidSkill(int Level);
		float GetSkill(int Level, int Type) const { return Skills[Level].Data[Type]; }
		float GetSkillPercentImprovement(int Level, int Type) const { return (Skills[Level].Data[Type] - 1.0f) * 100.0f; }

		_Label *GetLabel(const std::string &Identifier);
		_Image *GetImage(const std::string &Identifier);
		_Button *GetButton(const std::string &Identifier);
		_TextBox *GetTextBox(const std::string &Identifier);
		_Reel *GetReel(const std::string &Identifier);
		AttackSampleTemplateStruct *GetAttackSampleTemplate(const std::string &Identifier);
		_Animation *GetAnimation(const std::string &Identifier);
		_ParticleTemplate *GetParticleTemplate(const std::string &Identifier);
		_WeaponParticleTemplate *GetWeaponParticleTemplate(const std::string &Identifer);
		_MonsterTemplate *GetMonsterTemplate(const std::string &Identifier);
		_MiscItemTemplate *GetMiscItemTemplate(const std::string &Identifier);
		_UpgradeTemplate *GetUpgradeTemplate(const std::string &Identifier);
		_AmmoTemplate *GetAmmoTemplate(const std::string &Identifier);
		_WeaponTemplate *GetWeaponTemplate(const std::string &Identifier);
		_ArmorTemplate *GetArmorTemplate(const std::string &Identifier);
		_ItemGroup *GetItemGroup(const std::string &Identifier);
		_Monster *CreateMonster(const std::string &Identifier, const glm::vec2 &Position);
		_MiscItem *CreateMiscItem(const std::string &Identifier, int Count, const glm::vec2 &Position);
		_Ammo *CreateAmmoItem(const std::string &Identifier, int Count, const glm::vec2 &Position);
		_Ammo *CreateAmmoItem(int Type);
		_Upgrade *CreateUpgradeItem(const std::string &Identifier, int Count, const glm::vec2 &Position);
		_Weapon *CreateWeapon(const std::string &Identifier, int Count, const glm::vec2 &Position, bool Generate);
		_Armor *CreateArmor(const std::string &Identifier, int Count, const glm::vec2 &Position);
		void GetRandomDrop(const _ItemGroup *ItemGroup, _ObjectSpawn *ObjectSpawn);

		// Data
		std::unordered_map<std::string, std::string> Strings;
		std::unordered_map<std::string, _Animation *> Animations;

		std::unordered_map<std::string, const _Texture *> Textures;
		std::unordered_map<std::string, _Program *> Programs;
		std::unordered_map<std::string, glm::vec4> Colors;
		std::unordered_map<std::string, _Font *> Fonts;
		std::unordered_map<std::string, _Style *> Styles;
		std::unordered_map<std::string, _Element *> Elements;

		std::vector<std::string> MonsterSet;
		std::unordered_map<std::string, _MonsterTemplate> MonsterTable;
		std::unordered_map<std::string, _MiscItemTemplate> MiscItemTable;
		std::unordered_map<std::string, _UpgradeTemplate> UpgradeTable;
		std::unordered_map<std::string, _AmmoTemplate> AmmoTable;
		std::unordered_map<std::string, _WeaponTemplate> WeaponTable;
		std::unordered_map<std::string, _ArmorTemplate> ArmorTable;

	private:

		void LoadLevels(const std::string &Path);
		void LoadSkills(const std::string &Path);

		// Tables
		std::unordered_map<std::string, _ReelTemplate> ReelTable;
		std::unordered_map<std::string, AnimationTemplateStruct> AnimationTable;
		std::unordered_map<std::string, AttackSampleTemplateStruct> AttackSampleTable;
		std::unordered_map<std::string, _ParticleTemplate> ParticleTable;
		std::unordered_map<std::string, _WeaponParticleTemplate> WeaponParticleTable;
		std::unordered_map<std::string, _ItemGroup> ItemGroupTable;
		std::vector<LevelStruct> Levels;
		std::vector<SkillStruct> Skills;
		std::string AmmoTypeIdentifiers[AMMO_TYPES];

		// Data
		std::unordered_map<std::string, _Reel> Reels;
		std::unordered_map<std::string, const _Shader *> Shaders;
		_WeaponParticleTemplate BlankWeaponParticle;
};

extern _Assets Assets;
