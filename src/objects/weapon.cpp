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
#include <stats.h>
#include <algorithm>

// Constructor
_Weapon::_Weapon(const std::string &Identifier, int Count, const glm::vec2 &Position, const _WeaponTemplate &Weapon, const ae::_Texture *Texture, bool Generate) {

	this->Type = _Object::WEAPON;
	this->ID = Identifier;
	this->Texture = Texture;
	this->Color = Weapon.Color;
	this->Position = Position;
	Name = Weapon.Name;

	Attributes = Weapon.Attributes;

	if(Generate)
		Attributes["max_components"].Int = ae::GetRandomInt(Weapon.Attributes.at("min_components").Int, Weapon.Attributes.at("max_components").Int);
	else
		Attributes["max_components"].Int = Weapon.Attributes.at("min_components").Int;

	Attributes["ammo"].Int = Weapon.Attributes.at("rounds").Int;

	RecalculateStats();
}

// Destructor
_Weapon::~_Weapon() {

	for(size_t i = 0; i < Upgrades.size(); i++)
		delete Upgrades[i];
}

// Serialize weapon for saving
void _Weapon::Serialize(ae::_Buffer &Buffer) {

	// Write weapons
	Buffer.WriteString(ID.c_str());

	// Ammo
	Buffer.Write(Attributes.at("ammo").Int);

	// Max upgrades
	Buffer.Write(Attributes.at("max_components").Int);

	// Upgrades
	Buffer.Write<int>(Upgrades.size());
	for(size_t i = 0; i < Upgrades.size(); i++)
		Upgrades[i]->Serialize(Buffer);
}

// Get weapon sound sample
const std::string &_Weapon::GetSample(int SampleType) const {
	return Stats.Weapons[ID].Samples[SampleType];
}

// Set ammo amount
void _Weapon::SetAmmo(int Value) {
	Attributes["ammo"].Int = std::clamp(Value, 0, Attributes["rounds"].Int);
}

// Recalculates the weapon stats
void _Weapon::RecalculateStats() {

	for(int i = 0; i < UPGRADE_TYPES; i++)
		Bonus[i] = 0;

	// Sum bonuses
	for(size_t i = 0; i < Upgrades.size(); i++)
		Bonus[Upgrades[i]->Attributes.at("upgrade_type").Int] += Upgrades[i]->Attributes.at("bonus").Int;

	// Set stats
	Attributes["rounds"].Int = std::ceil(Stats.Weapons[ID].Attributes.at("rounds").Int * ((100 + Bonus[UPGRADE_CLIP]) * 0.01f));
	Attributes["min_damage"].Int = std::ceil(Stats.Weapons[ID].Attributes.at("min_damage").Int * ((100 + Bonus[UPGRADE_DAMAGE]) * 0.01f));
	Attributes["max_damage"].Int = std::ceil(Stats.Weapons[ID].Attributes.at("max_damage").Int * ((100 + Bonus[UPGRADE_DAMAGE]) * 0.01f));
	Attributes["min_accuracy"].Float = Stats.Weapons[ID].Attributes.at("min_accuracy").Float / ((100 + Bonus[UPGRADE_ACCURACY]) * 0.01f);
	Attributes["max_accuracy"].Float = Stats.Weapons[ID].Attributes.at("max_accuracy").Float / ((100 + Bonus[UPGRADE_ACCURACY]) * 0.01f);
	Attributes["fire_period"].Double = Stats.Weapons[ID].Attributes.at("fire_period").Double / ((100 + Bonus[UPGRADE_FIREPERIOD]) * 0.01f);
	Attributes["reload_period"].Double = Stats.Weapons[ID].Attributes.at("reload_period").Double / ((100 + Bonus[UPGRADE_RELOADPERIOD]) * 0.01f);
	Attributes["attack_count"].Int = Stats.Weapons[ID].Attributes.at("attack_count").Int + Bonus[UPGRADE_ATTACKS];

	SetAmmo(Attributes["ammo"].Int);
}

// Adds a component to the weapon
bool _Weapon::AddComponent(_Item *Upgrade) {
	if((int)Upgrades.size() >= Attributes.at("max_components").Int)
		return false;

	if(Upgrade->Attributes.at("weapon_type").Int != -1 && Upgrade->Attributes.at("weapon_type").Int != Attributes.at("weapon_type").Int)
		return false;

	if(Upgrade->Attributes.at("upgrade_type").Int == UPGRADE_CLIP && Stats.Weapons[ID].Attributes.at("rounds").Int == 0)
		return false;

	Upgrades.push_back(Upgrade);
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
