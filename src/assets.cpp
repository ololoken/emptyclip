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
#include <assets.h>
#include <font.h>
#include <texture.h>
#include <audio.h>
#include <random.h>
#include <utils.h>
#include <animation.h>
#include <program.h>
#include <ui/element.h>
#include <ui/label.h>
#include <ui/button.h>
#include <ui/image.h>
#include <ui/textbox.h>
#include <objects/monster.h>
#include <objects/particle.h>
#include <objects/player.h>
#include <objects/weapon.h>
#include <objects/armor.h>
#include <objects/misc.h>
#include <objects/upgrade.h>
#include <objects/ammo.h>
#include <constants.h>
#include <stdexcept>
#include <sstream>

_Assets Assets;

// Initialize
void _Assets::Init() {

	LoadPrograms("tables/programs.tsv");
	LoadStrings("tables/strings.tsv");
	LoadLevels("tables/levels.tsv");
	LoadSkills("tables/skills.tsv");
	LoadFonts("tables/fonts.tsv", false);
	LoadTextures("tables/textures/main.tsv");
	LoadTextures("tables/textures/map.tsv");
	LoadColors("tables/colors.tsv");
	LoadSounds("tables/sounds.tsv", "sounds/");
	LoadSoundGroups("tables/sound_groups.tsv");
	LoadParticles("tables/particles.tsv");
	LoadWeaponParticles("tables/weaponparticles.tsv");
	LoadStyles("tables/ui/styles.tsv");
	LoadElements("tables/ui/elements.tsv");
	LoadImages("tables/ui/images.tsv");
	LoadButtons("tables/ui/buttons.tsv");
	LoadTextBoxes("tables/ui/textboxes.tsv");
	LoadLabels("tables/ui/labels.tsv");
	LoadReelTable("tables/reels.tsv");
	LoadAnimationTable("tables/animation.tsv");
	LoadMiscItemTable("tables/items.tsv");
	LoadUpgradeTable("tables/upgrades.tsv");
	LoadAmmoTable("tables/ammo.tsv");
	LoadWeaponTable("tables/weapons.tsv");
	LoadArmorTable("tables/armor.tsv");
	LoadItemDrops("tables/itemdrops.tsv");
	LoadMonsterTable("tables/monsters.tsv");

	LoadAnimation("player_torso", "textures/player/");
	LoadAnimation("player_legs", "textures/player/");
	LoadFonts("tables/fonts.tsv");

	BlankWeaponParticle = _WeaponParticleTemplate();
}

// Shutdown
void _Assets::Close() {

	for(const auto &Monster : MonsterTable)
		UnloadAnimation(Monster.second.AnimationIdentifier);
	MonsterSet.clear();
	UnloadAnimation("player_torso");
	UnloadAnimation("player_legs");

	for(const auto &Style : Styles)
		delete Style.second;

	Styles.clear();

	for(const auto &Element : Elements)
		delete Element.second;

	Elements.clear();

	for(const auto &Font : Fonts)
		delete Font.second;

	Fonts.clear();

	for(const auto &Texture : Textures)
		delete Texture.second;

	for(const auto &Program : Programs)
		delete Program.second;

	for(const auto &Shader : Shaders)
		delete Shader.second;

	Textures.clear();
	Programs.clear();
	Shaders.clear();
}

// Loads the strings
void _Assets::LoadStrings(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Ignore the first line
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		std::string ID;
		std::string Text;
		std::getline(File, ID, '\t');
		std::getline(File, Text, '\n');

		// Check for duplicates
		if(Strings.find(ID) != Strings.end())
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + ID);

		Strings[ID] = Text;
	}

	File.close();
}

// Loads the fonts
void _Assets::LoadFonts(const std::string &Path, bool LoadFonts) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		// Read strings
		std::string Name;
		std::string FontFile;
		std::string ProgramName;
		std::getline(File, Name, '\t');
		std::getline(File, FontFile, '\t');
		std::getline(File, ProgramName, '\t');

		// Check for duplicates
		if(!LoadFonts && Fonts[Name])
			throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " - Duplicate entry: " + Name);

		// Find program
		if(Programs.find(ProgramName) == Programs.end())
			throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " - Cannot find program: " + ProgramName);

		// Get size
		uint32_t Size;
		File >> Size;

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Load font
		if(LoadFonts) {

			// Check for font name
			if(Fonts.find(Name) == Fonts.end())
				throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " - Cannot find font: " + Name);

			// Load font
			Fonts[Name]->Load(Name, FontFile, Programs[ProgramName], Size);
		}
		else {

			// Create empty font
			_Font *Font = new _Font();
			Font->ID = Name;
			Fonts[Name] = Font;
		}
	}

	File.close();
}

// Loads the level table
void _Assets::LoadLevels(const std::string &Path) {
	LevelStruct Level;

	// Load file
	std::ifstream InputFile(Path, std::ios::in);
	if(!InputFile) {
		throw std::runtime_error("Error loading: " + Path);
	}

	Levels.clear();

	// Load the data
	InputFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	for(int i = 0; i < GAME_MAX_LEVEL; i++) {
		if(InputFile.eof()) {
			throw std::runtime_error("LoadLevels - Premature end of file");
		}

		InputFile >> Level.Experience >> Level.HealthBonus >> Level.DamageBlockBonus >> Level.SkillPoints;

		Levels.push_back(Level);
	}
}

// Loads the skill table
void _Assets::LoadSkills(const std::string &Path) {
	SkillStruct Skill;

	// Load file
	std::ifstream InputFile(Path, std::ios::in);
	if(!InputFile) {
		throw std::runtime_error("LoadSkills: Cannot open " + Path);
	}

	Skills.clear();

	// Load the data
	InputFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	for(int i = 0; i < GAME_SKILLLEVELS+1; i++) {
		if(InputFile.eof()) {
			throw std::runtime_error("Premature end of file" + Path);
		}

		for(int i = 0; i < SKILL_COUNT; i++)
			InputFile >> Skill.Data[i];

		Skills.push_back(Skill);
	}
}

