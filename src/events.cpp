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
#include <events.h>

// Constructor
_Event::_Event(int Type, int Active, const glm::ivec2 &Start, const glm::ivec2 &End, int Level, int SpawnLevel, double ActivationPeriod, const std::string &ItemID, const std::string &MonsterID, const std::string &ParticleID) :
	Type(Type),
	Active(Active),
	Level(Level),
	SpawnLevel(SpawnLevel),
	Switched(false),
	Start(Start),
	End(End),
	ItemID(ItemID),
	MonsterID(MonsterID),
	ParticleID(ParticleID),
	Timer(0),
	ActivationPeriod(ActivationPeriod) {

}

// Destructor
_Event::~_Event() {
}

// Adds a tile to the list
void _Event::AddTile(_EventTile Tile) {

	// Search for existing blocks
	for(size_t i = 0; i < Tiles.size(); i++) {
		if(Tiles[i].Layer == Tile.Layer && Tiles[i].BlockID == Tile.BlockID) {
			Tile.BlockID = -1;
			break;
		}
	}

	Tiles.push_back(Tile);
}

// Deletes a block id from the tiles
void _Event::DeleteBlockID(int Layer, int Index) {
	for(size_t i = 0; i < Tiles.size(); i++) {
		if(Tiles[i].Layer == Layer) {
			if(Tiles[i].BlockID == Index)
				Tiles[i].BlockID = -1;
			else if(Tiles[i].BlockID > Index)
				Tiles[i].BlockID--;
		}
	}
}

// Searchs for a tile in the list given a position
std::vector<_EventTile>::iterator _Event::FindTile(const glm::ivec2 &Position) {
	for(auto Iterator = Tiles.begin(); Iterator != Tiles.end(); ++Iterator) {
		if(Iterator->Coord == Position)
			return Iterator;
	}

	return Tiles.end();
}

// Update the event
void _Event::Update(double FrameTime) {
	Timer += FrameTime;
}
