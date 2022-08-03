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
#include <objects/monster.h>
#include <objects/player.h>
#include <ae/random.h>
#include <gameassets.h>
#include <stats.h>
#include <map.h>
#include <iostream>
#include <glm/gtx/norm.hpp>

// Constants
const glm::vec2 MONSTER_WEAPONOFFSET = glm::vec2(32.0f / 64.0f - 0.5f, -0.5f);

// Constructor
_Monster::_Monster(const _ObjectTemplate &MonsterTemplate) :
	_Entity(MonsterTemplate),
	Player(nullptr),
	ItemDrop(nullptr),
	ReturnPosition(0.0f) {

	// Set stats
	MainWeaponType = Template.Attributes.at("weapon_type").Int;

	// Cache distances
	AttackRangeSquared = Template.Attributes.at("attack_range").Float;
	AttackRangeSquared *= AttackRangeSquared;
	ViewRangeSquared = Template.Attributes.at("view_range").Float;
	ViewRangeSquared *= ViewRangeSquared;

	// Set weapon offsets
	WeaponOffset[0] = glm::vec2(0, 0);
	for(int i = 1; i < WEAPON_COUNT; i++)
		WeaponOffset[i] = MONSTER_WEAPONOFFSET * Scale;

	// Set attack sounds
	_SoundGroup &SoundGroup = GameAssets.SoundGroups.at(Template.SoundGroupID);
	for(int i = 0; i < SOUND_COUNT; i++)
		Sounds[i] = SoundGroup.SoundID[i];

	AIType = Template.Attributes.at("ai_type").Int;
	if(IsCrate())
		Circle = false;
	else
		Rotation = ae::GetRandomReal(0.0f, 359.0f);

	LastPlayerVisible = false;
	Goal = GOAL_PURSUE;
	AttacksMade = 0;
	StaticTimer = 0.0;
	ReactionTimer = 0.0;
	ReturnTimer = 0.0;
	GenerateReactionTime();
}

// Set up stats used by the monster
void _Monster::RecalculateStats() {
	StopThresholdSquared = ENTITY_STOP_THRESHOLD * MoveSpeed;
	StopThresholdSquared *= StopThresholdSquared;
}

// Update
void _Monster::Update(double FrameTime) {

	// Update free pathing
	if(FreePathingTimer > 0.0) {
		FreePathingTimer -= FrameTime;
		if(FreePathingTimer < 0.0)
			FreePathingTimer = 0.0f;
	}

	// Update animation
	UpdateAnimation(FrameTime);

	// Move the monster
	if(IsDying() || !AIType || Player->IsInvulnerable() || Player->IsDying()) {
		MoveState = MOVE_NONE;
		PositionChanged = false;
		return;
	}

	// Update timers
	StaticTimer += FrameTime;
	for(int i = 0; i < WEAPONATTACK_COUNT; i++)
		AttackTimer[i] += FrameTime;

	// Handle returning to original position
	if(ReturnTimer > 0.0) {
		ReturnTimer -= FrameTime;
		if(ReturnTimer <= 0.0) {
			ReturnTimer = 0.0;
			Goal = GOAL_PURSUE;
			SetTarget(ReturnPosition);
		}
	}

	// Check for player in range
	bool PlayerVisible = false;
	float PlayerDistanceSquared = glm::distance2(Position, Player->Position);
	if(PlayerDistanceSquared <= ViewRangeSquared) {
		if(AIType == AI_SIMPLE) {
			PlayerVisible = true;
			SetTarget(Player->Position);
		}
		else if(Goal == GOAL_PURSUE) {

			// Check if player is visible
			PlayerVisible = CanFreePath() ? true : Map->CanMoveTo(Position, Player->Position, glm::vec2(Radius, Radius) * 0.3f);
			if(PlayerVisible) {
				ReactionTimer -= FrameTime;
				if(ReactionTimer <= 0) {
					ReactionTimer = 0.0;
					SetTarget(Player->Position);
				}
			}
			else
				GenerateReactionTime();
		}
	}
	// Stop AI if player gets too far away
	else if(PlayerDistanceSquared >= ENTITY_MAX_ACTIVE_RANGE * ENTITY_MAX_ACTIVE_RANGE) {
		PlayerVisible = false;
		MoveState = MOVE_NONE;
		GenerateReactionTime();
	}

	// Set return position if monster can't see player anymore
	if(AIType != AI_SIMPLE) {
		if(PlayerVisible != LastPlayerVisible && !PlayerVisible && Goal == GOAL_PURSUE) {
			ReturnPosition = Position;
			ReturnTimer = AI_RETURN_TIME;
		}
		LastPlayerVisible = PlayerVisible;

		// Check for reaching target
		float TargetDistanceSquared = glm::distance2(Position, TargetPosition);
		if(TargetDistanceSquared <= Radius * Radius * 1.1f) {
			if(MoveState != MOVE_NONE)
				GenerateReactionTime();

			MoveState = MOVE_NONE;
			Goal = GOAL_PURSUE;
		}
	}

	// Check for attack range
	if(PlayerDistanceSquared <= AttackRangeSquared)
		StartAttack();

	// Move
	if(MoveState == MOVE_NONE) {
		PositionChanged = false;
	}
	else {
		LastPosition = Position;
		Move(FrameTime);

		// Check for inactive distance
		if(glm::distance2(LastPosition, Position) > StopThresholdSquared) {
			StaticTimer = 0;
		}
		// Stop monster when static
		else if(StaticTimer > ENTITY_STATIC_TIME) {
			MoveState = MOVE_NONE;
			Goal = GOAL_PURSUE;
			GenerateReactionTime();
		}
	}
}

