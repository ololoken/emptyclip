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
#include <ui/button.h>
#include <texture.h>
#include <graphics.h>
#include <assets.h>

// Constructor
_Button::_Button(const std::string &Identifier, _Element *Parent, const glm::ivec2 &Offset, const glm::ivec2 &Size, const _Alignment &Alignment, const _Style *Style, const _Style *HoverStyle) :
	_Element(Identifier, Parent, Offset, Size, Alignment, Style, false) {

	this->HoverStyle = HoverStyle;
	this->Enabled = false;
}

// Destructor
_Button::~_Button() {
}

// Render the element
void _Button::Render() const {

	if(Style) {
		if(Style->Texture) {
			Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
			Graphics.SetColor(Style->TextureColor);
			Graphics.DrawImage(Bounds, Style->Texture, Style->Stretch);
		}
		else {
			Graphics.SetProgram(Assets.Programs["ortho_pos"]);
			Graphics.SetColor(Style->BackgroundColor);
			Graphics.DrawRectangle(Bounds, true);
			Graphics.SetColor(Style->BorderColor);
			Graphics.DrawRectangle(Bounds, false);
		}
	}

	// Draw hover texture
	if(HoverStyle && (Enabled || HitElement)) {
		if(HoverStyle->Texture) {
			Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
			Graphics.SetColor(HoverStyle->TextureColor);
			Graphics.DrawImage(Bounds, HoverStyle->Texture, Style->Stretch);
		}
		else {
			if(HoverStyle->HasBackgroundColor) {
				Graphics.SetProgram(Assets.Programs["ortho_pos"]);
				Graphics.SetColor(HoverStyle->BackgroundColor);
				Graphics.DrawRectangle(Bounds, true);
			}

			if(HoverStyle->HasBorderColor) {
				Graphics.SetProgram(Assets.Programs["ortho_pos"]);
				Graphics.SetColor(HoverStyle->BorderColor);
				Graphics.DrawRectangle(Bounds, false);
			}
		}
	}

	// Render all children
	for(size_t i = 0; i < Children.size(); i++) {
		Children[i]->Render();
	}
}
