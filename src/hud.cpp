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
#include <hud.h>
#include <objects/entity.h>
#include <objects/player.h>
#include <objects/inventory.h>
#include <objects/item.h>
#include <ae/input.h>
#include <ae/actions.h>
#include <ae/graphics.h>
#include <ae/font.h>
#include <ae/program.h>
#include <ae/assets.h>
#include <ae/actions.h>
#include <ae/audio.h>
#include <ae/util.h>
#include <ae/camera.h>
#include <states/play.h>
#include <map.h>
#include <menu.h>
#include <actiontype.h>
#include <config.h>
#include <stats.h>
#include <gameassets.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <SDL_mouse.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

struct _MinimapLegend {
	std::string Label;
	glm::vec4 Color;
};

struct _SkillText {
	std::string Text[2];
};

static std::vector<_MinimapLegend> MinimapLegends = {
	{ "Keys", MINIMAP_KEY_COLOR},
	{ "Gear",  MINIMAP_GEAR_COLOR },
	{ "Mod",  MINIMAP_MOD_COLOR },
	{ "Ammo", MINIMAP_AMMO_COLOR},
	{ "Consumables", MINIMAP_CONSUMABLE_COLOR },
	{ "Crates", MINIMAP_CRATE_COLOR },
	{ "Enemies", MINIMAP_ENEMY_COLOR },
	{ "Doors/Switches", MINIMAP_DOOR_COLOR },
};

static _SkillText SkillText[SKILL_COUNT] = {
	{ "Melee Damage", "Gun Handling" },
	{ "Reload Speed", "Switch Speed" },
	{ "Damage Resist", "Self Damage Resist" },
	{ "Max Health", "Heal Bonus" },
	{ "Attack/Fire Rate","Self Heal Speed" },
	{ "Move Speed", "Self Heal Delay" },
	{ "Max Stamina", "Max Ammo" },
	{ "Gun Accuracy", "Critical Hit Damage" },
	{ "Experience Gain", "Gear Mod Capacity" },
	{ "Rarity Chance", "Ammo Drop Amount" },
};

// Initialize
_HUD::_HUD(const ae::_Camera *Camera, _Player *Player) : Camera(Camera), Player(Player) {

	// Get textures
	Fonts[FONT_TINY] = ae::Assets.Fonts["hud_tiny"];
	Fonts[FONT_SMALL] = ae::Assets.Fonts["hud_small"];
	Fonts[FONT_MEDIUM] = ae::Assets.Fonts["hud_medium"];
	Fonts[FONT_LARGE] = ae::Assets.Fonts["hud_large"];

	// Elements
	Elements[LABEL_MESSAGE] = ae::Assets.Elements["label_hud_message"];
	Elements[LABEL_MESSAGEBOX] = ae::Assets.Elements["label_hud_messagebox_text"];
	Elements[LABEL_LEVELNAME] = ae::Assets.Elements["label_hud_levelname"];

	Elements[LABEL_MESSAGE]->SetActive(true);
	Elements[LABEL_MESSAGEBOX]->SetActive(true);
	Elements[LABEL_LEVELNAME]->SetActive(true);

	Elements[ELEMENT_PLAYERINFO] = ae::Assets.Elements["element_hud_player_info"];
	Elements[LABEL_PLAYERNAME] = ae::Assets.Elements["label_hud_player_name"];
	Elements[LABEL_PLAYERLEVEL] = ae::Assets.Elements["label_hud_player_level"];
	Elements[LABEL_PLAYERHARDCORE] = ae::Assets.Elements["label_hud_player_hardcore"];
	Elements[LABEL_PLAYERHEALTH] = ae::Assets.Elements["label_hud_player_health"];
	Elements[ELEMENT_PLAYERINFO]->SetActive(true);

	Elements[ELEMENT_LEVELINFO] = ae::Assets.Elements["element_hud_level_info"];
	Elements[LABEL_LEVELKILLS] = ae::Assets.Elements["label_hud_level_kills"];
	Elements[LABEL_LEVELCRATES] = ae::Assets.Elements["label_hud_level_crates"];
	Elements[LABEL_LEVELSECRETS] = ae::Assets.Elements["label_hud_level_secrets"];
	Elements[LABEL_LEVELTIME] = ae::Assets.Elements["label_hud_level_time"];
	Elements[ELEMENT_LEVELINFO]->BaseOffset.y = 80 + Player->Hardcore * 30;
	Elements[ELEMENT_LEVELINFO]->CalculateBounds();
	Elements[ELEMENT_LEVELINFO]->SetActive(true);

	Elements[ELEMENT_CLOCK] = ae::Assets.Elements["element_hud_clock"];
	Elements[LABEL_CLOCK] = ae::Assets.Elements["label_hud_clock"];
	Elements[ELEMENT_CLOCK]->SetActive(true);

	Elements[ELEMENT_ENEMYINFO] = ae::Assets.Elements["element_hud_enemy_info"];
	Elements[LABEL_ENEMYNAME] = ae::Assets.Elements["label_hud_enemy_name"];
	Elements[ELEMENT_ENEMYINFO]->SetActive(true);

	Elements[ELEMENT_PLAYERHEALTH] = ae::Assets.Elements["element_hud_player_health"];
	Elements[IMAGE_PLAYERHEALTH] = ae::Assets.Elements["image_hud_player_health_full"];
	Elements[LABEL_PLAYERHEALTH] = ae::Assets.Elements["label_hud_player_health_text"];
	Elements[ELEMENT_PLAYERHEALTH]->SetActive(true);

	Elements[ELEMENT_PLAYERSTAMINA] = ae::Assets.Elements["element_hud_player_stamina"];
	Elements[IMAGE_PLAYERSTAMINA] = ae::Assets.Elements["image_hud_player_stamina_full"];
	Elements[ELEMENT_PLAYERSTAMINA]->SetActive(true);

	Elements[LABEL_ENEMYHEALTH] = ae::Assets.Elements["label_hud_enemy_health_text"];
	Elements[IMAGE_ENEMYHEALTH] = ae::Assets.Elements["image_hud_enemy_health_full"];
	Elements[IMAGE_ENEMYHEALTH]->SetActive(true);

	Elements[ELEMENT_INDICATOR] = ae::Assets.Elements["element_hud_indicator"];
	Elements[IMAGE_RELOAD] = ae::Assets.Elements["image_hud_indicator_progress"];
	Elements[LABEL_INDICATOR] = ae::Assets.Elements["label_hud_indicator_text"];
	Elements[ELEMENT_INDICATOR]->SetActive(true);

	Elements[ELEMENT_EXPERIENCE] = ae::Assets.Elements["element_hud_experience"];
	Elements[IMAGE_EXPERIENCE] = ae::Assets.Elements["image_hud_experience_bar_full"];
	Elements[LABEL_EXPERIENCE] = ae::Assets.Elements["label_hud_experience_text"];
	Elements[ELEMENT_EXPERIENCE]->SetActive(true);

	ae::Assets.Elements["element_hud_mainhand"]->SetActive(true);
	ae::Assets.Elements["element_hud_offhand"]->SetActive(true);
	ae::Assets.Elements["element_hud_melee"]->SetActive(true);
	ae::Assets.Elements["element_hud_filters"]->SetActive(true);

	Elements[ELEMENT_INVENTORY] = ae::Assets.Elements["element_inventory"];
	Elements[ELEMENT_INVENTORY_TABS] = ae::Assets.Elements["element_inventory_tabs"];
	Elements[ELEMENT_INVENTORY_OUTFIT] = ae::Assets.Elements["element_inventory_outfit"];
	Elements[ELEMENT_INVENTORY_BACKPACK] = ae::Assets.Elements["element_inventory_backpack"];
	Elements[ELEMENT_INVENTORY_OVERLAY] = ae::Assets.Elements["element_inventory_overlay"];
	Elements[ELEMENT_SKILLS] = ae::Assets.Elements["element_skills"];
	Elements[LABEL_SKILL_REMAINING] = ae::Assets.Elements["label_hud_skill_remaining_value"];
	Elements[LABEL_SKILL0] = ae::Assets.Elements["label_hud_skill0_value"];
	Elements[LABEL_SKILL1] = ae::Assets.Elements["label_hud_skill1_value"];
	Elements[LABEL_SKILL2] = ae::Assets.Elements["label_hud_skill2_value"];
	Elements[LABEL_SKILL3] = ae::Assets.Elements["label_hud_skill3_value"];
	Elements[LABEL_SKILL4] = ae::Assets.Elements["label_hud_skill4_value"];
	Elements[LABEL_SKILL5] = ae::Assets.Elements["label_hud_skill5_value"];
	Elements[LABEL_SKILL6] = ae::Assets.Elements["label_hud_skill6_value"];
	Elements[LABEL_SKILL7] = ae::Assets.Elements["label_hud_skill7_value"];
	Elements[LABEL_SKILL8] = ae::Assets.Elements["label_hud_skill8_value"];
	Elements[LABEL_SKILL9] = ae::Assets.Elements["label_hud_skill9_value"];
	Elements[ELEMENT_INVENTORY]->SetActive(false);
	Elements[ELEMENT_SKILLS]->SetActive(false);

	Elements[ELEMENT_SKILLINFO] = ae::Assets.Elements["element_skill_info"];
	Elements[LABEL_SKILLTEXT] = ae::Assets.Elements["label_hud_skill_text"];
	Elements[LABEL_SKILLTEXTALT] = ae::Assets.Elements["label_hud_skill_textalt"];
	Elements[LABEL_SKILL_LEVEL] = ae::Assets.Elements["label_hud_skill_level"];
	Elements[LABEL_SKILL_LEVEL_NEXT] = ae::Assets.Elements["label_hud_skill_level_next"];
	Elements[ELEMENT_SKILLINFO]->SetActive(true);

	Elements[ELEMENT_MESSAGE] = ae::Assets.Elements["element_hud_messagebox"];
	Elements[ELEMENT_MESSAGE]->SetActive(true);
}

