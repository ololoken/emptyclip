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

#include <cstddef>

namespace Action {
	enum size_t {
		GAME_UP,
		GAME_DOWN,
		GAME_LEFT,
		GAME_RIGHT,
		GAME_USE,
		GAME_SPRINT,
		GAME_MAP,
		GAME_FLASHLIGHT,
		GAME_FIRE,
		GAME_AIM,
		GAME_MELEE,
		GAME_RELOAD,
		GAME_WEAPONSWITCH,
		GAME_INVENTORY,
		GAME_SORTINVENTORY,
		GAME_MOREINFO,
		GAME_FILTERGEAR,
		GAME_FILTERMODS,
		GAME_SWITCHOUTFIT,
		GAME_MINIMAP,
		MISC_CONSOLE,
		MISC_MENU,
		MISC_DEBUG,
		COUNT,
	};
}
