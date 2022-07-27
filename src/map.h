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
#include <ae/bounds.h>
#include <glm/vec2.hpp>
#include <color.h>
#include <string>
#include <list>
#include <vector>
#include <unordered_map>
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
	GRID_PROJECTILE,
	GRID_COUNT
};

// Types of maps
enum MapType {
	MAPTYPE_CAMPAIGN,
	MAPTYPE_ADVENTURE
};

// Types of attack outcomes
enum CollisionType {
	HIT_NONE,
	HIT_WALL,
	HIT_OBJECT
};

const int WALL_LEFT                 = 0x1;
const int WALL_TOP                  = 0x2;
const int WALL_RIGHT                = 0x4;
const int WALL_BOTTOM               = 0x8;
const float PARTICLE_GRID_PADDING   = 2;

// Forward Declarations
namespace ae {
	class _Camera;
	class _Texture;
}
class _Event;
class _Entity;
class _Object;
class _Item;
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

	_Tile() : Collision(0), CollisionChangeMask(ENTITY | BULLET) { }

	bool CanWalk() { return !(Collision & ENTITY); }
	bool CanShoot() { return !(Collision & BULLET); }

	std::unordered_map<_Object *, int> Objects[GRID_COUNT];
	std::vector<_Event *> Events;
	std::vector<_Particle *> Particles;
	int Collision;
	int CollisionChangeMask;
};

// Holds data for a tile bound
struct _TileBounds {
	glm::ivec2 Start;
	glm::ivec2 End;
};

// Holds data for a block of tiles
struct _Block {

	_Block() :
		Start(0.0f),
		End(0.0f),
		Texture(nullptr),
		AltTexture(nullptr),
		MinZ(0.0f),
		MaxZ(0.0f),
		Rotation(0.0f),
		ScaleX(0.0f),
		Walkable(false)
	{ }

	void GetBounds(glm::vec4 &Bounds) { Bounds[0] = Start.x; Bounds[1] = Start.y; Bounds[2] = End.x + 1.0f; Bounds[3] = End.y + 1.0f; }

	glm::ivec2 Start;
	glm::ivec2 End;
	const ae::_Texture *Texture;
	const ae::_Texture *AltTexture;
	float MinZ;
	float MaxZ;
	float Rotation;
	float ScaleX;
	bool Walkable;
};

// Holds information about a hit entity
struct _Hit {

	_Hit() { }
	_Hit(int Type) : Object(nullptr), Type(Type) { }

	_Object *Object;
	glm::vec2 Normal;
	glm::vec2 Position;
	glm::vec2 Push;
	float DistanceSquared;
	bool AxisAlignedPush;
	int Type;
};

// Holds information about object spawns
struct _ObjectSpawn {

	_ObjectSpawn() :
		ID(""),
		Position{0, 0},
		Scale(1.0f),
		Type(0),
		Level(1),
		Deleted(false) { }

	_ObjectSpawn(const std::string &ID, const glm::vec2 &Position, int Type, int Level) :
		ID(ID),
		Position(Position),
		Scale(1.0f),
		Type(Type),
		Level(Level),
		Deleted(false) { }

	std::string ID;
	glm::vec2 Position;
	float Scale;
	int Type;
	int Level;
	bool Deleted;
};

// Holds minimap layer data
struct _MinimapLayer {
	ae::_Bounds Bounds;
	glm::vec4 Color;
};

// Classes
class _Map {

	friend class _PlayState;

	public:

		_Map();
		_Map(const std::string &Filename, double Clock=0.0, int SpawnMultiplier=1);
		~_Map();

		void InitializeTiles();
		bool Save(const std::string &String);

		void Update(double FrameTime, double Clock);

		bool ResolveTileCollisions(const glm::vec2 &TargetPosition, float Radius, glm::vec2 &NewPosition);
		bool CheckCollisionsInGrid(const glm::vec2 &Position, float Radius, int GridType, _Hit &Hit);
		std::vector<_Hit> &ResolveCollisionsInGrid(const glm::vec2 &Position, float Radius, const _Object *SkipObject, bool &AxisAlignedPush, float PushFactor=1.0f);
		_Object *GetCloseObject(const glm::vec2 &Position, float Radius, int GridType) const;
		void GetCloseObjects(const glm::vec2 &Position, float Radius, int GridType, std::unordered_map<_Object *, int> &Objects, _Object **ClosestObject) const;
		void CheckMeleeCollisions(_Entity *Attacker, int GridType, int Penetration, std::vector<_Hit> &Hits);
		void CheckBulletCollisions(_Object *Attacker, const glm::vec2 &Direction, std::vector<_Hit> &Hits, int GridType, bool TestObjects, int Penetration, int CollisionFlag);
		bool IsVisible(const glm::vec2 &Start, const glm::vec2 &End, int CheckFlag) const;
		bool CanMoveTo(const glm::vec2 &Start, const glm::vec2 &End, const glm::vec2 &Size) const;
		void AddObjectToGrid(_Object *Object, int Type);
		void RemoveObjectFromGrid(_Object *Object, int Type);

		void ChangeMapState(const _Event *Event);
		bool CanChangeMapState(const _Event *Event);
		void ToggleEventActive(int Index);
		void SwapBlockTextures(int Layer, int Index);
		bool HasEvents(const glm::ivec2 &Position) const;

