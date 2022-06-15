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
#pragma once

// Libraries
#include <ui/ui.h>
#include <string>
#include <vector>

// Forward Declarations
class _Style;
struct _KeyEvent;

// Classes
class _Element {

	public:

		_Element(const std::string &Identifier, _Element *Parent, const glm::vec2 &Offset, const glm::vec2 &Size, const _Alignment &Alignment, const _Style *Style, bool MaskOutside);
		virtual ~_Element();

		virtual void Update(double FrameTime, const glm::vec2 &Mouse);

		virtual void CalculateBounds();
		virtual void Render() const;
		virtual void HandleKeyEvent(const _KeyEvent &KeyEvent);
		virtual void HandleTextEvent(const char *Text);
		virtual void HandleInput(bool Pressed);
		_Element *GetClickedElement();

		void AddChild(_Element *Element) { Children.push_back(Element); Element->ID = Children.size() - 1; }
		std::vector<_Element *> &GetChildren() { return Children; }
		void UpdateChildrenOffset(const glm::vec2 &Update) { ChildrenOffset += Update; CalculateChildrenBounds(); }
		virtual void CalculateChildrenBounds();

		void SetDebug(int Debug);
		void SetAlignment(const _Alignment &Alignment) { this->Alignment = Alignment; CalculateBounds(); }
		void SetIdentifier(const std::string &Identifier) { this->Identifier = Identifier; }
		void SetOffset(const glm::vec2 &Offset) { this->Offset = Offset; CalculateBounds(); }
		void SetChildrenOffset(const glm::vec2 &ChildrenOffset) { this->ChildrenOffset = ChildrenOffset; CalculateChildrenBounds(); }
		void SetParent(_Element *Parent) { this->Parent = Parent; CalculateBounds(); }
		void SetSize(const glm::vec2 &Size) { this->Size = Size; CalculateBounds(); }
		void SetHitElement(_Element *HitElement) { this->HitElement = HitElement; }
		void SetReleasedElement(_Element *ReleasedElement) { this->ReleasedElement = ReleasedElement; }
		void SetUserData(void *UserData) { this->UserData = UserData; }

		void SetWidth(int Width) { Size.x = Width; CalculateBounds(); }
		void SetHeight(int Height) { Size.y = Height; CalculateBounds(); }

		// Attributes
		std::string Identifier;
		_Element *Parent;
		const _Style *Style;
		glm::vec2 ChildrenOffset;
		std::vector<_Element *> Children;
		void *UserData;
		int ID;

		std::string Text;

		glm::vec2 Offset;
		glm::vec2 Size;
		_Alignment Alignment;
		_Bounds Bounds;
		bool MaskOutside;

		float Fade;

		// Input
		_Element *HitElement, *PressedElement, *ReleasedElement;

		int Debug;
};
