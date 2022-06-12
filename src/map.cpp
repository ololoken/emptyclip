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
#include <map.h>
#include <utils.h>
#include <graphics.h>
#include <assets.h>
#include <camera.h>
#include <events.h>
#include <objectmanager.h>
#include <objects/entity.h>
#include <objects/item.h>
#include <constants.h>
#include <fstream>
#include <stdexcept>
#include <iomanip>
#include <iostream>
#include <glm/gtx/norm.hpp>

// Initialize
_Map::_Map() :
	MapType(MAPTYPE_SINGLE),
	Width(MAP_WIDTH),
	Height(MAP_HEIGHT),
	Filename(""),
	Data(nullptr),
	ObjectManager(new _ObjectManager()),
	Camera(nullptr),
	MonsterSet(MAP_DEFAULTMONSTERSET),
	AmbientLight(0.0f, 0.0f, 0.0f, 1.0f),
	OldAmbientLight(0.0f, 0.0f, 0.0f, 1.0f),
	AmbientLightBlendFactor(1.0),
	AmbientLightPeriod(0.0),
	AmbientLightTimer(0.0) {

}

// Initialize
_Map::_Map(const std::string &Filename) : _Map() {
	if(Filename == "")
		throw std::runtime_error("Empty file name");

	this->Filename = Filename;

	// Load file
	std::ifstream InputFile((Assets.GetAssetPath() + ASSETS_MAPS + Filename).c_str(), std::ios::in);
	if(!InputFile)
		throw std::runtime_error("Cannot load file: " + Filename);

	// Get file version
	int FileVersion;
	InputFile >> FileVersion;
	if(FileVersion != MAP_FILEVERSION)
		throw std::runtime_error("Level version mismatch: ");

	// Get map type
	InputFile >> MapType;

	// Load monster set file name
	std::string SetFilename;
	InputFile >> SetFilename;
	if(!LoadMonsterSet(SetFilename))
		throw std::runtime_error("Cannot load monster set: " + SetFilename);

	// Read dimensions
	InputFile >> Width >> Height;

	// Load objects
	size_t ObjectCount;
	InputFile >> ObjectCount;
	for(size_t i = 0; i < ObjectCount; i++) {

		// Load Data
		_ObjectSpawn *Object = new _ObjectSpawn();
		InputFile >> Object->Type >> Object->Identifier >> Object->Position.x >> Object->Position.y;

		// Check for items
		switch(Object->Type) {
			case _Object::MONSTER:
				if(!Assets.IsMonsterLoaded(Object->Identifier))
					throw std::runtime_error("Cannot find monster: " + Object->Identifier);
			break;
			case _Object::MISCITEM:
				if(!Assets.IsMiscItemLoaded(Object->Identifier))
					throw std::runtime_error("Cannot find misc item: " + Object->Identifier);
			break;
			case _Object::AMMO:
				if(!Assets.IsAmmoLoaded(Object->Identifier))
					throw std::runtime_error("Cannot find ammo: " + Object->Identifier);
			break;
			case _Object::UPGRADE:
				if(!Assets.IsUpgradeLoaded(Object->Identifier))
					throw std::runtime_error("Cannot find upgrade: " + Object->Identifier);
			break;
			case _Object::WEAPON:
				if(!Assets.IsWeaponLoaded(Object->Identifier))
					throw std::runtime_error("Cannot find weapon: " + Object->Identifier);
			break;
			case _Object::ARMOR:
				if(!Assets.IsArmorLoaded(Object->Identifier))
					throw std::runtime_error("Cannot find armor: " + Object->Identifier);
			break;
		}

		ObjectSpawns.push_back(Object);
	}

	// Read events count
	size_t EventCount;
	InputFile >> EventCount;

	// Load events
	for(size_t i = 0; i < EventCount; i++) {

		int EventType, EventActive, EventLevel;
		_Coord EventStart, EventEnd;
		double EventActivationPeriod;
		size_t TilesSize;
		InputFile >> EventType >> EventActive >> EventStart.x >> EventStart.y >> EventEnd.x >> EventEnd.y >> EventLevel >> EventActivationPeriod >> TilesSize;
		std::string EventItemIdentifier = GetCSVText(InputFile);
		std::string EventMonsterIdentifier = GetCSVText(InputFile);
		std::string EventParticleIdentifier = GetCSVText(InputFile);

		// Check for existence
		if(EventMonsterIdentifier != "" && !Assets.IsMonsterLoaded(EventMonsterIdentifier))
			throw std::runtime_error("Cannot find monster: " + EventMonsterIdentifier);
		if(EventParticleIdentifier != "" && !Assets.IsParticleLoaded(EventParticleIdentifier))
			throw std::runtime_error("Cannot find particle: " + EventParticleIdentifier);

		_Event *Event = new _Event(EventType, EventActive, EventStart, EventEnd, EventLevel, EventActivationPeriod, EventItemIdentifier, EventMonsterIdentifier, EventParticleIdentifier);
		for(size_t j = 0; j < TilesSize; j++) {
			_Coord Tile;
			int TileLayer, TileBlockID;
			InputFile >> Tile.x >> Tile.y >> TileLayer >> TileBlockID;
			Tile = GetValidCoord(Tile);
			Event->AddTile(_EventTile(Tile, TileLayer, TileBlockID));
		}
		Events.push_back(Event);

		if(Event->Type == EVENT_CHECK)
			CheckpointEvents.push_back(Event);
	}

	// Read block size
	size_t BlockCount;
	InputFile >> BlockCount;

	// Load blocks
	_Block Block;
	for(size_t i = 0; i < BlockCount; i++) {

		int Layer;
		InputFile >> Layer >> Block.Start.x >> Block.Start.y >> Block.End.x >> Block.End.y >> Block.MinZ >> Block.MaxZ >> Block.Rotation >> Block.ScaleX >> Block.Wall >> Block.Walkable;
		Block.TextureIdentifier = GetCSVText(InputFile);
		Block.AltTextureIdentifier = GetCSVText(InputFile);

		Block.Texture = Assets.GetTexture(Block.TextureIdentifier);
		if(!Block.Texture)
			throw std::runtime_error("Cannot find texture: " + Block.TextureIdentifier);

		if(Block.AltTextureIdentifier != "") {
			Block.AltTexture = Assets.GetTexture(Block.AltTextureIdentifier);
			if(!Block.AltTexture)
				throw std::runtime_error("Cannot find alt texture: " + Block.AltTextureIdentifier);
		}
		else
			Block.AltTexture = nullptr;

		Block.Start = GetValidCoord(Block.Start);
		Block.End = GetValidCoord(Block.End);
		Blocks[Layer].push_back(Block);
	}

	InputFile.close();
}

