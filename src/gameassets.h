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
namespace ae {
	class _Sound;
}

// Stores a list of sounds
struct _SoundGroup {
	_SoundGroup() { }

	const ae::_Sound *SoundID[SOUND_COUNT];
};

// Classes
class _GameAssets {

	public:

		void Init();
		void Close();

		void LoadSounds(const std::string &Path, const std::string &SoundPath);
		void LoadSoundGroups(const std::string &Path);
		void LoadParticles(const std::string &Path);
		void LoadParticleGroups(const std::string &Path);

		bool IsParticleLoaded(const std::string &ID);

		_ParticleTemplate *GetParticleTemplate(const std::string &ID);

		// Tables
		std::unordered_map<std::string, _SoundGroup> SoundGroups;
		std::unordered_map<std::string, _ParticleTemplate> Particles;
		std::unordered_map<std::string, _ParticleGroup> ParticleGroups;

	private:

};

extern _GameAssets GameAssets;
