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
#include <algorithm>
#include <glm/gtx/norm.hpp>

const double SQRT1_2 = 0.70710678118654752440;

// Constructor
_Entity::_Entity(const _ObjectTemplate &EntityTemplate) :
	_Object(EntityTemplate),
	TriggerDownAudio(nullptr),
	MoveState(MOVE_NONE),
	BaseMoveSpeed(0),
	MoveSpeed(0),
	MoveModifier(1.0f),
	PositionChanged(false),
	Stamina(1),
	MaxStamina(1),
	StaminaRegenModifier(1.0f),
	WallState(0),
	Tired(false),
	Health(0),
	MaxHealth(0),
	DamageBlock(0),
	DamageResist(0),
	Action(ACTION_IDLE),
	WalkingAnimation(ENTITY_ANIMATIONWALKING),
	MeleeAnimation(ENTITY_ANIMATIONATTACK),
	ShootingOnehandAnimation(ENTITY_ANIMATIONATTACK),
	ShootingTwohandAnimation(ENTITY_ANIMATIONATTACK),
	DyingAnimation(ENTITY_ANIMATIONDYING),
	CurrentAccuracy(0),
	MinAccuracy(0),
	MaxAccuracy{0, 0},
	Recoil(0),
	RecoilRegen(0),
	RecoilModifier(1.0f),
	MoveRecoil(0.0f),
	AttackRange{0, 0},
	AttackTimer{0, 0},
	AttackPeriod{0, 0},
	AttackWidth{0, 0},
	AttackCount{1, 1},
	AttackRequestType(0),
	AttackRequested(false),
	AttackAllowed{true, true},
	AttackMade(false),
	ExperienceGiven(0) {

	for(int i = 0; i < WEAPON_COUNT; i++)
		WeaponOffset[i] = glm::vec2(0.0f, 0.0f);

	for(int i = 0; i < WEAPONATTACK_COUNT; i++)
		Penetration[i] = 1;

	for(int i = 0; i < SOUND_COUNT; i++)
		Sounds[i] = nullptr;

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
	float RandomOffset;
	float NewDirection;

	// Generate the offset
	RandomOffset = ae::GetRandomReal(-CurrentAccuracy / 2.0f, CurrentAccuracy / 2.0f);

	// Figure out new direction
	NewDirection = Rotation + RandomOffset;

	// Check bounds
	if(NewDirection < 0.0f)
		NewDirection += 360.0f;
	else if(NewDirection >= 360.0f)
		NewDirection -= 360.0f;

	// Update accuracy based on the weapon's recoil
	CurrentAccuracy += Recoil * RecoilModifier;
	if(CurrentAccuracy > MaxAccuracy[WEAPONATTACK_MAIN])
		CurrentAccuracy = MaxAccuracy[WEAPONATTACK_MAIN];

	return NewDirection;
}

// Generates damage after defenses
int _Entity::GenerateDamage(int AttackType, int DamageBlock, int DamageResist) {
	int Damage = ae::GetRandomInt(MinDamage[AttackType], MaxDamage[AttackType]);
	Damage -= (int)(Damage * DamageResist * 0.01f);
	Damage -= DamageBlock;

	// Cap the damage
	if(Damage < ENTITY_MINDAMAGEPOINTS)
		Damage = ENTITY_MINDAMAGEPOINTS;

	return Damage;
}

// Starts the attack animation
bool _Entity::StartAttack() {

	// Make sure object is allowed to attack
	if(!CanAttack(AttackRequestType))
		return false;

	// Check ammo
	if(!WeaponHasAmmo(AttackRequestType))
		return false;

	// Set animation
	if(AttackRequestType == WEAPONATTACK_MELEE || MainWeaponType == WEAPON_MELEE) {
		Action = ACTION_STARTMELEE;

		// Play weapon sound
		ae::Audio.PlaySound(GetSound(SOUND_FIRE, AttackRequestType), glm::vec3(Position.x, 0.0f, Position.y));
	}
	else {
		Action = ACTION_STARTSHOOT;
		ResetAttackAllowed(WEAPONATTACK_MELEE);
	}

	ResetAttackAllowed(AttackRequestType);

	return true;
}