// Handle mouse buttons
void _HUD::HandleMouseButton(const ae::_MouseEvent &MouseEvent) {
	if(!InventoryOpen)
		return;

	// Pass event to inventory
	Elements[ELEMENT_INVENTORY]->HandleMouseButton(MouseEvent.Pressed);

	// Handle button clicks
	ae::_Element *Clicked = Elements[ELEMENT_INVENTORY]->GetClickedElement();
	if(Clicked) {
		if(Clicked->ID == "button_inventory_sort") {
			if(!CursorItem)
				Player->SortInventory();
		}
		else if(Clicked->Parent && Clicked->Parent->ID == "element_inventory_tabs") {
			if(Clicked->Index >= 0 && Clicked->Index < HUD_BACKPACK_INDEX)
				Player->StartOutfitSwitch((size_t)Clicked->Index);
			else if(Clicked->Index - HUD_BACKPACK_INDEX < (int)Player->Inventory->Containers[(size_t)BagType::BACKPACK].size())
				Player->ActiveBackpack = (uint64_t)(Clicked->Index - HUD_BACKPACK_INDEX);
		}
	}

	// Get hit element
	ae::_Element *HitElement = Elements[ELEMENT_INVENTORY]->HitElement;
	_Slot HitSlot;
	GetHitSlot(HitElement, HitSlot);

	// Check click type
	switch(MouseEvent.Button) {
		case SDL_BUTTON_LEFT:

			// Mouse press
			if(MouseEvent.Pressed) {
				if(Player->CanDragItems()) {

					// Reorganize item
					if(ae::Input.ModKeyDown(KMOD_SHIFT)) {

						// Find better bag
						if(HitSlot.IsValidIndex() && HitSlot.GetItem()) {
							_Item *Item = HitSlot.GetItem();
							size_t BagIndex = Player->Inventory->FindSuitableBackpackBag(Item, Player->ActiveBackpack, true, false);
							if(BagIndex != Player->ActiveBackpack) {
								int AddResult = Player->AddItemToBackpack(Item, BagIndex, true);
								switch(AddResult) {
									case ADD_REMOVE:
										HitSlot.RemoveItem();
									break;
									case ADD_DELETE:
										HitSlot.RemoveItem();
										Item->Active = false;
									break;
								}
							}
						}
					}
					// Start dragging an item
					else {

						// Drag from inventory
						if(HitSlot.IsValidIndex()) {
							if(ae::Input.ModKeyDown(KMOD_CTRL)) {
								Player->DropItem(HitSlot);
							}
							else {
								DragSlot = HitSlot;
								CursorItem = DragSlot.GetItem();
							}
						}
						// Drag from world
						else if(CanGrabItem(HoverItem)) {
							if(HoverItem->CanMove()) {
								CursorItem = HoverItem;
								CursorItem->Visible = false;
							}
							else
								PlayState.PickupObject(HoverItem, true);
						}
					}
				}
			}
			// Mouse released
			else {

				// Was dragging an item
				if(CursorItem) {
					CursorItem->Visible = true;

					// From inventory
					if(DragSlot.Bag) {

						// No bag was hit
						if(!HitSlot.Bag) {

							// Dragged item to inventory tab
							size_t BagIndex = GetBackpackTabIndex(HitElement);
							if(BagIndex != (size_t)-1 && &Player->Inventory->Containers[(size_t)BagType::BACKPACK][BagIndex] != DragSlot.Bag) {
								int AddResult = Player->AddItemToBackpack(CursorItem, BagIndex, true);
								switch(AddResult) {
									case ADD_REMOVE:
										DragSlot.RemoveItem();
									break;
									case ADD_DELETE:
										DragSlot.RemoveItem();
										CursorItem->Active = false;
									break;
									case ADD_FULL:
										ShowTextMessage(HUD_BACKPACKFULLMESSAGE, HUD_INVENTORYFULLTIME);
									break;
								}
							}
							// Dragged outside window
							else if(!HitElement) {

								// Get world position
								glm::vec2 WorldPosition;
								Camera->ConvertScreenToWorld(ae::Input.GetMouse(), WorldPosition);
								PlayState.Map->GetDropPosition(Player, PLAYER_REACH_DISTANCE, WorldPosition);

								// Drop item
								Player->DropItem(DragSlot, WorldPosition);
							}
						}
						// Move item to another slot
						else if(!Player->IsSwitching())
							Player->SwapInventory(DragSlot, HitSlot);
					}
					// From world
					else {

						// Drag directly into inventory
						if(HitSlot.IsValidIndex()) {
							Player->UseTimer = 0.0;

							// Drag onto inventory
							bool CanEquip = HitSlot.IsGearSlot() && Player->CanEquipItem(CursorItem, HitSlot.Index);
							bool SetAndRemove = false;

							// Drag onto existing item
							_Item *ExistingItem = HitSlot.GetItem();
							if(ExistingItem) {
								if(CursorItem->Type == _Object::USABLE) {
									if(ApplyUsableItem(ExistingItem))
										HitSlot.DeleteItem();
								}
								else if(ExistingItem->AddMod(CursorItem)) {
									ae::Audio.PlaySound(ae::Assets.Sounds["game_mod.ogg"]);
									CursorItem->Visible = false;
									PlayState.Map->RemoveObject(CursorItem, GRID_ITEM);
								}
								else if(CursorItem->Type == _Object::MOD && ExistingItem->CanEquip() && !ExistingItem->ItemCompatible(CursorItem)) {
									MoveWorldItem(Player->Position);
								}
								else if(CanEquip || !HitSlot.IsGearSlot()) {
									Player->DropItem(HitSlot);
									SetAndRemove = true;
								}
							}
							// Drag onto empty slot
							else if(CursorItem->Moveable && (CanEquip || !HitSlot.IsGearSlot())) {
								SetAndRemove = true;
							}

							// Add item to inventory and remove from world
							if(SetAndRemove) {
								CursorItem->Visible = false;
								HitSlot.SetItem(CursorItem);
								PlayState.Map->RemoveObject(CursorItem, GRID_ITEM);
								Player->PlayEquipSound(HitSlot.Index);
							}

							Player->RecalculateStats();
						}
						else {

							// Drag onto inventory tab
							size_t BagIndex = GetBackpackTabIndex(HitElement);
							if(BagIndex != (size_t)-1) {
								int AddResult = Player->AddItemToBackpack(CursorItem, BagIndex, true);
								switch(AddResult) {
									case ADD_REMOVE:
										PlayState.Map->RemoveObject(CursorItem, GRID_ITEM);
									break;
									case ADD_DELETE:
										PlayState.Map->RemoveObject(CursorItem, GRID_ITEM);
										CursorItem->Active = false;
									break;
									case ADD_FULL:
										ShowTextMessage(HUD_BACKPACKFULLMESSAGE, HUD_INVENTORYFULLTIME);
									break;
								}
							}
							// Drop to another world location
							else
								MoveWorldItem(HitElement ? Player->Position : glm::vec2(-1.0f));
						}
					}

					CursorItem = nullptr;
				}

				DragSlot.Reset();
			}
		break;
		case SDL_BUTTON_RIGHT:
			if(MouseEvent.Pressed && Player->CanEquipItems() && !CursorItem) {

				// Equip from backpack
				if(HitSlot.IsValidIndex() && HitSlot.GetItem()) {

					// Unequip item
					if(HitSlot.Bag->Gear) {

						// Find bag
						size_t BagIndex = Player->Inventory->FindSuitableBackpackBag(HitSlot.GetItem(), Player->ActiveBackpack, false, true);

						// Find empty slot
						_Slot EmptySlot;
						EmptySlot.Bag = &Player->Inventory->Containers[(size_t)BagType::BACKPACK][BagIndex];
						EmptySlot.Index = EmptySlot.Bag->FindEmptySlot();
						if(EmptySlot.IsValidIndex())
							Player->SwapInventory(HitSlot, EmptySlot);
						else
							ShowTextMessage(HUD_BACKPACKFULLMESSAGE, HUD_INVENTORYFULLTIME);
					}
					// Equip item from backpack
					else {
						switch(HitSlot.GetItem()->Type) {
							case _Object::WEAPON: {
								if(HitSlot.GetItem()->IsMelee()) {
									Player->SwapInventory(HitSlot, _Slot(&Player->GetActiveOutfitBag(), GearType::MELEE));
								}
								else {
									_Slot EquipSlot;
									EquipSlot.Bag = &Player->GetActiveOutfitBag();
									if(!Player->GetMainHand())
										EquipSlot.Index = GearType::MAINHAND;
									else if(!Player->GetOffHand())
										EquipSlot.Index = GearType::OFFHAND;
									else
										EquipSlot.Index = ae::Input.ModKeyDown(KMOD_CTRL) ? GearType::OFFHAND : GearType::MAINHAND;
									Player->SwapInventory(HitSlot, EquipSlot);
								}
							} break;
							case _Object::ARMOR:
								Player->SwapInventory(HitSlot, _Slot(&Player->GetActiveOutfitBag(), GearType::ARMOR));
							break;
						}
					}

					HoverItem = HitSlot.GetItem();
				}
				// Equip from world
				else if(CanGrabItem(HoverItem)) {
					size_t SlotIndex = (size_t)-1;
					switch(HoverItem->Type) {
						case _Object::WEAPON: {
							if(HoverItem->IsMelee())
								SlotIndex = GearType::MELEE;
							else
								SlotIndex = ae::Input.ModKeyDown(KMOD_CTRL) ? GearType::OFFHAND : GearType::MAINHAND;
						} break;
						case _Object::ARMOR:
							SlotIndex = GearType::ARMOR;
						break;
						default:
							PlayState.PickupObject(HoverItem, true);
						break;
					}

					// Equip item
					if(SlotIndex != (size_t)-1) {
						Player->DropItem(_Slot(&Player->GetActiveOutfitBag(), SlotIndex));
						Player->GetActiveOutfitBag().Slots[SlotIndex] = HoverItem;
						PlayState.Map->RemoveObject(HoverItem, GRID_ITEM);
						Player->RecalculateStats();
						Player->PlayEquipSound(SlotIndex);
					}
				}
			}
		break;
		case SDL_BUTTON_MIDDLE:
			if(MouseEvent.Pressed) {
				if(HitSlot.IsValidIndex()) {
					_Item *DropItem = HitSlot.GetItem();
					if(DropItem && DropItem != CursorItem)
						Player->DropItem(HitSlot);
				}
			}
		break;
	}

	// Level up skill
	HitElement = Elements[ELEMENT_SKILLS]->HitElement;
	if(MouseEvent.Pressed && MouseEvent.Button == SDL_BUTTON_LEFT) {
		if(HitElement && HitElement->Index >= 0 && HitElement->Children.front()->Enabled) {
			int Amount = 1;
			if(ae::Input.ModKeyDown(KMOD_CTRL))
				Amount = 100;
			else if(ae::Input.ModKeyDown(KMOD_SHIFT))
				Amount = 5;

			Player->UpdateSkill(HitElement->Index, Amount);
		}
	}
}

