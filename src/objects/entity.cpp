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
#include <objects/entity.h>
#include <objects/monster.h>
#include <ae/graphics.h>
#include <ae/random.h>
#include <ae/assets.h>
#include <ae/program.h>
#include <ae/animation.h>
#include <ae/audio.h>
#include <map.h>
#include <constants.h>
#include <iostream>
#include <glm/gtx/norm.hpp>

const double SQRT1_2 = 0.70710678118654752440;

// Constructor
_Entity::_Entity() :
	TriggerDownAudio(nullptr),
	MoveState(MOVE_NONE),
	MovementSpeed(0),
	MovementModifier(1.0f),
	PositionChanged(false),
	Stamina(1),
	MaxStamina(1),
	StaminaRegenModifier(1.0f),
	Tired(false),
	Level(1),
	Health(0),
	MaxHealth(0),
	DamageBlock(0),
	DamageResist(0.0f),
	Action(ACTION_IDLE),
	WalkingAnimation(ENTITY_ANIMATIONWALKING),
	MeleeAnimation(ENTITY_ANIMATIONATTACK),
	ShootingOnehandAnimation(ENTITY_ANIMATIONATTACK),
	ShootingTwohandAnimation(ENTITY_ANIMATIONATTACK),
	DyingAnimation(ENTITY_ANIMATIONDYING),
	CurrentAccuracy(0),
	MinAccuracy(0),
	MaxAccuracy{0, 0},
	AccuracyModifier(1.0f),
	Recoil(0),
	RecoilRegen(0),
	AttackRange{0, 0},
	FireTimer{0, 0},
	FirePeriod{0,0},
	AttackCount(1),
	AttackRequested(false),
	AttackAllowed{true, true},
	AttackMade(false),
	AttackRequestType(0),
	ExperienceGiven(0) {

	for(int i = 0; i < WEAPON_TYPES; i++)
		WeaponParticleOffset[i] = glm::vec2(0.0f, 0.0f);

	for(int i = 0; i < SOUND_TYPES; i++)
		Samples[i] = -1;

	Animation = new ae::_Animation(nullptr);
	Map = nullptr;
}

// Destructor
_Entity::~_Entity() {
	delete Animation;

	StopAudio();
}

// Generates a direction (in degrees) and updates the entity's accuracy
float _Entity::GenerateShotDirection() {
	float RandomOffset, NewDirection;

	// Generate the offset
	RandomOffset = ae::GetRandomReal(-CurrentAccuracy * AccuracyModifier / 2.0f, CurrentAccuracy * AccuracyModifier / 2.0f);

	// Figure out new direction
	NewDirection = Rotation + RandomOffset;

	// Check bounds
	if(NewDirection < 0.0f)
		NewDirection += 360.0f;
	else if(NewDirection >= 360.0f)
		NewDirection -= 360.0f;

	// Update accuracy based on the weapon's recoil
	CurrentAccuracy += Recoil;
	if(CurrentAccuracy > MaxAccuracy[WEAPONATTACK_MAIN])
		CurrentAccuracy = MaxAccuracy[WEAPONATTACK_MAIN];

	return NewDirection;
}

// Generates damage after defenses
int _Entity::GenerateDamage(int AttackType, int DamageBlock, float DamageResist) {
	int Damage = ae::GetRandomInt(MinDamage[AttackType], MaxDamage[AttackType]);
	Damage -= (int)(Damage * DamageResist);
	Damage -= DamageBlock;

	// Cap the damage
	if(Damage < ENTITY_MINDAMAGEPOINTS)
		Damage = ENTITY_MINDAMAGEPOINTS;

	return Damage;
}

// Sets the movement state of the object
void _Entity::SetMoveState(MoveType State) {

	if(!IsMeleeAttacking())
		MoveState = State;
}

// Starts the attack animation
bool _Entity::StartAttack() {

	// Make sure object is allowed to attack
	if(!CanAttack(AttackRequestType))
		return false;

	// Check ammo
	if(!HasAmmo())
		return false;

	// Set animation
	if(AttackRequestType == WEAPONATTACK_MELEE || GetWeaponType() == WEAPON_MELEE) {
		Action = ACTION_STARTMELEE;

		// Play weapon sound
		ae::Audio.PlaySound(ae::Assets.Sounds[GetSample(SOUND_FIRE)], glm::vec3(Position.x, 0.0f, Position.y));
	}
	else
		Action = ACTION_STARTSHOOT;

	ResetAttackAllowed(AttackRequestType);

	return true;
}

// Start playing the trigger down audio loop
void _Entity::StartTriggerDownAudio() {
	if(TriggerDownAudio)
		return;

	const ae::_Sound *Sound = ae::Assets.Sounds[GetSample(SOUND_TRIGGERDOWN)];
	if(Sound) {
		TriggerDownAudio = new ae::_AudioSource(Sound);
		TriggerDownAudio->SetRelative(true);
		TriggerDownAudio->SetLooping(true);
		TriggerDownAudio->Play();
	}
}

// Stop all audio associated with entity
void _Entity::StopAudio() {
	delete TriggerDownAudio;
	TriggerDownAudio = nullptr;
}