// Shut down
_Map::~_Map() {

	// Remove objects
	for(size_t i = 0; i < ObjectSpawns.size(); i++)
		delete ObjectSpawns[i];

	// Remove events
	for(size_t i = 0; i < Events.size(); i++)
		delete Events[i];

	if(Data != nullptr) {
		for(int i = 0; i < Width; i++)
			delete[] Data[i];
		delete[] Data;
	}

	Assets.UnloadMonsterSet();
}

// Create tile data
void _Map::Init() {

	// Allocate memory
	Data = new _Tile*[Width];

	for(int i = 0; i < Width; i++)
		Data[i] = new _Tile[Height];

	// Clear out array
	for(int i = 0; i < Width; i++) {
		for(int j = 0; j < Height; j++) {
			Data[i][j] = _Tile();
		}
	}

	// Loop through layers and fill out walkable field
	for(int l = 0; l < MAPLAYER_FORE; l++) {
		for(size_t k = 0; k < Blocks[l].size(); k++) {
			for(int i = Blocks[l][k].Start.x; i <= Blocks[l][k].End.x; i++) {
				for(int j = Blocks[l][k].Start.y; j <= Blocks[l][k].End.y; j++) {
					if(Blocks[l][k].Walkable)
						Data[i][j].Collision &= ~_Tile::ENTITY;
					else
						Data[i][j].Collision |= _Tile::ENTITY;
				}
			}
		}
	}

	// Loop through walls
	for(size_t k = 0; k < Blocks[5].size(); k++) {
		for(int i = Blocks[5][k].Start.x; i <= Blocks[5][k].End.x; i++) {
			for(int j = Blocks[5][k].Start.y; j <= Blocks[5][k].End.y; j++) {
				if(Blocks[5][k].Wall) {
					if(Blocks[5][k].Walkable)
						Data[i][j].Collision &= ~_Tile::ENTITY & ~_Tile::BULLET;
					else
						Data[i][j].Collision |= _Tile::ENTITY | _Tile::BULLET;
				}
			}
		}
	}

	// Loop through the events and fill out array
	for(size_t k = 0; k < Events.size(); k++) {
		for(int i = Events[k]->Start.x; i <= Events[k]->End.x; i++) {
			for(int j = Events[k]->Start.y; j <= Events[k]->End.y; j++) {
				Data[i][j].Events.push_back(Events[k]);
			}
		}
	}
}

// Saves the level to a file
bool _Map::SaveLevel(const std::string &String) {

	Filename = String;
	std::ofstream Output((Assets.GetAssetPath() + ASSETS_MAPS + Filename).c_str(), std::ios::out);
	if(!Output)
		throw std::runtime_error("Cannot create file: " + Filename);

	Output << std::showpoint << std::fixed << std::setprecision(2);

	// Header
	Output << MAP_FILEVERSION << '\n';
	Output << MapType << '\n';
	Output << MonsterSet << '\n';
	Output << Width << " " << Height << '\n';

	// Objects
	Output << ObjectSpawns.size() << '\n';
	for(size_t i = 0; i < ObjectSpawns.size(); i++) {
		Output << ObjectSpawns[i]->Type << " " << ObjectSpawns[i]->Identifier << " " << ObjectSpawns[i]->Position.x << " " << ObjectSpawns[i]->Position.y << " " << '\n';
	}

	// Events
	Output << Events.size() << '\n';
	for(size_t i = 0; i < Events.size(); i++) {
		Output << Events[i]->Type << " ";
		Output << Events[i]->Active << " ";
		Output << Events[i]->Start.x << " ";
		Output << Events[i]->Start.y << " ";
		Output << Events[i]->End.x << " ";
		Output << Events[i]->End.y << " ";
		Output << Events[i]->Level << " ";
		Output << Events[i]->ActivationPeriod << " ";
		Output << Events[i]->Tiles.size() << " ";
		Output << '\"' << Events[i]->ItemIdentifier << '\"' << " ";
		Output << '\"' << Events[i]->MonsterIdentifier << '\"' << " ";
		Output << '\"' << Events[i]->ParticleIdentifier << '\"' << '\n';

		// Write tiles
		for(size_t j = 0; j < Events[i]->Tiles.size(); j++)
			Output << Events[i]->Tiles[j].Coord.x << " " << Events[i]->Tiles[j].Coord.y << " " << Events[i]->Tiles[j].Layer << " " << Events[i]->Tiles[j].BlockID << '\n';
	}

	// Blocks
	Output << GetTotalBlockSize() << '\n';
	for(int i = 0; i < MAPLAYER_COUNT; i++) {
		for(size_t j = 0; j < Blocks[i].size(); j++) {
			Output << i << " ";
			Output << Blocks[i][j].Start.x << " ";
			Output << Blocks[i][j].Start.y << " ";
			Output << Blocks[i][j].End.x << " ";
			Output << Blocks[i][j].End.y << " ";
			Output << Blocks[i][j].MinZ << " ";
			Output << Blocks[i][j].MaxZ << " ";
			Output << Blocks[i][j].Rotation << " ";
			Output << Blocks[i][j].ScaleX << " ";
			Output << Blocks[i][j].Wall << " ";
			Output << Blocks[i][j].Walkable << " ";
			Output << '\"' << Blocks[i][j].TextureIdentifier << '\"' << " ";
			Output << '\"' << Blocks[i][j].AltTextureIdentifier << '\"' << '\n';
		}
	}

	Output.close();

	return true;
}

// Loads a monster set
bool _Map::LoadMonsterSet(const std::string &String) {
	Assets.LoadMonsterSet(ASSETS_MONSTERSETS + String);
	MonsterSet = String;

	return true;
}

// Adds an object to the collision grid
void _Map::AddObjectToGrid(_Object *Object, int Type) {
	if(!Data)
		throw std::runtime_error("Tile data uninitialized!");

	// Get the object's bounding rectangle
	_TileBounds TileBounds;
	GetTileBounds(Object->Position, Object->Radius, TileBounds);

	for(int i = TileBounds.Start.x; i <= TileBounds.End.x; i++) {
		for(int j = TileBounds.Start.y; j <= TileBounds.End.y; j++) {
			Data[i][j].Objects[Type].push_front(Object);
		}
	}
}

