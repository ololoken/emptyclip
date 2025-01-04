/******************************************************************************
* Empty Clip
* Copyright (C) 2025 Alan Witkowski
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
#include <string>
#include <vector>

// Forward Declarations
class _Player;
namespace ae {
	class _Buffer;
}

// Classes
class _Save {

	public:

		_Save();
		~_Save();

		void CreateNewPlayer(size_t Slot, const std::string &Name, const std::string &ColorID, bool Hardcore);
		void DeletePlayer(size_t Slot);
		void LoadSaves();
		void LoadPlayer(_Player *Player);
		void SavePlayer(_Player *Player);

		_Player *GetPlayer(size_t Slot) { return Players[Slot]; }

	private:

		std::string GetSavePath(size_t Slot);

		void LoadInventory(_Player *Player, ae::_Buffer &Buffer);
		void LoadAmmo(_Player *Player, ae::_Buffer &Buffer);
		void LoadKeys(_Player *Player, ae::_Buffer &Buffer);

		void SaveInventory(_Player *Player, std::ofstream &File);
		void SaveAmmo(_Player *Player, std::ofstream &File);
		void SaveKeys(_Player *Player, std::ofstream &File);
		void SaveSkills(_Player *Player, std::ofstream &File);
		void SaveFilters(_Player *Player, std::ofstream &File);

		// Players
		std::vector<_Player *> Players;

};

extern _Save Save;