// Update phase
void _HUD::Update(double FrameTime, float Radius, double Clock) {
	HoverItem = nullptr;
	CursorSlot.Bag = nullptr;
	CursorSlot.Index = (size_t)-1;
	CursorSkill = -1;

	// Update clock
	std::ostringstream Buffer;
	if(PlayState.ShowMoreInfo()) {
		std::time_t CurrentTime = std::time(nullptr);
		Buffer << std::put_time(std::localtime(&CurrentTime), "%X");
	}
	else
		GetClockAsString(Buffer, Clock, false);

	Elements[LABEL_CLOCK]->Text = Buffer.str();
	Buffer.str("");

	// Update crosshair
	if(Radius < 0.0f)
		CrosshairScale = 0.0f;
	else
		CrosshairScale = std::max(HUD_MINCROSSHAIRSCALE, CrosshairScale + (Radius - CrosshairScale) / HUD_CROSSHAIRDIVISOR);

	// Update inventory
	if(InventoryOpen) {
		Menu.ShowDefaultCursor(true);

		// Get cursor item
		GetHitSlot(Elements[ELEMENT_INVENTORY]->HitElement, CursorSlot);
		if(CursorSlot.IsValidIndex() && CursorSlot.GetItem()) {
			HoverItem = CursorSlot.GetItem();
			CursorUseWorldPosition = false;
		}

		// Update type counts for each bag
		Player->Inventory->UpdateTypeCount();

		// Set outfit tab checked state
		_Container &OutfitContainer = Player->Inventory->Containers[(size_t)BagType::OUTFIT];
		for(auto &Element : Elements[ELEMENT_INVENTORY_TABS]->Children) {
			size_t Index = (size_t)Element->Index;

			// Set outfit tab state
			Element->Clickable = true;
			Element->Children.front()->Color = COLOR_WHITE;
			Element->Children.back()->Text = "";
			if(Index < HUD_BACKPACK_INDEX) {
				Element->Checked = (Index == Player->ActiveOutfit);
				Element->Children.front()->Texture = ae::Assets.Textures["textures/hud/outfit.png"];
				if(!Element->Checked)
					Element->Children.back()->Text = ae::Actions.GetInputNameForAction(Action::GAME_SWITCHOUTFIT);
				if(Index < OutfitContainer.size() && OutfitContainer[Index].Slots[GearType::ARMOR])
					Element->Children.front()->Texture = OutfitContainer[Index].Slots[GearType::ARMOR]->Texture;
			}
			// Set backpack tab state
			else {
				size_t BagIndex = Index - HUD_BACKPACK_INDEX;
				Element->Checked = BagIndex == Player->ActiveBackpack;
				if(BagIndex >= Player->Inventory->Containers[(size_t)BagType::BACKPACK].size()) {
					Element->Clickable = false;
					Element->Children.front()->Color = COLOR_GRAY;
					Element->Children.front()->Texture = ae::Assets.Textures["textures/hud/locked.png"];
				}
				else {
					Element->Children.front()->Texture = ae::Assets.Textures[Player->Inventory->GetBackpackIcon(BagIndex)];
					Element->Children.back()->Text = std::to_string(Player->Inventory->GetItemCount(BagType::BACKPACK, BagIndex));
				}
			}
		}

		// Update skill tooltip
		ae::_Element *HitElement = Elements[ELEMENT_SKILLS]->HitElement;
		if(HitElement && HitElement->Index >= 0)
			UpdateSkillTooltip(HitElement->Index, ae::Input.GetMouse());

		// Set skill button states
		for(int i = 0; i < SKILL_COUNT; i++) {
			ae::_Element *SkillButton = ae::Assets.Elements["button_skills_plus" + std::to_string(i)];
			if(!SkillButton)
				continue;

			SkillButton->Enabled = false;
			if(Player->SkillPointsRemaining && Player->Skills[i] < Stats.GetMaxSkillLevel(Player->Level) && Player->Skills[i] < Stats.GetSkillLevels())
				SkillButton->Enabled = true;
		}
	}
	else
		Menu.ShowDefaultCursor(false);

	// Update health display
	if(LastHitMaxHealth && !Player->InCombat())
		LastHitMaxHealth = 0;

	MessageTimer -= FrameTime;
	if(MessageTimer < 0.0)
		MessageTimer = 0.0;

	MessageBoxTimer -= FrameTime;
	if(MessageBoxTimer < 0.0)
		MessageBoxTimer = 0.0;

	LevelNameTimer -= FrameTime;
	if(LevelNameTimer < 0.0)
		LevelNameTimer = 0.0;
}

