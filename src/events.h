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
#include <glm/vec4.hpp>
#include <vector>
#include <string>

// Enumerations
enum EventType {
	EVENT_DOOR,
	EVENT_WALLSWITCH,
	EVENT_SPAWN,
	EVENT_CHECK,
	EVENT_ENDLEVEL,
	EVENT_TEXT,
	EVENT_SOUND,
	EVENT_FLOORSWITCH,
	EVENT_ENABLE,
	EVENT_TELEPORT,
	EVENT_LIGHT,
	EVENT_SECRET,
};

struct _EventTile {

	_EventTile() : Coord(0.0f), Layer(0), BlockID(0) { }
	_EventTile(const glm::ivec2 &Coord, int Layer, int BlockID) : Coord(Coord), Layer(Layer), BlockID(BlockID) { }

	glm::ivec2 Coord;
	int Layer;
	int BlockID;
};

// Classes
class _Event {

	public:

		_Event();
		~_Event();

		void Update(double FrameTime);

		void AddTile(_EventTile Tile);
		void RemoveTile(const std::vector<_EventTile>::iterator &Iterator) { Tiles.erase(Iterator); }
		void DeleteBlockID(int Layer, int Index);
		std::vector<_EventTile>::iterator FindTile(const glm::ivec2 &Position);
		void StartTimer() { Timer = 0; }
		void Decrement() { Level--; }
		bool TimerExpired() const { return Timer > ActivationPeriod; }

		void GetBounds(glm::vec4 &Bounds) { Bounds[0] = Start.x; Bounds[1] = Start.y; Bounds[2] = End.x + 1.0f; Bounds[3] = End.y + 1.0f; }

		int Type;
		int Active;
		int Level;
		int SpawnLevel;
		int SpawnMultiplier;
		bool Switched;
		glm::ivec2 Start;
		glm::ivec2 End;
		std::vector<_EventTile> Tiles;
		std::string ItemID;
		std::string MonsterID;
		std::string ParticleID;
		double Timer;
		double ActivationPeriod;
};