// Removes an object from the collision grid
void _Map::RemoveObjectFromGrid(_Object *Object, int Type) {
	if(!Data)
		throw std::runtime_error("Tile data uninitialized!");

	// Get the object's bounding rectangle
	_TileBounds TileBounds;
	GetTileBounds(Object->Position, Object->Radius, TileBounds);

	for(int i = TileBounds.Start.x; i <= TileBounds.End.x; i++) {
		for(int j = TileBounds.Start.y; j <= TileBounds.End.y; j++) {
			for(auto Iterator = Data[i][j].Objects[Type].begin(); Iterator != Data[i][j].Objects[Type].end(); ++Iterator) {
				if(*Iterator == Object) {
					Data[i][j].Objects[Type].erase(Iterator);
					break;
				}
			}
		}
	}
}

// Check collision with tiles and resolve
bool _Map::CheckCollisions(const glm::vec2 &TargetPosition, float Radius, glm::vec2 &NewPosition) {
	if(!Data)
		throw std::runtime_error("Tile data uninitialized!");

	NewPosition = TargetPosition;
	float Left = NewPosition.x - Radius;
	float Right = NewPosition.x + Radius;
	float Top = NewPosition.y - Radius;
	float Bottom = NewPosition.y + Radius;

	// Check boundaries
	bool Hit = false;
	if(Left < 0) {
		Left = NewPosition.x = Radius;
		Hit = true;
	}
	if(Top < 0) {
		Top = NewPosition.y = Radius;
		Hit = true;
	}
	if(Right >= (float)Width) {
		Right = NewPosition.x = (float)Width - Radius;
		Hit = true;
	}
	if(Bottom >= (float)Height) {
		Bottom = NewPosition.y = (float)Height - Radius;
		Hit = true;
	}

	// Check tiles
	int LeftTile = (int)Left;
	int RightTile = (int)Right;
	int TopTile = (int)Top;
	int BottomTile = (int)Bottom;

	std::list<glm::vec2> Pushes;
	bool NoDiag = false;
	for(int i = LeftTile; i <= RightTile; i++) {
		for(int j = TopTile; j <= BottomTile; j++) {
			if(!Data[i][j].CanWalk()) {

				bool DiagonalPush = false;
				glm::vec2 Push(0, 0);
				if(CheckTileCollision(NewPosition, Radius, (float)i, (float)j, true, Push, DiagonalPush)) {
					Hit = true;
					Pushes.push_back(Push);

					// If any non-diagonal vectors, flag it
					if(!DiagonalPush)
						NoDiag = true;
				}
			}
		}
	}

	// Resolve collision
	for(const auto &Push : Pushes) {
		if(!(NoDiag && Push.x != 0 && Push.y != 0)) {
			NewPosition += Push;
		}
	}

	return Hit;
}

// Resolve collision with a tile
bool _Map::CheckTileCollision(const glm::vec2 &Position, float Radius, float X, float Y, bool Resolve, glm::vec2 &Push, bool &DiagonalPush) {
	float AABB[4] = { X, Y, X + 1, Y + 1 };
	int ClampCount = 0;

	// Get closest point on AABB
	glm::vec2 Point = Position;
	if(Point.x < AABB[0]) {
		Point.x = AABB[0];
		ClampCount++;
	}
	if(Point.y < AABB[1]) {
		Point.y = AABB[1];
		ClampCount++;
	}
	if(Point.x > AABB[2]) {
		Point.x = AABB[2];
		ClampCount++;
	}
	if(Point.y > AABB[3]) {
		Point.y = AABB[3];
		ClampCount++;
	}

	// Test circle collision with point
	float DistanceSquared = glm::distance2(Point, Position);
	bool Hit = DistanceSquared < Radius * Radius;

	// Push object out
	if(Hit && Resolve) {

		// Check if object is inside the AABB
		if(ClampCount == 0) {
			glm::vec2 Center(X + 0.5f, Y + 0.5f);
			if(Position.x <= Center.x)
				Push.x = -(X - Position.x - Radius);
			else if(Position.x > Center.x)
				Push.x = (X - Position.x) + 1 + Radius;
		}
		else {

			// Get push direction
			Push = Position - Point;

			// Get push amount
			float Amount = Radius - glm::length(Push);

			// Scale push vector
			Push = glm::normalize(Push);
			Push *= Amount;

			// Set whether the push is diagnol or not
			DiagonalPush = ClampCount > 1;
		}
	}

	return Hit;
}

// Checks for collisions with an object in the collision grid
_Object *_Map::CheckCollisionsInGrid(const glm::vec2 &Position, float Radius, int GridType, const _Object *SkipObject) const {
	if(!Data)
		throw std::runtime_error("Tile data uninitialized!");

	float DistanceSquared, RadiiSum;

	// Get the object's bounding rectangle
	_TileBounds TileBounds;
	GetTileBounds(Position, Radius, TileBounds);

	for(int i = TileBounds.Start.x; i <= TileBounds.End.x; i++) {
		for(int j = TileBounds.Start.y; j <= TileBounds.End.y; j++) {
			for(auto Iterator : Data[i][j].Objects[GridType]) {
				if(Iterator != SkipObject) {
					DistanceSquared = glm::distance2(Iterator->Position, Position);
					RadiiSum = Iterator->Radius + Radius;

					// Check circle intersection
					if(DistanceSquared < RadiiSum * RadiiSum) {
						return Iterator;
					}
				}
			}
		}
	}

	return nullptr;
}

// Returns a list of entities that an object is colliding with
void _Map::CheckEntityCollisionsInGrid(const glm::vec2 &Position, float Radius, const _Object *SkipObject, std::list<_Entity *> &Entities) const {
	if(!Data)
		throw std::runtime_error("Tile data uninitialized!");

	// Get the object's bounding rectangle
	_TileBounds TileBounds;
	GetTileBounds(Position, Radius, TileBounds);

	for(int i = TileBounds.Start.x; i <= TileBounds.End.x; i++) {
		for(int j = TileBounds.Start.y; j <= TileBounds.End.y; j++) {
			for(int k = 0; k < 2; k++) {
				for(auto Iterator = Data[i][j].Objects[k].begin(); Iterator != Data[i][j].Objects[k].end(); ++Iterator) {
					_Entity *Entity = static_cast<_Entity *>(*Iterator);
					if(Entity != SkipObject && !Entity->IsDying()) {
						float DistanceSquared = glm::distance2(Entity->Position, Position);
						float RadiiSum = Entity->Radius + Radius;

						// Check circle intersection
						if(DistanceSquared < RadiiSum * RadiiSum)
							Entities.push_back(Entity);
					}
				}
			}
		}
	}
}

