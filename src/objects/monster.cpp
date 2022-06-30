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
const glm::vec2 MONSTER_WEAPONOFFSET = glm::vec2(32.0f / 64.0f - 0.5f, -0.5f);

// Enumerations
enum AITypes {
	AI_NONE,
	AI_ZOMBIE,
	AI_COUNT
};

// Constructor
_Monster::_Monster(const _ObjectTemplate &MonsterTemplate) :
	_Entity(MonsterTemplate),
	Player(nullptr),
	ItemDrop(nullptr) {

	// Set stats
	MainWeaponType = Template.Attributes.at("weapon_type").Int;

	// Cache distances
	AttackRangeSquared = Template.Attributes.at("attack_range").Float;
	AttackRangeSquared *= AttackRangeSquared;
	ViewRangeSquared = Template.Attributes.at("view_range").Float;
	ViewRangeSquared *= ViewRangeSquared;

	// Set weapon offsets
	WeaponParticles = Template.WeaponParticles;
	WeaponParticleOffset[0] = glm::vec2(0, 0);
	for(int i = 1; i < WEAPON_TYPES; i++)
		WeaponParticleOffset[i] = MONSTER_WEAPONOFFSET * Scale;

	// Set attack sounds
	_SoundGroup *SoundGroup = GameAssets.GetSoundGroupTemplate(Template.SoundGroupID);
	for(int i = 0; i < SOUND_TYPES; i++)
		Sounds[i] = SoundGroup->SoundID[i];

	//AIType = TemplateAttributes.at("ai_type").Int;
	Rotation = ae::GetRandomReal(0.0f, 359.0f);
	LastPlayerVisible = false;
}

// Update
void _Monster::Update(double FrameTime) {
	_Entity::Update(FrameTime);
	if(!Player || Player->IsDying())
		return;

	// Update animation
	UpdateAnimation(FrameTime);

	// Move the monster
	if(IsDying())
		return;

	// Check for player in range
	bool PlayerVisible = false;
	float PlayerDistanceSquared = glm::distance2(Position, Player->Position);
	if(PlayerDistanceSquared <= ViewRangeSquared) {

		// Check if player is visible
		PlayerVisible = Map->IsVisible(Position, Player->Position);
		if(PlayerVisible) {
			FacePosition(Player->Position);
			TargetPosition = Player->Position;
			MoveState = MOVE_TARGET;
		}
	}

	// Center target position when vision is lost
	if(!PlayerVisible && PlayerVisible != LastPlayerVisible)
		TargetPosition = glm::vec2(glm::ivec2(TargetPosition)) + glm::vec2(0.5f);
	LastPlayerVisible = PlayerVisible;

	// Check for reaching target
	float TargetDistanceSquared = glm::distance2(Position, TargetPosition);
	if(TargetDistanceSquared <= 0.1f)
		MoveState = MOVE_NONE;

	// Check for attack range
	if(PlayerDistanceSquared <= AttackRangeSquared)
		StartAttack();

	// Move
	MoveState = MOVE_NONE;
	Move(FrameTime);
}

// Get weapon particles used by monster
const _ParticleTemplate *_Monster::GetWeaponParticle(int Index) const {
	return WeaponParticles->ParticleTemplates[Index];
}