		void SetAmbientLight(const std::string &ColorID);

		int RenderFloors();
		int RenderWalls();
		int RenderFlatWalls();
		int RenderParticles(int Type);
		int RenderForeground();
		int RenderProps();
		void RenderEvents(std::vector<const ae::_Texture *> &Textures);
		void RenderGrid(int Mode);
		void DrawMinimap(bool FullMap, ae::_Bounds &MinimapBounds);
		void HighlightBlocks(int Layer);

		void AddBlock(int Layer, _Block Block) { Blocks[Layer].push_back(Block); }
		void AddEvent(_Event *Event) { Events.push_back(Event); }
		void AddParticle(_Particle *Particle);
		void GetSelectedObject(const glm::vec2 &Position, float RadiusSquared, _ObjectSpawn **Object, size_t *Index);
		void GetSelectedObjects(const glm::vec2 &Start, const glm::vec2 &End, std::vector<_ObjectSpawn *> *SelectedObjects, int Type);
		int GetSelectedBlock(int Layer, const glm::ivec2 &Index, _Block **Block);
		int GetSelectedBlock(int Layer, const glm::ivec2 &Index);
		int GetSelectedEvent(const glm::ivec2 &Index, _Event **Event);
		int GetLastBlock(int Layer, _Block **Block);
		int GetLayerSize(int Index);
		void ChangeLayer(int OldLayer, int NewLayer, int Index);
		void DeleteBlockIDFromTiles(int Layer, int Index);
		void RemoveLastBlock(int Layer) { if(Blocks[Layer].size() > 0) Blocks[Layer].pop_back(); }
		void RemoveBlock(int Layer, int Index);
		void RemoveEvent(int Index);
		void CleanObjectSpawns();

		void ClearEvent(const _Event *Event);

		std::vector<_Event *> &GetEventList(const glm::ivec2 &Coord);
		glm::vec2 GetStartingPositionByCheckpoint(int CheckpointLevel);
		int GetTotalBlockSize() const;
		int GetWallState(const glm::vec2 &Position, float Radius) const;
		void GetAdjacentTile(const glm::vec2 &Position, float Direction, glm::ivec2 &Coord) const;
		glm::ivec2 GetValidCoord(const glm::ivec2 &Coord) const;
		bool CheckCollisionFlag(const glm::ivec2 &Position, int Flag) const;
		void GetTileBounds(const glm::vec2 &Position, float Radius, _TileBounds &TileBounds) const;
		const _Block *GetBlock(int Layer, const size_t Index) const;
		glm::vec2 GetValidPosition(const glm::vec2 &Position) const;

		void AddObject(_Object *Object, int GridType);
		void RemoveObject(_Object *Object, int GridType);

		bool CheckMinimapBounds(const glm::vec4 &Bounds);
		void AddMinimapLayers();

		static glm::vec2 GenerateRandomPointInCircle(float Radius);
		static std::string FixFilename(const std::string &Filename);

		// Stats
		std::string Filename;
		glm::vec4 BaseAmbientLight;
		glm::ivec2 Size;
		int MapType;
		int Level;
		int Monsters;
		int Crates;
		int Secrets;
		bool BaseAmbientClock;
		bool AmbientClock;
		bool SimpleAI;

		// Objects
		ae::_Camera *Camera;
		std::unique_ptr<_ObjectManager> ObjectManager;
		std::vector<_ObjectSpawn *> ObjectSpawns;
		std::vector<_Event *> Events;

		// Minimap
		std::vector<_MinimapLayer> MinimapLayers;
		glm::vec2 MinimapCaptureSize;

		// Lights
		glm::vec4 AmbientLight;
		glm::vec4 TargetAmbientLight;

	private:

		void GetClockLight(double Clock, glm::vec4 &LightColor);
		void UpdateAmbientLight(double FrameTime, double Clock);
		bool CheckAABBCollision(const glm::vec2 &Position, float Radius, const float *AABB, bool Resolve, _Hit &Hit) const;

		// Blocks
		_Tile **Data;
		std::vector<_Block> Blocks[MAPLAYER_COUNT];
		std::vector<_Event *> CheckpointEvents;

		// Objects
		std::vector<_Particle *> Particles;
		std::unordered_map<_Object *, int> ObjectMap;
		std::vector<_Hit> CollisionHits;
		std::vector<glm::vec2> CollisionPushes;
};

// Returns a coordinate inside the map
inline glm::ivec2 _Map::GetValidCoord(const glm::ivec2 &Coord) const {
	return glm::ivec2(
		std::max(0, std::min(Coord.x, Size.x-1)),
		std::max(0, std::min(Coord.y, Size.y-1))
	);
}

// Check collision flag on a tile
inline bool _Map::CheckCollisionFlag(const glm::ivec2 &Position, int Flag) const {
	return !(Data[Position.x][Position.y].Collision & Flag);
}

// Returns a bounding rectangle
inline void _Map::GetTileBounds(const glm::vec2 &Position, float Radius, _TileBounds &TileBounds) const {

	// Get tile indices where the bounding rectangle touches
	TileBounds.Start = GetValidCoord(Position - Radius);
	TileBounds.End = GetValidCoord(Position + Radius);
}