// Checks for melee collisions with entities in the collision grid
_Entity *_Map::CheckMeleeCollisions(_Entity *Attacker, const glm::vec2 &Direction, int GridType) const {
	if(!Data)
		throw std::runtime_error("Tile data uninitialized!");

	// Get the object's bounding rectangle
	_TileBounds TileBounds;
	GetTileBounds(Attacker->Position, Attacker->GetWeaponRange(Attacker->AttackRequestType), TileBounds);
	for(int i = TileBounds.Start.x; i <= TileBounds.End.x; i++) {
		for(int j = TileBounds.Start.y; j <= TileBounds.End.y; j++) {
			for(auto Iterator = Data[i][j].Objects[GridType].begin(); Iterator != Data[i][j].Objects[GridType].end(); ++Iterator) {
				_Entity *Entity = static_cast<_Entity *>(*Iterator);
				if(!Entity->IsDying()) {
					float DistanceSquared = glm::distance2(Entity->Position, Attacker->Position);
					float RadiiSum = Entity->Radius + Attacker->GetWeaponRange(Attacker->AttackRequestType);

					// Check circle intersection
					if(DistanceSquared < RadiiSum * RadiiSum) {
						glm::vec2 ObjectDirection(glm::normalize(Entity->Position - Attacker->Position));

						// Compare angles
						if(glm::dot(Direction, ObjectDirection) > cosf(glm::radians(Attacker->GetMaxAccuracy(Attacker->AttackRequestType) * 0.5f))) {

							// Check for walls
							if(IsVisible(Attacker->Position, Entity->Position))
								return Entity;
						}
					}
				}
			}
		}
	}

	return nullptr;
}

// Determines which walls are adjacent to the object
int _Map::GetWallState(const glm::vec2 &Position, float Radius) const {
	if(!Data)
		throw std::runtime_error("Tile data uninitialized!");

	// Get the object's bounding rectangle
	_TileBounds TileBounds;
	GetTileBounds(Position, Radius, TileBounds);

	// Check left wall
	int WallState = 0;
	_Coord TopLeft = GetValidCoord(_Coord((int)(Position.x - Radius - MAP_EPSILON), (int)(Position.y - Radius - MAP_EPSILON)));
	for(int i = TileBounds.Start.y; i <= TileBounds.End.y; i++) {
		if(!Data[TopLeft.x][i].CanWalk()) {
			WallState |= WALL_LEFT;
			break;
		}
	}

	// Check top wall
	for(int i = TileBounds.Start.x; i <= TileBounds.End.x; i++) {
		if(!Data[i][TopLeft.y].CanWalk()) {
			WallState |= WALL_TOP;
			break;
		}
	}

	// Check right wall
	_Coord BottomRight = GetValidCoord(_Coord((int)(Position.x + Radius + MAP_EPSILON), (int)(Position.y + Radius + MAP_EPSILON)));
	for(int i = TileBounds.Start.y; i <= TileBounds.End.y; i++) {
		if(!Data[BottomRight.x][i].CanWalk()) {
			WallState |= WALL_RIGHT;
			break;
		}
	}

	// Check bottom wall
	for(int i = TileBounds.Start.x; i <= TileBounds.End.x; i++) {
		if(!Data[i][BottomRight.y].CanWalk()) {
			WallState |= WALL_BOTTOM;
			break;
		}
	}

	return WallState;
}

// Determines what adjacent square the object is facing
void _Map::GetAdjacentTile(const glm::vec2 &Position, float Direction, _Coord &Coord) const {

	// Check direction
	if(Direction > 45.0f && Direction < 135.0f) {
		Coord = GetValidCoord(_Coord(Position.x + 1.0f, Position.y));
	}
	else if(Direction >= 135.0f && Direction < 225.0f) {
		Coord = GetValidCoord(_Coord(Position.x, Position.y + 1.0f));
	}
	else if(Direction >= 225.0f && Direction < 315.0f) {
		Coord = GetValidCoord(_Coord(Position.x - 1.0f, Position.y));
	}
	else {
		Coord = GetValidCoord(_Coord(Position.x, Position.y - 1.0f));
	}
}

// Checks bullet collisions with objects and walls
void _Map::CheckBulletCollisions(const glm::vec2 &Position, const glm::vec2 &Direction, _Entity **HitEntity, glm::vec2 *HitPosition, int GridType, bool CheckObjects) const {
	if(!Data)
		throw std::runtime_error("Tile data uninitialized!");

	// Find slope
	float Slope = Direction.y / Direction.x;

	// Find starting tile
	_Coord TileTracer = GetValidCoord(_Coord(Position.x, Position.y));

	// Check x direction
	int TileIncrementX, FirstBoundaryTileX;
	if(Direction.x < 0) {
		FirstBoundaryTileX = TileTracer.x;
		TileIncrementX = -1;
	}
	else {
		FirstBoundaryTileX = TileTracer.x + 1;
		TileIncrementX = 1;
	}

	// Check y direction
	int TileIncrementY, FirstBoundaryTileY;
	if(Direction.y < 0) {
		FirstBoundaryTileY = TileTracer.y;
		TileIncrementY = -1;
	}
	else {
		FirstBoundaryTileY = TileTracer.y + 1;
		TileIncrementY = 1;
	}

	// Find ray direction ratios
	glm::vec2 Ratio(1.0f / Direction.x, 1.0f / Direction.y);

	// Calculate increments
	glm::vec2 Increment(TileIncrementX * Ratio.x, TileIncrementY * Ratio.y);

	// Get starting positions
	glm::vec2 Tracer((FirstBoundaryTileX - Position.x) * Ratio.x, (FirstBoundaryTileY - Position.y) * Ratio.y);

	// Traverse tiles
	if(CheckObjects)
		*HitEntity = nullptr;
	float MinDistance = HUGE_VAL;
	bool EndedOnX = false;
	while(TileTracer.x >= 0 && TileTracer.y >= 0 && TileTracer.x < Width && TileTracer.y < Height && CanShootThrough(TileTracer.x, TileTracer.y)) {

		// Check for object intersections
		if(CheckObjects) {
			for(auto Iterator = Data[TileTracer.x][TileTracer.y].Objects[GridType].begin(); Iterator != Data[TileTracer.x][TileTracer.y].Objects[GridType].end(); ++Iterator) {
				_Entity *Entity = static_cast<_Entity *>(*Iterator);
				if(!Entity->IsDying()) {
					float Distance = RayObjectIntersection(Position, Direction, Entity);
					if(Distance < MinDistance && Distance > 0.0f) {
						*HitEntity = Entity;
						MinDistance = Distance;
					}
				}
			}
		}

		// Determine which direction needs an update
		if(Tracer.x < Tracer.y) {
			Tracer.x += Increment.x;
			TileTracer.x += TileIncrementX;
			EndedOnX = true;
		}
		else {
			Tracer.y += Increment.y;
			TileTracer.y += TileIncrementY;
			EndedOnX = false;
		}
	}

	// An object was hit
	if(CheckObjects && *HitEntity != nullptr) {
		*HitPosition = Direction * MinDistance + Position;
		return;
	}

	// Determine which side has hit
	glm::vec2 WallHitPosition, WallBoundary;
	if(EndedOnX) {

		// Get correct side of the wall
		FirstBoundaryTileX = Direction.x < 0 ? TileTracer.x+1 : TileTracer.x;
		WallBoundary.x = FirstBoundaryTileX - Position.x;

		// Determine hit position
		WallHitPosition.x = WallBoundary.x;
		WallHitPosition.y = WallBoundary.x * Slope;
	}
	else {

		// Get correct side of the wall
		FirstBoundaryTileY = Direction.y < 0 ? TileTracer.y+1 : TileTracer.y;
		WallBoundary.y = FirstBoundaryTileY - Position.y;

		// Determine hit position
		WallHitPosition.x = WallBoundary.y / Slope;
		WallHitPosition.y = WallBoundary.y;
	}

	*HitPosition = WallHitPosition + Position;
	if(CheckObjects)
		*HitEntity = nullptr;
}

