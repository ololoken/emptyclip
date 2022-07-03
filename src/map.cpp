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
#include <objects/entity.h>
#include <objects/item.h>
#include <objects/particle.h>
#include <ae/random.h>
#include <ae/camera.h>
#include <ae/texture.h>
#include <ae/graphics.h>
#include <ae/assets.h>
#include <ae/program.h>
#include <ae/bounds.h>
#include <gameassets.h>
#include <events.h>
#include <stats.h>
#include <objectmanager.h>
#include <constants.h>
#include <fstream>
#include <stdexcept>
#include <iomanip>
#include <iostream>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <zlib/zfstream.h>

// Initialize
_Map::_Map() :
	MapType(MAPTYPE_CAMPAIGN),
	Width(MAP_WIDTH),
	Height(MAP_HEIGHT),
	Level(1),
	Camera(nullptr),
	ObjectManager(new _ObjectManager()),
	MinimapCaptureSize(HUD_MINIMAP_CAPTURE_SIZE),
	Data(nullptr),
	AmbientLight(0.5f, 0.5f, 0.5f, 1.0f),
	OldAmbientLight(0.5f, 0.5f, 0.5f, 1.0f),
	AmbientLightBlendFactor(1.0f),
	AmbientLightPeriod(0.0),
	AmbientLightTimer(0.0) {

}

