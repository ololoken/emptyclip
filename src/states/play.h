/******************************************************************************
* Empty Clip
* Copyright (C) 2023 Alan Witkowski
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
#include <map>
#include <list>
#include <vector>

// Forward Declarations
class _HUD;
class _Map;
class _Event;
class _Entity;
class _Monster;
class _Player;
class _Item;
class _Object;
class _Particle;
class _Particles;
struct _ObjectSpawn;
struct _ParticleTemplate;
struct _EventTile;
struct _Hit;
namespace ae {
	class _AudioSource;
	class _Framebuffer;
	class _Camera;
	class _Element;
}

// Play state
class _PlayState : public ae::_State {

	public:

		enum QualityFilterTypes {
			FILTERQUALITY_ALL,
			FILTERQUALITY_ZERO,
			FILTERQUALITY_FIFTEEN,
			FILTERQUALITY_COUNT,
		};

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

		void GenerateHitEffects(_Entity *Attacker, const int Type, const _Hit &Hit, bool Death, float Rotation, bool CreateWallDecal=true);
		void GenerateDamageText(const glm::vec2 &Position, int64_t Value, bool Crit, bool HitPlayer);
		_Particle *GenerateTextParticle(const glm::vec2 &Position, const std::string &Value, const glm::vec4 &Color=glm::vec4(1.0f));
		void GenerateExplosion(const _ParticleTemplate *ParticleTemplate, const glm::vec2 &Position, const glm::vec2 &Scale);
		void GenerateProjectileEffects(const _ParticleTemplate *ParticleTemplate, const glm::vec2 &Position, const glm::vec4 &Color);
		void CreateItemDrop(const _Entity *Entity, float DropRate);
		void DropItem(_Item *Item);
		void SpawnObject(const _ObjectSpawn *ObjectSpawn, bool GenerateStats, int AddedLevel, int AddedRarityChance);
		int PickupObject(_Item *Item, bool Manual);
		bool ShowMoreInfo();
		bool DrawMinimapIcons();

		// Parameters
		glm::vec2 SpawnLocation{-1.0f, -1.0f};
		std::string Level;
		bool TestMode{false};
		bool FromEditor{false};
		bool DebugMode{false};
		bool DevMode{false};
		bool GodMode{false};
		int DefaultQuality{0};
		int CheckpointIndex{0};

		// Objects
		_HUD *HUD{nullptr};
		_Map *Map{nullptr};
		_Player *Player{nullptr};
		int ActiveProjectiles{0};

	protected:

		bool IsPaused();
		void SetMinimapCaptureSize();

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
		void EndLevel();

		bool SetHoverItem();
		void SetFilterTextValue(ae::_Element *Element, int Value);
		void ChangeFilterLevel(int FilterType);

		// Game
		double Timer{0.0};
		double CursorItemTimer{0.0};
		double SaveTimer{0.0};

		// Map
		_Event *PreviousTouchingEndEvent{nullptr};
		_Event *TouchingEndEvent{nullptr};

		// Objects
		std::vector<_Entity *> Monsters;
		std::vector<_Event *> ActiveEvents;
		int ActiveAI{0};

		// HUD
		std::unordered_map<_Item *, int> IgnoreItems;
		_Item *CursorItem{nullptr};
		_Item *PreviousCursorItem{nullptr};
		_Item *ClosestItem{nullptr};
		_Item *LastClosestItem{nullptr};
		double ClosestItemTimer{0.0};

		// Graphics
		ae::_Framebuffer *Framebuffer{nullptr};
		double FlashTimer{0.0};

		// Particles
		_Particles *Particles{nullptr};

		// Camera
		ae::_Camera *Camera{nullptr};
		glm::vec2 PreviousWorldCursor{0.0f};
		glm::vec2 WorldCursor{0.0f};

		// Sounds
		const ae::_AudioSource *LockedSound{nullptr};
};

extern _PlayState PlayState;
