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
#include <objects/object.h>

// Upgrade component types
enum UpgradeType {
	UPGRADE_CLIP,
	UPGRADE_DAMAGE,
	UPGRADE_ACCURACY,
	UPGRADE_FIREPERIOD,
	UPGRADE_RELOADPERIOD,
	UPGRADE_ATTACKS,
	UPGRADE_TYPES
};

// Types of weapons
enum WeaponType {
	WEAPON_MELEE,
	WEAPON_PISTOL,
	WEAPON_SHOTGUN,
	WEAPON_RIFLE,
	WEAPON_HEAVY,
	WEAPON_TYPES
};

// Classes
class _Item : public _Object {

	public:

		_Item();

		void Serialize(ae::_Buffer &Buffer) override;
		void Render(double BlendFactor) override;

		int UpdateCount(int Amount) { Count += Amount; return Count; }
		bool CanStack() { return !(Type == _Object::WEAPON || Type == _Object::ARMOR || Type == _Object::UPGRADE); }

		float GetAverageDamage() const;
		float GetAverageAccuracy() const;

		virtual std::string GetTypeAsString() const override;

		int Level;
		int Quality;
		int Count;

};
