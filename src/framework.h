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
#include <ae/log.h>
#include <cstdint>

// Forward Declarations
union SDL_Event;

namespace ae {
	class _FrameLimit;
	class _State;
	class _Console;
}

// Manages SDL and game state
class _Framework {

	public:

		enum StateType {
		    INIT,
		    UPDATE,
		    CLOSE
		};

		// Setup
		void Init(int ArgumentCount, char **Arguments);
		void Close();

		// Update functions
		void Update();
		void Render();

		ae::_State *GetState() { return State; }
		void ChangeState(ae::_State *RequestedState);

		// Console
		ae::_Console *Console{nullptr};

		// State
		ae::_LogFile Log;
		bool Done{false};
		bool IgnoreNextInputEvent{false};

	private:

		int GlobalKeyHandler(const SDL_Event &Event);
		void HandleCommand(ae::_Console *Console);
		void LoadAssets();

		// States
		StateType FrameworkState{INIT};
		ae::_State *State{nullptr};
		ae::_State *RequestedState{nullptr};

		// Time
		ae::_FrameLimit *FrameLimit{nullptr};
		uint64_t Timer{0};
		double TimeStep{0.0};
		double TimeStepAccumulator{0.0};

};

extern _Framework Framework;
