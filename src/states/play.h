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

#include <ae/state.h>
#include <color.h>
#include <list>

// Forward Declarations
class _Font;
class _HUD;
class _Map;
class _Event;
class _Entity;
class _Monster;
class _Player;
class _Item;
class _Particles;
class _Camera;
struct _ObjectSpawn;
struct _ParticleTemplate;
struct _EventTile;
struct _Hit;

// Types of entity attack outcomes
enum CollisionType {
	HIT_NONE,
	HIT_WALL,
	HIT_OBJECT
};

// Play state
class _PlayState : public ae::_State {

	public:

		// Setup
		_PlayState();
		void Init() override;
		void Close() override;

		// Input
		bool HandleAction(int InputType, std::size_t Action, int Value) override;
		bool HandleKey(const ae::_KeyEvent &KeyEvent) override;
		void HandleMouseButton(const ae::_MouseEvent &MouseEvent) override;

		// Update
		void Update(double FrameTime) override;
		void Render(double BlendFactor) override;

		void SetLevel(const std::string &Level) { this->Level = Level; }
		void SetTestMode(bool Value) { TestMode = Value; }
		void SetFromEditor(bool Value) { FromEditor = Value; }
		void SetCheckpointIndex(int Value) { CheckpointIndex = Value; }
		bool GetFromEditor() const { return FromEditor; }

		void GenerateBulletEffects(_Entity *Attacker, const int Type, const _Hit &Hit);

		void SetPlayer(_Player *Player) { this->Player = Player; }
		_Player *GetPlayer() { return Player; }

	protected:

		bool IsPaused();
		void RestartFromDeath();

		void DeleteMonsters();
		void DeleteActiveEvents();

		void UpdateMonsters(double FrameTime);
		void CheckEvents(const _Entity *Entity);
		void UpdateEvents(double FrameTime);

		void SpawnObject(_ObjectSpawn *ObjectSpawn, bool GenerateStats=false);
		void AddMonster(_Monster *Monster);
		void RemoveMonster(_Monster *Monster);
		void CreateItemDrop(const _Entity *Entity);
		void EntityAttack(_Entity *Attacker, int GridType);
		void PickupObject();
		void UseObject();

		// Parameters
		std::string Level;
		bool TestMode, FromEditor;
		int CheckpointIndex;

		// Game
		double CursorItemTimer, SaveGameTimer;

		// Map
		_Map *Map;
		_Event *LastLightEvent;

		// Entities
		_Player *Player;
		std::list<_Entity *> Monsters;
		std::list<_Event *> ActiveEvents;

		// HUD
		_HUD *HUD;
		_Item *CursorItem, *PreviousCursorItem;

		// Particles
		_Particles *Particles;

		// Camera
		_Camera *Camera;
		glm::vec2 PreviousWorldCursor;
		glm::vec2 WorldCursor;
};

extern _PlayState PlayState;
