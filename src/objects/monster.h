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

// Libraries
#include <objects/entity.h>

struct _MonsterTemplate;
struct _ParticleGroup;
class _Player;
struct _ItemDrop;

// Classes
class _Monster : public _Entity {

	public:

		// Enumerations
		enum GoalTypes {
			GOAL_PURSUE,
			GOAL_RETREAT,
			GOAL_COUNT,
		};

		_Monster(const _ObjectTemplate &MonsterTemplate);
		void RecalculateStats(bool SoftReset=false) override;

		void Update(double FrameTime) override;
		void OnAttack(_Entity *Victim, const _Hit &Hit) override;
		void OnHit(_Entity *Attacker, const _Hit &Hit) override;
		void OnPlayerDeath();
		bool PlayerIsAttackable() const { return PlayerDistanceSquared <= AttackRangeSquared && (PlayerShootable || PlayerVisible); }
		bool ShowOnMinimap() const { return Action != ACTION_IDLE || MoveState || PlayerIsAttackable() || IsCrate(); }

		const _ParticleTemplate *GetParticle(int ParticleType) const override;

		// AI
		const _Player *Player{nullptr};
		const _ItemDrop *ItemDrop{nullptr};
		int AIAttacks{0};

	private:

		void SetTarget(const glm::vec2 &NewTargetPosition, float NewTargetRadius);
		void GenerateReactionTime();

		glm::vec2 ReturnPosition{0.0f};
		double StaticTimer{0.0};
		double ReactionTimer{0.0};
		double ReturnTimer{0.0};
		float AttackRangeSquared{0.0f};
		float ViewRangeSquared{0.0f};
		float StopThresholdSquared{0.0f};
		float PlayerDistanceSquared{0.0f};
		int AttacksMade{0};
		int Goal{GOAL_PURSUE};
		int Wiggles{0};
		bool PlayerVisible{false};
		bool LastPlayerVisible{false};
		bool PlayerShootable{false};
};