// Loads the color table
void _Assets::LoadColors(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Add default color
	glm::vec4 Color(1.0f);
	Colors[""] = Color;

	// Read table
	while(!File.eof() && File.peek() != EOF) {

		std::string Name;
		std::getline(File, Name, '\t');

		File >> Color.r >> Color.g >> Color.b >> Color.a;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for duplicates
		if(Colors.find(Name) != Colors.end())
			throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " - Duplicate entry: " + Name);

		Colors[Name] = Color;
	}

	File.close();
}

// Loads the reels table
void _Assets::LoadReelTable(const std::string &Path) {

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
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Identifier);

		ReelTable[Identifier] = ReelTemplate;
	}

	File.close();
}

// Loads the animation table
void _Assets::LoadAnimationTable(const std::string &Path) {
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
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Identifier);

		AnimationTable[Identifier] = AnimationTemplate;
	}

	File.close();
}

// Load shader programs
void _Assets::LoadPrograms(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str(), std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {
		std::string Name;
		std::string VertexPath;
		std::string FragmentPath;
		std::getline(File, Name, '\t');
		std::getline(File, VertexPath, '\t');
		std::getline(File, FragmentPath, '\t');

		// Get integer parameters
		GLuint Attribs;
		int MaxLights;
		File >> Attribs >> MaxLights;

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for duplicates
		if(Programs[Name])
			throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " - Duplicate entry: " + Name);

		// Load vertex shader
		if(Shaders.find(VertexPath) == Shaders.end())
			Shaders[VertexPath] = new _Shader(VertexPath, GL_VERTEX_SHADER);

		// Load fragment shader
		if(Shaders.find(FragmentPath) == Shaders.end())
			Shaders[FragmentPath] = new _Shader(FragmentPath, GL_FRAGMENT_SHADER);

		// Create program
		Programs[Name] = new _Program(Name, Shaders[VertexPath], Shaders[FragmentPath], Attribs, MaxLights);
	}

	File.close();
}

// Load textures
void _Assets::LoadTextures(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Add null texture
	Textures[""] = nullptr;

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		std::string Name;
		std::string TextureFile;
		std::getline(File, Name, '\t');
		std::getline(File, TextureFile, '\t');

		bool Repeat, MipMaps;
		File >> Repeat >> MipMaps;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Load texture
		std::string Path = "textures/" + TextureFile;
		_Texture *Texture = new _Texture(Path, false, Repeat, MipMaps, false);
		if(!Texture)
			throw std::runtime_error(std::string(__FUNCTION__) + " - Error loading: " + Path);

		// Check for duplicates
		if(IsTextureLoaded(Name))
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Name);

		Textures[Name] = Texture;
	}

	File.close();
}

// Load sounds
void _Assets::LoadSounds(const std::string &Path, const std::string &SamplePath) {

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
void _Assets::LoadSoundGroups(const std::string &Path) {
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
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Identifier);

		AttackSampleTable[Identifier] = SampleTemplate;
	}

	File.close();
}

// Loads the particle table
void _Assets::LoadParticles(const std::string &Path) {

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
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Identifier);

		// Get texture
		Particle.Texture = Assets.Textures[TextureIdentifier];
		if(TextureIdentifier != "" && !Particle.Texture)
			throw std::runtime_error("Unable to find texture: " + TextureIdentifier);

		// Set color
		Particle.Color = Colors[ColorIdentifier];

		// Get font
		Particle.Font = Assets.Fonts[FontIdentifier];
		if(FontIdentifier != "" && !Particle.Font)
			throw std::runtime_error("Unable to find font: " + FontIdentifier);

		ParticleTable[Identifier] = Particle;
	}

	File.close();
}

// Loads the weapon particles
void _Assets::LoadWeaponParticles(const std::string &Path) {

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
				throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find particle: " + ParticleIdentifier);
			else if(ParticleIdentifier == "")
				WeaponParticle.ParticleTemplates[i] = nullptr;
			else
				WeaponParticle.ParticleTemplates[i] = GetParticleTemplate(ParticleIdentifier);
		}

		WeaponParticleTable[Name] = WeaponParticle;
	}

	File.close();
}

// Loads the monsters table
void _Assets::LoadMonsterTable(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		_MonsterTemplate Monster;
		std::string Name;
		std::string ColorName;
		std::string WeaponParticlesIdentifier;
		std::getline(File, Name, '\t');
		std::getline(File, Monster.Name, '\t');
		std::getline(File, Monster.AnimationIdentifier, '\t');
		std::getline(File, WeaponParticlesIdentifier, '\t');
		std::getline(File, Monster.SamplesIdentifier, '\t');
		std::getline(File, Monster.ItemGroupIdentifier, '\t');
		std::getline(File, ColorName, '\t');

		File >> Monster.Level >> Monster.Health >> Monster.DamageBlock >> Monster.BehaviorType >> Monster.ViewRange >> Monster.ExperienceGiven
			>> Monster.MovementSpeed >> Monster.Radius >> Monster.Scale >> Monster.CurrentSpeed >> Monster.Accuracy
			>> Monster.AttackRange >> Monster.MinDamage >> Monster.MaxDamage >> Monster.FirePeriod >> Monster.WeaponType;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Set color
		if(ColorName != "") {
			if(!IsColorLoaded(ColorName))
				throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find color: " + ColorName);

			Monster.Color = Colors[ColorName];
		}
		else
			Monster.Color = COLOR_WHITE;

		// Check for item group
		if(Monster.ItemGroupIdentifier != "" && ItemGroupTable.find(Monster.ItemGroupIdentifier) == ItemGroupTable.end())
			throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find item group: " + Monster.ItemGroupIdentifier + " in " + Name);

		// Check for animation
		if(!IsAnimationLoaded(Monster.AnimationIdentifier))
			throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find animation: " + Monster.AnimationIdentifier + " in " + Name);

		// Check for samples
		if(!IsAttackSampleLoaded(Monster.SamplesIdentifier))
			throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find sample: " + Monster.SamplesIdentifier + " in " + Name);

		// Set particles
		if(IsWeaponParticleTemplateLoaded(WeaponParticlesIdentifier))
			Monster.WeaponParticles = GetWeaponParticleTemplate(WeaponParticlesIdentifier);
		else
			Monster.WeaponParticles = &BlankWeaponParticle;

		// Check for duplicates
		if(IsMonsterLoaded(Name))
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Name);

		MonsterTable[Name] = Monster;
	}

	File.close();
}

