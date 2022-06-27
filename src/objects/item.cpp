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
#include <objects/item.h>
#include <objects/weapon.h>
#include <objects/player.h>
#include <ae/buffer.h>
#include <ae/texture.h>
#include <ae/graphics.h>
#include <ae/font.h>
#include <ae/assets.h>
#include <ae/util.h>
#include <constants.h>
#include <stats.h>
#include <sstream>
#include <algorithm>
#include <iomanip>

// Constructor
_Item::_Item(const std::unordered_map<std::string, _Value> &TemplateAttributes) :
	TemplateAttributes(TemplateAttributes),
	Level(1),
	Quality(0),
	Count(0) {

	Texture = nullptr;
	PositionZ = ITEM_Z;
}

// Set two range attributes given a level, spread and multiplier
void _Item::SetAttributeRange(const std::string &AttributeName, int ItemLevel, float Multiplier) {
	float LevelValue = ItemLevel > 0 ? TemplateAttributes.at(AttributeName + "_level").Float * ItemLevel : 0;
	int Value = std::ceil((TemplateAttributes.at(AttributeName).Float + LevelValue) * Multiplier);
	int ValueRange = std::ceil(Value * TemplateAttributes.at(AttributeName + "_spread").Float);
	Attributes["min_" + AttributeName].Int = Value - ValueRange;
	Attributes["max_" + AttributeName].Int = Value + ValueRange;
}

// Set an attribute given a level and multiplier
void _Item::SetAttributeLevel(const std::string &AttributeName, int ItemLevel, float Multiplier) {
	float LevelValue = ItemLevel > 0 ? TemplateAttributes.at(AttributeName + "_level").Float * ItemLevel : 0;
	Attributes[AttributeName].Int = std::ceil((TemplateAttributes.at(AttributeName).Float + LevelValue) * Multiplier);
}

// Serialize for saving
void _Item::Serialize(ae::_Buffer &Buffer) {
	Buffer.WriteString(ID.c_str());
	Buffer.Write<int>(Level);
	Buffer.Write<int>(Quality);
}