// Update the entity
void _Entity::Update(double FrameTime) {
	LastPosition = Position;
	for(int i = 0; i < WEAPONATTACK_COUNT; i++)
		FireTimer[i] += FrameTime;
}

// Updates the animation
void _Entity::UpdateAnimation(double FrameTime, bool PlaySound) {

	// Check action
	switch(Action) {
		case ACTION_IDLE:
			if(!PositionChanged) {
				SetLegAnimationPlayMode(ae::_Animation::STOPPED);
				Animation->Stop();
			}
			else {
				Animation->Play(WalkingAnimation, MovementSpeed);
				SetLegAnimationPlayMode(ae::_Animation::PLAYING);
				SetAnimationPlaybackSpeedFactor();

				Action = ACTION_MOVING;
			}
		break;
		case ACTION_MOVING:
			if(!PositionChanged) {
				Animation->Stop();
				SetLegAnimationPlayMode(ae::_Animation::STOPPED);

				Action = ACTION_IDLE;
			}
		break;
		case ACTION_STARTMELEE:
			Animation->Stop();
			Animation->Play(MeleeAnimation);
			if(Type == _Object::PLAYER)
				Animation->FramePeriod = FirePeriod[WEAPONATTACK_MELEE] / (Animation->Reels[MeleeAnimation]->EndFrame + 1);
			SetLegAnimationPlayMode(ae::_Animation::STOPPED);

			Action = ACTION_MELEE;
			MoveState = MOVE_NONE;
		break;
		case ACTION_MELEE:
			if(Animation->IsStopped()) {
				Animation->Stop();
				Animation->Play(WalkingAnimation, MovementSpeed);
				SetAnimationPlaybackSpeedFactor();

				Action = ACTION_IDLE;
				AttackMade = true;
			}
		break;
		case ACTION_STARTSHOOT:
			if(GetWeaponType() == WEAPON_PISTOL) {
				Animation->Stop();
				Animation->Play(ShootingOnehandAnimation);
			}
			else {
				Animation->Stop();
				Animation->Play(ShootingTwohandAnimation);
			}

			Action = ACTION_SHOOT;
			AttackMade = true;
		break;
		case ACTION_SHOOT:
			if(Animation->IsStopped()) {
				Animation->Stop();
				Animation->Play(WalkingAnimation, MovementSpeed);
				SetAnimationPlaybackSpeedFactor();

				Action = ACTION_IDLE;
			}

			if(PositionChanged)
				SetLegAnimationPlayMode(ae::_Animation::PLAYING);
			else
				SetLegAnimationPlayMode(ae::_Animation::STOPPED);
		break;
		case ACTION_STARTDEATH:
			Animation->Stop();
			Animation->Play(DyingAnimation);
			SetLegAnimationPlayMode(ae::_Animation::STOPPED);
			MoveState = MOVE_NONE;
			IncurDeathPenalty();

			Action = ACTION_DYING;
		break;
		case ACTION_DYING:
			if(Animation->IsStopped())
				Active = false;
		break;
	}

	int LastFrame = Animation->Frame;
	Animation->Update(FrameTime);

	// Play move sound on first and last frame of animation
	if(Animation->Reel == (size_t)WalkingAnimation && PositionChanged && Action == ACTION_MOVING && PlaySound && LastFrame != Animation->Frame && (Animation->Frame == 0 || Animation->Frame == Animation->Reels[Animation->Reel]->EndFrame))
		ae::Audio.PlaySound(ae::Assets.Sounds[GetSample(SOUND_MOVE)], glm::vec3(Position.x, 0.0f, Position.y));
}

// Updates the entity's accuracy according to the weapon's recoil
void _Entity::UpdateRecoil() {

	// Update accuracy based on the weapon's recoil
	CurrentAccuracy -= RecoilRegen;

	// Check bounds
	if(CurrentAccuracy < MinAccuracy)
		CurrentAccuracy = MinAccuracy;
}

