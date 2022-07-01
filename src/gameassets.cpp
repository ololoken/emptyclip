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
void _GameAssets::LoadSounds(const std::string &Path, const std::string &SoundPath) {

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
		ae::_Sound *Sound = ae::Audio.LoadSound(SoundPath + SoundFile);

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

// Load sound groups
void _GameAssets::LoadSoundGroups(const std::string &Path) {

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
		_SoundGroup SoundGroup;
		for(int i = 0; i < SOUND_COUNT; i++) {
			std::string SoundID;
			std::getline(Buffer, SoundID, '\t');

			// Check for sound
			if(SoundID != "") {
				if(ae::Assets.Sounds.find(SoundID) == ae::Assets.Sounds.end())
					throw std::runtime_error(std::string(__func__) + " Unknown sound_id '" + SoundID + "'");

				SoundGroup.SoundID[i] = ae::Assets.Sounds.at(SoundID);
			}
			else
				SoundGroup.SoundID[i] = nullptr;
		}

		// Check for duplicates
		if(SoundGroups.find(ID) != SoundGroups.end())
			throw std::runtime_error(std::string(__func__) + " - Duplicate entry: " + ID);

		SoundGroups[ID] = SoundGroup;
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

		std::string ID;
		std::string TextureID;
		std::string ColorID;
		std::string FontID;
		std::getline(File, ID, '\t');
		std::getline(File, TextureID, '\t');
		std::getline(File, ColorID, '\t');
		std::getline(File, FontID, '\t');

		_ParticleTemplate Particle;
		File >> Particle.Type >> Particle.Count >> Particle.Lifetime >> Particle.StartDirection.x >> Particle.StartDirection.y >> Particle.TurnSpeed.x
				>> Particle.TurnSpeed.y >> Particle.VelocityScale.x >> Particle.VelocityScale.y >> Particle.AccelerationScale
				>> Particle.Size.x >> Particle.Size.y >> Particle.DeviationZ >> Particle.ScaleAspect >> Particle.AlphaSpeed;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for duplicates
		if(IsParticleLoaded(ID))
			throw std::runtime_error(std::string(__func__) + " - Duplicate entry: " + ID);

		// Get texture
		Particle.Texture = ae::Assets.Textures[TextureID];
		if(TextureID != "" && !Particle.Texture)
			throw std::runtime_error("Unable to find texture: " + TextureID);

		// Set color
		Particle.Color = ae::Assets.Colors[ColorID];

		// Get font
		Particle.Font = ae::Assets.Fonts[FontID];
		if(FontID != "" && !Particle.Font)
			throw std::runtime_error("Unable to find font: " + FontID);

		Particles[ID] = Particle;
	}

	File.close();
}

// Loads the weapon particles
void _GameAssets::LoadParticleGroups(const std::string &Path) {

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

		_ParticleGroup WeaponParticle;
		for(int i = 0; i < PARTICLE_COUNT; i++) {
			std::string ParticleID;
			std::getline(Buffer, ParticleID, '\t');

			if(ParticleID != "" && !IsParticleLoaded(ParticleID))
				throw std::runtime_error(std::string(__func__) + " - Cannot find particle: " + ParticleID);
			else if(ParticleID == "")
				WeaponParticle.ParticleTemplates[i] = nullptr;
			else
				WeaponParticle.ParticleTemplates[i] = GetParticleTemplate(ParticleID);
		}

		ParticleGroups[Name] = WeaponParticle;
	}

	File.close();
}

bool _GameAssets::IsParticleLoaded(const std::string &ID) { return Particles.find(ID) != Particles.end(); }

_ParticleTemplate *_GameAssets::GetParticleTemplate(const std::string &ID) {
	if(Particles.find(ID) == Particles.end())
		return nullptr;

	return &Particles[ID];
}
