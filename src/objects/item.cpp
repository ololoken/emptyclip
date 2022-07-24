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
#include <ae/input.h>
#include <ae/util.h>
#include <ae/random.h>
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

// Draw attribute text
void _Item::DrawAttribute(const std::string &Attribute, const std::string &Label, glm::vec2 &DrawPosition, const _Item *EquippedItem, bool Plus, bool Percent) const {
	if(!Attributes.at(Attribute).Int)
		return;

	// Get color
	glm::vec4 TextColor = COLOR_WHITE;
	if(EquippedItem) {
		if(Attributes.at(Attribute).Int > EquippedItem->Attributes.at(Attribute).Int)
			TextColor = COLOR_GREEN;
		else if(Attributes.at(Attribute).Int < EquippedItem->Attributes.at(Attribute).Int)
			TextColor = COLOR_RED;
	}

	// Get text
	std::ostringstream Buffer;
	if(Plus && Attributes.at(Attribute).Int > 0)
		Buffer << "+";
	Buffer << Attributes.at(Attribute).Int;
	if(Percent)
		Buffer << "%";

	// Draw
	glm::vec2 DrawOffset(8 * ae::_Element::GetUIScale(), 0);
	DrawPosition.y += 36 * ae::_Element::GetUIScale();
	ae::Assets.Fonts["hud_medium"]->DrawText(Label, glm::ivec2(DrawPosition - DrawOffset), ae::RIGHT_BASELINE);
	ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + DrawOffset), ae::LEFT_BASELINE, TextColor);
}

