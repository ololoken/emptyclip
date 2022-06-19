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
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <string>
#include <vector>

// Forward Declarations
namespace tinyxml2 {
	class XMLDocument;
	class XMLElement;
}
class _Font;
class _Program;
namespace ae {
	class _Texture;
	struct _KeyEvent;
}

// Bounds struct
struct _Bounds {

	_Bounds() : Start(0.0f), End(0.0f) { }
	_Bounds(const glm::vec2 &Start, const glm::ivec2 &End) : Start(Start), End(End) { }
	_Bounds(const glm::vec4 &Bounds) : Start(Bounds[0], Bounds[1]), End(Bounds[2], Bounds[3]) { }

	glm::vec2 GetMidPoint() const { return (Start + End) * 0.5f; }
	bool Inside(const glm::vec2 &Point) { return Point.x >= Start.x && Point.y >= Start.y && Point.x < End.x && Point.y < End.y; }

	glm::vec2 Start;
	glm::vec2 End;
};

// Alignment struct
struct _Alignment {

	enum HorizontalAlignment {
		LEFT,
		CENTER,
		RIGHT,
	};

	enum VerticalAlignment {
		TOP,
		MIDDLE,
		BOTTOM,
		BASELINE,
	};

	_Alignment() : Horizontal(CENTER), Vertical(MIDDLE) { }
	_Alignment(int Horizontal, int Vertical) : Horizontal(Horizontal), Vertical(Vertical) { }

	int Horizontal;
	int Vertical;
};

// Style struct
struct _Style {

	_Style() :
		TextureColor(0.0f),
		BackgroundColor(0.0f),
		BorderColor(0.0f),
		HasBackgroundColor(false),
		HasBorderColor(false),
		Program(nullptr),
		Texture(nullptr),
		Stretch(false) { }

	// Attributes
	std::string Name;

	// Colors
	glm::vec4 TextureColor;
	glm::vec4 BackgroundColor;
	glm::vec4 BorderColor;
	bool HasBackgroundColor;
	bool HasBorderColor;

	// Graphics
	const _Program *Program;
	const ae::_Texture *Texture;

	// Properties
	bool Stretch;
};

// Classes
class _Element {

	public:

		_Element();
		_Element(tinyxml2::XMLElement *Node, _Element *Parent);
		~_Element();

		static float GetUIScale();

		void SerializeElement(tinyxml2::XMLDocument &Document, tinyxml2::XMLElement *ParentNode);

		void Update(double FrameTime, const glm::vec2 &Mouse);
		void Render() const;
		bool HandleKey(const ae::_KeyEvent &KeyEvent);
		void HandleMouseButton(bool Pressed);
		void CalculateBounds(bool Scale=true);
		_Element *GetClickedElement();

		void RemoveChild(_Element *Element);
		void UpdateChildrenOffset(const glm::vec2 &Update) { ChildrenOffset += Update; CalculateChildrenBounds(); }
		void CalculateChildrenBounds(bool Scale=true);

		void Clear() { CursorTimer = 0; Text = ""; CursorPosition = 0; }
		void ResetCursor() { CursorTimer = 0; }

		void SetDebug(int Value);
		void SetClickable(bool Value, int Depth=-1);
		void SetActive(bool Value);
		void SetFade(float Value);
		void SetEnabled(bool Value);
		void SetWrap(float Width);
		void SetOffsetPercent(const glm::vec2 &Value) { BaseOffset = Value * (Parent->BaseSize - BaseSize); CalculateBounds(); }
		void SetWidth(float Width) { BaseSize.x = Size.x = Width; CalculateBounds(false); }
		void SetHeight(float Height) { BaseSize.y = Size.y = Height; CalculateBounds(false); }
		void SetText(const std::string &Text) { this->Text = Text; CursorPosition = Text.length(); }

		glm::vec2 GetOffsetPercent() { return Offset / (Parent->Size - Size); }

		// Attributes
		std::string Name;
		_Element *Parent;
		int Index;
		void *UserData;

		bool Active;
		bool Enabled;
		bool Checked;
		bool Clickable;
		bool Draggable;
		bool MaskOutside;
		bool Stretch;
		bool Wrap;
		bool Format;
		bool SizePercent[2];
		int Debug;

		// Graphics
		glm::vec4 Color;
		std::string ColorName;
		const _Style *Style;
		const _Style *HoverStyle;
		const _Style *DisabledStyle;
		const ae::_Texture *Texture;
		uint32_t TextureIndex;
		float Fade;

		// Layout
		_Bounds Bounds;
		_Alignment Alignment;
		glm::vec2 BaseOffset;
		glm::vec2 BaseSize;
		glm::vec2 Size;
		glm::vec2 Offset;

		// Input
		_Element *HitElement;
		_Element *PressedElement;
		_Element *ReleasedElement;
		glm::vec2 HitOffset;
		glm::vec2 PressedOffset;

		// Text
		const _Font *Font;
		std::string Text;
		std::string AllowedCharacters;
		std::size_t MaxLength;
		std::size_t CursorPosition;
		double CursorTimer;
		int LastKeyPressed;
		bool Password;

		// Children
		std::vector<_Element *> Children;
		glm::vec2 ChildrenOffset;

	private:

		void DrawStyle(const _Style *DrawStyle) const;
		void AssignAttributeString(tinyxml2::XMLElement *Node, const char *Attribute, std::string &String);

		std::vector<std::string> Texts;

};

extern _Element *FocusedElement;

const _Alignment LEFT_TOP         = _Alignment(_Alignment::LEFT,   _Alignment::TOP);
const _Alignment LEFT_BOTTOM      = _Alignment(_Alignment::LEFT,   _Alignment::BOTTOM);
const _Alignment RIGHT_BOTTOM     = _Alignment(_Alignment::RIGHT,  _Alignment::BOTTOM);
const _Alignment CENTER_MIDDLE    = _Alignment(_Alignment::CENTER, _Alignment::MIDDLE);
const _Alignment LEFT_BASELINE    = _Alignment(_Alignment::LEFT,   _Alignment::BASELINE);
const _Alignment RIGHT_BASELINE   = _Alignment(_Alignment::RIGHT,  _Alignment::BASELINE);
const _Alignment CENTER_BASELINE  = _Alignment(_Alignment::CENTER, _Alignment::BASELINE);
