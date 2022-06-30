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
_Item::_Item(const _ObjectTemplate &ItemTemplate) :
	_Object(ItemTemplate),
	Quality(0),
	Count(1) {

	PositionZ = ITEM_Z;
}

// Destructor
_Item::~_Item() {
	for(size_t i = 0; i < Mods.size(); i++)
		delete Mods[i];
}

// Draw the item popup window
void _Item::DrawTooltip(const _Player *Player, std::size_t CompareSlot, glm::ivec2 DrawPosition) {
	std::ostringstream Buffer;

	glm::ivec2 Size(300, 120);
	if(Type == _Object::WEAPON)
		Size.y = 280;
	else if(Type == _Object::ARMOR)
		Size.y = 220;
	else if(Type == _Object::MOD)
		Size.y = 170;

	// Increase size for each unique mod
	for(int i = 1; i < MOD_COUNT; i++) {
		if(Bonus[i])
			Size.y += 20;
	}

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
	_Item *EquippedItem = nullptr;
	if(CompareSlot < INVENTORY_SIZE) {
		EquippedItem = Player->Inventory[CompareSlot];
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
	if(Type == _Object::WEAPON || Type == _Object::ARMOR || Type == _Object::MOD) {
		if(EquippedItem) {
			if(Quality > EquippedItem->Quality)
				TextColor = COLOR_GREEN;
			else if(Quality < EquippedItem->Quality)
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

			// Damage
			TextColor = COLOR_WHITE;
			if(EquippedItem) {
				if(GetAverageDamage() > EquippedItem->GetAverageDamage())
					TextColor = COLOR_GREEN;
				else if(GetAverageDamage() < EquippedItem->GetAverageDamage())
					TextColor = COLOR_RED;
			}
			DrawPosition.y += 20;
			Buffer << Attributes.at("min_damage").Int << " - " << Attributes.at("max_damage").Int;
			ae::Assets.Fonts["hud_medium"]->DrawText("Damage", DrawPosition - DrawOffset, ae::RIGHT_BASELINE);
			ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + DrawOffset, ae::LEFT_BASELINE, TextColor);
			Buffer.str("");

			// Clip size
			if(Attributes.at("rounds").Int) {
				TextColor = COLOR_WHITE;
				if(EquippedItem) {
					if(Attributes.at("rounds").Int > EquippedItem->Attributes.at("rounds").Int)
						TextColor = COLOR_GREEN;
					else if(Attributes.at("rounds").Int < EquippedItem->Attributes.at("rounds").Int)
						TextColor = COLOR_RED;
				}
				DrawPosition.y += 20;
				Buffer << Attributes.at("ammo").Int << "/" << Attributes.at("rounds").Int;
				ae::Assets.Fonts["hud_medium"]->DrawText("Rounds", DrawPosition - DrawOffset, ae::RIGHT_BASELINE);
				ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + DrawOffset, ae::LEFT_BASELINE, TextColor);
				Buffer.str("");
			}

			// Attacks
			if(Attributes.at("attack_count").Int > 1) {
				TextColor = COLOR_WHITE;
				if(EquippedItem) {
					if(Attributes.at("attack_count").Int > EquippedItem->Attributes.at("attack_count").Int)
						TextColor = COLOR_GREEN;
					else if(Attributes.at("attack_count").Int < EquippedItem->Attributes.at("attack_count").Int)
						TextColor = COLOR_RED;
				}

				DrawPosition.y += 20;
				Buffer << Attributes.at("attack_count").Int;
				std::string AttackCountText;
				if(IsMelee())
					AttackCountText = "Attacks/Swing";
				else
					AttackCountText = "Bullets/Shot";
				ae::Assets.Fonts["hud_medium"]->DrawText(AttackCountText, DrawPosition - DrawOffset, ae::RIGHT_BASELINE);
				ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + DrawOffset, ae::LEFT_BASELINE, TextColor);
				Buffer.str("");
			}

			// Penetration
			if(Attributes.at("penetration").Int > 1) {
				TextColor = COLOR_WHITE;
				if(EquippedItem) {
					if(Attributes.at("penetration").Int > EquippedItem->Attributes.at("penetration").Int)
						TextColor = COLOR_GREEN;
					else if(Attributes.at("penetration").Int < EquippedItem->Attributes.at("penetration").Int)
						TextColor = COLOR_RED;
				}

				DrawPosition.y += 20;
				Buffer << Attributes.at("penetration").Int;
				std::string AttackCountText;
				AttackCountText = "Penetration";
				ae::Assets.Fonts["hud_medium"]->DrawText(AttackCountText, DrawPosition - DrawOffset, ae::RIGHT_BASELINE);
				ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + DrawOffset, ae::LEFT_BASELINE, TextColor);
				Buffer.str("");
			}

			// Fire rate
			if(Attributes.at("fire_period").Double) {
				TextColor = COLOR_WHITE;
				if(EquippedItem) {
					if(Attributes.at("fire_period").Double < EquippedItem->Attributes.at("fire_period").Double)
						TextColor = COLOR_GREEN;
					else if(Attributes.at("fire_period").Double > EquippedItem->Attributes.at("fire_period").Double)
						TextColor = COLOR_RED;
				}

				DrawPosition.y += 20;
				Buffer << std::setprecision(3) << 1 / Attributes.at("fire_period").Double << "/s";
				std::string AttackCountText;
				if(IsMelee())
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
			if(EquippedItem && EquippedItem->IsMelee() == IsMelee()) {
				if(GetAverageAccuracy() < EquippedItem->GetAverageAccuracy()) {

					// Less is worse for melee
					if(IsMelee())
						TextColor = COLOR_RED;
					else
						TextColor = COLOR_GREEN;
				}
				else if(GetAverageAccuracy() > EquippedItem->GetAverageAccuracy()) {

					// Bigger is better for melee
					if(IsMelee())
						TextColor = COLOR_GREEN;
					else
						TextColor = COLOR_RED;
				}
			}
			DrawPosition.y += 20;
			if(IsMelee()) {
				Buffer << Attributes.at("max_accuracy").Int << " degrees";
				ae::Assets.Fonts["hud_medium"]->DrawText("Swing Arc", DrawPosition - DrawOffset, ae::RIGHT_BASELINE);
			}
			else {
				Buffer << Attributes.at("min_accuracy").Int << " - " << Attributes.at("max_accuracy").Int;
				ae::Assets.Fonts["hud_medium"]->DrawText("Accuracy", DrawPosition - DrawOffset, ae::RIGHT_BASELINE);
			}
			ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + DrawOffset, ae::LEFT_BASELINE, TextColor);
			Buffer.str("");

			// Reload speed
			if(Attributes.at("reload_period").Double > 1) {
				TextColor = COLOR_WHITE;
				if(EquippedItem) {
					if(Attributes.at("reload_period").Double < EquippedItem->Attributes.at("reload_period").Double)
						TextColor = COLOR_GREEN;
					else if(Attributes.at("reload_period").Double > EquippedItem->Attributes.at("reload_period").Double)
						TextColor = COLOR_RED;
				}

				DrawPosition.y += 20;
				Buffer << ae::Round2(Attributes.at("reload_period").Double) << "s";
				std::string AttackCountText;
				ae::Assets.Fonts["hud_medium"]->DrawText("Reload Time", DrawPosition - DrawOffset, ae::RIGHT_BASELINE);
				ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + DrawOffset, ae::LEFT_BASELINE, TextColor);
				Buffer.str("");
			}

			// Ammo type
			if(!Template.AmmoID.empty()) {
				DrawPosition.y += 20;
				Buffer << Template.Name;
				ae::Assets.Fonts["hud_medium"]->DrawText("Ammo Type", DrawPosition - DrawOffset, ae::RIGHT_BASELINE);
				ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + DrawOffset);
				Buffer.str("");
			}
		} break;
		case _Object::ARMOR: {

			// Damage Block
			if(Attributes.at("damage_block").Int != 0) {
				TextColor = COLOR_WHITE;
				if(EquippedItem) {
					if(Attributes.at("damage_block").Int > EquippedItem->Attributes.at("damage_block").Int)
						TextColor = COLOR_GREEN;
					else if(Attributes.at("damage_block").Int < EquippedItem->Attributes.at("damage_block").Int)
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
				if(EquippedItem) {
					if(Attributes.at("damage_resist").Int > EquippedItem->Attributes.at("damage_resist").Int)
						TextColor = COLOR_GREEN;
					else if(Attributes.at("damage_resist").Int < EquippedItem->Attributes.at("damage_resist").Int)
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
				if(EquippedItem) {
					if(Attributes.at("max_ammo").Int > EquippedItem->Attributes.at("max_ammo").Int)
						TextColor = COLOR_GREEN;
					else if(Attributes.at("max_ammo").Int < EquippedItem->Attributes.at("max_ammo").Int)
						TextColor = COLOR_RED;
				}

				DrawPosition.y += 20;
				Buffer << (Attributes.at("max_ammo").Int < 0 ? "" : "+") << Attributes.at("max_ammo").Int << "%";
				ae::Assets.Fonts["hud_medium"]->DrawText("Max Ammo", DrawPosition - DrawOffset, ae::RIGHT_BASELINE);
				ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + DrawOffset, ae::LEFT_BASELINE, TextColor);
				Buffer.str("");
			}

			// Move Speed
			if(Attributes.at("move_speed").Int != 0) {
				TextColor = COLOR_WHITE;
				if(EquippedItem) {
					if(Attributes.at("move_speed").Int > EquippedItem->Attributes.at("move_speed").Int)
						TextColor = COLOR_GREEN;
					else if(Attributes.at("move_speed").Int < EquippedItem->Attributes.at("move_speed").Int)
						TextColor = COLOR_RED;
				}

				DrawPosition.y += 20;
				Buffer << (Attributes.at("move_speed").Int < 0 ? "" : "+") << Attributes.at("move_speed").Int << "%";
				ae::Assets.Fonts["hud_medium"]->DrawText("Move Speed", DrawPosition - DrawOffset, ae::RIGHT_BASELINE);
				ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + DrawOffset, ae::LEFT_BASELINE, TextColor);
				Buffer.str("");
			}
		} break;
		case _Object::MOD: {
			DrawPosition.y += 20;
			std::string Percent = Template.Attributes.at("percent_sign").Int ? "%" : "";
			Buffer << "+" << Attributes.at("bonus").Int << Percent;
			ae::Assets.Fonts["hud_medium"]->DrawText(ModTypeToString(Attributes.at("mod_type").Int), DrawPosition - DrawOffset, ae::RIGHT_BASELINE);
			ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + DrawOffset, ae::LEFT_BASELINE, TextColor);
		} break;
		case _Object::MEDKIT: {
			DrawPosition.y += 20;
			Buffer << "+" << Attributes.at("health_restored").Int << " HP";
			ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::vec2(DrawPosition.x, DrawPosition.y), ae::CENTER_BASELINE, COLOR_GREEN);
		} break;
	}

	// Mods
	if(Attributes.find("max_mods") != Attributes.end() && Attributes.at("max_mods").Int >= 1) {
		TextColor = COLOR_WHITE;
		if(EquippedItem) {
			if(Attributes.at("max_mods").Int > EquippedItem->Attributes.at("max_mods").Int)
				TextColor = COLOR_GREEN;
			else if(Attributes.at("max_mods").Int < EquippedItem->Attributes.at("max_mods").Int)
				TextColor = COLOR_RED;
		}

		DrawPosition.y += 20;
		Buffer << Mods.size() << "/" << Attributes.at("max_mods").Int;
		ae::Assets.Fonts["hud_medium"]->DrawText("Mods", DrawPosition - DrawOffset, ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + DrawOffset, ae::LEFT_BASELINE, TextColor);
		Buffer.str("");
	}

	// Bonuses
	TextColor = COLOR_WHITE;
	bool First = true;
	for(int i = 1; i < MOD_COUNT; i++) {
		if(!Bonus[i])
			continue;

		if(First)
			DrawPosition.y += 10;
		DrawPosition.y += 20;

		std::string Percent = " ";
		if(Stats.Objects.at(Stats.ModNames[i]).Attributes.at("percent_sign").Int)
			Percent = "% ";

		Buffer << "+" << Bonus[i] << Percent << ModTypeToString(i);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::vec2(DrawPosition.x, DrawPosition.y), ae::CENTER_BASELINE, TextColor);
		Buffer.str("");

		First = false;
	}
}

