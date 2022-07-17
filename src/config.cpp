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
#include <config.h>
#include <ae/actions.h>
#include <ae/util.h>
#include <actiontype.h>
#include <constants.h>
#include <sstream>
#include <fstream>
#include <SDL_filesystem.h>

// Globals
_Config Config;

// Initializes the config system
void _Config::Init(const std::string &ConfigFile) {

	// Set names for actions
	ae::Actions.State.resize(Action::COUNT);
	ae::Actions.ResetState();
	ae::Actions.State[Action::GAME_LEFT].Name = "game_left";
	ae::Actions.State[Action::GAME_RIGHT].Name = "game_right";
	ae::Actions.State[Action::GAME_UP].Name = "game_up";
	ae::Actions.State[Action::GAME_DOWN].Name = "game_down";
	ae::Actions.State[Action::GAME_USE].Name = "game_use";
	ae::Actions.State[Action::GAME_SPRINT].Name = "game_sprint";
	ae::Actions.State[Action::GAME_MAP].Name = "game_map";
	ae::Actions.State[Action::GAME_FLASHLIGHT].Name = "game_flashlight";
	ae::Actions.State[Action::GAME_FIRE].Name = "game_fire";
	ae::Actions.State[Action::GAME_AIM].Name = "game_aim";
	ae::Actions.State[Action::GAME_MELEE].Name = "game_melee";
	ae::Actions.State[Action::GAME_RELOAD].Name = "game_reload";
	ae::Actions.State[Action::GAME_WEAPONSWITCH].Name = "game_weaponswitch";
	ae::Actions.State[Action::GAME_INVENTORY].Name = "game_inventory";
	ae::Actions.State[Action::MISC_CONSOLE].Name = "misc_console";
	ae::Actions.State[Action::MISC_DEBUG].Name = "misc_debug";

	// Create config path
	char *PrefPath = SDL_GetPrefPath("", "emptyclip");
	if(PrefPath) {
		ConfigPath = PrefPath;
		SDL_free(PrefPath);
	}
	else {
		throw std::runtime_error("Cannot create config path!");
	}

	ConfigFilePath = ConfigPath + ConfigFile;
	ae::MakeDirectory(ConfigPath);

	// Load defaults
	SetDefaults();

	// Load config
	Load();
}

// Closes the config system
void _Config::Close() {
}

// Set defaults
void _Config::SetDefaults() {

	Version = CONFIG_VERSION;
	WindowSize = DEFAULT_WINDOW_SIZE;
	MSAA = 0;
	Anisotrophy = 0;
	Fullscreen = DEFAULT_FULLSCREEN;
	Vsync = DEFAULT_VSYNC;
	MaxFPS = DEFAULT_MAXFPS;
	AudioEnabled = DEFAULT_AUDIOENABLED;

	WeaponFlashes = true;
	Tutorial = true;

	SoundVolume = 1.0f;
	MusicVolume = 1.0f;

	LoadDefaultInputBindings(false);
}

// Load default key bindings
void _Config::LoadDefaultInputBindings(bool IfNone) {

	// Clear mappings
	if(!IfNone) {
		for(int i = 0; i < ae::_Input::INPUT_COUNT; i++)
			ae::Actions.ClearMappings(i);
	}

	// Game
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_E, Action::GAME_UP, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_D, Action::GAME_DOWN, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_S, Action::GAME_LEFT, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_F, Action::GAME_RIGHT, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_A, Action::GAME_SPRINT, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_SPACE, Action::GAME_USE, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_C, Action::GAME_INVENTORY, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::MOUSE_BUTTON, 1, Action::GAME_FIRE, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::MOUSE_BUTTON, 3, Action::GAME_AIM, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::MOUSE_BUTTON, 4, Action::GAME_MELEE, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_R, Action::GAME_RELOAD, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_W, Action::GAME_WEAPONSWITCH, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_TAB, Action::GAME_MAP, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_V, Action::GAME_FLASHLIGHT, 1.0f, -1.0f, IfNone);

	// Misc
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_GRAVE, Action::MISC_CONSOLE, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_F2, Action::MISC_DEBUG, 1.0f, -1.0f, IfNone);
}

