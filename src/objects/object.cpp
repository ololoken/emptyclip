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
#include <objects/entity.h>
#include <states/play.h>
#include <ae/random.h>
#include <ae/graphics.h>
#include <constants.h>
#include <stats.h>
#include <map.h>
#include <glm/geometric.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/norm.hpp>

// Constructor
_Object::_Object(const _ObjectTemplate &ObjectTemplate) :
	Template(ObjectTemplate),
	Owner(nullptr),
	Name(ObjectTemplate.Name),
	Type(ObjectTemplate.Type),
	Level(1),
	Active(true),
	PenetrationDamage(0.0f),
	MinDamage(0),
	MaxDamage(0),
	CritChance(0),
	CritDamage(0),
	Depth(0),
	Action(ACTION_IDLE),
	Map(nullptr),
	TileChanged(false),
	Position(0.0f),
	LastPosition(0.0f),
	Direction(0.0f, 1.0f),
	Velocity(0.0f),
	Radius(0.25f),
	Circle(true),
	FreePathing(false),
	Texture(nullptr),
	Mesh(nullptr),
	Color(ObjectTemplate.Color),
	Rotation(0.0f),
	Scale(1.0f),
	PositionZ(0.0f)	{

}

// Update object
void _Object::Update(double FrameTime) {
	switch(Type) {
		case PROJECTILE: {
			LastPosition = Position;
			Position += Velocity * (float)FrameTime;
			CheckProjectileCollisions();
		} break;
	}
}

// Render object
void _Object::Render(double BlendFactor) {
	if(Texture) {
		glm::vec2 DrawPosition;
		GetDrawPosition(DrawPosition, BlendFactor);

		ae::Graphics.SetColor(Color);
		if(Mesh)
			ae::Graphics.DrawMesh(glm::vec3(DrawPosition, PositionZ), Mesh, Texture, Rotation, glm::vec3(Scale));
		else
			ae::Graphics.DrawSprite(glm::vec3(DrawPosition, PositionZ), Texture, Rotation, glm::vec2(Scale));
	}
}

// Set two range attributes given a level, spread and multiplier
void _Object::SetAttributeRange(const std::string &AttributeName, float Multiplier) {
	GetAttributeRange(AttributeName, Multiplier, Attributes["min_" + AttributeName].Int, Attributes["max_" + AttributeName].Int);
}

// Set an attribute given a level and multiplier
void _Object::SetAttributeLevel(const std::string &AttributeName, float Multiplier) {
	Attributes[AttributeName].Int = GetAttributeLevel(AttributeName, Multiplier) + 0.5f;
}

// Set attribute range given a spread
void _Object::SetAttributeSpread(const std::string &AttributeName, float Multiplier) {
	float Value = Template.Attributes.at(AttributeName).Float * Multiplier;
	float ValueRange = Value * Template.Attributes.at(AttributeName + "_spread").Float;
	Attributes["min_" + AttributeName].Float = Value - ValueRange;
	Attributes["max_" + AttributeName].Float = Value + ValueRange;
}

// Get an attribute value given a level and multiplier
float _Object::GetAttributeLevel(const std::string &AttributeName, float Multiplier, int MaxLevel) {
	int CalcLevel = MaxLevel ? std::min(Level, MaxLevel) : Level;
	float LevelValue = CalcLevel > 0 ? Template.Attributes.at(AttributeName + "_level").Float * (CalcLevel - 1) : 0;
	float Value = Template.Attributes.at(AttributeName).Float + LevelValue;
	if(Value < 0)
		return Value / Multiplier;
	else
		return Value * Multiplier;
}

// Get two range attributes given a level, spread and multiplier
void _Object::GetAttributeRange(const std::string &AttributeName, float Multiplier, int &Min, int &Max) {
	float LevelValue = Level > 0 ? Template.Attributes.at(AttributeName + "_level").Float * (Level - 1) : 0;
	int Value = (Template.Attributes.at(AttributeName).Float + LevelValue) * Multiplier + 0.5f;
	int ValueRange = Value * Template.Attributes.at(AttributeName + "_spread").Float + 0.5f;
	Min = Value - ValueRange;
	Max = Value + ValueRange;
}

