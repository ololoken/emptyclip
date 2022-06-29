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
#include <objects/weapon.h>
#include <objects/particle.h>
#include <ae/random.h>
#include <ae/buffer.h>
#include <constants.h>
#include <stats.h>
#include <algorithm>

// Destructor
_Weapon::~_Weapon() {
	for(size_t i = 0; i < Mods.size(); i++)
		delete Mods[i];
}

// Serialize weapon for saving
void _Weapon::Serialize(ae::_Buffer &Buffer) {
	_Item::Serialize(Buffer);

	// Ammo
	Buffer.Write(Attributes.at("ammo").Int);

	// Max mods
	Buffer.Write(Attributes.at("max_mods").Int);

	// Mods
	Buffer.Write<int>(Mods.size());
	for(size_t i = 0; i < Mods.size(); i++)
		Mods[i]->Serialize(Buffer);
}

// Get weapon sound
const std::string &_Weapon::GetSound(int SoundType) const {
	return Stats.Objects.at(ID).SoundID[SoundType];
}

// Set ammo amount
void _Weapon::SetAmmo(int Value) {
	Attributes["ammo"].Int = std::clamp(Value, 0, Attributes["rounds"].Int);
}

// Recalculates the weapon stats
void _Weapon::RecalculateStats() {
	for(int i = 0; i < MOD_TYPES; i++)
		Bonus[i] = 0;

	// Sum bonuses
	for(size_t i = 0; i < Mods.size(); i++)
		Bonus[Mods[i]->Attributes.at("mod_type").Int] += Mods[i]->Attributes.at("bonus").Int;

	SetAttributeRange("damage", Level, GetBonusMultiplier(MOD_DAMAGE) + Quality * 0.01f);
	SetAttributeRange("accuracy", 0, 1.0f / GetBonusMultiplier(MOD_ACCURACY));
	Attributes["rounds"].Int = std::ceil(Template.Attributes.at("rounds").Int * GetBonusMultiplier(MOD_CLIP));
	Attributes["fire_period"].Double = Template.Attributes.at("fire_period").Double / GetBonusMultiplier(MOD_FIREPERIOD);
	Attributes["reload_period"].Double = Template.Attributes.at("reload_period").Double / GetBonusMultiplier(MOD_RELOADPERIOD);
	Attributes["attack_count"].Int = Template.Attributes.at("attack_count").Int + Bonus[MOD_ATTACKCOUNT];
	Attributes["reload_rounds"].Int = Template.Attributes.at("reload_rounds").Int;
	Attributes["penetration"].Int = Template.Attributes.at("penetration").Int;

	// For melee, min accuracy is 0 and max is swing arc
	if(IsMelee())
		Attributes["min_accuracy"].Int = 0;

	SetAmmo(Attributes["ammo"].Int);
}

// Adds a mod to the weapon
bool _Weapon::AddMod(_Item *Mod) {
	if((int)Mods.size() >= Attributes.at("max_mods").Int)
		return false;

	if(Mod->Attributes.at("weapon_type").Int != 0 && Mod->Attributes.at("weapon_type").Int != Attributes.at("weapon_type").Int)
		return false;

	if(Mod->Attributes.at("mod_type").Int == MOD_CLIP && Stats.Objects.at(ID).Attributes.at("rounds").Int == 0)
		return false;

	Mods.push_back(Mod);
	RecalculateStats();

	return true;
}

// Converts the weapon's type into a string
std::string _Weapon::ToString(int Type) {

	switch(Type) {
		case WEAPON_MELEE:
			return "Melee";
		break;
		case WEAPON_PISTOL:
			return "Pistol";
		break;
		case WEAPON_SHOTGUN:
			return "Shotgun";
		break;
		case WEAPON_RIFLE:
			return "Rifle";
		break;
		case WEAPON_HEAVY:
			return "Heavy";
		break;
		default:
		break;
	}

	return "";
}
