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
#include <coord.h>
#include <color.h>
#include <string>
#include <list>
#include <vector>
#include <memory>
#include <stdexcept>

// Types of map layers
enum MapLayerTypes {
	MAPLAYER_BASE,
	MAPLAYER_FLOOR0,
	MAPLAYER_FLOOR1,
	MAPLAYER_FLOOR2,
	MAPLAYER_FLAT,
	MAPLAYER_WALL,
	MAPLAYER_FORE,
	MAPLAYER_COUNT
};

// Types of objects in the collision grid
enum CollisionGridType {
	GRID_PLAYER,
	GRID_MONSTER,
	GRID_ITEM,
	GRID_COUNT
};

// Types of maps
enum MapType {
	MAPTYPE_SINGLE,
	MAPTYPE_MULTI,
	MAPTYPE_TUTORIAL,
	MAPTYPE_ADVENTURE
};

const int WALL_LEFT                 = 0x1;
const int WALL_TOP                  = 0x2;
const int WALL_RIGHT                = 0x4;
const int WALL_BOTTOM               = 0x8;
const float PARTICLE_GRID_PADDING   = 2;

// Forward Declarations
class _Event;
class _Entity;
class _Object;
class _Item;
class _Camera;
class _Texture;
class _Particle;
class _ObjectManager;
struct _ObjectSpawn;
struct _Hit;

// Holds data for a single tile
struct _Tile {
	enum CollisionFlagType {
		ENTITY = 1,
		BULLET = 2,
	};

	_Tile() : Collision(0) { }

	bool CanWalk() { return !(Collision & ENTITY); }
	bool CanShoot() { return !(Collision & BULLET); }

	std::list<_Object *> Objects[GRID_COUNT];
	std::list<_Event *> Events;
	std::vector<_Particle *> Particles;
	int Collision;
};

// Holds data for a tile bound
struct _TileBounds {
	_Coord Start;
	_Coord End;
};

// Holds data for a block of tiles
struct _Block {

	void GetBounds(float *Bounds) { Bounds[0] = (float)Start.x; Bounds[1] = (float)Start.y; Bounds[2] = End.x + 1.0f; Bounds[3] = End.y + 1.0f; }

	_Coord Start;
	_Coord End;
	const _Texture *Texture;
	const _Texture *AltTexture;
	float MinZ;
	float MaxZ;
	float Rotation;
	float ScaleX;
	bool Wall;
	bool Walkable;
};

// Holds information about a hit entity
struct _Hit {

	_Hit() { }
	_Hit(_Entity *Object, const glm::vec2 &Position, int Type) :
		Object(Object),
		Position(Position),
		Type(Type) { }

	_Entity *Object;
	glm::vec2 Normal;
	glm::vec2 Position;
	int Type;
};

// Holds information about object spawns
struct _ObjectSpawn {

	_ObjectSpawn() :
		Identifier(""),
		Position{0, 0},
		Type(-1),
		Deleted(false) { }

	_ObjectSpawn(const std::string &Identifier, const glm::vec2 &Position, int Type) :
		Identifier(Identifier),
		Position(Position),
		Type(Type),
		Deleted(false) { }

	std::string Identifier;
	glm::vec2 Position;
	int Type;
	bool Deleted;
};

// Classes
class _Map {

	public:

		_Map();
		_Map(const std::string &Filename);
		~_Map();

		void Init();
		void Update(double FrameTime);

		bool SaveLevel(const std::string &String);
		bool LoadMonsterSet(const std::string &String);
		bool CheckCollisions(const glm::vec2 &TargetPosition, float Radius, glm::vec2 &NewPosition);
		void CheckEntityCollisionsInGrid(const glm::vec2 &Position, float Radius, const _Object *SkipObject, std::list<_Entity *> &Entities) const;
		_Object *CheckCollisionsInGrid(const glm::vec2 &Position, float Radius, int GridType, const _Object *SkipObject) const;
		_Entity *CheckMeleeCollisions(_Entity *Attacker, const glm::vec2 &Direction, int GridType) const;
		void CheckBulletCollisions(const glm::vec2 &Position, const glm::vec2 &Direction, _Hit &Hit, int GridType, bool CheckObjects) const;
		float RayObjectIntersection(const glm::vec2 &Origin, const glm::vec2 &Direction, const _Object *Object) const;
		bool IsVisible(const glm::vec2 &Start, const glm::vec2 &End) const;
		bool IsVisibleWithBounds(const glm::vec2 &Start, const glm::vec2 &End, float BoundSize) const;
		void AddObjectToGrid(_Object *Object, int Type);
		void RemoveObjectFromGrid(_Object *Object, int Type);

		void ChangeMapState(const _Event *Event);
		bool CanChangeMapState(const _Event *Event);
		void ToggleEventActive(int Index);
		void SwapBlockTextures(int Layer, int Index);
		bool HasEvents(const _Coord &Position) const;

		void SetAmbientLight(const glm::vec4 &Color) { OldAmbientLight = AmbientLight; AmbientLight = Color; }
		void SetAmbientLightChangePeriod(double Value) { AmbientLightPeriod = Value; AmbientLightTimer = AmbientLightBlendFactor = 0.0; }

