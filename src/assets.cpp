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
#include <ui/button.h>
#include <ui/image.h>
#include <ui/textbox.h>
#include <objects/monster.h>
#include <objects/particle.h>
#include <objects/player.h>
#include <objects/weapon.h>
#include <constants.h>
#include <stdexcept>
#include <sstream>

_Assets Assets;

// Initialize
void _Assets::Init() {

	LoadPrograms("tables/programs.tsv");
	LoadStrings("tables/strings.tsv");
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

	LoadAnimation("player_torso", "textures/player/");
	LoadAnimation("player_legs", "textures/player/");
	LoadFonts("tables/fonts.tsv");

	BlankWeaponParticle = _WeaponParticleTemplate();
}

// Shutdown
void _Assets::Close() {

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
			throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " - Duplicate entry: " + ID);

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
			throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " - Duplicate entry: " + Identifier);

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
			throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " - Duplicate entry: " + Identifier);

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
			throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " - Error loading: " + Path);

		// Check for duplicates
		if(IsTextureLoaded(Name))
			throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " - Duplicate entry: " + Name);

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
			throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " - Duplicate entry: " + Identifier);

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
			throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " - Duplicate entry: " + Identifier);

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
void _Assets::LoadReel(const std::string &Identifier, const std::string &Path) {

	auto ReelTableIterator = ReelTable.find(Identifier);
	if(ReelTableIterator == ReelTable.end())
		return;

	auto ReelIterator = Reels.find(Identifier);
	if(ReelIterator == Reels.end()) {
		_Reel Reel;
		Reel.StartPosition = ReelTableIterator->second.StartPosition;
		Reel.RepeatMode = (RepeatType)(ReelTableIterator->second.RepeatMode);
		Reel.PlaybackSpeed = ReelTableIterator->second.PlaybackSpeed;

		for(std::size_t i = 0; i < ReelTableIterator->second.TextureFiles.size(); i++) {
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
		for(std::size_t i = 0; i < AnimationTableIterator->second.Identifiers.size(); i++) {
			LoadReel(AnimationTableIterator->second.Identifiers[i], Path);

			Animation->Reels.push_back(GetReel(AnimationTableIterator->second.Identifiers[i]));
		}

		Animation->ChangeReel(0);
		Animations[Identifier] = Animation;
	}
}

// Loads all the monster animations in a monster set
void _Assets::LoadMonsterAnimation() {
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
		_Element *Element = new _Element(Name, ParentElement, Offset, Size, Alignment, nullptr, false);
		Element->Color = Color;
		Element->Font = Font;
		Element->Text = Text;

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
		for(size_t i = 0; i < ReelIterator->second.Textures.size(); i++)
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

bool _Assets::IsColorLoaded(const std::string &Identifier) { return Colors.find(Identifier) != Colors.end(); }
bool _Assets::IsTextureLoaded(const std::string &Identifier) { return Textures.find(Identifier) != Textures.end(); }
bool _Assets::IsAttackSampleLoaded(const std::string &Identifier) { return AttackSampleTable.find(Identifier) != AttackSampleTable.end(); }
bool _Assets::IsParticleLoaded(const std::string &Identifier) { return ParticleTable.find(Identifier) != ParticleTable.end(); }
bool _Assets::IsWeaponParticleTemplateLoaded(const std::string &Identifier) { return WeaponParticleTable.find(Identifier) != WeaponParticleTable.end(); }
bool _Assets::IsReelLoaded(const std::string &Identifier) { return ReelTable.find(Identifier) != ReelTable.end(); }
bool _Assets::IsAnimationLoaded(const std::string &Identifier) { return AnimationTable.find(Identifier) != AnimationTable.end(); }

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

_Element *_Assets::GetLabel(const std::string &Identifier) { return Elements[Identifier]; }
_Image *_Assets::GetImage(const std::string &Identifier) { return (_Image *)Elements[Identifier]; }
_Button *_Assets::GetButton(const std::string &Identifier) { return (_Button *)Elements[Identifier]; }
_TextBox *_Assets::GetTextBox(const std::string &Identifier) { return (_TextBox *)Elements[Identifier]; }