// Draw the item popup window
void _Item::DrawTooltip(const _Player *Player, std::size_t CompareSlot, glm::ivec2 DrawPosition) {
	std::ostringstream Buffer;

	glm::ivec2 Size(300, 120);
	if(Type == _Object::WEAPON)
		Size.y = 400;
	else if(Type == _Object::ARMOR)
		Size.y = 320;
	else if(Type == _Object::UPGRADE)
		Size.y = 170;

	// Get title width
	ae::_TextBounds TextBounds;
	ae::Assets.Fonts["hud_large"]->GetStringDimensions(Name, TextBounds);
	Size.x = std::max(Size.x, TextBounds.Width) + 20;

	// Offset position
	int WindowOffsetX = 20;
	int MinX = 0;
	DrawPosition.x += WindowOffsetX;
	DrawPosition.y -= Size.y/2;

	// Get current equipment
	_Weapon *EquippedWeapon = nullptr;
	_Item *EquippedArmor = nullptr;
	if(CompareSlot < INVENTORY_SIZE) {
		EquippedWeapon = (_Weapon *)Player->Inventory[CompareSlot];
		EquippedArmor = Player->Inventory[CompareSlot];

		MinX = Size.x;
	}

	// Clamp position of window
	DrawPosition.x = std::clamp(DrawPosition.x, MinX, ae::Graphics.CurrentSize.x - Size.x);
	DrawPosition.y = std::clamp(DrawPosition.y, 0, ae::Graphics.CurrentSize.y - Size.y);

	// Draw background
	ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos"]);
	ae::Graphics.SetColor(glm::vec4(0, 0, 0, 0.8f));
	ae::Graphics.DrawRectangle(glm::vec2(DrawPosition.x, DrawPosition.y), DrawPosition + Size, true);

	// Draw name
	DrawPosition.y += 25;
	DrawPosition.x += Size.x/2;
	ae::Assets.Fonts["hud_large"]->DrawText(Name, DrawPosition, ae::CENTER_BASELINE);

	// Draw type
	DrawPosition.y += 18;
	ae::Assets.Fonts["hud_small"]->DrawText(GetTypeAsString(), DrawPosition, ae::CENTER_BASELINE);

	// Draw Level
	if(Type != _Object::KEY) {
		DrawPosition.y += 16;
		Buffer << "Level " << Level;
		ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE);
		Buffer.str("");
	}

	DrawPosition.y += 10;
	glm::ivec2 DrawOffset(8, 0);

	// Quality
	glm::vec4 TextColor = COLOR_WHITE;
	if(Type == _Object::WEAPON || Type == _Object::ARMOR || Type == _Object::UPGRADE) {
		if(EquippedWeapon) {
			if(Quality > EquippedWeapon->Quality)
				TextColor = COLOR_GREEN;
			else if(Quality < EquippedWeapon->Quality)
				TextColor = COLOR_RED;
		}
		DrawPosition.y += 20;
		Buffer << Quality << "%";
		ae::Assets.Fonts["hud_medium"]->DrawText("Quality", DrawPosition - DrawOffset, ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + DrawOffset, ae::LEFT_BASELINE, TextColor);
		Buffer.str("");
	}

	switch(Type) {
		case _Object::WEAPON: {
			_Weapon *Weapon = (_Weapon *)this;

			// Damage
			TextColor = COLOR_WHITE;
			if(EquippedWeapon) {
				if(Weapon->GetAverageDamage() > EquippedWeapon->GetAverageDamage())
					TextColor = COLOR_GREEN;
				else if(Weapon->GetAverageDamage() < EquippedWeapon->GetAverageDamage())
					TextColor = COLOR_RED;
			}
			DrawPosition.y += 20;
			Buffer << Weapon->Attributes.at("min_damage").Int << " - " << Weapon->Attributes.at("max_damage").Int;
			ae::Assets.Fonts["hud_medium"]->DrawText("Damage", DrawPosition - DrawOffset, ae::RIGHT_BASELINE);
			ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + DrawOffset, ae::LEFT_BASELINE, TextColor);
			Buffer.str("");

			// Clip size
			if(Weapon->Attributes.at("rounds").Int) {
				TextColor = COLOR_WHITE;
				if(EquippedWeapon) {
					if(Weapon->Attributes.at("rounds").Int > EquippedWeapon->Attributes.at("rounds").Int)
						TextColor = COLOR_GREEN;
					else if(Weapon->Attributes.at("rounds").Int < EquippedWeapon->Attributes.at("rounds").Int)
						TextColor = COLOR_RED;
				}
				DrawPosition.y += 20;
				Buffer << Weapon->Attributes.at("ammo").Int << "/" << Weapon->Attributes.at("rounds").Int;
				ae::Assets.Fonts["hud_medium"]->DrawText("Rounds", DrawPosition - DrawOffset, ae::RIGHT_BASELINE);
				ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + DrawOffset, ae::LEFT_BASELINE, TextColor);
				Buffer.str("");
			}

			// Attacks
			if(Weapon->Attributes.at("attack_count").Int > 1) {
				TextColor = COLOR_WHITE;
				if(EquippedWeapon) {
					if(Weapon->Attributes.at("attack_count").Int > EquippedWeapon->Attributes.at("attack_count").Int)
						TextColor = COLOR_GREEN;
					else if(Weapon->Attributes.at("attack_count").Int < EquippedWeapon->Attributes.at("attack_count").Int)
						TextColor = COLOR_RED;
				}

				DrawPosition.y += 20;
				Buffer << Weapon->Attributes.at("attack_count").Int;
				std::string AttackCountText;
				if(Weapon->IsMelee())
					AttackCountText = "Attacks/Swing";
				else
					AttackCountText = "Bullets/Shot";
				ae::Assets.Fonts["hud_medium"]->DrawText(AttackCountText, DrawPosition - DrawOffset, ae::RIGHT_BASELINE);
				ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + DrawOffset, ae::LEFT_BASELINE, TextColor);
				Buffer.str("");
			}

			// Fire rate
			if(Weapon->Attributes.at("fire_period").Double) {
				TextColor = COLOR_WHITE;
				if(EquippedWeapon) {
					if(Weapon->Attributes.at("fire_period").Double < EquippedWeapon->Attributes.at("fire_period").Double)
						TextColor = COLOR_GREEN;
					else if(Weapon->Attributes.at("fire_period").Double > EquippedWeapon->Attributes.at("fire_period").Double)
						TextColor = COLOR_RED;
				}

				DrawPosition.y += 20;
				Buffer << std::setprecision(3) << 1 / Weapon->Attributes.at("fire_period").Double << "/s";
				std::string AttackCountText;
				if(Weapon->IsMelee())
					AttackCountText = "Attack Rate";
				else
					AttackCountText = "Fire Rate";
				ae::Assets.Fonts["hud_medium"]->DrawText(AttackCountText, DrawPosition - DrawOffset, ae::RIGHT_BASELINE);
				ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + DrawOffset, ae::LEFT_BASELINE, TextColor);
				Buffer.str("");
				Buffer << std::setprecision(6);
			}

			// Weapon Spread
			TextColor = COLOR_WHITE;
			if(EquippedWeapon && EquippedWeapon->IsMelee() == Weapon->IsMelee()) {
				if(Weapon->GetAverageAccuracy() < EquippedWeapon->GetAverageAccuracy()) {

					// Less is worse for melee
					if(Weapon->IsMelee())
						TextColor = COLOR_RED;
					else
						TextColor = COLOR_GREEN;
				}
				else if(Weapon->GetAverageAccuracy() > EquippedWeapon->GetAverageAccuracy()) {

					// Bigger is better for melee
					if(Weapon->IsMelee())
						TextColor = COLOR_GREEN;
					else
						TextColor = COLOR_RED;
				}
			}
			DrawPosition.y += 20;
			if(Weapon->IsMelee()) {
				Buffer << Weapon->Attributes.at("max_accuracy").Int << " degrees";
				ae::Assets.Fonts["hud_medium"]->DrawText("Swing Arc", DrawPosition - DrawOffset, ae::RIGHT_BASELINE);
			}
			else {
				Buffer << Weapon->Attributes.at("min_accuracy").Int << " - " << Weapon->Attributes.at("max_accuracy").Int;
				ae::Assets.Fonts["hud_medium"]->DrawText("Accuracy", DrawPosition - DrawOffset, ae::RIGHT_BASELINE);
			}
			ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + DrawOffset, ae::LEFT_BASELINE, TextColor);
			Buffer.str("");

			// Reload speed
			if(Weapon->Attributes.at("reload_period").Double > 1) {
				TextColor = COLOR_WHITE;
				if(EquippedWeapon) {
					if(Weapon->Attributes.at("reload_period").Double < EquippedWeapon->Attributes.at("reload_period").Double)
						TextColor = COLOR_GREEN;
					else if(Weapon->Attributes.at("reload_period").Double > EquippedWeapon->Attributes.at("reload_period").Double)
						TextColor = COLOR_RED;
				}

				DrawPosition.y += 20;
				Buffer << ae::Round2(Weapon->Attributes.at("reload_period").Double) << "s";
				std::string AttackCountText;
				ae::Assets.Fonts["hud_medium"]->DrawText("Reload Time", DrawPosition - DrawOffset, ae::RIGHT_BASELINE);
				ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + DrawOffset, ae::LEFT_BASELINE, TextColor);
				Buffer.str("");
			}

			// Ammo type
			std::string AmmoType = Stats.Weapons[Weapon->ID].AmmoID;
			if(!AmmoType.empty()) {
				DrawPosition.y += 20;
				Buffer << Stats.Items[AmmoType].Name;
				ae::Assets.Fonts["hud_medium"]->DrawText("Ammo Type", DrawPosition - DrawOffset, ae::RIGHT_BASELINE);
				ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + DrawOffset);
				Buffer.str("");
			}

			// Components
			if(Weapon->Attributes.at("max_components").Int >= 1) {
				TextColor = COLOR_WHITE;
				if(EquippedWeapon) {
					if(Weapon->Attributes.at("max_components").Int > EquippedWeapon->Attributes.at("max_components").Int)
						TextColor = COLOR_GREEN;
					else if(Weapon->Attributes.at("max_components").Int < EquippedWeapon->Attributes.at("max_components").Int)
						TextColor = COLOR_RED;
				}

				DrawPosition.y += 20;
				Buffer << Weapon->Upgrades.size() << "/" << Weapon->Attributes.at("max_components").Int;
				ae::Assets.Fonts["hud_medium"]->DrawText("Components", DrawPosition - DrawOffset, ae::RIGHT_BASELINE);
				ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + DrawOffset, ae::LEFT_BASELINE, TextColor);
				Buffer.str("");
			}

			// Bonuses
			TextColor = COLOR_WHITE;
			bool First = true;
			for(int i = 0; i < UPGRADE_TYPES; i++) {
				if(!Weapon->Bonus[i])
					continue;

				if(First)
					DrawPosition.y += 10;
				DrawPosition.y += 20;

				std::string Percent = "% ";
				if(i == UPGRADE_ATTACKS)
					Percent = " ";

				Buffer << "+" << Weapon->Bonus[i] << Percent << UpgradeTypeToString(i, Weapon->Attributes.at("weapon_type").Int);
				ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::vec2(DrawPosition.x, DrawPosition.y), ae::CENTER_BASELINE, TextColor);
				Buffer.str("");

				First = false;
			}
		} break;
		case _Object::ARMOR: {

			// Damage Block
			if(Attributes.at("damage_block").Int != 0) {
				TextColor = COLOR_WHITE;
				if(EquippedArmor) {
					if(Attributes.at("damage_block").Int > EquippedArmor->Attributes.at("damage_block").Int)
						TextColor = COLOR_GREEN;
					else if(Attributes.at("damage_block").Int < EquippedArmor->Attributes.at("damage_block").Int)
						TextColor = COLOR_RED;
				}

				DrawPosition.y += 20;
				Buffer << Attributes.at("damage_block").Int;
				ae::Assets.Fonts["hud_medium"]->DrawText("Damage Block", DrawPosition - DrawOffset, ae::RIGHT_BASELINE);
				ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + DrawOffset, ae::LEFT_BASELINE, TextColor);
				Buffer.str("");
			}

			// Damage Resist
			if(Attributes.at("damage_resist").Int != 0) {
				TextColor = COLOR_WHITE;
				if(EquippedArmor) {
					if(Attributes.at("damage_resist").Int > EquippedArmor->Attributes.at("damage_resist").Int)
						TextColor = COLOR_GREEN;
					else if(Attributes.at("damage_resist").Int < EquippedArmor->Attributes.at("damage_resist").Int)
						TextColor = COLOR_RED;
				}

				DrawPosition.y += 20;
				Buffer << (Attributes.at("damage_resist").Int < 0 ? "" : "+") << Attributes.at("damage_resist").Int << "%";
				ae::Assets.Fonts["hud_medium"]->DrawText("Damage Resist", DrawPosition - DrawOffset, ae::RIGHT_BASELINE);
				ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + DrawOffset, ae::LEFT_BASELINE, TextColor);
				Buffer.str("");
			}

			// Damage Resist
			if(Attributes.at("max_ammo").Int != 0) {
				TextColor = COLOR_WHITE;
				if(EquippedArmor) {
					if(Attributes.at("max_ammo").Int > EquippedArmor->Attributes.at("max_ammo").Int)
						TextColor = COLOR_GREEN;
					else if(Attributes.at("max_ammo").Int < EquippedArmor->Attributes.at("max_ammo").Int)
						TextColor = COLOR_RED;
				}

				DrawPosition.y += 20;
				Buffer << (Attributes.at("max_ammo").Int < 0 ? "" : "+") << Attributes.at("max_ammo").Int << "%";
				ae::Assets.Fonts["hud_medium"]->DrawText("Max Ammo", DrawPosition - DrawOffset, ae::RIGHT_BASELINE);
				ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + DrawOffset, ae::LEFT_BASELINE, TextColor);
				Buffer.str("");
			}

			// Movement Speed
			if(Attributes.at("move_speed").Int != 0) {
				TextColor = COLOR_WHITE;
				if(EquippedArmor) {
					if(Attributes.at("move_speed").Int > EquippedArmor->Attributes.at("move_speed").Int)
						TextColor = COLOR_GREEN;
					else if(Attributes.at("move_speed").Int < EquippedArmor->Attributes.at("move_speed").Int)
						TextColor = COLOR_RED;
				}

				DrawPosition.y += 20;
				Buffer << (Attributes.at("move_speed").Int < 0 ? "" : "+") << Attributes.at("move_speed").Int << "%";
				ae::Assets.Fonts["hud_medium"]->DrawText("Movement Speed", DrawPosition - DrawOffset, ae::RIGHT_BASELINE);
				ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + DrawOffset, ae::LEFT_BASELINE, TextColor);
				Buffer.str("");
			}
		} break;
		case _Object::MEDKIT: {

			// Heal amount
			DrawPosition.y += 20;
			Buffer << "+" << Attributes.at("health_restored").Int << " HP";
			ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::vec2(DrawPosition.x, DrawPosition.y), ae::CENTER_BASELINE, COLOR_GREEN);
		} break;
		case _Object::UPGRADE: {

			// Bonus
			DrawPosition.y += 20;
			if(Attributes.at("upgrade_type").Int == UPGRADE_ATTACKS)
				Buffer << "+" << Attributes.at("bonus").Int;
			else
				Buffer << "+" << Attributes.at("bonus").Int << "%";

			ae::Assets.Fonts["hud_medium"]->DrawText(UpgradeTypeToString(Attributes.at("upgrade_type").Int, -1), DrawPosition - DrawOffset, ae::RIGHT_BASELINE);
			ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + DrawOffset, ae::LEFT_BASELINE, TextColor);
		} break;
	}
}

