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

// Constructor
_Weapon::_Weapon(const std::string &ID, int Level, const glm::vec2 &Position, const _ObjectTemplate &Template, const ae::_Texture *Texture, bool RandomStats) :
	_Item(Template) {

	this->Type = _Object::WEAPON;
	this->ID = ID;
	this->Texture = Texture;
	this->Color = Template.Color;
	this->Position = Position;
	Name = Template.Name;

	Attributes = Template.Attributes;

	int Components = Template.Attributes.at("components").Float + Template.Attributes.at("components_level").Float * Level;
	if(RandomStats) {
		Quality = ae::GetRandomInt(-ITEM_QUALITY_RANGE, ITEM_QUALITY_RANGE);
		Attributes["max_components"].Int = Components + ae::GetRandomInt(0, 1);
	}
	else
		Attributes["max_components"].Int = Components;

	Attributes["ammo"].Int = Template.Attributes.at("rounds").Int;

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
	Buffer.Write<int>(Level);
	Buffer.Write<int>(Quality);

	// Ammo
	Buffer.Write(Attributes.at("ammo").Int);

	// Max upgrades
	Buffer.Write(Attributes.at("max_components").Int);

	// Upgrades
	Buffer.Write<int>(Upgrades.size());
	for(size_t i = 0; i < Upgrades.size(); i++)
		Upgrades[i]->Serialize(Buffer);
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

	for(int i = 0; i < UPGRADE_TYPES; i++)
		Bonus[i] = 0;

	// Sum bonuses
	for(size_t i = 0; i < Upgrades.size(); i++)
		Bonus[Upgrades[i]->Attributes.at("upgrade_type").Int] += Upgrades[i]->Attributes.at("bonus").Int;

	SetAttributeRange("damage", Level, GetBonusMultiplier(UPGRADE_DAMAGE) + Quality * 0.01f);
	SetAttributeRange("accuracy", 0, 1.0f / GetBonusMultiplier(UPGRADE_ACCURACY));
	Attributes["rounds"].Int = std::ceil(Template.Attributes.at("rounds").Int * GetBonusMultiplier(UPGRADE_CLIP));
	Attributes["fire_period"].Double = Template.Attributes.at("fire_period").Double / GetBonusMultiplier(UPGRADE_FIREPERIOD);
	Attributes["reload_period"].Double = Template.Attributes.at("reload_period").Double / GetBonusMultiplier(UPGRADE_RELOADPERIOD);
	Attributes["attack_count"].Int = Template.Attributes.at("attack_count").Int + Bonus[UPGRADE_ATTACKS];

	// For melee, min accuracy is 0 and max is swing arc
	if(IsMelee())
		Attributes["min_accuracy"].Int = 0;

	SetAmmo(Attributes["ammo"].Int);
}

// Adds a component to the weapon
bool _Weapon::AddComponent(_Item *Upgrade) {
	if((int)Upgrades.size() >= Attributes.at("max_components").Int)
		return false;

	if(Upgrade->Attributes.at("weapon_type").Int != 0 && Upgrade->Attributes.at("weapon_type").Int != Attributes.at("weapon_type").Int)
		return false;

	if(Upgrade->Attributes.at("upgrade_type").Int == UPGRADE_CLIP && Stats.Objects.at(ID).Attributes.at("rounds").Int == 0)
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
