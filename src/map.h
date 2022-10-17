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
		VISION = 4,
	};

	bool CanWalk() { return !(Collision & ENTITY); }
	bool CanShoot() { return !(Collision & BULLET); }
	bool CanSee() { return !(Collision & VISION); }

	std::unordered_map<_Object *, int> Objects[GRID_COUNT];
	std::vector<_Event *> Events;
	std::vector<_Particle *> Particles;
	int Collision{0};
	int CollisionChangeMask{ENTITY | BULLET | VISION};
};

// Holds data for a tile bound
struct _TileBounds {
	glm::ivec2 Start;
	glm::ivec2 End;
};

// Holds data for a block of tiles
struct _Block {

	void GetBounds(glm::vec4 &Bounds, bool AdjustForZ) {
		float Adjust = AdjustForZ ? std::abs(std::min(MinZ, MaxZ)) * 2.0f : 0.0f;
		Bounds[0] = Start.x - Adjust;
		Bounds[1] = Start.y - Adjust;
		Bounds[2] = End.x + Adjust + 1.0f;
		Bounds[3] = End.y + Adjust + 1.0f;
	}

	int GetLargestAxis() { if(End.y - Start.y > End.x - Start.x) { return 1; } else { return 0; } }

	glm::vec4 Color{1.0f};
	glm::ivec2 MoveStart{0};
	glm::ivec2 MoveEnd{0};
	glm::ivec2 Start{0};
	glm::ivec2 End{0};
	const ae::_Texture *Texture{nullptr};
	const ae::_Texture *AltTexture{nullptr};
	float MinZ{0.0f};
	float MaxZ{0.0f};
	float Rotation{0.0f};
	float ScaleX{0.0f};
	bool Walkable{false};
};

// Holds information about a hit entity
struct _Hit {

	_Hit() {}
	_Hit(int Type) : Object(nullptr), Type(Type) {}

	_Object *Object;
	glm::vec2 Normal;
	glm::vec2 Position;
	glm::vec2 ClosestPoint;
	glm::vec2 Push;
	float DistanceSquared;
	bool AxisAlignedPush;
	int Type;
};

// Holds information about object spawns
struct _ObjectSpawn {

	_ObjectSpawn() {}
	_ObjectSpawn(const std::string &ID, const glm::vec2 &Position, int Type, int Level) : ID(ID), Position(Position), Type(Type), Level(Level) {}

	std::string ID;
	glm::vec2 Position{0.0f};
	float Rotation{0.0f};
	float Scale{1.0f};
	int Type{0};
	int Level{1};
	bool Deleted{false};
};

// Holds minimap icon data
struct _MinimapIcon {
	ae::_Bounds Bounds;
};

// Classes
class _Map {

	friend class _PlayState;

	public:

		enum MinimapIconType {
			MINIMAP_WALL,
			MINIMAP_DOOR,
			MINIMAP_DOOR_RED,
			MINIMAP_DOOR_GREEN,
			MINIMAP_DOOR_BLUE,
			MINIMAP_DOOR_BOSS,
			MINIMAP_DOOR_OPEN,
			MINIMAP_AMMO,
			MINIMAP_MEDKIT,
			MINIMAP_EQUIPMENT,
			MINIMAP_UNIQUE,
			MINIMAP_KEY,
			MINIMAP_CRATE,
			MINIMAP_MONSTER,
			MINIMAP_PROJECTILE,
			MINIMAP_PLAYER,
			MINIMAP_COUNT
		};

		_Map();
		_Map(const std::string &Filename, double Clock=0.0, int Progression=0);
		~_Map();

		void InitializeTiles();
		bool Save(const std::string &String);

		void Update(double FrameTime, double Clock);

		bool ResolveTileCollisions(const glm::vec2 &TargetPosition, float Radius, int CollisionFlag, int &Bounces, glm::vec2 &NewPosition, glm::vec2 &Velocity);
		std::vector<_Hit> &CheckCollisionsInGrid(const glm::vec2 &Position, float Radius, const std::vector<int> &GridTypes);
		std::vector<_Hit> &ResolveCollisionsInGrid(const glm::vec2 &Position, float Radius, const _Object *SkipObject, bool &AxisAlignedPush, float PushFactor=1.0f);
		_Item *GetClosestItem(const glm::vec2 &Position, bool SkipHideable) const;
		void GetCloseObjects(const glm::vec2 &Position, float Radius, int GridType, std::unordered_map<_Object *, int> &Objects, _Object **ClosestObject) const;
		void CheckMeleeCollisions(_Entity *Attacker, int GridType, int Penetration, std::vector<_Hit> &Hits);
		void CheckBulletCollisions(_Object *Attacker, const glm::vec2 &Direction, std::vector<_Hit> &Hits, int GridType, bool TestObjects, int Penetration, int CollisionFlag);
		void GetDropPosition(_Object *Player, float MaxDistance, glm::vec2 &WorldPosition) const;
		bool IsVisible(const glm::vec2 &Start, const glm::vec2 &End, int CheckFlag) const;
		bool CanMoveTo(const glm::vec2 &Start, const glm::vec2 &End, const glm::vec2 &Size) const;
		void AddObjectToGrid(_Object *Object, int Type);
		void RemoveObjectFromGrid(_Object *Object, int Type);

