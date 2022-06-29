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
#include <objects/object.h>
#include <constants.h>
#include <stats.h>
#include <glm/geometric.hpp>
#include <glm/gtx/rotate_vector.hpp>

// Constructor
_Object::_Object(const _ObjectTemplate &Template) :
	Template(Template),
	Name(Template.Name),
	Type(Template.Type),
	Level(1),
	Active(true),
	Map(nullptr),
	TileChanged(false),
	Position(0, 0),
	LastPosition(0, 0),
	Direction(0.0, 1.0f),
	Radius(0.25f),
	WallState(0),
	Texture(nullptr),
	Color(Template.Color),
	Rotation(0.0f),
	Scale(1.0f),
	PositionZ(OBJECT_Z)	{

}

// Set two range attributes given a level, spread and multiplier
void _Object::SetAttributeRange(const std::string &AttributeName, int ItemLevel, float Multiplier) {
	float LevelValue = ItemLevel > 0 ? Template.Attributes.at(AttributeName + "_level").Float * ItemLevel : 0;
	int Value = std::ceil((Template.Attributes.at(AttributeName).Float + LevelValue) * Multiplier);
	int ValueRange = std::ceil(Value * Template.Attributes.at(AttributeName + "_spread").Float);
	Attributes["min_" + AttributeName].Int = Value - ValueRange;
	Attributes["max_" + AttributeName].Int = Value + ValueRange;
}

// Set an attribute given a level and multiplier
void _Object::SetAttributeLevel(const std::string &AttributeName, int ItemLevel, float Multiplier) {
	float LevelValue = ItemLevel > 0 ? Template.Attributes.at(AttributeName + "_level").Float * ItemLevel : 0;
	Attributes[AttributeName].Int = std::ceil((Template.Attributes.at(AttributeName).Float + LevelValue) * Multiplier);
}

// Set the max number of components based on level
void _Object::SetMaxComponents() {
	Attributes["max_components"].Int = Template.Attributes.at("components").Float + Template.Attributes.at("components_level").Float * Level;
}

// Get render bounds of object
void _Object::GetRenderBounds(glm::vec4 &Bounds) {
	Bounds[0] = Position.x - Scale * 0.5f;
	Bounds[1] = Position.y - Scale * 0.5f;
	Bounds[2] = Position.x + Scale * 0.5f;
	Bounds[3] = Position.y + Scale * 0.5f;
}

// Calculates the angle from a slope
void _Object::FacePosition(const glm::vec2 &Target) {
	Direction = Target - Position;
	if(Direction.x == 0 && Direction.y == 0.0f)
		Direction.y = 1.0f;

	Direction = glm::normalize(Direction);

	Rotation = glm::degrees(atan2(Direction.y, Direction.x)) + 90.0f;
	if(Rotation < 0.0f)
		Rotation += 360.0f;
}

// Force position of object
void _Object::SetPosition(const glm::vec2 &NewPosition) {
	LastPosition = Position = NewPosition;
}

// Get direction of object as a unit vector
glm::vec2 _Object::GetDirectionVector(float RotationOffset) const {
	return glm::rotate(glm::vec2(0, -1), glm::radians(Rotation + RotationOffset));
}
