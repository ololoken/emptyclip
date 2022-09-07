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
#include <ae/ui.h>
#include <gameassets.h>
#include <events.h>
#include <stats.h>
#include <objectmanager.h>
#include <constants.h>
#include <fstream>
#include <stdexcept>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <zlib/zfstream.h>

inline bool CompareHitDistance(_Hit &First, _Hit &Second) {
	return First.DistanceSquared < Second.DistanceSquared;
}

// Colors of each time cycle
static const std::vector<glm::vec4> DayCycles = {
	{ 0.0625f, 0.0625f, 0.375f,  1 },
	{ 0.125f,  0.125f,  0.125f,  1 },
	{ 0.75f,   0.75f,   0.5625f, 1 },
	{ 0.6875f, 0.5625f, 0.375f,  1 },
	{ 0.625f,  0.5f,    0.375f,  1 },
};

// Time of each cycle change
static const std::vector<double> DayCyclesTime = {
	0.0  * 60.0,
	6.0  * 60.0,
	12.5 * 60.0,
	16.5 * 60.0,
	18.0 * 60.0,
};

// Colors for highlighting blocks at different MinZ values
static const std::vector<glm::vec4> HighlightColors = {
	COLOR_MAGENTA,
	COLOR_YELLOW,
	COLOR_CYAN,
	COLOR_RED,
	COLOR_GREEN,
	COLOR_BLUE,
};

// Initialize
_Map::_Map() :
	Size{MAP_WIDTH, MAP_HEIGHT},
	ObjectManager(new _ObjectManager()),
	MinimapCaptureSize(HUD_MINIMAP_CAPTURE_SIZE),
	AmbientLight(BaseAmbientLight),
	TargetAmbientLight(BaseAmbientLight) {

	ObjectMap.reserve(10);
	CollisionHits.reserve(10);
}

// Initialize
_Map::_Map(const std::string &Filename, double Clock, int Progression) : _Map() {
	if(Filename.empty())
		throw std::runtime_error(std::string(__func__) + " empty file name");

	this->Progression = Progression;
	this->Filename = FixFilename(Filename);
	int SpawnMultiplier = std::min(Progression + 1, GAME_MAX_PROGRESSION_SPAWN);

	// Used when resizing maps
	glm::ivec2 Offset(0, 0);
	bool Adjust = (Offset.x != 0 && Offset.y != 0);

	// Load file
	gzifstream File(("maps/" + this->Filename).c_str(), std::ios::in);
	if(!File)
		throw std::runtime_error(std::string(__func__) + " error opening '" + this->Filename + "'");

	// Read file
	_ObjectSpawn *ObjectSpawn = nullptr;
	_Event *Event = nullptr;
	_Block *Block = nullptr;
	_EventTile *EventTile = nullptr;
	while(!File.eof() && File.peek() != EOF) {

		// Read chunk type
		char ChunkType;
		File >> ChunkType;

		// Handle chunk type
		switch(ChunkType) {
			// Header
			case 'H': {
				char SubChunkType;
				File >> SubChunkType;
				switch(SubChunkType) {
					// Version
					case 'v': {
						int FileVersion;
						File >> FileVersion;
						if(FileVersion != MAP_FILEVERSION)
							throw std::runtime_error(std::string(__func__) + " level version mismatch '" + std::to_string(FileVersion) + "'");
					} break;
					// Level used for default item/monster levels
					case 'l': {
						File >> Level;
					} break;
					// Type
					case 't': {
						File >> MapType;
					} break;
					// Size
					case 's': {
						File >> Size.x >> Size.y;
					} break;
					case 'n': {
						File.ignore(1);
						std::getline(File, Name, '\n');
					} break;
					// Ambient light
					case 'a': {
						File >> BaseAmbientLight.r >> BaseAmbientLight.g >> BaseAmbientLight.b;
						AmbientLight = TargetAmbientLight = BaseAmbientLight;
					} break;
					// Ambient light uses day/night cycle
					case 'c': {
						File >> BaseAmbientClock;
						AmbientClock = BaseAmbientClock;
					} break;
					// Benchmark flag
					case 'b': {
						File >> SimpleAI;
					} break;
				}
			} break;
			// Objects
			case 'O': {
				char SubChunkType;
				File >> SubChunkType;
				switch(SubChunkType) {
					// New
					case 'n': {
						ObjectSpawn = new _ObjectSpawn();
						ObjectSpawns.push_back(ObjectSpawn);
					} break;
					// ID
					case 'i': {
						File.ignore(1);
						std::getline(File, ObjectSpawn->ID, '\n');
						if(Stats.Objects.find(ObjectSpawn->ID) == Stats.Objects.end())
							throw std::runtime_error(std::string(__func__) + " Unknown object '" + ObjectSpawn->ID + "'");

						ObjectSpawn->Type = Stats.Objects.at(ObjectSpawn->ID).Type;
					} break;
					// Level
					case 'l': {
						File >> ObjectSpawn->Level;
					} break;
					// Position
					case 'p': {
						glm::vec2 Position;
						File >> Position.x >> Position.y;
						ObjectSpawn->Position = Position;
						if(Adjust)
							ObjectSpawn->Position += glm::vec2(Offset);
					} break;
					// Rotation
					case 'r': {
						File >> ObjectSpawn->Rotation;
					} break;
					// Scale
					case 's': {
						File >> ObjectSpawn->Scale;
					} break;
				}
			} break;
			// Events
			case 'E': {
				char SubChunkType;
				File >> SubChunkType;
				switch(SubChunkType) {
					// New
					case 'n': {
						Event = new _Event();
						Events.push_back(Event);
					} break;
					// Type
					case 't': {
						File >> Event->Type;
					} break;
					// Active
					case 'a': {
						File >> Event->Active;
					} break;
					// Bounds
					case 'b': {
						File >> Event->Start.x >> Event->Start.y >> Event->End.x >> Event->End.y;
						if(Adjust) {
							Event->Start += Offset;
							Event->End += Offset;
						}
					} break;
					// Level
					case 'l': {
						File >> Event->Level;
					} break;
					// Spawn level
					case 's': {
						File >> Event->SpawnLevel;
					} break;
					// Activation period
					case 'p': {
						File >> Event->ActivationPeriod;
					} break;
					// Generic ID
					case 'I': {
						File.ignore(1);
						std::getline(File, Event->ItemID, '\n');
					} break;
					// Monster ID
					case 'M': {
						File.ignore(1);
						std::getline(File, Event->MonsterID, '\n');
						if(Stats.Objects.find(Event->MonsterID) == Stats.Objects.end())
							throw std::runtime_error(std::string(__func__) + " unknown monster '" + Event->MonsterID + "'");
					} break;
					// Particle ID
					case 'P': {
						File.ignore(1);
						std::getline(File, Event->ParticleID, '\n');
						if(GameAssets.Particles.find(Event->ParticleID) == GameAssets.Particles.end())
							throw std::runtime_error(std::string(__func__) + " unknown particle '" + Event->ParticleID + "'");
					} break;
					// Sound ID
					case 'S': {
						File.ignore(1);
						std::getline(File, Event->SoundID, '\n');
						if(ae::Assets.Sounds.find(Event->SoundID) == ae::Assets.Sounds.end())
							throw std::runtime_error(std::string(__func__) + " unknown sound '" + Event->SoundID + "'");
					} break;
				}
			} break;
			// Event data
			case 'D': {
				char SubChunkType;
				File >> SubChunkType;
				switch(SubChunkType) {
					// New
					case 'n': {
						Event->Tiles.push_back(_EventTile());
						EventTile = &Event->Tiles.back();
					} break;
					// Position
					case 'p': {
						File >> EventTile->Coord.x >> EventTile->Coord.y;
						if(Adjust)
							EventTile->Coord += Offset;
					} break;
					// Layer
					case 'l': {
						File >> EventTile->Layer;
					} break;
					// Block ID
					case 'B': {
						File >> EventTile->BlockID;
					} break;
				}
			} break;
			// Blocks
			case 'B': {
				char SubChunkType;
				File >> SubChunkType;
				switch(SubChunkType) {
					// New
					case 'n': {
						int Layer;
						File >> Layer;
						Blocks[Layer].push_back(_Block());
						Block = &Blocks[Layer].back();
					} break;
					// Bounds
					case 'b': {
						File >> Block->Start.x >> Block->Start.y >> Block->End.x >> Block->End.y >> Block->MinZ >> Block->MaxZ;
						if(Adjust) {
							Block->Start += Offset;
							Block->End += Offset;
						}
					} break;
					// Color
					case 'c': {
						File >> Block->Color.r >> Block->Color.g >> Block->Color.b >> Block->Color.a;
					} break;
					// Rotation
					case 'r': {
						File >> Block->Rotation;
					} break;
					// Mirrored
					case 'm': {
						File >> Block->ScaleX;
					} break;
					// Walkable
					case 'w': {
						File >> Block->Walkable;
					} break;
					// Texture
					case 't': {
						char TextureType;
						File >> TextureType;

						File.ignore(1);
						std::string TexturePath;
						std::getline(File, TexturePath, '\n');
						if(TextureType == '1') {
							Block->Texture = ae::Assets.Textures[TexturePath];
							if(!Block->Texture)
								throw std::runtime_error(std::string(__func__) + " unknown texture  '" + TexturePath + "'");
						}
						else {
							Block->AltTexture = ae::Assets.Textures[TexturePath];
							if(!Block->AltTexture)
								throw std::runtime_error(std::string(__func__) + " unknown texture  '" + TexturePath + "'");
						}
					} break;
				}
			} break;
		}
	}

	// Add to stats
	for(const auto &Event : Events) {
		switch(Event->Type) {
			case EVENT_SPAWN: {
				Event->Level = std::max(1, Event->Level);
				if(!Event->MonsterID.empty()) {
					_ObjectTemplate &Template = Stats.Objects.at(Event->MonsterID);
					if(Template.Type == _Object::MONSTER) {
						int TileCount = (int)Event->Tiles.size();
						if(Template.Attributes.at("ai_type").Int) {
							Monsters += TileCount * Event->Level * SpawnMultiplier;
							int Experience = Template.Attributes.at("xp").Float + Template.Attributes.at("xp_level").Float * (GetAddedLevel() + Event->SpawnLevel - 1);
							Experience *= TileCount * Event->Level * SpawnMultiplier;
							TotalExperience += Experience;
							Event->SpawnMultiplier = SpawnMultiplier;
						}
						else {
							Crates += TileCount * Event->Level;
						}
					}
				}
			} break;
			case EVENT_CHECK:
				CheckpointEvents.push_back(Event);
			break;
			case EVENT_SECRET:
				Secrets++;
			break;
		}
	}

	// Set up lights
	if(BaseAmbientClock)
		GetClockLight(Clock, AmbientLight);

	File.close();
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

	// Delete tile data
	if(Data) {
		for(int i = 0; i < Size.x; i++)
			delete[] Data[i];
		delete[] Data;
	}
}

