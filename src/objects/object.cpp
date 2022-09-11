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
#include <ae/audio.h>
#include <constants.h>
#include <stats.h>
#include <menu.h>
#include <map.h>
#include <glm/geometric.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/norm.hpp>

// Constructor
_Object::_Object(const _ObjectTemplate &ObjectTemplate) :
	Template(ObjectTemplate),
	Name(ObjectTemplate.Name),
	Type(ObjectTemplate.Type),
	Color(ObjectTemplate.Color)	{

}

// Update object
void _Object::Update(double FrameTime) {
	switch(Type) {
		case PROJECTILE: {
			LastPosition = Position;
			Position += Velocity * (float)FrameTime;
			if(Template.ParticleTemplate)
				PlayState.GenerateProjectileEffects(Template.ParticleTemplate, Position);
			CheckProjectileCollisions();
		} break;
	}
}

// Render object
void _Object::Render(double BlendFactor) {
	if(!Texture)
		return;

	glm::vec2 DrawPosition;
	GetDrawPosition(DrawPosition, BlendFactor);

	ae::Graphics.SetColor(Color);
	if(Mesh)
		ae::Graphics.DrawMesh(glm::vec3(DrawPosition, PositionZ), Mesh, Texture, Rotation, glm::vec3(Scale));
	else
		ae::Graphics.DrawSprite(glm::vec3(DrawPosition, PositionZ), Texture, Rotation, glm::vec2(Scale));
}

// Render lights
void _Object::RenderLights(double BlendFactor) {
	if(!LightTexture)
		return;

	ae::Graphics.SetColor(LightColor);
	ae::Graphics.DrawSprite(glm::vec3(Position, OBJECT_Z), LightTexture, 0, LightScale);
}

// Get sound for a sound type
const ae::_Sound *_Object::GetSound(int SoundType) const {
	const auto &SoundIDs = Template.SoundID[SoundType];
	if(SoundIDs.empty())
		return nullptr;

	return SoundIDs[ae::GetRandomInt((size_t)0, SoundIDs.size()-1)];
}

// Set two range attributes given a level, spread and multiplier
void _Object::SetAttributeRange(const std::string &AttributeName, float Multiplier) {
	GetAttributeRange(AttributeName, Multiplier, Attributes["min_" + AttributeName].Int, Attributes["max_" + AttributeName].Int);
}

// Set an attribute given a level and multiplier
void _Object::SetAttributeLevel(const std::string &AttributeName, float Multiplier) {
	Attributes[AttributeName].Int = std::round(GetAttributeLevel(AttributeName, Multiplier));
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
	int Value = std::round((Template.Attributes.at(AttributeName).Float + LevelValue) * Multiplier);
	int ValueRange = std::round(Value * Template.Attributes.at(AttributeName + "_spread").Float);
	Min = Value - ValueRange;
	Max = Value + ValueRange;
}

// Set the max number of mods based on level and quality
void _Object::SetMaxMods(float QualityFactor, bool RandomStats) {
	Attributes["max_mods"].Int = std::max(1, (int)std::round(QualityFactor * (Template.Attributes.at("mods").Float + Template.Attributes.at("mods_level").Float * (Level - 1))));
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

// Get bounds of object's light
void _Object::GetLightBounds(glm::vec4 &Bounds) {
	Bounds[0] = Position.x - LightScale[0] * 0.5f;
	Bounds[1] = Position.y - LightScale[1] * 0.5f;
	Bounds[2] = Position.x + LightScale[0] * 0.5f;
	Bounds[3] = Position.y + LightScale[1] * 0.5f;
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
	_Entity *OwnerEntity = (_Entity *)Owner;

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
	}
	else {

		// Check object hits
		std::vector<_Hit> &Hits = Map->CheckCollisionsInGrid(Position, Radius, GridCheckType);
		for(const auto &Hit : Hits) {
			if(Hit.Object->Type == PROP) {
				Active = false;
				break;
			}

			if(HitObjects.find(Hit.Object) != HitObjects.end())
				continue;

			HitObjects[Hit.Object] = 1;

			// Proceed to explosion
			if(ProjectileExplosionSize > 0.0f) {
				HitPosition = Hit.Object->Position;
				Active = false;
				break;
			}

			// Apply damage
			ApplyDamage(Hit);

			// Apply depth
			ProjectileMinDamage *= ProjectilePenetrationDamage;
			ProjectileMaxDamage *= ProjectilePenetrationDamage;
			Depth--;
			if(Depth <= 0) {
				Active = false;
				break;
			}
		}

		// Check achievement
		if(HitObjects.size() >= ACHIEVEMENTS_LINE_EM_UP_HITS && Template.ID == "bolt")
			Menu.UnlockAchievement("pierce30");
	}

	// Hit
	if(!Active) {
		ae::Audio.PlaySound(GetSound(SOUND_EXPLODE), ae::_SoundSettings(glm::vec3(Position.x, 0.0f, Position.y), 1.0f, AUDIO_REFERENCE_DISTANCE, AUDIO_MAX_DISTANCE, AUDIO_ROLL_OFF));

		// Explode
		if(ProjectileExplosionSize == 0.0f)
			return;

		// Generate particle
		PlayState.GenerateExplosion(OwnerEntity->GetParticle(PARTICLE_EXPLOSION), HitPosition, glm::vec2(ProjectileExplosionSize));

		// Check hits
		float ExplosionRadius = ProjectileExplosionSize * 0.5f;
		std::vector<_Hit> &Hits = Map->CheckCollisionsInGrid(Position, ExplosionRadius, GridCheckType);

		// Check self hit
		if(OwnerEntity->Type == _Object::PLAYER && !OwnerEntity->IsInvulnerable() && !PlayState.GodMode) {
			float DistanceSquared;
			if(OwnerEntity->IsTouchingCircle(Position, ExplosionRadius, DistanceSquared)) {
				_Hit Hit;
				Hit.Object = OwnerEntity;
				Hit.Position = OwnerEntity->Position;
				Hits.push_back(Hit);
			}
		}

		// Apply damage
		for(const auto &Hit : Hits) {
			if(Hit.Object->Type == PROP)
				continue;

			// Check for walls
			if(!Map->IsVisible(Position, Hit.Object->Position, _Tile::BULLET))
				continue;

			// Apply damage
			ApplyDamage(Hit);
		}
	}
}

// Apply damage to entity
void _Object::ApplyDamage(const _Hit &Hit) {
	_Entity *HitEntity = (_Entity *)Hit.Object;
	_Entity *OwnerEntity = (_Entity *)Owner;

	// Get damage
	int Damage = ae::GetRandomInt(ProjectileMinDamage, ProjectileMaxDamage);
	bool Crit = false;
	if(ae::GetRandomInt(1, 100) <= ProjectileCritChance) {
		Damage *= ProjectileCritDamage * 0.01f;
		Crit = true;
	}

	// Apply damage
	Damage = HitEntity->ReduceDamage(Damage);
	HitEntity->UpdateHealth(-Damage);

	// Callbacks
	//OwnerEntity->OnAttack(HitEntity, Hit);
	HitEntity->OnHit(OwnerEntity, Hit);

	// Particles
	PlayState.GenerateHitEffects(OwnerEntity, HIT_OBJECT, Hit);
	PlayState.GenerateDamageText(Hit.Position, Damage, Crit, HitEntity->Type == PLAYER);
}