// Returns a t value for when a ray intersects a circle
float _Map::RayObjectIntersection(const glm::vec2 &Origin, const glm::vec2 &Direction, const _Object *Object) const {

	glm::vec2 Vector2EMinusC(Origin - Object->Position);
	float QuantityDDotD = glm::dot(Direction, Direction);
	float QuantityDDotEMC = glm::dot(Direction, Vector2EMinusC);
	float Discriminant = QuantityDDotEMC * QuantityDDotEMC - QuantityDDotD * (glm::dot(Vector2EMinusC, Vector2EMinusC) - Object->Radius * Object->Radius);
	if(Discriminant >= 0) {
		float ProductRayOMinusC = glm::dot(Direction * -1.0f, Vector2EMinusC);
		float SqrtDiscriminant = sqrt(Discriminant);

		float TMinus = (ProductRayOMinusC - SqrtDiscriminant) / QuantityDDotD;
		if(TMinus > 0)
			return TMinus;
		else
			return (ProductRayOMinusC + SqrtDiscriminant) / QuantityDDotD;
	}
	else
		return HUGE_VAL;
}

// Determines if two positions are mutually visible
bool _Map::IsVisible(const glm::vec2 &Start, const glm::vec2 &End) const {
	glm::vec2 Direction, Tracer, Increment, Ratio;
	int TileIncrementX, TileIncrementY, FirstBoundaryTileX, FirstBoundaryTileY, TileTracerX, TileTracerY;

	// Find starting and ending tiles
	_Coord StartTile = GetValidCoord(_Coord(Start));
	_Coord EndTile = GetValidCoord(_Coord(End));

	// Get direction
	Direction = End - Start;

	// Check degenerate cases
	if(!CanShootThrough(StartTile.x, StartTile.y) || !CanShootThrough(EndTile.x, EndTile.y))
		return false;

	// Only need to check vertical tiles
	if(StartTile.x == EndTile.x) {

		// Check degenerate cases
		if(StartTile.y == EndTile.y)
			return true;

		// Check direction
		if(Direction.y < 0) {
			for(int i = EndTile.y; i <= StartTile.y; i++) {
				if(!CanShootThrough(StartTile.x, i))
					return false;
			}
		}
		else {
			for(int i = StartTile.y; i <= EndTile.y; i++) {
				if(!CanShootThrough(StartTile.x, i))
					return false;
			}
		}
		return true;
	}
	else if(StartTile.y == EndTile.y) {

		// Check direction
		if(Direction.x < 0) {
			for(int i = EndTile.x; i <= StartTile.x; i++) {
				if(!CanShootThrough(i, StartTile.y))
					return false;
			}
		}
		else {
			for(int i = StartTile.x; i <= EndTile.x; i++) {
				if(!CanShootThrough(i, StartTile.y))
					return false;
			}
		}
		return true;
	}

	// Check x direction
	if(Direction.x < 0) {
		FirstBoundaryTileX = StartTile.x;
		TileIncrementX = -1;
	}
	else {
		FirstBoundaryTileX = StartTile.x + 1;
		TileIncrementX = 1;
	}

	// Check y direction
	if(Direction.y < 0) {
		FirstBoundaryTileY = StartTile.y;
		TileIncrementY = -1;
	}
	else {
		FirstBoundaryTileY = StartTile.y + 1;
		TileIncrementY = 1;
	}

	// Find ray direction ratios
	Ratio.x = 1.0f / Direction.x;
	Ratio.y = 1.0f / Direction.y;

	// Calculate increments
	Increment.x = TileIncrementX * Ratio.x;
	Increment.y = TileIncrementY * Ratio.y;

	// Get starting positions
	Tracer.x = (FirstBoundaryTileX - Start.x) * Ratio.x;
	Tracer.y = (FirstBoundaryTileY - Start.y) * Ratio.y;

	// Starting tiles
	TileTracerX = StartTile.x;
	TileTracerY = StartTile.y;

	// Traverse tiles
	while(true) {

		// Check for walls
		if(TileTracerX < 0 || TileTracerY < 0 || TileTracerX >= Width || TileTracerY >= Height || !CanShootThrough(TileTracerX, TileTracerY))
			return false;

		// Determine which direction needs an update
		if(Tracer.x < Tracer.y) {
			Tracer.x += Increment.x;
			TileTracerX += TileIncrementX;
		}
		else {
			Tracer.y += Increment.y;
			TileTracerY += TileIncrementY;
		}

		// Exit condition
		if((Direction.x < 0 && TileTracerX < EndTile.x)
			|| (Direction.x > 0 && TileTracerX > EndTile.x)
			|| (Direction.y < 0 && TileTracerY < EndTile.y)
			|| (Direction.y > 0 && TileTracerY > EndTile.y))
			break;
	}

	return true;
}

