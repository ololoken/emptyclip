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
#include <objects/entity.h>

struct _MonsterTemplate;
struct _ParticleGroup;
class _Player;
struct _ItemDrop;

// Classes
class _Monster : public _Entity {

	public:

		// Enumerations
		enum AITypes {
			AI_NONE,
			AI_BOSS,
			AI_BASIC,
			AI_HITANDRUN,
			AI_COUNT
		};

		enum GoalTypes {
			GOAL_PURSUE,
			GOAL_RETREAT,
			GOAL_COUNT,
		};

		_Monster(const _ObjectTemplate &MonsterTemplate);
		void RecalculateStats() override;

		void Update(double FrameTime) override;
		void OnAttack(_Entity *Victim, const _Hit &Hit) override;
		void OnHit(_Entity *Attacker, const _Hit &Hit) override;
		void OnPlayerDeath();

		bool IsCrate() const { return AIType == AI_NONE; }
		const _ParticleTemplate *GetParticle(int ParticleType) const override;

		// AI
		const _Player *Player;
		const _ItemDrop *ItemDrop;
		int AIType;

	private:

		void SetTarget(const glm::vec2 &NewTargetPosition);
		void GenerateReactionTime();

		bool LastPlayerVisible;
		int AttacksMade;
		int Goal;
		float AttackRangeSquared;
		float ViewRangeSquared;
		float StopThresholdSquared;
		double StaticTimer;
		double ReactionTimer;
};