// Loads the misc item table
void _Assets::LoadMiscItemTable(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		_MiscItemTemplate MiscItem;
		std::string Identifier;
		std::string ColorName;
		std::getline(File, Identifier, '\t');
		std::getline(File, MiscItem.Name, '\t');
		std::getline(File, MiscItem.IconIdentifier, '\t');
		std::getline(File, ColorName, '\t');
		File >> MiscItem.Type >> MiscItem.Level;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for loaded textures
		if(!IsTextureLoaded(MiscItem.IconIdentifier))
			throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find texture: " + MiscItem.IconIdentifier);

		// Set color
		if(ColorName != "") {
			if(!IsColorLoaded(ColorName))
				throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find color: " + ColorName);

			MiscItem.Color = Colors[ColorName];
		}
		else
			MiscItem.Color = COLOR_WHITE;

		// Check for duplicates
		if(IsMiscItemLoaded(Identifier))
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Identifier);

		MiscItemTable[Identifier] = MiscItem;
	}

	File.close();
}

// Loads the upgrade table
void _Assets::LoadUpgradeTable(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		_UpgradeTemplate Upgrade;
		std::string Name;
		std::string ColorName;
		std::getline(File, Name, '\t');
		std::getline(File, Upgrade.Name, '\t');
		std::getline(File, Upgrade.IconIdentifier, '\t');
		std::getline(File, ColorName, '\t');
		File >> Upgrade.UpgradeType >> Upgrade.WeaponType >> Upgrade.Bonus;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for loaded textures
		if(!IsTextureLoaded(Upgrade.IconIdentifier))
			throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find texture: " + Upgrade.IconIdentifier);

		// Set color
		if(ColorName != "") {
			if(!IsColorLoaded(ColorName))
				throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find color: " + ColorName);

			Upgrade.Color = Colors[ColorName];
		}
		else
			Upgrade.Color = COLOR_WHITE;

		// Check for duplicates
		if(IsUpgradeLoaded(Name))
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Name);

		UpgradeTable[Name] = Upgrade;
	}

	File.close();
}

// Loads the ammo table
void _Assets::LoadAmmoTable(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		_AmmoTemplate Ammo;
		std::string Name;
		std::string ColorName;
		std::getline(File, Name, '\t');
		std::getline(File, Ammo.Name, '\t');
		std::getline(File, Ammo.IconIdentifier, '\t');
		std::getline(File, ColorName, '\t');

		File >> Ammo.AmmoType;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for loaded textures
		if(!IsTextureLoaded(Ammo.IconIdentifier))
			throw std::runtime_error(std::string(__FUNCTION__) + " - Texture not found: " + Ammo.IconIdentifier);

		// Set color
		if(ColorName != "") {
			if(!IsColorLoaded(ColorName))
				throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find color: " + ColorName);

			Ammo.Color = Colors[ColorName];
		}
		else
			Ammo.Color = COLOR_WHITE;

		// Check for duplicates
		if(IsAmmoLoaded(Name))
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Name);

		AmmoTypeIdentifiers[Ammo.AmmoType] = Name;
		AmmoTable[Name] = Ammo;
	}

	File.close();
}

// Loads the weapon table
void _Assets::LoadWeaponTable(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {
		_WeaponTemplate Weapon;
		AttackSampleTemplateStruct *AttackSample;

		std::string Name;
		std::string ColorName;
		std::string SamplesIdentifier;
		std::string WeaponParticlesIdentifier;
		std::getline(File, Name, '\t');
		std::getline(File, Weapon.Name, '\t');
		std::getline(File, Weapon.IconIdentifier, '\t');
		std::getline(File, SamplesIdentifier, '\t');
		std::getline(File, WeaponParticlesIdentifier, '\t');
		std::getline(File, ColorName, '\t');

		File >> Weapon.Type >> Weapon.ZoomScale >> Weapon.MinAccuracy >> Weapon.MaxAccuracy >> Weapon.Recoil >> Weapon.RecoilRegen >> Weapon.Range
				>> Weapon.FireRate >> Weapon.FirePeriod >> Weapon.ReloadPeriod >> Weapon.MinComponents >> Weapon.MaxComponents
				>> Weapon.MinDamage >> Weapon.MaxDamage	>> Weapon.BulletsShot >> Weapon.RoundSize >> Weapon.AmmoType;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for loaded textures
		if(!IsTextureLoaded(Weapon.IconIdentifier))
			throw std::runtime_error(std::string(__FUNCTION__) + " - Texture not found: " + Weapon.IconIdentifier);

		// Set color
		if(ColorName != "") {
			if(!IsColorLoaded(ColorName))
				throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find color: " + ColorName);

			Weapon.Color = Colors[ColorName];
		}
		else
			Weapon.Color = COLOR_WHITE;

		// Check for attack sample
		if(!IsAttackSampleLoaded(SamplesIdentifier))
			throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find sample: " + SamplesIdentifier);

		// Set samples
		AttackSample = GetAttackSampleTemplate(SamplesIdentifier);
		for(int i = 0; i < SAMPLE_TYPES; i++) {
			if(AttackSample)
				Weapon.Samples[i] = AttackSample->Samples[i];
		}

		// Set particles
		if(IsWeaponParticleTemplateLoaded(WeaponParticlesIdentifier))
			Weapon.WeaponParticles = GetWeaponParticleTemplate(WeaponParticlesIdentifier);
		else
			Weapon.WeaponParticles = &BlankWeaponParticle;

		// Check for duplicates
		if(IsWeaponLoaded(Name))
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Name);

		WeaponTable[Name] = Weapon;
	}

	File.close();
}

