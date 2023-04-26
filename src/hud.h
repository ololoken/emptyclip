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
#include <objects/inventory.h>
#include <ae/ui.h>
#include <string>
#include <glm/vec2.hpp>

// Forward Declarations
class _Entity;
class _Player;
class _Item;
class _Weapon;
struct _Bag;
namespace ae {
	class _Camera;
	struct _MouseEvent;
}

class _HUD {

	public:

		enum FontTypes {
			FONT_TINY,
			FONT_SMALL,
			FONT_MEDIUM,
			FONT_LARGE,
			FONT_COUNT
		};

		enum ElementTypes {
			ELEMENT_PLAYERINFO,
			ELEMENT_PLAYERHEALTH,
			ELEMENT_PLAYERSTAMINA,
			ELEMENT_LEVELINFO,
			ELEMENT_ENEMYINFO,
			ELEMENT_INDICATOR,
			ELEMENT_EXPERIENCE,
			ELEMENT_INVENTORY,
			ELEMENT_INVENTORY_TABS,
			ELEMENT_INVENTORY_OUTFIT,
			ELEMENT_INVENTORY_BACKPACK,
			ELEMENT_INVENTORY_OVERLAY,
			ELEMENT_SKILLS,
			ELEMENT_SKILLS_REMAINING,
			ELEMENT_MESSAGE,
			ELEMENT_SKILLINFO,
			ELEMENT_CLOCK,
			LABEL_MESSAGE,
			LABEL_MESSAGEBOX,
			LABEL_LEVELNAME,
			LABEL_PLAYERNAME,
			LABEL_PLAYERLEVEL,
			LABEL_PLAYERHARDCORE,
			LABEL_PLAYERHEALTH,
			LABEL_LEVELKILLS,
			LABEL_LEVELCRATES,
			LABEL_LEVELSECRETS,
			LABEL_LEVELTIME,
			LABEL_CLOCK,
			LABEL_ENEMYNAME,
			LABEL_ENEMYHEALTH,
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
			LABEL_SKILL9,
			LABEL_SKILLTEXT,
			LABEL_SKILLTEXTALT,
			LABEL_SKILL_LEVEL,
			LABEL_SKILL_LEVEL_NEXT,
			IMAGE_PLAYERHEALTH,
			IMAGE_PLAYERSTAMINA,
			IMAGE_ENEMYHEALTH,
			IMAGE_RELOAD,
			ELEMENT_EXPERIENCE_EMPTY,
			IMAGE_EXPERIENCE_FULL,
			ELEMENT_COUNT,
		};

		_HUD(const ae::_Camera *Camera, _Player *Player);
		~_HUD() { }

		void SetStats(int MaxKills, int MaxCrates, int MaxSecrets);
		void SetLastHit(_Entity *Entity);
		void SetInventoryOpen(bool Value);
		void MoveWorldItem(const glm::vec2 &DropPosition=glm::vec2(-1.0f));

		void HandleMouseButton(const ae::_MouseEvent &MouseEvent);
		void Update(double FrameTime, float Radius, double Clock);

		void Render(bool FullMap);
		void DrawCharacterScreen();
		void DrawInventory();
		void DrawCrosshair(const glm::vec2 &Position);
		void DrawDeathScreen();

		void ClearMessageBox(double Duration=0.0) { MessageBoxTimer = Duration; }
		void ShowTextMessage(const std::string &Message, double Time, bool Override=true);
		void ShowMessageBox(const std::string &Message, double Time, const glm::vec2 &Size);
		void ShowLevelName(const std::string &Name, double Time);

		static void FormatTime(char *Buffer, double Time);
		static void FormatTimeHMS(std::ostringstream &Buffer, int64_t Time);

		// Objects
		const ae::_Camera *Camera{nullptr};
		int64_t LastHitHealth{0};
		int64_t LastHitMaxHealth{0};

		// Inventory
		_Item *CursorItem{nullptr};
		_Item *HoverItem{nullptr};
		bool CursorUseWorldPosition{false};
		bool InventoryOpen{false};

		// Stats
		int Kills[2]{0};
		int Crates[2]{0};
		int Secrets[2]{0};

	private:

		void DrawIndicator(const std::string &String, float Percent=0.0f, const ae::_Texture *Texture=nullptr);
		void DrawHUDWeapon(const _Item *Weapon, ae::_Element *Element, ae::_Element *Image, ae::_Element *Label);
		void DrawItemValue(const _Item *Item, const glm::vec2 &Position);
		void DrawItemQuality(const _Item *Item, const glm::vec2 &Position);
		void DrawItemLevel(const _Item *Item, const glm::vec2 &Position);
		void DrawAttribute(const std::string &Label, std::ostringstream &Buffer, glm::vec2 &DrawPosition) const;
		void DrawBag(const _Bag &Bag, ae::_Element *Element);
		void DrawBagHighlights(const _Bag &Bag, ae::_Element *Element);
		void DrawBagGlanceValueText(const _Bag &Bag, ae::_Element *Element);
		void DrawBagInfo(const _Bag &Bag, ae::_Element *Element);
		void DrawInventoryItem(const _Item *Item, const glm::vec2 &Position, bool Unique);
		void DrawUniqueHighlight(const _Item *Item, const glm::vec2 &Position);
		void DrawGlanceValueText(const _Item *Item, const glm::vec2 &Position);
		void UpdateSkillTooltip(int Skill, const glm::vec2 &DrawPosition);
		void GetClockAsString(std::ostringstream &Buffer, double Clock, bool Clock24Hour) const;
		bool ApplyUsableItem(_Item *ExistingItem);
		void GetHitSlot(ae::_Element *Element, _Slot &Slot);
		bool CanGrabItem(const _Item *Item);
		size_t GetBackpackTabIndex(const ae::_Element *Element);

		// State
		_Player *Player{nullptr};

		// UI
		ae::_Element *Elements[ELEMENT_COUNT]{nullptr};
		_Slot DragSlot;
		_Slot CursorSlot;
		int CursorSkill{-1};

		// Displays
		float CrosshairScale{0.0f};

		// Messages
		double MessageTimer{0.0};
		double MessageBoxTimer{0.0};
		double LevelNameTimer{0.0};

		// Text
		ae::_Font *Fonts[FONT_COUNT]{nullptr};
};
