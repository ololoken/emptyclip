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

#include <input.h>
#include <cstdint>

class _Console;

// Classes
class _State {

	public:

		// Setup
		_State();
		virtual ~_State();
		virtual void Init() { }
		virtual void Close() { }

		// Input
		virtual bool HandleAction(int InputType, std::size_t Action, int Value) { return false; }
		virtual bool HandleKey(const _KeyEvent &KeyEvent) { return true; }
		virtual void HandleMouseButton(const _MouseEvent &MouseEvent) { }
		virtual void HandleMouseMove(const glm::ivec2 &Position) { }
		virtual void HandleMouseWheel(int Direction) { }
		virtual bool HandleCommand(_Console *Console) { return false; }
		virtual void HandleWindow(uint8_t Event) { }
		virtual void HandleQuit() { }

		// Update
		virtual void Update(double FrameTime) { }
		virtual void Render(double BlendFactor) { }

	protected:

};
