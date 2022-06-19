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

#include <value.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <unordered_map>
#include <string>
#include <cstdint>

namespace ae {
	class _Texture;
	class _Font;
}

// Determines if you can hold down the mouse to fire or not
enum FireRateType {
	FIRERATE_SEMI,
	FIRERATE_AUTO
};

// Types of weapon samples
enum WeaponSampleTypes {
	WEAPON_FIRESAMPLE,
	WEAPON_MISSSAMPLE,
	WEAPON_RICOCHETSAMPLE,
	WEAPON_EMPTYSAMPLE,
	WEAPON_RELOADSAMPLE
};

// Types of weapon particles
enum WeaponParticleTypes {
	WEAPONPARTICLE_FIRE,
	WEAPONPARTICLE_SMOKE,
	WEAPONPARTICLE_RICOCHET,
	WEAPONPARTICLE_BULLETHOLE,
	WEAPONPARTICLE_TYPES
};

// Types of samples
enum SampleTypes {
	SAMPLE_FIRE,
	SAMPLE_TRIGGERDOWN,
	SAMPLE_RICOCHET,
	SAMPLE_EMPTY,
	SAMPLE_RELOAD,
	SAMPLE_HIT,
	SAMPLE_TAKEDAMAGE,
	SAMPLE_DEATH,
	SAMPLE_MOVE,
	SAMPLE_TYPES,
};

// Skill types
enum SkillTypes {
	SKILL_STRENGTH,
	SKILL_HEALTH,
	SKILL_ACCURACY,
	SKILL_RELOADSPEED,
	SKILL_ATTACKSPEED,
	SKILL_MOVESPEED,
	SKILL_DAMAGERESIST,
	SKILL_MAXINVENTORY,
	SKILL_MAXSTAMINA,
	SKILL_UNUSED2,
	SKILL_MAXUSED = SKILL_UNUSED2,
	SKILL_UNUSED3,
	SKILL_UNUSED4,
	SKILL_UNUSED5,
	SKILL_UNUSED6,
	SKILL_UNUSED7,
	SKILL_UNUSED8,
	SKILL_UNUSED9,
	SKILL_UNUSED10,
	SKILL_COUNT,
};

struct _ParticleTemplate {
	glm::vec2 StartDirection;
	glm::vec2 VelocityScale;
	glm::vec2 TurnSpeed;
	glm::vec2 Size;
	glm::vec4 Color;
	const ae::_Texture *Texture;
	const ae::_Font *Font;
	double Lifetime;
	float AccelerationScale;
	float AlphaSpeed;
	float DeviationZ;
	float ScaleAspect;
	int Count;
	int Type;
};

struct _WeaponParticleTemplate {

	_WeaponParticleTemplate() {
		for(int i = 0; i < WEAPONPARTICLE_TYPES; i++)
			ParticleTemplates[i] = nullptr;
	}

	_ParticleTemplate *ParticleTemplates[WEAPONPARTICLE_TYPES];
};
