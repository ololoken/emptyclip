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
#include <ui/label.h>
#include <font.h>

// Constructor
_Label::_Label(const std::string &Identifier, _Element *Parent, const glm::ivec2 &Offset, const glm::ivec2 &Size, const _Alignment &Alignment, const _Font *Font, const glm::vec4 &Color, const std::string &Text) :
	_Element(Identifier, Parent, Offset, Size, Alignment, nullptr, false) {

	this->Font = Font;
	this->Color = Color;
	this->Text = Text;
}

// Destructor
_Label::~_Label() {
}

// Render the element
void _Label::Render() const {
	glm::vec4 RenderColor(Color.r, Color.g, Color.b, Color.a * Fade);
	if(Texts.size()) {

		// Center box
		float LineHeight = Font->MaxHeight + 2;
		float Y = Bounds.Start.y - (int)((LineHeight * Texts.size() - LineHeight) / 2);
		for(size_t i = 0; i < Texts.size(); i++) {
			Font->DrawText(Texts[i], glm::vec2(Bounds.Start.x, Y), Alignment, RenderColor);

			Y += LineHeight;
		}
	}
	else {
		Font->DrawText(Text, Bounds.Start, Alignment,RenderColor);
	}

	_Element::Render();
}

// Break up text into multiple strings
void _Label::SetWrap(float Width) {

	Texts.clear();
	Font->BreakupString(Text, Width, Texts);
}
