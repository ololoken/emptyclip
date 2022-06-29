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

// Types of weapons
enum WeaponType {
	WEAPON_NONE,
	WEAPON_MELEE,
	WEAPON_PISTOL,
	WEAPON_SHOTGUN,
	WEAPON_RIFLE,
	WEAPON_HEAVY,
	WEAPON_TYPES
};

// Classes
class _Weapon : public _Item {

	public:

		_Weapon(const _ObjectTemplate &Template) : _Item(Template) { }
		~_Weapon() override;

		void Serialize(ae::_Buffer &Buffer) override;

		void RecalculateStats() override;
		bool AddComponent(_Item *Upgrade);
		void SetAmmo(int Value);

		float GetBonusMultiplier(int UpgradeType) const { return (100 + Bonus[UpgradeType]) * 0.01f; }
		const std::string &GetSound(int SoundType) const;
		bool IsMelee() const { return Attributes.at("weapon_type").Int == WEAPON_MELEE; }
		virtual std::string GetTypeAsString() const override { return ToString(Attributes.at("weapon_type").Int) + " class weapon"; }
		static std::string ToString(int Type);

		std::vector<_Item *> Upgrades;
		int Bonus[UPGRADE_TYPES];

	protected:

};