// Set the max number of mods based on level
void _Object::SetMaxMods(bool RandomStats) {
	Attributes["max_mods"].Int = std::max(1, (int)(Template.Attributes.at("mods").Float + Template.Attributes.at("mods_level").Float * (Level - 1) + 0.5f));

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

// Returns a t value for when a ray intersects the object
float _Object::RayIntersection(const glm::vec2 &Origin, const glm::vec2 &Direction) const {

	if(Circle) {

		// Ray circle test
		glm::vec2 Offset = Origin - glm::vec2(Position);
		float B = glm::dot(Offset, Direction);
		float C = glm::dot(Offset, Offset) - Radius * Radius;

		// Ray pointing away from circle and not inside
		if(C > 0.0f && B > 0.0f)
			return HUGE_VAL;

		// Ray missed circle
		float Discriminant = B * B - C;
		if(Discriminant < 0.0f)
			return HUGE_VAL;

		// Find intersect time
		float Time = -B - std::sqrt(Discriminant);
		if(Time < 0.0f)
		   Time = 0.0f;

		return Time;

	}
	else {

		// Ray AABB test
		float TimeMin = 0.0f;
		float TimeMax = HUGE_VAL;
		for(int i = 0; i < 2; i++) {
			float AABBMin = Position[i] - Radius;
			float AABBMax = Position[i] + Radius;
			if(std::abs(Direction[i]) == 0.0f) {
				if(Origin[i] < AABBMin || Origin[i] > AABBMax)
					return HUGE_VAL;
			}
			else {

				float OneOverDirection =  1.0f / Direction[i];
				float HitTimeMin = (AABBMin - Origin[i]) * OneOverDirection;
				float HitTimeMax = (AABBMax - Origin[i]) * OneOverDirection;

				if(HitTimeMin > HitTimeMax)
					std::swap(HitTimeMin, HitTimeMax);

				TimeMin = std::max(TimeMin, HitTimeMin);
				TimeMax = std::min(TimeMax, HitTimeMax);

				if(TimeMin > TimeMax)
					return HUGE_VAL;
			}
		}

		return TimeMin;
	}

	return HUGE_VAL;
}

// Determine if a circle is touching the object
bool _Object::IsTouchingCircle(const glm::vec2 &CircleCenter, float CircleRadius, float &DistanceSquared) const {

	// Test against circle object
	if(Circle) {
		DistanceSquared = glm::distance2(CircleCenter, Position);
		float RadiiSum = CircleRadius + Radius;

		return DistanceSquared < RadiiSum * RadiiSum;
	}
	// Test against AABB object
	else {
		glm::vec2 ClosetPoint = CircleCenter;

		// Get AABB of object
		float AABB[4] = {
			Position.x - Radius,
			Position.y - Radius,
			Position.x + Radius,
			Position.y + Radius
		};

		// Get closest point on AABB
		if(ClosetPoint.x < AABB[0])
			ClosetPoint.x = AABB[0];
		if(ClosetPoint.y < AABB[1])
			ClosetPoint.y = AABB[1];
		if(ClosetPoint.x > AABB[2])
			ClosetPoint.x = AABB[2];
		if(ClosetPoint.y > AABB[3])
			ClosetPoint.y = AABB[3];

		// Test circle collision with point
		float DistanceSquared = glm::distance2(ClosetPoint, CircleCenter);
		return DistanceSquared < CircleRadius * CircleRadius;
	}
}

// Check collisions between projectiles and objects
void _Object::CheckProjectileCollisions() {

	// Check wall hits
	glm::vec2 HitPosition;
	if(Map->ResolveTileCollisions(Position, Radius, _Tile::BULLET, HitPosition)) {
	/*
		_Hit &Hit = Map->CollisionHits.front();
		_Entity *OwnerEntity = (_Entity *)Owner;
		_Hit WallHit;
		WallHit.Position = Hit.ClosetPoint;
		WallHit.Normal = glm::normalize(HitPosition - Position);
		PlayState.GenerateHitEffects(OwnerEntity, HIT_WALL, WallHit, true);
	*/
		Active = false;
		return;
	}

	// Check object hits
	std::vector<_Hit> &Hits = Map->CheckCollisionsInGrid(Position, Radius, GRID_MONSTER);
	for(const auto &Hit : Hits) {
		_Entity *HitEntity = (_Entity *)Hit.Object;
		if(HitEntity->Type == PROP) {
			Active = false;
			break;
		}

		if(HitObjects.find(HitEntity) != HitObjects.end())
			continue;

		HitObjects[HitEntity] = 1;

		// Get damage
		int Damage = ae::GetRandomInt(MinDamage, MaxDamage);
		bool Crit = false;
		if(ae::GetRandomInt(1, 100) <= CritChance) {
			Damage *= CritDamage * 0.01f;
			Crit = true;
		}

		// Apply damage
		Damage = HitEntity->ReduceDamage(Damage);
		HitEntity->UpdateHealth(-Damage);

		// Callbacks
		_Entity *OwnerEntity = (_Entity *)Owner;
		OwnerEntity->OnAttack(HitEntity, Hit);
		HitEntity->OnHit(OwnerEntity, Hit);

		// Particles
		PlayState.GenerateHitEffects(OwnerEntity, HIT_OBJECT, Hit);
		PlayState.GenerateDamageText(Hit.Position, Damage, Crit, Type == PLAYER);

		// Apply depth
		MinDamage *= PenetrationDamage;
		MaxDamage *= PenetrationDamage;
		Depth--;
		if(Depth <= 0) {
			Active = false;
			break;
		}
	}
}
