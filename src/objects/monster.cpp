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
_Monster::_Monster(_MonsterTemplate &MonsterTemplate) :
	_Entity() {

	const std::unordered_map<std::string, _Value> &TemplateAttributes = MonsterTemplate.Attributes;

	Type = _Object::MONSTER;
	Level = 1;

	// Monster stats
	Name = MonsterTemplate.Name;
	Color = MonsterTemplate.Color;
	ItemGroupID = MonsterTemplate.ItemGroupID;
	MovementSpeed = TemplateAttributes.at("movement_speed").Float;
	Radius = TemplateAttributes.at("radius").Float;
	Scale = TemplateAttributes.at("scale").Float;
	Recoil = 0;
	RecoilRegen = 0;
	Health = MaxHealth = TemplateAttributes.at("max_health").Int;
	DamageBlock = TemplateAttributes.at("damage_block").Int;
	ExperienceGiven = TemplateAttributes.at("experience").Int;
	MinAccuracy = TemplateAttributes.at("accuracy").Int;
	for(int i = 0; i < WEAPONATTACK_COUNT; i++) {
		int Damage = TemplateAttributes.at("damage").Int;
		MinDamage[i] = Damage;
		MaxDamage[i] = Damage;
		FirePeriod[i] = TemplateAttributes.at("attack_period").Double;
		MaxAccuracy[i] = TemplateAttributes.at("accuracy").Int;
		AttackRange[i] = TemplateAttributes.at("attack_range").Float;
	}
	MainWeaponType = TemplateAttributes.at("weapon_type").Int;
	WeaponParticles = MonsterTemplate.WeaponParticles;

	//PersonalityType = TemplateAttributes.at("ai_type").Int;
	Rotation = ae::GetRandomReal(0.0f, 359.0f);

	WeaponParticleOffset[0] = glm::vec2(0, 0);
	for(int i = 1; i < WEAPON_TYPES; i++)
		WeaponParticleOffset[i] = MONSTER_WEAPONOFFSET * Scale;


	// Set attack sounds
	_SoundGroup *SoundGroup = GameAssets.GetSoundGroupTemplate(MonsterTemplate.SoundGroupID);
	for(int i = 0; i < SOUND_TYPES; i++)
		Sounds[i] = SoundGroup->SoundID[i];

}

// Updates the entity's states
void _Monster::UpdateMonster(double FrameTime, _Player *Player) {
	_Entity::Update(FrameTime);

	if(Player->IsDying())
		return;

	// Update animation
	UpdateAnimation(FrameTime);

	// Move the monster
	if(IsDying())
		return;

	//WallState & WALL_TOP
	//PlayerVisible = IsVisible(Player->Position);
	//FacePosition(Player->Position);
	//StartAttack();
	//Move(FrameTime);
}

// Get weapon particles used by monster
const _ParticleTemplate *_Monster::GetWeaponParticle(int Index) const {
	return WeaponParticles->ParticleTemplates[Index];
}