// Determines if a rectangle can travel to a position without hitting a wall
bool _Map::IsVisibleWithBounds(const glm::vec2 &Start, const glm::vec2 &End, float BoundSize) const {

	// Get x components of the starting corners
	glm::vec2 LeftStartPosition, RightStartPosition;
	LeftStartPosition.x = Start.x - BoundSize;
	RightStartPosition.x = Start.x + BoundSize;

	// Get direction
	glm::vec2 Direction(End - Start);

	// Get y components of the starting corners
	if((Direction.x < 0 && Direction.y < 0) || (Direction.x >= 0 && Direction.y >= 0)) {
		LeftStartPosition.y = Start.y + BoundSize;
		RightStartPosition.y = Start.y - BoundSize;
	}
	else {
		LeftStartPosition.y = Start.y - BoundSize;
		RightStartPosition.y = Start.y + BoundSize;
	}

	// Get ending positions
	glm::vec2 LeftEndPosition = LeftStartPosition + Direction;
	glm::vec2 RightEndPosition = RightStartPosition + Direction;

	// Check the first corner
	if(IsVisible(LeftStartPosition, LeftEndPosition)) {

		// No wall, so check the second corner
		if(IsVisible(RightStartPosition, RightEndPosition))
			return true;
		else
			return false;
	}
	else
		return false;
}

// Return an object at a given position
void _Map::GetSelectedObject(const glm::vec2 &Position, float RadiusSquared, _ObjectSpawn **Object, size_t *Index) {

	for(size_t i = 0; i < ObjectSpawns.size(); i++) {

		// Circle test
		if(glm::distance2(ObjectSpawns[i]->Position, Position) < RadiusSquared) {
			*Object = ObjectSpawns[i];
			*Index = i;
			return;
		}
	}

	*Object = nullptr;
}

// Returns all the objects that fall inside the rectangle
void _Map::GetSelectedObjects(const glm::vec2 &Start, const glm::vec2 &End, std::list<_ObjectSpawn *> *SelectedObjects, std::list<size_t> *SelectedObjectIndices) {

	glm::vec2 StartPoint, EndPoint;
	if(End.x < Start.x) {
		StartPoint.x = End.x;
		EndPoint.x = Start.x;
	}
	else {
		StartPoint.x = Start.x;
		EndPoint.x = End.x;
	}

	if(End.y < Start.y) {
		StartPoint.y = End.y;
		EndPoint.y = Start.y;
	}
	else {
		StartPoint.y = Start.y;
		EndPoint.y = End.y;
	}

	for(size_t i = 0; i < ObjectSpawns.size(); i++) {

		if(ObjectSpawns[i]->Position.x > StartPoint.x && ObjectSpawns[i]->Position.y > StartPoint.y && ObjectSpawns[i]->Position.x <= EndPoint.x && ObjectSpawns[i]->Position.y <= EndPoint.y) {
			SelectedObjects->push_back(ObjectSpawns[i]);
			SelectedObjectIndices->push_back(i);
		}
	}

}

// Removes a block from the list
void _Map::RemoveBlock(int Layer, int Index) {

	if(Index >= 0 && Index < (int)Blocks[Layer].size()) {
		DeleteBlockIDFromTiles(Layer, Index);
		Blocks[Layer].erase(Blocks[Layer].begin() + Index);
	}
}

// Deletes a block id from the events list given a block id and layer
void _Map::DeleteBlockIDFromTiles(int Layer, int Index) {

	for(size_t i = 0; i < Events.size(); i++)
		Events[i]->DeleteBlockID(Layer, Index);
}
// Removes an event from the list
void _Map::RemoveEvent(int Index) {

	if(Index >= 0 && Index < (int)Events.size()) {
		DeleteBlockIDFromTiles(-1, Index);
		delete Events[Index];
		Events.erase(Events.begin() + Index);
	}
}

// Removes objects from the list
void _Map::RemoveObjects(std::list<size_t> &SelectedObjectIndices) {
	SelectedObjectIndices.sort();
	for(std::list<size_t>::reverse_iterator Iterator = SelectedObjectIndices.rbegin(); Iterator != SelectedObjectIndices.rend(); ++Iterator)
		ObjectSpawns.erase(ObjectSpawns.begin() + *Iterator);
}

// Return the block at a given position
int _Map::GetSelectedBlock(int Layer, const _Coord &Index) {

	for(int i = static_cast<int>(Blocks[Layer].size())-1; i >= 0; i--) {
		if(Index.x >= Blocks[Layer][i].Start.x && Index.y >= Blocks[Layer][i].Start.y && Index.x <= Blocks[Layer][i].End.x && Index.y <= Blocks[Layer][i].End.y)
			return i;
	}

	return -1;
}

// Return the block at a given position
int _Map::GetSelectedBlock(int Layer, const _Coord &Index, _Block **Block) {

	int BlockIndex = GetSelectedBlock(Layer, Index);
	if(BlockIndex != -1) {
		*Block = &Blocks[Layer][BlockIndex];
		return BlockIndex;
	}

	*Block = nullptr;
	return -1;
}

// Returns a block by its layer and index
const _Block *_Map::GetBlock(int Layer, const size_t Index) const {
	if(Layer < 0 || Layer >= MAPLAYER_COUNT || Index >= Blocks[Layer].size())
		return nullptr;

	return &Blocks[Layer][Index];
}

// Gets the last block in the list
int _Map::GetLastBlock(int Layer, _Block **Block) {
	if(Blocks[Layer].size() > 0) {
		*Block = &Blocks[Layer][Blocks[Layer].size() - 1];
		return Blocks[Layer].size() - 1;
	}

	*Block = nullptr;
	return -1;
}

// Get the size of a layer
int _Map::GetLayerSize(int Index) {
	if(Index < 0 || Index >= MAPLAYER_COUNT)
		return -1;

	return Blocks[Index].size();
}

// Toggles an event's active state
void _Map::ToggleEventActive(int Index) {
	if(Index >= 0 && Index < (int)Events.size())
		Events[Index]->Active = !Events[Index]->Active;
}

// Gets an event
_Event *_Map::GetEvent(int Index) const {

	return Events[Index];
}

// Determines if a tile has any events
bool _Map::HasEvents(const _Coord &Position) const {
	if(!Data)
		throw std::runtime_error("Tile data uninitialized!");

	return Data[Position.x][Position.y].Events.size() > 0;
}

// Gets a list of event based on a position
std::list<_Event *> &_Map::GetEventList(const _Coord &Position) {
	if(!Data)
		throw std::runtime_error("Tile data uninitialized!");

	return Data[Position.x][Position.y].Events;
}

