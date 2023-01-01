/******************************************************************************
* Empty Clip
* Copyright (C) 2023 Alan Witkowski
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
#include <states/null.h>
#include <ae/graphics.h>
#include <ae/ui.h>
#include <framework.h>
#include <menu.h>

_NullState NullState;

// Initialize
void _NullState::Init() {
	if(LevelComplete)
		Menu.InitScore();
	else
		Menu.InitTitle();

	LevelComplete = false;
}

// Close
void _NullState::Close() {
}

// Key handler
bool _NullState::HandleKey(const ae::_KeyEvent &KeyEvent) {
	bool Handled = ae::Graphics.Element->HandleKey(KeyEvent);
	if(!Handled)
		return Menu.HandleKey(KeyEvent);

	return false;
}

// Mouse handler
void _NullState::HandleMouseButton(const ae::_MouseEvent &MouseEvent) {
	Menu.HandleMouseButton(MouseEvent);
}

// Handle window events
void _NullState::HandleWindow(uint8_t Event) {
	if(Event == SDL_WINDOWEVENT_SIZE_CHANGED)
		Menu.HandleResize();
}

// Handle quit events
void _NullState::HandleQuit() {
	Framework.Done = true;
}

// Update
void _NullState::Update(double FrameTime) {
	ae::Graphics.Element->Update(FrameTime, ae::Input.GetMouse());
	//if(ae::Graphics.Element->HitElement)
	//	std::cout << ae::Graphics.Element->HitElement->Name << std::endl;

	Menu.Update(FrameTime);
}

// Render the state
void _NullState::Render(double BlendFactor) {
	Menu.Render();
}
