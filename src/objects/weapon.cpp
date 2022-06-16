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
#include <objects/upgrade.h>
#include <objects/particle.h>
#include <stats.h>
#include <random.h>
#include <buffer.h>
#include <algorithm>

// Constructor
_Weapon::_Weapon(const std::string &Identifier, int Count, const glm::vec2 &Position, const _WeaponTemplate &Weapon, const _Texture *Texture, bool Generate) :
	WeaponType(Weapon.Type) {

	this->Type = _Object::WEAPON;
	this->Identifier = Identifier;
	this->Texture = Texture;
	this->Color = Weapon.Color;
	this->Position = Position;
	Name = Weapon.Name;

	for(const auto &Attribute : Weapon.Attributes)
		Attributes[Attribute.first] = Attribute.second;

	if(Generate)
		Attributes["max_components"].Int = GetRandomInt(Weapon.Attributes.at("min_components").Int, Weapon.Attributes.at("max_components").Int);
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
void _Weapon::Serialize(_Buffer &Buffer) {

	// Write weapons
	Buffer.WriteString(Identifier.c_str());

	// Ammo
	Buffer.Write(Attributes.at("ammo").Int);

	// Max upgrades
	Buffer.Write(Attributes.at("max_components").Int);

	// Upgrades
	Buffer.Write<int>(Upgrades.size());
	for(size_t i = 0; i < Upgrades.size(); i++)
		Upgrades[i]->Serialize(Buffer);
}

const std::string &_Weapon::GetSample(int SampleType) const {
	return Stats.Weapons[Identifier].Samples[SampleType];
}

// Set ammo amount
void _Weapon::SetAmmo(int Value) {
	Attributes["ammo"].Int = std::clamp(Value, 0, Attributes["rounds"].Int);
}

// Recalculates the weapon stats
void _Weapon::RecalculateStats() {

	for(int i = 0; i < UPGRADE_TYPES; i++)
		Bonus[i] = 0.0f;

	// Sum bonuses
	for(size_t i = 0; i < Upgrades.size(); i++)
		Bonus[Upgrades[i]->GetUpgradeType()] += Upgrades[i]->GetBonus();

	// Set stats
	Attributes["rounds"].Int = (int)(Stats.Weapons[Identifier].Attributes.at("rounds").Int * (1.0f + Bonus[UPGRADE_CLIP]));
	Attributes["min_damage"].Int = (int)(Stats.Weapons[Identifier].Attributes.at("min_damage").Int * (1.0f + Bonus[UPGRADE_DAMAGE]));
	Attributes["max_damage"].Int = (int)(Stats.Weapons[Identifier].Attributes.at("max_damage").Int * (1.0f + Bonus[UPGRADE_DAMAGE]));
	Attributes["min_accuracy"].Float = Stats.Weapons[Identifier].Attributes.at("min_accuracy").Float / (1.0f + Bonus[UPGRADE_ACCURACY]);
	Attributes["max_accuracy"].Float = Stats.Weapons[Identifier].Attributes.at("max_accuracy").Float / (1.0f + Bonus[UPGRADE_ACCURACY]);
	Attributes["fire_period"].Double = Stats.Weapons[Identifier].Attributes.at("fire_period").Double / (1.0f + Bonus[UPGRADE_FIREPERIOD]);
	Attributes["reload_period"].Double = Stats.Weapons[Identifier].Attributes.at("reload_period").Double / (1.0f + Bonus[UPGRADE_RELOADPERIOD]);
	Attributes["attack_count"].Int = Stats.Weapons[Identifier].Attributes.at("attack_count").Int + (int)(Bonus[UPGRADE_ATTACKS]);

	SetAmmo(Attributes["ammo"].Int);
}

// Adds a component to the weapon
bool _Weapon::AddComponent(_Upgrade *Upgrade) {
	if((int)Upgrades.size() >= Attributes.at("max_components").Int)
		return false;

	if(Upgrade->GetWeaponType() != -1 && Upgrade->GetWeaponType() != WeaponType)
		return false;

	if(Upgrade->GetUpgradeType() == UPGRADE_CLIP && Stats.Weapons[Identifier].Attributes.at("rounds").Int == 0)
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
