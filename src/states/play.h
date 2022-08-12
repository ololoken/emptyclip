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
#include <unordered_map>
#include <list>

// Forward Declarations
class _HUD;
class _Map;
class _Event;
class _Entity;
class _Monster;
class _Player;
class _Item;
class _Object;
class _Particles;
struct _ObjectSpawn;
struct _ParticleTemplate;
struct _EventTile;
struct _Hit;
namespace ae {
	class _Framebuffer;
	class _Camera;
}

// Play state
class _PlayState : public ae::_State {

	public:

		// Setup
		void Init() override;
		void Close() override;

		// Input
		bool HandleAction(int InputType, size_t Action, int Value) override;
		bool HandleKey(const ae::_KeyEvent &KeyEvent) override;
		void HandleMouseButton(const ae::_MouseEvent &MouseEvent) override;
		bool HandleCommand(ae::_Console *Console) override;
		void HandleWindow(uint8_t Event) override;
		void HandleQuit() override;

		// Update
		void Update(double FrameTime) override;
		void Render(double BlendFactor) override;

		void GenerateHitEffects(_Entity *Attacker, const int Type, const _Hit &Hit, bool CreateWallDecal=true);
		void GenerateDamageText(glm::vec2 Position, int Value, bool Crit, bool HitPlayer);
		void GenerateExplosion(const _ParticleTemplate *ParticleTemplate, const glm::vec2 &Position, const glm::vec2 &Scale);
		void GenerateProjectileEffects(const _ParticleTemplate *ParticleTemplate, const glm::vec2 &Position);
		void CreateItemDrop(const _Entity *Entity, float DropRate);

		// Parameters
		std::string Level;
		bool TestMode{false};
		bool FromEditor{false};
		bool DebugMode{false};
		bool DevMode{false};
		bool GodMode{false};
		int CheckpointIndex{0};

		// Objects
		_HUD *HUD{nullptr};
		_Player *Player{nullptr};
		double FlashTimer{0.0};

	protected:

		bool IsPaused();

		void AddMonster(_Monster *Monster);
		void RemoveMonster(_Monster *Monster);
		void DeleteMonsters();
		void UpdateMonsters(double FrameTime);

		void ActivateEvent();
		void CheckEvents(const _Entity *Entity);
		void UpdateEvents(double FrameTime);
		void ResolveAttack(_Entity *Attacker, int GridType);
		void HandlePickup();
		void PlayerDied();

		void SpawnObject(_ObjectSpawn *ObjectSpawn, bool GenerateStats=false, int AddedLevel=0);
		void UseObject(_Item *Item);
		void PickupObject(_Item *Item, int &AmountAdded);

		// Game
		double Timer{0.0};
		double CursorItemTimer{0.0};

		// Map
		_Map *Map{nullptr};

		// Objects
		std::list<_Entity *> Monsters;
		std::list<_Event *> ActiveEvents;
		int ActiveAI{0};

		// HUD
		std::unordered_map<_Item *, int> IgnoreItems;
		_Item *CursorItem{nullptr};
		_Item *PreviousCursorItem{nullptr};
		_Object *ClosestItem{nullptr};
		_Object *LastClosestItem{nullptr};
		double ClosestItemTimer{0.0};

		// Graphics
		ae::_Framebuffer *Framebuffer{nullptr};

		// Particles
		_Particles *Particles{nullptr};

		// Camera
		ae::_Camera *Camera{nullptr};
		glm::vec2 PreviousWorldCursor{0.0f};
		glm::vec2 WorldCursor{0.0f};
};

extern _PlayState PlayState;
