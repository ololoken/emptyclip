/******************************************************************************
* Empty Clip
* Copyright (C) 2024 Alan Witkowski
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
#include <ae/assets.h>
#include <ae/audio.h>
#include <ae/buffer.h>
#include <ae/font.h>
#include <ae/graphics.h>
#include <ae/random.h>
#include <ae/util.h>
#include <objects/player.h>
#include <states/play.h>
#include <config.h>
#include <constants.h>
#include <hud.h>
#include <stats.h>
#include <algorithm>
#include <iomanip>

static const float ATTRIBUTE_SPACING = 26;

// Function for sorting mods
inline bool CompareModType(_Item *First, _Item *Second) {
	if(First->Template.Attributes.at("mod_type").Int == Second->Template.Attributes.at("mod_type").Int)
		return First->Quality > Second->Quality;

	return First->Template.Attributes.at("mod_type").Int < Second->Template.Attributes.at("mod_type").Int;
}

// Function for sorting mods by quality
inline bool CompareModQuality(_Item *First, _Item *Second) {
	return First->Quality > Second->Quality;
}

// Constructor
_Item::_Item(const _ObjectTemplate &ItemTemplate) :
	_Object(ItemTemplate) {

	Radius = ITEM_RADIUS;
	PositionZ = ITEM_Z;

	if(CanEquip())
		FilterType = 0;
	else if(Type == _Object::MOD)
		FilterType = 1;
}

// Destructor
_Item::~_Item() {
	for(size_t i = 0; i < Mods.size(); i++)
		delete Mods[i];
}

// Draw attribute text
void _Item::DrawAttribute(const ae::_Font *Font, bool Float, const std::string &Attribute, const std::string &Label, glm::vec2 &DrawPosition, const _Item *EquippedItem, bool Plus, bool Percent) const {
	float Value = Float ? Attributes.at(Attribute).Float : Attributes.at(Attribute).Int;
	if(!Value)
		return;

	// Get color
	glm::vec4 TextColor = COLOR_WHITE;
	if(EquippedItem) {
		float EquippedValue = Float ? EquippedItem->Attributes.at(Attribute).Float : EquippedItem->Attributes.at(Attribute).Int;
		if(Value > EquippedValue)
			TextColor = COLOR_GREEN;
		else if(Value < EquippedValue)
			TextColor = COLOR_RED;
	}

	// Get text
	std::ostringstream Buffer;
	Buffer.imbue(std::locale(Config.Locale));
	if(Plus && Value > 0.0f)
		Buffer << "+";

	if(PlayState.ShowMoreInfo())
		Buffer << ae::Round2(Value);
	else
		Buffer << std::round(Value);

	if(Percent)
		Buffer << "%";

	// Draw
	glm::vec2 DrawOffset(8 * ae::_Element::GetUIScale(), 0);
	DrawPosition.y += ATTRIBUTE_SPACING * ae::_Element::GetUIScale();
	Font->DrawText(Label, glm::ivec2(DrawPosition - DrawOffset), ae::RIGHT_BASELINE);
	Font->DrawText(Buffer.str(), glm::ivec2(DrawPosition + DrawOffset), ae::LEFT_BASELINE, TextColor);
}

// Draw the item popup window
void _Item::DrawTooltip(const _Player *Player, glm::vec2 DrawPosition, const _Item *EquippedItem, const _Slot &Slot, bool ShowHelp, bool ShowEquipHelp) const {
	std::ostringstream Buffer;
	Buffer.imbue(std::locale(Config.Locale));

	glm::vec2 Size = glm::vec2(470, 150) * ae::_Element::GetUIScale();
	glm::vec2 Spacing = glm::vec2(0, ATTRIBUTE_SPACING) * ae::_Element::GetUIScale();
	glm::vec2 SmallSpacing = glm::vec2(0, 20) * ae::_Element::GetUIScale();
	glm::vec2 HelpSpacing = glm::vec2(0, 24) * ae::_Element::GetUIScale();

	const ae::_Font *AttributeFont = ae::Assets.Fonts["hud_damage"];
	const ae::_Font *LargeFont = ae::Assets.Fonts["hud_medium"];
	const ae::_Font *SmallFont = ae::Assets.Fonts["hud_char"];

	// Set size based on type
	if(Type == _Object::WEAPON) {
		Size.y = 526 * ae::_Element::GetUIScale();
		if(Attributes.at("reload_amount").Float > 0.0f)
			Size.y += Spacing.y;
		if(Attributes.at("attack_count").Float > 1.0f)
			Size.y += Spacing.y;
	}
	else if(Type == _Object::ARMOR)
		Size.y = 380 * ae::_Element::GetUIScale();
	else if(Type == _Object::CONSUMABLE)
		Size.y = 180 * ae::_Element::GetUIScale();
	else if(Type == _Object::USABLE) {
		Size.x = 480 * ae::_Element::GetUIScale();
		Size.y = 240 * ae::_Element::GetUIScale();
		if(Template.Attributes.at("usable_type").Int == USABLE_DYNAMITE)
			Size.y += Spacing.y;
		else if(Template.Attributes.at("usable_type").Int == USABLE_PLIERS)
			Size.x += 50 * ae::_Element::GetUIScale();
	}
	else if(Type == _Object::MOD) {
		Size.x = 480 * ae::_Element::GetUIScale();
		Size.y = 250 * ae::_Element::GetUIScale();
	}

	// Get title width
	ae::_TextBounds TextBounds;
	LargeFont->GetStringDimensions(Name, TextBounds);
	Size.x = std::max(Size.x, (float)TextBounds.Width) + 20 * ae::_Element::GetUIScale();

	// Calculate mod icon sizes
	glm::vec2 ModIconSize;
	glm::vec2 ModIconSpacing;
	float ModIconsHeight = 0.0f;
	int ModColumns = 0;
	if(CanMod()) {
		if(Mods.size() > 532)
			ModIconSize = glm::vec2(8, 8);
		else if(Mods.size() > 345)
			ModIconSize = glm::vec2(16, 16);
		else if(Mods.size() > 228)
			ModIconSize = glm::vec2(20, 20);
		else if(Mods.size() > 126)
			ModIconSize = glm::vec2(24, 24);
		else
			ModIconSize = glm::vec2(32, 32);

		ModIconSpacing = ModIconSize * ae::_Element::GetUIScale();
		ModColumns = std::ceil((Size.x - ModIconSpacing.x * 1.5f) / ModIconSpacing.x);
		int Rows = std::ceil((float)Mods.size() / ModColumns);
		ModIconsHeight = std::max(0, Rows - 3) * ModIconSpacing.y;
	}

	// Calculate mod text sizes
	float ModTextHeight = 0.0f;
	bool HasOneBonus = false;
	for(int i = 1; i < MOD_COUNT; i++) {
		if(Bonus[i]) {
			ModTextHeight += SmallSpacing.y;
			HasOneBonus = true;
		}
	}
	if(HasOneBonus)
		ModTextHeight += Spacing.y;

	// Increase window height by tallest mod section
	Size.y += std::max(ModIconsHeight, ModTextHeight);

	// Offset position
	float WindowOffsetX = 20 * ae::_Element::GetUIScale();
	float MinX = 0;
	DrawPosition.x += WindowOffsetX;
	DrawPosition.y -= Size.y/2;

	// Move comparison window
	if(EquippedItem)
		MinX = Size.x;

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
	std::string DrawName = Name;
	glm::vec4 DrawColor = glm::vec4(1.0f);
	if(Unique)
		DrawColor = LightColor;
	LargeFont->DrawText(DrawName, glm::ivec2(DrawPosition), ae::CENTER_BASELINE, DrawColor);

	// Draw type
	DrawPosition.y += SmallSpacing.y;
	SmallFont->DrawText(GetTypeAsString(), glm::ivec2(DrawPosition), ae::CENTER_BASELINE);

	// Draw Level
	if(CanLevel()) {
		DrawPosition.y += SmallSpacing.y;
		Buffer << "Level " << Level;
		SmallFont->DrawText(Buffer.str(), glm::ivec2(DrawPosition), ae::CENTER_BASELINE);
		Buffer.str("");
	}

	// Quality
	glm::vec4 TextColor = COLOR_WHITE;
	if(CanQuality()) {
		DrawPosition.y += SmallSpacing.y;
		if(CanDynamite() && PlayState.ShowMoreInfo() && Mods.size())
			Buffer << "Dynamite Quality " << GetTotalQuality() << "%";
		else
			Buffer << "Quality " << Quality << "%";
		SmallFont->DrawText(Buffer.str(), glm::ivec2(DrawPosition), ae::CENTER_BASELINE, DrawColor);
		Buffer.str("");
	}

	DrawPosition.y += 10 * ae::_Element::GetUIScale();
	glm::vec2 DrawOffset(8 * ae::_Element::GetUIScale(), 0);

	// Add help text
	std::vector<std::string> HelpTextList;
	if(ShowHelp && Slot.Bag && Slot.IsGearSlot())
		HelpTextList.push_back("Right-click to unequip");

	// Show attributes
	switch(Type) {
		case _Object::WEAPON: {
			if(ShowHelp && ShowEquipHelp) {
				HelpTextList.push_back("Right-click to equip");
				HelpTextList.push_back("Ctrl+Right-click to equip off-hand");
			}

			// Damage
			TextColor = COLOR_WHITE;
			if(EquippedItem) {
				if(GetAverageDamage() > EquippedItem->GetAverageDamage())
					TextColor = COLOR_GREEN;
				else if(GetAverageDamage() < EquippedItem->GetAverageDamage())
					TextColor = COLOR_RED;
			}
			DrawPosition.y += Spacing.y;

			double AverageDamage = GetAverageDamage();
			if(PlayState.ShowMoreInfo())
				Buffer << ae::Round1(AverageDamage) << " avg";
			else
				Buffer << Attributes.at("min_damage").Int << " - " << Attributes.at("max_damage").Int;
			const ae::_Font *Font = (AverageDamage >= 1e6) ? SmallFont : AttributeFont;
			Font->DrawText("Damage", glm::ivec2(DrawPosition - DrawOffset), ae::RIGHT_BASELINE);
			Font->DrawText(Buffer.str(), glm::ivec2(DrawPosition + DrawOffset), ae::LEFT_BASELINE, TextColor);
			Buffer.str("");

			// Clip size
			if(Attributes.at("rounds").Float) {
				TextColor = COLOR_WHITE;
				if(EquippedItem) {
					if(Attributes.at("rounds").Float > EquippedItem->Attributes.at("rounds").Float)
						TextColor = COLOR_GREEN;
					else if(Attributes.at("rounds").Float < EquippedItem->Attributes.at("rounds").Float)
						TextColor = COLOR_RED;
				}
				DrawPosition.y += Spacing.y;
				if(PlayState.ShowMoreInfo())
					Buffer << Attributes.at("ammo").Int << "/" << Attributes.at("rounds").Float;
				else
					Buffer << Attributes.at("ammo").Int << "/" << std::round(Attributes.at("rounds").Float);
				AttributeFont->DrawText("Rounds", glm::ivec2(DrawPosition - DrawOffset), ae::RIGHT_BASELINE);
				AttributeFont->DrawText(Buffer.str(), glm::ivec2(DrawPosition + DrawOffset), ae::LEFT_BASELINE, TextColor);
				Buffer.str("");
			}

			// Critical hit chance
			if(Attributes.at("crit_chance").Float)
				DrawAttribute(AttributeFont, true, "crit_chance", "Critical Hit Chance", DrawPosition, EquippedItem, false, true);

			// Attack count
			if(Attributes.at("attack_count").Float > 1.0f)
				DrawAttribute(AttributeFont, true, "attack_count", IsMelee() ? "Attacks" : "Bullets Shot", DrawPosition, EquippedItem, false, false);

			// Penetration
			if(Template.Attributes.at("explosion_size").Float == 0.0f) {
				DrawAttribute(AttributeFont, true, "penetration", "Penetration", DrawPosition, EquippedItem, false, false);

				// Penetration Damage
				TextColor = COLOR_WHITE;
				if(EquippedItem) {
					if(Attributes.at("penetration_damage").Float > EquippedItem->Attributes.at("penetration_damage").Float)
						TextColor = COLOR_GREEN;
					else if(Attributes.at("penetration_damage").Float < EquippedItem->Attributes.at("penetration_damage").Float)
						TextColor = COLOR_RED;
				}

				DrawPosition.y += Spacing.y;
				Buffer << ae::Round1(100 * Attributes.at("penetration_damage").Float) << "%";
				AttributeFont->DrawText("Penetration Damage", glm::ivec2(DrawPosition - DrawOffset), ae::RIGHT_BASELINE);
				AttributeFont->DrawText(Buffer.str(), glm::ivec2(DrawPosition + DrawOffset), ae::LEFT_BASELINE, TextColor);
				Buffer.str("");
			}

			// Fire rate
			if(Attributes.at("fire_period").Double > 0.0) {
				TextColor = COLOR_WHITE;
				if(EquippedItem) {
					if(Attributes.at("fire_period").Double < EquippedItem->Attributes.at("fire_period").Double)
						TextColor = COLOR_GREEN;
					else if(Attributes.at("fire_period").Double > EquippedItem->Attributes.at("fire_period").Double)
						TextColor = COLOR_RED;
				}

				DrawPosition.y += Spacing.y;
				Buffer << ae::Round2(1 / std::max(WEAPON_MINFIREPERIOD, Attributes.at("fire_period").Double)) << "/s";
				std::string AttackCountText;
				if(IsMelee())
					AttackCountText = "Attack Speed";
				else
					AttackCountText = "Fire Rate";
				AttributeFont->DrawText(AttackCountText, glm::ivec2(DrawPosition - DrawOffset), ae::RIGHT_BASELINE);
				AttributeFont->DrawText(Buffer.str(), glm::ivec2(DrawPosition + DrawOffset), ae::LEFT_BASELINE, TextColor);
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
				if(PlayState.ShowMoreInfo())
					Buffer << ae::Round2(GetAverageAccuracy()) << " avg";
				else
					Buffer << ae::Round2(Attributes.at("accuracy_min").Float) << " - " << ae::Round2(Attributes.at("accuracy_max").Float) << " deg";
				AttributeFont->DrawText("Accuracy", glm::ivec2(DrawPosition - DrawOffset), ae::RIGHT_BASELINE);
				AttributeFont->DrawText(Buffer.str(), glm::ivec2(DrawPosition + DrawOffset), ae::LEFT_BASELINE, TextColor);
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
				Buffer << ae::Round2(Attributes.at("recoil").Float) << " deg";
				AttributeFont->DrawText("Recoil", glm::ivec2(DrawPosition - DrawOffset), ae::RIGHT_BASELINE);
				AttributeFont->DrawText(Buffer.str(), glm::ivec2(DrawPosition + DrawOffset), ae::LEFT_BASELINE, TextColor);
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
				AttributeFont->DrawText("Range", glm::ivec2(DrawPosition - DrawOffset), ae::RIGHT_BASELINE);
				AttributeFont->DrawText(Buffer.str(), glm::ivec2(DrawPosition + DrawOffset), ae::LEFT_BASELINE, TextColor);
				Buffer.str("");
			}

			// Explosion Size
			if(Attributes.at("explosion_size").Float > 0.0f) {
				TextColor = COLOR_WHITE;
				if(EquippedItem) {
					if(Attributes.at("explosion_size").Float > EquippedItem->Attributes.at("explosion_size").Float)
						TextColor = COLOR_GREEN;
					else if(Attributes.at("explosion_size").Float < EquippedItem->Attributes.at("explosion_size").Float)
						TextColor = COLOR_RED;
				}

				DrawPosition.y += Spacing.y;
				Buffer << Attributes.at("explosion_size").Float;
				AttributeFont->DrawText("Explosion Size", glm::ivec2(DrawPosition - DrawOffset), ae::RIGHT_BASELINE);
				AttributeFont->DrawText(Buffer.str(), glm::ivec2(DrawPosition + DrawOffset), ae::LEFT_BASELINE, TextColor);
				Buffer.str("");
			}

			// Reload Amount
			if(Attributes.at("reload_amount").Float > 0.0)
				DrawAttribute(AttributeFont, true, "reload_amount", "Reload Amount", DrawPosition, EquippedItem, false, false);

			// Reload speed
			if(Attributes.at("reload_period").Double > 0.0) {
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
				AttributeFont->DrawText("Reload Time", glm::ivec2(DrawPosition - DrawOffset), ae::RIGHT_BASELINE);
				AttributeFont->DrawText(Buffer.str(), glm::ivec2(DrawPosition + DrawOffset), ae::LEFT_BASELINE, TextColor);
				Buffer.str("");
			}

			// Ammo type
			if(!Template.AmmoID.empty()) {
				DrawPosition.y += Spacing.y;
				Buffer << Stats.Ammo.at(Template.AmmoID).Name;
				AttributeFont->DrawText("Ammo Type", glm::ivec2(DrawPosition - DrawOffset), ae::RIGHT_BASELINE);
				AttributeFont->DrawText(Buffer.str(), glm::ivec2(DrawPosition + DrawOffset));
				Buffer.str("");
			}
		} break;
		case _Object::ARMOR: {
			if(ShowHelp && ShowEquipHelp)
				HelpTextList.push_back("Right-click to equip");

			DrawAttribute(AttributeFont, true, "damage_block", "Damage Block", DrawPosition, EquippedItem, false, false);
			DrawAttribute(AttributeFont, true, "damage_resist", "Damage Resist", DrawPosition, EquippedItem, true, true);
			DrawAttribute(AttributeFont, true, "move_speed", "Move Speed", DrawPosition, EquippedItem, true, true);
			DrawAttribute(AttributeFont, true, "max_stamina", "Max Stamina", DrawPosition, EquippedItem, true, true);
			DrawAttribute(AttributeFont, true, "max_ammo", "Max Ammo", DrawPosition, EquippedItem, true, true);
			DrawAttribute(AttributeFont, true, "max_health", "Max Health", DrawPosition, EquippedItem, true, true);
		} break;
		case _Object::MOD: {
			if(ShowHelp) {
				if(Template.Attributes.at("object_type").Int == _Object::WEAPON)
					HelpTextList.push_back("Drag onto weapon");
				else if(Template.Attributes.at("object_type").Int == _Object::ARMOR)
					HelpTextList.push_back("Drag onto armor");

				if(!Slot.Bag && PlayState.HUD->InventoryOpen)
					HelpTextList.push_back("Right-click to pick up");
			}

			if(GetModType() == MOD_BURST) {
				DrawPosition.y += Spacing.y;
				Buffer << "+";
				if(PlayState.ShowMoreInfo())
					Buffer << Attributes.at("bonus").Float;
				else
					Buffer << std::round(Attributes.at("bonus").Float);
				Buffer << " Round Burst Fire";
				AttributeFont->DrawText(Buffer.str(), glm::ivec2(DrawPosition), ae::CENTER_BASELINE, TextColor);
			}
			else if(!IsChangeMod()) {
				DrawPosition.y += Spacing.y;
				std::string Percent = Template.Attributes.at("percent_sign").Int ? "%" : "";
				std::string Positive = Template.Attributes.at("negative").Int ? "" : "+";
				Buffer << Positive << ae::Round2(Attributes.at("bonus").Float) << Percent;
				AttributeFont->DrawText(ModTypeToString(GetModType()), glm::ivec2(DrawPosition - DrawOffset), ae::RIGHT_BASELINE);
				AttributeFont->DrawText(Buffer.str(), glm::ivec2(DrawPosition + DrawOffset), ae::LEFT_BASELINE, TextColor);
			}
			Buffer.str("");

			if(Attributes.at("bonus_2nd").Float) {
				DrawPosition.y += Spacing.y;
				Buffer << "+" << ae::Round2(Attributes.at("bonus_2nd").Float) << "%";
				AttributeFont->DrawText(ModTypeToString(Template.Attributes.at("mod_type_2nd").Int), glm::ivec2(DrawPosition - DrawOffset), ae::RIGHT_BASELINE);
				AttributeFont->DrawText(Buffer.str(), glm::ivec2(DrawPosition + DrawOffset), ae::LEFT_BASELINE, TextColor);
				Buffer.str("");
			}
		} break;
		case _Object::AMMO: {
			DrawPosition.y += Spacing.y;
			if(Unique)
				Buffer << "Restores all " << Template.Name;
			else
				Buffer << "+" << Attributes.at("amount").Int;
			AttributeFont->DrawText(Buffer.str(), glm::ivec2(DrawPosition), ae::CENTER_BASELINE);
			Buffer.str("");
		} break;
		case _Object::CONSUMABLE: {
			if(ShowHelp)
				HelpTextList.push_back("Used when picked up");

			DrawPosition.y += Spacing.y;

			float Value = GetConsumableValue(Player);
			if(Unique)
				Buffer << "+100";
			else
				Buffer << "+" << ae::Round2(Value);

			Buffer << GetConsumableSuffix(true);
			AttributeFont->DrawText(Buffer.str(), glm::ivec2(DrawPosition), ae::CENTER_BASELINE, COLOR_GREEN);
			Buffer.str("");
		} break;
		case _Object::USABLE: {
			if(ShowHelp)
				HelpTextList.push_back("Drag onto item");

			DrawPosition.y += Spacing.y;

			switch(Template.Attributes.at("usable_type").Int) {
				case USABLE_HAMMER: {
					AttributeFont->DrawText("Destroys an item and releases its mods", glm::ivec2(DrawPosition), ae::CENTER_BASELINE);
					DrawPosition.y += Spacing.y;
					int Change = GetHammerQualityChange();
					if(Change != 0) {
						Buffer << (Change > 0 ? "Increases" : "Reduces");
						Buffer << " quality of mods by [c green]" << std::abs(Change) << "%";
						AttributeFont->DrawTextFormatted(Buffer.str(), glm::ivec2(DrawPosition), ae::CENTER_BASELINE);
						Buffer.str("");
					}
				} break;
				case USABLE_WHETSTONE:
					Buffer << "Increases the quality of an item by [c green]" << GetWhetstoneQuality() << "%";
					AttributeFont->DrawTextFormatted(Buffer.str(), glm::ivec2(DrawPosition), ae::CENTER_BASELINE);
					Buffer.str("");

					DrawPosition.y += Spacing.y;
					Buffer << "Max quality for Progression [c green]" << Player->Progression << "[c white] is [c green]" << Stats.Progressions[(size_t)Player->Progression].MaxQuality << "%";
					AttributeFont->DrawTextFormatted(Buffer.str(), glm::ivec2(DrawPosition), ae::CENTER_BASELINE);
					Buffer.str("");
					if(ShowHelp && !Slot.Bag && PlayState.HUD->InventoryOpen)
						HelpTextList.push_back("Right-click to pick up");
				break;
				case USABLE_WRENCH:
					Buffer << "Increases the level of an item by [c green]" << GetWrenchLevel();
					AttributeFont->DrawTextFormatted(Buffer.str(), glm::ivec2(DrawPosition), ae::CENTER_BASELINE);
					Buffer.str("");

					DrawPosition.y += Spacing.y;
					Buffer << "Max level for Progression [c green]" << Player->Progression << "[c white] is [c green]" << Stats.Progressions[(size_t)Player->Progression].MaxLevel;
					AttributeFont->DrawTextFormatted(Buffer.str(), glm::ivec2(DrawPosition), ae::CENTER_BASELINE);
					Buffer.str("");
					if(ShowHelp && !Slot.Bag && PlayState.HUD->InventoryOpen)
						HelpTextList.push_back("Right-click to pick up");
				break;
				case USABLE_DYNAMITE: {
					AttributeFont->DrawText("Dismantles a unique item into upgrade parts", glm::ivec2(DrawPosition), ae::CENTER_BASELINE);
					DrawPosition.y += Spacing.y;
					Buffer << "Creates an upgrade for every [c green]" << GetDynamiteQuality() << "%[c white] quality";
					AttributeFont->DrawTextFormatted(Buffer.str(), glm::ivec2(DrawPosition), ae::CENTER_BASELINE);
					Buffer.str("");

					DrawPosition.y += Spacing.y;
					Buffer << "Unique mods inside gear contribute " << std::round(ITEM_DYNAMITE_MOD_FACTOR * 100) << "% of their quality";
					SmallFont->DrawText(Buffer.str(), glm::ivec2(DrawPosition), ae::CENTER_BASELINE, COLOR_GRAY);
					Buffer.str("");
				} break;
				case USABLE_PLIERS: {
					int PliersLevel = GetPliersLevel();
					if(PliersLevel > 1)
						Buffer << "Removes the [c green]" << PliersLevel << "[c white] lowest quality mods from an item";
					else
						Buffer << "Removes the lowest quality mod from an item";

					AttributeFont->DrawTextFormatted(Buffer.str(), glm::ivec2(DrawPosition), ae::CENTER_BASELINE);
					Buffer.str("");

					if(ShowHelp && !Slot.Bag && PlayState.HUD->InventoryOpen)
						HelpTextList.push_back("Right-click to pick up");
				} break;
			}
		} break;
	}

	// Add help text
	if(ShowHelp && Slot.IsValidIndex())
		HelpTextList.push_back("Ctrl+click to drop");

	// Mods
	if(CanMod()) {
		TextColor = COLOR_WHITE;
		if(EquippedItem) {
			if(Attributes.at("max_mods").Float > EquippedItem->Attributes.at("max_mods").Float)
				TextColor = COLOR_GREEN;
			else if(Attributes.at("max_mods").Float < EquippedItem->Attributes.at("max_mods").Float)
				TextColor = COLOR_RED;
		}

		DrawPosition.y += Spacing.y;

		const ae::_Font *Font;
		if(PlayState.ShowMoreInfo()) {
			float TotalMods = GetMaxMods(false);
			Buffer
				<< ae::Round2(TotalMods)
				<< " = " << ae::Round2(TotalMods - ExtraMods - Player->ExtraMods)
				<< "+" << ae::Round2(ExtraMods)
				<< "+" << ae::Round2(Player->ExtraMods);
			Font = SmallFont;
		}
		else {
			Buffer << Mods.size() << "/" << GetMaxMods(true);
			Font = AttributeFont;
		}
		AttributeFont->DrawText("Mods", glm::ivec2(DrawPosition - DrawOffset), ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), glm::ivec2(DrawPosition + DrawOffset), ae::LEFT_BASELINE, TextColor);
		Buffer.str("");
	}

	if(HasOneBonus)
		DrawPosition.y += Spacing.y * 0.5f;

	// Show mod icons
	if(PlayState.ShowMoreInfo()) {

		// Set up ui parameters
		glm::vec2 IconStart = DrawPosition + glm::vec2(-Size.x * 0.5f + ModIconSpacing.x, ModIconSpacing.y * 0.75f);
		glm::vec2 IconOffset(0.0f);

		// Draw icons
		int Column = 0;
		for(const auto &Mod : Mods) {
			glm::vec2 IconPosition = IconStart + IconOffset;
			ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
			ae::Graphics.DrawScaledImage(IconPosition, Mod->Texture, ModIconSize, Mod->Color);

			// Draw highlight
			if(Mod->Unique) {
				glm::vec4 HighlightColor(Mod->LightColor.r, Mod->LightColor.g, Mod->LightColor.b, ITEM_HIGHLIGHT_ALPHA);
				ae::Graphics.DrawScaledImage(IconPosition, ae::Assets.Textures["textures/lights/circle.png"], ModIconSize * ITEM_HIGHLIGHT_SCALE, HighlightColor);
			}

			// Update position
			Column++;
			IconOffset.x += ModIconSpacing.x;
			if(Column >= ModColumns) {
				Column = 0;
				IconOffset.x = 0.0f;
				IconOffset.y += ModIconSpacing.y;
			}
		}

		// Hide rest of text
		if(Mods.size())
			return;
	}
	// Mod bonuses
	else {
		TextColor = glm::vec4(0.85f, 0.85f, 0.85f, 1.0f);
		for(size_t i = 1; i < MOD_COUNT; i++) {
			if(!Bonus[i])
				continue;

			_ObjectTemplate &ModTemplate = Stats.Objects.at(Stats.ModNames[i]);

			DrawPosition.y += SmallSpacing.y;

			// Set bonus
			int ModType = ModTemplate.Attributes.at("mod_type").Int;
			if(ModType == MOD_BURST) {
				Buffer << Bonus[i] << " Round ";
			}
			else if(ModType != MOD_FULLAUTO) {
				std::string Percent = ModTemplate.Attributes.at("percent_sign").Int ? "% " : " ";
				std::string Positive = ModTemplate.Attributes.at("negative").Int ? "" : "+";

				Buffer << Positive << ae::Round2(Bonus[i]) << Percent;
			}

			// Set label
			Buffer << ModTypeToString((int)i);
			SmallFont->DrawText(Buffer.str(), glm::ivec2(DrawPosition), ae::CENTER_BASELINE, TextColor);
			Buffer.str("");
		}
	}

	// Draw ui hints
	DrawPosition.y += 50 * ae::_Element::GetUIScale();
	for(const auto &Text : HelpTextList) {
		SmallFont->DrawText(Text, glm::ivec2(DrawPosition), ae::CENTER_BASELINE, COLOR_GRAY);
		DrawPosition.y += HelpSpacing.y;
	}

	if(!Carry)
		SmallFont->DrawText("Cannot be placed in inventory", glm::ivec2(DrawPosition), ae::CENTER_BASELINE, COLOR_RED);
}

// Draws the object
void _Item::Render(double BlendFactor) const {
	if(!Visible)
		return;

	glm::vec4 RenderColor = Color;
	RenderColor.a = (Filtered && !AmmoPickup) ? ITEM_FILTERED_ALPHA : 1.0f;

	ae::Graphics.SetColor(RenderColor);
	ae::Graphics.DrawSprite(glm::vec3(Position, PositionZ), Texture, Rotation, glm::vec2(ITEM_SCALE));
}

// Serialize for saving
void _Item::Serialize(ae::_Buffer &Buffer) {
	Buffer.WriteString(ID.c_str());
	Buffer.Write<int32_t>(Level);
	Buffer.Write<int32_t>(Quality);

	// Write mods
	if(Type == _Object::WEAPON || Type == _Object::ARMOR) {
		Buffer.Write<float>(ExtraMods);
		Buffer.Write<int32_t>((int32_t)Mods.size());
		for(size_t i = 0; i < Mods.size(); i++)
			Mods[i]->Serialize(Buffer);
	}

	// Write ammo
	if(Type == _Object::WEAPON)
		Buffer.Write(Attributes.at("ammo").Int);
}

// Recalulate stats for item
void _Item::RecalculateStats() {

	// Set unique stats
	Unique = Stats.GetUnique(Quality);
	if(Unique) {
		LightTexture = Unique->Texture;
		if(CanAutoPickup()) {
			LightColor = COLOR_GOLD;
			Name = Template.Name + " Bundle";
		}
		else {
			LightColor = Unique->Color;
			Name = Unique->Name + " " + Template.Name;
		}
	}
	else {
		LightTexture = nullptr;
		Name = Template.Name;
	}

	// Recalculate max mods
	SetMaxMods();

	// Sort mods
	std::sort(Mods.begin(), Mods.end(), CompareModType);

	for(int i = 0; i < MOD_COUNT; i++)
		Bonus[i] = 0.0f;

	// Sum bonuses
	for(size_t i = 0; i < Mods.size(); i++)
		Bonus[Mods[i]->GetModType()] += Mods[i]->Attributes.at("bonus").Float;

	// Sum secondary bonuses
	for(size_t i = 0; i < Mods.size(); i++)
		Bonus[Mods[i]->Template.Attributes.at("mod_type_2nd").Int] += Mods[i]->Attributes.at("bonus_2nd").Float;

	float QualityFactor = 1.0f + Quality * 0.01f;
	switch(Type) {
		case _Object::WEAPON: {
			float MinAccuracyMultiplier = 1.0f / (QualityFactor * std::max(0.0f, (100.0f + Bonus[MOD_ACCURACY] + Bonus[MOD_SPREAD])) * 0.01f);

			Attributes["zoom_scale"].Float = Template.Attributes.at("zoom_scale").Float;
			Attributes["range"].Float = Template.Attributes.at("range").Float;
			Attributes["fire_rate"].Int = Template.Attributes.at("fire_rate").Int;
			Attributes["burst_rounds"].Float = Template.Attributes.at("burst_rounds").Float;
			Attributes["burst_period"].Double = Template.Attributes.at("burst_period").Double;
			Attributes["attack_movespeed"].Float = Template.Attributes.at("attack_movespeed").Float;
			Attributes["fire_allrounds"].Int = Template.Attributes.at("fire_allrounds").Int;
			Attributes["shoot_period"].Double = Template.Attributes.at("shoot_period").Double;

			SetAttributeRange("damage", GetQualityBonusMultiplier(MOD_DAMAGE));
			Attributes["accuracy_min"].Float = Template.Attributes.at("accuracy_min").Float * MinAccuracyMultiplier;
			Attributes["accuracy_max"].Float = std::max(Template.Attributes.at("accuracy_max").Float, Attributes["accuracy_min"].Float);
			Attributes["fire_period"].Double = Template.Attributes.at("fire_period").Double * GetQualityBonusMultiplier(MOD_ATTACKSPEED, true);
			Attributes["shoot_period"].Double = Template.Attributes.at("shoot_period").Double * GetQualityBonusMultiplier(MOD_HANDLING, true);
			Attributes["attack_count"].Float = Template.Attributes.at("attack_count").Float;
			Attributes["reload_period"].Double = Template.Attributes.at("reload_period").Double * GetQualityBonusMultiplier(MOD_RELOADSPEED, true);
			Attributes["reload_amount"].Float = Template.Attributes.at("reload_amount").Float * QualityFactor + Bonus[MOD_RELOADAMOUNT];
			Attributes["recoil"].Float = Template.Attributes.at("recoil").Float * GetQualityBonusMultiplier(MOD_HANDLING, true);
			Attributes["accuracy_regen"].Float = Template.Attributes.at("accuracy_regen").Float * GetQualityBonusMultiplier(MOD_HANDLING);
			Attributes["move_recoil"].Float = Template.Attributes.at("move_recoil").Float * GetQualityBonusMultiplier(MOD_HANDLING, true);
			Attributes["penetration"].Float = Template.Attributes.at("penetration").Float * QualityFactor + Bonus[MOD_PENETRATION];
			Attributes["penetration_damage"].Float = std::clamp(Template.Attributes.at("penetration_damage").Float * QualityFactor, 0.0f, 1.0f);
			Attributes["bounces"].Float = Bonus[MOD_BOUNCE];
			Attributes["crit_chance"].Float = std::clamp(Template.Attributes.at("crit_chance").Float * QualityFactor + Bonus[MOD_CRITCHANCE], 0.0f, 100.0f);
			Attributes["rounds"].Float = Template.Attributes.at("rounds").Float * GetQualityBonusMultiplier(MOD_MAXROUNDS) + Bonus[MOD_MAXROUNDSPLUS];

			if(Bonus[MOD_SEMIAUTO]) {
				Attributes["fire_rate"].Int = 0;
				if(Template.Attributes.at("burst_rounds").Float) {
					Attributes["burst_rounds"].Float = 0.0f;
					Attributes["fire_period"].Double = Attributes["fire_period"].Double / Template.Attributes.at("burst_rounds").Float;
				}
			}

			if(Bonus[MOD_FULLAUTO]) {
				Attributes["fire_rate"].Int = 1;
				if(Template.Attributes.at("burst_rounds").Float) {
					Attributes["burst_rounds"].Float = 0.0f;
					Attributes["fire_period"].Double = Attributes["fire_period"].Double / Template.Attributes.at("burst_rounds").Float;
				}
			}

			if(Bonus[MOD_BURST]) {
				Attributes["burst_rounds"].Float = Bonus[MOD_BURST];
				Attributes["burst_period"].Double = Template.Attributes.at("fire_period").Double / Bonus[MOD_BURST];
				Attributes["fire_period"].Double = Attributes["fire_period"].Double * MOD_BURST_FIREPERIOD_FACTOR;
				Attributes["fire_rate"].Int = 0;
			}

			float ExplosionSize = Template.Attributes.at("explosion_size").Float;
			Attributes["explosion_size"].Float = ExplosionSize > 0.0f ? ExplosionSize * GetBonusMultiplier(MOD_EXPLOSION) : 0.0f;

			SetAmmo(Attributes["ammo"].Int);
		} break;
		case _Object::ARMOR:
			SetAttributeLevel("damage_block", QualityFactor);
			SetAttributeLevel("damage_resist", QualityFactor);
			SetAttributeLevel("max_stamina", QualityFactor);
			SetAttributeLevel("max_ammo", QualityFactor);
			SetAttributeLevel("move_speed", QualityFactor);
			SetAttributeLevel("max_health", QualityFactor);
			Attributes.at("damage_block").Float += Bonus[MOD_DAMAGEBLOCK];
			Attributes.at("damage_resist").Float += Bonus[MOD_DAMAGERESIST];
			Attributes.at("max_ammo").Float += Bonus[MOD_MAXAMMO];
			Attributes.at("move_speed").Float += Bonus[MOD_MOVESPEED];
			Attributes.at("move_speed").Float = std::clamp(Attributes.at("move_speed").Float, ITEM_MIN_MOVESPEED, ITEM_MAX_MOVESPEED);
			Attributes.at("max_health").Float += Bonus[MOD_MAXHEALTH];
			Attributes.at("max_stamina").Float += Bonus[MOD_MAXSTAMINA];
			Attributes["melee_damage"].Float = Bonus[MOD_MELEEDAMAGE];
			Attributes["pistol_damage"].Float = Bonus[MOD_PISTOLDAMAGE];
			Attributes["shotgun_damage"].Float = Bonus[MOD_SHOTGUNDAMAGE];
			Attributes["rifle_damage"].Float = Bonus[MOD_RIFLEDAMAGE];
			Attributes["heavy_damage"].Float = Bonus[MOD_HEAVYDAMAGE];
		break;
		case _Object::MOD:
			RecalculateModBonus();
		break;
		case _Object::USABLE:
			if(!Unique && Template.Attributes.at("light").Int) {
				LightColor = Template.LightColor;
				LightTexture = ae::Assets.Textures["textures/lights/circle.png"];
			}
			Carry = Template.Attributes.at("carry").Int;
		break;
	}
}

// Update mod value from quality
void _Item::RecalculateModBonus() {
	float QualityFactor = 1.0f + Quality * 0.01f;

	SetAttributeLevel("bonus", QualityFactor);
	Attributes["bonus_2nd"].Float = Template.Attributes.at("mod_type_2nd").Int ? MOD_SECONDARY_BONUS * QualityFactor : 0.0f;
	if(Template.Attributes.at("negative").Int)
		Attributes.at("bonus").Float = -Attributes.at("bonus").Float;
}

// Add mod to item
bool _Item::AddMod(_Item *Mod, bool Recalculate) {
	if(!ModCompatible(Mod, Recalculate))
		return false;

	Mods.push_back(Mod);

	if(Recalculate)
		RecalculateStats();

	return true;
}

// Apply usable to item
bool _Item::ApplyUsable(_Item *Usable) {
	if(!ItemCompatible(Usable))
		return false;

	switch(Usable->Template.Attributes.at("usable_type").Int) {
		case USABLE_WHETSTONE: {
			Quality = std::clamp(Quality + Usable->GetWhetstoneQuality(), ITEM_QUALITY_MIN, Stats.Progressions[(size_t)PlayState.Player->Progression].MaxQuality);
			ae::Audio.PlaySound(ae::Assets.Sounds["game_whetstone.ogg"]);
		} break;
		case USABLE_WRENCH: {
			Level = std::clamp(Level + Usable->GetWrenchLevel(), 1, Stats.Progressions[(size_t)PlayState.Player->Progression].MaxLevel);
			ae::Audio.PlaySound(ae::Assets.Sounds["game_wrench.ogg"]);
		} break;
		case USABLE_PLIERS: {

			// Sort mods by quality descending
			std::sort(Mods.begin(), Mods.end(), CompareModQuality);

			// Shuffle lowest quality mods
			if(Mods.size() > 1) {
				size_t LowestIndex = 0;
				for(size_t i = Mods.size() - 2; i < Mods.size(); i--) {
					if(Mods[i]->Quality > Mods.back()->Quality) {
						LowestIndex = i + 1;
						break;
					}
				}

				std::shuffle(Mods.begin() + (int)LowestIndex, Mods.end(), ae::RandomGenerator);
			}

			// Drop mods
			int QuantityApplied = 0;
			int PliersLevel = Usable->GetPliersLevel();
			while(Mods.size() && PliersLevel) {
				PlayState.DropItem(Mods.back());
				Mods.pop_back();

				PliersLevel--;
				QuantityApplied++;
			}

			std::ostringstream Buffer;
			Buffer << "-" << QuantityApplied;
			PlayState.GenerateTextParticle(PlayState.Player->Position - glm::vec2(0.0f, 0.5f), Buffer.str());
			ae::Audio.PlaySound(ae::Assets.Sounds["game_pliers.ogg"]);
		} break;
	}

	// Update item stats
	RecalculateStats();

	return true;
}

// Determine if an item is compatible with another item
bool _Item::ItemCompatible(_Item *Item, bool CheckCount) const {
	switch(Item->Type) {
		case _Object::MOD:
			return ModCompatible(Item, CheckCount);
		break;
		case _Object::USABLE:
			switch(Item->Template.Attributes.at("usable_type").Int) {
				case USABLE_HAMMER:
					if(CanMod())
						return true;
				break;
				case USABLE_WHETSTONE:
					if(CanIncreaseQuality(true))
						return true;
				break;
				case USABLE_WRENCH:
					if(CanIncreaseLevel(true))
						return true;
				break;
				case USABLE_DYNAMITE:
					if(CanDynamite() && Quality >= ITEM_DYNAMITE_VALUE)
						return true;
				break;
				case USABLE_PLIERS:
					if(Mods.size())
						return true;
				break;
			}
		break;
	}

	return false;
}

// Determine if an item is compatible with a mod
bool _Item::ModCompatible(_Item *Mod, bool CheckCount) const {
	if(!CanMod())
		return false;

	if(Mod->Type != _Object::MOD)
		return false;

	if(Mod->Template.Attributes.at("object_type").Int != Type)
		return false;

	if(CheckCount && (int)Mods.size() >= GetMaxMods(true))
		return false;

	switch(Type) {
		case _Object::WEAPON: {

			// Check weapon type
			if(Mod->Template.Attributes.at("weapon_type").Int != 0 && Mod->Template.Attributes.at("weapon_type").Int != Template.Attributes.at("weapon_type").Int)
				return false;

			// Check for ammo
			int ModType = Mod->GetModType();
			if(Template.Attributes.at("rounds").Float == 0.0f && (ModType == MOD_MAXROUNDS || ModType == MOD_MAXROUNDSPLUS || ModType == MOD_RELOADSPEED || ModType == MOD_RELOADAMOUNT || ModType == MOD_HANDLING))
				return false;

			// Max rounds doesn't affect "fire all rounds" weapons
			if(ModType == MOD_MAXROUNDS && Template.Attributes.at("fire_allrounds").Int)
				return false;

			// Reload amount only affects manual reload weapons
			if(ModType == MOD_RELOADAMOUNT && !Template.Attributes.at("reload_amount").Float)
				return false;

			// Penetration doesn't affect explosion weapons
			if(ModType == MOD_PENETRATION && Template.Attributes.at("explosion_size").Float > 0.0f)
				return false;

			// Accuracy only affects guns
			if((ModType == MOD_ACCURACY || ModType == MOD_SPREAD) && IsMelee())
				return false;

			// Explosion only affects explosive weapons
			if(ModType == MOD_EXPLOSION && !Template.Attributes.at("explosion_size").Float)
				return false;

			// Semi only affects burst/full automatic
			if(ModType == MOD_SEMIAUTO) {
				if(IsMelee() || (Template.Attributes.at("fire_rate").Int == 0 && !Template.Attributes.at("burst_rounds").Float))
					return false;

				// Check for other mods
				for(const auto &OtherMod : Mods) {
					if(OtherMod->Type == _Object::MOD && OtherMod->IsChangeMod())
						return false;
				}
			}

			// Full auto only affects semiauto weapons
			if(ModType == MOD_FULLAUTO) {
				if(IsMelee() || Template.Attributes.at("fire_rate").Int || Template.Attributes.at("fire_allrounds").Int)
					return false;

				// Check for other mods
				for(const auto &OtherMod : Mods) {
					if(OtherMod->Type == _Object::MOD && OtherMod->IsChangeMod())
						return false;
				}
			}

			// Burst fire
			if(ModType == MOD_BURST) {
				if(IsMelee() || Template.Attributes.at("burst_rounds").Float || Template.Attributes.at("fire_allrounds").Int)
					return false;

				// Check for other mods
				for(const auto &OtherMod : Mods) {
					if(OtherMod->Type == _Object::MOD && OtherMod->IsChangeMod())
						return false;
				}
			}

			// Bounce only affects projectile weapons
			if(ModType == MOD_BOUNCE && Template.ProjectileID.empty())
				return false;

			// Check max
			if(Mod->Template.Attributes.at("max").Int && Bonus[ModType])
				return false;

		} break;
	}

	return true;
}

// Get bonus multiplier from mod type and quality
float _Item::GetQualityBonusMultiplier(int ModType, bool Inverse) const {
	float Factor = (100 + Quality) * 0.01f * (100.0f + Bonus[ModType]) * 0.01f;
	return Inverse ? 1.0f / Factor : Factor;
}

// Get bonus multiplier from mod type
float _Item::GetBonusMultiplier(int ModType, bool Inverse) const {
	float Factor = (100.0f + Bonus[ModType]) * 0.01f;
	return Inverse ? 1.0f / Factor : Factor;
}

// Set mod capacity
void _Item::SetMaxMods() {
	if(!CanMod())
		return;

	// Set mod count from level
	Attributes["max_mods"].Float = Template.Attributes.at("mods").Float + Template.Attributes.at("mods_level").Float * (Level - 1);

	// Add random extra mod count
	Attributes["max_mods"].Float += ExtraMods;

	// Add mods from unique modifier
	if(Unique)
		Attributes["max_mods"].Float += Unique->Mods;
}

// Return the number of mods an item can hold
float _Item::GetMaxMods(bool Round) const {
	float Value = Attributes.at("max_mods").Float;
	if(PlayState.Player)
		Value += PlayState.Player->ExtraMods;

	return Round ? std::round(Value) : Value;
}

// Return type of mod, otherwise 0
int _Item::GetModType() const {
	if(Type != _Object::MOD)
		return 0;

	return Template.Attributes.at("mod_type").Int;
}

// Return true if mod is considered 'change' type
bool _Item::IsChangeMod() const {
	if(Type != _Object::MOD)
		return MODCLASS_NONE;

	return Template.Attributes.at("class").Int == MODCLASS_CHANGE;
}

// Determine if the item can be compared with another item
bool _Item::IsComparable(const _Item *CompareItem) const {
	if(Type != CompareItem->Type)
		return false;

	if((IsMelee() && !CompareItem->IsMelee()) || (CompareItem->IsMelee() && !IsMelee()))
		return false;

	return true;
}

// Determine the bag icon for an item
size_t _Item::GetIconType() const {

	switch(Type) {
		case _Object::WEAPON:
			return BAGICON_GEAR_WEAPON;
		break;
		case _Object::ARMOR:
			return BAGICON_GEAR_ARMOR;
		break;
		case _Object::MOD:
			switch(Template.Attributes.at("class").Int) {
				case MODCLASS_WEAPON:
					return BAGICON_MOD_WEAPON;
				break;
				case MODCLASS_ARMOR:
					return BAGICON_MOD_ARMOR;
				break;
				case MODCLASS_SPECIAL:
					return BAGICON_MOD_SPECIAL;
				break;
				case MODCLASS_CHANGE:
					return BAGICON_MOD_CHANGE;
				break;
			}
		break;
		case _Object::USABLE:
			return BAGICON_USABLE;
		break;
	}

	return BAGICON_DEFAULT;
}

// Get average damage from range
float _Item::GetAverageDamage() const {
	return (Attributes.at("min_damage").Int + Attributes.at("max_damage").Int) * 0.5f;
}

// Get average accuracy from range
float _Item::GetAverageAccuracy() const {
	return (Attributes.at("accuracy_min").Float + Attributes.at("accuracy_max").Float) * 0.5f;
}

// Get quality color
void _Item::GetQualityColor(glm::vec4 &ReturnColor) const {
	if(Quality < 0)
		ReturnColor = ITEM_QUALITY_GOOD_COLOR;
	else if(Quality > 0)
		ReturnColor = ITEM_QUALITY_BAD_COLOR;
	else
		ReturnColor = COLOR_WHITE;
}

// Get change amount from hammer quality
int _Item::GetHammerQualityChange() const {
	if(Unique)
		return Unique->HammerValue;

	return 0;
}

// Get whetstone quality value
int _Item::GetWhetstoneQuality() const {
	if(Unique)
		return Unique->WhetstoneValue;

	return std::max(1, (int)std::round(Template.Attributes.at("range").Float * (Quality + GAME_QUALITY_RANGE) / (float)(GAME_QUALITY_RANGE * 2)));
}

// Get wrench value
int _Item::GetWrenchLevel() const {
	if(Unique)
		return Unique->WrenchValue;

	return 1;
}

// Get dynamite quality value
int _Item::GetDynamiteQuality() const {
	if(Unique)
		return Unique->DynamiteValue;

	return ITEM_DYNAMITE_VALUE;
}

// Get pliers value
int _Item::GetPliersLevel() const {
	if(Unique)
		return Unique->PliersValue;

	return 1;
}

// Get total quality of item for dynamite use
int _Item::GetTotalQuality() const {

	int TotalModQuality = 0;
	for(const auto &Mod : Mods) {
		if(Mod->Quality >= ITEM_DYNAMITE_VALUE)
			TotalModQuality += Mod->Quality;
	}

	return TotalModQuality * ITEM_DYNAMITE_MOD_FACTOR + Quality;
}

// Get consumable value based on attributes
float _Item::GetConsumableValue(const _Player *Player) const {

	if(Template.Attributes.at("health").Float)
		return Template.Attributes.at("health").Float * Player->HealModifier;
	else if(Template.Attributes.at("stamina").Float)
		return Template.Attributes.at("stamina").Float;

	return 0;
}

// Get suffix for consumable description
std::string _Item::GetConsumableSuffix(bool Percent) const {

	std::string Prefix = Percent ? "%" : "";
	if(Template.Attributes.at("health").Float)
		return Prefix + " HP";
	else if(Template.Attributes.at("stamina").Float)
		return "% Stamina";

	return "";
}

// Build consumble particle text string
void _Item::GetConsumableParticleText(std::ostringstream &Buffer, float Amount) const {
	if(Template.Attributes.at("health").Float)
		Buffer << "+" << (int)(Amount) <<  " HP";
	else if(Template.Attributes.at("stamina").Float)
		Buffer << "+Stamina";
}

// Get type as string
std::string _Item::GetTypeAsString() const {

	switch(Type) {
		case _Object::WEAPON: {
			std::string WeaponTypeString;
			switch(Template.Attributes.at("weapon_type").Int) {
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
		case _Object::CONSUMABLE:
			return "Consumable";
		case _Object::USABLE: {
			std::string TypeString;
			switch(Template.Attributes.at("usable_type").Int) {
				case USABLE_WHETSTONE:
				case USABLE_WRENCH:
					TypeString = "Upgrade";
				break;
				default:
					TypeString = "Usable";
				break;
			}

			return TypeString + " Item";
		}
	}

	return "";
}

// Convert a mod type to string
std::string _Item::ModTypeToString(int ModType) const {

	switch(ModType) {
		case MOD_MAXROUNDS:
			return "Max Rounds";
		break;
		case MOD_DAMAGE:
			return "Damage";
		break;
		case MOD_ACCURACY:
			return "Minimum Accuracy";
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
		case MOD_MAXROUNDSPLUS:
			return "Max Rounds";
		break;
		case MOD_EXPLOSION:
			return "Explosion Size";
		break;
		case MOD_MELEEDAMAGE:
			return "Melee Damage";
		break;
		case MOD_PISTOLDAMAGE:
			return "Pistol Damage";
		break;
		case MOD_SHOTGUNDAMAGE:
			return "Shotgun Damage";
		break;
		case MOD_RIFLEDAMAGE:
			return "Rifle Damage";
		break;
		case MOD_HEAVYDAMAGE:
			return "Heavy Damage";
		break;
		case MOD_CRITCHANCE:
			return "Critical Hit Chance";
		break;
		case MOD_FULLAUTO:
			return "Full-Automatic";
		break;
		case MOD_MAXHEALTH:
			return "Max Health";
		break;
		case MOD_MAXSTAMINA:
			return "Max Stamina";
		break;
		case MOD_BURST:
			return "Burst Fire";
		break;
		case MOD_BOUNCE:
			return "Projectile Bounce";
		break;
		case MOD_SPREAD:
			return "Minimum Accuracy";
		break;
		case MOD_SEMIAUTO:
			return "Semi-Automatic";
		break;
	}

	return "";
}

// Set ammo for weapon
void _Item::SetAmmo(int Value) {
	if(Type != _Object::WEAPON)
		return;

	Attributes["ammo"].Int = std::clamp(Value, 0, (int)std::round(Attributes["rounds"].Float));
}

// Return true if weapon is melee
bool _Item::IsMelee() const {
	return Type == _Object::WEAPON && Template.Attributes.at("weapon_type").Int == WEAPON_MELEE;
}
