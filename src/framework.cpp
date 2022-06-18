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
#include <config.h>
#include <graphics.h>
#include <input.h>
#include <actions.h>
#include <audio.h>
#include <state.h>
#include <framelimit.h>
#include <ae/random.h>
#include <stdexcept>
#include <constants.h>
#include <assets.h>
#include <stats.h>
#include <save.h>
#include <states/null.h>
#include <states/convert.h>
#include <states/play.h>
#include <states/editor.h>
#include <SDL.h>

// Global instance
_Framework Framework;

// Initialize
void _Framework::Init(int ArgumentCount, char **Arguments) {
	RequestedState = nullptr;
	Done = false;
	TimeStepAccumulator = 0.0;
	TimeStep = GAME_TIMESTEP;
	FrameworkState = INIT;
	IgnoreNextInputEvent = false;
	State = &NullState;

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
			State = &EditorState;
			if(TokensRemaining && Arguments[i+1][0] != '-')
				EditorState.SetMapFilename(Arguments[++i]);
		}
		else if(Token == "-convert" && TokensRemaining > 0) {
			State = &ConvertState;
			ConvertState.SetParam1(Arguments[++i]);
		}
		else if(Token == "-level" && TokensRemaining > 0) {
			PlayState.SetLevel(Arguments[++i]);
			PlayState.SetTestMode(true);

			State = &PlayState;
		}
		else if(Token == "-noaudio") {
			AudioEnabled = false;
		}
	}

	// Initialize SDL
	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		throw std::runtime_error("Failed to initialize SDL");
	}

	// Get window settings
	_WindowSettings WindowSettings;
	WindowSettings.WindowTitle = GAME_WINDOWTITLE;
	WindowSettings.Fullscreen = Fullscreen;
	WindowSettings.Vsync = Config.Vsync;
	WindowSettings.Size = Config.WindowSize;
	WindowSettings.MSAA = Config.MSAA;
	WindowSettings.Position = glm::ivec2(SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

	// Set up subsystems
	Graphics.Init(WindowSettings);
	Graphics.SetCullFace(false);
	Audio.Init(AudioEnabled);
	Audio.SetGain(Config.SoundVolume);

	FrameLimit = new _FrameLimit(Config.MaxFPS);
	Timer = SDL_GetPerformanceCounter();
	ae::RandomGenerator.seed(SDL_GetPerformanceCounter());

	// Load assets
	LoadAssets();
	Stats.Init();
	Actions.LoadActionNames();
	Save.LoadSaves();
}

// Shutdown
void _Framework::Close() {

	// Close the current state
	if(State)
		State->Close();

	Assets.UnloadAnimation("player_torso");
	Assets.UnloadAnimation("player_legs");

	Stats.Close();
	Assets.Close();
	delete FrameLimit;

	Audio.Close();
	Graphics.Close();
	SDL_Quit();
}

