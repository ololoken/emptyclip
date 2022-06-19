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
#include <particles.h>
#include <ae/camera.h>
#include <map.h>
#include <objects/templates.h>
#include <objects/particle.h>

// Constructor
_Particles::_Particles() :
	Camera(nullptr),
	Map(nullptr) {

	for(int i = 0; i < COUNT; i++)
		RenderList[i].reserve(5000);
}

// Destructor
_Particles::~_Particles() {
	Clear();
}

// Update all particles
void _Particles::Update(double FrameTime) {

	// Clear render list
	for(int i = 0; i < COUNT; i++)
		RenderList[i].clear();

	// Update all particles
	for(auto Iterator = Particles.begin(); Iterator != Particles.end(); ) {
		_Particle *Particle = *Iterator;

		// Update
		Particle->Update(FrameTime);

		// Delete
		if(Particle->Deleted) {
			delete Particle;
			Iterator = Particles.erase(Iterator);
		}
		else {

			if(Camera->IsCircleInView(Particle->Position, std::max(Particle->Scale.x, Particle->Scale.y)))
				RenderList[Particle->Type].push_back(Particle);

			++Iterator;
		}
	}
}

// Render
void _Particles::Render(int Type) {
	for(auto Iterator : RenderList[Type])
		Iterator->Render();
}

// Deletes all
void _Particles::Clear() {
	for(auto Iterator : Particles)
		delete Iterator;

	Particles.clear();
}

// Add an existing particle
void _Particles::Add(_Particle *Particle) {
	if(!Particle)
		return;

	Particles.push_back(Particle);
}

// Spawn a particle
void _Particles::Create(const _ParticleSpawn &Spawn) {
	if(!Spawn.Template)
		return;

	// Add floor and wall decals to map grid
	if(Spawn.Template->Type == FLOOR_DECALS || Spawn.Template->Type == WALL_DECALS) {
		if(!Map)
			return;

		// Create particles and add to grid
		for(int i = 0; i < Spawn.Template->Count; i++) {
			_Particle *Particle = new _Particle(Spawn);
			Map->AddParticle(Particle);
		}

		return;
	}

	// Add general particles
	for(int i = 0; i < Spawn.Template->Count; i++) {
		_Particle *Particle = new _Particle(Spawn);
		Particles.push_back(Particle);
	}
}