// Saves the level to a file
bool _Map::Save(const std::string &String) {

	// Add extension
	Filename = FixFilename(String);

	// Open gz output stream
	gzofstream File(("maps/" + Filename).c_str(), std::ios::out);
	if(!File)
		throw std::runtime_error(std::string(__func__) + " error opening '" + Filename + "'");

	File << std::showpoint << std::fixed << std::setprecision(2);

	// Header
	File << "Hv " << MAP_FILEVERSION << '\n';
	File << "Hl " << Level << '\n';
	File << "Ht " << MapType << '\n';
	File << "Hs " << Size.x << ' ' << Size.y << '\n';
	File << "Hn " << Name << '\n';
	File << "Ha " << BaseAmbientLight.r << ' ' << BaseAmbientLight.g << ' ' << BaseAmbientLight.b << '\n';
	File << "Hc " << AmbientClock << '\n';
	File << "Hb " << SimpleAI << '\n';

	// Objects
	for(const auto &ObjectSpawn : ObjectSpawns) {
		File << "On" << '\n';
		File << "Oi " << ObjectSpawn->ID << '\n';
		File << "Ol " << ObjectSpawn->Level << '\n';
		File << "Op " << ObjectSpawn->Position.x << ' ' << ObjectSpawn->Position.y << '\n';
		if(ObjectSpawn->Rotation != 0.0f)
			File << "Or " << ObjectSpawn->Rotation << '\n';
		if(ObjectSpawn->Scale != 1.0f)
			File << "Os " << ObjectSpawn->Scale << '\n';
	}

	// Events
	for(const auto &Event : Events) {
		File << "En" << '\n';
		File << "Et " << Event->Type << '\n';
		File << "Ea " << Event->Active << '\n';
		File << "Eb " << Event->Start.x << ' ' << Event->Start.y << ' ' << Event->End.x << ' ' << Event->End.y << '\n';
		File << "El " << Event->Level << '\n';
		File << "Es " << Event->SpawnLevel << '\n';
		File << "Ep " << Event->ActivationPeriod << '\n';
		if(Event->ItemID.size())
			File << "EI " << Event->ItemID << '\n';
		if(Event->MonsterID.size())
			File << "EM " << Event->MonsterID << '\n';
		if(Event->ParticleID.size())
			File << "EP " << Event->ParticleID << '\n';
		if(Event->SoundID.size())
			File << "ES " << Event->SoundID << '\n';

		// Event tiles
		for(const auto &Tile : Event->Tiles) {
			File << "Dn" << '\n';
			File << "Dp " << Tile.Coord.x << ' ' << Tile.Coord.y << '\n';
			File << "Dl " << Tile.Layer << '\n';
			File << "DB " << Tile.BlockID << '\n';
		}
	}

	// Blocks
	for(int i = 0; i < MAPLAYER_COUNT; i++) {
		for(auto &Block : Blocks[i]) {
			File << "Bn" << i << '\n';
			File << "Bb " << Block.Start.x << ' ' << Block.Start.y << ' ' << Block.End.x << ' ' << Block.End.y << ' ' << Block.MinZ << ' ' << Block.MaxZ << '\n';
			File << "Bc" << Block.Color.r << ' ' << Block.Color.g << ' ' << Block.Color.b << ' ' << Block.Color.a << '\n';
			File << "Br " << Block.Rotation << '\n';
			File << "Bm " << Block.ScaleX << '\n';
			File << "Bw " << Block.Walkable << '\n';
			if(Block.Texture)
				File << "Bt1 " << Block.Texture->Name << '\n';
			if(Block.AltTexture)
				File << "Bt2 " << Block.AltTexture->Name << '\n';
		}
	}

	File.close();

	return true;
}

