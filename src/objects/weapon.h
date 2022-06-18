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
#include <objects/item.h>
#include <objects/templates.h>
#include <vector>
#include <string>

// Forward Declarations
struct _ParticleTemplate;
struct _WeaponTemplate;

// Classes
class _Weapon : public _Item {

	public:

		_Weapon(const std::string &Identifier, int Count, const glm::vec2 &Position, const _WeaponTemplate &Weapon, const _Texture *Texture, bool Generate);
		~_Weapon() override;

		void Serialize(ae::_Buffer &Buffer) override;

		void RecalculateStats();
		bool AddComponent(_Item *Upgrade);

		void SetAmmo(int Value);

		const std::string &GetSample(int SampleType) const;

		bool IsMelee() const { return Attributes.at("weapon_type").Int == WEAPON_MELEE; }
		virtual std::string GetTypeAsString() const override { return ToString(Attributes.at("weapon_type").Int) + " class weapon"; }
		static std::string ToString(int Type);

		std::vector<_Item *> Upgrades;
		float Bonus[UPGRADE_TYPES];

	protected:

};
