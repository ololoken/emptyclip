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
#include <ae/assets.h>
#include <ae/files.h>
#include <ae/random.h>
#include <ae/graphics.h>
#include <ae/font.h>
#include <ae/texture.h>
#include <ae/program.h>
#include <ae/ui.h>
#include <audio.h>
#include <animation.h>
#include <objects/monster.h>
#include <objects/particle.h>
#include <objects/player.h>
#include <objects/weapon.h>
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

// Loads the reels table
void _GameAssets::LoadReelTable(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier;
		std::getline(File, Identifier, '\t');

		_ReelTemplate ReelTemplate;
		File >> ReelTemplate.PlaybackSpeed >> ReelTemplate.RepeatMode >> ReelTemplate.StartPosition;
		File.ignore(1);
		ReelTemplate.TextureFiles.clear();

		// Read rest of line into buffer
		std::string Line;
		std::getline(File, Line, '\n');
		std::stringstream Buffer(Line);

		// Get textures
		std::string TextureFile;
		while(std::getline(Buffer, TextureFile, '\t')) {
			if(TextureFile != "")
				ReelTemplate.TextureFiles.push_back(TextureFile);
		}

		// Check for duplicates
		if(IsReelLoaded(Identifier))
			throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " - Duplicate entry: " + Identifier);

		ReelTable[Identifier] = ReelTemplate;
	}

	File.close();
}

// Loads the animation table
void _GameAssets::LoadAnimationTable(const std::string &Path) {
	AnimationTemplateStruct AnimationTemplate;

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {
		std::string Identifier;
		std::getline(File, Identifier, '\t');
		AnimationTemplate.Identifiers.clear();

		// Read rest of line into buffer
		std::string Line;
		std::getline(File, Line, '\n');
		std::stringstream Buffer(Line);

		// Get reels
		std::string ReelIdentifier;
		while(std::getline(Buffer, ReelIdentifier, '\t')) {
			if(ReelIdentifier != "")
				AnimationTemplate.Identifiers.push_back(ReelIdentifier);
		}

		// Check for duplicates
		if(IsAnimationLoaded(Identifier))
			throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " - Duplicate entry: " + Identifier);

		AnimationTable[Identifier] = AnimationTemplate;
	}

	File.close();
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
		std::string Identifier;
		std::string SampleFile;
		std::getline(File, Identifier, '\t');
		std::getline(File, SampleFile, '\t');

		float Volume;
		int Limit;
		File >> Volume >> Limit;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Load sample file
		std::string Path = SamplePath + SampleFile;
		if(!Audio.LoadBuffer(Identifier, Path, Volume, Limit))
			throw std::runtime_error("Error loading: " + Path);
	}

	File.close();
}

// Loads the attack samples table
void _GameAssets::LoadSoundGroups(const std::string &Path) {
	AttackSampleTemplateStruct SampleTemplate;

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier;
		std::getline(File, Identifier, '\t');

		// Read rest of line into buffer
		std::string Line;
		std::getline(File, Line, '\n');
		std::stringstream Buffer(Line);

		// Read sounds
		for(int i = 0; i < SAMPLE_TYPES; i++)
			std::getline(Buffer, SampleTemplate.Samples[i], '\t');

		// Check for duplicates
		if(IsAttackSampleLoaded(Identifier))
			throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " - Duplicate entry: " + Identifier);

		AttackSampleTable[Identifier] = SampleTemplate;
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
			throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " - Duplicate entry: " + Identifier);

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
				throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " - Cannot find particle: " + ParticleIdentifier);
			else if(ParticleIdentifier == "")
				WeaponParticle.ParticleTemplates[i] = nullptr;
			else
				WeaponParticle.ParticleTemplates[i] = GetParticleTemplate(ParticleIdentifier);
		}

		WeaponParticleTable[Name] = WeaponParticle;
	}

	File.close();
}