// Loads the armor table
void _Assets::LoadArmorTable(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		_ArmorTemplate Armor;
		std::string Name;
		std::string ColorName;
		std::getline(File, Name, '\t');
		std::getline(File, Armor.Name, '\t');
		std::getline(File, Armor.IconIdentifier, '\t');
		std::getline(File, ColorName, '\t');

		File >> Armor.StrengthRequirement >> Armor.DamageBlock >> Armor.DamageResist >> Armor.MovementSpeed;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for loaded textures
		if(!IsTextureLoaded(Armor.IconIdentifier))
			throw std::runtime_error(std::string(__FUNCTION__) + " - Texture not found: " + Armor.IconIdentifier);

		// Set color
		if(ColorName != "") {
			if(!IsColorLoaded(ColorName))
				throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find color: " + ColorName);

			Armor.Color = Colors[ColorName];
		}
		else
			Armor.Color = COLOR_WHITE;

		// Check for duplicates
		if(IsArmorLoaded(Name))
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Name);

		ArmorTable[Name] = Armor;
	}

	File.close();
}

// Load item drop table
void _Assets::LoadItemDrops(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip first two fields
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\t');
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\t');

	// Read rest of line into buffer
	std::string Line;
	std::getline(File, Line, '\n');
	std::stringstream Buffer(Line);

	// Get item drop names first
	int ItemDrops = 0;
	std::vector<std::string> ItemDropNames;
	std::string DropName;
	while(std::getline(Buffer, DropName, '\t')) {
		if(DropName == "")
			continue;

		ItemDropNames.push_back(DropName);

		auto ItemGroupTableIterator = ItemGroupTable.find(DropName);
		if(ItemGroupTableIterator == ItemGroupTable.end()) {
			_ItemGroup ItemGroup;
			ItemGroup.Total = 0;
			ItemGroup.Quantity = 1;
			ItemGroupTable[DropName] = ItemGroup;
		}

		ItemDrops++;
	}

	// Read rest of data
	while(!File.eof() && File.peek() != EOF) {

		ItemGroupEntryStruct ItemGroupEntry;
		File >> ItemGroupEntry.Type;
		File.ignore(1, '\t');
		std::getline(File, ItemGroupEntry.ItemIdentifier, '\t');

		// See if items exist
		switch(ItemGroupEntry.Type) {
			case -1:
			break;
			case _Object::MEDKIT:
				if(!IsMiscItemLoaded(ItemGroupEntry.ItemIdentifier))
					throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find: " + ItemGroupEntry.ItemIdentifier + " in " + Path);
			break;
			case _Object::AMMO:
				if(!IsAmmoLoaded(ItemGroupEntry.ItemIdentifier))
					throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find: " + ItemGroupEntry.ItemIdentifier + " in " + Path);
			break;
			case _Object::UPGRADE:
				if(!IsUpgradeLoaded(ItemGroupEntry.ItemIdentifier))
					throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find: " + ItemGroupEntry.ItemIdentifier + " in " + Path);
			break;
			case _Object::WEAPON:
				if(!IsWeaponLoaded(ItemGroupEntry.ItemIdentifier))
					throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find: " + ItemGroupEntry.ItemIdentifier + " in " + Path);
			break;
			case _Object::ARMOR:
				if(!IsArmorLoaded(ItemGroupEntry.ItemIdentifier))
					throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find: " + ItemGroupEntry.ItemIdentifier + " in " + Path);
			break;
			case _Object::KEY:
				if(!IsMiscItemLoaded(ItemGroupEntry.ItemIdentifier))
					throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find: " + ItemGroupEntry.ItemIdentifier + " in " + Path);
			break;
			default:
				throw std::runtime_error(std::string(__FUNCTION__) + " - Bad item type: " + ItemGroupEntry.ItemIdentifier + " in " + Path);
			break;
		}

		// Add counts to item groups
		for(int i = 0; i < ItemDrops; i++) {
			File >> ItemGroupEntry.Count;
			if(ItemGroupEntry.Count <= 0)
				continue;

			ItemGroupTable[ItemDropNames[i]].Total += ItemGroupEntry.Count;
			ItemGroupEntry.Count = ItemGroupTable[ItemDropNames[i]].Total;
			ItemGroupTable[ItemDropNames[i]].Entries.push_back(ItemGroupEntry);
		}

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}

	File.close();
}

// Loads a monster set
void _Assets::LoadMonsterSet(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		return;

	MonsterSet.clear();

	// Read file
	std::string Identifier;
	while(!File.eof() && File.peek() != EOF) {
		File >> Identifier;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		if(!IsMonsterLoaded(Identifier))
			throw std::runtime_error("Cannot find monster: " + Identifier);

		MonsterSet.push_back(Identifier);
	}

	File.close();

	// Load the animation textures
	LoadMonsterAnimation();
}