// Draws the object
void _Item::Render(double BlendFactor) {
	ae::Graphics.SetColor(Color);
	ae::Graphics.DrawSprite(glm::vec3(Position, PositionZ), Texture, Rotation, glm::vec2(ITEM_SCALE));
}

// Serialize for saving
void _Item::Serialize(ae::_Buffer &Buffer) {
	Buffer.WriteString(ID.c_str());
	Buffer.Write<int>(Level);
	Buffer.Write<int>(Quality);

	// Write mods
	if(Type == _Object::WEAPON || Type == _Object::ARMOR) {
		Buffer.Write(Attributes.at("max_mods").Int);
		Buffer.Write<int>(Mods.size());
		for(size_t i = 0; i < Mods.size(); i++)
			Mods[i]->Serialize(Buffer);
	}

	// Write ammo
	if(Type == _Object::WEAPON)
		Buffer.Write(Attributes.at("ammo").Int);
}

// Recalulate stats for item
void _Item::RecalculateStats() {
	for(int i = 0; i < MOD_COUNT; i++)
		Bonus[i] = 0;

	// Sum bonuses
	for(size_t i = 0; i < Mods.size(); i++)
		Bonus[Mods[i]->Attributes.at("mod_type").Int] += Mods[i]->Attributes.at("bonus").Int;

	switch(Type) {
		case _Object::WEAPON: {

			SetAttributeRange("damage", GetBonusMultiplier(MOD_DAMAGE) + Quality * 0.01f);
			SetAttributeSpread("accuracy", IsMelee() ? GetBonusMultiplier(MOD_ACCURACY) : 1.0f / GetBonusMultiplier(MOD_ACCURACY));
			Attributes["rounds"].Int = std::ceil(Template.Attributes.at("rounds").Int * GetBonusMultiplier(MOD_MAXROUNDS));
			Attributes["fire_period"].Double = Template.Attributes.at("fire_period").Double / GetBonusMultiplier(MOD_ATTACKSPEED);
			Attributes["reload_period"].Double = Template.Attributes.at("reload_period").Double / GetBonusMultiplier(MOD_RELOADSPEED);
			Attributes["attack_count"].Int = Template.Attributes.at("attack_count").Int;
			Attributes["reload_amount"].Int = Template.Attributes.at("reload_amount").Int + Bonus[MOD_RELOADAMOUNT];
			Attributes["penetration"].Int = Template.Attributes.at("penetration").Int + Bonus[MOD_PENETRATION];

			// For melee, min accuracy is 0 and max is swing arc
			if(IsMelee())
				Attributes["min_accuracy"].Int = 0;

			SetAmmo(Attributes["ammo"].Int);
		} break;
		case _Object::ARMOR:
			SetAttributeLevel("damage_block", 1.0f + Quality * 0.01f);
			SetAttributeLevel("damage_resist", 1.0f + Quality * 0.01f);
			SetAttributeLevel("max_ammo", 1.0f + Quality * 0.01f);
			SetAttributeLevel("move_speed",  1.0f - Quality * 0.01f);
			Attributes.at("damage_block").Int += Bonus[MOD_DAMAGEBLOCK];
			Attributes.at("damage_resist").Int += Bonus[MOD_DAMAGERESIST];
			Attributes.at("max_ammo").Int += Bonus[MOD_MAXAMMO];
			Attributes.at("move_speed").Int += Bonus[MOD_MOVESPEED];
		break;
	}
}

