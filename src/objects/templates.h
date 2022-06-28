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

// Types of weapon sounds
enum WeaponSoundTypes {
	WEAPONSOUND_FIRE,
	WEAPONSOUND_MISS,
	WEAPONSOUND_RICOCHET,
	WEAPONSOUND_EMPTY,
	WEAPONSOUND_RELOAD
};

// Types of weapon particles
enum WeaponParticleTypes {
	WEAPONPARTICLE_FIRE,
	WEAPONPARTICLE_SMOKE,
	WEAPONPARTICLE_RICOCHET,
	WEAPONPARTICLE_BULLETHOLE,
	WEAPONPARTICLE_TYPES
};

// Types of sounds
enum SoundTypes {
	SOUND_FIRE,
	SOUND_TRIGGERDOWN,
	SOUND_RICOCHET,
	SOUND_EMPTY,
	SOUND_RELOAD,
	SOUND_HIT,
	SOUND_TAKEDAMAGE,
	SOUND_DEATH,
	SOUND_MOVE,
	SOUND_TYPES,
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
	SKILL_MAXSTAMINA,
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