// Loads the reel from the given identifier
void _Assets::LoadReel(const std::string &Identifier, const std::string &Path) {

	auto ReelTableIterator = ReelTable.find(Identifier);
	if(ReelTableIterator == ReelTable.end())
		return;

	auto ReelIterator = Reels.find(Identifier);
	if(ReelIterator == Reels.end()) {
		_Reel Reel;
		Reel.StartPosition = ReelTableIterator->second.StartPosition;
		Reel.RepeatMode = static_cast<RepeatType>(ReelTableIterator->second.RepeatMode);
		Reel.PlaybackSpeed = ReelTableIterator->second.PlaybackSpeed;

		for(int i = 0; i < static_cast<int>(ReelTableIterator->second.TextureFiles.size()); i++) {
			std::string ReelPath = Path + ReelTableIterator->second.TextureFiles[i];
			_Texture *Texture = new _Texture(ReelPath, false, false, true, false);
			if(!Texture)
				throw std::runtime_error("Error loading: " + ReelPath);

			Reel.Textures.push_back(Texture);
		}

		Reels[Identifier] = Reel;
	}
}

// Loads the reels for an animation
void _Assets::LoadAnimation(const std::string &Identifier, const std::string &Path) {

	auto AnimationTableIterator = AnimationTable.find(Identifier);
	if(AnimationTableIterator == AnimationTable.end())
		return;

	// Check if animation has already been loaded
	auto AnimationIterator = Animations.find(Identifier);
	if(AnimationIterator == Animations.end()) {
		_Animation *Animation = new _Animation();

		// Load reels
		for(int i = 0; i < static_cast<int>(AnimationTableIterator->second.Identifiers.size()); i++) {
			LoadReel(AnimationTableIterator->second.Identifiers[i], Path);

			Animation->Reels.push_back(GetReel(AnimationTableIterator->second.Identifiers[i]));
		}

		Animation->ChangeReel(0);
		Animations[Identifier] = Animation;
	}
}

// Loads all the monster animations in a monster set
void _Assets::LoadMonsterAnimation() {
	for(size_t i = 0; i < MonsterSet.size(); i++)
		LoadAnimation(GetMonsterTemplate(MonsterSet[i])->AnimationIdentifier, "textures/monsters/");
}

// Loads the styles
void _Assets::LoadStyles(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		std::string Name;
		std::string BackgroundColorName;
		std::string BorderColorName;
		std::string ProgramName;
		std::string TextureName;
		std::string TextureColorName;
		std::getline(File, Name, '\t');
		std::getline(File, BackgroundColorName, '\t');
		std::getline(File, BorderColorName, '\t');
		std::getline(File, ProgramName, '\t');
		std::getline(File, TextureName, '\t');
		std::getline(File, TextureColorName, '\t');

		// Check for background color
		if(BackgroundColorName != "" && Colors.find(BackgroundColorName) == Colors.end())
			throw std::runtime_error("Unable to find background color: " + BackgroundColorName + " for style: " + Name);

		// Check for border color
		if(BorderColorName != "" && Colors.find(BorderColorName) == Colors.end())
			throw std::runtime_error("Unable to find border color: " + BorderColorName + " for style: " + Name);

		// Check for texture color
		if(TextureColorName != "" && Colors.find(TextureColorName) == Colors.end())
			throw std::runtime_error("Unable to find texture color: " + TextureColorName + " for style: " + Name);

		// Find program
		if(Programs.find(ProgramName) == Programs.end())
			throw std::runtime_error("Cannot find program: " + ProgramName + " for style: " + Name);

		// Check for texture
		if(TextureName != "" && Textures.find(TextureName) == Textures.end())
			throw std::runtime_error("Unable to find texture: " + TextureName + " for style: " + Name);

		bool Stretch;
		File >> Stretch;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Get colors
		glm::vec4 BackgroundColor = Colors[BackgroundColorName];
		glm::vec4 BorderColor = Colors[BorderColorName];
		glm::vec4 TextureColor = Colors[TextureColorName];

		// Get textures
		const _Texture *Texture = Textures[TextureName];

		// Create style
		_Style *Style = new _Style();
		Style->Name = Name;
		Style->HasBackgroundColor = BackgroundColorName != "";
		Style->HasBorderColor = BorderColorName != "";
		Style->BackgroundColor = BackgroundColor;
		Style->BorderColor = BorderColor;
		Style->Program = Programs[ProgramName];
		Style->Texture = Texture;
		Style->TextureColor = TextureColor;
		Style->Stretch = Stretch;

		// Check for duplicates
		if(Styles.find(Name) != Styles.end())
			throw std::runtime_error("Duplicate style Name: " + Name);

		Styles[Name] = Style;
	}

	File.close();
}

// Loads the ui elements
void _Assets::LoadElements(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File) {
		throw std::runtime_error("Error loading: " + Path);
	}

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier;
		std::string ParentIdentifier;
		std::string StyleIdentifier;
		std::getline(File, Identifier, '\t');
		std::getline(File, ParentIdentifier, '\t');
		std::getline(File, StyleIdentifier, '\t');

		glm::ivec2 Offset, Size;
		_Alignment Alignment;
		bool MaskOutside;
		File >> Offset.x >> Offset.y >> Size.x >> Size.y >> Alignment.Horizontal >> Alignment.Vertical >> MaskOutside;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Look for parent
		_Element *ParentElement = nullptr;
		if(ParentIdentifier != "") {
			ParentElement = Elements[ParentIdentifier];
			if(!ParentElement) {
				throw std::runtime_error("Parent element not found: " + ParentIdentifier);
			}
		}

		// Get style
		_Style *Style = nullptr;
		if(StyleIdentifier != "") {
			Style = Styles[StyleIdentifier];
			if(!Style)
				throw std::runtime_error("Unable to find style: " + StyleIdentifier);
		}

		// Create
		_Element *Element = new _Element(Identifier, ParentElement, Offset, Size, Alignment, Style, MaskOutside);

		// Check for duplicates
		if(Elements[Identifier])
			throw std::runtime_error("Duplicate element identifier: " + Identifier);

		// Add as child for parent
		if(ParentElement) {
			ParentElement->AddChild(Element);
		}

		Elements[Identifier] = Element;
	}

	File.close();
}

