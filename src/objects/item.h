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

// Forward Declarations
class _Player;

// Mod types
enum ModType {
	MOD_NONE,
	MOD_MAXROUNDS,
	MOD_DAMAGE,
	MOD_ACCURACY,
	MOD_ATTACKSPEED,
	MOD_RELOADSPEED,
	MOD_RELOADAMOUNT,
	MOD_PENETRATION,
	MOD_HANDLING,
	MOD_DAMAGEBLOCK,
	MOD_DAMAGERESIST,
	MOD_MAXAMMO,
	MOD_MOVESPEED,
	MOD_MAXROUNDSPLUS,
	MOD_EXPLOSION,
	MOD_COUNT
};

// Types of weapons
enum WeaponType {
	WEAPON_NONE,
	WEAPON_MELEE,
	WEAPON_PISTOL,
	WEAPON_SHOTGUN,
	WEAPON_RIFLE,
	WEAPON_HEAVY,
	WEAPON_COUNT
};

// Classes
class _Item : public _Object {

	public:

		_Item(const _ObjectTemplate &ItemTemplate);
		~_Item() override;

		void RecalculateStats();
		void Serialize(ae::_Buffer &Buffer) override;
		void DrawTooltip(const _Player *Player, size_t CompareSlot, int InventorySlot, glm::vec2 DrawPosition);
		void Render(double BlendFactor) override;

		bool AddMod(_Item *Mod);
		bool ModCompatible(_Item *Mod);
		float GetBonusMultiplier(int ModType, bool Inverse=false) const;

		int UpdateCount(int Amount) { Count += Amount; return Count; }
		bool CanStack() const { return Type == _Object::MEDKIT; }
		bool CanPickup() const { return Type == _Object::WEAPON || Type == _Object::ARMOR || Type == _Object::MOD; }
		bool CanEquip() const { return Type == _Object::WEAPON || Type == _Object::ARMOR; }
		bool IsAutoPickup() const { return Type == _Object::AMMO || Type == _Object::KEY || Type == _Object::MEDKIT; }

		void SetAmmo(int Value);
		bool IsMelee() const { return Attributes.at("weapon_type").Int == WEAPON_MELEE; }
		float GetAverageDamage() const;
		float GetAverageAccuracy() const;

		virtual std::string GetTypeAsString() const override;
		std::string ModTypeToString(int ModType);

		int Quality{0};
		int Count{1};

		std::vector<_Item *> Mods;
		int Bonus[MOD_COUNT]{0};

	private:

		void DrawAttribute(const std::string &Attribute, const std::string &Label, glm::vec2 &DrawPosition, const _Item *EquippedItem, bool Plus, bool Percent) const;

};
