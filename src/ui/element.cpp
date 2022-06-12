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
#include <ui/element.h>
#include <ui/style.h>
#include <graphics.h>
#include <input.h>

const _Color DebugColors[] = { COLOR_CYAN, COLOR_YELLOW, COLOR_RED, COLOR_GREEN, COLOR_BLUE };
const int DebugColorCount = sizeof(DebugColors) / sizeof(_Color);

// Constructor for ui element
_Element::_Element(const std::string &Identifier, _Element *Parent, const glm::ivec2 &Offset, const glm::ivec2 &Size, const _Alignment &Alignment, const _Style *Style, bool MaskOutside) {
	if(!Parent)
		Parent = Graphics.GetElement();

	this->Identifier = Identifier;
	this->Parent = Parent;
	this->Offset = Offset;
	this->Size = Size;
	this->Alignment = Alignment;
	this->Style = Style;
	this->MaskOutside = MaskOutside;
	this->Debug = 0;
	this->UserData = 0;
	this->ID = -1;
	this->Fade = 1.0f;
	this->HitElement = nullptr;
	this->PressedElement = nullptr;
	this->ReleasedElement = nullptr;
	this->ChildrenOffset = glm::ivec2(0);

	CalculateBounds();
}

// Destructor
_Element::~_Element() {
}

// Handle key event
void _Element::HandleKeyEvent(const _KeyEvent &KeyEvent) {

	// Pass event to children
	for(size_t i = 0; i < Children.size(); i++) {
		Children[i]->HandleKeyEvent(KeyEvent);
	}
}

// Handle text event
void _Element::HandleTextEvent(const char *Text) {

	// Pass event to children
	for(size_t i = 0; i < Children.size(); i++) {
		Children[i]->HandleTextEvent(Text);
	}
}

// Handle a press event
void _Element::HandleInput(bool Pressed) {

	// Pass event to children
	for(size_t i = 0; i < Children.size(); i++) {
		Children[i]->HandleInput(Pressed);
	}

	// Set pressed element
	if(Pressed)
		PressedElement = HitElement;

	// Get released element
	if(!Pressed && PressedElement && HitElement) {
		ReleasedElement = PressedElement;
		PressedElement = nullptr;
	}
}

// Get the element that was clicked and released
_Element *_Element::GetClickedElement() {
	if(HitElement == ReleasedElement)
		return HitElement;

	return nullptr;
}

// Handle mouse movement
void _Element::Update(double FrameTime, const glm::ivec2 &Mouse) {
	HitElement = nullptr;
	ReleasedElement = nullptr;

	// Test element first
	if(Bounds.Inside(Mouse)) {
		HitElement = this;
	}
	else if(MaskOutside) {
		HitElement = nullptr;
		return;
	}

	// Test children
	for(size_t i = 0; i < Children.size(); i++) {
		Children[i]->Update(FrameTime, Mouse);
		if(Children[i]->GetHitElement())
			HitElement = Children[i]->GetHitElement();
	}
}

// Calculate the screen space bounds for the element
void _Element::CalculateBounds() {
	Bounds.Start = Offset;

	// Handle horizontal alignment
	switch(Alignment.Horizontal) {
		case _Alignment::CENTER:
			if(Parent)
				Bounds.Start.x += Parent->GetSize().x / 2;
			Bounds.Start.x -= Size.x / 2;
		break;
		case _Alignment::RIGHT:
			if(Parent)
				Bounds.Start.x += Parent->GetSize().x;
			Bounds.Start.x -= Size.x;
		break;
	}

	// Handle vertical alignment
	switch(Alignment.Vertical) {
		case _Alignment::MIDDLE:
			if(Parent)
				Bounds.Start.y += Parent->GetSize().y / 2;
			Bounds.Start.y -= Size.y / 2;
		break;
		case _Alignment::BOTTOM:
			if(Parent)
				Bounds.Start.y += Parent->GetSize().y;
			Bounds.Start.y -= Size.y;
		break;
	}

	// Offset from parent
	if(Parent)
		Bounds.Start += Parent->GetBounds().Start + Parent->GetChildrenOffset();

	// Set end
	Bounds.End = Bounds.Start + Size;

	// Update children
	CalculateChildrenBounds();
}

// Update children bounds
void _Element::CalculateChildrenBounds() {

	// Update children
	for(size_t i = 0; i < Children.size(); i++) {
		Children[i]->CalculateBounds();
	}
}

// Render the element
void _Element::Render() const {

	if(MaskOutside) {
		Graphics.EnableStencilTest();
		Graphics.DrawMask(Bounds);
	}

	if(Style) {
		if(Style->GetHasBackgroundColor()) {
			_Color RenderColor(Style->GetBackgroundColor());
			RenderColor.Alpha *= Fade;
			Graphics.DrawRectangle(Bounds, RenderColor, true);
		}

		if(Style->GetHasBorderColor()) {
			_Color RenderColor(Style->GetBorderColor());
			RenderColor.Alpha *= Fade;
			Graphics.DrawRectangle(Bounds, RenderColor, false);
		}
	}

	// Render all children
	for(size_t i = 0; i < Children.size(); i++) {
		Children[i]->SetFade(Fade);
		Children[i]->Render();
	}

	if(MaskOutside)
		Graphics.DisableStencilTest();

	if(Debug && Debug-1 < DebugColorCount) {
		Graphics.DrawRectangle(Bounds.Start, Bounds.End, DebugColors[1]);
	}
}

// Set the debug, and increment for children
void _Element::SetDebug(int Debug) {
	this->Debug = Debug;

	for(size_t i = 0; i < Children.size(); i++) {
		Children[i]->SetDebug(Debug + 1);
	}
}