// Loads labels elements
void _Assets::LoadLabels(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File) {
		throw std::runtime_error("Error loading: " + Path);
	}

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		std::string Name;
		std::string ParentName;
		std::string FontName;
		std::string ColorName;
		std::string Text;
		std::getline(File, Name, '\t');
		std::getline(File, ParentName, '\t');
		std::getline(File, FontName, '\t');
		std::getline(File, ColorName, '\t');
		std::getline(File, Text, '\t');

		glm::ivec2 Offset, Size;
		_Alignment Alignment;
		File >> Offset.x >> Offset.y >> Size.x >> Size.y >> Alignment.Horizontal >> Alignment.Vertical;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Look for parent
		_Element *ParentElement = nullptr;
		if(ParentName != "") {
			ParentElement = Elements[ParentName];
			if(!ParentElement)
				throw std::runtime_error("Parent element not found: " + ParentName);
		}

		// Get font
		_Font *Font = Fonts[FontName];
		if(!Font)
			throw std::runtime_error("Unable to find font: " + FontName);

		// Get color
		glm::vec4 Color = Colors[ColorName];

		// Create
		_Label *Element = new _Label(Name, ParentElement, Offset, Size, Alignment, Font, Color, Text);

		// Check for duplicates
		if(Elements[Name])
			throw std::runtime_error("Duplicate element identifier: " + Name);

		// Add as child for parent
		if(ParentElement)
			ParentElement->AddChild(Element);

		Elements[Name] = Element;
	}

	File.close();
}

// Loads image elements
void _Assets::LoadImages(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File) {
		throw std::runtime_error("Error loading: " + Path);
	}

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier;
		std::string ParentIdentifier;
		std::string TextureIdentifier;
		std::string ColorIdentifier;
		std::getline(File, Identifier, '\t');
		std::getline(File, ParentIdentifier, '\t');
		std::getline(File, TextureIdentifier, '\t');
		std::getline(File, ColorIdentifier, '\t');

		glm::ivec2 Offset, Size;
		_Alignment Alignment;
		int Stretch;
		File >> Offset.x >> Offset.y >> Size.x >> Size.y >> Alignment.Horizontal >> Alignment.Vertical >> Stretch;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Look for parent
		_Element *ParentElement = nullptr;
		if(ParentIdentifier != "") {
			ParentElement = Elements[ParentIdentifier];
			if(!ParentElement)
				throw std::runtime_error("Parent element not found: " + ParentIdentifier);
		}

		// Get texture
		const _Texture *Texture = Textures[TextureIdentifier];

		// Get color
		glm::vec4 Color = Colors[ColorIdentifier];

		// Create
		_Image *Element = new _Image(Identifier, ParentElement, Offset, Size, Alignment, Texture, Color, Stretch);

		// Check for duplicates
		if(Elements[Identifier])
			throw std::runtime_error("Duplicate element identifier: " + Identifier);

		// Add as child for parent
		if(ParentElement)
			ParentElement->AddChild(Element);

		Elements[Identifier] = Element;
	}

	File.close();
}

// Loads button elements
void _Assets::LoadButtons(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier;
		std::string ParentIdentifier;
		std::string StyleIdentifier;
		std::string HoverStyleIdentifier;
		std::getline(File, Identifier, '\t');
		std::getline(File, ParentIdentifier, '\t');
		std::getline(File, StyleIdentifier, '\t');
		std::getline(File, HoverStyleIdentifier, '\t');

		glm::ivec2 Offset, Size;
		_Alignment Alignment;
		File >> Offset.x >> Offset.y >> Size.x >> Size.y >> Alignment.Horizontal >> Alignment.Vertical;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Look for parent
		_Element *ParentElement = nullptr;
		if(ParentIdentifier != "") {
			ParentElement = Elements[ParentIdentifier];
			if(!ParentElement)
				throw std::runtime_error("Parent element not found: " + ParentIdentifier);
		}

		// Get style
		_Style *Style = Styles[StyleIdentifier];
		_Style *HoverStyle = Styles[HoverStyleIdentifier];

		// Create
		_Button *Element = new _Button(Identifier, ParentElement, Offset, Size, Alignment, Style, HoverStyle);

		// Check for duplicates
		if(Elements[Identifier])
			throw std::runtime_error("Duplicate element identifier: " + Identifier);

		// Add as child for parent
		if(ParentElement)
			ParentElement->AddChild(Element);

		Elements[Identifier] = Element;
	}

	File.close();
}

// Loads textbox elements
void _Assets::LoadTextBoxes(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File) {
		throw std::runtime_error("Error loading: " + Path);
	}

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier;
		std::string ParentIdentifier;
		std::string StyleIdentifier;
		std::string FontIdentifier;
		std::getline(File, Identifier, '\t');
		std::getline(File, ParentIdentifier, '\t');
		std::getline(File, StyleIdentifier, '\t');
		std::getline(File, FontIdentifier, '\t');

		glm::ivec2 Offset, Size;
		_Alignment Alignment;
		int MaxLength;
		File >> Offset.x >> Offset.y >> Size.x >> Size.y >> Alignment.Horizontal >> Alignment.Vertical >> MaxLength;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Look for parent
		_Element *ParentElement = nullptr;
		if(ParentIdentifier != "") {
			ParentElement = Elements[ParentIdentifier];
			if(!ParentElement)
				throw std::runtime_error("Parent element not found: " + ParentIdentifier);
		}

		// Get style
		_Style *Style = Styles[StyleIdentifier];

		// Get font
		_Font *Font = Fonts[FontIdentifier];
		if(!Font)
			throw std::runtime_error("Unable to find font: " + FontIdentifier);

		// Create
		_TextBox *Element = new _TextBox(Identifier, ParentElement, Offset, Size, Alignment, Style, Font, MaxLength);

		// Check for duplicates
		if(Elements[Identifier])
			throw std::runtime_error("Duplicate element identifier: " + Identifier);

		// Add as child for parent
		if(ParentElement)
			ParentElement->AddChild(Element);

		Elements[Identifier] = Element;
	}

	File.close();
}

