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
#include <ae/graphics.h>
#include <ae/font.h>
#include <ae/camera.h>
#include <ae/random.h>
#include <ae/animation.h>
#include <particles.h>
#include <glm/gtx/rotate_vector.hpp>

// Constructor
_Particle::_Particle(const _ParticleSpawn &Spawn) :
	Type(Spawn.Template->Type),
	Lifetime(Spawn.Template->Lifetime),
	Texture(Spawn.Template->Texture),
	Font(Spawn.Template->Font),
	Text(Spawn.Text),
	Color(Spawn.Template->Color),
	AlphaSpeed(Spawn.Template->AlphaSpeed),
	PositionZ(Spawn.PositionZ),
	ScaleAspect(Spawn.Template->ScaleAspect) {

	// Randomize
	Position = Spawn.Position;
	Rotation = Spawn.RotationAdjust + (float)(ae::GetRandomReal(Spawn.Template->StartDirection.x, Spawn.Template->StartDirection.y));
	Velocity = glm::rotate(glm::vec2(0, -1), glm::radians(this->Rotation)) * (float)ae::GetRandomReal(Spawn.Template->VelocityScale.x, Spawn.Template->VelocityScale.y);
	Acceleration = Velocity * Spawn.Template->AccelerationScale;
	TurnSpeed = ae::GetRandomReal(Spawn.Template->TurnSpeed.x, Spawn.Template->TurnSpeed.y);
	float Size = ae::GetRandomReal(Spawn.Template->Size.x, Spawn.Template->Size.y);
	if(ScaleAspect >= 1.0f) {
		Scale.x = Size;
		Scale.y = Size / ScaleAspect;
	}
	else {
		Scale.x = Size * ScaleAspect;
		Scale.y = Size;
	}

	PositionZ += ae::GetRandomReal(-Spawn.Template->DeviationZ, Spawn.Template->DeviationZ);

	if(Type == _Particles::WALL_DECALS) {
		Position += Spawn.Normal * 0.01f;
		if(Spawn.Normal.y < 0.0f)
			Side = 3;
		else if(Spawn.Normal.y > 0.0f)
			Side = 1;
		else if(Spawn.Normal.x < 0.0f)
			Side = 2;
		else if(Spawn.Normal.x > 0.0f)
			Side = 4;
	}
	LastPosition = Position;

	// Create animation
	if(Spawn.Template->Reel) {
		Animation = new ae::_Animation(nullptr);
		Animation->Reels.push_back(Spawn.Template->Reel);
		Animation->CalculateTextureCoords();
		Animation->Play(0);
	}
}

// Destructor
_Particle::~_Particle() {
	delete Animation;
}

// Update
void _Particle::Update(double FrameTime) {
	if(Animation) {
		Animation->Update(FrameTime);
		if(Animation->IsStopped())
			Deleted = true;
	}

	LastPosition = Position;
	Position += Velocity * (float)FrameTime;
	Velocity += Acceleration * (float)FrameTime;
	Rotation += TurnSpeed * FrameTime;
	Color.a += AlphaSpeed * FrameTime;
	Lifetime -= FrameTime;

	if(Color.a < 0.0f)
		Color.a = 0.0f;

	if(!Animation && Lifetime < 0)
		Deleted = true;
}

// Render
void _Particle::Render(const ae::_Camera *Camera, double BlendFactor) {
	glm::vec2 DrawPosition = Position * (float)BlendFactor + LastPosition * (float)(1.0 - BlendFactor);

	if(Animation) {
		ae::Graphics.SetColor(Color);
		ae::Graphics.DrawAnimationFrame(
			glm::vec3(DrawPosition, PositionZ),
			Animation->Reels[Animation->Reel]->Texture,
			glm::vec4(Animation->TextureCoords),
			Rotation,
			Scale
		);
	}

	if(Texture) {
		ae::Graphics.SetColor(Color);
		if(Type == _Particles::WALL_DECALS)
			ae::Graphics.DrawWallDecal(glm::vec3(Position, PositionZ), Texture, Side, Scale);
		else
			ae::Graphics.DrawSprite(glm::vec3(DrawPosition, PositionZ), Texture, Rotation, Scale);
	}

	if(Font && Text != "") {
		glm::vec2 ScreenPosition;
		Camera->ConvertWorldToScreen(Position, ScreenPosition);
		Font->DrawText(Text.c_str(), ScreenPosition, ae::CENTER_BASELINE, Color);
	}
}