// Load the config file
void _Config::Load() {

	// Open file
	std::ifstream File(ConfigFilePath.c_str());
	if(!File) {
		Save();
		return;
	}

	// Read data into map
	Map.clear();
	char Buffer[256];
	while(File) {
		File.getline(Buffer, 256);
		if(File.good()) {
			std::string Line(Buffer);
			size_t Pos = Line.find_first_of('=');
			if(Pos != std::string::npos) {
				std::string Field = Line.substr(0, Pos);
				std::string Value = Line.substr(Pos+1, Line.size());

				Map[Field].push_back(Value);
			}
		}
	}

	// Close
	File.close();

	// Read version
	int ReadVersion = 0;
	GetValue("version", ReadVersion);
	if(ReadVersion != Version) {
		std::rename(ConfigFilePath.c_str(), (ConfigFilePath + "." + std::to_string(ReadVersion)).c_str());
		Save();
		return;
	}

	// Read config
	GetValue("window_width", WindowSize.x);
	GetValue("window_height", WindowSize.y);
	GetValue("fullscreen", Fullscreen);
	GetValue("vsync", Vsync);
	GetValue("max_fps", MaxFPS);
	GetValue("anisotrophy", Anisotrophy);
	GetValue("msaa", MSAA);
	GetValue("audio_enabled", AudioEnabled);
	GetValue("weapon_flashes", WeaponFlashes);
	GetValue("tutorial", Tutorial);
	GetValue("sound_volume", SoundVolume);
	GetValue("music_volume", MusicVolume);

	// Clear bindings
	for(int i = 0; i < ae::_Input::INPUT_COUNT; i++)
		ae::Actions.ClearMappings(i);

	// Load bindings
	for(size_t i = 0; i < ae::Actions.State.size(); i++) {
		std::ostringstream Buffer;
		Buffer << "action_" << ae::Actions.State[i].Name;

		// Get list of inputs for each action
		const auto &Values = Map[Buffer.str()];
		for(auto &Iterator : Values) {

			// Parse input bind
			int Rank;
			int InputType;
			int Input;
			char Dummy;
			std::istringstream Stream(Iterator);
			Stream >> Rank >> Dummy >> InputType >> Dummy >> Input;
			ae::Actions.AddInputMap(Rank, InputType, Input, i, 1.0f, -1.0f, false);
		}
	}

	// Add missing bindings
	LoadDefaultInputBindings(true);
}

// Save variables to the config file
void _Config::Save() {

	// Open file
	std::ofstream File(ConfigFilePath.c_str());
	if(!File.is_open())
		return;

	// Write variables
	File << "version=" << Version << std::endl;
	File << "window_width=" << WindowSize.x << std::endl;
	File << "window_height=" << WindowSize.y << std::endl;
	File << "fullscreen=" << Fullscreen << std::endl;
	File << "vsync=" << Vsync << std::endl;
	File << "max_fps=" << MaxFPS << std::endl;
	File << "msaa=" << MSAA << std::endl;
	File << "aniso=" << Anisotrophy << std::endl;
	File << "audio_enabled=" << AudioEnabled << std::endl;
	File << "weapon_flashes=" << WeaponFlashes << std::endl;
	File << "tutorial=" << Tutorial << std::endl;
	File << "sound_volume=" << SoundVolume << std::endl;
	File << "music_volume=" << MusicVolume << std::endl;

	// Write out input map
	ae::Actions.Serialize(File, ae::_Input::KEYBOARD);
	ae::Actions.Serialize(File, ae::_Input::MOUSE_AXIS);
	ae::Actions.Serialize(File, ae::_Input::MOUSE_BUTTON);

	File.close();
}
