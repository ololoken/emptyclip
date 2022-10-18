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
#include <objects/item.h>
#include <states/play.h>
#include <ae/camera.h>
#include <ae/assets.h>
#include <ae/program.h>
#include <ae/graphics.h>
#include <map.h>
#include <stats.h>
#include <constants.h>

// Constructor
_ObjectManager::_ObjectManager() {
	for(int i = 0; i < RENDER_COUNT; i++)
		RenderList[i].reserve(5000);
}

// Destructor
_ObjectManager::~_ObjectManager() {
	ClearObjects();
}

// Updates all objects
void _ObjectManager::Update(double FrameTime, _Map *Map) {
	for(int i = 0; i < RENDER_COUNT; i++)
		RenderList[i].clear();

	// Update objects
	for(size_t i = Objects.size() - 1; i < Objects.size(); i--) {
		_Object *Object = Objects[i];

		// Update the object
		Object->Update(FrameTime);

		// Delete old objects
		if(!Object->Active) {

			// Delete object
			delete Object;
			Objects.erase(Objects.begin() + i);
		}
		else {

			// Get object bounds
			glm::vec4 Bounds;
			Object->GetRenderBounds(Bounds);

			// Add to minimap
			if(Map->CheckMinimapBounds(Bounds)) {
				_MinimapIcon MinimapIcon;

				// Get bounds
				float Size = Object->IsUnique() ? Object->Radius * 2.0f : Object->Radius;
				MinimapIcon.Bounds = glm::vec4(
					Object->Position.x - Size, Object->Position.y - Size,
					Object->Position.x + Size, Object->Position.y + Size
				);

				// Get color
				switch(Object->Type) {
					case _Object::WEAPON:
					case _Object::ARMOR:
					case _Object::MOD:
						if(Object->IsUnique())
							Map->MinimapIcons[_Map::MINIMAP_UNIQUE].push_back(MinimapIcon);
						else
							Map->MinimapIcons[_Map::MINIMAP_EQUIPMENT].push_back(MinimapIcon);
					break;
					case _Object::KEY:
						Map->MinimapIcons[_Map::MINIMAP_KEY].push_back(MinimapIcon);
					break;
					case _Object::AMMO:
						Map->MinimapIcons[_Map::MINIMAP_AMMO].push_back(MinimapIcon);
					break;
					case _Object::MEDKIT:
						Map->MinimapIcons[_Map::MINIMAP_MEDKIT].push_back(MinimapIcon);
					break;
					case _Object::PROP:
						Map->MinimapIcons[_Map::MINIMAP_WALL].push_back(MinimapIcon);
					break;
					case _Object::PROJECTILE:
						Map->MinimapIcons[_Map::MINIMAP_PROJECTILE].push_back(MinimapIcon);
					break;
				}
			}

			// Add to render list
			bool DrawLight = true;
			if(Map->Camera->IsAABBInView(Bounds)) {
				if(Object->Template.IsItem()) {
					_Item *Item = (_Item *)Object;

					// Hide pickups when more info is shown
					if(Item->IsHideable() && PlayState.ShowMoreInfo())
						DrawLight = false;
					else
						RenderList[RENDER_ITEMS].push_back(Object);
				}
				else if(Object->Template.Type == _Object::PROP)
					RenderList[RENDER_PROP].push_back(Object);
				else if(Object->Template.Type == _Object::PROJECTILE) {
					RenderList[RENDER_PROJECTILES].push_back(Object);
				}
			}

			// Get light bounds
			if(Object->LightTexture && DrawLight) {
				Object->GetLightBounds(Bounds);
				if(Map->Camera->IsAABBInView(Bounds))
					RenderList[RENDER_LIGHTS].push_back(Object);
			}
		}
	}
}

// Render objects
int _ObjectManager::Render(int Type, double BlendFactor) {
	for(auto Iterator = RenderList[Type].rbegin(); Iterator != RenderList[Type].rend(); ++Iterator)
		(*Iterator)->Render(BlendFactor);

	return (int)RenderList[Type].size();
}

// Render object lights
int _ObjectManager::RenderLights(int Type, double BlendFactor) {
	for(auto Iterator : RenderList[Type])
		Iterator->RenderLights(BlendFactor);

	return (int)RenderList[Type].size();
}

// Deletes all of the objects
void _ObjectManager::ClearObjects() {

	// Delete objects
	for(auto Iterator : Objects)
		delete Iterator;

	Objects.clear();

	for(int i = 0; i < RENDER_COUNT; i++)
		RenderList[i].clear();
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
