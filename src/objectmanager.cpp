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
#include <objects/player.h>
#include <states/play.h>
#include <ae/camera.h>
#include <ae/assets.h>
#include <ae/program.h>
#include <ae/graphics.h>
#include <ae/texture.h>
#include <map.h>
#include <stats.h>
#include <constants.h>

const int RENDER_VBO_VERTICES_PER_DRAW = 24;
const int RENDER_VBO_SIZE = 100000 * RENDER_VBO_VERTICES_PER_DRAW;

// Constructor
_ObjectManager::_ObjectManager() {
	RenderVBO = ae::Graphics.CreateVBO(nullptr, RENDER_VBO_SIZE * sizeof(float), GL_DYNAMIC_DRAW);
	RenderVertices = new float[RENDER_VBO_SIZE];

	for(int i = 0; i < RENDER_COUNT; i++)
		RenderList[i].reserve(5000);

	for(int i = 0; i < OBJECT_MAX_RENDERLIST; i++) {
		ItemRenderList[i].Objects.reserve(2000);
		ItemRenderList[i].PositionZ = ITEM_Z;
	}

	for(const auto &Template : Stats.Objects) {
		if(Template.second.RenderListType == -1)
			continue;

		_RenderList &RenderList = ItemRenderList[Template.second.RenderListType];
		RenderList.Texture = ae::Assets.Textures.at(Template.second.IconID);
		RenderList.AmmoTypeID = Template.second.AmmoTypeID;
		RenderList.Health = Template.second.GiveHealth;
		RenderList.Stamina = Template.second.GiveStamina;
	}
}

// Destructor
_ObjectManager::~_ObjectManager() {
	ae::Graphics.DeleteVBO(RenderVBO);
	delete[] RenderVertices;
	ClearObjects();
}