// Moves the object with collision detection
void _Entity::Move(double FrameTime) {
	UpdateSpeed(1.0f);
	if(MoveState == MOVE_NONE)
		PositionChanged = false;

	// Make a move
	if(MoveState != MOVE_NONE) {

		// Get direction
		glm::vec2 Goal = GetGoal();
		glm::vec2 NewDirection(0);
		glm::vec2 Delta;
		switch(MoveState) {
			case MOVE_DIRECTION:
				if(MoveDirection.x != 0 || MoveDirection.y != 0) {
					Delta = WallInPath(MoveDirection);
					NewDirection = Delta;
				}
			break;
			case MOVE_GOAL:
				Delta = (Goal - Position);
				if(glm::distance2(Delta, Delta) >= 0.01f) {
					Delta = WallInPath(Delta);
					NewDirection = Delta;
				}
				else {
					Position = Goal;
				}
			break;
			case MOVE_FORWARD:
				NewDirection.y = -1;
			break;
			case MOVE_BACKWARD:
				NewDirection.y = 1;
			break;
			case MOVE_LEFT:
				NewDirection.x = -1;
			break;
			case MOVE_RIGHT:
				NewDirection.x = 1;
			break;
			case MOVE_FORWARDLEFT:
				NewDirection.x = -SQRT1_2;
				NewDirection.y = -SQRT1_2;
			break;
			case MOVE_FORWARDRIGHT:
				NewDirection.x = SQRT1_2;
				NewDirection.y = -SQRT1_2;
			break;
			case MOVE_BACKWARDLEFT:
				NewDirection.x = -SQRT1_2;
				NewDirection.y = SQRT1_2;
			break;
			case MOVE_BACKWARDRIGHT:
				NewDirection.x = SQRT1_2;
				NewDirection.y = SQRT1_2;
			break;
			default:
			break;
		}

		// Moving backwards
		if(glm::dot(NewDirection, Direction) < 0)
			UpdateSpeed(PLAYER_BACKWARDSPEEDFACTOR);

		float Speed = MovementSpeed * MovementModifier * FrameTime;
		NewDirection *= Speed;

		// Get a list of entities that the object is colliding with
		std::list<_Entity *> HitEntities;
		Map->CheckEntityCollisionsInGrid(Position, Radius, this, HitEntities);

		// Limit movement
		for(auto Iterator : HitEntities) {
			glm::vec2 HitObjectDirection = Iterator->Position - Position;

			// Determine if we need to clip the direction
			if(glm::dot(HitObjectDirection, NewDirection) > 0) {
				glm::vec2 DividingLine;

				// Rotate vector
				DividingLine.x = -HitObjectDirection.y;
				DividingLine.y = HitObjectDirection.x;
				DividingLine = glm::normalize(DividingLine);

				// Project the direction onto the dividing line
				NewDirection = DividingLine * glm::dot(NewDirection, DividingLine);
			}
		}

		// Check collisions with walls and map boundaries
		glm::vec2 NewPosition;
		Map->CheckCollisions(Position + NewDirection, Radius, NewPosition);

		// Determine if the object has moved
		if(Position != NewPosition) {
			int AltGridType = (Type == _Object::PLAYER) ? GRID_PLAYER : GRID_MONSTER;

			// Update grid and position
			Map->RemoveObjectFromGrid(this, AltGridType);

			// Check for updated tile position
			glm::ivec2 LastTilePosition = Map->GetValidCoord(Position);
			glm::ivec2 TilePosition = Map->GetValidCoord(NewPosition);
			if(TilePosition != LastTilePosition)
				TileChanged = true;

			Position = NewPosition;

			Map->AddObjectToGrid(this, AltGridType);

			PositionChanged = true;
		}
		else {
			Action = ACTION_IDLE;
			PositionChanged = false;
		}

		// Determine which walls are adjacent to the object
		WallState = Map->GetWallState(Position, Radius);
	}
}

// Draws the object
void _Entity::Render(double BlendFactor) {
	ae::Graphics.SetColor(Color);
	glm::vec2 DrawPosition(Position * (float)BlendFactor + LastPosition * (float)(1.0f - BlendFactor));

	ae::Graphics.SetColor(Color);
	ae::Assets.Programs["pos_uv"]->ResetTextureTransform();
	ae::Graphics.DrawAnimationFrame(
		glm::vec3(DrawPosition, PositionZ),
		Animation->Reels[Animation->Reel]->Texture,
		glm::vec4(Animation->TextureCoords),
		Rotation,
		glm::vec2(Scale)
	);

}

// Updates the Entity's maximum health
void _Entity::UpdateMaxHealth(int Adjust) {

	// Update health
	MaxHealth += Adjust;

	// In case we allow decreasing max health, make sure it stays above 0.
	if(MaxHealth < 1)
		MaxHealth = 1;
}

// Updates the Entity's current health
void _Entity::UpdateHealth(int Adjust) {

	// Update health
	Health += Adjust;

	// Make sure current health doesn't exceed the maximum
	if(Health > MaxHealth)
		Health = MaxHealth;

	// Object has died
	if(Health < 0)
		Health = 0;

	if(Health == 0 && !IsDying()) {
		Action = ACTION_STARTDEATH;
	}
}

glm::vec2 _Entity::GetGoal() const {
	if(Goals.empty())
		return Position;
	else
		return Goals.front();
}

glm::vec2 _Entity::WallInPath(const glm::vec2 &Delta) const {
	int WallState = Map->GetWallState(Position, Radius);
	if(!WallState)
		return glm::normalize(Delta);

	glm::vec2 NewDelta = Delta;
	if((WallState & WALL_RIGHT) && NewDelta.x > 0)
		NewDelta.x = 0;
	if((WallState & WALL_LEFT) && NewDelta.x < 0)
		NewDelta.x = 0;
	if((WallState & WALL_TOP) && NewDelta.y < 0)
		NewDelta.y = 0;
	if((WallState & WALL_BOTTOM) && NewDelta.y > 0)
		NewDelta.y = 0;

	if(!(NewDelta.x == 0 && NewDelta.y == 0))
		NewDelta = glm::normalize(NewDelta);

	return NewDelta;
}