// Create tile data
void _Map::InitializeTiles() {

	// Allocate memory
	Data = new _Tile*[(size_t)Size.x];
	for(int i = 0; i < Size.x; i++)
		Data[i] = new _Tile[(size_t)Size.y];

	// Clear out array
	for(int i = 0; i < Size.x; i++) {
		for(int j = 0; j < Size.y; j++) {
			Data[i][j] = _Tile();
		}
	}

	// Loop through floor layers and fill out walkable field
	for(int l = 0; l < MAPLAYER_FLAT; l++) {
		for(size_t k = 0; k < Blocks[l].size(); k++) {
			for(int i = Blocks[l][k].Start.x; i <= Blocks[l][k].End.x; i++) {
				for(int j = Blocks[l][k].Start.y; j <= Blocks[l][k].End.y; j++) {
					if(Blocks[l][k].Walkable)
						Data[i][j].Collision &= ~_Tile::ENTITY;
					else
						Data[i][j].Collision |= _Tile::ENTITY;

					// Don't allow changing of bullet flags
					Data[i][j].CollisionChangeMask &= ~_Tile::BULLET;
				}
			}
		}
	}

	// Loop through walls
	for(size_t k = 0; k < Blocks[MAPLAYER_WALL].size(); k++) {
		for(int i = Blocks[MAPLAYER_WALL][k].Start.x; i <= Blocks[MAPLAYER_WALL][k].End.x; i++) {
			for(int j = Blocks[MAPLAYER_WALL][k].Start.y; j <= Blocks[MAPLAYER_WALL][k].End.y; j++) {
				if(Blocks[MAPLAYER_WALL][k].Walkable)
					Data[i][j].Collision &= ~_Tile::ENTITY & ~_Tile::BULLET;
				else
					Data[i][j].Collision |= _Tile::ENTITY | _Tile::BULLET;

				// Walls override floor change masks
				Data[i][j].CollisionChangeMask = _Tile::ENTITY | _Tile::BULLET;
			}
		}
	}

	// Flat layer overrides existing collision flags if MinZ <= 0
	for(size_t k = 0; k < Blocks[MAPLAYER_FLAT].size(); k++) {
		for(int i = Blocks[MAPLAYER_FLAT][k].Start.x; i <= Blocks[MAPLAYER_FLAT][k].End.x; i++) {
			for(int j = Blocks[MAPLAYER_FLAT][k].Start.y; j <= Blocks[MAPLAYER_FLAT][k].End.y; j++) {
				if(Blocks[MAPLAYER_FLAT][k].MinZ > 0)
					continue;

				if(Blocks[MAPLAYER_FLAT][k].Walkable)
					Data[i][j].Collision = 0;
				else
					Data[i][j].Collision = _Tile::ENTITY;

				// Don't allow changing of bullet flags
				Data[i][j].CollisionChangeMask &= ~_Tile::BULLET;
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

// Adds an object to the collision grid
void _Map::AddObjectToGrid(_Object *Object, int Type) {

	// Get the object's bounding rectangle
	_TileBounds TileBounds;
	GetTileBounds(Object->Position, Object->Radius, TileBounds);

	for(int i = TileBounds.Start.x; i <= TileBounds.End.x; i++) {
		for(int j = TileBounds.Start.y; j <= TileBounds.End.y; j++) {
			Data[i][j].Objects[Type][Object] = 1;
		}
	}
}

// Removes an object from the collision grid
void _Map::RemoveObjectFromGrid(_Object *Object, int Type) {

	// Get the object's bounding rectangle
	_TileBounds TileBounds;
	GetTileBounds(Object->Position, Object->Radius, TileBounds);

	for(int i = TileBounds.Start.x; i <= TileBounds.End.x; i++) {
		for(int j = TileBounds.Start.y; j <= TileBounds.End.y; j++) {
			Data[i][j].Objects[Type].erase(Object);
		}
	}
}

// Check collision with tiles and resolve
bool _Map::ResolveTileCollisions(const glm::vec2 &TargetPosition, float Radius, int CollisionFlag, glm::vec2 &NewPosition) {

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
	if(Right >= (float)Size.x) {
		Right = NewPosition.x = (float)Size.x - Radius;
		Touching = true;
	}
	if(Bottom >= (float)Size.y) {
		Bottom = NewPosition.y = (float)Size.y - Radius;
		Touching = true;
	}

	// Check tiles
	int LeftTile = (int)Left;
	int RightTile = (int)Right;
	int TopTile = (int)Top;
	int BottomTile = (int)Bottom;

	CollisionHits.clear();
	bool AxisAlignedPush = false;
	for(int i = LeftTile; i <= RightTile; i++) {
		for(int j = TopTile; j <= BottomTile; j++) {
			if(!(Data[i][j].Collision & CollisionFlag))
				continue;

			float AABB[4] = { (float)i, (float)j, i + 1.0f, j + 1.0f };
			_Hit Hit;
			Hit.AxisAlignedPush = false;
			if(CheckAABBCollision(NewPosition, Radius, AABB, true, Hit)) {
				Touching = true;
				CollisionHits.push_back(Hit);

				// Flag at least one axis aligned push
				if(Hit.AxisAlignedPush)
					AxisAlignedPush = true;
			}
		}
	}

	// Resolve collision
	for(const auto &Hit : CollisionHits) {
		if(!(AxisAlignedPush && Hit.Push.x != 0 && Hit.Push.y != 0)) {
			NewPosition += Hit.Push;
		}
	}

	return Touching;
}

// Resolve collision with an axis aligned bounding box
bool _Map::CheckAABBCollision(const glm::vec2 &Position, float Radius, const float *AABB, bool Resolve, _Hit &Hit) const {
	int ClampCount = 0;

	// Get closest point on AABB
	Hit.ClosetPoint = Position;
	if(Hit.ClosetPoint.x < AABB[0]) {
		Hit.ClosetPoint.x = AABB[0];
		ClampCount++;
	}
	if(Hit.ClosetPoint.y < AABB[1]) {
		Hit.ClosetPoint.y = AABB[1];
		ClampCount++;
	}
	if(Hit.ClosetPoint.x > AABB[2]) {
		Hit.ClosetPoint.x = AABB[2];
		ClampCount++;
	}
	if(Hit.ClosetPoint.y > AABB[3]) {
		Hit.ClosetPoint.y = AABB[3];
		ClampCount++;
	}

	// Test circle collision with point
	float DistanceSquared = glm::distance2(Hit.ClosetPoint, Position);
	bool Touching = DistanceSquared < Radius * Radius;

	// Push object out
	Hit.Push.x = 0.0f;
	Hit.Push.y = 0.0f;
	if(Touching && Resolve) {

		// Check if object is inside the AABB
		if(ClampCount == 0) {
			glm::vec2 Center((AABB[0] + AABB[2]) * 0.5f, (AABB[1] + AABB[3]) * 0.5f);

			// Push left or right
			if(Position.x <= Center.x)
				Hit.Push.x = AABB[0] - Position.x - Radius;
			else if(Position.x > Center.x)
				Hit.Push.x = AABB[2] - Position.x + Radius;

			// Push up or down
			if(Position.y <= Center.y)
				Hit.Push.y = AABB[1] - Position.y - Radius;
			else if(Position.y > Center.y)
				Hit.Push.y = AABB[3] - Position.y + Radius;
		}
		else {

			// Get push direction
			Hit.Push = Position - Hit.ClosetPoint;

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

	// Get the object's bounding rectangle
	_TileBounds TileBounds;
	GetTileBounds(Position, Radius, TileBounds);

	// Iterate through tiles covered by the bounds
	for(int i = TileBounds.Start.x; i <= TileBounds.End.x; i++) {
		for(int j = TileBounds.Start.y; j <= TileBounds.End.y; j++) {
			for(auto Iterator : Data[i][j].Objects[GridType]) {
				_Object *Object = Iterator.first;

				// Check circle intersection
				float RadiiSum = Object->Radius + Radius;
				if(glm::distance2(Object->Position, Position) < RadiiSum * RadiiSum)
					return Object;
			}
		}
	}

	return nullptr;
}

// Return objects that are touching a circle
void _Map::GetCloseObjects(const glm::vec2 &Position, float Radius, int GridType, std::unordered_map<_Object *, int> &Objects, _Object **ClosestObject) const {

	// Get bounding rectangle
	_TileBounds TileBounds;
	GetTileBounds(Position, Radius, TileBounds);

	// Iterate through tiles covered by the bounds
	float ClosestDistance = HUGE_VAL;
	for(int i = TileBounds.Start.x; i <= TileBounds.End.x; i++) {
		for(int j = TileBounds.Start.y; j <= TileBounds.End.y; j++) {
			for(auto Iterator : Data[i][j].Objects[GridType]) {
				_Object *Object = Iterator.first;

				// Check distance
				float RadiiSum = Object->Radius + Radius;
				float DistanceSquared = glm::distance2(Object->Position, Position);
				if(DistanceSquared >= RadiiSum * RadiiSum)
					continue;

				Objects[Object] = 1;

				// Keep track of closest object
				if(DistanceSquared < ClosestDistance) {
					ClosestDistance = DistanceSquared;
					*ClosestObject = Object;
				}
			}
		}
	}
}

// Check for collisions in a grid
std::vector<_Hit> &_Map::CheckCollisionsInGrid(const glm::vec2 &Position, float Radius, int GridType) {

	// Get the object's bounding rectangle
	_TileBounds TileBounds;
	GetTileBounds(Position, Radius, TileBounds);

	// Get unique list of objects to check against
	ObjectMap.clear();
	for(int i = TileBounds.Start.x; i <= TileBounds.End.x; i++) {
		for(int j = TileBounds.Start.y; j <= TileBounds.End.y; j++) {
			for(auto &Iterator : Data[i][j].Objects[GridType]) {
				_Object *Object = Iterator.first;
				if(Object->IsDying())
					continue;

				ObjectMap[Object] = 1;
			}
		}
	}

	// Check potential objects
	CollisionHits.clear();
	for(const auto &HitObjects : ObjectMap) {
		_Hit Hit;
		Hit.Object = HitObjects.first;
		if(Hit.Object->IsTouchingCircle(Position, Radius, Hit.DistanceSquared)) {
			Hit.Position = Hit.Object->Position;
			CollisionHits.push_back(Hit);
		}
	}

	// Sort by distance
	if(CollisionHits.size() > 1)
		std::sort(CollisionHits.begin(), CollisionHits.end(), CompareHitDistance);

	return CollisionHits;
}

// Returns a list of entities that an object is colliding with
std::vector<_Hit> &_Map::ResolveCollisionsInGrid(const glm::vec2 &Position, float Radius, const _Object *SkipObject, bool &AxisAlignedPush, float PushFactor) {

	// Get the object's bounding rectangle
	_TileBounds TileBounds;
	GetTileBounds(Position, Radius, TileBounds);

	// Get unique list of objects to check against
	ObjectMap.clear();
	for(int i = TileBounds.Start.x; i <= TileBounds.End.x; i++) {
		for(int j = TileBounds.Start.y; j <= TileBounds.End.y; j++) {
			for(int k = GRID_PLAYER; k <= GRID_MONSTER; k++) {
				for(auto &Iterator : Data[i][j].Objects[k]) {
					_Object *Object = Iterator.first;
					if(Object == SkipObject || Object->IsDying() || Object->CanFreePath())
						continue;

					ObjectMap[Object] = 1;
				}
			}
		}
	}

	// Get push vectors for each hit object
	CollisionHits.clear();
	for(const auto &HitEntity : ObjectMap) {
		_Object *Object = HitEntity.first;
		if(Object->Circle) {
			float DistanceSquared = glm::distance2(Object->Position, Position);
			float RadiiSum = Object->Radius + Radius;

			// Check circle intersection
			if(DistanceSquared < RadiiSum * RadiiSum) {
				glm::vec2 CenterVector = Position - Object->Position;

				_Hit Hit;
				if(CenterVector.x == 0.0f && CenterVector.y == 0.0f) {
					Hit.Push.x = PushFactor;
					Hit.Push.y = 0.0f;
				}
				else {
					Hit.Push = glm::normalize(CenterVector);
					Hit.Push *= (RadiiSum - sqrtf(DistanceSquared)) * PushFactor;
				}
				CollisionHits.push_back(Hit);
			}
		}
		else {

			// Get AABB of object
			float AABB[4] = {
				Object->Position.x - Object->Radius,
				Object->Position.y - Object->Radius,
				Object->Position.x + Object->Radius,
				Object->Position.y + Object->Radius
			};

			_Hit Hit;
			Hit.AxisAlignedPush = false;
			if(CheckAABBCollision(Position, Radius, AABB, true, Hit)) {
				Hit.Push *= PushFactor;
				CollisionHits.push_back(Hit);
				if(Hit.AxisAlignedPush)
					AxisAlignedPush = true;
			}
		}
	}

	return CollisionHits;
}

// Checks for melee collisions with entities in the collision grid
void _Map::CheckMeleeCollisions(_Entity *Attacker, int GridType, int Penetration, std::vector<_Hit> &Hits) {

	// Get the object's bounding rectangle
	_TileBounds TileBounds;
	GetTileBounds(Attacker->Position, Attacker->AttackRange[Attacker->AttackRequestType], TileBounds);

	// Set up player attacking calculations
	float AttackRange = Attacker->AttackRange[Attacker->AttackRequestType];
	glm::vec2 LeftLineStart;
	glm::vec2 RightLineStart;
	glm::vec2 Direction;
	if(Attacker->Type == _Object::PLAYER) {

		// Get attacker direction and normal
		Direction = Attacker->GetDirectionVector();
		glm::vec2 NormalDirection(-Direction.y, Direction.x);

		// Get starting points of attack ranges
		float AttackWidth = Attacker->AttackWidth[Attacker->AttackRequestType];
		LeftLineStart = Attacker->Position - NormalDirection * (AttackWidth - Attacker->MeleeOffset);
		RightLineStart = Attacker->Position + NormalDirection * (AttackWidth + Attacker->MeleeOffset);
	}

	// Check tiles for objects
	ObjectMap.clear();
	for(int i = TileBounds.Start.x; i <= TileBounds.End.x; i++) {
		for(int j = TileBounds.Start.y; j <= TileBounds.End.y; j++) {
			for(auto &Iterator : Data[i][j].Objects[GridType]) {
				_Object *Object = Iterator.first;
				if(Object->Type == _Object::PROP)
					continue;

				// Check unique list of entities
				if(ObjectMap.find(Object) != ObjectMap.end())
					continue;

				// Add to list of hit entities
				ObjectMap[Object] = 1;

				// Check if dying
				if(Object->IsDying())
					continue;

				// Test if attacker is within attack range
				float DistanceSquared = HUGE_VAL;
				if(!Object->IsTouchingCircle(Attacker->Position, AttackRange, DistanceSquared))
					continue;

				// Do additional tests when the player is attacking
				if(Attacker->Type == _Object::PLAYER) {

					// Test left side
					float Time = Object->RayIntersection(LeftLineStart, Direction);
					if(Time >= AttackRange) {

						// Test right side
						Time = Object->RayIntersection(RightLineStart, Direction);
						if(Time >= AttackRange)
							continue;
					}

				/*
					// Test angle
					glm::vec2 Direction = Attacker->GetDirectionVector();
					glm::vec2 ObjectDirection(glm::normalize(Entity->Position - Attacker->Position));
					std::cout << glm::dot(Direction, CenterDirection) << std::endl;
					if(glm::dot(Direction, CenterDirection) <= cosf(glm::radians(Attacker->MaxAccuracy[Attacker->AttackRequestType] * 0.5f)))
						continue;
				*/

					// Check for walls
					if(!IsVisible(Attacker->Position, Object->Position, _Tile::BULLET))
						continue;
				}

				// Add to potential hits
				_Hit Hit(HIT_OBJECT);
				Hit.Object = Object;
				Hit.Position = Object->Position;
				Hit.DistanceSquared = DistanceSquared;
				Hits.push_back(Hit);

				// Early exit for monsters
				if(Attacker->Type == _Object::MONSTER)
					return;
			}
		}
	}

	// Check for hits
	if(Hits.empty())
		return;

	// Sort by distance
	if(Hits.size() > 1)
		std::sort(Hits.begin(), Hits.end(), CompareHitDistance);

	// Handle penetration
	if(Hits.size() > (size_t)Penetration)
		Hits.resize((size_t)Penetration);
}

// Determines which walls are adjacent to the object
int _Map::GetWallState(const glm::vec2 &Position, float Radius) const {

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
void _Map::CheckBulletCollisions(_Object *Attacker, const glm::vec2 &Direction, std::vector<_Hit> &Hits, int GridType, bool TestObjects, int Penetration, int CollisionFlag) {

	// Initialize state
	_Hit Hit;
	if(TestObjects) {
		Hit.Normal = -Direction;
		ObjectMap.clear();
		ObjectMap[Attacker] = 1;
	}

	// Find starting tile
	glm::ivec2 TileTracer = GetValidCoord(Attacker->Position);

	// Check x direction
	glm::ivec2 TileIncrement;
	glm::ivec2 FirstBoundaryTile;
	if(Direction.x < 0) {
		FirstBoundaryTile.x = TileTracer.x;
		TileIncrement.x = -1;
	}
	else {
		FirstBoundaryTile.x = TileTracer.x + 1;
		TileIncrement.x = 1;
	}

	// Check y direction
	if(Direction.y < 0) {
		FirstBoundaryTile.y = TileTracer.y;
		TileIncrement.y = -1;
	}
	else {
		FirstBoundaryTile.y = TileTracer.y + 1;
		TileIncrement.y = 1;
	}

	// Find ray direction ratios
	glm::vec2 Ratio(1.0f / Direction.x, 1.0f / Direction.y);

	// Calculate increments
	glm::vec2 Increment(TileIncrement.x * Ratio.x, TileIncrement.y * Ratio.y);

	// Get starting positions
	glm::vec2 Tracer((FirstBoundaryTile.x - Attacker->Position.x) * Ratio.x, (FirstBoundaryTile.y - Attacker->Position.y) * Ratio.y);

	// Traverse tiles
	bool EndedOnX = false;
	while(TileTracer.x >= 0 && TileTracer.y >= 0 && TileTracer.x < Size.x && TileTracer.y < Size.y && CheckCollisionFlag(TileTracer, CollisionFlag)) {

		// Check for object intersections
		if(TestObjects) {
			for(auto &Iterator : Data[TileTracer.x][TileTracer.y].Objects[GridType]) {
				_Object *Object = Iterator.first;
				if(Object->IsDying() || ObjectMap.find(Object) != ObjectMap.end())
					continue;

				// Only check object once
				ObjectMap[Object] = 1;

				// Get distance to object
				float Distance = Object->RayIntersection(Attacker->Position, Direction);
				if(Distance == HUGE_VAL)
					continue;

				// Add to hits
				Hit.Type = (Object->Type == _Object::PROP) ? HIT_WALL : HIT_OBJECT;
				Hit.Position = Attacker->Position + Direction * Distance;
				Hit.DistanceSquared = Distance * Distance;
				Hit.Object = Object;
				Hits.push_back(Hit);
			}

			// Check penetration
			if((int)Hits.size() >= Penetration) {

				// Sort by distance
				if(Hits.size() > 1)
					std::sort(Hits.begin(), Hits.end(), CompareHitDistance);

				// Handle penetration
				if(Hits.size() > (size_t)Penetration)
					Hits.resize((size_t)Penetration);

				return;
			}
		}

		// Determine which direction needs an update
		if(Tracer.x < Tracer.y) {
			Tracer.x += Increment.x;
			TileTracer.x += TileIncrement.x;
			EndedOnX = true;
		}
		else {
			Tracer.y += Increment.y;
			TileTracer.y += TileIncrement.y;
			EndedOnX = false;
		}
	}

	// Find slope
	float Slope = Direction.y / Direction.x;

	// Determine which side has hit
	glm::vec2 WallHitPosition;
	glm::vec2 WallBoundary;
	Hit.Type = HIT_WALL;
	Hit.Object = nullptr;
	if(EndedOnX) {

		// Get correct side of the wall
		if(Direction.x < 0) {
			FirstBoundaryTile.x = TileTracer.x + 1;
			Hit.Normal.x = 1;
			Hit.Normal.y = 0;
		}
		else {
			FirstBoundaryTile.x = TileTracer.x;
			Hit.Normal.x = -1;
			Hit.Normal.y = 0;
		}
		WallBoundary.x = FirstBoundaryTile.x - Attacker->Position.x;

		// Determine hit position
		WallHitPosition.x = WallBoundary.x;
		WallHitPosition.y = WallBoundary.x * Slope;
	}
	else {

		// Get correct side of the wall
		if(Direction.y < 0) {
			FirstBoundaryTile.y = TileTracer.y + 1;
			Hit.Normal.x = 0;
			Hit.Normal.y = 1;
		}
		else {
			FirstBoundaryTile.y = TileTracer.y;
			Hit.Normal.x = 0;
			Hit.Normal.y = -1;
		}
		WallBoundary.y = FirstBoundaryTile.y - Attacker->Position.y;

		// Determine hit position
		WallHitPosition.x = WallBoundary.y / Slope;
		WallHitPosition.y = WallBoundary.y;
	}

	Hit.Position = WallHitPosition + Attacker->Position;
	Hits.push_back(Hit);
}

// Clip ray against walls
void _Map::GetDropPosition(_Object *Player, float MaxDistance, glm::vec2 &WorldPosition) const {

	// Check wall collisions
	glm::vec2 Direction = WorldPosition - Player->Position;
	std::vector<_Hit> Hits;
	Player->Map->CheckBulletCollisions(Player, Direction, Hits, -1, false, 0, _Tile::ENTITY);

	// Clip drop position
	if(Hits.size()) {
		float MaxDistanceSquared = MaxDistance * MaxDistance;
		float OriginalDistanceSquared = glm::length2(Direction);
		float HitDistanceSquared = glm::distance2(Hits[0].Position, Player->Position);
		if(HitDistanceSquared < OriginalDistanceSquared && HitDistanceSquared < MaxDistanceSquared) {
			WorldPosition = Hits[0].Position;
		}
		else if(OriginalDistanceSquared > MaxDistanceSquared) {
			Direction = glm::normalize(Direction);
			WorldPosition = Player->Position + Direction * MaxDistance * 0.999f;
		}
	}
}

// Determines if two positions are mutually visible
bool _Map::IsVisible(const glm::vec2 &Start, const glm::vec2 &End, int CheckFlag) const {

	// Find starting and ending tiles
	glm::ivec2 StartTile = GetValidCoord(glm::ivec2(Start));
	glm::ivec2 EndTile = GetValidCoord(glm::ivec2(End));

	// Check degenerate cases
	if(!CheckCollisionFlag(StartTile, CheckFlag) || !CheckCollisionFlag(EndTile, CheckFlag))
		return false;

	// Get direction
	glm::vec2 Direction = End - Start;

	// Only need to check vertical tiles
	if(StartTile.x == EndTile.x) {

		// Check degenerate cases
		if(StartTile.y == EndTile.y)
			return true;

		// Check direction
		if(Direction.y < 0) {
			for(int i = EndTile.y; i <= StartTile.y; i++) {
				if(!CheckCollisionFlag(glm::ivec2(StartTile.x, i), CheckFlag))
					return false;
			}
		}
		else {
			for(int i = StartTile.y; i <= EndTile.y; i++) {
				if(!CheckCollisionFlag(glm::ivec2(StartTile.x, i), CheckFlag))
					return false;
			}
		}

		return true;
	}
	else if(StartTile.y == EndTile.y) {

		// Check direction
		if(Direction.x < 0) {
			for(int i = EndTile.x; i <= StartTile.x; i++) {
				if(!CheckCollisionFlag(glm::ivec2(i, StartTile.y), CheckFlag))
					return false;
			}
		}
		else {
			for(int i = StartTile.x; i <= EndTile.x; i++) {
				if(!CheckCollisionFlag(glm::ivec2(i, StartTile.y), CheckFlag))
					return false;
			}
		}

		return true;
	}

	// Check x direction
	glm::ivec2 TileIncrement;
	glm::ivec2 FirstBoundaryTile;
	if(Direction.x < 0) {
		FirstBoundaryTile.x = StartTile.x;
		TileIncrement.x = -1;
	}
	else {
		FirstBoundaryTile.x = StartTile.x + 1;
		TileIncrement.x = 1;
	}

	// Check y direction
	if(Direction.y < 0) {
		FirstBoundaryTile.y = StartTile.y;
		TileIncrement.y = -1;
	}
	else {
		FirstBoundaryTile.y = StartTile.y + 1;
		TileIncrement.y = 1;
	}

	// Find ray direction ratios
	glm::vec2 Ratio = 1.0f / Direction;

	// Calculate increments
	glm::vec2 Increment;
	Increment.x = TileIncrement.x * Ratio.x;
	Increment.y = TileIncrement.y * Ratio.y;

	// Get starting positions
	glm::vec2 Tracer;
	Tracer.x = (FirstBoundaryTile.x - Start.x) * Ratio.x;
	Tracer.y = (FirstBoundaryTile.y - Start.y) * Ratio.y;

	// Starting tiles
	glm::ivec2 TileTracer = StartTile;

	// Traverse tiles
	while(true) {

		// Check for walls
		if(TileTracer.x < 0 || TileTracer.y < 0 || TileTracer.x >= Size.x || TileTracer.y >= Size.y || !CheckCollisionFlag(TileTracer, CheckFlag))
			return false;

		// Determine which direction needs an update
		if(Tracer.x < Tracer.y) {
			Tracer.x += Increment.x;
			TileTracer.x += TileIncrement.x;
		}
		else {
			Tracer.y += Increment.y;
			TileTracer.y += TileIncrement.y;
		}

		// Exit condition
		if(
			(Direction.x < 0 && TileTracer.x < EndTile.x) ||
			(Direction.x > 0 && TileTracer.x > EndTile.x) ||
			(Direction.y < 0 && TileTracer.y < EndTile.y) ||
			(Direction.y > 0 && TileTracer.y > EndTile.y)) {
				break;
		}
	}

	return true;
}

// Determine if an AABB can move to a position without hitting walls
bool _Map::CanMoveTo(const glm::vec2 &Start, const glm::vec2 &End, const glm::vec2 &Size) const {

	// Get x components of the starting corners
	glm::vec2 LeftStartPosition;
	glm::vec2 RightStartPosition;
	LeftStartPosition.x = Start.x - Size.x;
	RightStartPosition.x = Start.x + Size.x;

	// Get direction
	glm::vec2 Direction(End - Start);

	// Get y components of the starting corners
	if((Direction.x < 0 && Direction.y < 0) || (Direction.x >= 0 && Direction.y >= 0)) {
		LeftStartPosition.y = Start.y + Size.y;
		RightStartPosition.y = Start.y - Size.y;
	}
	else {
		LeftStartPosition.y = Start.y - Size.y;
		RightStartPosition.y = Start.y + Size.y;
	}

	// Get ending positions
	glm::vec2 LeftEndPosition = LeftStartPosition + Direction;
	glm::vec2 RightEndPosition = RightStartPosition + Direction;

	// Check the first corner
	if(IsVisible(LeftStartPosition, LeftEndPosition, _Tile::ENTITY)) {

		// No wall, so check the second corner
		if(IsVisible(RightStartPosition, RightEndPosition, _Tile::ENTITY))
			return true;
	}

	return false;
}

// Return an object at a given position
void _Map::GetSelectedObject(const glm::vec2 &Position, float RadiusSquared, _ObjectSpawn **Object, size_t *Index) {
	for(size_t i = 0; i < ObjectSpawns.size(); i++) {
		if(glm::distance2(ObjectSpawns[i]->Position, Position) < RadiusSquared) {
			*Object = ObjectSpawns[i];
			*Index = i;
			return;
		}
	}

	*Object = nullptr;
}

// Returns all the objects that fall inside the rectangle
void _Map::GetSelectedObjects(const glm::vec2 &Start, const glm::vec2 &End, std::vector<_ObjectSpawn *> *SelectedObjects, int Type) {
	glm::vec4 Bounds(glm::min(Start, End), glm::max(Start, End));

	for(const auto &ObjectSpawn : ObjectSpawns) {
		if(Type == 0 && ObjectSpawn->Type == _Object::MONSTER)
			continue;
		else if(Type == 1 && ObjectSpawn->Type != _Object::MONSTER)
			continue;

		if(ObjectSpawn->Position.x > Bounds[0] && ObjectSpawn->Position.y > Bounds[1] && ObjectSpawn->Position.x <= Bounds[2] && ObjectSpawn->Position.y <= Bounds[3]) {
			SelectedObjects->push_back(ObjectSpawn);
		}
	}
}

// Get a list of blocks inside the bounds of a selection box
void _Map::GetSelectedBlocks(const glm::vec2 &Start, const glm::vec2 &End, int Layer, std::vector<size_t> &SelectedBlocks, glm::ivec4 &SelectionBounds) {
	glm::vec4 Bounds(glm::min(Start, End), glm::max(Start, End));

	SelectionBounds[0] = Size.x;
	SelectionBounds[1] = Size.y;
	SelectionBounds[2] = -1;
	SelectionBounds[3] = -1;
	for(int i = (int)(Blocks[Layer].size())-1; i >= 0; i--) {
		_Block &Block = Blocks[Layer][(size_t)i];
		if(Bounds[0] > Block.End.x + 1 || Bounds[2] < Block.Start.x || Bounds[1] > Block.End.y + 1 || Bounds[3] < Block.Start.y)
			continue;

		SelectionBounds[0] = std::min(SelectionBounds[0], Block.Start.x);
		SelectionBounds[1] = std::min(SelectionBounds[1], Block.Start.y);
		SelectionBounds[2] = std::max(SelectionBounds[2], Block.End.x);
		SelectionBounds[3] = std::max(SelectionBounds[3], Block.End.y);
		Block.MoveStart = Block.Start;
		Block.MoveEnd = Block.End;
		SelectedBlocks.push_back((size_t)i);
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
	delete Events[(size_t)Index];
	Events.erase(Events.begin() + Index);
}

// Remove deleted blocks
void _Map::DeleteBlocks(int Layer, std::vector<size_t> &BlockIDs) {

	// Sort block ids descending
	std::sort(BlockIDs.begin(), BlockIDs.end(), std::greater<int>());

	// Remove blocks from events
	for(size_t i = 0; i < BlockIDs.size(); i++) {
		int Index = (int)BlockIDs[i];
		DeleteBlockIDFromTiles(Layer, Index);
		Blocks[Layer].erase(Blocks[Layer].begin() + Index);
	}
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

// Return the block index at a given layer and position
size_t _Map::GetSelectedBlock(int Layer, const glm::ivec2 &Index) {
	for(int i = (int)(Blocks[Layer].size())-1; i >= 0; i--) {
		if(Index.x >= Blocks[Layer][(size_t)i].Start.x && Index.y >= Blocks[Layer][(size_t)i].Start.y && Index.x <= Blocks[Layer][(size_t)i].End.x && Index.y <= Blocks[Layer][(size_t)i].End.y)
			return (size_t)i;
	}

	return (size_t)-1;
}

// Return the block at a given position
size_t _Map::GetSelectedBlock(int Layer, const glm::ivec2 &Index, _Block **Block) {
	size_t BlockIndex = GetSelectedBlock(Layer, Index);
	if(BlockIndex != (size_t)-1) {
		*Block = &Blocks[Layer][BlockIndex];
		return BlockIndex;
	}

	*Block = nullptr;
	return (size_t)-1;
}

// Returns a block by its layer and index
_Block *_Map::GetBlock(int Layer, const size_t Index) {
	if(Layer < 0 || Layer >= MAPLAYER_COUNT || Index >= Blocks[Layer].size())
		return nullptr;

	return &Blocks[Layer][Index];
}

// Toggles an event's active state
void _Map::ToggleEventActive(size_t Index) {
	if(Index >= Events.size())
		return;

	Events[Index]->Active = !Events[Index]->Active;
}

// Determines if a tile has any events
bool _Map::HasEvents(const glm::ivec2 &Position) const {
	return Data[Position.x][Position.y].Events.size() > 0;
}

// Update ambient light
void _Map::UpdateAmbientLight(double FrameTime, double Clock) {

	// Check for day night cycle
	if(AmbientClock)
		GetClockLight(Clock, TargetAmbientLight);

	glm::vec4 Delta = TargetAmbientLight - AmbientLight;
	if(glm::dot(Delta, Delta) < 0.0001f)
		AmbientLight = TargetAmbientLight;
	else
		AmbientLight += Delta * (float)FrameTime * LIGHT_CHANGE_SPEED;
}

// Get light from day night cycle
void _Map::GetClockLight(double Clock, glm::vec4 &LightColor) {

	// Find index by time
	size_t NextCycle = DayCyclesTime.size();
	for(size_t i = 0; i < DayCyclesTime.size(); i++) {
		if(Clock < DayCyclesTime[i]) {
			NextCycle = i;
			break;
		}
	}

	// Get indices for current and next cycle
	size_t CurrentCycle = NextCycle - 1;
	if(CurrentCycle >= DayCyclesTime.size())
		CurrentCycle = 0;
	if(NextCycle >= DayCyclesTime.size())
		NextCycle = 0;

	// Get current time diff
	double Diff = Clock - DayCyclesTime[CurrentCycle];
	if(Diff < 0)
		Diff += MAP_DAY_LENGTH;

	// Get length of cycle
	double Length = DayCyclesTime[NextCycle] - DayCyclesTime[CurrentCycle];
	if(Length < 0)
		Length += MAP_DAY_LENGTH;

	// Get percent to next cycle
	float Percent = (float)(Diff / Length);

	// Set color
	LightColor = glm::mix(DayCycles[CurrentCycle], DayCycles[NextCycle], Percent);
}

// Change ambient light with color id
void _Map::SetAmbientLight(const std::string &ColorID) {
	if(ColorID.empty()) {
		TargetAmbientLight = BaseAmbientLight;
		AmbientClock = BaseAmbientClock;
	}
	else {
		AmbientClock = false;
		TargetAmbientLight = ae::Assets.Colors[ColorID];
	}
}

// Gets a list of event based on a position
std::vector<_Event *> &_Map::GetEventList(const glm::ivec2 &Position) {
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

		if(Index.x >= Event->Start.x && Index.y >= Event->Start.y && Index.x <= Event->End.x && Index.y <= Event->End.y) {
			*ReturnEvent = Event;
			return Events.size() - 1 - (Iterator - Events.rbegin());
		}
	}

	*ReturnEvent = nullptr;
	return -1;
}

// Changes the layer a block is in
size_t _Map::ChangeLayer(int OldLayer, int NewLayer, int Index) {

	// Delete block ids from events
	DeleteBlockIDFromTiles(OldLayer, Index);

	// Add new block
	Blocks[NewLayer].push_back(Blocks[OldLayer][(size_t)Index]);
	Blocks[OldLayer].erase(Blocks[OldLayer].begin() + Index);

	return Blocks[NewLayer].size() - 1;
}

// Draws a grid on the map
void _Map::RenderGrid(int Mode) {
	if(Mode <= 0)
		return;

	// Draw vertical lines
	ae::Graphics.SetColor(COLOR_TWHITE);
	for(int i = Mode; i < Size.x; i += Mode)
		ae::Graphics.DrawLine(glm::vec2(i, 0), glm::vec2(i, Size.y));

	// Draw horizontal lines
	for(int i = Mode; i < Size.y; i += Mode)
		ae::Graphics.DrawLine(glm::vec2(0, i), glm::vec2(Size.x, i));
}

// Draw the mini map
void _Map::DrawMinimap(bool FullMap, ae::_Bounds &MinimapBounds) {

	// Get bounds of minimap window
	glm::vec2 DrawSize = FullMap ? glm::vec2(ae::Graphics.CurrentSize.y, ae::Graphics.CurrentSize.y) * 0.75f : HUD_MINIMAP_SIZE * ae::_Element::GetUIScale();
	if(FullMap) {
		DrawSize.x *= ae::Graphics.AspectRatio;
		MinimapBounds = ae::_Bounds(
			glm::ivec2((ae::Graphics.CurrentSize - glm::ivec2(DrawSize))/2),
			glm::ivec2((ae::Graphics.CurrentSize + glm::ivec2(DrawSize))/2)
		);
	}
	else {
		glm::vec2 Padding = HUD_MINIMAP_PADDING * ae::_Element::GetUIScale();
		MinimapBounds = ae::_Bounds(
			glm::ivec2(ae::Graphics.CurrentSize.x - DrawSize.x - Padding.x, Padding.y),
			glm::ivec2(ae::Graphics.CurrentSize.x - Padding.x, Padding.y + DrawSize.y)
		);
	}

	// Draw minimap background
	ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos"]);
	ae::Graphics.SetColor(HUD_MINIMAP_BACKGROUND_COLOR);
	ae::Graphics.EnableScissorTest();
	ae::Graphics.SetScissor(MinimapBounds);
	ae::Graphics.DrawRectangle(MinimapBounds, true);

	// Draw layers
	glm::vec2 CameraPosition(Camera->GetPosition());
	ae::_Bounds CaptureBounds(CameraPosition - MinimapCaptureSize, CameraPosition + MinimapCaptureSize);
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
	for(size_t i = 0; i < Blocks[Layer].size(); i++) {
		_Block &Block = Blocks[Layer][i];
		if(Layer == MAPLAYER_WALL) {
			size_t ColorIndex = std::clamp((size_t)(Block.MinZ * 2), (size_t)0, HighlightColors.size()-1);
			ae::Graphics.SetColor(HighlightColors[ColorIndex]);
		}
		ae::Graphics.DrawRectangle3D(glm::vec2(Block.Start.x, Block.Start.y), glm::vec2(Block.End.x + 1.0f, Block.End.y + 1.0f), false);
	}
}

// Add particle to grid
void _Map::AddParticle(_Particle *Particle) {
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
	return glm::vec2(std::clamp(Position.x, 0.0f, Size.x - MAP_EPSILON), std::clamp(Position.y, 0.0f, Size.y - MAP_EPSILON));
}

// Opens a door or hits a floor switch
void _Map::ChangeMapState(const _Event *Event) {

	// Check for the proper event
	if(!(Event->Type == EVENT_DOOR || Event->Type == EVENT_WALLSWITCH || Event->Type == EVENT_FLOORSWITCH))
		return;

	const std::vector<_EventTile> &Tiles = Event->Tiles;

	// Switch the texture of the first block for wall switches
	size_t StartIndex = 0;
	if(Event->Type == EVENT_WALLSWITCH && Tiles.size() > 0 && Tiles[0].BlockID != -1) {
		SwapBlockTextures(Tiles[0].Layer, Tiles[0].BlockID);
		StartIndex = 1;
	}

	// Change all the tiles
	for(size_t i = StartIndex; i < Tiles.size(); i++) {
		_Tile *Tile = &Data[Tiles[i].Coord.x][Tiles[i].Coord.y];

		// Change flags only allowed by the change mask
		Tile->Collision ^= Tile->CollisionChangeMask & (_Tile::ENTITY | _Tile::BULLET);

		// Switch textures
		SwapBlockTextures(Tiles[i].Layer, Tiles[i].BlockID);
	}
}

// Determines if the map state can be changed
bool _Map::CanChangeMapState(const _Event *Event) {

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

	_Block *Block = &Blocks[Layer][(size_t)Index];
	std::swap(Block->Texture, Block->AltTexture);
}

// Renders the floor
int _Map::RenderFloors() {
	if(!Camera)
		return 0;

	// Draw base layer
	ae::_Program *Program = ae::Assets.Programs["map"];
	ae::Graphics.SetProgram(Program);
	ae::Graphics.SetDepthTest(false);
	ae::Graphics.SetDepthMask(false);
	Program->ResetTransform(Program->NormalTransformID);

	int Count = 0;
	for(size_t i = 0; i < Blocks[MAPLAYER_BASE].size(); i++) {
		_Block *Block = &Blocks[MAPLAYER_BASE][i];

		// Check render bounds
		glm::vec4 Bounds;
		Block->GetBounds(Bounds, true);
		bool Draw = Camera->IsAABBInView(Bounds);
		if(!Draw || !Block->Texture)
			continue;

		ae::Graphics.SetColor(Block->Color);
		ae::Graphics.DrawRepeatable(
			glm::vec3(Block->Start.x, Block->Start.y, Block->MinZ),
			glm::vec3(Block->End.x + 1.0f, Block->End.y + 1.0f, Block->MinZ),
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
		for(size_t j = 0; j < Blocks[i].size(); j++) {
			_Block *Block = &Blocks[i][j];

			// Check render bounds
			glm::vec4 Bounds;
			Block->GetBounds(Bounds, true);
			bool Draw = Camera->IsAABBInView(Bounds);
			if(!Draw || !Block->Texture)
				continue;

			if(Block->MinZ == Block->MaxZ) {
				ae::Graphics.SetColor(Block->Color);
				ae::Graphics.DrawRepeatable(
					glm::vec3(Block->Start.x, Block->Start.y, Block->MinZ + MAP_LAYEROFFSET * i),
					glm::vec3(Block->End.x + 1.0f, Block->End.y + 1.0f, Block->MinZ + MAP_LAYEROFFSET * i),
					Block->Texture,
					Block->Rotation,
					Block->ScaleX
				);
			}
			else if(Block->Texture) {
				ae::Graphics.SetColor(Block->Color);
				ae::Graphics.DrawCube(
					glm::vec3(Block->Start.x, Block->Start.y, Block->MinZ),
					glm::vec3(Block->End.x - Block->Start.x + 1.0f, Block->End.y - Block->Start.y + 1.0f, Block->MaxZ - Block->MinZ),
					Block->Texture
				);
			}

			Count++;
		}
	}

	return Count;
}

// Renders the walls
int _Map::RenderWalls(bool SkipFloating) {
	if(!Camera)
		return 0;

	// Set up graphics
	ae::_Program *Program = ae::Assets.Programs["map_norm"];
	ae::Graphics.SetProgram(Program);
	ae::Graphics.SetDepthMask(true);
	ae::Graphics.SetDepthTest(true);
	ae::Graphics.SetCullFace(true);
	Program->ResetTransform(Program->NormalTransformID);

	// Draw walls
	int Count = 0;
	for(size_t i = 0; i < Blocks[MAPLAYER_WALL].size(); i++) {
		_Block *Block = &Blocks[MAPLAYER_WALL][i];
		if(SkipFloating && Block->MinZ > 0)
			continue;

		// Check render bounds
		glm::vec4 Bounds;
		Block->GetBounds(Bounds, true);
		bool Draw = Camera->IsAABBInView(Bounds);
		if(!Draw || !Block->Texture)
			continue;

		// Draw cube
		ae::Graphics.SetColor(Block->Color);
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

	ae::_Program *Program = ae::Assets.Programs["map_norm"];
	ae::Graphics.SetProgram(Program);
	ae::Graphics.SetDepthMask(false);
	ae::Graphics.SetDepthTest(true);
	Program->ResetTransform(Program->NormalTransformID);

	int Count = 0;
	for(size_t i = 0; i < Blocks[MAPLAYER_FLAT].size(); i++) {
		_Block *Block = &Blocks[MAPLAYER_FLAT][i];

		// Check render bounds
		glm::vec4 Bounds;
		Block->GetBounds(Bounds, true);
		bool Draw = Camera->IsAABBInView(Bounds);
		if(!Draw || !Block->Texture)
			continue;

		// Draw
		glm::vec2 Offset(0);
		int Side;
		if(Block->Rotation == 0.0f || Block->Rotation == 180.0f) {
			if(Block->End.y + 0.5f > Camera->GetPosition().y) {
				Side = 3;
				Offset.y = 0.5f;
			}
			else {
				Side = 1;
				Offset.y = -0.5f;
			}
		}
		else {
			if(Block->End.x + 0.5f > Camera->GetPosition().x) {
				Side = 2;
				Offset.x = 0.5f;
			}
			else {
				Side = 4;
				Offset.x = -0.5f;
			}
		}

		ae::Graphics.SetColor(Block->Color);
		ae::Graphics.DrawWall(
			glm::vec3(glm::vec2(Block->Start) + Offset, Block->MinZ),
			glm::vec3(Block->End.x - Block->Start.x + 1.0f, Block->End.y - Block->Start.y + 1.0f, Block->MaxZ - Block->MinZ),
			Block->Texture,
			Side
		);

		Count++;
	}

	return Count;
}

// Draws the events
void _Map::RenderEvents(std::vector<const ae::_Texture *> &Textures, int Type) {
	if(!Camera)
		return;

	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
	ae::Graphics.SetColor(glm::vec4(1.0f));
	ae::Graphics.SetDepthTest(false);

	// Draw events
	for(size_t i = 0; i < Events.size(); i++) {
		if(Type != -1 && Events[i]->Type != Type)
			continue;

		glm::vec4 Bounds(Events[i]->Start.x, Events[i]->Start.y, Events[i]->End.x + 1.0f, Events[i]->End.y + 1.0f);
		if(!Camera->IsAABBInView(Bounds))
			continue;

		// Draw event overlay
		ae::Graphics.DrawRepeatable(
			glm::vec3(Events[i]->Start.x, Events[i]->Start.y, MAP_LAYEROFFSET),
			glm::vec3(Events[i]->End.x + 1.0f, Events[i]->End.y + 1.0f, MAP_LAYEROFFSET),
			Textures[(size_t)Events[i]->Type],
			0,
			1.0f
		);
	}
}

// Renders the foreground tiles
int _Map::RenderForeground(const glm::vec2 &PlayerPosition, bool AlwaysFade) {
	if(!Camera)
		return 0;

	// Set up graphics
	ae::_Program *Program = ae::Assets.Programs["map_norm"];
	ae::Graphics.SetProgram(Program);
	ae::Graphics.SetDepthMask(true);
	ae::Graphics.SetDepthTest(true);
	Program->ResetTransform(Program->NormalTransformID);

	// Draw foreground
	int Count = 0;
	for(size_t i = 0; i < Blocks[MAPLAYER_FORE].size(); i++) {
		_Block *Block = &Blocks[MAPLAYER_FORE][i];

		// Check bounds
		bool Draw = true;
		glm::vec4 Color = Block->Color;
		if(Block->MinZ >= 0) {
			glm::vec4 Bounds;
			Block->GetBounds(Bounds, false);
			Draw = Camera->IsAABBInView(Bounds);

			// Change alpha when player is directly under block
			if(Draw && (AlwaysFade || (PlayerPosition.x >= Bounds[0] && PlayerPosition.y >= Bounds[1] && PlayerPosition.x <= Bounds[2] && PlayerPosition.y <= Bounds[3])))
				Color.a = MAP_FOREGROUND_FADE;
		}

		if(!Draw)
			continue;

		// Draw
		ae::Graphics.SetColor(Color);
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

// Render map props
int _Map::RenderProps() {
	ae::_Program *Program = ae::Assets.Programs["map_norm"];
	ae::Graphics.SetProgram(Program);
	ae::Graphics.SetDepthMask(true);
	ae::Graphics.SetDepthTest(true);
	ae::Graphics.SetCullFace(true);
	Program->ResetTransform(Program->NormalTransformID);
	Program->ResetTransform(Program->TextureTransformID);
	int Count = ObjectManager->Render(_ObjectManager::RENDER_PROP, 0.0);
	ae::Graphics.SetCullFace(false);

	return Count;
}

// Render map decals
int _Map::RenderParticles(int Type, double BlendFactor) {

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

				Particle->Render(Camera, BlendFactor);
				Count++;
			}
		}
	}

	return Count;
}

// Update map
void _Map::Update(double FrameTime, double Clock) {

	// Update ambient light
	UpdateAmbientLight(FrameTime, Clock);

	// Add blocks and events to minimap
	MinimapLayers.clear();
	AddMinimapLayers();

	// Update objects
	ObjectManager->Update(FrameTime, this);
}

// Adds an item to the item list and collision grid
void _Map::AddObject(_Object *Object, int GridType) {
	ObjectManager->AddObject(Object);
	AddObjectToGrid(Object, GridType);
}

// Removes an item from object list and collision grid
void _Map::RemoveObject(_Object *Object, int GridType) {
	ObjectManager->RemoveObject(Object);
	RemoveObjectFromGrid(Object, GridType);
}

// Check if bounds are in minimap range
bool _Map::CheckMinimapBounds(const glm::vec4 &Bounds) {

	if(Bounds[2] < Camera->GetPosition().x - MinimapCaptureSize.x || Bounds[0] > Camera->GetPosition().x + MinimapCaptureSize.x)
	   return false;
	if(Bounds[3] < Camera->GetPosition().y - MinimapCaptureSize.y || Bounds[1] > Camera->GetPosition().y + MinimapCaptureSize.y)
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
			Block->GetBounds(Bounds, false);
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

		// Ignore inactive events
		if(!Event->Active)
			continue;

		// Ignore events except doors and wall switches
		if(Event->Type != EVENT_DOOR && Event->Type != EVENT_WALLSWITCH)
			continue;

		// Ignore switched doors
		if(Event->Type == EVENT_DOOR && Event->Switched)
			continue;

		// Check bounds
		glm::vec4 Bounds;
		Event->GetBounds(Bounds);
		if(!CheckMinimapBounds(Bounds))
			continue;

		// Add to minimap
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

// Get added monster/item level based on progression
int _Map::GetAddedLevel() const {
	return Progression * GAME_PROGRESSION_DIFFICULTY;
}
