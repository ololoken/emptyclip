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
#include <objects/item.h>
#include <ae/buffer.h>
#include <ae/texture.h>
#include <ae/graphics.h>
#include <constants.h>

// Constructor
_Item::_Item() :
	Level(0),
	Quality(0),
	Count(0) {

	Texture = nullptr;
	PositionZ = ITEM_Z;
}

// Serialize for saving
void _Item::Serialize(ae::_Buffer &Buffer) {
	Buffer.WriteString(ID.c_str());
}

// Draws the object
void _Item::Render(double BlendFactor) {
	ae::Graphics.SetColor(Color);
	ae::Graphics.DrawSprite(glm::vec3(Position, PositionZ), Texture, Rotation, glm::vec2(ITEM_SCALE));
}

float _Item::GetAverageDamage() const {
	return (Attributes.at("min_damage").Int + Attributes.at("max_damage").Int) * 0.5f;
}

float _Item::GetAverageAccuracy() const {
	return (Attributes.at("min_accuracy").Float + Attributes.at("max_accuracy").Float) * 0.5f;
}

// Get type as string
std::string _Item::GetTypeAsString() const {

	switch(Type) {
		case _Object::KEY:
			return "Key";
		case _Object::AMMO:
			return "Ammo";
		case _Object::UPGRADE:
			return "Upgrade Component";
		case _Object::ARMOR:
			return "Armor";
		case _Object::MEDKIT:
			return "Medkit";
	}

	return "";
}
