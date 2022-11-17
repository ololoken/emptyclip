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
struct _Slot;
namespace ae {
	class _Font;
}

// Mod types
enum ModType {
	MOD_NONE,
	MOD_DAMAGE,
	MOD_ACCURACY,
	MOD_HANDLING,
	MOD_ATTACKSPEED,
	MOD_RELOADSPEED,
	MOD_MAXROUNDS,
	MOD_DAMAGEBLOCK,
	MOD_DAMAGERESIST,
	MOD_MOVESPEED,
	MOD_MAXSTAMINA,
	MOD_MAXAMMO,
	MOD_MAXHEALTH,
	MOD_MELEEDAMAGE,
	MOD_PISTOLDAMAGE,
	MOD_SHOTGUNDAMAGE,
	MOD_RIFLEDAMAGE,
	MOD_HEAVYDAMAGE,
	MOD_PENETRATION,
	MOD_BOUNCE,
	MOD_RELOADAMOUNT,
	MOD_MAXROUNDSPLUS,
	MOD_CRITCHANCE,
	MOD_SPREAD,
	MOD_EXPLOSION,
	MOD_SEMIAUTO,
	MOD_BURST,
	MOD_FULLAUTO,
	MOD_COUNT
};

// Mod class types
enum ModClassType {
	MODCLASS_NONE,
	MODCLASS_WEAPON,
	MODCLASS_ARMOR,
	MODCLASS_SPECIAL,
	MODCLASS_CHANGE,
};

// Usable types
enum UsableType {
	USABLE_NONE,
	USABLE_HAMMER,
	USABLE_WHETSTONE,
	USABLE_COUNT,
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

// Types of bag icons
enum BagIconType {
	BAGICON_DEFAULT,
	BAGICON_GEAR_WEAPON,
	BAGICON_GEAR_ARMOR,
	BAGICON_MOD_WEAPON,
	BAGICON_MOD_ARMOR,
	BAGICON_MOD_SPECIAL,
	BAGICON_MOD_CHANGE,
	BAGICON_USABLE,
	BAGICON_COUNT,
};

// Classes
class _Item : public _Object {

	public:

		_Item(const _ObjectTemplate &ItemTemplate);
		~_Item() override;

		void RecalculateStats();
		void RecalculateModBonus();
		void Serialize(ae::_Buffer &Buffer) override;
		void DrawTooltip(const _Player *Player, glm::vec2 DrawPosition, const _Item *EquippedItem, const _Slot &Slot, bool ShowEquipHelp) const;
		void Render(double BlendFactor) const override;

		bool AddMod(_Item *Mod, bool Recalculate=true);
		bool ApplyUsable(_Item *Usable);
		bool ItemCompatible(_Item *Item, bool CheckCount=true) const;
		bool ModCompatible(_Item *Mod, bool CheckCount=true) const;
		float GetBonusMultiplier(int ModType, bool Inverse=false) const;
		void SetMaxMods();
		float GetMaxMods(bool Round) const;
		int GetModType() const;
		bool IsChangeMod() const;
		int GetIconType() const;

		int UpdateCount(int Amount) { Count += Amount; return Count; }

		void SetAmmo(int Value);
		bool IsMelee() const;
		float GetAverageDamage() const;
		float GetAverageAccuracy() const;
		void GetQualityColor(glm::vec4 &ReturnColor) const;
		int GetHammerQualityChange() const;
		int GetWhetstoneQuality() const;

		float GetConsumableValue(const _Player *Player) const;
		std::string GetConsumableSuffix(bool Percent) const;
		std::string GetConsumableParticleText(float Amount) const;

		virtual std::string GetTypeAsString() const override;
		std::string ModTypeToString(int ModType) const;

		int Count{1};

		std::vector<_Item *> Mods;
		float Bonus[MOD_COUNT]{0.0f};

	private:

		void DrawAttribute(const ae::_Font *Font, bool Float, const std::string &Attribute, const std::string &Label, glm::vec2 &DrawPosition, const _Item *EquippedItem, bool Plus, bool Percent) const;

};