// Draw the item popup window
void _Item::DrawTooltip(const _Player *Player, size_t CompareSlot, int InventorySlot, glm::vec2 DrawPosition) {
	std::ostringstream Buffer;

	glm::vec2 Size = glm::vec2(500, 150) * ae::_Element::GetUIScale();
	glm::vec2 Spacing = glm::vec2(0, 36) * ae::_Element::GetUIScale();
	glm::vec2 SmallSpacing = glm::vec2(0, 24) * ae::_Element::GetUIScale();

	// Set size based on type
	if(Type == _Object::WEAPON)
		Size.y = 540 * ae::_Element::GetUIScale();
	else if(Type == _Object::ARMOR)
		Size.y = 380 * ae::_Element::GetUIScale();
	else if(Type == _Object::MEDKIT)
		Size.y = 240 * ae::_Element::GetUIScale();
	else if(Type == _Object::MOD) {
		Size.x = 520 * ae::_Element::GetUIScale();
		Size.y = 300 * ae::_Element::GetUIScale();
	}

	// Increase size for each unique mod
	bool HasOneBonus = false;
	for(int i = 1; i < MOD_COUNT; i++) {
		if(Bonus[i]) {
			Size.y += SmallSpacing.y;
			HasOneBonus = true;
		}
	}
	if(HasOneBonus)
		Size.y += Spacing.y;

	// Get title width
	ae::_TextBounds TextBounds;
	ae::Assets.Fonts["menu_buttons"]->GetStringDimensions(Name, TextBounds);
	Size.x = std::max(Size.x, (float)TextBounds.Width) + 20 * ae::_Element::GetUIScale();

	// Offset position
	float WindowOffsetX = 20 * ae::_Element::GetUIScale();
	float MinX = 0;
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
	DrawPosition.y = std::clamp(DrawPosition.y, 0.0f, ae::Graphics.CurrentSize.y - Size.y);

	// Draw background
	ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos"]);
	ae::Graphics.SetColor(ae::Assets.Colors["tooltip_bg"]);
	ae::Graphics.DrawRectangle(glm::ivec2(DrawPosition), glm::ivec2(DrawPosition + Size), true);

	// Draw name
	DrawPosition.y += 40 * ae::_Element::GetUIScale();
	DrawPosition.x += Size.x/2;
	ae::Assets.Fonts["menu_buttons"]->DrawText(Name, glm::ivec2(DrawPosition), ae::CENTER_BASELINE);

	// Draw type
	DrawPosition.y += 24 * ae::_Element::GetUIScale();
	ae::Assets.Fonts["hud_small"]->DrawText(GetTypeAsString(), glm::ivec2(DrawPosition), ae::CENTER_BASELINE);

	// Draw Level
	if(Type != _Object::KEY && Type != _Object::AMMO && Type != _Object::MEDKIT) {
		DrawPosition.y += 24 * ae::_Element::GetUIScale();
		Buffer << "Level " << Level;
		ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition), ae::CENTER_BASELINE);
		Buffer.str("");
	}

	// Quality
	glm::vec4 TextColor = COLOR_WHITE;
	if(Type == _Object::WEAPON || Type == _Object::ARMOR || Type == _Object::MOD) {
		DrawPosition.y += 24 * ae::_Element::GetUIScale();
		Buffer << "Quality " << Quality << "%";
		ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition), ae::CENTER_BASELINE);
		Buffer.str("");
	}

	DrawPosition.y += 10 * ae::_Element::GetUIScale();
	glm::vec2 DrawOffset(8 * ae::_Element::GetUIScale(), 0);

	std::vector<std::string> HelpTextList;
	switch(Type) {
		case _Object::WEAPON: {
			if(InventorySlot >= INVENTORY_BAGSTART)
				HelpTextList.push_back("Right-click to equip");

			// Damage
			TextColor = COLOR_WHITE;
			if(EquippedItem) {
				if(GetAverageDamage() > EquippedItem->GetAverageDamage())
					TextColor = COLOR_GREEN;
				else if(GetAverageDamage() < EquippedItem->GetAverageDamage())
					TextColor = COLOR_RED;
			}
			DrawPosition.y += Spacing.y;
			if(ae::Input.ModKeyDown(KMOD_ALT))
				Buffer << ae::Round1(GetAverageDamage()) << " avg";
			else
				Buffer << Attributes.at("min_damage").Int << " - " << Attributes.at("max_damage").Int;
			ae::Assets.Fonts["hud_medium"]->DrawText("Damage", glm::ivec2(DrawPosition - DrawOffset), ae::RIGHT_BASELINE);
			ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + DrawOffset), ae::LEFT_BASELINE, TextColor);
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
				DrawPosition.y += Spacing.y;
				Buffer << Attributes.at("ammo").Int << "/" << Attributes.at("rounds").Int;
				ae::Assets.Fonts["hud_medium"]->DrawText("Rounds", glm::ivec2(DrawPosition - DrawOffset), ae::RIGHT_BASELINE);
				ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + DrawOffset), ae::LEFT_BASELINE, TextColor);
				Buffer.str("");
			}

			// Crit chance
			if(Attributes.at("crit_chance").Int)
				DrawAttribute("crit_chance", "Critical Hit Chance", DrawPosition, EquippedItem, false, true);

			// Attack count
			if(Attributes.at("attack_count").Int > 1)
				DrawAttribute("attack_count", IsMelee() ? "Attacks" : "Bullets Shot", DrawPosition, EquippedItem, false, false);

			// Penetration
			if(Attributes.at("penetration").Int > 1)
				DrawAttribute("penetration", "Penetration", DrawPosition, EquippedItem, false, false);

			// Fire rate
			if(Attributes.at("fire_period").Double) {
				TextColor = COLOR_WHITE;
				if(EquippedItem) {
					if(Attributes.at("fire_period").Double < EquippedItem->Attributes.at("fire_period").Double)
						TextColor = COLOR_GREEN;
					else if(Attributes.at("fire_period").Double > EquippedItem->Attributes.at("fire_period").Double)
						TextColor = COLOR_RED;
				}

				DrawPosition.y += Spacing.y;
				Buffer << std::setprecision(3) << 1 / Attributes.at("fire_period").Double << "/s";
				std::string AttackCountText;
				if(IsMelee())
					AttackCountText = "Attack Speed";
				else
					AttackCountText = "Fire Rate";
				ae::Assets.Fonts["hud_medium"]->DrawText(AttackCountText, glm::ivec2(DrawPosition - DrawOffset), ae::RIGHT_BASELINE);
				ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + DrawOffset), ae::LEFT_BASELINE, TextColor);
				Buffer.str("");
				Buffer << std::setprecision(6);
			}

			// Accuracy
			if(!IsMelee()) {
				TextColor = COLOR_WHITE;
				if(EquippedItem) {
					if(GetAverageAccuracy() < EquippedItem->GetAverageAccuracy())
						TextColor = COLOR_GREEN;
					else if(GetAverageAccuracy() > EquippedItem->GetAverageAccuracy())
						TextColor = COLOR_RED;
				}

				DrawPosition.y += Spacing.y;
				if(ae::Input.ModKeyDown(KMOD_ALT))
					Buffer << ae::Round1(GetAverageAccuracy()) << " avg";
				else
					Buffer << ae::Round1(Attributes.at("min_accuracy").Float) << " - " << ae::Round1(Attributes.at("max_accuracy").Float) << " deg";
				ae::Assets.Fonts["hud_medium"]->DrawText("Accuracy", glm::ivec2(DrawPosition - DrawOffset), ae::RIGHT_BASELINE);
				ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + DrawOffset), ae::LEFT_BASELINE, TextColor);
				Buffer.str("");
			}

			// Recoil
			if(!IsMelee()) {
				TextColor = COLOR_WHITE;
				if(EquippedItem) {
					if(Attributes.at("recoil").Float < EquippedItem->Attributes.at("recoil").Float)
						TextColor = COLOR_GREEN;
					else if(Attributes.at("recoil").Float > EquippedItem->Attributes.at("recoil").Float)
						TextColor = COLOR_RED;
				}

				DrawPosition.y += Spacing.y;
				Buffer << ae::Round1(Attributes.at("recoil").Float);
				ae::Assets.Fonts["hud_medium"]->DrawText("Recoil", glm::ivec2(DrawPosition - DrawOffset), ae::RIGHT_BASELINE);
				ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + DrawOffset), ae::LEFT_BASELINE, TextColor);
				Buffer.str("");
			}

			// Range
			if(IsMelee()) {
				TextColor = COLOR_WHITE;
				if(EquippedItem) {
					if(Attributes.at("range").Float > EquippedItem->Attributes.at("range").Float)
						TextColor = COLOR_GREEN;
					else if(Attributes.at("range").Float < EquippedItem->Attributes.at("range").Float)
						TextColor = COLOR_RED;
				}

				DrawPosition.y += Spacing.y;
				Buffer << Attributes.at("range").Float;
				ae::Assets.Fonts["hud_medium"]->DrawText("Range", glm::ivec2(DrawPosition - DrawOffset), ae::RIGHT_BASELINE);
				ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + DrawOffset), ae::LEFT_BASELINE, TextColor);
				Buffer.str("");
			}

			// Reload speed
			if(Attributes.at("reload_period").Double > 1) {
				TextColor = COLOR_WHITE;
				if(EquippedItem) {
					if(Attributes.at("reload_period").Double < EquippedItem->Attributes.at("reload_period").Double)
						TextColor = COLOR_GREEN;
					else if(Attributes.at("reload_period").Double > EquippedItem->Attributes.at("reload_period").Double)
						TextColor = COLOR_RED;
				}

				DrawPosition.y += Spacing.y;
				Buffer << ae::Round2(Attributes.at("reload_period").Double) << "s";
				std::string AttackCountText;
				ae::Assets.Fonts["hud_medium"]->DrawText("Reload Time", glm::ivec2(DrawPosition - DrawOffset), ae::RIGHT_BASELINE);
				ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + DrawOffset), ae::LEFT_BASELINE, TextColor);
				Buffer.str("");
			}

			// Ammo type
			if(!Template.AmmoID.empty()) {
				DrawPosition.y += Spacing.y;
				Buffer << Stats.Objects.at(Template.AmmoID).Name;
				ae::Assets.Fonts["hud_medium"]->DrawText("Ammo Type", glm::ivec2(DrawPosition - DrawOffset), ae::RIGHT_BASELINE);
				ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + DrawOffset));
				Buffer.str("");
			}
		} break;
		case _Object::ARMOR: {
			if(InventorySlot >= INVENTORY_BAGSTART)
				HelpTextList.push_back("Right-click to equip");

			DrawAttribute("damage_block", "Damage Block", DrawPosition, EquippedItem, false, false);
			DrawAttribute("damage_resist", "Damage Resist", DrawPosition, EquippedItem, true, true);
			DrawAttribute("max_ammo", "Max Ammo", DrawPosition, EquippedItem, true, true);
			DrawAttribute("move_speed", "Move Speed", DrawPosition, EquippedItem, true, true);
		} break;
		case _Object::MOD: {
			if(Template.Attributes.at("object_type").Int == _Object::WEAPON)
				HelpTextList.push_back("Drag onto weapon");
			else if(Template.Attributes.at("object_type").Int == _Object::ARMOR)
				HelpTextList.push_back("Drag onto armor");

			DrawPosition.y += Spacing.y;
			std::string Percent = Template.Attributes.at("percent_sign").Int ? "%" : "";
			Buffer << "+" << Attributes.at("bonus").Int << Percent;
			ae::Assets.Fonts["hud_medium"]->DrawText(ModTypeToString(Attributes.at("mod_type").Int), glm::ivec2(DrawPosition - DrawOffset), ae::RIGHT_BASELINE);
			ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + DrawOffset), ae::LEFT_BASELINE, TextColor);
		} break;
		case _Object::MEDKIT: {
			HelpTextList.push_back("Used when picked up");
			DrawPosition.y += Spacing.y;
			Buffer << "+" << (int)(GAME_MEDKIT_HEALTH_PERCENT * Player->HealModifier) << "% HP";
			ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition), ae::CENTER_BASELINE, COLOR_GREEN);
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

		DrawPosition.y += Spacing.y;
		Buffer << Mods.size() << "/" << Attributes.at("max_mods").Int;
		ae::Assets.Fonts["hud_medium"]->DrawText("Mods", glm::ivec2(DrawPosition - DrawOffset), ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + DrawOffset), ae::LEFT_BASELINE, TextColor);
		Buffer.str("");
	}

	// Mod bonuses
	TextColor = COLOR_WHITE;
	if(HasOneBonus)
		DrawPosition.y += Spacing.y * 0.5f;

	for(int i = 1; i < MOD_COUNT; i++) {
		if(!Bonus[i])
			continue;

		DrawPosition.y += SmallSpacing.y;

		std::string Percent = " ";
		if(Stats.Objects.at(Stats.ModNames[i]).Attributes.at("percent_sign").Int)
			Percent = "% ";

		Buffer << "+" << Bonus[i] << Percent << ModTypeToString(i);
		ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition), ae::CENTER_BASELINE, TextColor);
		Buffer.str("");
	}

	// Draw ui hints
	DrawPosition.y += 60 * ae::_Element::GetUIScale();
	for(const auto &Text : HelpTextList) {
		ae::Assets.Fonts["hud_small"]->DrawText(Text, glm::ivec2(DrawPosition), ae::CENTER_BASELINE, COLOR_GRAY);
		DrawPosition.y += 32 * ae::_Element::GetUIScale();
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

	float QualityFactor = 1.0f + Quality * 0.01f;
	switch(Type) {
		case _Object::WEAPON:
			SetAttributeRange("damage", GetBonusMultiplier(MOD_DAMAGE));
			SetAttributeSpread("accuracy", GetBonusMultiplier(MOD_ACCURACY, true));
			Attributes["rounds"].Int = Template.Attributes.at("rounds").Int * GetBonusMultiplier(MOD_MAXROUNDS) + 0.5f;
			Attributes["fire_period"].Double = Template.Attributes.at("fire_period").Double * GetBonusMultiplier(MOD_ATTACKSPEED, true);
			Attributes["attack_count"].Int = Template.Attributes.at("attack_count").Int;
			Attributes["reload_period"].Double = Template.Attributes.at("reload_period").Double * GetBonusMultiplier(MOD_RELOADSPEED, true);
			Attributes["reload_amount"].Int = Template.Attributes.at("reload_amount").Int + Bonus[MOD_RELOADAMOUNT];
			Attributes["recoil"].Float = Template.Attributes.at("recoil").Float * GetBonusMultiplier(MOD_HANDLING, true);
			Attributes["recoil_regen"].Float = Template.Attributes.at("recoil_regen").Float * GetBonusMultiplier(MOD_HANDLING);
			Attributes["move_recoil"].Float = Template.Attributes.at("move_recoil").Float * GetBonusMultiplier(MOD_HANDLING, true);
			Attributes["penetration"].Int = Template.Attributes.at("penetration").Int + Bonus[MOD_PENETRATION];
			Attributes["crit_chance"].Int = Template.Attributes.at("crit_chance").Int * QualityFactor + 0.5f;

			SetAmmo(Attributes["ammo"].Int);
		break;
		case _Object::ARMOR:
			SetAttributeLevel("damage_block", QualityFactor);
			SetAttributeLevel("damage_resist", QualityFactor);
			SetAttributeLevel("max_ammo", QualityFactor);
			SetAttributeLevel("move_speed", QualityFactor);
			Attributes.at("damage_block").Int += Bonus[MOD_DAMAGEBLOCK];
			Attributes.at("damage_resist").Int += Bonus[MOD_DAMAGERESIST];
			Attributes.at("max_ammo").Int += Bonus[MOD_MAXAMMO];
			Attributes.at("move_speed").Int += Bonus[MOD_MOVESPEED];
			Attributes.at("move_speed").Int = std::clamp(Attributes.at("move_speed").Int, -ITEM_MAX_MOVESPEED, ITEM_MAX_MOVESPEED);
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

			// Reload amount only affects manual reload weapons
			if(ModType == MOD_RELOADAMOUNT && !Template.Attributes.at("reload_amount").Int)
				return false;

			if(ModType == MOD_ACCURACY && IsMelee())
				return false;

		} break;
	}
	Mods.push_back(Mod);
	RecalculateStats();

	return true;
}