// Draw phase
void _HUD::Render(bool FullMap) {

	// Set labels
	if(PlayState.ShowMoreInfo()) {
		ae::Assets.Elements["label_hud_offhand_switch_key"]->Text = "";
		ae::Assets.Elements["label_hud_melee_key"]->Text = "";
	}
	else {
		ae::Assets.Elements["label_hud_offhand_switch_key"]->Text = ae::Actions.GetInputNameForAction(Action::GAME_WEAPONSWITCH);
		ae::Assets.Elements["label_hud_melee_key"]->Text = ae::Actions.GetInputNameForAction(Action::GAME_MELEE);
	}
	ae::Assets.Elements["label_hud_filters_gear_key"]->Text = ae::Actions.GetInputNameForAction(Action::GAME_FILTERGEAR);
	ae::Assets.Elements["label_hud_filters_mods_key"]->Text = ae::Actions.GetInputNameForAction(Action::GAME_FILTERMODS);
	ae::Assets.Elements["label_inventory_sort_key"]->Text = ae::Actions.GetInputNameForAction(Action::GAME_SORTINVENTORY);

	// Message
	if(MessageTimer > 0.0) {
		if(MessageTimer < 1.0)
			Elements[LABEL_MESSAGE]->SetFade(MessageTimer);

		Elements[LABEL_MESSAGE]->Render();
	}

	// Message Box
	if(MessageBoxTimer > 0.0) {
		if(MessageBoxTimer < 1.0)
			Elements[ELEMENT_MESSAGE]->SetFade(MessageBoxTimer);

		Elements[ELEMENT_MESSAGE]->Render();
	}

	// Level Name
	if(LevelNameTimer > 0.0) {
		if(LevelNameTimer < 1.0)
			Elements[LABEL_LEVELNAME]->SetFade(LevelNameTimer);

		Elements[LABEL_LEVELNAME]->Render();
	}

	// Clock
	if(!FullMap)
		Elements[ELEMENT_CLOCK]->Render();

	// Draw enemy health
	std::ostringstream Buffer;
	Buffer.imbue(std::locale(Config.Locale));
	if(LastHitMaxHealth) {
		Buffer << LastHitHealth << "/" << LastHitMaxHealth;
		Elements[LABEL_ENEMYHEALTH]->Font = LastHitFont;
		Elements[LABEL_ENEMYHEALTH]->Text = Buffer.str();
		Elements[LABEL_ENEMYNAME]->Text = LastHitName;
		Elements[IMAGE_ENEMYHEALTH]->SetWidth(Elements[ELEMENT_ENEMYINFO]->Size.x * ((float)LastHitHealth / LastHitMaxHealth));
		Elements[ELEMENT_ENEMYINFO]->Render();
		Buffer.str("");
	}

	// Draw stamina
	Elements[IMAGE_PLAYERSTAMINA]->SetWidth(Elements[ELEMENT_PLAYERSTAMINA]->Size.x * Player->GetStaminaPercentage());
	if(Player->Tired)
		Elements[IMAGE_PLAYERSTAMINA]->Color = glm::vec4(0.5f, 0.25f, 0.0f, 1.0f);
	else
		Elements[IMAGE_PLAYERSTAMINA]->Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	if(Player->GetStaminaPercentage() < 1.0f)
		Elements[ELEMENT_PLAYERSTAMINA]->Render();

	// Draw player health
	Buffer << Player->Health << "/" << Player->MaxHealth;
	Elements[LABEL_PLAYERHEALTH]->Text = Buffer.str();
	Buffer.str("");

	Elements[IMAGE_PLAYERHEALTH]->SetWidth(Elements[ELEMENT_PLAYERHEALTH]->Size.x * Player->GetHealthPercentage());
	Elements[ELEMENT_PLAYERHEALTH]->Render();

	// Draw experience bar
	float LevelPercentage = 1.0f;
	if(Player->ExperienceNeeded) {
		LevelPercentage = Player->ExperienceNextLevel > 0 ? 1.0f - (float)Player->ExperienceNeeded / Player->ExperienceNextLevel : 0;
		Buffer << Player->ExperienceNextLevel - Player->ExperienceNeeded << " / " << Player->ExperienceNextLevel << " XP";
	}

	Elements[LABEL_EXPERIENCE]->Text = Buffer.str();
	Buffer.str("");
	Elements[IMAGE_EXPERIENCE]->SetWidth(Elements[ELEMENT_EXPERIENCE]->Size.x * LevelPercentage);
	Elements[ELEMENT_EXPERIENCE]->Render();

	// Draw player name and level
	Elements[LABEL_PLAYERNAME]->Text = Player->Name;
	Buffer << "Level " << Player->Level;
	Elements[LABEL_PLAYERLEVEL]->Text = Buffer.str();
	Buffer.str("");
	Elements[LABEL_PLAYERHARDCORE]->Text = Player->Hardcore ? "Hardcore" : "";
	Elements[ELEMENT_PLAYERINFO]->Render();

	// Draw stats
	Elements[LABEL_LEVELKILLS]->Color = Kills[0] >= Kills[1] ? COLOR_GREEN : COLOR_WHITE;
	Elements[LABEL_LEVELCRATES]->Color = Crates[0] >= Crates[1] ? COLOR_GREEN : COLOR_WHITE;
	Elements[LABEL_LEVELSECRETS]->Color = Secrets[0] >= Secrets[1] ? COLOR_GREEN : COLOR_WHITE;

	Buffer << Kills[0] << "/" << Kills[1];
	Elements[LABEL_LEVELKILLS]->Text = Buffer.str();
	Buffer.str("");
	Buffer << Crates[0] << "/" << Crates[1];
	Elements[LABEL_LEVELCRATES]->Text = Buffer.str();
	Buffer.str("");
	Buffer << Secrets[0] << "/" << Secrets[1];
	Elements[LABEL_LEVELSECRETS]->Text = Buffer.str();
	Buffer.str("");
	char TimeString[256];
	FormatTime(TimeString, Player->LevelTime);
	Elements[LABEL_LEVELTIME]->Text = TimeString;
	Elements[ELEMENT_LEVELINFO]->Render();

	// Reload indicator
	if(Player->Reloading)
		DrawIndicator("Reloading", Player->GetReloadPercent(), ae::Assets.Textures["textures/hud/indicator_reload.png"]);
	else if(!Player->WeaponHasAmmo(WEAPONATTACK_MAIN) && !Player->IsSwitching() && Player->GetMainHand() && Player->GetMainHand()->Attributes.at("rounds").Float > 0.0f) {
		if(Player->HasAmmoForMain())
			DrawIndicator("Hit " + ae::Actions.GetInputNameForAction(Action::GAME_RELOAD) + " to Reload");
		else
			DrawIndicator("No ammo");
	}

	// Weapon switch indicator
	if(Player->SwitchingWeapons)
		DrawIndicator("Switching Weapons", Player->GetWeaponSwitchPercent(), ae::Assets.Textures["textures/hud/indicator_weaponswitch.png"]);
	else if(Player->SwitchingOutfits)
		DrawIndicator("Switching Outfits", Player->GetOutfitSwitchPercent(), ae::Assets.Textures["textures/hud/indicator_outfitswitch.png"]);

	// Draw weapons
	DrawHUDWeapon(Player->GetMainHand(), ae::Assets.Elements["element_hud_mainhand"], ae::Assets.Elements["image_hud_mainhand_icon"], ae::Assets.Elements["label_hud_mainhand_ammo"]);
	DrawHUDWeapon(Player->GetOffHand(), ae::Assets.Elements["element_hud_offhand"], ae::Assets.Elements["image_hud_offhand_icon"], ae::Assets.Elements["label_hud_offhand_ammo"]);
	DrawHUDWeapon(Player->GetMelee(), ae::Assets.Elements["element_hud_melee"], ae::Assets.Elements["image_hud_melee_icon"], nullptr);

	// Draw ammo amounts
	glm::vec2 AmmoSpacing = glm::vec2(0, 22) * ae::_Element::GetUIScale();
	glm::vec2 DrawPosition(25 * ae::_Element::GetUIScale(), ae::Graphics.CurrentSize.y - AmmoSpacing.y);
	for(const auto &AmmoType : Stats.AmmoNames) {
		if(Player->Ammo.find(AmmoType) == Player->Ammo.end())
			continue;

		_Ammo &Ammo = Stats.Ammo.at(AmmoType);
		const ae::_Texture *Texture = ae::Assets.Textures[Ammo.IconID];
		if(!Texture)
			continue;

		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
		ae::Graphics.DrawScaledImage(DrawPosition, Texture, UI_HUD_AMMO_SIZE);

		Buffer << Player->Ammo[AmmoType] << " / " << Player->AmmoMax[AmmoType] << "";
		Fonts[FONT_TINY]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + glm::vec2(16, 5) * ae::_Element::GetUIScale()), ae::LEFT_BASELINE);
		Buffer.str("");

		DrawPosition -= AmmoSpacing;
	}

	// Draw keys
	glm::vec2 KeySpacing = glm::vec2(0, UI_INVENTORY_ITEM_SIZE.y * 0.5f) * ae::_Element::GetUIScale();
	DrawPosition.x = (5 + UI_INVENTORY_ITEM_SIZE.x * 0.5f) * ae::_Element::GetUIScale();
	DrawPosition.y -= 50 * ae::_Element::GetUIScale();
	for(const auto &Key : Player->Keys) {
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
		ae::Graphics.DrawScaledImage(DrawPosition, ae::Assets.Textures[Stats.Objects.at(Key.first).IconID], UI_INVENTORY_ITEM_SIZE, COLOR_WHITE);

		DrawPosition -= KeySpacing;
	}

	// Draw filter settings
	ae::Assets.Elements["element_hud_filters"]->Render();

	// Draw mini map
	if(PlayState.Map && !FullMap && !InventoryOpen) {
		ae::_Bounds MinimapBounds;
		PlayState.Map->DrawMinimap(MinimapBounds, HoverItem, FullMap, PlayState.ShowMoreInfo());
	}

	// Draw inventory and character screen
	if(InventoryOpen) {
		DrawCharacterScreen();
		DrawInventory();
	}

	// Draw item tooltips
	if(HoverItem && CursorItem != HoverItem) {
		_Slot CompareSlot;
		const _Item *LeftItem = nullptr;
		const _Item *RightItem = HoverItem;
		bool ShowHelp = true;

		// Show equip help if item is on ground or in backpack
		bool ShowEquipHelp = (CursorSlot.Bag && !CursorSlot.IsGearSlot()) || (!CursorSlot.IsValidIndex() && InventoryOpen);

		// Search inventory if it's closed or the item is on the ground
		bool SearchOtherBags = !InventoryOpen || !CursorSlot.IsValidIndex();

		// Compare cursor item with hover item
		if(CursorItem) {
			ShowHelp = false;
			if(HoverItem && CursorItem->IsComparable(HoverItem)) {
				LeftItem = HoverItem;
				RightItem = CursorItem;
			}
		}
		else {

			// Holding ctrl forces a search for armor/melee
			bool Search = true;
			if(CursorSlot.Bag && (RightItem->Type == _Object::ARMOR || RightItem->IsMelee()))
				Search = ae::Input.ModKeyDown(KMOD_CTRL);

			// Search for similar item
			if(Search) {

				// Check active outfit first
				_Bag &OutfitBag = Player->GetActiveOutfitBag();
				CompareSlot.Index = OutfitBag.FindSimilarGearItem(RightItem);
				if(CompareSlot.Index != (size_t)-1)
					CompareSlot.Bag = &OutfitBag;
				else if(SearchOtherBags)
					Player->Inventory->FindSimilarGearItem(RightItem, CursorSlot.Bag, CompareSlot);
			}

			// Hold control to compare with other equipped weapon
			if(ae::Input.ModKeyDown(KMOD_CTRL) && CompareSlot.IsValidIndex() && CompareSlot.Bag->Gear && Player->GetMainHand() && Player->GetOffHand()) {
				if(CompareSlot.Index == GearType::MAINHAND)
					CompareSlot.Index = GearType::OFFHAND;
				else if(CompareSlot.Index == GearType::OFFHAND)
					CompareSlot.Index = GearType::MAINHAND;
			}

			// Couldn't find similar type, compare with equipped gear
			if(!CompareSlot.Bag)
				Player->GetEquippedCompareSlot(RightItem, ae::Input.ModKeyDown(KMOD_CTRL), CompareSlot);

			// Set left item comparison
			if(CompareSlot.IsValidIndex() && HoverItem != CompareSlot.GetItem())
				LeftItem = CompareSlot.GetItem();
		}

		// Draw left hand comparison tooltip
		if(LeftItem)
			LeftItem->DrawTooltip(Player, glm::ivec2(-100, ae::Graphics.CurrentSize.y/2), nullptr, _Slot(), ShowHelp, false);

		// Get position of tooltip
		glm::vec2 HoverPosition;
		if(CursorUseWorldPosition)
			Camera->ConvertWorldToScreen(RightItem->Position, HoverPosition);
		else
			HoverPosition = ae::Input.GetMouse();

		// Draw right hand tooltip
		RightItem->DrawTooltip(Player, HoverPosition, LeftItem, CursorSlot, ShowHelp, ShowEquipHelp);
	}

	// Draw full map
	if(PlayState.Map && FullMap) {
		ae::_Bounds MinimapBounds;
		PlayState.Map->DrawMinimap(MinimapBounds, HoverItem, FullMap, PlayState.ShowMoreInfo());

		// Draw icon hint
		if(ae::Actions.HasInputForAction(Action::GAME_MOREINFO)) {
			ae::Assets.Fonts["hud_tiny"]->DrawTextFormatted(
				"[c gray]Hold [c white]" + ae::Actions.GetInputNameForAction(Action::GAME_MOREINFO) + "[c gray] to show icons",
				glm::vec2(MinimapBounds.Start.x, MinimapBounds.End.y + 18 * ae::_Element::GetUIScale()),
				ae::LEFT_BASELINE
			);
		}

		// Draw legend
		glm::vec2 DrawPosition(MinimapBounds.Start);
		glm::vec2 LegendHalfSize = glm::vec2(8, 8) * ae::_Element::GetUIScale();
		glm::vec2 LegendOffset = glm::vec2(-14, -8) * ae::_Element::GetUIScale();
		DrawPosition.y -= 10 * ae::_Element::GetUIScale();
		DrawPosition.x -= LegendOffset.x - LegendHalfSize.x - 4 * ae::_Element::GetUIScale();
		for(const auto &Legend : MinimapLegends) {
			ae::_TextBounds TextBounds;
			ae::Assets.Fonts["hud_small"]->GetStringDimensions(Legend.Label, TextBounds);
			ae::Assets.Fonts["hud_small"]->DrawText(Legend.Label, DrawPosition, ae::LEFT_BASELINE);

			ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos"]);
			ae::Graphics.SetColor(Legend.Color);
			ae::Graphics.DrawRectangle(DrawPosition - LegendHalfSize + LegendOffset, DrawPosition + LegendHalfSize + LegendOffset, true);

			DrawPosition.x += TextBounds.Width + LegendHalfSize.x * 2 + 20 * ae::_Element::GetUIScale();
		}
	}
}