// Updates all objects
void _ObjectManager::Update(double FrameTime, _Map *Map) {
	for(int i = 0; i < RENDER_COUNT; i++)
		RenderList[i].clear();

	for(int i = 0; i < OBJECT_MAX_RENDERLIST; i++)
		ItemRenderList[i].Objects.clear();

	// Update objects
	bool Delete = false;
	bool ReduceMinimap = PlayState.ShowMoreInfo();
	size_t ObjectCount = Objects.size();
	for(size_t i = 0; i < ObjectCount; i++) {
		_Object *Object = Objects[i];

		// Update the object
		Object->Update(FrameTime);

		// Flag deleted objects
		if(!Object->Active) {
			Delete = true;
			continue;
		}

		// Set filtered state
		switch(Object->Type) {
			case _Object::AMMO:
				Object->Filtered = !PlayState.Player->AmmoNeeded[(size_t)Object->Template.AmmoTypeID];
			break;
			case _Object::CONSUMABLE:
				if(Object->Template.GiveHealth && PlayState.Player->Health == PlayState.Player->MaxHealth)
					Object->Filtered = true;
				else if(Object->Template.GiveStamina && PlayState.Player->Stamina == PlayState.Player->MaxStamina)
					Object->Filtered = true;
				else
					Object->Filtered = false;
			break;
			default:
				Object->Filtered = (Object->FilterType >= 0 && Object->Quality < PlayState.GetFilterLevel(Object->FilterType)) ? true : false;
			break;
		}

		// Get object bounds
		glm::vec4 Bounds;
		Object->GetRenderBounds(Bounds);

		// Add to minimap
		if(!Object->Filtered && Map->CheckMinimapBounds(Bounds)) {
			_MinimapIcon MinimapIcon;

			// Get bounds
			float Size = Object->Unique ? Object->Radius * 2.0f : Object->Radius;
			MinimapIcon.Bounds = glm::vec4(
				Object->Position.x - Size, Object->Position.y - Size,
				Object->Position.x + Size, Object->Position.y + Size
			);

			// Get color
			switch(Object->Type) {
				case _Object::WEAPON:
				case _Object::ARMOR:
				case _Object::MOD:
					if(Object->Unique)
						Map->MinimapIcons[_Map::MINIMAP_UNIQUE].push_back(MinimapIcon);
					else
						Map->MinimapIcons[_Map::MINIMAP_GEAR].push_back(MinimapIcon);
				break;
				case _Object::KEY:
					Map->MinimapIcons[_Map::MINIMAP_KEY].push_back(MinimapIcon);
				break;
				case _Object::AMMO:
					if(!ReduceMinimap && !Object->Filtered)
						Map->MinimapIcons[_Map::MINIMAP_AMMO].push_back(MinimapIcon);
				break;
				case _Object::CONSUMABLE:
					if(!ReduceMinimap)
						Map->MinimapIcons[_Map::MINIMAP_CONSUMABLE].push_back(MinimapIcon);
				break;
				case _Object::USABLE:
					Map->MinimapIcons[_Map::MINIMAP_UNIQUE].push_back(MinimapIcon);
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
				if(Item->CanHide() && PlayState.ShowMoreInfo())
					DrawLight = false;
				else {
					if(Item->Template.RenderListType != -1)
						ItemRenderList[Item->Template.RenderListType].Objects.push_back(Object);
					else
						RenderList[RENDER_ITEMS].push_back(Object);
				}
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

	if(!Delete)
		return;

	// Clean up deleted objects
	for(size_t i = Objects.size() - 1; i < Objects.size(); i--) {
		if(Objects[i]->Active)
			continue;

		// Delete object
		delete Objects[i];
		Objects.erase(Objects.begin() + (int)i);
	}
}

// Render objects
int _ObjectManager::Render(int Type, double BlendFactor) {
	for(const auto &Object : RenderList[Type])
		Object->Render(BlendFactor);

	return (int)RenderList[Type].size();
}

// Render items
int _ObjectManager::RenderItems(double BlendFactor) {
	int RenderCount = 0;

	// Set up program
	ae::Graphics.SetProgram(ae::Assets.Programs["item"]);
	ae::Graphics.SetVertexBufferID(RenderVBO);
	ae::Graphics.SetAttribLevel(2);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, nullptr);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (GLvoid *)(sizeof(float) * 2));

	// Iterate through each render list type
	for(int i = 0; i < OBJECT_MAX_RENDERLIST; i++) {
		if(ItemRenderList[i].Objects.empty())
			continue;

		// Set up program
		glm::vec4 RenderColor(1.0f);
		if(ItemRenderList[i].AmmoTypeID != -1 && !PlayState.Player->AmmoNeeded[(size_t)ItemRenderList[i].AmmoTypeID])
			RenderColor.a = ITEM_FILTERED_ALPHA;
		else if(ItemRenderList[i].Health && PlayState.Player->Health == PlayState.Player->MaxHealth)
			RenderColor.a = ITEM_FILTERED_ALPHA;
		else if(ItemRenderList[i].Stamina && PlayState.Player->Stamina == PlayState.Player->MaxStamina)
			RenderColor.a = ITEM_FILTERED_ALPHA;

		ae::Graphics.SetColor(RenderColor);
		ae::Assets.Programs["item"]->SetUniformFloat("pos_z", ItemRenderList[i].PositionZ);
		ae::Graphics.SetTextureID(ItemRenderList[i].Texture->ID);

		// Build vertex buffer
		int VertexIndex = 0;
		for(const auto &Object : ItemRenderList[i].Objects) {
			if(VertexIndex + RENDER_VBO_VERTICES_PER_DRAW > RENDER_VBO_SIZE)
				break;

			// Get blended draw position
			glm::vec2 DrawPosition;
			Object->GetDrawPosition(DrawPosition, BlendFactor);

			// Get bounds
			glm::vec2 Start = DrawPosition - ItemRenderList[i].Scale;
			glm::vec2 End = DrawPosition + ItemRenderList[i].Scale;

			// First triangle of quad
			RenderVertices[VertexIndex++] = Start.x;
			RenderVertices[VertexIndex++] = Start.y;
			RenderVertices[VertexIndex++] = 0.0f;
			RenderVertices[VertexIndex++] = 0.0f;
			RenderVertices[VertexIndex++] = Start.x;
			RenderVertices[VertexIndex++] = End.y;
			RenderVertices[VertexIndex++] = 0.0f;
			RenderVertices[VertexIndex++] = 1.0f;
			RenderVertices[VertexIndex++] = End.x;
			RenderVertices[VertexIndex++] = End.y;
			RenderVertices[VertexIndex++] = 1.0f;
			RenderVertices[VertexIndex++] = 1.0f;

			// Second triangle of quad
			RenderVertices[VertexIndex++] = End.x;
			RenderVertices[VertexIndex++] = End.y;
			RenderVertices[VertexIndex++] = 1.0f;
			RenderVertices[VertexIndex++] = 1.0f;
			RenderVertices[VertexIndex++] = End.x;
			RenderVertices[VertexIndex++] = Start.y;
			RenderVertices[VertexIndex++] = 1.0f;
			RenderVertices[VertexIndex++] = 0.0f;
			RenderVertices[VertexIndex++] = Start.x;
			RenderVertices[VertexIndex++] = Start.y;
			RenderVertices[VertexIndex++] = 0.0f;
			RenderVertices[VertexIndex++] = 0.0f;
		}

		// Draw buffer
		glBufferSubData(GL_ARRAY_BUFFER, 0, VertexIndex * sizeof(float), RenderVertices);
		glDrawArrays(GL_TRIANGLES, 0, VertexIndex >> 2);

		// Update total
		RenderCount += (int)ItemRenderList[i].Objects.size();
	}

	return RenderCount;
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

	for(int i = 0; i < OBJECT_MAX_RENDERLIST; i++)
		ItemRenderList[i].Objects.clear();
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
