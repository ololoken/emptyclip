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
#include <objects/item.h>
#include <ae/input.h>
#include <ae/actions.h>
#include <ae/graphics.h>
#include <ae/font.h>
#include <ae/program.h>
#include <ae/assets.h>
#include <ae/actions.h>
#include <ae/util.h>
#include <map.h>
#include <actiontype.h>
#include <config.h>
#include <stats.h>
#include <gameassets.h>
#include <sstream>
#include <iomanip>
#include <SDL_mouse.h>
#include <glm/gtc/type_ptr.hpp>

// Initialize
_HUD::_HUD(_Player *Player) :
	Player(Player) {

	LastEntityHit = nullptr;
	DragStart = nullptr;
	CursorItem = CursorOverItem = nullptr;
	CursorSkill = -1;
	CursorInventorySlot = -1;
	CrosshairScale = 0.0f;
	MessageTimer = 0.0;
	MessageBoxTimer = 0.0;
	InventoryOpen = false;

	// Get textures
	Fonts[FONT_TINY] = ae::Assets.Fonts["hud_tiny"];
	Fonts[FONT_SMALL] = ae::Assets.Fonts["hud_small"];
	Fonts[FONT_MEDIUM] = ae::Assets.Fonts["hud_medium"];
	Fonts[FONT_LARGE] = ae::Assets.Fonts["hud_large"];
	Fonts[FONT_LARGER] = ae::Assets.Fonts["hud_larger"];
	Fonts[FONT_LARGEST] = ae::Assets.Fonts["hud_largest"];
	CrosshairTexture = ae::Assets.Textures["textures/hud/crosshair0.png"];
	ReloadTexture = ae::Assets.Textures["textures/hud/reload0.png"];
	WeaponSwitchTexture = ae::Assets.Textures["textures/hud/weaponswitch0.png"];

	// Elements
	Elements[LABEL_MESSAGE] = ae::Assets.Elements["label_hud_message"];
	Elements[LABEL_MESSAGEBOX] = ae::Assets.Elements["label_hud_messagebox_text"];

	Elements[LABEL_MESSAGE]->SetActive(true);
	Elements[LABEL_MESSAGEBOX]->SetActive(true);

	Elements[ELEMENT_PLAYERINFO] = ae::Assets.Elements["element_hud_player_info"];
	Elements[LABEL_PLAYERNAME] = ae::Assets.Elements["label_hud_player_name"];
	Elements[LABEL_PLAYERLEVEL] = ae::Assets.Elements["label_hud_player_level"];
	Elements[LABEL_PLAYERHEALTH] = ae::Assets.Elements["label_hud_player_health"];
	Elements[ELEMENT_PLAYERINFO]->SetActive(true);

	Elements[ELEMENT_ENEMYINFO] = ae::Assets.Elements["element_hud_enemy_info"];
	Elements[LABEL_ENEMYNAME] = ae::Assets.Elements["label_hud_enemy_name"];
	Elements[ELEMENT_ENEMYINFO]->SetActive(true);

	Elements[ELEMENT_PLAYERHEALTH] = ae::Assets.Elements["element_hud_player_health"];
	Elements[IMAGE_PLAYERHEALTH] = ae::Assets.Elements["image_player_health_full"];
	Elements[LABEL_PLAYERHEALTH] = ae::Assets.Elements["label_hud_player_health_text"];
	Elements[ELEMENT_PLAYERHEALTH]->SetActive(true);

	Elements[ELEMENT_PLAYERSTAMINA] = ae::Assets.Elements["element_hud_player_stamina"];
	Elements[IMAGE_PLAYERSTAMINA] = ae::Assets.Elements["image_player_stamina_full"];
	Elements[ELEMENT_PLAYERSTAMINA]->SetActive(true);

	Elements[LABEL_ENEMYHEALTH] = ae::Assets.Elements["label_hud_enemy_health_text"];
	Elements[IMAGE_ENEMYHEALTH] = ae::Assets.Elements["image_enemy_health_full"];
	Elements[IMAGE_ENEMYHEALTH]->SetActive(true);

	Elements[ELEMENT_INDICATOR] = ae::Assets.Elements["element_hud_indicator"];
	Elements[IMAGE_RELOAD] = ae::Assets.Elements["image_indicator_progress"];
	Elements[LABEL_INDICATOR] = ae::Assets.Elements["label_hud_indicator_text"];
	Elements[ELEMENT_INDICATOR]->SetActive(true);

	Elements[ELEMENT_EXPERIENCE] = ae::Assets.Elements["element_hud_experience"];
	Elements[IMAGE_EXPERIENCE] = ae::Assets.Elements["image_experience_bar_full"];
	Elements[LABEL_EXPERIENCE] = ae::Assets.Elements["label_hud_experience_text"];
	Elements[ELEMENT_EXPERIENCE]->SetActive(true);

	ae::Assets.Elements["element_hud_mainhand"]->SetActive(true);
	ae::Assets.Elements["element_hud_offhand"]->SetActive(true);
	ae::Assets.Elements["element_hud_melee"]->SetActive(true);

	Elements[ELEMENT_INVENTORY] = ae::Assets.Elements["element_inventory"];
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

// Shut down
_HUD::~_HUD() {
}

// Sets the last entity hit object
void _HUD::SetLastEntityHit(_Entity *Entity) {
	LastEntityHit = Entity;
	LastEntityHitTimer = 0;
}

// Set inventory state
void _HUD::SetInventoryOpen(bool Value) {
	if(InventoryOpen == Value)
		return;

	InventoryOpen = Value;
	if(InventoryOpen) {
		Elements[ELEMENT_INVENTORY]->SetActive(true);
		Elements[ELEMENT_SKILLS]->SetActive(true);
	}
	else {
		Elements[ELEMENT_INVENTORY]->SetActive(false);
		Elements[ELEMENT_SKILLS]->SetActive(false);
		DragStart = nullptr;
		CursorItem = CursorOverItem = nullptr;
	}

	ae::Graphics.SetCursor(InventoryOpen);
}

// Handle mouse events
void _HUD::MouseEvent(const ae::_MouseEvent &MouseEvent) {
	if(!GetInventoryOpen())
		return;

	ae::_Element *HitElement = Elements[ELEMENT_INVENTORY]->HitElement;
	if(MouseEvent.Button == SDL_BUTTON_LEFT) {

		// Start dragging an item
		if(MouseEvent.Pressed) {
			if(HitElement && HitElement->Index >= 0 && Player->CanDropItem()) {
				DragStart = HitElement;
				CursorItem = Player->Inventory[DragStart->Index];
				ClickOffset = glm::vec2(MouseEvent.Position) - HitElement->Bounds.GetCenter();
			}
		}
		else {

			// Was dragging an item
			if(CursorItem) {

				// Dropped outside the inventory
				if(!HitElement) {
					Player->DropItem(DragStart->Index);
				}
				else if(HitElement->Index >= 0) {
					Player->SwapInventory(DragStart->Index, HitElement->Index);
				}
			}

			// Swap inventory
			CursorItem = nullptr;
			DragStart = nullptr;
		}
	}
	else if(MouseEvent.Button == SDL_BUTTON_RIGHT) {
		if(MouseEvent.Pressed) {
			if(HitElement && HitElement->Index >= 0) {
				const _Item *Item = Player->Inventory[HitElement->Index];
				if(Item) {
					switch(Item->Type) {
						case _Object::WEAPON: {
							if(Item->IsMelee())
								Player->SwapInventory(HitElement->Index, INVENTORY_MELEE);
							else
								Player->SwapInventory(HitElement->Index, ae::Input.ModKeyDown(KMOD_CTRL) ? INVENTORY_OFFHAND : INVENTORY_MAINHAND);
						} break;
						case _Object::ARMOR:
							Player->SwapInventory(HitElement->Index, INVENTORY_ARMOR);
						break;
						case _Object::MEDKIT:
							Player->UseMedkit(HitElement->Index);
						break;
					}
				}

				if(!Player->HasInventory(HitElement->Index))
					CursorOverItem = nullptr;
			}
		}
	}
	else if(MouseEvent.Button == SDL_BUTTON_MIDDLE) {
		if(MouseEvent.Pressed) {
			if(HitElement && HitElement->Index >= 0) {
				Player->DropItem(HitElement->Index);
			}
		}
	}

	HitElement = Elements[ELEMENT_SKILLS]->HitElement;
	if(MouseEvent.Pressed && MouseEvent.Button == SDL_BUTTON_LEFT) {
		if(HitElement && HitElement->Index >= 0) {
			Player->UpdateSkill(HitElement->Index, 1);
		}
	}
}

// Update phase
void _HUD::Update(double FrameTime, float Radius) {
	LastEntityHitTimer += FrameTime;
	CursorOverItem = nullptr;
	CursorInventorySlot = -1;
	CursorSkill = -1;

	// Update crosshair
	CrosshairScale += (Radius - CrosshairScale) / HUD_CROSSHAIRDIVISOR;
	if(CrosshairScale < HUD_MINCROSSHAIRSCALE)
		CrosshairScale = HUD_MINCROSSHAIRSCALE;

	// Update inventory
	if(GetInventoryOpen()) {
		ae::Graphics.SetCursor(true);

		ae::_Element *HitElement;
		HitElement = Elements[ELEMENT_INVENTORY]->HitElement;
		if(HitElement && HitElement->Index >= 0) {
			CursorOverItem = Player->Inventory[HitElement->Index];
			CursorInventorySlot = HitElement->Index;
		}

		HitElement = Elements[ELEMENT_SKILLS]->HitElement;
		if(HitElement && HitElement->Index >= 0)
			UpdateSkillInfo(HitElement->Index, ae::Input.GetMouse().x, ae::Input.GetMouse().y);
	}
	else
		ae::Graphics.SetCursor(false);

	// Update health display
	if(LastEntityHit != nullptr && (LastEntityHitTimer > HUD_ENTITYHEALTHDISPLAYPERIOD || !LastEntityHit->Active))
		LastEntityHit = nullptr;

	if(MessageTimer > 0.0)
		MessageTimer -= FrameTime;

	if(MessageBoxTimer > 0.0)
		MessageBoxTimer -= FrameTime;
}

// Draw phase
void _HUD::Render(bool FullMap) {

	// Set labels
	ae::Assets.Elements["label_hud_offhand_switch_key"]->Text = ae::Actions.GetInputNameForAction(Action::GAME_WEAPONSWITCH);
	ae::Assets.Elements["label_hud_melee_key"]->Text = ae::Actions.GetInputNameForAction(Action::GAME_MELEE);

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

	// Draw enemy health
	std::ostringstream Buffer;
	if(LastEntityHit != nullptr) {
		Buffer << LastEntityHit->Health << "/" << LastEntityHit->MaxHealth;
		Elements[LABEL_ENEMYHEALTH]->Text = Buffer.str();
		Elements[LABEL_ENEMYNAME]->Text = LastEntityHit->Name;
		Elements[IMAGE_ENEMYHEALTH]->SetWidth(Elements[ELEMENT_ENEMYINFO]->Size.x * LastEntityHit->GetHealthPercentage());
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
	else
		Buffer.str("");
	Elements[LABEL_EXPERIENCE]->Text = Buffer.str();
	Buffer.str("");
	Elements[IMAGE_EXPERIENCE]->SetWidth(Elements[ELEMENT_EXPERIENCE]->Size.x * LevelPercentage);
	Elements[ELEMENT_EXPERIENCE]->Render();

	// Draw player name and level
	Elements[LABEL_PLAYERNAME]->Text = Player->Name;
	Buffer << "Level " << Player->Level;
	Elements[LABEL_PLAYERLEVEL]->Text = Buffer.str();
	Buffer.str("");
	Elements[ELEMENT_PLAYERINFO]->Render();

	// Reload indicator
	if(Player->Reloading)
		DrawIndicator("Reloading", Player->GetReloadPercent(), ReloadTexture);
	else if(!Player->WeaponHasAmmo(WEAPONATTACK_MAIN) && !Player->SwitchingWeapons && Player->GetMainHand() && Player->GetMainHand()->Attributes.at("rounds").Int > 0) {
		if(Player->HasAmmoForMain())
			DrawIndicator("Hit " + ae::Actions.GetInputNameForAction(Action::GAME_RELOAD) + " to Reload");
		else
			DrawIndicator("No ammo");
	}

	// Weapon switch indicator
	if(Player->SwitchingWeapons)
		DrawIndicator("Switching Weapons", Player->GetWeaponSwitchPercent(), WeaponSwitchTexture);

	// Draw weapons
	DrawHUDWeapon(Player->GetMainHand(), ae::Assets.Elements["element_hud_mainhand"], ae::Assets.Elements["image_mainhand_icon"], ae::Assets.Elements["label_hud_mainhand_ammo"]);
	DrawHUDWeapon(Player->GetOffHand(), ae::Assets.Elements["element_hud_offhand"], ae::Assets.Elements["image_offhand_icon"], ae::Assets.Elements["label_hud_offhand_ammo"]);
	DrawHUDWeapon(Player->GetMelee(), ae::Assets.Elements["element_hud_melee"], ae::Assets.Elements["image_melee_icon"], nullptr);

	// Draw ammo amounts
	glm::vec2 AmmoSpacing = glm::vec2(0, 22) * ae::_Element::GetUIScale();
	glm::vec2 DrawPosition(15 * ae::_Element::GetUIScale(), ae::Graphics.CurrentSize.y - AmmoSpacing.y);
	for(const auto &AmmoType : Stats.AmmoNames) {
		if(Player->Ammo.find(AmmoType) == Player->Ammo.end())
			continue;

		_ObjectTemplate &Ammo = Stats.Objects.at(AmmoType);
		const ae::_Texture *Texture = ae::Assets.Textures[Ammo.IconID];
		if(!Texture)
			continue;

		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
		ae::Graphics.DrawScaledImage(DrawPosition, Texture, UI_HUD_AMMO_SIZE);

		Buffer << Player->Ammo[AmmoType] << " / " << Player->AmmoMax[AmmoType] << "";
		Fonts[FONT_SMALL]->DrawText(Buffer.str(), DrawPosition + glm::vec2(16, 5) * ae::_Element::GetUIScale(), ae::LEFT_BASELINE);
		Buffer.str("");

		DrawPosition -= AmmoSpacing;
	}

	// Draw keys
	glm::vec2 KeySpacing = glm::vec2(0, UI_INVENTORY_ITEM_SIZE.y * 0.5f) * ae::_Element::GetUIScale();
	DrawPosition.x = UI_INVENTORY_ITEM_SIZE.x * 0.5f * ae::_Element::GetUIScale();
	DrawPosition.y -= 50 * ae::_Element::GetUIScale();
	for(const auto &Key : Player->Keys) {
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
		ae::Graphics.DrawScaledImage(DrawPosition, ae::Assets.Textures[Stats.Objects.at(Key.first).IconID], UI_INVENTORY_ITEM_SIZE, COLOR_WHITE);

		DrawPosition -= KeySpacing;
	}

	// Draw mini map
	if(Player->Map)
		Player->Map->DrawMinimap(FullMap);

	// Draw character screen
	DrawCharacterScreen();

	// Draw item tooltip
	if(CursorOverItem && CursorItem != CursorOverItem) {
		size_t CompareSlot = (size_t)-1;

		// Compare with equipment
		if(CursorInventorySlot == -1 || CursorInventorySlot >= INVENTORY_BAGSTART) {
			if(CursorOverItem->Type == _Object::WEAPON) {
				if(CursorOverItem->IsMelee()) {
					if(Player->GetMelee()) {
						Player->GetMelee()->DrawTooltip(Player, size_t(-1), -1, glm::ivec2(-100, ae::Graphics.CurrentSize.y/2));
						CompareSlot = INVENTORY_MELEE;
					}
				}
				else {
					_Item *CompareWeapon = Player->GetOffHand();
					CompareSlot = INVENTORY_OFFHAND;
					if(Player->GetMainHand() && Player->GetMainHand()->Attributes.at("weapon_type").Int == CursorOverItem->Attributes.at("weapon_type").Int) {
						CompareWeapon = Player->GetMainHand();
						CompareSlot = INVENTORY_MAINHAND;
					}

					if(CompareWeapon)
						CompareWeapon->DrawTooltip(Player, size_t(-1), -1, glm::ivec2(-100, ae::Graphics.CurrentSize.y/2));
				}
			}
			else if(CursorOverItem->Type == _Object::ARMOR && Player->GetArmor()) {
				Player->GetArmor()->DrawTooltip(Player, size_t(-1), -1, glm::ivec2(-100, ae::Graphics.CurrentSize.y/2));
				CompareSlot = INVENTORY_ARMOR;
			}
		}

		CursorOverItem->DrawTooltip(Player, CompareSlot, CursorInventorySlot, ae::Input.GetMouse());
	}
}

// Draws the crosshair
void _HUD::RenderCrosshair(const glm::vec2 &Position) {
	if(InventoryOpen)
		return;

	ae::Graphics.SetDepthTest(false);

	ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
	ae::Graphics.SetColor(COLOR_WHITE);
	ae::Graphics.DrawCircle(glm::vec3(Position, 0.0f), CrosshairScale);

	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
	ae::Graphics.SetColor(COLOR_WHITE);
	ae::Assets.Programs["pos_uv"]->ResetTextureTransform();
	ae::Graphics.DrawSprite(glm::vec3(Position, 0.0f), CrosshairTexture, 0);
}

// Draws a box and text
void _HUD::DrawIndicator(const std::string &String, float Percent, const ae::_Texture *Texture) {

	// Set text
	Elements[LABEL_INDICATOR]->Text = String;
	ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos"]);
	ae::Graphics.SetColor(COLOR_TGRAY);
	ae::Graphics.DrawRectangle(Elements[ELEMENT_INDICATOR]->Bounds);

	// Set progress size
	Elements[IMAGE_RELOAD]->Texture = Texture;
	Elements[IMAGE_RELOAD]->SetWidth(Elements[ELEMENT_INDICATOR]->Size.x * Percent);
	Elements[ELEMENT_INDICATOR]->Render();
}

// Draw the weapons on the HUD
void _HUD::DrawHUDWeapon(const _Item *Weapon, ae::_Element *Element, ae::_Element *Image, ae::_Element *Label) {
	if(!Weapon)
		return;

	Image->Texture = Weapon->Texture;
	Image->Color = Weapon->Color;
	if(Weapon->Attributes.at("rounds").Int) {
		std::ostringstream Buffer;
		Buffer << Weapon->Attributes.at("ammo").Int << "/" << Weapon->Attributes.at("rounds").Int;
		if(Label)
			Label->Text = Buffer.str();
	}
	else if(Label)
		Label->Text = "";

	Element->Render();
}

// Draw the inventory and character screen
void _HUD::DrawCharacterScreen() {
	if(!InventoryOpen)
		return;

	// Draw the inventory background
	Elements[ELEMENT_INVENTORY]->Render();

	// Set skill labels
	std::ostringstream Buffer;
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
	glm::vec2 DrawPosition(ae::Graphics.CurrentSize.x - 160, ae::Graphics.CurrentSize.y/2 + 40);

	// Offense
	if(Player->HasMainHand()) {
		Buffer << Player->MinDamage[WEAPONATTACK_MAIN] << " - " << Player->MaxDamage[WEAPONATTACK_MAIN];
		DrawAttribute("Damage", Buffer, DrawPosition);

		Buffer << ae::Round1(Player->MinAccuracyNormal) << " - " << ae::Round1(Player->MaxAccuracyNormal);
		DrawAttribute("Accuracy", Buffer, DrawPosition);

		Buffer << ae::Round1(1.0 / Player->FirePeriod[WEAPONATTACK_MAIN]) << "/s";
		DrawAttribute("Fire Rate", Buffer, DrawPosition);
	}

	DrawPosition.y += 20;

	Buffer << Player->MinDamage[WEAPONATTACK_MELEE] << " - " << Player->MaxDamage[WEAPONATTACK_MELEE];
	DrawAttribute("Melee Damage", Buffer, DrawPosition);

	Buffer << ae::Round1(1.0 / Player->FirePeriod[WEAPONATTACK_MELEE]) << "/s";
	DrawAttribute("Attack Speed", Buffer, DrawPosition);

	DrawPosition.y += 20;

	// Defense
	Buffer << Player->DamageBlock;
	DrawAttribute("Damage Block", Buffer, DrawPosition);

	Buffer << Player->DamageResist << "%";
	DrawAttribute("Damage Resist", Buffer, DrawPosition);

	Buffer << int(100 * Player->MovementSpeed / PLAYER_MOVEMENTSPEED + 0.5f) << "%";
	DrawAttribute("Move Speed", Buffer, DrawPosition);

	Buffer << int(100 * Player->MaxStamina + 0.5f) << "%";
	DrawAttribute("Stamina", Buffer, DrawPosition);

	DrawPosition.y += 20;

	// Misc
	Buffer << Player->DropRate << "%";
	DrawAttribute("Drop Rate", Buffer, DrawPosition);

	Buffer << Player->MonsterKills;
	DrawAttribute("Kills", Buffer, DrawPosition);

	// Draw inventory
	for(int i = INVENTORY_MAINHAND; i < INVENTORY_BAGEND; i++) {
		if(!Player->HasInventory(i) || Player->Inventory[i] == CursorItem)
			continue;

		ae::_Element *Button = Elements[ELEMENT_INVENTORY]->Children[i];
		if(!Button)
			continue;

		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
		ae::Graphics.DrawScaledImage(Button->Bounds.GetCenter(), Player->Inventory[i]->Texture, UI_INVENTORY_ITEM_SIZE, Player->Inventory[i]->Color);
		if(i >= INVENTORY_BAGSTART && Player->Inventory[i]->CanStack())
			DrawItemCount(Player->Inventory[i], Button->Bounds.End.x - 2, Button->Bounds.End.y - 2);
	}

	// Draw cursor item
	if(CursorItem) {
		glm::ivec2 Position(ae::Input.GetMouse() - ClickOffset);
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
		ae::Graphics.DrawScaledImage(Position, CursorItem->Texture, UI_INVENTORY_ITEM_SIZE, CursorItem->Color);
		if(CursorItem->CanStack()) {
			ae::_Element *Button = Elements[ELEMENT_INVENTORY]->Children[DragStart->Index];
			DrawItemCount(CursorItem, Position.x + Button->BaseSize.x/2 - 2, Position.y + Button->BaseSize.y/2 - 2);
		}
	}

	// Draw cursor skill
	if(CursorSkill != -1)
		Elements[ELEMENT_SKILLINFO]->Render();
}

// Draw character stat on character screen
void _HUD::DrawAttribute(const std::string &Label, std::ostringstream &Buffer, glm::vec2 &DrawPosition) const {
	glm::vec2 DrawOffset(10, 0);
	ae::Assets.Fonts["hud_medium"]->DrawText(Label, DrawPosition, ae::RIGHT_BASELINE);
	ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + DrawOffset, ae::LEFT_BASELINE);
	Buffer.str("");

	DrawPosition.y += 20;
}

// Draw the item count text
void _HUD::DrawItemCount(_Item *Item, int X, int Y) {
	std::ostringstream Buffer;
	Buffer << Item->Count;
	Fonts[FONT_TINY]->DrawText(Buffer.str(), glm::vec2(X, Y), ae::RIGHT_BASELINE, COLOR_WHITE);
	Buffer.str("");
}

// Draw the skill popup window
void _HUD::UpdateSkillInfo(int Skill, int DrawX, int DrawY) {
	CursorSkill = Skill;

	DrawX -= Elements[ELEMENT_SKILLINFO]->Size.x + 15;
	DrawY -= Elements[ELEMENT_SKILLINFO]->Size.y + 15;
	if(DrawX < 10)
		DrawX = 10;
	if(DrawY < 10)
		DrawY = 10;

	// Move window
	Elements[ELEMENT_SKILLINFO]->Offset = glm::ivec2(DrawX, DrawY);
	Elements[ELEMENT_SKILLINFO]->CalculateBounds(false);

	// Get skill description
	std::ostringstream Buffer, BufferNext;
	Buffer << std::setprecision(3);
	BufferNext << std::setprecision(3);
	int Level = Player->Skills[Skill];
	Elements[LABEL_SKILLTEXTALT]->Text = "";
	switch(Skill) {
		case SKILL_STRENGTH:
			Elements[LABEL_SKILLTEXT]->Text = "Increases Melee Damage";
			Buffer << "+" << Stats.GetSkill(Level, Skill) << "%";
			BufferNext << "+" << Stats.GetSkill(Stats.GetValidSkillLevel(Player->Skills[Skill]+1), Skill) << "%";
		break;
		case SKILL_DEXTERITY:
			Elements[LABEL_SKILLTEXT]->Text = "Increases Reload Speed";
			Elements[LABEL_SKILLTEXTALT]->Text = "Increases Weapon Switch Speed";
			Buffer << "+" << Stats.GetSkill(Level, Skill) << "%";
			BufferNext << "+" << Stats.GetSkill(Stats.GetValidSkillLevel(Level+1), Skill) << "%";
		break;
		case SKILL_FORTITUDE:
			Elements[LABEL_SKILLTEXT]->Text = "Increases Damage Resist";
			Buffer << "+" << Stats.GetSkill(Level, Skill) << "%";
			BufferNext << "+" << Stats.GetSkill(Stats.GetValidSkillLevel(Level+1), Skill) << "%";
		break;
		case SKILL_VITALITY:
			Elements[LABEL_SKILLTEXT]->Text = "Increases Max Health";
			Buffer << "+" << Stats.GetSkill(Level, Skill) << "%";
			BufferNext << "+" << Stats.GetSkill(Stats.GetValidSkillLevel(Level+1), Skill) << "%";
		break;
		case SKILL_AGILITY:
			Elements[LABEL_SKILLTEXT]->Text = "Increases Attack Speed";
			Elements[LABEL_SKILLTEXTALT]->Text = "Increases Fire Rate";
			Buffer << "+" << Stats.GetSkill(Level, Skill) << "%";
			BufferNext << "+" << Stats.GetSkill(Stats.GetValidSkillLevel(Level+1), Skill) << "%";
		break;
		case SKILL_CUNNING:
			Elements[LABEL_SKILLTEXT]->Text = "Increases Move Speed";
			Buffer << "+" << Stats.GetSkill(Level, Skill) << "%";
			BufferNext << "+" << Stats.GetSkill(Stats.GetValidSkillLevel(Level+1), Skill) << "%";
		break;
		case SKILL_ENDURANCE:
			Elements[LABEL_SKILLTEXT]->Text = "Increases Max Stamina";
			Buffer << "+" << Stats.GetSkill(Level, Skill) << "%";
			BufferNext << "+" << Stats.GetSkill(Stats.GetValidSkillLevel(Level+1), Skill) << "%";
		break;
		case SKILL_PERCEPTION:
			Elements[LABEL_SKILLTEXT]->Text = "Increases Gun Accuracy";
			Buffer << "+" << Stats.GetSkill(Level, Skill) << "%";
			BufferNext << "+" << Stats.GetSkill(Stats.GetValidSkillLevel(Level+1), Skill) << "%";
		break;
		case SKILL_LUCK: {
			int Bonus = Stats.GetSkill(Level, Skill);
			int BonusNext = Stats.GetSkill(Stats.GetValidSkillLevel(Level+1), Skill);
			Elements[LABEL_SKILLTEXT]->Text = "Increases Drop Rate";
			Elements[LABEL_SKILLTEXTALT]->Text = "Increases Ammo Pickup Bonus";
			Buffer << "+" << Bonus << "% Drop Rate / +" << Bonus * PLAYER_AMMO_LUCK_MULTIPLIER << "% Ammo";
			BufferNext << "+" << BonusNext << "% Drop Rate / +" << BonusNext * PLAYER_AMMO_LUCK_MULTIPLIER << "% Ammo";
		} break;
	}

	// Wrap text
	Elements[LABEL_SKILLTEXT]->SetWrap(Elements[ELEMENT_SKILLINFO]->Size.x - 20);

	Elements[LABEL_SKILL_LEVEL]->Text = Buffer.str();
	if(Player->Skills[Skill]+1 > GAME_SKILLLEVELS)
		BufferNext.str("");
	Elements[LABEL_SKILL_LEVEL_NEXT]->Text = BufferNext.str();
}

// Draw death message
void _HUD::RenderDeathScreen() {
	Fonts[FONT_LARGEST]->DrawText("You Died!", glm::vec2(ae::Graphics.CurrentSize.x / 2, ae::Graphics.CurrentSize.y / 2 - 200), ae::CENTER_MIDDLE);
	Fonts[FONT_LARGE]->DrawText(std::string("Press [") + ae::Actions.GetInputNameForAction(Action::GAME_USE) + "] to continue", glm::vec2(ae::Graphics.CurrentSize.x / 2, ae::Graphics.CurrentSize.y / 2 - 150), ae::CENTER_MIDDLE);
}

// Show hud message
void _HUD::ShowTextMessage(const std::string &Message, double Time) {
	Elements[LABEL_MESSAGE]->Text = Message;
	Elements[LABEL_MESSAGE]->SetFade(1.0f);
	MessageTimer = Time;
}

// Show message box
void _HUD::ShowMessageBox(const std::string &Message, double Time) {
	if(Message == "")
		return;

	if(MessageBoxTimer > 0.0 && Elements[LABEL_MESSAGEBOX]->Text == Message)
		return;

	Elements[LABEL_MESSAGEBOX]->Text = Message;
	Elements[LABEL_MESSAGEBOX]->SetWrap(Elements[ELEMENT_MESSAGE]->Size.x - 25);

	Elements[ELEMENT_MESSAGE]->SetFade(1.0f);
	MessageBoxTimer = Time;
}