// Update input
void _Framework::Update() {

	// Get frame time
	double FrameTime = (SDL_GetPerformanceCounter() - Timer) / (double)SDL_GetPerformanceFrequency();
	Timer = SDL_GetPerformanceCounter();

	// Get events from SDL
	SDL_PumpEvents();
	Input.Update(FrameTime);

	// Loop through events
	SDL_Event Event;
	while(SDL_PollEvent(&Event)) {
		if(!State || FrameworkState != UPDATE)
			continue;

		switch(Event.type){
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				if(!GlobalKeyHandler(Event)) {
					_KeyEvent KeyEvent("", Event.key.keysym.scancode, Event.type == SDL_KEYDOWN, Event.key.repeat);

					// Handle console input
					bool SendAction = true;
					SendAction = State->HandleKey(KeyEvent);

					// Pass keys to action handler
					if(!Event.key.repeat && SendAction)
						Actions.InputEvent(_Input::KEYBOARD, Event.key.keysym.scancode, Event.type == SDL_KEYDOWN);
				}
			break;
			case SDL_TEXTINPUT:
				if(!IgnoreNextInputEvent) {
					_KeyEvent KeyEvent(Event.text.text, 0, 1, 1);
					State->HandleKey(KeyEvent);
				}

				IgnoreNextInputEvent = false;
			break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				if(State && FrameworkState == UPDATE) {
					_MouseEvent MouseEvent(glm::ivec2(Event.motion.x, Event.motion.y), Event.button.button, Event.type == SDL_MOUSEBUTTONDOWN);
					State->HandleMouseButton(MouseEvent);
					Actions.InputEvent(_Input::MOUSE_BUTTON, Event.button.button, Event.type == SDL_MOUSEBUTTONDOWN);
				}
			break;
			case SDL_MOUSEWHEEL:
				if(State)
					State->HandleMouseWheel(Event.wheel.y);
			break;
			case SDL_QUIT:
				Done = true;
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
			TimeStepAccumulator += FrameTime;
			while(TimeStepAccumulator >= TimeStep) {
				State->Update(TimeStep);
				TimeStepAccumulator -= TimeStep;
			}
			State->Render(TimeStepAccumulator / TimeStep);
			//printf("%f\n", TimeStepAccumulator);
		} break;
		case CLOSE: {
			if(State)
				State->Close();

			State = RequestedState;
			FrameworkState = INIT;
		} break;
	}

	Audio.Update(FrameTime);
	Graphics.Flip(FrameTime);
	if(FrameLimit && !Config.Vsync)
		FrameLimit->Update();
}

// Handles global hotkeys
int _Framework::GlobalKeyHandler(const SDL_Event &Event) {

	if(Event.type == SDL_KEYDOWN) {

		// Handle alt-enter
		if((Event.key.keysym.mod & KMOD_ALT) && (Event.key.keysym.scancode == SDL_SCANCODE_RETURN || Event.key.keysym.scancode == SDL_SCANCODE_KP_ENTER)) {
			if(!Event.key.repeat) {
				//Config.Fullscreen = !Config.Fullscreen;
				//Graphics.SetFullscreen(Config.Fullscreen);

				//Menu.SetFullscreen(!Config.Fullscreen);
				//if(Console)
				//	Console->UpdateSize();
			}

			return 1;
		}
	}

	return 0;
}

// Load game assets
void _Framework::LoadAssets() {

	Assets.LoadPrograms("tables/programs.tsv");
	Assets.LoadStrings("tables/strings.tsv");
	Assets.LoadFonts("tables/fonts.tsv", false);
	Assets.LoadTextures("tables/textures/main.tsv");
	Assets.LoadTextureDirectory(MAP_TEXTURE_PATH, false, true, true, false);
	Assets.LoadColors("tables/colors.tsv");
	Assets.LoadSounds("tables/sounds.tsv", "sounds/");
	Assets.LoadSoundGroups("tables/sound_groups.tsv");
	Assets.LoadParticles("tables/particles.tsv");
	Assets.LoadWeaponParticles("tables/weaponparticles.tsv");
	Assets.LoadReelTable("tables/reels.tsv");
	Assets.LoadAnimationTable("tables/animation.tsv");

	Assets.LoadAnimation("player_torso", "textures/player/");
	Assets.LoadAnimation("player_legs", "textures/player/");

	Assets.LoadStyles("tables/ui/styles.tsv");
	Assets.LoadUI("tables/ui.xml");
	Assets.LoadElements("tables/ui/elements.tsv");
	Assets.LoadImages("tables/ui/images.tsv");
	Assets.LoadButtons("tables/ui/buttons.tsv");
	Assets.LoadTextBoxes("tables/ui/textboxes.tsv");
	Assets.LoadLabels("tables/ui/labels.tsv");
	Graphics.Element->CalculateBounds(false);

	//Assets.SaveUI("tables/ui_new.xml");
	Assets.LoadFonts("tables/fonts.tsv");
}

// Change states
void _Framework::ChangeState(_State *RequestedState) {
	this->RequestedState = RequestedState;
	FrameworkState = CLOSE;
}
