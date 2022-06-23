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
#pragma once

// Libraries
#include <string>
#include <ae/ui.h>
#include <glm/vec2.hpp>

// Forward Declarations
class _Element;
class _Font;
class _Entity;
class _Player;
class _Item;
class _Weapon;
namespace ae {
	struct _MouseEvent;
}

class _HUD {

	public:

		enum FontTypes {
			FONT_TINY,
			FONT_SMALL,
			FONT_MEDIUM,
			FONT_LARGE,
			FONT_LARGER,
			FONT_LARGEST,
			FONT_COUNT
		};

		enum ElementTypes {
			ELEMENT_PLAYERINFO,
			ELEMENT_PLAYERHEALTH,
			ELEMENT_PLAYERSTAMINA,
			ELEMENT_ENEMYINFO,
			ELEMENT_INDICATOR,
			ELEMENT_EXPERIENCE,
			ELEMENT_INVENTORY,
			ELEMENT_SKILLS,
			ELEMENT_MESSAGE,
			ELEMENT_SKILLINFO,
			LABEL_FPS,
			LABEL_MESSAGE,
			LABEL_MESSAGEBOX,
			LABEL_PLAYERNAME,
			LABEL_PLAYERLEVEL,
			LABEL_PLAYERHEALTH,
			LABEL_ENEMYNAME,
			LABEL_INDICATOR,
			LABEL_EXPERIENCE,
			LABEL_SKILL_REMAINING,
			LABEL_SKILL0,
			LABEL_SKILL1,
			LABEL_SKILL2,
			LABEL_SKILL3,
			LABEL_SKILL4,
			LABEL_SKILL5,
			LABEL_SKILL6,
			LABEL_SKILL7,
			LABEL_SKILL8,
			LABEL_SKILLTEXT,
			LABEL_SKILL_LEVEL,
			LABEL_SKILL_LEVEL_NEXT,
			LABEL_DAMAGE,
			LABEL_MELEEDAMAGE,
			LABEL_DAMAGEBLOCK,
			LABEL_DAMAGERESIST,
			LABEL_MOVEMENTSPEED,
			LABEL_KILLS,
			IMAGE_PLAYERHEALTH,
			IMAGE_PLAYERSTAMINA,
			IMAGE_ENEMYHEALTH,
			IMAGE_RELOAD,
			IMAGE_EXPERIENCE,
			ELEMENT_COUNT,
		};

		_HUD(_Player *Player);
		~_HUD();

		void SetLastEntityHit(_Entity *Entity);
		void SetInventoryOpen(bool Value);
		bool GetInventoryOpen() { return InventoryOpen; }
		bool IsDragging() const { return CursorItem != nullptr; }

		void MouseEvent(const ae::_MouseEvent &MouseEvent);
		void Update(double FrameTime, float Radius);

		void Render();
		void RenderCharacterScreen();
		void DrawMinimap();
		void UpdateSkillInfo(int Skill, int DrawX, int DrawY);
		void RenderCrosshair(const glm::vec2 &Position);
		void RenderDeathScreen();

		void ShowTextMessage(const std::string &Message, double Time);
		void ShowMessageBox(const std::string &Message, double Time);
		double GetMessageBoxTimer() { return MessageBoxTimer; }

		_Item *CursorOverItem;

	private:

		void DrawIndicator(const std::string &String, float Percent=0.0f, const ae::_Texture *Texture=nullptr);
		void DrawHUDWeapon(const _Weapon *Weapon, ae::_Element *Element, ae::_Element *Image, ae::_Element *Label);
		void DrawItemCount(_Item *Item, int X, int Y);

		// State
		_Player *Player;
		bool InventoryOpen;

		// UI
		ae::_Element *Elements[ELEMENT_COUNT];
		ae::_Element *DragStart;
		_Item *CursorItem;
		glm::ivec2 ClickOffset;
		int CursorSkill;

		// Displays
		_Entity *LastEntityHit;
		double LastEntityHitTimer;
		float CrosshairScale;

		// Messages
		double MessageTimer, MessageBoxTimer;

		// Text
		ae::_Font *Fonts[FONT_COUNT];

		// Textures
		const ae::_Texture *CrosshairTexture;
		const ae::_Texture *ReloadTexture;
		const ae::_Texture *WeaponSwitchTexture;
};
