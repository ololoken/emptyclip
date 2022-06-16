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
#include <objects/armor.h>
#include <objects/templates.h>

// Constructor
_Armor::_Armor(const std::string &Identifier, int Count, const glm::vec2 &Position, const _ArmorTemplate &Armor, const _Texture *Texture) {
	this->Type = _Object::ARMOR;
	this->ID = Identifier;
	this->Count = Count;
	this->Texture = Texture;
	this->Position = Position;

	StrengthRequirement = Armor.StrengthRequirement;
	DamageBlock = Armor.DamageBlock;
	DamageResist = Armor.DamageResist;
	MovementSpeed = Armor.MovementSpeed;
	Name = Armor.Name;
	Color = Armor.Color;
}