// Get bonus multiplier from mod type and quality
float _Item::GetBonusMultiplier(int ModType, bool Inverse) const {
	float Factor = ((100 + Quality) * 0.01f) * ((100 + Bonus[ModType]) * 0.01f);
	return Inverse ? 1.0f / Factor : Factor;
}

// Get average damage from range
float _Item::GetAverageDamage() const {
	return (Attributes.at("min_damage").Int + Attributes.at("max_damage").Int) * 0.5f;
}

// Get average accuracy from range
float _Item::GetAverageAccuracy() const {
	return (Attributes.at("min_accuracy").Float + Attributes.at("max_accuracy").Float) * 0.5f;
}

// Get weapon sound for a sound type
const ae::_Sound *_Item::GetSound(int SoundType) const {
	const auto &SoundIDs = Template.SoundID[SoundType];
	if(SoundIDs.empty())
		return nullptr;

	return SoundIDs[ae::GetRandomInt((size_t)0, SoundIDs.size()-1)];
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

			return WeaponTypeString;
		} break;
		case _Object::ARMOR:
			return "Armor";
		case _Object::MOD: {
			std::string TypeString;
			switch(Template.Attributes.at("object_type").Int) {
				case _Object::WEAPON:
					TypeString = "Weapon";
				break;
				case _Object::ARMOR:
					TypeString = "Armor";
				break;
			}

			return TypeString + " Mod";
		} break;
		case _Object::AMMO:
			return "Ammo";
		case _Object::KEY:
			return "Key";
		case _Object::MEDKIT:
			return "Healing Item";
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
			return "Accuracy";
		break;
		case MOD_ATTACKSPEED:
			if(Type == _Object::MOD)
				return "Attack/Fire Rate";
			else if(IsMelee())
				return "Attack Speed";
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
