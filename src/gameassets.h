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
#include <glm/vec2.hpp>
#include <string>
#include <unordered_map>
#include <vector>

// Forward Declarations
class _Particle;
class _Entity;
class _Player;
struct _ParticleTemplate;

// Stores information about a collection of sound samples used for attacking
struct _SoundGroup {
	_SoundGroup() { }

	std::string Sounds[SOUND_TYPES];
};

// Classes
class _GameAssets {

	public:

		void Init();
		void Close();

		void LoadSoundGroups(const std::string &Path);
		void LoadParticles(const std::string &Path);
		void LoadSounds(const std::string &Path, const std::string &SamplePath);

		void LoadWeaponParticles(const std::string &Path);
		void LoadMonsterAnimation();

		bool IsAttackSampleLoaded(const std::string &Identifier);
		bool IsParticleLoaded(const std::string &Identifier);
		bool IsWeaponParticleTemplateLoaded(const std::string &Identifier);

		_SoundGroup *GetAttackSampleTemplate(const std::string &Identifier);
		_ParticleTemplate *GetParticleTemplate(const std::string &Identifier);
		_WeaponParticleTemplate *GetWeaponParticleTemplate(const std::string &Identifer);

	private:

		// Tables
		std::unordered_map<std::string, _SoundGroup> SoundGroups;
		std::unordered_map<std::string, _ParticleTemplate> ParticleTable;
		std::unordered_map<std::string, _WeaponParticleTemplate> WeaponParticleTable;
};

extern _GameAssets GameAssets;
