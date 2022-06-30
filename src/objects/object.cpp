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
#include <ae/random.h>
#include <constants.h>
#include <stats.h>
#include <glm/geometric.hpp>
#include <glm/gtx/rotate_vector.hpp>

// Constructor
_Object::_Object(const _ObjectTemplate &ObjectTemplate) :
	Template(ObjectTemplate),
	Name(ObjectTemplate.Name),
	Type(ObjectTemplate.Type),
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
	Color(ObjectTemplate.Color),
	Rotation(0.0f),
	Scale(1.0f),
	PositionZ(OBJECT_Z)	{

}

// Set two range attributes given a level, spread and multiplier
void _Object::SetAttributeRange(const std::string &AttributeName, float Multiplier) {
	GetAttributeRange(AttributeName, Multiplier, Attributes["min_" + AttributeName].Int, Attributes["max_" + AttributeName].Int);
}

// Set an attribute given a level and multiplier
void _Object::SetAttributeLevel(const std::string &AttributeName, float Multiplier) {
	Attributes[AttributeName].Int = std::ceil(GetAttributeLevel(AttributeName, Multiplier));
}

// Set attribute range given a spread
void _Object::SetAttributeSpread(const std::string &AttributeName, float Multiplier) {
	int Value = std::ceil(Template.Attributes.at(AttributeName).Float * Multiplier);
	int ValueRange = std::ceil(Value * Template.Attributes.at(AttributeName + "_spread").Float);
	Attributes["min_" + AttributeName].Int = std::ceil(Value - ValueRange);
	Attributes["max_" + AttributeName].Int = std::ceil(Value + ValueRange);
}

// Get an attribute value given a level and multiplier
float _Object::GetAttributeLevel(const std::string &AttributeName, float Multiplier) {
	float LevelValue = Level > 0 ? Template.Attributes.at(AttributeName + "_level").Float * Level : 0;
	return (Template.Attributes.at(AttributeName).Float + LevelValue) * Multiplier;
}

// Get two range attributes given a level, spread and multiplier
void _Object::GetAttributeRange(const std::string &AttributeName, float Multiplier, int &Min, int &Max) {
	float LevelValue = Level > 0 ? Template.Attributes.at(AttributeName + "_level").Float * Level : 0;
	int Value = std::ceil((Template.Attributes.at(AttributeName).Float + LevelValue) * Multiplier);
	int ValueRange = std::ceil(Value * Template.Attributes.at(AttributeName + "_spread").Float);
	Min = Value - ValueRange;
	Max = Value + ValueRange;
}

// Set the max number of mods based on level
void _Object::SetMaxMods(bool RandomStats) {
	Attributes["max_mods"].Int = Template.Attributes.at("mods").Float + Template.Attributes.at("mods_level").Float * Level;

	if(RandomStats)
		Attributes["max_mods"].Int += ae::GetRandomInt(0, 1);
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
