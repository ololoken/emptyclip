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
struct _WeaponParticleTemplate;
class _Player;
struct _ItemDrop;

// Classes
class _Monster : public _Entity {

	public:

		_Monster(_ObjectTemplate &Template);
		~_Monster() override { }

		void Update(double FrameTime) override;
		const _ParticleTemplate *GetWeaponParticle(int Index) const override;

		// Object
		const std::unordered_map<std::string, _Value> &TemplateAttributes;

		// AI
		const _Player *Player;
		const _ItemDrop *ItemDrop;

	private:

		float AttackRangeSquared;
		float ViewRangeSquared;

		_WeaponParticleTemplate *WeaponParticles;
};