// Draws the crosshair
void _HUD::DrawCrosshair(const glm::vec2 &Position) {
	if(InventoryOpen || CrosshairScale <= 0.0f)
		return;

	ae::Graphics.SetDepthTest(false);

	glm::vec4 Color = Player->IsSteady() ? COLOR_YELLOW : COLOR_WHITE;

	ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
	ae::Graphics.SetColor(Color);
	ae::Graphics.DrawCircle(glm::vec3(Position, 0.0f), CrosshairScale);
}

// Draws a box and text
void _HUD::DrawIndicator(const std::string &String, float Percent, const ae::_Texture *Texture) {

	// Set text
	Elements[LABEL_INDICATOR]->Text = String;

	// Set progress size
	Elements[IMAGE_RELOAD]->Texture = Texture;
	Elements[IMAGE_RELOAD]->SetWidth(Elements[ELEMENT_INDICATOR]->Size.x * Percent);
	Elements[ELEMENT_INDICATOR]->Render();
}

// Draw the weapons on the HUD
void _HUD::DrawHUDWeapon(const _Item *Item, ae::_Element *Element, ae::_Element *Image, ae::_Element *Label) {
	if(!Item)
		return;

	Image->Texture = Item->Texture;
	Image->Color = Item->Color;
	int Rounds = -1;
	if(PlayState.ShowMoreInfo())
		Rounds = std::round(Item->Attributes.at("rounds").Float);
	else if(Item->Template.AmmoID.size()) {
		if(Player->Ammo.find(Item->Template.AmmoID) != Player->Ammo.end())
			Rounds = Player->Ammo.at(Item->Template.AmmoID);
		else
			Rounds = 0;
	}

	if(Label) {
		if(Rounds != -1) {

			// Set font size
			int LargestAmount = std::max(Item->Attributes.at("ammo").Int, Rounds);
			if(LargestAmount > 9999)
				Label->Font = ae::Assets.Fonts["hud_tiny"];
			else if(LargestAmount > 999)
				Label->Font = ae::Assets.Fonts["hud_small"];
			else
				Label->Font = ae::Assets.Fonts["hud_medium"];

			std::ostringstream Buffer;
			Buffer.imbue(std::locale(Config.Locale));
			Buffer << Item->Attributes.at("ammo").Int << "/" << Rounds;
			if(Label)
				Label->Text = Buffer.str();
		}
		else
			Label->Text = "";
	}

	Element->Render();

	// Highlight unique items
	if(Item->Unique) {
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
		DrawUniqueHighlight(Element->Bounds.GetCenter(), Item);
	}

	// Draw more info
	if(PlayState.ShowMoreInfo()) {
		glm::vec4 Color;
		Item->GetQualityColor(Color);
		std::ostringstream Buffer;

		// Draw item level
		Buffer << Item->Level;
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), glm::ivec2(Element->Bounds.GetCenter() + glm::vec2(0, -12) * ae::_Element::GetUIScale()), ae::CENTER_BASELINE, COLOR_GOLD);
		Buffer.str("");

		// Draw item quality
		Buffer << Item->Quality << "%";
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), glm::ivec2(Element->Bounds.GetCenter() + glm::vec2(0, 24) * ae::_Element::GetUIScale()), ae::CENTER_BASELINE, Color);
		Buffer.str("");
	}
}

// Draw character skills and stats
void _HUD::DrawCharacterScreen() {

	// Set skill labels
	std::ostringstream Buffer;
	Buffer.imbue(std::locale(Config.Locale));
	Buffer << std::setprecision(5);
	Buffer << Player->SkillPointsRemaining;
	Elements[LABEL_SKILL_REMAINING]->Text = Buffer.str();
	Buffer.str("");

	for(int i = 0; i < SKILL_COUNT; i++) {
		Buffer << Player->Skills[i];
		Elements[LABEL_SKILL0 + i]->Text = Buffer.str();
		Buffer.str("");
	}
	Elements[ELEMENT_SKILLS]->Render();

	// Draw stats
	glm::vec2 DrawPosition(ae::Graphics.CurrentSize.x - 190 * ae::_Element::GetUIScale(), 420 * ae::_Element::GetUIScale());

	// Offense
	if(Player->GetMainHand()) {
		Player->GetDamageText(Buffer, WEAPONATTACK_MAIN, PlayState.ShowMoreInfo(), 1.0);
		DrawAttribute("Damage", Buffer, DrawPosition);

		if(PlayState.ShowMoreInfo())
			Buffer << ae::Round2((Player->MinAccuracyNormal + Player->MaxAccuracyNormal) * 0.5f) << " avg";
		else
			Buffer << ae::Round2(Player->MinAccuracyNormal) << " - " << ae::Round2(Player->MaxAccuracyNormal);
		DrawAttribute("Accuracy", Buffer, DrawPosition);

		Buffer << ae::Round2(1.0 / Player->AttackPeriod[WEAPONATTACK_MAIN]) << "/s";
		DrawAttribute("Fire Rate", Buffer, DrawPosition);

		Buffer << ae::Round2(Player->Recoil) << " deg";
		DrawAttribute("Recoil", Buffer, DrawPosition);

		Buffer << Player->CritChance[WEAPONATTACK_MAIN] << "%";
		DrawAttribute("Critical Hit Chance", Buffer, DrawPosition);

		Player->GetDamageText(Buffer, WEAPONATTACK_MAIN, PlayState.ShowMoreInfo(), Player->CritDamage[WEAPONATTACK_MAIN] * 0.01);
		DrawAttribute("Critical Hit Damage", Buffer, DrawPosition);
	}

	DrawPosition.y += 10 * ae::_Element::GetUIScale();

	Player->GetDamageText(Buffer, WEAPONATTACK_MELEE, PlayState.ShowMoreInfo(), 1.0);
	DrawAttribute("Melee Damage", Buffer, DrawPosition);

	Buffer << ae::Round1(Player->AttackRange[WEAPONATTACK_MELEE]);
	DrawAttribute("Range", Buffer, DrawPosition);

	Buffer << ae::Round1(1.0 / Player->AttackPeriod[WEAPONATTACK_MELEE]) << "/s";
	DrawAttribute("Attack Speed", Buffer, DrawPosition);

	Buffer << Player->CritChance[WEAPONATTACK_MELEE] << "%";
	DrawAttribute("Melee Crit Chance", Buffer, DrawPosition);

	Player->GetDamageText(Buffer, WEAPONATTACK_MELEE, PlayState.ShowMoreInfo(), Player->CritDamage[WEAPONATTACK_MELEE] * 0.01);
	DrawAttribute("Melee Crit Damage", Buffer, DrawPosition);

	DrawPosition.y += 10 * ae::_Element::GetUIScale();

	// Defense
	Buffer << ae::Round2(Player->SelfHealDelay) << "s";
	DrawAttribute("Self Heal Delay", Buffer, DrawPosition);

	Buffer << ae::Round2(Player->SelfHealPercent) << "%";
	DrawAttribute("Self Heal Percent", Buffer, DrawPosition);

	//Buffer << Player->DamageBlock;
	//DrawAttribute("Damage Block", Buffer, DrawPosition);

	Buffer << ae::Round2(Player->SelfDamageResist) << "%";
	DrawAttribute("Self Damage Resist", Buffer, DrawPosition);

	Buffer << ae::Round2(Player->DamageResist) << "%";
	DrawAttribute("Damage Resist", Buffer, DrawPosition);

	Buffer << ae::Round2(Player->BaseMoveSpeed) << "%";
	DrawAttribute("Move Speed", Buffer, DrawPosition);

	Buffer << ae::Round2(100.0f * Player->MaxStamina) << "%";
	DrawAttribute("Stamina", Buffer, DrawPosition);

	DrawPosition.y += 10 * ae::_Element::GetUIScale();

	// Misc
	//Buffer << Player->RarityChance << "%";
	//DrawAttribute("Rarity Chance", Buffer, DrawPosition);

	Buffer << ae::Round2(100.0f * Player->ExperienceModifier) << "%";
	DrawAttribute("Experience Gain", Buffer, DrawPosition);

	Buffer << "+" << ae::Round3(Player->ExtraMods);
	DrawAttribute("Gear Mod Capacity", Buffer, DrawPosition);

	if(Player->LavaTouches > 0) {
		Buffer << Player->LavaTouches;
		DrawAttribute("Lava Touches", Buffer, DrawPosition);
	}

	Buffer << Player->TotalKills;
	DrawAttribute("Total Kills", Buffer, DrawPosition);

	Buffer << Player->TotalDeaths;
	DrawAttribute("Total Deaths", Buffer, DrawPosition);

	FormatTimeHMS(Buffer, Player->PlayTime);
	DrawAttribute("Total Play Time", Buffer, DrawPosition);

	DrawPosition.y += 10 * ae::_Element::GetUIScale();

	Buffer << Player->Progression;
	DrawAttribute("Progression", Buffer, DrawPosition);

	FormatTimeHMS(Buffer, Player->ProgressionTime);
	DrawAttribute("Progression Time", Buffer, DrawPosition);

	// Draw cursor skill
	if(CursorSkill != -1)
		Elements[ELEMENT_SKILLINFO]->Render();
}