// Add mod to item
bool _Item::AddMod(_Item *Mod) {
	if(Mod->Template.Attributes.at("object_type").Int != Type)
		return false;

	if((int)Mods.size() >= Attributes.at("max_mods").Int)
		return false;

	switch(Type) {
		case _Object::WEAPON: {

			// Check weapon type
			if(Mod->Template.Attributes.at("weapon_type").Int != 0 && Mod->Attributes.at("weapon_type").Int != Attributes.at("weapon_type").Int)
				return false;

			// Check for ammo
			int ModType = Mod->Template.Attributes.at("mod_type").Int;
			if(Template.Attributes.at("rounds").Int == 0 && (ModType == MOD_MAXROUNDS || ModType == MOD_RELOADSPEED || ModType == MOD_RELOADAMOUNT || ModType == MOD_HANDLING))
				return false;

		} break;
	}
	Mods.push_back(Mod);
	RecalculateStats();

	return true;
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
		case _Object::WEAPON: {
			std::string WeaponTypeString;
			switch(Attributes.at("weapon_type").Int) {
				case WEAPON_MELEE:
					WeaponTypeString = "Melee";
				break;
				case WEAPON_PISTOL:
					WeaponTypeString = "Pistol";
				break;
				case WEAPON_SHOTGUN:
					WeaponTypeString = "Shotgun";
				break;
				case WEAPON_RIFLE:
					WeaponTypeString = "Rifle";
				break;
				case WEAPON_HEAVY:
					WeaponTypeString = "Heavy";
				break;
			}
			return WeaponTypeString + " class weapon";
		} break;
		case _Object::ARMOR:
			return "Armor";
		case _Object::MOD:
			return "Weapon Mod";
		case _Object::AMMO:
			return "Ammo";
		case _Object::KEY:
			return "Key";
		case _Object::MEDKIT:
			return "Medkit";
	}

	return "";
}