// Draws the object
void _Item::Render(double BlendFactor) {
	ae::Graphics.SetColor(Color);
	ae::Graphics.DrawSprite(glm::vec3(Position, PositionZ), Texture, Rotation, glm::vec2(ITEM_SCALE));
}

// Get average damage from range
float _Item::GetAverageDamage() const {
	return (Attributes.at("min_damage").Int + Attributes.at("max_damage").Int) * 0.5f;
}

// Get average accuracy from range
float _Item::GetAverageAccuracy() const {
	return (Attributes.at("min_accuracy").Int + Attributes.at("max_accuracy").Int) * 0.5f;
}

// Get type as string
std::string _Item::GetTypeAsString() const {

	switch(Type) {
		case _Object::KEY:
			return "Key";
		case _Object::AMMO:
			return "Ammo";
		case _Object::UPGRADE:
			return "Upgrade Component";
		case _Object::ARMOR:
			return "Armor";
		case _Object::MEDKIT:
			return "Medkit";
	}

	return "";
}

// Convert an upgrade type to string
std::string _Item::UpgradeTypeToString(int Type, int WeaponType) {

	switch(Type) {
		case UPGRADE_CLIP:
			return "Round Size";
		break;
		case UPGRADE_DAMAGE:
			return "Damage";
		break;
		case UPGRADE_ACCURACY:
			return "Accuracy";
		break;
		case UPGRADE_FIREPERIOD:
			if(WeaponType == WEAPON_MELEE)
				return "Attack Rate";
			else
				return "Fire Rate";
		break;
		case UPGRADE_RELOADPERIOD:
			return "Reload Speed";
		break;
		case UPGRADE_ATTACKS:
			if(WeaponType == WEAPON_MELEE)
				return "Attack Count";
			else
				return "Bullets/Shot";
		break;
	}

	return "";
}
