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
#include <vector>
#include <string>
#include <coord.h>

// Enumerations
enum EventType {
	EVENT_DOOR,
	EVENT_WSWITCH,
	EVENT_SPAWN,
	EVENT_CHECK,
	EVENT_END,
	EVENT_TEXT,
	EVENT_SOUND,
	EVENT_FSWITCH,
	EVENT_ENABLE,
	EVENT_TELE,
	EVENT_LIGHT,
};

struct _EventTile {
	_EventTile() { }
	_EventTile(const _Coord &Coord, int Layer, int BlockID) : Coord(Coord), Layer(Layer), BlockID(BlockID) { }

	_Coord Coord;
	int Layer;
	int BlockID;
};

// Classes
class _Event {

	public:

		_Event(int Type, int Active, const _Coord &Start, const _Coord &End, int Level, double ActivationPeriod, const std::string &ItemIdentifier, const std::string &MonsterIdentifier, const std::string &ParticleIdentifier);
		~_Event();

		void Update(double FrameTime);

		void AddTile(_EventTile Tile);
		void RemoveTile(const std::vector<_EventTile>::iterator &Iterator) { Tiles.erase(Iterator); }
		void DeleteBlockID(int Layer, int Index);
		std::vector<_EventTile>::iterator FindTile(int X, int Y);
		void StartTimer() { Timer = 0; }
		void Decrement() { Level--; }
		bool TimerExpired() const { return (Timer > ActivationPeriod); }

		int Type;
		int Active;
		int Level;
		_Coord Start;
		_Coord End;
		std::vector<_EventTile> Tiles;
		std::string ItemIdentifier;
		std::string MonsterIdentifier;
		std::string ParticleIdentifier;
		double Timer;
		double ActivationPeriod;
};