// Loads the reel from the given identifier
void _GameAssets::LoadReel(const std::string &Identifier, const std::string &Path) {

	auto ReelTableIterator = ReelTable.find(Identifier);
	if(ReelTableIterator == ReelTable.end())
		return;

	auto ReelIterator = Reels.find(Identifier);
	if(ReelIterator == Reels.end()) {
		_Reel Reel;
		Reel.StartPosition = ReelTableIterator->second.StartPosition;
		Reel.RepeatMode = (RepeatType)(ReelTableIterator->second.RepeatMode);
		Reel.FramePeriod = ReelTableIterator->second.PlaybackSpeed;

		for(std::size_t i = 0; i < ReelTableIterator->second.TextureFiles.size(); i++) {
			std::string ReelPath = Path + ReelTableIterator->second.TextureFiles[i];
			ae::_Texture *Texture = new ae::_Texture(ReelPath, false, false, true, false);
			if(!Texture)
				throw std::runtime_error("Error loading: " + ReelPath);

			Reel.Textures.push_back(Texture);
		}

		Reels[Identifier] = Reel;
	}
}

// Loads the reels for an animation
void _GameAssets::LoadAnimation(const std::string &Identifier, const std::string &Path) {

	auto AnimationTableIterator = AnimationTable.find(Identifier);
	if(AnimationTableIterator == AnimationTable.end())
		return;

	// Check if animation has already been loaded
	auto AnimationIterator = Animations.find(Identifier);
	if(AnimationIterator == Animations.end()) {
		_Animation *Animation = new _Animation();

		// Load reels
		for(std::size_t i = 0; i < AnimationTableIterator->second.Identifiers.size(); i++) {
			LoadReel(AnimationTableIterator->second.Identifiers[i], Path);

			Animation->Reels.push_back(GetReel(AnimationTableIterator->second.Identifiers[i]));
		}

		Animation->ChangeReel(0);
		Animations[Identifier] = Animation;
	}
}

// Frees memory and textures used by a reel
void _GameAssets::UnloadReel(const std::string &Identifier) {

	auto ReelIterator = Reels.find(Identifier);
	if(ReelIterator != Reels.end()) {
		for(size_t i = 0; i < ReelIterator->second.Textures.size(); i++)
			delete ReelIterator->second.Textures[i];

		Reels.erase(ReelIterator);
	}
}

// Frees memory and textures used by an animation
void _GameAssets::UnloadAnimation(const std::string &Identifier) {

	auto AnimationIterator = Animations.find(Identifier);
	if(AnimationIterator == Animations.end())
		return;

	// Unload reels
	auto AnimationTableIterator = AnimationTable.find(Identifier);
	if(AnimationTableIterator != AnimationTable.end()) {
		for(size_t i = 0; i < AnimationTableIterator->second.Identifiers.size(); i++)
			UnloadReel(AnimationTableIterator->second.Identifiers[i]);
	}

	delete AnimationIterator->second;
	Animations.erase(AnimationIterator);
}

bool _GameAssets::IsAttackSampleLoaded(const std::string &Identifier) { return AttackSampleTable.find(Identifier) != AttackSampleTable.end(); }
bool _GameAssets::IsParticleLoaded(const std::string &Identifier) { return ParticleTable.find(Identifier) != ParticleTable.end(); }
bool _GameAssets::IsWeaponParticleTemplateLoaded(const std::string &Identifier) { return WeaponParticleTable.find(Identifier) != WeaponParticleTable.end(); }
bool _GameAssets::IsReelLoaded(const std::string &Identifier) { return ReelTable.find(Identifier) != ReelTable.end(); }
bool _GameAssets::IsAnimationLoaded(const std::string &Identifier) { return AnimationTable.find(Identifier) != AnimationTable.end(); }

_Reel *_GameAssets::GetReel(const std::string &Identifier) {
	if(Reels.find(Identifier) == Reels.end())
		return nullptr;

	return &Reels[Identifier];
}
AttackSampleTemplateStruct *_GameAssets::GetAttackSampleTemplate(const std::string &Identifier) {
	if(AttackSampleTable.find(Identifier) == AttackSampleTable.end())
		return nullptr;

	return &AttackSampleTable[Identifier];
}
_Animation *_GameAssets::GetAnimation(const std::string &Identifier) {
	if(Animations.find(Identifier) == Animations.end())
		return nullptr;

	return Animations[Identifier];
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