		void ChangeMapState(const _Event *Event);
		bool CanChangeMapState(const _Event *Event);
		void ToggleEventActive(size_t Index);
		void SwapBlockTextures(int Layer, int Index);
		bool HasEvents(const glm::ivec2 &Position) const;

		void SetAmbientLight(const std::string &ColorID);

		int RenderFloors();
		int RenderWalls(bool SkipFloating=false);
		int RenderFlatWalls();
		int RenderParticles(int Type, double BlendFactor);
		int RenderForeground(const glm::vec2 &PlayerPosition, bool AlwaysFade);
		int RenderProps();
		void RenderEvents(std::vector<const ae::_Texture *> &Textures, int Type);
		void RenderGrid(int Mode);
		void DrawMinimap(bool FullMap, ae::_Bounds &MinimapBounds);
		void HighlightBlocks(int Layer);

		void AddBlock(int Layer, _Block Block) { Blocks[Layer].push_back(Block); }
		void AddEvent(_Event *Event) { Events.push_back(Event); }
		void AddParticle(_Particle *Particle);
		void GetSelectedObject(const glm::vec2 &Position, float RadiusSquared, _ObjectSpawn **Object, size_t *Index);
		void GetSelectedObjects(const glm::vec2 &Start, const glm::vec2 &End, std::vector<_ObjectSpawn *> *SelectedObjects, int Type);
		void GetSelectedBlocks(const glm::vec2 &Start, const glm::vec2 &End, int Layer, std::vector<size_t> &SelectedBlocks, glm::ivec4 &SelectionBounds);
		size_t GetSelectedBlock(int Layer, const glm::ivec2 &Index, _Block **Block);
		size_t GetSelectedBlock(int Layer, const glm::ivec2 &Index);
		int GetSelectedEvent(const glm::ivec2 &Index, int Type, _Event **Event);
		size_t ChangeLayer(int OldLayer, int NewLayer, int Index);
		void DeleteBlockIDFromTiles(int Layer, int Index);
		void RemoveEvent(int Index);
		void DeleteBlocks(int Layer, std::vector<size_t> &BlockIDs);
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
		_Block *GetBlock(int Layer, const size_t Index);
		glm::vec2 GetValidPosition(const glm::vec2 &Position) const;

		void AddObject(_Object *Object, int GridType);
		void RemoveObject(_Object *Object, int GridType);

		bool CheckMinimapBounds(const glm::vec4 &Bounds);
		void AddMinimapIcons();

		int GetAddedLevel() const;
		static glm::vec2 GenerateRandomPointInCircle(float Radius);
		static std::string FixFilename(const std::string &Filename);

		// Stats
		std::string Filename;
		std::string Name;
		glm::vec4 BaseAmbientLight{0.5f, 0.5f, 0.5f, 1.0f};
		glm::ivec2 Size{0, 0};
		int MapType{MAPTYPE_CAMPAIGN};
		int Progression{0};
		int Level{1};
		int Monsters{0};
		int Crates{0};
		int Secrets{0};
		int TotalExperience{0};
		bool BaseAmbientClock{false};
		bool AmbientClock{false};
		bool SimpleAI{false};

		// Objects
		ae::_Camera *Camera{nullptr};
		std::unique_ptr<_ObjectManager> ObjectManager;
		std::vector<_ObjectSpawn *> ObjectSpawns;
		std::vector<_Event *> Events;
		std::vector<_Hit> CollisionHits;

		// Minimap
		std::vector<_MinimapIcon> MinimapIcons[MINIMAP_COUNT];
		glm::vec2 MinimapCaptureSize{0.0f};
		uint32_t MinimapVBO{0};
		float *MinimapVertices{nullptr};

		// Lights
		glm::vec4 AmbientLight;
		glm::vec4 TargetAmbientLight;

	private:

		void GetClockLight(double Clock, glm::vec4 &LightColor);
		void UpdateAmbientLight(double FrameTime, double Clock);
		bool CheckAABBCollision(const glm::vec2 &Position, float Radius, const float *AABB, bool Resolve, _Hit &Hit) const;

		// Blocks
		_Tile **Data{nullptr};
		std::vector<_Block> Blocks[MAPLAYER_COUNT];
		std::vector<_Event *> CheckpointEvents;

		// Objects
		std::vector<_Particle *> Particles;
		std::unordered_map<_Object *, int> ObjectMap;
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
