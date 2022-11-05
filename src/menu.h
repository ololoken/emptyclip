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
#include <constants.h>
#include <vector>
#include <string>
#include <list>

// Forward Declarations
namespace ae {
	class _Element;
	struct _MouseEvent;
	struct _KeyEvent;
}

struct _Message {
	std::string Name;
	double Time;
};

// Classes
class _Menu {

	public:

		// Menu states
		enum StateType {
			STATE_NONE,
			STATE_TITLE,
			STATE_SINGLEPLAYER,
			STATE_OPTIONS,
			STATE_CONTROLS,
			STATE_INGAME,
			STATE_SCORE,
			STATE_ACHIEVEMENTS,
		};

		enum OptionsStateType {
			OPTION_NONE,
			OPTION_ACCEPT_INPUT,
		};

		enum SinglePlayerStateType {
			SINGLEPLAYER_NONE,
			SINGLEPLAYER_NEW_PLAYER,
			SINGLEPLAYER_DELETE,
		};

		enum KeyLabelType {
			LABEL_UP,
			LABEL_DOWN,
			LABEL_LEFT,
			LABEL_RIGHT,
			LABEL_USE,
			LABEL_SPRINT,
			LABEL_MAP,
			LABEL_FLASHLIGHT,
			LABEL_FIRE,
			LABEL_AIM,
			LABEL_MELEE,
			LABEL_RELOAD,
			LABEL_SWITCH,
			LABEL_INVENTORY,
			LABEL_SORTINVENTORY,
			LABEL_MOREINFO,
			LABEL_COUNT
		};

		enum ColorType {
			PLAYER_COLOR_BLACK,
			PLAYER_COLOR_RED,
			PLAYER_COLOR_GREEN,
			PLAYER_COLOR_BLUE,
			PLAYER_COLOR_COUNT,
		};

		void Init();
		void InitTitle();
		void InitSinglePlayer();
		void InitOptions();
		void InitControls();
		void InitInGame();
		void InitPlay();
		void InitScore();
		void InitAchievements();
		void ConfirmAction();
		void Close();

		bool HandleKey(const ae::_KeyEvent &KeyEvent);
		void HandleMouseButton(const ae::_MouseEvent &MouseEvent);
		void HandleResize();
		void SetFullscreen(bool Fullscreen);
		void ShowDefaultCursor(bool Value);

		void Update(double FrameTime);
		void Render();
		void DrawMessages();

		void UnlockAchievement(const std::string &ID);
		void SetScoreStats(bool EndOfGame, double LevelTime, int *Kills, int *Crates, int *Secrets, int Progression, bool GotOneHundredPercent);

		const StateType &GetState() const { return State; }

	private:

		void ChangeLayout(const std::string &ElementName);

		void InitNewPlayer();
		void LaunchGame();

		void UpdateOptions();
		void UpdateVolume();
		void UpdateMSAA();
		void UpdateAnisotropy();
		void UpdateTextures();
		void RefreshInputLabels();
		void RefreshSaveSlots();
		void SinglePlayerCancel();
		void CreatePlayer();
		void ClearAction(int Action, int Type);
		void RemapInput(int InputType, int Input);

		// States
		StateType State{STATE_NONE};

		// UI
		ae::_Element *Background{nullptr};
		ae::_Element *CurrentLayout{nullptr};
		ae::_Element *InputLabels[LABEL_COUNT]{nullptr};
		ae::_Element *SaveSlots[SAVE_SLOTS]{nullptr};
		ae::_Element *ColorButtons[4]{nullptr};

		// Double click
		ae::_Element *PreviousClick{nullptr};
		double PreviousClickTimer{0.0};

		// Options
		OptionsStateType OptionsState{OPTION_NONE};
		std::vector<int> MSAAValues;
		std::vector<int> AnisotropyValues;
		int LastAnisotropy{-1};
		int CurrentAction{-1};

		// Singleplayer
		SinglePlayerStateType SinglePlayerState{SINGLEPLAYER_NONE};
		int SelectedSlot{-1};
		int SelectedColor{0};

		// Achievements
		std::vector<_Message> AchievementMessages;
};

extern _Menu Menu;
