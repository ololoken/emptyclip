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

#include <glm/vec2.hpp>

// A coordinate for the map
struct _Coord {
	_Coord() = default;
	_Coord(int X, int Y) : x(X), y(Y) { }
	_Coord(const glm::vec2 &Vector) : x(Vector.x), y(Vector.y) { }

	bool operator==(const _Coord &Coord) const { return x == Coord.x && y == Coord.y; }
	bool operator!=(const _Coord &Coord) const { return !(x == Coord.x && y == Coord.y); }
	void operator+=(const _Coord &Coord) { x += Coord.x; y += Coord.y; }
	void operator-=(const _Coord &Coord) { x -= Coord.x; y -= Coord.y; }
	_Coord operator+(const _Coord &Coord) const { return _Coord(x + Coord.x, y + Coord.y); }
	_Coord operator-(const _Coord &Coord) const { return _Coord(x - Coord.x, y - Coord.y); }
	_Coord operator+(const int Value) const { return _Coord(x + Value, y + Value); }
	_Coord operator-(const int Value) const { return _Coord(x - Value, y - Value); }
	bool operator>(const _Coord &Coord) const { return x > Coord.x && y > Coord.y; }
	bool operator>=(const _Coord &Coord) const { return x >= Coord.x && y >= Coord.y; }
	bool operator<(const _Coord &Coord) const { return x < Coord.x && y < Coord.y; }
	bool operator<=(const _Coord &Coord) const { return x <= Coord.x && y <= Coord.y; }

	int x;
	int y;
};