// Returns a starting position by level and player id
glm::vec2 _Map::GetStartingPositionByCheckpoint(int Level) {

	// Degenerate case
	if(CheckpointEvents.size() == 0)
		return glm::vec2(2.5f, 2.5f);

	// Look through events
	for(size_t i = 0; i < CheckpointEvents.size(); i++) {
		_Event *Event = CheckpointEvents[i];
		if(Event->Level == Level) {
			const std::vector<_EventTile> &Tiles = Event->Tiles;

			if(Tiles.size() == 0)
				return glm::vec2(Event->Start.x + 0.5f, Event->Start.y + 0.5f);
			else
				return glm::vec2(Tiles[0].Coord.x + 0.5f, Tiles[0].Coord.y + 0.5f);
		}
	}

	return glm::vec2(2.5f, 2.5f);
}

// Return the event at a given position
int _Map::GetSelectedEvent(const _Coord &Index, _Event **ReturnEvent) {

	// Loop through events
	for(auto Iterator = Events.rbegin(); Iterator != Events.rend(); ++Iterator) {
		_Event *Event = *Iterator;

		if(Index.x >= Event->Start.x && Index.y >=Event->Start.y && Index.x <= Event->End.x && Index.y <= Event->End.y) {
			*ReturnEvent = Event;
			return Events.size() - 1 - (Iterator - Events.rbegin());
		}
	}

	*ReturnEvent = nullptr;
	return -1;
}

// Changes the layer that block is in
void _Map::ChangeLayer(int OldLayer, int NewLayer, int Index) {

	// Delete block ids from events
	DeleteBlockIDFromTiles(OldLayer, Index);

	// Add new block
	Blocks[NewLayer].push_back(Blocks[OldLayer][Index]);
	Blocks[OldLayer].erase(Blocks[OldLayer].begin() + Index);

}

// Draws a grid on the map
void _Map::RenderGrid(int Mode) {
	if(Mode > 0) {

		// Draw vertical lines
		for(int i = Mode; i < Width; i += Mode)
			Graphics.DrawLine((float)i, 0, (float)i, (float)Height, COLOR_TWHITE);

		// Draw horizontal lines
		for(int i = Mode; i < Height; i += Mode)
			Graphics.DrawLine(0, (float)i, (float)Width, (float)i, COLOR_TWHITE);
	}
}

// Draws rectangles around all the blocks
void _Map::HighlightBlocks(int Layer) {
	for(int i = 0; i < static_cast<int>(Blocks[Layer].size()); i++) {
		Graphics.DrawRectangle((float)Blocks[Layer][i].Start.x, (float)Blocks[Layer][i].Start.y, (float)Blocks[Layer][i].End.x + 1.0f, (float)Blocks[Layer][i].End.y + 1.0f, COLOR_MAGENTA);
	}
}

// Returns the total number of blocks
int _Map::GetTotalBlockSize() const {
	int Sum = 0;

	for(int i = 0; i < MAPLAYER_COUNT; i++)
		Sum += Blocks[i].size();

	return Sum;
}

// Returns a valid position on the map
glm::vec2 _Map::GetValidPosition(const glm::vec2 &Position) const {
	glm::vec2 NewPosition;

	if(Position.x <= 0)
		NewPosition.x = 0;
	else if(Position.x >= Width - MAP_EPSILON)
		NewPosition.x = Width - MAP_EPSILON;
	else
		NewPosition.x = Position.x;

	if(Position.y <= 0)
		NewPosition.y = 0;
	else if(Position.y >= Height - MAP_EPSILON)
		NewPosition.y = Height - MAP_EPSILON;
	else
		NewPosition.y = Position.y;

	return NewPosition;
}

// Opens a door or hits a floor switch
void _Map::ChangeMapState(const _Event *Event) {
	if(!Data)
		throw std::runtime_error("Tile data uninitialized!");

	// Check for the proper event
	if(Event->Type == EVENT_DOOR || Event->Type == EVENT_WSWITCH || Event->Type == EVENT_FSWITCH) {
		const std::vector<_EventTile> &Tiles = Event->Tiles;

		// Switch the texture of the first block for wall switches
		int StartIndex = 0;
		if(Event->Type == EVENT_WSWITCH && Tiles.size() > 0 && Tiles[0].BlockID != -1) {
			SwapBlockTextures(Tiles[0].Layer, Tiles[0].BlockID);
			StartIndex = 1;
		}

		// Change all the tiles
		for(size_t i = StartIndex; i < Tiles.size(); i++) {
			_Tile *Tile = &Data[Tiles[i].Coord.x][Tiles[i].Coord.y];

			Tile->Collision ^= _Tile::ENTITY;

			// Switch textures
			SwapBlockTextures(Tiles[i].Layer, Tiles[i].BlockID);
		}
	}
}

// Determines if the map state can be changed
bool _Map::CanChangeMapState(const _Event *Event) {
	if(!Data)
		throw std::runtime_error("Tile data uninitialized!");

	// Check for the proper event
	const std::vector<_EventTile> &Tiles = Event->Tiles;

	// Check for objects in the wall
	for(size_t i = 0; i < Tiles.size(); i++) {
		_Tile *Tile = &Data[Tiles[i].Coord.x][Tiles[i].Coord.y];
		if(Tile->CanWalk() && (Tile->Objects[GRID_PLAYER].size() > 0 || Tile->Objects[GRID_MONSTER].size() > 0))
			return false;
	}

	return true;
}

// Swaps a block's texture with its alternate texture
void _Map::SwapBlockTextures(int Layer, int Index) {

	if(Index != -1) {
		_Block *Block = &Blocks[Layer][Index];
		if(Block->AltTexture) {
			std::swap(Block->Texture, Block->AltTexture);
		}
	}
}