// Frees memory and textures used by a reel
void _Assets::UnloadReel(const std::string &Identifier) {

	auto ReelIterator = Reels.find(Identifier);
	if(ReelIterator != Reels.end()) {
		for(int i = 0; i < static_cast<int>(ReelIterator->second.Textures.size()); i++)
			delete ReelIterator->second.Textures[i];

		Reels.erase(ReelIterator);
	}
}

// Frees memory and textures used by an animation
void _Assets::UnloadAnimation(const std::string &Identifier) {

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

// Returns the valid amount of experience
int64_t _Assets::GetValidExperience(int64_t Experience) {

	if(Experience < 0)
		return 0;
	else if(Experience > Levels[GAME_MAX_LEVEL-1].Experience)
		return Levels[GAME_MAX_LEVEL-1].Experience;

	return Experience;
}

// Returns the level given the experience number
int _Assets::GetLevel(int64_t Experience) {

	// Degenerate case
	if(Experience <= 0)
		return 1;
	else if(Experience >= Levels[GAME_MAX_LEVEL-1].Experience)
		return GAME_MAX_LEVEL;

	// Perform linear search through array
	for(int i = 1; i < GAME_MAX_LEVEL; i++) {
		if(Experience < Levels[i].Experience)
			return i;
	}

	return 1;
}

// Returns the total experience required for a level
int64_t _Assets::GetExperienceForLevel(int Level) {

	// Degenerate case
	if(Level <= 0)
		return Levels[0].Experience;
	else if(Level > GAME_MAX_LEVEL)
		return 0;

	return  Levels[Level-1].Experience;
}

// Returns a skill value in a valid range
int _Assets::GetValidSkill(int Level) {
	if(Level < 0)
		return 0;
	else if(Level >= GAME_SKILLLEVELS)
		return GAME_SKILLLEVELS;

	return Level;
}

// Creates a monster
_Monster *_Assets::CreateMonster(const std::string &Identifier, const glm::vec2 &Position) {
	_MonsterTemplate *MonsterTemplate;
	AttackSampleTemplateStruct *AttackSample;

	MonsterTemplate = GetMonsterTemplate(Identifier);
	AttackSample = GetAttackSampleTemplate(MonsterTemplate->SamplesIdentifier);

	// Creates a monster
	_Monster *Monster = new _Monster(MonsterTemplate, GetAnimation(MonsterTemplate->AnimationIdentifier), Position);
	for(int i = 0; i < SAMPLE_TYPES; i++)
		Monster->Samples[i] = AttackSample->Samples[i];

	return Monster;
}

// Creates a misc item
_MiscItem *_Assets::CreateMiscItem(const std::string &Identifier, int Count, const glm::vec2 &Position) {
	_MiscItemTemplate *MiscItemTemplate = GetMiscItemTemplate(Identifier);

	return new _MiscItem(Identifier, Count, Position, MiscItemTemplate, Textures[MiscItemTemplate->IconIdentifier]);
}

// Creates ammo
_Ammo *_Assets::CreateAmmoItem(const std::string &Identifier, int Count, const glm::vec2 &Position) {
	_AmmoTemplate *AmmoTemplate = GetAmmoTemplate(Identifier);

	return new _Ammo(Identifier, Count, Position, AmmoTemplate, Textures[AmmoTemplate->IconIdentifier]);
}

// Creates ammo from an ammo type
_Ammo *_Assets::CreateAmmoItem(int Type) {
	return CreateAmmoItem(AmmoTypeIdentifiers[Type], 1, glm::vec2(0, 0));
}

// Creates a weapon
_Weapon *_Assets::CreateWeapon(const std::string &Identifier, int Count, const glm::vec2 &Position, bool Generate) {
	_WeaponTemplate *WeaponTemplate = GetWeaponTemplate(Identifier);
	_Weapon *Weapon = new _Weapon(Identifier, Count, Position, WeaponTemplate, Textures[WeaponTemplate->IconIdentifier], Generate);

	return Weapon;
}

// Creates an upgrade item
_Upgrade *_Assets::CreateUpgradeItem(const std::string &Identifier, int Count, const glm::vec2 &Position) {
	_UpgradeTemplate *UpgradeTemplate = GetUpgradeTemplate(Identifier);

	return new _Upgrade(Identifier, Count, Position, UpgradeTemplate, Textures[UpgradeTemplate->IconIdentifier]);
}

// Creates armor
_Armor *_Assets::CreateArmor(const std::string &Identifier, int Count, const glm::vec2 &Position) {
	_ArmorTemplate *ArmorTemplate = GetArmorTemplate(Identifier);

	return new _Armor(Identifier, Count, Position, ArmorTemplate, Textures[ArmorTemplate->IconIdentifier]);
}

// Returns a random item identifier from an item group
void _Assets::GetRandomDrop(const _ItemGroup *ItemGroup, _ObjectSpawn *ObjectSpawn) {
	ObjectSpawn->Type = -1;

	// Get item group
	size_t ItemGroupSize = ItemGroup->Entries.size();
	if(ItemGroupSize == 0)
		return;

	// Get total
	if(ItemGroup->Total <= 0.0f)
		return;

	// Generate roll
	float RandomNumber = Random.GenerateRange(0.0f, ItemGroup->Total);

	// Get item
	for(size_t i = 0; i < ItemGroupSize; i++) {
		if(RandomNumber <= ItemGroup->Entries[i].Count) {
			ObjectSpawn->Type = ItemGroup->Entries[i].Type;
			ObjectSpawn->Identifier = ItemGroup->Entries[i].ItemIdentifier;
			return;
		}
	}
}

bool _Assets::IsColorLoaded(const std::string &Identifier) { return Colors.find(Identifier) != Colors.end(); }
bool _Assets::IsTextureLoaded(const std::string &Identifier) { return Textures.find(Identifier) != Textures.end(); }
bool _Assets::IsAttackSampleLoaded(const std::string &Identifier) { return AttackSampleTable.find(Identifier) != AttackSampleTable.end(); }
bool _Assets::IsParticleLoaded(const std::string &Identifier) { return ParticleTable.find(Identifier) != ParticleTable.end(); }
bool _Assets::IsWeaponParticleTemplateLoaded(const std::string &Identifier) { return WeaponParticleTable.find(Identifier) != WeaponParticleTable.end(); }
bool _Assets::IsReelLoaded(const std::string &Identifier) { return ReelTable.find(Identifier) != ReelTable.end(); }
bool _Assets::IsAnimationLoaded(const std::string &Identifier) { return AnimationTable.find(Identifier) != AnimationTable.end(); }
bool _Assets::IsMonsterLoaded(const std::string &Identifier) { return MonsterTable.find(Identifier) != MonsterTable.end(); }
bool _Assets::IsMiscItemLoaded(const std::string &Identifier) { return MiscItemTable.find(Identifier) != MiscItemTable.end(); }
bool _Assets::IsUpgradeLoaded(const std::string &Identifier) { return UpgradeTable.find(Identifier) != UpgradeTable.end(); }
bool _Assets::IsAmmoLoaded(const std::string &Identifier) { return AmmoTable.find(Identifier) != AmmoTable.end(); }
bool _Assets::IsWeaponLoaded(const std::string &Identifier) { return WeaponTable.find(Identifier) != WeaponTable.end(); }
bool _Assets::IsArmorLoaded(const std::string &Identifier) { return ArmorTable.find(Identifier) != ArmorTable.end(); }
bool _Assets::IsItemGroupLoaded(const std::string &Identifier) { return ItemGroupTable.find(Identifier) != ItemGroupTable.end(); }

_Reel *_Assets::GetReel(const std::string &Identifier) {
	if(Reels.find(Identifier) == Reels.end())
		return nullptr;

	return &Reels[Identifier];
}
AttackSampleTemplateStruct *_Assets::GetAttackSampleTemplate(const std::string &Identifier) {
	if(AttackSampleTable.find(Identifier) == AttackSampleTable.end())
		return nullptr;

	return &AttackSampleTable[Identifier];
}
_Animation *_Assets::GetAnimation(const std::string &Identifier) {
	if(Animations.find(Identifier) == Animations.end())
		return nullptr;

	return Animations[Identifier];
}
_ParticleTemplate *_Assets::GetParticleTemplate(const std::string &Identifier) {
	if(ParticleTable.find(Identifier) == ParticleTable.end())
		return nullptr;

	return &ParticleTable[Identifier];
}
_WeaponParticleTemplate *_Assets::GetWeaponParticleTemplate(const std::string &Identifier) {
	if(WeaponParticleTable.find(Identifier) == WeaponParticleTable.end())
		return nullptr;

	return &WeaponParticleTable[Identifier];
}
_MonsterTemplate *_Assets::GetMonsterTemplate(const std::string &Identifier) {
	if(MonsterTable.find(Identifier) == MonsterTable.end())
		return nullptr;

	return &MonsterTable[Identifier];
}
_MiscItemTemplate *_Assets::GetMiscItemTemplate(const std::string &Identifier) {
	if(MiscItemTable.find(Identifier) == MiscItemTable.end())
		return nullptr;

	return &MiscItemTable[Identifier];
}
_UpgradeTemplate *_Assets::GetUpgradeTemplate(const std::string &Identifier) {
	if(UpgradeTable.find(Identifier) == UpgradeTable.end())
		return nullptr;

	return &UpgradeTable[Identifier];
}
_AmmoTemplate *_Assets::GetAmmoTemplate(const std::string &Identifier) {
	if(AmmoTable.find(Identifier) == AmmoTable.end())
		return nullptr;

	return &AmmoTable[Identifier];
}
_WeaponTemplate *_Assets::GetWeaponTemplate(const std::string &Identifier) {
	if(WeaponTable.find(Identifier) == WeaponTable.end())
		return nullptr;

	return &WeaponTable[Identifier];
}
_ArmorTemplate *_Assets::GetArmorTemplate(const std::string &Identifier) {
	if(ArmorTable.find(Identifier) == ArmorTable.end())
		return nullptr;

	return &ArmorTable[Identifier];
}
_ItemGroup *_Assets::GetItemGroup(const std::string &Identifier) {
	if(ItemGroupTable.find(Identifier) == ItemGroupTable.end())
		return nullptr;

	return &ItemGroupTable[Identifier];
}

_Label *_Assets::GetLabel(const std::string &Identifier) { return (_Label *)Elements[Identifier]; }
_Image *_Assets::GetImage(const std::string &Identifier) { return (_Image *)Elements[Identifier]; }
_Button *_Assets::GetButton(const std::string &Identifier) { return (_Button *)Elements[Identifier]; }
_TextBox *_Assets::GetTextBox(const std::string &Identifier) { return (_TextBox *)Elements[Identifier]; }
