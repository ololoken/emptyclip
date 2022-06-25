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
#include <gameassets.h>
#include <objects/monster.h>
#include <objects/particle.h>
#include <objects/player.h>
#include <objects/weapon.h>
#include <ae/assets.h>
#include <ae/files.h>
#include <ae/random.h>
#include <ae/graphics.h>
#include <ae/font.h>
#include <ae/texture.h>
#include <ae/program.h>
#include <ae/ui.h>
#include <ae/audio.h>
#include <constants.h>
#include <tinyxml2/tinyxml2.h>
#include <stdexcept>
#include <sstream>
#include <fstream>

_GameAssets GameAssets;

// Initialize
void _GameAssets::Init() {
}

// Shutdown
void _GameAssets::Close() {
}

// Load sounds
void _GameAssets::LoadSounds(const std::string &Path, const std::string &SamplePath) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {
		std::string ID;
		std::string SoundFile;
		std::getline(File, ID, '\t');
		std::getline(File, SoundFile, '\t');

		// Load sound
		ae::_Sound *Sound = ae::Audio.LoadSound(SamplePath + SoundFile);

		// Read parameters
		float Volume;
		int Limit;
		File >> Volume >> Limit;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		if(Sound) {
			Sound->Volume = Volume;
			Sound->Limit = Limit;
		}

		ae::Assets.Sounds[ID] = Sound;
	}

	File.close();
}

// Loads the attack samples table
void _GameAssets::LoadSoundGroups(const std::string &Path) {
	_SoundGroup SampleTemplate;

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {
		std::string ID;
		std::getline(File, ID, '\t');

		// Read rest of line into buffer
		std::string Line;
		std::getline(File, Line, '\n');
		std::stringstream Buffer(Line);

		// Read sounds
		for(int i = 0; i < SOUND_TYPES; i++) {
			std::string SoundID;
			std::getline(Buffer, SoundID, '\t');

			// Check for sound
			if(SoundID != "" && ae::Assets.Sounds.find(SoundID) == ae::Assets.Sounds.end())
				throw std::runtime_error(std::string(__func__) + " - Cannot find: " + SoundID);

			SampleTemplate.SoundID[i] = SoundID;
		}

		// Check for duplicates
		if(SoundGroups.find(ID) != SoundGroups.end())
			throw std::runtime_error(std::string(__func__) + " - Duplicate entry: " + ID);

		SoundGroups[ID] = SampleTemplate;
	}

	File.close();
}

// Loads the particle table
void _GameAssets::LoadParticles(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier;
		std::string TextureIdentifier;
		std::string ColorIdentifier;
		std::string FontIdentifier;
		std::getline(File, Identifier, '\t');
		std::getline(File, TextureIdentifier, '\t');
		std::getline(File, ColorIdentifier, '\t');
		std::getline(File, FontIdentifier, '\t');

		_ParticleTemplate Particle;
		File >> Particle.Type >> Particle.Count >> Particle.Lifetime >> Particle.StartDirection.x >> Particle.StartDirection.y >> Particle.TurnSpeed.x
				>> Particle.TurnSpeed.y >> Particle.VelocityScale.x >> Particle.VelocityScale.y >> Particle.AccelerationScale
				>> Particle.Size.x >> Particle.Size.y >> Particle.DeviationZ >> Particle.ScaleAspect >> Particle.AlphaSpeed;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for duplicates
		if(IsParticleLoaded(Identifier))
			throw std::runtime_error(std::string(__func__) + " - Duplicate entry: " + Identifier);

		// Get texture
		Particle.Texture = ae::Assets.Textures[TextureIdentifier];
		if(TextureIdentifier != "" && !Particle.Texture)
			throw std::runtime_error("Unable to find texture: " + TextureIdentifier);

		// Set color
		Particle.Color = ae::Assets.Colors[ColorIdentifier];

		// Get font
		Particle.Font = ae::Assets.Fonts[FontIdentifier];
		if(FontIdentifier != "" && !Particle.Font)
			throw std::runtime_error("Unable to find font: " + FontIdentifier);

		ParticleTable[Identifier] = Particle;
	}

	File.close();
}

// Loads the weapon particles
void _GameAssets::LoadWeaponParticles(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		// Get name
		std::string Name;
		std::getline(File, Name, '\t');

		// Read rest of line into buffer
		std::string Line;
		std::getline(File, Line, '\n');
		std::stringstream Buffer(Line);

		_WeaponParticleTemplate WeaponParticle;
		for(int i = 0; i < WEAPONPARTICLE_TYPES; i++) {
			std::string ParticleIdentifier;
			std::getline(Buffer, ParticleIdentifier, '\t');

			if(ParticleIdentifier != "" && !IsParticleLoaded(ParticleIdentifier))
				throw std::runtime_error(std::string(__func__) + " - Cannot find particle: " + ParticleIdentifier);
			else if(ParticleIdentifier == "")
				WeaponParticle.ParticleTemplates[i] = nullptr;
			else
				WeaponParticle.ParticleTemplates[i] = GetParticleTemplate(ParticleIdentifier);
		}

		WeaponParticleTable[Name] = WeaponParticle;
	}

	File.close();
}

bool _GameAssets::IsSoundGroupLoaded(const std::string &Identifier) { return SoundGroups.find(Identifier) != SoundGroups.end(); }
bool _GameAssets::IsParticleLoaded(const std::string &Identifier) { return ParticleTable.find(Identifier) != ParticleTable.end(); }
bool _GameAssets::IsWeaponParticleTemplateLoaded(const std::string &Identifier) { return WeaponParticleTable.find(Identifier) != WeaponParticleTable.end(); }

_SoundGroup *_GameAssets::GetSoundGroupTemplate(const std::string &Identifier) {
	if(SoundGroups.find(Identifier) == SoundGroups.end())
		return nullptr;

	return &SoundGroups[Identifier];
}
_ParticleTemplate *_GameAssets::GetParticleTemplate(const std::string &Identifier) {
	if(ParticleTable.find(Identifier) == ParticleTable.end())
		return nullptr;

	return &ParticleTable[Identifier];
}
_WeaponParticleTemplate *_GameAssets::GetWeaponParticleTemplate(const std::string &Identifier) {
	if(WeaponParticleTable.find(Identifier) == WeaponParticleTable.end())
		return nullptr;

	return &WeaponParticleTable[Identifier];
}