// Called when the monster lands a hit
void _Monster::OnAttack(_Entity *Victim, const _Hit &Hit) {
	_Entity::OnAttack(Victim, Hit);

	AttacksMade++;
	if(AIType == AI_HITANDRUN && AttacksMade >= Template.Attributes.at("ai_attacks").Int) {
		AttacksMade = 0;
		Goal = GOAL_RETREAT;
		ReturnPosition = Position;
		ReturnTimer = AI_RETREAT_TIME;

		// Ray cast away from player
		std::vector<_Hit> Hits;
		Map->CheckBulletCollisions(this, glm::normalize(Position - Player->Position), Hits, GRID_MONSTER, true, 1, _Tile::ENTITY);
		if(Hits.size())
			SetTarget(Hits.front().Position);
	}
}

// Called when the monster gets hit
void _Monster::OnHit(_Entity *Attacker, const _Hit &Hit) {
	_Entity::OnHit(Attacker, Hit);

	if(IsCrate())
		return;

	Goal = GOAL_PURSUE;
	SetTarget(Attacker->Position);
}

// Called when the player dies
void _Monster::OnPlayerDeath() {
	MoveState = MOVE_NONE;
	PositionChanged = false;
	StaticTimer = 0;
	AttacksMade = 0;
	Goal = GOAL_PURSUE;
	GenerateReactionTime();
}

// Get weapon particles used by monster
const _ParticleTemplate *_Monster::GetParticle(int ParticleType) const {
	const auto &ParticleTemplate = Template.ParticleGroup->ParticleTemplates[ParticleType];
	if(ParticleTemplate.empty())
		return nullptr;

	return ParticleTemplate[ae::GetRandomInt((size_t)0, ParticleTemplate.size()-1)];
}

// Set a target position
void _Monster::SetTarget(const glm::vec2 &NewTargetPosition) {
	FacePosition(NewTargetPosition);
	TargetPosition = NewTargetPosition;
	MoveState = MOVE_TARGET;
	StaticTimer = 0.0;
}

// Reset the reaction timer
void _Monster::GenerateReactionTime() {
	if(ReactionTimer <= 0.0)
		ReactionTimer = ae::GetRandomReal(AI_REACTION_TIME_MIN, AI_REACTION_TIME_MAX);
}