		void RenderFloors();
		void RenderWalls();
		void RenderFlatWalls();
		void RenderObjects(double BlendFactor);
		int RenderParticles(int Type);
		void RenderForeground();
		void RenderLights(const glm::vec2 &PlayerPosition);
		void RenderEvents(std::vector<const _Texture *> &Textures);
		void RenderGrid(int Mode);
		void HighlightBlocks(int Layer);

		void AddBlock(int Layer, _Block Block) { Blocks[Layer].push_back(Block); }
		void AddEvent(_Event *Event) { Events.push_back(Event); }
		void AddObject(_ObjectSpawn *Object) { ObjectSpawns.push_back(Object); }
		void AddParticle(_Particle *Particle);
		const std::vector<_ObjectSpawn *> &GetObjectsList() { return ObjectSpawns; }
		void GetSelectedObject(const glm::vec2 &Position, float RadiusSquared, _ObjectSpawn **Object, size_t *Index);
		void GetSelectedObjects(const glm::vec2 &Start, const glm::vec2 &End, std::list<_ObjectSpawn *> *SelectedObjects);
		int GetSelectedBlock(int Layer, const _Coord &Index, _Block **Block);
		int GetSelectedBlock(int Layer, const _Coord &Index);
		int GetSelectedEvent(const _Coord &Index, _Event **Event);
		int GetLastBlock(int Layer, _Block **Block);
		int GetLayerSize(int Index);
		void ChangeLayer(int OldLayer, int NewLayer, int Index);
		void DeleteBlockIDFromTiles(int Layer, int Index);
		void RemoveLastBlock(int Layer) { if(Blocks[Layer].size() > 0) Blocks[Layer].pop_back(); }
		void RemoveBlock(int Layer, int Index);
		void RemoveEvent(int Index);
		void CleanObjectSpawns();

		void ClearEvent(const _Event *Event);

		const std::string &GetFilename() const { return Filename; }
		_Event *GetEvent(int Index) const;
		std::list<_Event *> &GetEventList(const _Coord &Coord);
		glm::vec2 GetStartingPositionByCheckpoint(int Level);
		int GetTotalBlockSize() const;
		int GetMapType() const { return MapType; }
		int GetWidth() const { return Width; }
		int GetHeight() const { return Height; }
		int GetWallState(const glm::vec2 &Position, float Radius) const;
		void GetAdjacentTile(const glm::vec2 &Position, float Direction, _Coord &Coord) const;
		_Coord GetValidCoord(const _Coord &Coord) const;
		bool CanShootThrough(int IndexX, int IndexY) const;
		void GetTileBounds(const glm::vec2 &Position, float Radius, _TileBounds &TileBounds) const;
		const _Block *GetBlock(int Layer, const size_t Index) const;
		glm::vec2 GetValidPosition(const glm::vec2 &Position) const;

		void AddItem(_Item *Item);
		void RemoveItem(_Item *Item);

		void SetCamera(_Camera *Camera) { this->Camera = Camera; }
		_Camera *GetCamera() { return Camera; }

		void AddRenderList(_Object *Object, int Layer);
		static glm::vec2 GenerateRandomPointInCircle(float Radius);

		std::vector<std::string> MonsterSet;

	private:

		bool CheckTileCollision(const glm::vec2 &Position, float Radius, float X, float Y, bool Resolve, glm::vec2 &Push, bool &DiagonalPush);

		// Map
		int MapType;
		int Width;
		int Height;
		std::string Filename;

		// Blocks
		_Tile **Data;
		std::vector<_Block> Blocks[MAPLAYER_COUNT];
		std::vector<_Event *> Events;
		std::vector<_Event *> CheckpointEvents;

		// Objects
		std::unique_ptr<_ObjectManager> ObjectManager;
		std::list<_Object *> Objects;
		std::vector<_ObjectSpawn *> ObjectSpawns;
		std::vector<_Particle *> Particles;

		// Graphics
		_Camera *Camera;
		std::string MonsterSetID;

		// Lights
		glm::vec4 AmbientLight;
		glm::vec4 OldAmbientLight;
		double AmbientLightBlendFactor;
		double AmbientLightPeriod;
		double AmbientLightTimer;

};

// Returns a coordinate inside the map
inline _Coord _Map::GetValidCoord(const _Coord &Coord) const {
	return _Coord(
		std::max(0, std::min(Coord.x, Width-1)),
		std::max(0, std::min(Coord.y, Height-1))
	);
}

// Determines if a tile can be shot through
inline bool _Map::CanShootThrough(int IndexX, int IndexY) const {
	if(!Data)
		throw std::runtime_error("Tile data uninitialized!");

	return Data[IndexX][IndexY].CanShoot();
}

// Returns a bounding rectangle
inline void _Map::GetTileBounds(const glm::vec2 &Position, float Radius, _TileBounds &TileBounds) const {

	// Get tile indices where the bounding rectangle touches
	TileBounds.Start = GetValidCoord(Position - Radius);
	TileBounds.End = GetValidCoord(Position + Radius);
}
