/******************************************************************************
* Empty Clip
* Copyright (C) 2023 Alan Witkowski
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
#pragma once

// Libraries
#include <constants.h>
#include <vector>

// Forward Declarations
class _Object;
class _Map;
namespace ae {
	class _Texture;
}

// Manages all the objects for a map
class _ObjectManager {

	public:

		enum RenderType {
			RENDER_ITEMS,
			RENDER_PLAYER,
			RENDER_MONSTER,
			RENDER_PROP,
			RENDER_PROJECTILES,
			RENDER_LIGHTS,
			RENDER_COUNT
		};

		struct _RenderList {
			std::vector<_Object *> Objects;
			glm::vec2 Scale{0.25f};
			const ae::_Texture *Texture{nullptr};
			double FadeTime{0.0};
			float PositionZ{0.0f};
			float Alpha{1.0f};
			int AmmoTypeID{-1};
			bool Health{false};
			bool Stamina{false};
		};

		_ObjectManager();
		~_ObjectManager();

		// Updates
		void Update(double FrameTime, _Map *Map);
		int Render(int Type, double BlendFactor);
		int RenderItems(double BlendFactor);
		int RenderLights(int Type, double BlendFactor);

		// Management
		void AddObject(_Object *Object);
		void RemoveObject(_Object *Object);
		void ClearObjects();

		// Objects
		std::vector<_Object *> Objects;
		std::vector<_Object *> RenderList[RENDER_COUNT];
		_RenderList ItemRenderList[OBJECT_MAX_RENDERLIST];

		// VBO
		uint32_t RenderVBO{0};
		float *RenderVertices{nullptr};

	private:

		bool IsRenderListFiltered(size_t RenderListIndex);

};