// Convert a mod type to string
std::string _Item::ModTypeToString(int ModType) {

	switch(ModType) {
		case MOD_MAXROUNDS:
			return "Round Size";
		break;
		case MOD_DAMAGE:
			return "Damage";
		break;
		case MOD_ACCURACY:
			if(IsMelee())
				return "Swing Arc";
			else
				return "Accuracy";
		break;
		case MOD_ATTACKSPEED:
			if(IsMelee())
				return "Attack Rate";
			else
				return "Fire Rate";
		break;
		case MOD_RELOADSPEED:
			return "Reload Speed";
		break;
		case MOD_RELOADAMOUNT:
			return "Reload Amount";
		break;
		case MOD_PENETRATION:
			return "Penetration";
		break;
		case MOD_HANDLING:
			return "Handling";
		break;
		case MOD_DAMAGEBLOCK:
			return "Damage Block";
		break;
		case MOD_DAMAGERESIST:
			return "Damage Resist";
		break;
		case MOD_MAXAMMO:
			return "Max Ammo";
		break;
		case MOD_MOVESPEED:
			return "Move Speed";
		break;
	}

	return "";
}

// Set ammo for weapon
void _Item::SetAmmo(int Value) {
	if(Type != _Object::WEAPON)
		return;

	Attributes["ammo"].Int = std::clamp(Value, 0, Attributes["rounds"].Int);
}
