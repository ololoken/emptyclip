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
#include <objects/ammo.h>
#include <objects/templates.h>
#include <stats.h>

// Constructor
_Ammo::_Ammo(const std::string &Identifier, int Count, const glm::vec2 &Position, const _AmmoTemplate &Ammo, const _Texture *Texture) {
	this->AmmoType = Ammo.AmmoType;
	this->Type = _Object::AMMO;
	this->ID = Identifier;
	this->Count = Count;
	this->Name = Ammo.Name;
	this->Texture = Texture;
	this->Color = Ammo.Color;
	this->Position = Position;
}
