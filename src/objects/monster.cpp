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
#include <glm/gtx/norm.hpp>

// Constants
const glm::vec2 MONSTER_WEAPONOFFSET = glm::vec2(32.0f / GAME_TILE_SIZE - 0.5f, -0.5f);

// Constructor
_Monster::_Monster(const _ObjectTemplate &MonsterTemplate) :
	_Entity(MonsterTemplate) {

	// Set stats
	MainWeaponType = Template.Attributes.at("weapon_type").Int;

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

	GenerateReactionTime();
}

// Set up stats used by the monster
void _Monster::RecalculateStats(bool SoftReset) {
	ViewRangeSquared = Template.Attributes.at("view_range").Float;
	ViewRangeSquared *= ViewRangeSquared;
	AttackRangeSquared = AttackRange[0];
	AttackRangeSquared *= AttackRangeSquared;
	StopThresholdSquared = ENTITY_STOP_THRESHOLD * MoveSpeed;
	StopThresholdSquared *= StopThresholdSquared;
	CurrentAccuracy = MinAccuracy;
}

// Update
void _Monster::Update(double FrameTime) {
	_Entity::Update(FrameTime);

	// Update animation
	UpdateAnimation(FrameTime);

	// Move the monster
	if(IsDying() || !AIType || Player->InvulnerableTimer > 0.0 || Player->IsDying()) {
		MoveState = MOVE_NONE;
		PositionChanged = false;

		// Keep velocity updated
		if(Player->IsInvulnerable() || IsDying())
			Move(FrameTime);

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
			SetTarget(ReturnPosition, Radius);
		}
	}

	// Check for player in range
	PlayerVisible = false;
	PlayerShootable = false;
	PlayerDistanceSquared = glm::distance2(Position, Player->Position);
	if(PlayerDistanceSquared <= ViewRangeSquared) {
		if(AIType == AI_SIMPLE) {
			PlayerVisible = true;
			SetTarget(Player->Position, Player->Radius);
		}
		else if(Goal == GOAL_PURSUE) {
			if(IsRanged())
				PlayerShootable = Map->IsVisible(Position, Player->Position, _Tile::BULLET);

			// Check if player is visible
			PlayerVisible = (AIType == AI_GHOST) ? true : Map->CanMoveTo(Position, Player->Position, glm::vec2(Radius, Radius) * 0.3f);
			if(PlayerVisible || PlayerShootable) {
				ReactionTimer -= FrameTime;
				if(ReactionTimer <= 0) {
					ReactionTimer = 0.0;
					if(PlayerVisible)
						SetTarget(Player->Position, Player->Radius);
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
			ReturnPosition = SpawnPosition;
			ReturnTimer = Template.IsBoss ? AI_RETURN_TIME_BOSS : AI_RETURN_TIME;
		}
		LastPlayerVisible = PlayerVisible;

		// Check for reaching target
		float TargetDistanceSquared = glm::distance2(Position, TargetPosition);
		float RadiiSum = Radius + TargetRadius;
		if(TargetDistanceSquared <= RadiiSum * RadiiSum * 1.1f) {
			if(MoveState != MOVE_NONE && !PlayerVisible)
				GenerateReactionTime();

			MoveState = MOVE_NONE;
			Goal = GOAL_PURSUE;
		}
	}

	// Check for attacks
	if(PlayerIsAttackable() && CanAttack(WEAPONATTACK_MAIN))
		StartAttack();

	// Move
	Move(FrameTime);

	// Move out of walls
	if(!FreePathing && !Map->CanPass(Map->GetValidCoord(Position), _Tile::ENTITY)) {
		Map->RemoveObjectFromGrid(this, GRID_MONSTER);
		SetPosition(SpawnPosition);
		Map->AddObjectToGrid(this, GRID_MONSTER);
	}

	// Check static timer
	if(MoveState != MOVE_NONE) {

		// Check for inactive distance
		if(glm::distance2(LastPosition, Position) > StopThresholdSquared)
			StaticTimer = 0;

		// Check for oscillating between opposite directions
		if((LastDirection[0] < 0.0f && Direction[0] > 0.0f) || (LastDirection[0] > 0.0f && Direction[0] < 0.0f) || (LastDirection[1] < 0.0f && Direction[1] > 0.0f) || (LastDirection[1] > 0.0f && Direction[1] < 0.0f)) {
			Wiggles++;
			if(Wiggles >= 3) {
				StaticTimer = ENTITY_STATIC_TIME;
				Wiggles = 0;
				FacePosition(TargetPosition);
			}
		}
		else
			Wiggles = 0;

		// Stop monster when static
		if(StaticTimer >= ENTITY_STATIC_TIME) {
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
	if(AIType == AI_HITANDRUN && AttacksMade >= AIAttacks) {
		AttacksMade = 0;
		Goal = GOAL_RETREAT;
		ReturnPosition = Position;
		ReturnTimer = AI_RETREAT_TIME;

		// Ray cast away from player
		std::vector<_Hit> Hits;
		Map->CheckBulletCollisions(this, glm::normalize(Position - Player->Position), Hits, GRID_MONSTER, true, 1, _Tile::ENTITY);
		if(Hits.size())
			SetTarget(Hits.front().Position, Radius);
	}
}

// Called when the monster gets hit
void _Monster::OnHit(_Entity *Attacker, const _Hit &Hit, bool PlaySound) {
	_Entity::OnHit(Attacker, Hit, PlaySound);

	if(IsCrate())
		return;

	Goal = GOAL_PURSUE;
	if(Attacker)
		SetTarget(Attacker->Position, Attacker->Radius);
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

	return ParticleTemplate[ae::GetRandomInt<size_t>(0, ParticleTemplate.size()-1)];
}

// Set a target position
void _Monster::SetTarget(const glm::vec2 &NewTargetPosition, float NewTargetRadius) {
	FacePosition(NewTargetPosition);
	TargetPosition = NewTargetPosition;
	TargetRadius = NewTargetRadius;
	MoveState = MOVE_TARGET;
	StaticTimer = 0.0;
}

// Reset the reaction timer
void _Monster::GenerateReactionTime() {
	if(ReactionTimer <= 0.0)
		ReactionTimer = ae::GetRandomReal(AI_REACTION_TIME_MIN, AI_REACTION_TIME_MAX);
}
