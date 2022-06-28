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
#include <objectmanager.h>
#include <objects/object.h>
#include <ae/camera.h>
#include <map.h>
#include <constants.h>

// Constructor
_ObjectManager::_ObjectManager() {
	RenderList[0].reserve(5000);
	RenderList[1].reserve(5000);
	RenderList[2].reserve(5000);
}

// Destructor
_ObjectManager::~_ObjectManager() {

	ClearObjects();
}

// Updates all objects
void _ObjectManager::Update(double FrameTime, _Map *Map) {
	RenderList[0].clear();
	RenderList[1].clear();
	RenderList[2].clear();

	// Update objects
	for(auto Iterator = Objects.begin(); Iterator != Objects.end(); ) {
		_Object *Object = *Iterator;

		// Update the object
		Object->Update(FrameTime);

		// Delete old objects
		if(!Object->Active) {

			// Delete object
			delete Object;
			Iterator = Objects.erase(Iterator);
		}
		else {

			// Get object bounds
			glm::vec4 Bounds;
			Object->GetRenderBounds(Bounds);

			// Add to minimap
			if(Map->CheckMinimapBounds(Bounds, HUD_MINIMAP_CAPTURE_SIZE)) {
				_MinimapLayer MinimapLayer;
				MinimapLayer.Bounds = glm::vec4(
					Object->Position.x - Object->Scale * 0.25f,	Object->Position.y - Object->Scale * 0.25f,
					Object->Position.x + Object->Scale * 0.25f, Object->Position.y + Object->Scale * 0.25f
				);

				MinimapLayer.Color = COLOR_WHITE;
				switch(Object->Type) {
					case _Object::KEY:
						MinimapLayer.Color = COLOR_YELLOW;
					break;
					case _Object::AMMO:
						MinimapLayer.Color = COLOR_CYAN;
					break;
					case _Object::UPGRADE:
					case _Object::WEAPON:
					case _Object::ARMOR:
						MinimapLayer.Color = COLOR_GREEN;
					break;
					case _Object::MEDKIT:
					break;
				}
				Map->MinimapLayers.push_back(MinimapLayer);
			}

			// Add to render list
			if(Map->Camera->IsAABBInView(Bounds)) {

				// Add object to render list
				switch(Object->Type) {
					case _Object::KEY:
					case _Object::AMMO:
					case _Object::UPGRADE:
					case _Object::WEAPON:
					case _Object::ARMOR:
					case _Object::MEDKIT:
						RenderList[0].push_back(Object);
					break;
				}
			}

			++Iterator;
		}
	}
}

// Render objects
void _ObjectManager::Render(double BlendFactor) {

	// Draw items
	for(auto Iterator : RenderList[0])
		Iterator->Render(BlendFactor);

	// Draw player
	for(auto Iterator : RenderList[1])
		Iterator->Render(BlendFactor);

	// Draw monsters
	for(auto Iterator : RenderList[2])
		Iterator->Render(BlendFactor);
}

// Deletes all of the objects
void _ObjectManager::ClearObjects() {

	// Delete objects
	for(auto Iterator : Objects)
		delete Iterator;

	Objects.clear();
	RenderList[0].clear();
	RenderList[1].clear();
	RenderList[2].clear();
}

// Adds an object to the manager
void _ObjectManager::AddObject(_Object *Object) {
	Objects.push_back(Object);
}

// Remove an object from the update list
void _ObjectManager::RemoveObject(_Object *Object) {

	for(auto Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
		if(*Iterator == Object) {
			Objects.erase(Iterator);
			break;
		}
	}
}