// Renders the floor
void _Map::RenderFloors() {
	if(!Camera)
		return;

	// Draw base layer
	Graphics.SetDepthMask(false);
	for(int i = 0; i < static_cast<int>(Blocks[0].size()); i++) {
		_Block *Block = &Blocks[0][i];
		bool Draw = true;
		if(Block->MinZ >= 0) {
			float Bounds[4] = { (float)Block->Start.x, (float)Block->Start.y, (float)Block->End.x + 1.0f, (float)Block->End.y + 1.0f };
			Draw = Camera->IsAABBInView(Bounds);
		}

		if(Draw)
			Graphics.DrawRepeatable((float)Block->Start.x, (float)Block->Start.y, (float)Block->MinZ + MAP_LAYEROFFSET * i, (float)Block->End.x + 1.0f, (float)Block->End.y + 1.0f, Block->MinZ + MAP_LAYEROFFSET * i, Block->Texture, Block->Rotation, Block->ScaleX);
	}
	Graphics.SetDepthMask(true);

	// Draw floor layers 0-2
	for(int i = MAPLAYER_FLOOR0; i <= MAPLAYER_FLOOR2; i++) {

		for(int j = 0; j < static_cast<int>(Blocks[i].size()); j++) {
			_Block *Block = &Blocks[i][j];

			if(Block->MinZ == Block->MaxZ) {

				bool Draw = true;
				if(Block->MinZ >= 0) {
					float Bounds[4] = { (float)Block->Start.x, (float)Block->Start.y, (float)Block->End.x + 1.0f, (float)Block->End.y + 1.0f };
					Draw = Camera->IsAABBInView(Bounds);
				}

				if(Draw)
					Graphics.DrawRepeatable((float)Block->Start.x, (float)Block->Start.y, (float)Block->MinZ + MAP_LAYEROFFSET * i, (float)Block->End.x + 1.0f, (float)Block->End.y + 1.0f, Block->MinZ + MAP_LAYEROFFSET * i, Block->Texture, Block->Rotation, Block->ScaleX);
			}
			else {
				Graphics.EnableVBO(VBO_CUBE);
				Graphics.DrawCube(
					(float)Block->Start.x,
					(float)Block->Start.y,
					(float)Block->MinZ,
					(float)Block->End.x - Block->Start.x + 1.0f,
					(float)Block->End.y - Block->Start.y + 1.0f,
					(float)Block->MaxZ - Block->MinZ,
					Block->Texture);
				Graphics.DisableVBO(VBO_CUBE);
			}
		}
	}
}

// Renders the walls
void _Map::RenderWalls() {
	if(!Camera)
		return;

	Graphics.EnableVBO(VBO_CUBE);

	// Draw walls
	for(int i = 0; i < static_cast<int>(Blocks[5].size()); i++) {
		_Block *Block = &Blocks[5][i];

		bool Draw = true;
		if(Block->MinZ >= 0) {
			float Bounds[4] = { (float)Block->Start.x, (float)Block->Start.y, (float)Block->End.x + 1.0f, (float)Block->End.y + 1.0f };
			Draw = Camera->IsAABBInView(Bounds);
		}
		if(Draw)
			Graphics.DrawCube((float)Block->Start.x, (float)Block->Start.y, (float)Block->MinZ, (float)Block->End.x - Block->Start.x + 1.0f, (float)Block->End.y - Block->Start.y + 1.0f, Block->MaxZ - Block->MinZ, Block->Texture);
	}

	// Draw flat walls
	Graphics.SetDepthMask(false);
	for(int i = 0; i < static_cast<int>(Blocks[4].size()); i++) {
		_Block *Block = &Blocks[4][i];
		bool Draw = true;
		if(Block->MinZ >= 0) {
			float Bounds[4] = { (float)Block->Start.x, (float)Block->Start.y + 0.0f, (float)Block->End.x + 1.0f, (float)Block->End.y + 1.0f };
			Draw = Camera->IsAABBInView(Bounds);
		}

		if(Draw)
			Graphics.DrawWall((float)Block->Start.x, (float)Block->Start.y, (float)Block->MinZ, (float)Block->End.x - Block->Start.x + 1.0f, (float)Block->End.y - Block->Start.y + 1.0f, Block->MaxZ - Block->MinZ, Block->Rotation, Block->Texture);
	}
	Graphics.SetDepthMask(true);

	Graphics.DisableVBO(VBO_CUBE);
}

// Draws the events
void _Map::RenderEvents(std::vector<_Texture *> &Textures) {
	if(!Camera)
		return;

	Graphics.DisableDepthTest();

	// Draw events
	for(size_t i = 0; i < Events.size(); i++) {
		float Bounds[4] = { (float)Events[i]->Start.x,(float) Events[i]->Start.y, (float)Events[i]->End.x + 1.0f, (float)Events[i]->End.y + 1.0f };
		if(Camera->IsAABBInView(Bounds))
			Graphics.DrawRepeatable((float)Events[i]->Start.x, (float)Events[i]->Start.y, MAP_LAYEROFFSET, (float)Events[i]->End.x + 1.0f, (float)Events[i]->End.y + 1.0f, MAP_LAYEROFFSET, Textures[Events[i]->Type], 0, 1.0f);
	}

	Graphics.EnableDepthTest();
}

// Renders the foreground tiles
void _Map::RenderForeground() {
	if(!Camera)
		return;

	// Draw foreground
	for(int i = 0; i < static_cast<int>(Blocks[6].size()); i++) {
		_Block *Block = &Blocks[6][i];
		bool Draw = true;
		if(Block->MinZ >= 0) {
			float Bounds[4] = { (float)Block->Start.x, (float)Block->Start.y, (float)Block->End.x + 1.0f, (float)Block->End.y + 1.0f };
			Draw = Camera->IsAABBInView(Bounds);
		}

		if(Draw)
			Graphics.DrawRepeatable((float)Block->Start.x, (float)Block->Start.y, (float)Block->MaxZ + 0.01f * i, (float)Block->End.x + 1.0f, (float)Block->End.y + 1.0f, Block->MaxZ + 0.01f * i, Block->Texture, Block->Rotation, Block->ScaleX);
	}
}

// Update map
void _Map::Update(double FrameTime) {
	ObjectManager->Update(FrameTime, Camera);
	if(AmbientLightPeriod > 0 && AmbientLightTimer <= AmbientLightPeriod) {
		AmbientLightBlendFactor = AmbientLightTimer / AmbientLightPeriod;
		AmbientLightTimer += FrameTime;
	}
	else
		AmbientLightBlendFactor = 1.0;
}

// Render entities and items
void _Map::RenderObjects(double BlendFactor) {
	ObjectManager->Render(BlendFactor);
}

// Adds an item to the item list and collision grid
void _Map::AddItem(_Item *Item) {
	ObjectManager->AddObject(Item);
	AddObjectToGrid(Item, GRID_ITEM);
}

// Removes an item from object list and collision grid
void _Map::RemoveItem(_Item *Item) {
	ObjectManager->RemoveObject(Item);
	RemoveObjectFromGrid(Item, GRID_ITEM);
}

void _Map::AddRenderList(_Object *Object, int Layer) {
	ObjectManager->AddRenderList(Object, Layer);
}