// Draw inventory
void _HUD::DrawInventory() {

	// Draw inventory background
	Elements[ELEMENT_INVENTORY_OVERLAY]->SetActive(false);
	Elements[ELEMENT_INVENTORY]->Render();

	// Draw inventory
	ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
	DrawBag(Player->GetActiveOutfitBag(), Elements[ELEMENT_INVENTORY_OUTFIT]);
	DrawBag(Player->GetActiveBackpackBag(), Elements[ELEMENT_INVENTORY_BACKPACK]);

	// Draw highlights for compatible mod/usable types
	if(CursorItem && (CursorItem->Type == _Item::MOD || CursorItem->Type == _Item::USABLE)) {
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos"]);
		DrawBagHighlights(Player->GetActiveOutfitBag(), Elements[ELEMENT_INVENTORY_OUTFIT]);
		DrawBagHighlights(Player->GetActiveBackpackBag(), Elements[ELEMENT_INVENTORY_BACKPACK]);
	}

	// Draw more info
	if(PlayState.ShowMoreInfo()) {
		Elements[ELEMENT_INVENTORY_OVERLAY]->SetActive(true);
		Elements[ELEMENT_INVENTORY_OVERLAY]->Render();
		DrawBagInfo(Player->GetActiveOutfitBag(), Elements[ELEMENT_INVENTORY_OUTFIT]);
		DrawBagInfo(Player->GetActiveBackpackBag(), Elements[ELEMENT_INVENTORY_BACKPACK]);
		Elements[ELEMENT_INVENTORY_OVERLAY]->SetActive(false);
	}
	else {
		DrawBagModCounts(Player->GetActiveOutfitBag(), Elements[ELEMENT_INVENTORY_OUTFIT]);
		DrawBagModCounts(Player->GetActiveBackpackBag(), Elements[ELEMENT_INVENTORY_BACKPACK]);
	}

	// Draw cursor item
	if(CursorItem) {
		glm::vec2 Position(ae::Input.GetMouse());
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
		DrawInventoryItem(Position, CursorItem, CursorItem->Unique);
	}
}

// Draw a single inventory bag
void _HUD::DrawBag(const _Bag &Bag, ae::_Element *Element) {
	for(size_t i = 0; i < Bag.Slots.size(); i++) {
		const _Item *Item = Bag.Slots[i];
		if(!Item)
			continue;

		if(Item == CursorItem)
			continue;

		ae::_Element *Button = Element->Children[i];

		// Draw icon
		DrawInventoryItem(Button->Bounds.GetCenter(), Item, Item->Unique);
	}
}

// Draw item highlights when dragging a mod/usable
void _HUD::DrawBagHighlights(const _Bag &Bag, ae::_Element *Element) {
	for(size_t i = 0; i < Bag.Slots.size(); i++) {
		const _Item *Item = Bag.Slots[i];
		if(!Item || Item == CursorItem)
			continue;

		if(CursorItem->Type == _Object::MOD && !Item->CanMod())
			continue;

		if(CursorItem->Type == _Object::USABLE) {
			if(CursorItem->Template.Attributes.at("usable_type").Int == USABLE_HAMMER && !Item->CanMod())
				continue;
			else if(CursorItem->Template.Attributes.at("usable_type").Int == USABLE_WHETSTONE && !Item->CanIncreaseQuality(false))
				continue;
			else if(CursorItem->Template.Attributes.at("usable_type").Int == USABLE_WRENCH && !Item->CanIncreaseLevel(false))
				continue;
			else if(CursorItem->Template.Attributes.at("usable_type").Int == USABLE_DYNAMITE && !Item->CanDynamite())
				continue;
		}

		// Set overlay color
		if(Item->ItemCompatible(CursorItem))
			ae::Graphics.SetColor(glm::vec4(0.0f, 1.0f, 0.0f, HoverItem == Item ? 0.4f : 0.2f));
		else
			ae::Graphics.SetColor(glm::vec4(1.0f, 0.0f, 0.0f, 0.2f));

		ae::_Element *Button = Element->Children[i];
		ae::Graphics.DrawRectangle(glm::ivec2(Button->Bounds.Start), glm::ivec2(Button->Bounds.End), true);
	}
}

// Draw mod counts
void _HUD::DrawBagModCounts(const _Bag &Bag, ae::_Element *Element) {
	for(size_t i = 0; i < Bag.Slots.size(); i++) {
		const _Item *Item = Bag.Slots[i];
		if(!Item || Item == CursorItem)
			continue;

		ae::_Element *Button = Element->Children[i];
		DrawItemModCounts(Item, Button->Bounds.Start);
	}
}

// Draw extra item info
void _HUD::DrawBagInfo(const _Bag &Bag, ae::_Element *Element) {
	for(size_t i = 0; i < Bag.Slots.size(); i++) {
		const _Item *Item = Bag.Slots[i];
		if(!Item || Item == CursorItem)
			continue;

		ae::_Element *Button = Element->Children[i];
		DrawItemLevel(Item, Button->Bounds.Start);
		DrawItemQuality(Item, Button->Bounds.Start);
		DrawItemValue(Item, Button->Bounds.Start);
	}
}

// Draw character stat on character screen
void _HUD::DrawAttribute(const std::string &Label, std::ostringstream &Buffer, glm::vec2 &DrawPosition) const {
	glm::vec2 DrawOffset(10 * ae::_Element::GetUIScale(), 0);
	const char *FontID = Buffer.str().length() > 15 ? "hud_tiny" : "hud_char";
	ae::Assets.Fonts["hud_char"]->DrawText(Label, glm::ivec2(DrawPosition), ae::RIGHT_BASELINE);
	ae::Assets.Fonts[FontID]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + DrawOffset), ae::LEFT_BASELINE);
	Buffer.str("");

	DrawPosition.y += 20 * ae::_Element::GetUIScale();
}

// Draw item icon
void _HUD::DrawInventoryItem(const glm::vec2 &Position, const _Item *Item, bool Unique) {
	ae::Graphics.DrawScaledImage(Position, Item->Texture, UI_INVENTORY_ITEM_SIZE, Item->Color);

	// Highlight unique items
	if(Unique)
		DrawUniqueHighlight(Position, Item);
}

// Draw unique item's highlight
void _HUD::DrawUniqueHighlight(const glm::vec2 &Position, const _Item *Item) {
	glm::vec4 HighlightColor = Item->LightColor;
	HighlightColor.a = ITEM_HIGHLIGHT_ALPHA;
	ae::Graphics.DrawScaledImage(Position, ae::Assets.Textures["textures/lights/circle.png"], UI_INVENTORY_ITEM_SIZE * ITEM_HIGHLIGHT_SCALE, HighlightColor);
}