// Initialize
_Map::_Map(const std::string &Filename) : _Map() {
	if(Filename == "")
		throw std::runtime_error("Empty file name");

	this->Filename = FixFilename(Filename);

	// Load file
	gzifstream InputFile(("maps/" + this->Filename).c_str(), std::ios::in);
	if(!InputFile)
		throw std::runtime_error("Cannot load file: " + this->Filename);

	// Get file version
	int FileVersion;
	InputFile >> FileVersion;
	if(FileVersion != MAP_FILEVERSION)
		throw std::runtime_error("Level version mismatch: ");

	// Level used for default item/monster levels
	InputFile >> Level;

	// Get map type
	InputFile >> MapType;

	// Read dimensions
	InputFile >> Width >> Height;

	// Load objects
	size_t ObjectCount;
	InputFile >> ObjectCount;
	for(size_t i = 0; i < ObjectCount; i++) {

		// Load Data
		_ObjectSpawn *Object = new _ObjectSpawn();
		InputFile >> Object->ID >> Object->Level >> Object->Position.x >> Object->Position.y;

		// Check for object
		if(Stats.Objects.find(Object->ID) == Stats.Objects.end())
			throw std::runtime_error(std::string(__func__) + " Unknown object '" + Object->ID + "'");

		Object->Type = Stats.Objects.at(Object->ID).Type;
		ObjectSpawns.push_back(Object);
	}

	// Read events count
	size_t EventCount;
	InputFile >> EventCount;

	// Load events
	for(size_t i = 0; i < EventCount; i++) {

		int EventType;
		int EventActive;
		int EventLevel;
		int EventSpawnLevel;
		glm::ivec2 EventStart;
		glm::ivec2 EventEnd;
		double EventActivationPeriod;
		size_t TilesSize;
		InputFile
				>> EventType
				>> EventActive
				>> EventStart.x
				>> EventStart.y
				>> EventEnd.x
				>> EventEnd.y
				>> EventLevel
				>> EventSpawnLevel
				>> EventActivationPeriod
				>> TilesSize;

		InputFile.ignore(std::numeric_limits<std::streamsize>::max(), ' ');
		std::string EventItemID;
		std::string EventMonsterID;
		std::string EventParticleID;
		std::getline(InputFile, EventItemID, '\t');
		std::getline(InputFile, EventMonsterID, '\t');
		std::getline(InputFile, EventParticleID, '\n');

		// Check for existence
		if(EventMonsterID != "" && Stats.Objects.find(EventMonsterID) == Stats.Objects.end())
			throw std::runtime_error("Cannot find monster: " + EventMonsterID);
		if(EventParticleID != "" && !GameAssets.IsParticleLoaded(EventParticleID))
			throw std::runtime_error("Cannot find particle: " + EventParticleID);

		_Event *Event = new _Event(EventType, EventActive, EventStart, EventEnd, EventLevel, EventSpawnLevel, EventActivationPeriod, EventItemID, EventMonsterID, EventParticleID);
		for(size_t j = 0; j < TilesSize; j++) {
			glm::ivec2 Tile;
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
		InputFile
			>> Layer
			>> Block.Start.x
			>> Block.Start.y
			>> Block.End.x
			>> Block.End.y
			>> Block.MinZ
			>> Block.MaxZ
			>> Block.Rotation
			>> Block.ScaleX
			>> Block.Wall
			>> Block.Walkable;

		InputFile.ignore(std::numeric_limits<std::streamsize>::max(), ' ');
		std::string TexturePath;
		std::string AltTexturePath;
		std::getline(InputFile, TexturePath, '\t');
		std::getline(InputFile, AltTexturePath, '\n');

		Block.Texture = ae::Assets.Textures[TexturePath];
		if(!Block.Texture)
			throw std::runtime_error("Cannot find texture: " + TexturePath);

		if(AltTexturePath != "") {
			Block.AltTexture = ae::Assets.Textures[AltTexturePath];
			if(!Block.AltTexture)
				throw std::runtime_error("Cannot find alt texture: " + AltTexturePath);
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

	// Delete particles
	for(const auto &Particle : Particles)
		delete Particle;

	if(Data != nullptr) {
		for(int i = 0; i < Width; i++)
			delete[] Data[i];
		delete[] Data;
	}
}

// Create tile data
void _Map::InitializeTiles() {

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
bool _Map::Save(const std::string &String) {

	Filename = FixFilename(String);
	gzofstream Output(("maps/" + Filename).c_str(), std::ios::out);
	if(!Output)
		throw std::runtime_error("Cannot create file: " + Filename);

	Output << std::showpoint << std::fixed << std::setprecision(2);

	// Header
	Output
		<< MAP_FILEVERSION << '\n'
		<< Level << '\n'
		<< MapType << '\n'
		<< Width << ' ' << Height << '\n';

	// Objects
	Output << ObjectSpawns.size() << '\n';
	for(size_t i = 0; i < ObjectSpawns.size(); i++) {
		Output
			<< ObjectSpawns[i]->ID << ' '
			<< ObjectSpawns[i]->Level << ' '
			<< ObjectSpawns[i]->Position.x << ' '
			<< ObjectSpawns[i]->Position.y
			<< '\n';
	}

	// Events
	Output << Events.size() << '\n';
	for(size_t i = 0; i < Events.size(); i++) {
		Output
			<< Events[i]->Type << ' '
			<< Events[i]->Active << ' '
			<< Events[i]->Start.x << ' '
			<< Events[i]->Start.y << ' '
			<< Events[i]->End.x << ' '
			<< Events[i]->End.y << ' '
			<< Events[i]->Level << ' '
			<< Events[i]->SpawnLevel << ' '
			<< Events[i]->ActivationPeriod << ' '
			<< Events[i]->Tiles.size() << ' '
			<< Events[i]->ItemID << '\t'
			<< Events[i]->MonsterID << '\t'
			<< Events[i]->ParticleID << '\n';

		// Write tiles
		for(size_t j = 0; j < Events[i]->Tiles.size(); j++)
			Output << Events[i]->Tiles[j].Coord.x << ' ' << Events[i]->Tiles[j].Coord.y << ' ' << Events[i]->Tiles[j].Layer << ' ' << Events[i]->Tiles[j].BlockID << '\n';
	}

	// Blocks
	Output << GetTotalBlockSize() << '\n';
	for(int i = 0; i < MAPLAYER_COUNT; i++) {
		for(size_t j = 0; j < Blocks[i].size(); j++) {

			std::string AltTextureID;
			if(Blocks[i][j].AltTexture)
				AltTextureID = Blocks[i][j].AltTexture->Name;

			Output
				<< i << ' '
				<< Blocks[i][j].Start.x << ' '
				<< Blocks[i][j].Start.y << ' '
				<< Blocks[i][j].End.x << ' '
				<< Blocks[i][j].End.y << ' '
				<< Blocks[i][j].MinZ << ' '
				<< Blocks[i][j].MaxZ << ' '
				<< Blocks[i][j].Rotation << ' '
				<< Blocks[i][j].ScaleX << ' '
				<< Blocks[i][j].Wall << ' '
				<< Blocks[i][j].Walkable << ' '
				<< Blocks[i][j].Texture->Name << '\t'
				<< AltTextureID << '\n';
		}
	}

	Output.close();

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
bool _Map::CheckTileCollisions(const glm::vec2 &TargetPosition, float Radius, glm::vec2 &NewPosition) {
	if(!Data)
		throw std::runtime_error("Tile data uninitialized!");

	NewPosition = TargetPosition;
	float Left = NewPosition.x - Radius;
	float Right = NewPosition.x + Radius;
	float Top = NewPosition.y - Radius;
	float Bottom = NewPosition.y + Radius;

	// Check boundaries
	bool Touching = false;
	if(Left < 0) {
		Left = NewPosition.x = Radius;
		Touching = true;
	}
	if(Top < 0) {
		Top = NewPosition.y = Radius;
		Touching = true;
	}
	if(Right >= (float)Width) {
		Right = NewPosition.x = (float)Width - Radius;
		Touching = true;
	}
	if(Bottom >= (float)Height) {
		Bottom = NewPosition.y = (float)Height - Radius;
		Touching = true;
	}

	// Check tiles
	int LeftTile = (int)Left;
	int RightTile = (int)Right;
	int TopTile = (int)Top;
	int BottomTile = (int)Bottom;

	std::vector<glm::vec2> Pushes;
	bool AxisAlignedPush = false;
	for(int i = LeftTile; i <= RightTile; i++) {
		for(int j = TopTile; j <= BottomTile; j++) {
			if(Data[i][j].CanWalk())
				continue;

			float AABB[4] = { (float)i, (float)j, i + 1.0f, j + 1.0f };
			_Hit Hit;
			Hit.AxisAlignedPush = false;
			if(CheckAABBCollision(NewPosition, Radius, AABB, true, Hit)) {
				Touching = true;
				Pushes.push_back(Hit.Push);

				// Flag at least one axis aligned push
				if(Hit.AxisAlignedPush)
					AxisAlignedPush = true;
			}
		}
	}

	// Resolve collision
	for(const auto &Push : Pushes) {
		if(!(AxisAlignedPush && Push.x != 0 && Push.y != 0)) {
			NewPosition += Push;
		}
	}

	return Touching;
}

// Resolve collision with an axis aligned bounding box
bool _Map::CheckAABBCollision(const glm::vec2 &Position, float Radius, const float *AABB, bool Resolve, _Hit &Hit) const {
	int ClampCount = 0;

	// Get closest point on AABB
	glm::vec2 ClosetPoint = Position;
	if(ClosetPoint.x < AABB[0]) {
		ClosetPoint.x = AABB[0];
		ClampCount++;
	}
	if(ClosetPoint.y < AABB[1]) {
		ClosetPoint.y = AABB[1];
		ClampCount++;
	}
	if(ClosetPoint.x > AABB[2]) {
		ClosetPoint.x = AABB[2];
		ClampCount++;
	}
	if(ClosetPoint.y > AABB[3]) {
		ClosetPoint.y = AABB[3];
		ClampCount++;
	}

	// Test circle collision with point
	float DistanceSquared = glm::distance2(ClosetPoint, Position);
	bool Touching = DistanceSquared < Radius * Radius;

	// Push object out
	if(Touching && Resolve) {

		// Check if object is inside the AABB
		if(ClampCount == 0) {
			glm::vec2 Center((AABB[0] + AABB[2]) * 0.5f, (AABB[1] + AABB[3]) * 0.5f);
			if(Position.x <= Center.x)
				Hit.Push.x = -(AABB[0] - Position.x - Radius);
			else if(Position.x > Center.x)
				Hit.Push.x = (AABB[0] - Position.x) + 1 + Radius;

			Hit.Push.y = 0.0f;
		}
		else {

			// Get push direction
			Hit.Push = Position - ClosetPoint;

			// Get push amount
			float Amount = Radius - glm::length(Hit.Push);

			// Scale push vector
			Hit.Push = glm::normalize(Hit.Push);
			Hit.Push *= Amount;

			// Flag axis aligned pushes
			if(ClampCount == 1)
				Hit.AxisAlignedPush = true;
		}
	}

	return Touching;
}

// Get the first object that collides with a circle
_Object *_Map::GetCloseObject(const glm::vec2 &Position, float Radius, int GridType) const {
	if(!Data)
		throw std::runtime_error("Tile data uninitialized!");

	// Get the object's bounding rectangle
	_TileBounds TileBounds;
	GetTileBounds(Position, Radius, TileBounds);

	// Iterate through tiles covered by the bounds
	for(int i = TileBounds.Start.x; i <= TileBounds.End.x; i++) {
		for(int j = TileBounds.Start.y; j <= TileBounds.End.y; j++) {
			for(auto Iterator : Data[i][j].Objects[GridType]) {

				// Check circle intersection
				float RadiiSum = Iterator->Radius + Radius;
				if(glm::distance2(Iterator->Position, Position) < RadiiSum * RadiiSum)
					return Iterator;
			}
		}
	}

	return nullptr;
}

// Return objects that are touching a circle
void _Map::GetCloseObjects(const glm::vec2 &Position, float Radius, int GridType, std::unordered_map<_Object *, int> &TouchedObjects) const {
	if(!Data)
		throw std::runtime_error("Tile data uninitialized!");

	// Get bounding rectangle
	_TileBounds TileBounds;
	GetTileBounds(Position, Radius, TileBounds);

	// Iterate through tiles covered by the bounds
	for(int i = TileBounds.Start.x; i <= TileBounds.End.x; i++) {
		for(int j = TileBounds.Start.y; j <= TileBounds.End.y; j++) {
			for(auto Iterator : Data[i][j].Objects[GridType]) {
				float RadiiSum = Iterator->Radius + Radius;
				if(glm::distance2(Iterator->Position, Position) < RadiiSum * RadiiSum)
					TouchedObjects[Iterator] = 1;
			}
		}
	}
}

// Returns a list of entities that an object is colliding with
void _Map::CheckEntityCollisionsInGrid(const glm::vec2 &Position, float Radius, const _Object *SkipObject, std::vector<_Hit> &Hits, bool &AxisAlignedPush) const {
	if(!Data)
		throw std::runtime_error("Tile data uninitialized!");

	// Get the object's bounding rectangle
	_TileBounds TileBounds;
	GetTileBounds(Position, Radius, TileBounds);

	// Get unique list of objects to check against
	std::unordered_map<_Entity *, int> HitEntities;
	for(int i = TileBounds.Start.x; i <= TileBounds.End.x; i++) {
		for(int j = TileBounds.Start.y; j <= TileBounds.End.y; j++) {
			for(int k = 0; k < 2; k++) {
				for(auto Iterator = Data[i][j].Objects[k].begin(); Iterator != Data[i][j].Objects[k].end(); ++Iterator) {
					_Entity *Entity = (_Entity *)*Iterator;
					if(Entity == SkipObject || Entity->IsDying())
						continue;

					HitEntities[Entity] = 1;
				}
			}
		}
	}

	// Get push vectors for each hit object
	for(const auto &HitEntity : HitEntities) {
		_Entity *Entity = HitEntity.first;
		if(Entity->Circle) {
			float DistanceSquared = glm::distance2(Entity->Position, Position);
			float RadiiSum = Entity->Radius + Radius;

			// Check circle intersection
			if(DistanceSquared < RadiiSum * RadiiSum) {
				glm::vec2 CenterVector = Position - Entity->Position;

				_Hit Hit;
				if(CenterVector.x == 0.0f && CenterVector.y == 0.0f) {
					Hit.Push.x = 1.0f;
					Hit.Push.y = 0.0f;
				}
				else {
					Hit.Push = glm::normalize(CenterVector);
					Hit.Push *= RadiiSum - sqrtf(DistanceSquared);
				}
				Hits.push_back(Hit);
			}
		}
		else {

			// Get AABB of object
			float AABB[4] = {
				Entity->Position.x - Entity->Radius,
				Entity->Position.y - Entity->Radius,
				Entity->Position.x + Entity->Radius,
				Entity->Position.y + Entity->Radius
			};

			_Hit Hit;
			Hit.AxisAlignedPush = false;
			if(CheckAABBCollision(Position, Radius, AABB, true, Hit)) {
				Hits.push_back(Hit);
				if(Hit.AxisAlignedPush)
					AxisAlignedPush = true;
			}
		}
	}
}

// Checks for melee collisions with entities in the collision grid
void _Map::CheckMeleeCollisions(_Entity *Attacker, const glm::vec2 &Direction, int GridType, int Penetration, std::vector<_Hit> &Hits) const {
	if(!Data)
		throw std::runtime_error("Tile data uninitialized!");

	// Get the object's bounding rectangle
	_TileBounds TileBounds;
	GetTileBounds(Attacker->Position, Attacker->AttackRange[Attacker->AttackRequestType], TileBounds);
	for(int i = TileBounds.Start.x; i <= TileBounds.End.x; i++) {
		for(int j = TileBounds.Start.y; j <= TileBounds.End.y; j++) {
			for(auto Iterator = Data[i][j].Objects[GridType].begin(); Iterator != Data[i][j].Objects[GridType].end(); ++Iterator) {
				_Entity *Entity = (_Entity *)*Iterator;
				if(Entity->IsDying())
					continue;

				float DistanceSquared = glm::distance2(Entity->Position, Attacker->Position);
				float RadiiSum = Entity->Radius + Attacker->AttackRange[Attacker->AttackRequestType];

				// Check circle intersection
				if(DistanceSquared >= RadiiSum * RadiiSum)
					continue;

				glm::vec2 ObjectDirection(glm::normalize(Entity->Position - Attacker->Position));

				// Compare angles
				if(glm::dot(Direction, ObjectDirection) > cosf(glm::radians(Attacker->MaxAccuracy[Attacker->AttackRequestType] * 0.5f))) {

					// Check for walls
					if(IsVisible(Attacker->Position, Entity->Position)) {
						_Hit Hit(HIT_OBJECT);
						Hit.Object = Entity;
						Hit.Position = Entity->Position;
						Hits.push_back(Hit);

						Penetration--;
						if(Penetration <= 0)
							return;
					}
				}
			}
		}
	}

	return;
}

// Determines which walls are adjacent to the object
int _Map::GetWallState(const glm::vec2 &Position, float Radius) const {
	if(!Data)
		throw std::runtime_error("Tile data uninitialized!");

	// Get the tile the object is standing on
	glm::ivec2 TileCoord = GetValidCoord(Position);
	int WallState = 0;

	// Check left wall
	glm::ivec2 TopLeft = GetValidCoord(glm::ivec2(Position.x - Radius - MAP_EPSILON, Position.y - Radius - MAP_EPSILON));
	if(!Data[TopLeft.x][TileCoord.y].CanWalk())
		WallState |= WALL_LEFT;

	// Check top wall
	if(!Data[TileCoord.x][TopLeft.y].CanWalk())
		WallState |= WALL_TOP;

	// Check right wall
	glm::ivec2 BottomRight = GetValidCoord(glm::ivec2(Position.x + Radius + MAP_EPSILON, Position.y + Radius + MAP_EPSILON));
	if(!Data[BottomRight.x][TileCoord.y].CanWalk())
		WallState |= WALL_RIGHT;

	// Check bottom wall
	if(!Data[TileCoord.x][BottomRight.y].CanWalk())
		WallState |= WALL_BOTTOM;

	return WallState;
}

// Determines what adjacent square the object is facing
void _Map::GetAdjacentTile(const glm::vec2 &Position, float Direction, glm::ivec2 &Coord) const {

	// Check direction
	if(Direction > 45.0f && Direction < 135.0f) {
		Coord = GetValidCoord(glm::ivec2(Position.x + 1.0f, Position.y));
	}
	else if(Direction >= 135.0f && Direction < 225.0f) {
		Coord = GetValidCoord(glm::ivec2(Position.x, Position.y + 1.0f));
	}
	else if(Direction >= 225.0f && Direction < 315.0f) {
		Coord = GetValidCoord(glm::ivec2(Position.x - 1.0f, Position.y));
	}
	else {
		Coord = GetValidCoord(glm::ivec2(Position.x, Position.y - 1.0f));
	}
}

// Checks bullet collisions with objects and walls
void _Map::CheckBulletCollisions(const glm::vec2 &Position, const glm::vec2 &Direction, std::vector<_Hit> &Hits, int GridType, bool CheckObjects, int Penetration) const {
	if(!Data)
		throw std::runtime_error("Tile data uninitialized!");

	// Find slope
	float Slope = Direction.y / Direction.x;

	// Find starting tile
	glm::ivec2 TileTracer = GetValidCoord(glm::ivec2(Position.x, Position.y));

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
	bool EndedOnX = false;
	std::unordered_map<_Entity *, int> HitObjects;
	while(TileTracer.x >= 0 && TileTracer.y >= 0 && TileTracer.x < Width && TileTracer.y < Height && CanShootThrough(TileTracer.x, TileTracer.y)) {

		// Check for object intersections
		_Hit Hit(HIT_OBJECT);
		float MinDistance = HUGE_VAL;
		if(CheckObjects) {
			for(auto Iterator = Data[TileTracer.x][TileTracer.y].Objects[GridType].begin(); Iterator != Data[TileTracer.x][TileTracer.y].Objects[GridType].end(); ++Iterator) {
				_Entity *Entity = (_Entity *)(*Iterator);
				if(Entity->IsDying() || HitObjects.find(Entity) != HitObjects.end())
					continue;

				float Distance = RayObjectIntersection(Position, Direction, Entity);
				if(Distance < MinDistance && Distance > 0.0f) {
					Hit.Object = Entity;
					MinDistance = Distance;
				}
			}
		}

		// An object was hit
		if(CheckObjects && Hit.Object) {
			Hit.Position = Direction * MinDistance + Position;
			Hit.Normal = glm::normalize(-Direction);
			Hits.push_back(Hit);
			HitObjects[Hit.Object] = 1;

			// Update depth count
			Penetration--;
			if(!Penetration)
				return;
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

	// Determine which side has hit
	glm::vec2 WallHitPosition;
	glm::vec2 WallBoundary;
	_Hit Hit(HIT_WALL);
	if(EndedOnX) {

		// Get correct side of the wall
		if(Direction.x < 0) {
			FirstBoundaryTileX = TileTracer.x + 1;
			Hit.Normal.x = 1;
			Hit.Normal.y = 0;
		}
		else {
			FirstBoundaryTileX = TileTracer.x;
			Hit.Normal.x = -1;
			Hit.Normal.y = 0;
		}
		WallBoundary.x = FirstBoundaryTileX - Position.x;

		// Determine hit position
		WallHitPosition.x = WallBoundary.x;
		WallHitPosition.y = WallBoundary.x * Slope;
	}
	else {

		// Get correct side of the wall
		if(Direction.y < 0) {
			FirstBoundaryTileY = TileTracer.y + 1;
			Hit.Normal.x = 0;
			Hit.Normal.y = 1;
		}
		else {
			FirstBoundaryTileY = TileTracer.y;
			Hit.Normal.x = 0;
			Hit.Normal.y = -1;
		}
		WallBoundary.y = FirstBoundaryTileY - Position.y;

		// Determine hit position
		WallHitPosition.x = WallBoundary.y / Slope;
		WallHitPosition.y = WallBoundary.y;
	}

	Hit.Position = WallHitPosition + Position;
	Hits.push_back(Hit);
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
	glm::ivec2 StartTile = GetValidCoord(glm::ivec2(Start));
	glm::ivec2 EndTile = GetValidCoord(glm::ivec2(End));

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
void _Map::GetSelectedObjects(const glm::vec2 &Start, const glm::vec2 &End, std::vector<_ObjectSpawn *> *SelectedObjects) {

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

	for(const auto &ObjectSpawn : ObjectSpawns) {
		if(ObjectSpawn->Position.x > StartPoint.x && ObjectSpawn->Position.y > StartPoint.y && ObjectSpawn->Position.x <= EndPoint.x && ObjectSpawn->Position.y <= EndPoint.y) {
			SelectedObjects->push_back(ObjectSpawn);
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
	if(Index < 0 || Index >= (int)Events.size())
		return;

	DeleteBlockIDFromTiles(-1, Index);
	delete Events[Index];
	Events.erase(Events.begin() + Index);
}

// Remove deleted object spawns
void _Map::CleanObjectSpawns() {
	for(auto Iterator = ObjectSpawns.begin(); Iterator != ObjectSpawns.end(); ) {
		if((*Iterator)->Deleted) {
			delete *Iterator;
			Iterator = ObjectSpawns.erase(Iterator);
		}
		else
			++Iterator;
	}
}

// Return the block at a given position
int _Map::GetSelectedBlock(int Layer, const glm::ivec2 &Index) {
	for(int i = (int)(Blocks[Layer].size())-1; i >= 0; i--) {
		if(Index.x >= Blocks[Layer][i].Start.x && Index.y >= Blocks[Layer][i].Start.y && Index.x <= Blocks[Layer][i].End.x && Index.y <= Blocks[Layer][i].End.y)
			return i;
	}

	return -1;
}

// Return the block at a given position
int _Map::GetSelectedBlock(int Layer, const glm::ivec2 &Index, _Block **Block) {
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
bool _Map::HasEvents(const glm::ivec2 &Position) const {
	if(!Data)
		throw std::runtime_error("Tile data uninitialized!");

	return Data[Position.x][Position.y].Events.size() > 0;
}

// Gets a list of event based on a position
std::vector<_Event *> &_Map::GetEventList(const glm::ivec2 &Position) {
	if(!Data)
		throw std::runtime_error("Tile data uninitialized!");

	return Data[Position.x][Position.y].Events;
}

// Returns a starting position by level and player id
glm::vec2 _Map::GetStartingPositionByCheckpoint(int CheckpointLevel) {

	// Look through checkpoint events
	for(size_t i = 0; i < CheckpointEvents.size(); i++) {
		_Event *Event = CheckpointEvents[i];
		if(Event->Level != CheckpointLevel)
			continue;

		if(Event->Tiles.size() == 0) {
			return glm::vec2(Event->Start.x + 0.5f, Event->Start.y + 0.5f);
		}
		else {
			size_t TileID = ae::GetRandomInt((size_t)0, Event->Tiles.size()-1);
			return glm::vec2(Event->Tiles[TileID].Coord.x + 0.5f, Event->Tiles[TileID].Coord.y + 0.5f);
		}
	}

	return glm::vec2(2.5f, 2.5f);
}

// Return the event at a given position
int _Map::GetSelectedEvent(const glm::ivec2 &Index, _Event **ReturnEvent) {

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
	if(Mode <= 0)
		return;

	// Draw vertical lines
	ae::Graphics.SetColor(COLOR_TWHITE);
	for(int i = Mode; i < Width; i += Mode)
		ae::Graphics.DrawLine(glm::vec2(i, 0), glm::vec2(i, Height));

	// Draw horizontal lines
	for(int i = Mode; i < Height; i += Mode)
		ae::Graphics.DrawLine(glm::vec2(0, i), glm::vec2(Width, i));
}

// Draw the mini map
void _Map::DrawMinimap(bool FullMap) {

	// Get bounds of minimap window
	glm::vec2 DrawSize = FullMap ? glm::vec2(ae::Graphics.CurrentSize.y, ae::Graphics.CurrentSize.y) : HUD_MINIMAP_SIZE;
	ae::_Bounds MinimapBounds;
	if(FullMap) {
		MinimapBounds = ae::_Bounds(
							glm::ivec2((ae::Graphics.CurrentSize - glm::ivec2(DrawSize))/2),
							glm::ivec2((ae::Graphics.CurrentSize + glm::ivec2(DrawSize))/2)
						);
	}
	else {
		MinimapBounds = ae::_Bounds(
							glm::ivec2(ae::Graphics.CurrentSize.x - DrawSize.x - HUD_MINIMAP_PADDING.x, HUD_MINIMAP_PADDING.y),
							glm::ivec2(ae::Graphics.CurrentSize.x - HUD_MINIMAP_PADDING.x, HUD_MINIMAP_PADDING.y + DrawSize.y)
						);
	}

	// Draw minimap background
	ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos"]);
	ae::Graphics.SetColor(HUD_MINIMAP_BACKGROUND_COLOR);
	ae::Graphics.EnableScissorTest();
	ae::Graphics.SetScissor(MinimapBounds);
	ae::Graphics.DrawRectangle(MinimapBounds, true);

	// Draw layers
	ae::_Bounds CaptureBounds(Camera->GetPosition() - MinimapCaptureSize, Camera->GetPosition() + MinimapCaptureSize);
	glm::vec2 VisionSize = CaptureBounds.End - CaptureBounds.Start;
	for(const auto &MinimapLayer : MinimapLayers) {
		glm::vec2 Start = MinimapBounds.Start + ((MinimapLayer.Bounds.Start - CaptureBounds.Start) / VisionSize) * DrawSize;
		glm::vec2 End = MinimapBounds.Start + ((MinimapLayer.Bounds.End - CaptureBounds.Start) / VisionSize) * DrawSize;

		ae::Graphics.SetColor(MinimapLayer.Color);
		ae::Graphics.DrawRectangle(Start, End, true);
	}

	ae::Graphics.DisableScissorTest();
}

// Draws rectangles around all the blocks
void _Map::HighlightBlocks(int Layer) {
	ae::Graphics.SetColor(COLOR_MAGENTA);
	for(size_t i = 0; i < Blocks[Layer].size(); i++)
		ae::Graphics.DrawRectangle3D(glm::vec2(Blocks[Layer][i].Start.x, Blocks[Layer][i].Start.y), glm::vec2(Blocks[Layer][i].End.x + 1.0f, Blocks[Layer][i].End.y + 1.0f), false);
}

// Add particle to grid
void _Map::AddParticle(_Particle *Particle) {
	if(!Data)
		throw std::runtime_error("Tile data uninitialized!");

	glm::ivec2 Coord = GetValidCoord(glm::ivec2(Particle->Position.x, Particle->Position.y));
	Data[Coord.x][Coord.y].Particles.push_back(Particle);

	Particles.push_back(Particle);
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
	if(!(Event->Type == EVENT_DOOR || Event->Type == EVENT_WALLSWITCH || Event->Type == EVENT_FLOORSWITCH))
		return;

	const std::vector<_EventTile> &Tiles = Event->Tiles;

	// Switch the texture of the first block for wall switches
	int StartIndex = 0;
	if(Event->Type == EVENT_WALLSWITCH && Tiles.size() > 0 && Tiles[0].BlockID != -1) {
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
	if(Index == -1)
		return;

	_Block *Block = &Blocks[Layer][Index];
	std::swap(Block->Texture, Block->AltTexture);
}

// Renders the floor
int _Map::RenderFloors() {
	if(!Camera)
		return 0;

	// Draw base layer
	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
	ae::Graphics.SetColor(glm::vec4(1.0f));
	ae::Graphics.SetDepthTest(false);
	ae::Graphics.SetDepthMask(false);

	int Count = 0;
	for(size_t i = 0; i < Blocks[MAPLAYER_BASE].size(); i++) {
		_Block *Block = &Blocks[MAPLAYER_BASE][i];

		bool Draw = true;
		if(Block->MinZ >= 0) {
			glm::vec4 Bounds;
			Block->GetBounds(Bounds);
			Draw = Camera->IsAABBInView(Bounds);
		}

		if(!Draw || !Block->Texture)
			continue;

		ae::Graphics.DrawRepeatable(
			glm::vec3(Block->Start.x, Block->Start.y, Block->MinZ + MAP_LAYEROFFSET * i),
			glm::vec3(Block->End.x + 1.0f, Block->End.y + 1.0f, Block->MinZ + MAP_LAYEROFFSET * i),
			Block->Texture,
			Block->Rotation,
			Block->ScaleX
		);

		Count++;
	}

	// Draw floor layers 0-2
	ae::Graphics.SetDepthMask(true);
	ae::Graphics.SetDepthTest(true);
	for(int i = MAPLAYER_FLOOR0; i <= MAPLAYER_FLOOR2; i++) {
		for(int j = 0; j < (int)(Blocks[i].size()); j++) {
			_Block *Block = &Blocks[i][j];
			if(Block->MinZ == Block->MaxZ) {

				bool Draw = true;
				if(Block->MinZ >= 0) {
					glm::vec4 Bounds;
					Block->GetBounds(Bounds);
					Draw = Camera->IsAABBInView(Bounds);
				}

				if(!Draw || !Block->Texture)
					continue;

				ae::Graphics.DrawRepeatable(
					glm::vec3(Block->Start.x, Block->Start.y, Block->MinZ + MAP_LAYEROFFSET * i),
					glm::vec3(Block->End.x + 1.0f, Block->End.y + 1.0f, Block->MinZ + MAP_LAYEROFFSET * i),
					Block->Texture,
					Block->Rotation,
					Block->ScaleX
				);

				Count++;
			}
			else if(Block->Texture) {
				ae::Graphics.DrawCube(
					glm::vec3(Block->Start.x, Block->Start.y, Block->MinZ),
					glm::vec3(Block->End.x - Block->Start.x + 1.0f, Block->End.y - Block->Start.y + 1.0f, Block->MaxZ - Block->MinZ),
					Block->Texture
				);

				Count++;
			}
		}
	}

	return Count;
}

// Renders the walls
int _Map::RenderWalls() {
	if(!Camera)
		return 0;

	// Set up graphics
	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
	ae::Graphics.SetColor(glm::vec4(1.0f));
	ae::Graphics.SetDepthMask(true);
	ae::Graphics.SetDepthTest(true);
	ae::Graphics.SetCullFace(true);

	// Draw walls
	int Count = 0;
	for(size_t i = 0; i < Blocks[MAPLAYER_WALL].size(); i++) {
		_Block *Block = &Blocks[MAPLAYER_WALL][i];

		// Always draw walls that go lower than floor
		bool Draw = true;
		if(Block->MinZ >= 0) {
			glm::vec4 Bounds;
			Block->GetBounds(Bounds);
			Draw = Camera->IsAABBInView(Bounds);
		}

		// Skip
		if(!Draw || !Block->Texture)
			continue;

		// Draw cube
		ae::Graphics.DrawCube(
			glm::vec3(Block->Start.x, Block->Start.y, Block->MinZ),
			glm::vec3(Block->End.x - Block->Start.x + 1.0f, Block->End.y - Block->Start.y + 1.0f, Block->MaxZ - Block->MinZ),
			Block->Texture
		);

		Count++;
	}

	ae::Graphics.SetCullFace(false);

	return Count;
}

// Render flat walls
int _Map::RenderFlatWalls() {

	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
	ae::Graphics.SetColor(glm::vec4(1.0f));
	ae::Graphics.SetDepthMask(false);
	ae::Graphics.SetDepthTest(true);

	int Count = 0;
	for(size_t i = 0; i < Blocks[MAPLAYER_FLAT].size(); i++) {
		_Block *Block = &Blocks[MAPLAYER_FLAT][i];

		// Check bounds
		bool Draw = true;
		if(Block->MinZ >= 0) {
			glm::vec4 Bounds;
			Block->GetBounds(Bounds);
			Draw = Camera->IsAABBInView(Bounds);
		}

		if(!Draw || !Block->Texture)
			continue;

		// Draw
		ae::Graphics.DrawWall(
			glm::vec3(Block->Start.x, Block->Start.y, Block->MinZ),
			glm::vec3(Block->End.x - Block->Start.x + 1.0f, Block->End.y - Block->Start.y + 1.0f, Block->MaxZ - Block->MinZ),
			Block->Rotation,
			Block->Texture
		);

		Count++;
	}

	return Count;
}

// Draws the events
void _Map::RenderEvents(std::vector<const ae::_Texture *> &Textures) {
	if(!Camera)
		return;

	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
	ae::Graphics.SetColor(glm::vec4(1.0f));
	ae::Graphics.SetDepthTest(false);

	// Draw events
	for(size_t i = 0; i < Events.size(); i++) {
		glm::vec4 Bounds(Events[i]->Start.x, Events[i]->Start.y, Events[i]->End.x + 1.0f, Events[i]->End.y + 1.0f);
		if(!Camera->IsAABBInView(Bounds))
			continue;

		// Draw event overlay
		ae::Graphics.DrawRepeatable(
			glm::vec3(Events[i]->Start.x, Events[i]->Start.y, MAP_LAYEROFFSET),
			glm::vec3(Events[i]->End.x + 1.0f, Events[i]->End.y + 1.0f, MAP_LAYEROFFSET),
			Textures[Events[i]->Type],
			0,
			1.0f
		);
	}
}

// Renders the foreground tiles
int _Map::RenderForeground() {
	if(!Camera)
		return 0;

	// Set up graphics
	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
	ae::Graphics.SetColor(glm::vec4(1.0f));
	ae::Graphics.SetDepthMask(true);
	ae::Graphics.SetDepthTest(true);

	// Draw foreground
	int Count = 0;
	for(size_t i = 0; i < Blocks[6].size(); i++) {
		_Block *Block = &Blocks[6][i];

		// Check bounds
		bool Draw = true;
		if(Block->MinZ >= 0) {
			glm::vec4 Bounds;
			Block->GetBounds(Bounds);
			Draw = Camera->IsAABBInView(Bounds);
		}

		if(!Draw)
			continue;

		// Draw
		ae::Graphics.DrawRepeatable(
			glm::vec3(Block->Start.x, Block->Start.y, Block->MaxZ + MAP_LAYEROFFSET),
			glm::vec3(Block->End.x + 1.0f, Block->End.y + 1.0f, Block->MaxZ + MAP_LAYEROFFSET),
			Block->Texture,
			Block->Rotation,
			Block->ScaleX
		);

		Count++;
	}

	return Count;
}

// Render entities and items
void _Map::RenderObjects(double BlendFactor) {
	ae::Assets.Programs["pos_uv"]->ResetTextureTransform();
	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
	ae::Graphics.SetDepthMask(false);
	ae::Graphics.SetDepthTest(true);
	ObjectManager->Render(BlendFactor);
}

// Render map decals
int _Map::RenderParticles(int Type) {

	// Get start and end range of tiles to render
	glm::ivec2 Start = GetValidCoord(glm::ivec2(Camera->AABB[0] - PARTICLE_GRID_PADDING, Camera->AABB[1] - PARTICLE_GRID_PADDING));
	glm::ivec2 End = GetValidCoord(glm::ivec2(Camera->AABB[2] + PARTICLE_GRID_PADDING, Camera->AABB[3] + PARTICLE_GRID_PADDING));

	// Draw particles
	int Count = 0;
	for(int i = Start.x; i <= End.x; i++) {
		for(int j = Start.y; j <= End.y; j++) {
			for(const auto &Particle : Data[i][j].Particles) {
				if(Particle->Type != Type)
					continue;

				Particle->Render();
				Count++;
			}
		}
	}

	return Count;
}

// Update map
void _Map::Update(double FrameTime) {

	// Add blocks and events to minimap
	MinimapLayers.clear();
	AddMinimapLayers();

	// Update objects
	ObjectManager->Update(FrameTime, this);

	// Update ambient light
	if(AmbientLightPeriod > 0 && AmbientLightTimer <= AmbientLightPeriod) {
		AmbientLightBlendFactor = AmbientLightTimer / AmbientLightPeriod;
		AmbientLightTimer += FrameTime;
	}
	else
		AmbientLightBlendFactor = 1.0f;
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

// Check if bounds are in minimap range
bool _Map::CheckMinimapBounds(const glm::vec4 &Bounds) {

	if(Bounds[2] < Camera->GetPosition().x - MinimapCaptureSize || Bounds[0] > Camera->GetPosition().x + MinimapCaptureSize)
	   return false;
	if(Bounds[3] < Camera->GetPosition().y - MinimapCaptureSize || Bounds[1] > Camera->GetPosition().y + MinimapCaptureSize)
	   return false;

	return true;
}

// Add objects to the minimap
void _Map::AddMinimapLayers() {

	// Add walls
	for(int Layer = MAPLAYER_FLAT; Layer <= MAPLAYER_WALL; Layer++) {
		for(size_t i = 0; i < Blocks[Layer].size(); i++) {
			_Block *Block = &Blocks[Layer][i];
			if(Block->MinZ > 0)
				continue;

			// Check bounds
			glm::vec4 Bounds;
			Block->GetBounds(Bounds);
			if(!CheckMinimapBounds(Bounds))
				continue;

			// Check for empty texture
			if(!Block->Texture)
				continue;

			_MinimapLayer MinimapLayer;
			MinimapLayer.Bounds = Bounds;
			MinimapLayer.Color = HUD_MINIMAP_WALL_COLOR;
			MinimapLayers.push_back(MinimapLayer);
		}
	}

	// Add doors to minimap
	for(const auto &Event : Events) {
		if(Event->Type != EVENT_DOOR && Event->Type != EVENT_WALLSWITCH)
			continue;

		// Check bounds
		glm::vec4 Bounds;
		Event->GetBounds(Bounds);
		if(!CheckMinimapBounds(Bounds))
			continue;

		_MinimapLayer MinimapLayer;
		MinimapLayer.Bounds = Bounds;
		if(Event->Switched)
			MinimapLayer.Color = HUD_MINIMAP_TOGGLED_COLOR;
		else if(Event->ItemID.empty())
			MinimapLayer.Color = HUD_MINIMAP_DOOR_COLOR;
		else
			MinimapLayer.Color = Stats.Objects.at(Event->ItemID).DoorColor;
		MinimapLayers.push_back(MinimapLayer);
	}
}

// Generates a random point inside of a circle
glm::vec2 _Map::GenerateRandomPointInCircle(float Radius) {
	return glm::rotate(glm::vec2(0, -1), glm::radians((float)(ae::GetRandomReal(0, 1) * 360.0))) * Radius * (float)sqrt(ae::GetRandomReal(0, 1));
}

// Add missing extensions to filename
std::string _Map::FixFilename(const std::string &Filename) {
	std::string NewFilename = Filename;
	if(NewFilename.find(".map", 0) == std::string::npos)
		NewFilename = NewFilename + ".map";
	if(NewFilename.find(".gz", 0) == std::string::npos)
		NewFilename = NewFilename + ".gz";

	return NewFilename;
}
