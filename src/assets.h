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
#include <animation.h>
#include <string>
#include <unordered_map>
#include <vector>

// Forward Declarations
namespace ae {
	class _Texture;
}
class _Style;
class _Font;
class _Element;
class _Image;
class _Button;
class _TextBox;
class _Animation;
class _Particle;
class _Entity;
class _Player;
class _Program;
class _Shader;
struct _Reel;
struct _ParticleTemplate;

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
		void LoadParticles(const std::string &Path);
		void LoadPrograms(const std::string &Path);
		void LoadTextures(const std::string &Path);
		void LoadTextureDirectory(const std::string &Path, bool IsServer=false, bool Repeat=false, bool MipMaps=false, bool Nearest=false);
		void LoadSounds(const std::string &Path, const std::string &SamplePath);

		void LoadFonts(const std::string &Path, bool LoadFonts=true);
		void LoadReel(const std::string &Identifier, const std::string &Path);
		void LoadAnimation(const std::string &Identifier, const std::string &Path);
		void LoadWeaponParticles(const std::string &Path);
		void LoadMonsterAnimation();

		void LoadStyles(const std::string &Path);
		void LoadUI(const std::string &Path, bool CalculateBounds=true);
		void SaveUI(const std::string &Path);

		bool IsColorLoaded(const std::string &Identifier);
		bool IsTextureLoaded(const std::string &Identifier);
		bool IsAttackSampleLoaded(const std::string &Identifier);
		bool IsParticleLoaded(const std::string &Identifier);
		bool IsWeaponParticleTemplateLoaded(const std::string &Identifier);
		bool IsReelLoaded(const std::string &Identifier);
		bool IsAnimationLoaded(const std::string &Identifier);

		void UnloadReel(const std::string &Identifier);
		void UnloadAnimation(const std::string &Identifier);

		_Reel *GetReel(const std::string &Identifier);
		AttackSampleTemplateStruct *GetAttackSampleTemplate(const std::string &Identifier);
		_Animation *GetAnimation(const std::string &Identifier);
		_ParticleTemplate *GetParticleTemplate(const std::string &Identifier);
		_WeaponParticleTemplate *GetWeaponParticleTemplate(const std::string &Identifer);

		// Data
		std::unordered_map<std::string, std::string> Strings;
		std::unordered_map<std::string, _Animation *> Animations;

		std::unordered_map<std::string, const ae::_Texture *> Textures;
		std::unordered_map<std::string, _Program *> Programs;
		std::unordered_map<std::string, glm::vec4> Colors;
		std::unordered_map<std::string, _Font *> Fonts;
		std::unordered_map<std::string, _Style *> Styles;
		std::unordered_map<std::string, _Element *> Elements;

	private:

		// Tables
		std::unordered_map<std::string, _ReelTemplate> ReelTable;
		std::unordered_map<std::string, AnimationTemplateStruct> AnimationTable;
		std::unordered_map<std::string, AttackSampleTemplateStruct> AttackSampleTable;
		std::unordered_map<std::string, _ParticleTemplate> ParticleTable;
		std::unordered_map<std::string, _WeaponParticleTemplate> WeaponParticleTable;

		// Data
		std::unordered_map<std::string, _Reel> Reels;
		std::unordered_map<std::string, const _Shader *> Shaders;
};

extern _Assets Assets;
