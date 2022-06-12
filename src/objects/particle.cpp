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
#include <objects/particle.h>
#include <objects/templates.h>
#include <particles.h>
#include <graphics.h>
#include <font.h>
#include <random.h>
#include <glm/gtx/rotate_vector.hpp>

// Constructor
_Particle::_Particle(const _ParticleSpawn &Spawn) :
	Type(Spawn.Template->Type),
	Lifetime(Spawn.Template->Lifetime),
	Deleted(false),
	Texture(Spawn.Template->Texture),
	Font(Spawn.Template->Font),
	Text(Spawn.Text),
	Color(Spawn.Template->Color),
	AlphaSpeed(Spawn.Template->AlphaSpeed),
	PositionZ(Spawn.PositionZ),
	ScaleAspect(Spawn.Template->ScaleAspect) {

	// Random
	Rotation = Spawn.RotationAdjust + (float)(Random.GenerateRange(Spawn.Template->StartDirection.x, Spawn.Template->StartDirection.y));
	Velocity = glm::rotate(glm::vec2(0, -1), glm::radians(this->Rotation)) * (float)Random.GenerateRange(Spawn.Template->VelocityScale.x, Spawn.Template->VelocityScale.y);
	Acceleration = Velocity * Spawn.Template->AccelerationScale;
	TurnSpeed = Random.GenerateRange(Spawn.Template->TurnSpeed.x, Spawn.Template->TurnSpeed.y);
	float Size = Random.GenerateRange(Spawn.Template->Size.x, Spawn.Template->Size.y);
	if(ScaleAspect >= 1.0f) {
		Scale.x = Size;
		Scale.y = Size / ScaleAspect;
	}
	else {
		Scale.x = Size * ScaleAspect;
		Scale.y = Size;
	}

	Position = Spawn.Position;
}

// Destructor
_Particle::~_Particle() {
}

// Update
void _Particle::Update(double FrameTime) {
	Position += Velocity;
	Velocity += Acceleration;
	Rotation += TurnSpeed;
	Color.a += AlphaSpeed;
	Lifetime -= FrameTime;

	if(Color.a < 0.0f)
		Color.a = 0.0f;

	if(Lifetime < 0)
		Deleted = true;
}

// Render
void _Particle::Render() {

	if(Texture)
		Graphics.DrawTexture(glm::vec3(Position, PositionZ), Texture, Color, Rotation, Scale);

	if(Font && Text != "")
		Font->DrawText(Text.c_str(), Position.x, Position.y, Color, CENTER_BASELINE, 1/64.0f);
}
