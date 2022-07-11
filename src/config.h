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
#include <glm/vec2.hpp>
#include <unordered_map>
#include <sstream>
#include <string>
#include <vector>

// Stores configuration data
class _Config {

	public:

		void Init(const std::string &ConfigFile);
		void Close();

		void Load();
		void Save();
		void SetDefaults();
		void LoadDefaultInputBindings(bool IfNone);

		// State
		int Version;
		std::string ConfigPath;

		// Graphics
		glm::ivec2 WindowSize;
		double MaxFPS;
		int Vsync;
		int MSAA;
		int Anisotrophy;
		int Fullscreen;

		// Game
		bool WeaponFlashes;

		// Audio
		int AudioEnabled;
		float SoundVolume;
		float MusicVolume;

	private:

		template <typename Type>
		void GetValue(const std::string &Field, Type &Value) {
			const auto &MapIterator = Map.find(Field);
			if(MapIterator != Map.end()) {
				std::stringstream Stream(MapIterator->second.front());
				Stream >> Value;
			}
		}

		// State
		std::string ConfigFilePath;
		std::unordered_map<std::string, std::vector<std::string>> Map;
};

extern _Config Config;
