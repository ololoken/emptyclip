/******************************************************************************
* Empty Clip
* Copyright (C) 2024 Alan Witkowski
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
#include <unordered_map>
#include <ctime>

// Classes
class _Achievements {

	public:

		struct _Stat {
			std::string Version;
			int Value{0};
			std::time_t Time{0};
		};

		void Init();
		void Load();
		void Save();
		void Backup();

		bool Enabled{true};
		std::string Path;

		std::unordered_map<std::string, _Stat> Stats;
};

extern _Achievements Achievements;