// Start playing the trigger down audio loop
void _Entity::StartTriggerDownAudio() {
	if(TriggerDownAudio)
		return;

	const ae::_Sound *Sound = GetSound(SOUND_TRIGGERDOWN, AttackRequestType);
	if(!Sound)
		return;

	TriggerDownAudio = new ae::_AudioSource(Sound);
	TriggerDownAudio->SetRelative(true);
	TriggerDownAudio->SetLooping(true);
	TriggerDownAudio->Play();
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
		AttackTimer[i] += FrameTime;

	// Check timer to see if the object can attack
	for(int i = 0; i < WEAPONATTACK_COUNT; i++) {
		if(!AttackAllowed[i] && AttackTimer[i] >= AttackPeriod[i])
			AttackAllowed[i] = true;
	}

	UpdateRecoil(FrameTime);
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
				Animation->Play(WalkingAnimation, MoveSpeed);
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
				Animation->FramePeriod = AttackPeriod[WEAPONATTACK_MELEE] / (Animation->Reels[MeleeAnimation]->EndFrame + 1);
			SetLegAnimationPlayMode(ae::_Animation::STOPPED);

			Action = ACTION_MELEE;
		break;
		case ACTION_MELEE:
			if(Animation->IsStopped()) {
				Animation->Stop();
				Animation->Play(WalkingAnimation, MoveSpeed);
				SetAnimationPlaybackSpeedFactor();

				Action = ACTION_IDLE;
			}
		break;
		case ACTION_STARTSHOOT:
			if(MainWeaponType == WEAPON_PISTOL) {
				Animation->Stop();
				Animation->Play(ShootingOnehandAnimation);
			}
			else {
				Animation->Stop();
				Animation->Play(ShootingTwohandAnimation);
			}
			Animation->FramePeriod = AttackPeriod[WEAPONATTACK_MAIN];

			Action = ACTION_SHOOT;
			AttackMade = true;
		break;
		case ACTION_SHOOT:
			if(Animation->IsStopped()) {
				Animation->Stop();
				Animation->Play(WalkingAnimation, MoveSpeed);
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

	if(LastFrame != Animation->Frame) {
		if(Action == ACTION_MELEE && Animation->Frame == 1)
			AttackMade = true;

		// Play move sound on first and last frame of animation
		if(Animation->Reel == (size_t)WalkingAnimation && PositionChanged && Action == ACTION_MOVING && PlaySound && (Animation->Frame == 0 || Animation->Frame == Animation->Reels[Animation->Reel]->EndFrame))
			ae::Audio.PlaySound(GetSound(SOUND_MOVE, -1), glm::vec3(Position.x, 0.0f, Position.y));
	}
}

// Updates the entity's accuracy according to the weapon's recoil
void _Entity::UpdateRecoil(double FrameTime) {

	// Update accuracy
	CurrentAccuracy -= RecoilRegen * FrameTime / RecoilModifier;
	if(CurrentAccuracy < MinAccuracy)
		CurrentAccuracy = MinAccuracy;
}

// Moves the object with collision detection
void _Entity::Move(double FrameTime) {
	UpdateSpeed(1.0f);

	// Check for moving
	if(MoveState == MOVE_NONE) {
		PositionChanged = false;
		return;
	}

	// Get move direction
	glm::vec2 MoveDirection(0);
	switch(MoveState) {
		case MOVE_TARGET: {

			// Get vector to target
			glm::vec2 TargetVector = TargetPosition - Position;

			// Correct move direction based on wall state
			if(WallState) {
				if((WallState & WALL_RIGHT) && TargetVector.x > 0)
					TargetVector.x = 0;
				if((WallState & WALL_LEFT) && TargetVector.x < 0)
					TargetVector.x = 0;
				if((WallState & WALL_TOP) && TargetVector.y < 0)
					TargetVector.y = 0;
				if((WallState & WALL_BOTTOM) && TargetVector.y > 0)
					TargetVector.y = 0;
			}

			// Set move direction
			if(TargetVector.x != 0 || TargetVector.y != 0) {
				Direction = MoveDirection = glm::normalize(TargetVector);
				Rotation = glm::degrees(atan2(MoveDirection.y, MoveDirection.x)) + 90.0f;
				if(Rotation < 0.0f)
					Rotation += 360.0f;
			}
		} break;
		case MOVE_FORWARD:
			MoveDirection.y = -1;
		break;
		case MOVE_BACKWARD:
			MoveDirection.y = 1;
		break;
		case MOVE_LEFT:
			MoveDirection.x = -1;
		break;
		case MOVE_RIGHT:
			MoveDirection.x = 1;
		break;
		case MOVE_FORWARDLEFT:
			MoveDirection.x = -SQRT1_2;
			MoveDirection.y = -SQRT1_2;
		break;
		case MOVE_FORWARDRIGHT:
			MoveDirection.x = SQRT1_2;
			MoveDirection.y = -SQRT1_2;
		break;
		case MOVE_BACKWARDLEFT:
			MoveDirection.x = -SQRT1_2;
			MoveDirection.y = SQRT1_2;
		break;
		case MOVE_BACKWARDRIGHT:
			MoveDirection.x = SQRT1_2;
			MoveDirection.y = SQRT1_2;
		break;
		default:
		break;
	}

	// Moving backwards
	if(glm::dot(MoveDirection, Direction) < 0)
		UpdateSpeed(PLAYER_BACKWARDS_SPEEDFACTOR);

	// Get speed
	float Speed = MoveSpeed * MoveModifier * FrameTime;

	// Update speed while attacking
	if(Action == ACTION_SHOOT || Action == ACTION_MELEE)
		Speed *= AttackMoveSpeed[AttackRequestType];

	// Update move vector
	MoveDirection *= Speed;

	// Update accuracy
	if(MoveState) {
		CurrentAccuracy += Speed * MoveRecoil;
		if(CurrentAccuracy > MaxAccuracy[WEAPONATTACK_MAIN])
			CurrentAccuracy = MaxAccuracy[WEAPONATTACK_MAIN];
	}

	// Get a list of entities that the object is colliding with
	std::vector<_Hit> Hits;
	glm::vec2 NewPosition = Position + MoveDirection;
	bool AxisAlignedPush = false;
	Map->CheckEntityCollisionsInGrid(NewPosition, Radius, this, Hits, AxisAlignedPush);

	// Resolve pushes
	for(auto Hit : Hits) {

		// If at least one push is axis aligned, don't push with diagonals
		if(AxisAlignedPush && Hit.Push.x != 0 && Hit.Push.y != 0)
			continue;

		NewPosition += Hit.Push;
	}

	// Check collisions with walls and map boundaries
	Map->CheckTileCollisions(NewPosition, Radius, NewPosition);

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
	else
		PositionChanged = false;

	// Determine which walls are adjacent to the object
	WallState = Map->GetWallState(Position, Radius);
}

// Draws the object
void _Entity::Render(double BlendFactor) {
	ae::Graphics.SetColor(Color);
	glm::vec2 DrawPosition(Position * (float)BlendFactor + LastPosition * (float)(1.0f - BlendFactor));

	ae::Graphics.SetColor(Color);
	ae::Graphics.DrawAnimationFrame(
		glm::vec3(DrawPosition, PositionZ),
		Animation->Reels[Animation->Reel]->Texture,
		glm::vec4(Animation->TextureCoords),
		Rotation,
		glm::vec2(Scale)
	);

	/*
	ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
	ae::Graphics.SetDepthMask(false);
	ae::Graphics.SetDepthTest(false);
	ae::Graphics.SetColor(COLOR_WHITE);
	if(Circle)
		ae::Graphics.DrawCircle(glm::vec3(DrawPosition, 0), Radius);
	else
		ae::Graphics.DrawRectangle3D(Position - glm::vec2(Radius), Position + glm::vec2(Radius), false);
	ae::Graphics.SetDepthTest(true);
	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
	*/
}

// Update current health
void _Entity::UpdateHealth(int Adjust) {
	Health = std::clamp(Health + Adjust, 0, MaxHealth);
	if(Health == 0 && !IsDying())
		Action = ACTION_STARTDEATH;
}
