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
namespace ae {
	class _Camera;
}
class _Font;
class _HUD;
class _Map;
class _Event;
class _Entity;
class _Monster;
class _Player;
class _Item;
class _Particles;
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
		bool HandleCommand(ae::_Console *Console) override;
		void HandleWindow(uint8_t Event) override;
		void HandleQuit() override;

		// Update
		void Update(double FrameTime) override;
		void Render(double BlendFactor) override;

		void GenerateBulletEffects(_Entity *Attacker, const int Type, const _Hit &Hit);

		// Parameters
		std::string Level;
		bool TestMode;
		bool FromEditor;
		int CheckpointIndex;

		_Player *Player;

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
		void PickupObject(_Item *NearbyItem);
		void UseObject(_Item *NearbyItem);

			// Game
		double CursorItemTimer;
		double SaveGameTimer;

		// Map
		_Map *Map;
		_Event *LastLightEvent;

		// Entities
		std::list<_Entity *> Monsters;
		std::list<_Event *> ActiveEvents;

		// HUD
		_HUD *HUD;
		_Item *CursorItem;
		_Item *PreviousCursorItem;

		// Particles
		_Particles *Particles;

		// Camera
		ae::_Camera *Camera;
		glm::vec2 PreviousWorldCursor;
		glm::vec2 WorldCursor;
};

extern _PlayState PlayState;