// Draw quick glance value attribute for item
void _HUD::DrawItemValue(const _Item *Item, const glm::vec2 &Position) {
	if(!Item)
		return;

	std::ostringstream Buffer;
	switch(Item->Type) {
		case _Object::MOD:
			if(Item->IsChangeMod()) {
				Buffer << "+" << ae::Round2(Item->Attributes.at("bonus_2nd").Float) << "%";
			}
			else {
				if(!Item->Template.Attributes.at("negative").Int)
					Buffer << "+";
				Buffer << ae::Round2(Item->Attributes.at("bonus").Float);
				if(Item->Template.Attributes.at("percent_sign").Int)
					Buffer << "%";
			}
		break;
		case _Object::WEAPON:
			Buffer << ae::Round1(Item->GetAverageDamage());
		break;
		case _Object::ARMOR:
			Buffer << Item->Attributes.at("damage_resist").Float << "%";
		break;
		case _Object::USABLE:
			if(Item->Template.Attributes.at("usable_type").Int == USABLE_WHETSTONE)
				Buffer << Item->GetWhetstoneQuality() << "%";
			else if(Item->Template.Attributes.at("usable_type").Int == USABLE_WRENCH)
				Buffer << "+" << Item->GetWrenchLevel();
		break;
		default:
			return;
		break;
	}

	Fonts[FONT_TINY]->DrawText(Buffer.str(), glm::ivec2(Position + glm::vec2(74, 74) * ae::_Element::GetUIScale()), ae::RIGHT_BASELINE, COLOR_WHITE);
}

// Draw item quality
void _HUD::DrawItemQuality(const _Item *Item, const glm::vec2 &Position) {
	if(!Item || Item->Type == _Object::USABLE)
		return;

	std::ostringstream Buffer;
	Buffer << Item->Quality << "%";
	glm::vec4 DrawColor;
	Item->GetQualityColor(DrawColor);

	float PositionY = (Item->CanMod()) ? 46 : 18;
	Fonts[FONT_TINY]->DrawText(Buffer.str(), glm::ivec2(Position + glm::vec2(74, PositionY) * ae::_Element::GetUIScale()), ae::RIGHT_BASELINE, DrawColor);
}

// Draw item level
void _HUD::DrawItemLevel(const _Item *Item, const glm::vec2 &Position) {
	if(!Item || !Item->CanLevel())
		return;

	std::ostringstream Buffer;
	Buffer << Item->Level;
	Fonts[FONT_TINY]->DrawText(Buffer.str(), glm::ivec2(Position + glm::vec2(74, 18) * ae::_Element::GetUIScale()), ae::RIGHT_BASELINE, COLOR_GOLD);
}

// Draw item mod counts
void _HUD::DrawItemModCounts(const _Item *Item, const glm::vec2 &Position) {
	if(!Item || !Item->CanMod())
		return;

	std::ostringstream Buffer;
	Buffer << Item->Mods.size() << "/" << Item->GetMaxMods(true);
	Fonts[FONT_TINY]->DrawText(Buffer.str(), glm::ivec2(Position + glm::vec2(74, 74) * ae::_Element::GetUIScale()), ae::RIGHT_BASELINE, COLOR_WHITE);
}

// Update skill tooltip information
void _HUD::UpdateSkillTooltip(int Skill, const glm::vec2 &Position) {
	CursorSkill = Skill;
	int Level = Player->Skills[Skill];

	// Update size
	Elements[ELEMENT_SKILLINFO]->BaseSize.y = Level >= Stats.GetSkillLevels() ?  230 : 320;
	Elements[ELEMENT_SKILLINFO]->CalculateBounds(true);

	// Get window position
	glm::vec2 DrawPosition(Position);
	DrawPosition -= Elements[ELEMENT_SKILLINFO]->Size + glm::vec2(15) * ae::_Element::GetUIScale();
	DrawPosition.y = std::max(DrawPosition.y, 10 * ae::_Element::GetUIScale());

	// Move window
	Elements[ELEMENT_SKILLINFO]->Offset = DrawPosition;
	Elements[ELEMENT_SKILLINFO]->CalculateBounds(false);

	// Set text
	bool Positive[2];
	std::string Prefix[2];
	std::string Plus[2];
	std::ostringstream Buffer[2];
	for(int i = 0; i < 2; i++) {
		Positive[i] = Stats.GetSkill(1, Skill, i) > 0.0f;
		Prefix[i] = Positive[i] ? "Increases " : "Decreases ";
		Plus[i] = Positive[i] ? "+" : "";
		Buffer[i] << std::setprecision(5);
		Elements[LABEL_SKILLTEXT + i]->Text = Prefix[i] + SkillText[Skill].Text[i];
	}

	// Set skill description
	std::string Percent[2] = { "% ", "% "};
	if(Skill == SKILL_INTELLIGENCE)
		Percent[1] = " ";
	Buffer[0]
		<< Plus[0] << Stats.GetSkill(Level, Skill, 0) << Percent[0] << SkillText[Skill].Text[0] << "\\n"
		<< Plus[1] << Stats.GetSkill(Level, Skill, 1) << Percent[1] << SkillText[Skill].Text[1];
	Buffer[1]
		<< Plus[0] << Stats.GetSkill(Stats.GetValidSkillLevel(Level+1), Skill, 0) << Percent[0] << SkillText[Skill].Text[0] << "\\n"
		<< Plus[1] << Stats.GetSkill(Stats.GetValidSkillLevel(Level+1), Skill, 1) << Percent[1] << SkillText[Skill].Text[1];

	// Format text
	Elements[LABEL_SKILL_LEVEL]->Text = Buffer[0].str();
	Elements[LABEL_SKILL_LEVEL]->SetWrap(Elements[ELEMENT_SKILLINFO]->Size.x);
	if(Level + 1 > Stats.GetSkillLevels())
		Buffer[1].str("");
	Elements[LABEL_SKILL_LEVEL_NEXT]->Text = Buffer[1].str();
	Elements[LABEL_SKILL_LEVEL_NEXT]->SetWrap(Elements[ELEMENT_SKILLINFO]->Size.x);

	// Handle skill caps
	ae::Assets.Elements["label_hud_skill_more"]->Text = "";
	ae::Assets.Elements["label_hud_skill_max"]->Text = "";
	ae::Assets.Elements["label_hud_skill_next"]->Text = "Next Level";
	ae::Assets.Elements["label_hud_skill_help"]->SetActive(Player->SkillPointsRemaining);
	if(Level >= Stats.GetSkillLevels()) {
		ae::Assets.Elements["label_hud_skill_next"]->Text = "";
		ae::Assets.Elements["label_hud_skill_max"]->Text = "Max Level";
		ae::Assets.Elements["label_hud_skill_help"]->SetActive(false);
	}
	else {
		int LevelRequired = 0;
		if(Player->Level <= GAME_PLAYERLEVEL_SOFTCAP && Level >= GAME_SKILL_SOFTCAP)
			LevelRequired = GAME_PLAYERLEVEL_SOFTCAP + 1;
		else if(Level >= Stats.GetMaxSkillLevel(Player->Level))
			LevelRequired = Player->Level + 1;

		if(LevelRequired) {
			ae::Assets.Elements["label_hud_skill_more"]->Text = "Player Level " + std::to_string(LevelRequired) + " Required";
			ae::Assets.Elements["label_hud_skill_help"]->SetActive(false);
		}
	}
}

// Draw death message
void _HUD::DrawDeathScreen() {
	std::ostringstream Buffer;

	glm::vec2 DrawPosition = glm::vec2(ae::Graphics.CurrentSize) * 0.5f;
	DrawPosition.y = 300 * ae::_Element::GetUIScale();
	ae::Assets.Fonts["hud_large"]->DrawText("You Died!", DrawPosition , ae::CENTER_MIDDLE);

	if(!Player->Hardcore) {
		glm::vec2 DrawPosition = glm::vec2(ae::Graphics.CurrentSize) * 0.5f;
		Buffer << "You lost [c red]" << Stats.Progressions[(size_t)Player->Progression].ExperienceLost << "%[c white] experience";
		ae::Assets.Fonts["menu_buttons"]->DrawTextFormatted(Buffer.str(), DrawPosition, ae::CENTER_MIDDLE);
		Buffer.str("");
	}

	// Show stats
	if(Player->Hardcore) {
		glm::vec2 Spacing = glm::vec2(16, 0) * ae::_Element::GetUIScale();
		DrawPosition.y = 500 * ae::_Element::GetUIScale();

		Buffer << Player->TotalKills;
		ae::Assets.Fonts["hud_medium"]->DrawText("Total Kills", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + Spacing, ae::LEFT_BASELINE);
		Buffer.str("");

		DrawPosition.y += 80 * ae::_Element::GetUIScale();

		FormatTimeHMS(Buffer, Player->PlayTime);
		ae::Assets.Fonts["hud_medium"]->DrawText("Total Play Time", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + Spacing, ae::LEFT_BASELINE);
		Buffer.str("");
	}

	DrawPosition.y = 800 * ae::_Element::GetUIScale();
	if(Player->Hardcore)
		Buffer << "Press [Escape] to quit";
	else
		Buffer << "Press [" << ae::Actions.GetInputNameForAction(Action::GAME_USE) << "] to respawn";
	ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_MIDDLE);
	Buffer.str("");
}

