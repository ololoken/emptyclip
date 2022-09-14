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
#include <framework.h>
#include <states/null.h>
#include <states/convert.h>
#include <states/play.h>
#include <states/editor.h>
#include <ae/framelimit.h>
#include <ae/random.h>
#include <ae/state.h>
#include <ae/input.h>
#include <ae/actions.h>
#include <ae/graphics.h>
#include <ae/assets.h>
#include <ae/console.h>
#include <ae/ui.h>
#include <ae/util.h>
#include <ae/audio.h>
#include <ae/texture.h>
#include <gameassets.h>
#include <achievements.h>
#include <menu.h>
#include <config.h>
#include <stdexcept>
#include <constants.h>
#include <stats.h>
#include <SDL.h>
#include <algorithm>

// Global instance
_Framework Framework;

// Initialize
void _Framework::Init(int ArgumentCount, char **Arguments) {
	TimeStep = GAME_TIMESTEP;
	State = &NullState;

	#if defined ENABLE_ACHIEVEMENTS && ENABLE_ACHIEVEMENTS == 0
		Achievements.Enabled = false;
	#endif
	bool AudioEnabled = Config.AudioEnabled;
	bool Fullscreen = Config.Fullscreen;

	// Process arguments
	std::string Token;
	int TokensRemaining;
	for(int i = 1; i < ArgumentCount; i++) {
		Token = std::string(Arguments[i]);
		TokensRemaining = ArgumentCount - i - 1;

		if(Token == "-fullscreen") {
			Fullscreen = true;
		}
		else if(Token == "-window") {
			Fullscreen = false;
		}
		else if(Token == "-editor") {
			#ifdef ENABLE_EDITOR
				State = &EditorState;
				if(TokensRemaining && Arguments[i+1][0] != '-')
					EditorState.SetMapFilename(Arguments[++i]);
			#endif
		}
		else if(Token == "-convert" && TokensRemaining > 0) {
			State = &ConvertState;
			ConvertState.SetParam1(Arguments[++i]);
		}
		else if(Token == "-level" && TokensRemaining > 0) {
			PlayState.Level = Arguments[++i];
			PlayState.TestMode = true;

			State = &PlayState;
		}
		else if(Token == "-noaudio") {
			AudioEnabled = false;
		}
		else if(Token == "-dev") {
			#ifndef NDEBUG
				PlayState.DevMode = true;
			#endif
		}
	}

	// Set random seed
	ae::RandomGenerator.seed(SDL_GetPerformanceCounter());

	// Create frame limiter
	FrameLimit = new ae::_FrameLimit(Config.MaxFPS);

	// Initialize SDL
	if(SDL_Init(SDL_INIT_VIDEO) < 0)
		throw std::runtime_error(std::string(__func__) + " failed to initialize SDL");

	// Initialize audio
	ae::Audio.Init(AudioEnabled, false);
	ae::Audio.SetMaxDistance(AUDIO_MAX_DISTANCE);
	ae::Audio.SetDirection(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	ae::Audio.SetSoundVolume(Config.SoundVolume);
	ae::Audio.SetMusicVolume(Config.MusicVolume);

	// Get window settings
	ae::_WindowSettings WindowSettings;
	WindowSettings.WindowTitle = GAME_WINDOWTITLE;
	WindowSettings.Fullscreen = Fullscreen;
	WindowSettings.Vsync = Config.Vsync;
	WindowSettings.Size = Config.WindowSize;
	WindowSettings.MSAA = Config.MSAA;
	WindowSettings.Anisotrophy = Config.Anisotrophy;
	WindowSettings.Position = glm::ivec2(SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

	// Set up subsystems
	ae::Graphics.CircleVertices = 64;
	ae::Graphics.Init(WindowSettings);
	ae::Graphics.SetCullFace(false);
	LoadAssets();
	Stats.Init();
	Menu.Init();
	try {
		Achievements.Load();
	}
	catch(std::exception &Error) {
		Achievements.Save();
	}

	// Setup console
	Console = new ae::_Console(ae::Assets.Programs["ortho_pos"], ae::Assets.Fonts["console"]);
	Console->LoadHistory(Config.ConfigPath + "history.txt");
	Console->CommandList.push_back("maxfps");
	Console->CommandList.push_back("suicide");
	Console->CommandList.push_back("quit");
	Console->CommandList.push_back("volume");
	Console->CommandList.push_back("vsync");

	// Add dev mode commands
	if(PlayState.DevMode) {
		Console->CommandList.push_back("ammo");
		Console->CommandList.push_back("clock");
		Console->CommandList.push_back("experience");
		Console->CommandList.push_back("god");
		Console->CommandList.push_back("health");
		Console->CommandList.push_back("keys");
		Console->CommandList.push_back("progression");
		Console->CommandList.push_back("reset");
	}

	// Sort commands
	std::sort(Console->CommandList.begin(), Console->CommandList.end());

	Timer = SDL_GetPerformanceCounter();
}

// Shutdown
void _Framework::Close() {

	// Close the current state
	if(State)
		State->Close();

	Achievements.Save();
	Stats.Close();
	ae::Assets.Close();
	GameAssets.Close();
	delete Console;
	delete FrameLimit;

	ae::Audio.Close();
	ae::Graphics.Close();
	SDL_Quit();
}

// Change states
void _Framework::ChangeState(ae::_State *RequestedState) {
	this->RequestedState = RequestedState;
	FrameworkState = CLOSE;
}

// Update input
void _Framework::Update() {

	// Get frame time
	double FrameTime = (SDL_GetPerformanceCounter() - Timer) / (double)SDL_GetPerformanceFrequency();
	Timer = SDL_GetPerformanceCounter();

	// Get events from SDL
	SDL_PumpEvents();
	ae::Input.Update(FrameTime);

	// Loop through events
	SDL_Event Event;
	while(SDL_PollEvent(&Event)) {
		if(!State || FrameworkState != UPDATE)
			continue;

		switch(Event.type){
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				if(!GlobalKeyHandler(Event)) {
					ae::_KeyEvent KeyEvent("", Event.key.keysym.scancode, Event.type == SDL_KEYDOWN, Event.key.repeat);

					// Handle console input
					bool SendAction = true;
					if(Console->IsOpen())
						ae::Graphics.Element->HandleKey(KeyEvent);
					else
						SendAction = State->HandleKey(KeyEvent);

					// Pass keys to action handler
					if(!Event.key.repeat && SendAction)
						ae::Actions.InputEvent(State, ae::_Input::KEYBOARD, Event.key.keysym.scancode, Event.type == SDL_KEYDOWN);
				}
			break;
			case SDL_TEXTINPUT:
				if(!IgnoreNextInputEvent) {
					ae::_KeyEvent KeyEvent(Event.text.text, 0, 1, 1);
					if(Console->IsOpen())
						ae::Graphics.Element->HandleKey(KeyEvent);
					else
						State->HandleKey(KeyEvent);
				}

				IgnoreNextInputEvent = false;
			break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				if(!Console->IsOpen()) {
					ae::_MouseEvent MouseEvent(glm::ivec2(Event.motion.x, Event.motion.y), Event.button.button, Event.type == SDL_MOUSEBUTTONDOWN);
					State->HandleMouseButton(MouseEvent);
					ae::Actions.InputEvent(State, ae::_Input::MOUSE_BUTTON, Event.button.button, Event.type == SDL_MOUSEBUTTONDOWN);
				}
			break;
			case SDL_MOUSEWHEEL:
				if(!Console->IsOpen())
					State->HandleMouseWheel(Event.wheel.y);
			break;
			case SDL_WINDOWEVENT:
				if(Event.window.event)
					State->HandleWindow(Event.window.event);
			break;
			case SDL_QUIT:
				State->HandleQuit();
			break;
		}
	}

	switch(FrameworkState) {
		case INIT: {
			if(State) {
				State->Init();
				FrameworkState = UPDATE;
			}
			else
				Done = true;
		} break;
		case UPDATE: {
			TimeStepAccumulator += std::min(FrameTime, GAME_MAX_FRAMETIME);
			while(TimeStepAccumulator >= TimeStep) {
				State->Update(TimeStep);
				if(Console) {
					Console->Update(TimeStep);
					if(!Console->Command.empty()) {
						bool Handled = State->HandleCommand(Console);
						if(!Handled)
							HandleCommand(Console);
						Console->Command = "";
					}
				}

				TimeStepAccumulator -= TimeStep;
			}

			double BlendFactor = TimeStepAccumulator / TimeStep;
			State->Render(BlendFactor);
			if(Console)
				Console->Render(BlendFactor);
		} break;
		case CLOSE: {
			if(State)
				State->Close();

			State = RequestedState;
			FrameworkState = INIT;
		} break;
	}

	ae::Audio.Update(FrameTime);
	ae::Graphics.Flip(FrameTime);
	if(FrameLimit && !Config.Vsync)
		FrameLimit->Update();
}

// Handles global hotkeys
int _Framework::GlobalKeyHandler(const SDL_Event &Event) {

	if(Event.type == SDL_KEYDOWN) {

		// Handle alt-enter
		if((Event.key.keysym.mod & KMOD_ALT) && (Event.key.keysym.scancode == SDL_SCANCODE_RETURN || Event.key.keysym.scancode == SDL_SCANCODE_KP_ENTER)) {
			if(!Event.key.repeat) {
				Config.Fullscreen = !Config.Fullscreen;
				Config.Save();
				Menu.SetFullscreen(Config.Fullscreen);
				if(Console)
					Console->UpdateSize();
			}

			return 1;
		}
	}

	return 0;
}

// Handle generic console command
void _Framework::HandleCommand(ae::_Console *Console) {

	// Get parameters
	std::vector<std::string> Parameters;
	ae::TokenizeString(Console->Parameters, Parameters);

	if(Console->Command == "maxfps") {
		if(Parameters.size() == 1) {
			Config.MaxFPS = ae::ToNumber<int>(Parameters[0]);
			Framework.FrameLimit->SetFrameRate(Config.MaxFPS);
			Config.Save();
		}
		else {
			Console->AddMessage("maxfps = " + std::to_string(Config.MaxFPS));
			Console->AddMessage("usage: maxfps [value]");
		}
	}
	else if(Console->Command == "volume") {
		if(Parameters.size() == 1) {
			Config.SoundVolume = Config.MusicVolume = std::clamp(ae::ToNumber<float>(Console->Parameters), 0.0f, 1.0f);
			ae::Audio.SetSoundVolume(Config.SoundVolume);
			Config.Save();
		}
		else
			Console->AddMessage("usage: volume [value]");
	}
	else if(Console->Command == "vsync") {
		if(Parameters.size() == 1) {
			Config.Vsync = ae::ToNumber<bool>(Console->Parameters);
			ae::Graphics.SetVsync(Config.Vsync);
			Config.Save();
		}
		else {
			Console->AddMessage("vsync = " + std::to_string(ae::Graphics.GetVsync()));
			Console->AddMessage("usage: vsync [value]");
		}
	}
	else {
		Console->AddMessage("Command \"" + Console->Command + "\" not found");
	}
}

// Load game assets
void _Framework::LoadAssets() {

	// Load textures
	ae::_TextureSettings TextureSettings;
	TextureSettings.WrapMode = ae::_Texture::REPEAT;
	TextureSettings.Mipmaps = false;
	ae::Assets.LoadTexturePack("textures/editor_repeat", TextureSettings);
	ae::Assets.LoadTexturePack("textures/hud_repeat", TextureSettings);

	TextureSettings.WrapMode = ae::_Texture::REPEAT;
	TextureSettings.Mipmaps = true;
	ae::Assets.LoadTexturePack("textures/map", TextureSettings);
	ae::Assets.LoadTexturePack("textures/props", TextureSettings);

	TextureSettings.WrapMode = ae::_Texture::CLAMP_TO_EDGE;
	TextureSettings.Mipmaps = false;
	ae::Assets.LoadTexturePack("textures/editor", TextureSettings);
	ae::Assets.LoadTexturePack("textures/hud", TextureSettings);
	ae::Assets.LoadTexturePack("textures/menu", TextureSettings);
	ae::Assets.LoadTexturePack("textures/particles", TextureSettings);
	ae::Assets.LoadTexturePack("textures/projectiles", TextureSettings);

	TextureSettings.WrapMode = ae::_Texture::CLAMP_TO_EDGE;
	TextureSettings.Mipmaps = true;
	ae::Assets.LoadTexturePack("textures/animations", TextureSettings);
	ae::Assets.LoadTexturePack("textures/items", TextureSettings);
	ae::Assets.LoadTexturePack("textures/icons", TextureSettings);
	ae::Assets.LoadReels("tables/reels.tsv");

	TextureSettings.WrapMode = ae::_Texture::CLAMP_TO_BORDER;
	TextureSettings.Mipmaps = true;
	ae::Assets.LoadTexturePack("textures/lights", TextureSettings);
	ae::Assets.LoadTexturePack("textures/melee", TextureSettings);

	// Load assets
	ae::Assets.LoadPrograms("tables/programs.tsv");
	ae::Assets.LoadFonts("ui/fonts.tsv", false);
	ae::Assets.LoadMeshDirectory("meshes/");
	ae::Assets.LoadColors("tables/colors.tsv");
	ae::Assets.LoadSoundPack("data/sounds");
	GameAssets.LoadSounds("tables/sounds.tsv");
	GameAssets.LoadSoundGroups("tables/sound_groups.tsv");
	GameAssets.LoadParticles("tables/particles.tsv");
	GameAssets.LoadParticleGroups("tables/particle_groups.tsv");

	ae::Assets.LoadAnimations("tables/animations.tsv");
	ae::Assets.LoadStyles("ui/styles.tsv");
	ae::Assets.LoadUI("ui/elements.xml");
	//Assets.SaveUI("tables/elements_new.xml");

	ae::Assets.LoadFonts("ui/fonts.tsv");
}