// Show hud message
void _HUD::ShowTextMessage(const std::string &Message, double Time, bool Override) {
	if(!Override && MessageTimer > 0)
		return;

	Elements[LABEL_MESSAGE]->Text = Message;
	Elements[LABEL_MESSAGE]->SetFade(1.0f);
	MessageTimer = Time;
}

// Show message box
void _HUD::ShowMessageBox(const std::string &Message, double Time, const glm::vec2 &Size) {
	if(Message.empty())
		return;

	Elements[ELEMENT_MESSAGE]->BaseSize = Size;
	Elements[ELEMENT_MESSAGE]->CalculateBounds();
	Elements[ELEMENT_MESSAGE]->SetFade(1.0f);

	Elements[LABEL_MESSAGEBOX]->Text = Message;
	Elements[LABEL_MESSAGEBOX]->SetWrap(Elements[ELEMENT_MESSAGE]->Size.x - 35 * ae::_Element::GetUIScale());
	MessageBoxTimer = Time;
}

// Display level name
void _HUD::ShowLevelName(const std::string &Name, double Time) {
	Elements[LABEL_LEVELNAME]->Text = Name;
	Elements[LABEL_LEVELNAME]->SetFade(1.0f);
	LevelNameTimer = Time;
}

// Format time for elapsed time
void _HUD::FormatTime(char *Buffer, double Time) {
	uint32_t Hours = Time / 3600;
	uint32_t Minutes = (uint32_t)(Time / 60) % 60;
	uint32_t Seconds = (uint32_t)(Time) % 60;
	uint32_t Centiseconds = (uint32_t)((Time - (uint32_t)(Time)) * 100);
	snprintf(Buffer, 255, "%.2d:%.2d:%.2d.%.2d", Hours, Minutes, Seconds, Centiseconds);
}

// Format time with h m s
void _HUD::FormatTimeHMS(std::ostringstream &Buffer, int64_t Time) {
	if(Time < 60)
		Buffer << Time << "s";
	else if(Time < 3600)
		Buffer << Time / 60 << "m";
	else
		Buffer << Time / 3600 << "h" << (Time / 60 % 60) << "m";
}

// Format day night clock
void _HUD::GetClockAsString(std::ostringstream &Buffer, double Clock, bool Clock24Hour) const {
	int Hours = (int)(Clock / 60.0);
	int Minutes = (int)std::fmod(Clock, 60.0);
	if(!Clock24Hour) {
		if(Hours == 0)
			Hours = 12;
		else if(Hours > 12)
			Hours -= 12;

		Buffer << Hours << ":" << std::setfill('0') << std::setw(2) << Minutes;
		if(Clock < MAP_DAY_LENGTH / 2)
			Buffer << " AM";
		else
			Buffer << " PM";
	}
	else {
		Buffer << std::setfill('0') << std::setw(2) << Hours << ":" << std::setfill('0') << std::setw(2) << Minutes;
	}
}

// Set up max stats for the level
void _HUD::SetStats(int MaxKills, int MaxCrates, int MaxSecrets) {
	Kills[0] = 0;
	Kills[1] = MaxKills;
	Crates[0] = 0;
	Crates[1] = MaxCrates;
	Secrets[0] = 0;
	Secrets[1] = MaxSecrets;
}

// Sets the last hit monster display
void _HUD::SetLastHit(_Entity *Entity) {
	if(!Entity)
		return;

	LastHitName = Entity->Name;
	LastHitHealth = Entity->Health;
	LastHitMaxHealth = Entity->MaxHealth;
	LastHitFont = (LastHitMaxHealth > 1e12) ? ae::Assets.Fonts["hud_char"] : ae::Assets.Fonts["hud_small"];
}

// Set inventory state
void _HUD::SetInventoryOpen(bool Value) {
	if(InventoryOpen == Value)
		return;

	InventoryOpen = Value;
	Elements[ELEMENT_INVENTORY]->SetActive(InventoryOpen);
	Elements[ELEMENT_SKILLS]->SetActive(InventoryOpen);

	// Reset state
	if(!InventoryOpen) {

		// Was dragging an item
		if(CursorItem)
			MoveWorldItem();

		DragSlot.Reset();
		CursorItem = nullptr;
		HoverItem = nullptr;
		CursorUseWorldPosition = false;
	}

	Menu.ShowDefaultCursor(InventoryOpen);
}

// Move item in the world to another location
void _HUD::MoveWorldItem(const glm::vec2 &DropPosition) {
	if(DragSlot.Bag || !CursorItem)
		return;

	// Update position
	if(CursorItem->Moveable) {

		// Remove item from old position
		PlayState.Map->RemoveObjectFromGrid(CursorItem, GRID_ITEM);

		// Get new position
		if(DropPosition.x < 0.0f)
			Camera->ConvertScreenToWorld(ae::Input.GetMouse(), CursorItem->Position);
		else
			CursorItem->Position = DropPosition;

		// Check drop position
		PlayState.Map->GetDropPosition(Player, PLAYER_REACH_DISTANCE, CursorItem->Position);

		// Place item in new position
		PlayState.Map->AddObjectToGrid(CursorItem, GRID_ITEM);
	}

	// Reset state
	CursorItem->SetPosition(CursorItem->Position);
	CursorItem->Visible = true;
	CursorItem = nullptr;
}

// Apply usable item to another item, return true to delete existing item
bool _HUD::ApplyUsableItem(_Item *ExistingItem) {
	if(!ExistingItem->ItemCompatible(CursorItem, false))
		return false;

	int UsableType = CursorItem->Template.Attributes.at("usable_type").Int;

	// Check type
	switch(UsableType) {
		case USABLE_HAMMER:
		case USABLE_DYNAMITE: {

			// Destroy usable item
			CursorItem->Active = false;
			HoverItem = nullptr;
			PlayState.Map->RemoveObjectFromGrid(CursorItem, GRID_ITEM);

			// Get text
			std::ostringstream Buffer;
			Buffer.imbue(std::locale(Config.Locale));
			if(UsableType == USABLE_HAMMER) {

				// Drop mods
				int QualityChange = CursorItem->GetHammerQualityChange();
				for(auto &Mod : ExistingItem->Mods) {
					Mod->Visible = true;
					Mod->Quality = std::clamp(Mod->Quality + QualityChange, ITEM_QUALITY_MIN, Stats.Progressions[(size_t)Player->Progression].MaxQuality);
					Mod->RecalculateStats();
					Mod->SetPosition(PlayState.Map->FindSuitableItemPosition(Player->Position, Mod->Type, ITEM_RADIUS, ITEM_PLACEMENT_ATTEMPTS));
					PlayState.Map->AddObject(Mod, GRID_ITEM);
				}
				ExistingItem->Mods.clear();

				Buffer << (QualityChange > 0 ? "+" : "") << QualityChange << "%";
				PlayState.GenerateTextParticle(Player->Position - glm::vec2(0.0f, 0.5f), Buffer.str());
				ae::Audio.PlaySound(ae::Assets.Sounds["game_hammer.ogg"]);
			}
			else if(UsableType == USABLE_DYNAMITE) {
				int Rolls = ExistingItem->GetTotalQuality() / CursorItem->GetDynamiteQuality();

				// Roll for drop
				for(int i = 0; i < Rolls; i++) {
					_ObjectSpawn ObjectSpawn;
					Stats.GetRandomDrop(&Stats.ItemDrops.at("dynamite"), &ObjectSpawn);
					if(!ObjectSpawn.Type)
						continue;

					ObjectSpawn.Position = PlayState.Map->FindSuitableItemPosition(Player->Position, ObjectSpawn.Type, ITEM_RADIUS, ITEM_PLACEMENT_ATTEMPTS);
					PlayState.SpawnObject(&ObjectSpawn, true);
				}

				Buffer << "+" << Rolls;
				PlayState.GenerateTextParticle(Player->Position - glm::vec2(0.0f, 0.5f), Buffer.str());
				ae::Audio.PlaySound(ae::Assets.Sounds["game_dynamite.ogg"]);
			}

			// Destroy item
			return true;
		} break;
		case USABLE_WHETSTONE:
		case USABLE_WRENCH:
			if(ExistingItem->ApplyUsable(CursorItem)) {
				CursorItem->Active = false;
				PlayState.Map->RemoveObjectFromGrid(CursorItem, GRID_ITEM);
			}
		break;
	}

	return false;
}

// Populate hit slot from hit element
void _HUD::GetHitSlot(ae::_Element *Element, _Slot &Slot) {
	if(!Element || !Element->Parent)
		return;

	if(Element->Parent->ID == "element_inventory_outfit")
		Slot.Bag = &Player->GetActiveOutfitBag();
	else if(Element->Parent->ID == "element_inventory_backpack")
		Slot.Bag = &Player->GetActiveBackpackBag();

	Slot.Index = (size_t)Element->Index;
}

// Determine if an item can be grabbed in the world
bool _HUD::CanGrabItem(const _Item *Item) {
	return !CursorItem && Item && !Item->Filtered && glm::distance2(Player->Position, Item->Position) <= PLAYER_REACH_DISTANCE_SQUARED;
}

// Get a backpack index from a tab element
size_t _HUD::GetBackpackTabIndex(const ae::_Element *Element) {
	if(!Element || !Element->Parent || Element->Parent->ID != "element_inventory_tabs" || Element->Index < HUD_BACKPACK_INDEX)
		return (size_t)-1;

	return (size_t)(Element->Index - HUD_BACKPACK_INDEX);
}
