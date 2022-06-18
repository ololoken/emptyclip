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
#include <ui/ui.h>
#include <graphics.h>
#include <font.h>
#include <color.h>
#include <input.h>
#include <assets.h>
#include <texture.h>
#include <program.h>
#include <SDL_keycode.h>
#include <tinyxml2/tinyxml2.h>
#include <regex>

_Element *FocusedElement = nullptr;
const glm::vec4 DebugColors[] = {
	{ 0.0f, 1.0f, 1.0f, 1.0f },
	{ 1.0f, 1.0f, 0.0f, 1.0f },
	{ 1.0f, 0.0f, 0.0f, 1.0f },
	{ 0.0f, 1.0f, 0.0f, 1.0f },
	{ 0.0f, 0.0f, 1.0f, 1.0f }
};
const int DebugColorCount = sizeof(DebugColors) / sizeof(glm::vec4);
static int BaseHeight = 1080;

// Constructor
_Element::_Element() :
	Parent(nullptr),
	Index(-1),
	UserData(nullptr),
	Active(false),
	Enabled(true),
	Checked(false),
	Clickable(false),
	Draggable(false),
	MaskOutside(false),
	Stretch(true),
	Wrap(false),
	Format(false),
	SizePercent{false, false},
	Debug(0),
	Color(1.0f),
	Style(nullptr),
	HoverStyle(nullptr),
	DisabledStyle(nullptr),
	Texture(nullptr),
	TextureIndex(0),
	Fade(1.0f),
	BaseOffset(0.0f, 0.0f),
	BaseSize(0.0f, 0.0f),
	Size(0.0f, 0.0f),
	Offset(0.0f, 0.0f),
	HitElement(nullptr),
	PressedElement(nullptr),
	ReleasedElement(nullptr),
	HitOffset(0.0f, 0.0f),
	PressedOffset(0.0f, 0.0f),
	Font(nullptr),
	MaxLength(0),
	CursorPosition(0),
	CursorTimer(0),
	LastKeyPressed(SDL_SCANCODE_UNKNOWN),
	Password(false),
	ChildrenOffset(0.0f, 0.0f) {
}

// Constructor for loading from xml
_Element::_Element(tinyxml2::XMLElement *Node, _Element *Parent) :
	_Element() {

	// Load attributes
	this->Parent = Parent;
	std::string TextureName;
	std::string StyleName;
	std::string HoverStyleName;
	std::string DisabledStyleName;
	std::string FontName;
	std::string BaseSizeString[2];
	AssignAttributeString(Node, "id", Name);
	AssignAttributeString(Node, "texture", TextureName);
	AssignAttributeString(Node, "style", StyleName);
	AssignAttributeString(Node, "hover_style", HoverStyleName);
	AssignAttributeString(Node, "disabled_style", DisabledStyleName);
	AssignAttributeString(Node, "color", ColorName);
	AssignAttributeString(Node, "font", FontName);
	AssignAttributeString(Node, "text", Text);
	AssignAttributeString(Node, "size_x", BaseSizeString[0]);
	AssignAttributeString(Node, "size_y", BaseSizeString[1]);
	AssignAttributeString(Node, "allowed", AllowedCharacters);
	Node->QueryUnsignedAttribute("maxlength", (uint32_t *)&MaxLength);
	Node->QueryFloatAttribute("offset_x", &BaseOffset.x);
	Node->QueryFloatAttribute("offset_y", &BaseOffset.y);
	Node->QueryIntAttribute("alignment_x", &Alignment.Horizontal);
	Node->QueryIntAttribute("alignment_y", &Alignment.Vertical);
	Node->QueryBoolAttribute("wrap", &Wrap);
	Node->QueryBoolAttribute("format", &Format);
	Node->QueryBoolAttribute("clickable", &Clickable);
	Node->QueryBoolAttribute("draggable", &Draggable);
	Node->QueryBoolAttribute("stretch", &Stretch);
	Node->QueryIntAttribute("index", &Index);
	Node->QueryIntAttribute("debug", &Debug);
	Node->QueryBoolAttribute("enabled", &Enabled);
	Node->QueryIntAttribute("base_height", &BaseHeight);

	// Handle size
	for(int i = 0; i < 2; i++) {

		// Check for percent sign
		if(BaseSizeString[i].find('%') != std::string::npos)
			SizePercent[i] = true;

		// Convert to number
		if(BaseSizeString[i] != "") {
			BaseSize[i] = std::stoi(BaseSizeString[i]);
			Size[i] = BaseSize[i] * GetUIScale();
		}
	}

	// Scale offset
	Offset = BaseOffset * GetUIScale();

	// Check ids
	if(Assets.Elements.find(Name) != Assets.Elements.end())
		throw std::runtime_error("Duplicate element id: " + Name);
	if(TextureName != "" && Assets.Textures.find(TextureName) == Assets.Textures.end())
		throw std::runtime_error("Unable to find texture: " + TextureName + " for image: " + Name);
	if(StyleName != "" && Assets.Styles.find(StyleName) == Assets.Styles.end())
		throw std::runtime_error("Unable to find style: " + StyleName + " for element: " + Name);
	if(HoverStyleName != "" && Assets.Styles.find(HoverStyleName) == Assets.Styles.end())
		throw std::runtime_error("Unable to find hover_style: " + HoverStyleName + " for element: " + Name);
	if(DisabledStyleName != "" && Assets.Styles.find(DisabledStyleName) == Assets.Styles.end())
		throw std::runtime_error("Unable to find disabled_style: " + DisabledStyleName + " for element: " + Name);
	if(ColorName != "" && Assets.Colors.find(ColorName) == Assets.Colors.end())
		throw std::runtime_error("Unable to find color: " + ColorName + " for element: " + Name);
	if(FontName != "" && Assets.Fonts.find(FontName) == Assets.Fonts.end())
		throw std::runtime_error("Unable to find font: " + FontName + " for element: " + Name);

	// Assign pointers
	Texture = Assets.Textures[TextureName];
	Style = Assets.Styles[StyleName];
	HoverStyle = Assets.Styles[HoverStyleName];
	DisabledStyle = Assets.Styles[DisabledStyleName];
	Color = Assets.Colors[ColorName];
	Font = Assets.Fonts[FontName];

	// Assign to list
	if(Name != "")
		Assets.Elements[Name] = this;

	// Load children
	for(tinyxml2::XMLElement *ChildNode = Node->FirstChildElement(); ChildNode != nullptr; ChildNode = ChildNode->NextSiblingElement()) {
		_Element *ChildElement = new _Element(ChildNode, this);
		Children.push_back(ChildElement);
	}

	// Set debug for children
	SetDebug(Debug);
}

// Destructor
_Element::~_Element() {
	for(auto &Child : Children) {
		if(Graphics.Element->HitElement == Child)
			Graphics.Element->HitElement = nullptr;

		delete Child;
	}
}

// Get UI scale factor
float _Element::GetUIScale() {
	//return Graphics.CurrentSize.y / (float)BaseHeight;
	return 1.0f;
}

// Serialize element and children to xml node
void _Element::SerializeElement(tinyxml2::XMLDocument &Document, tinyxml2::XMLElement *ParentNode) {

	// Create xml node
	tinyxml2::XMLElement *Node = Document.NewElement("element");

	// Set attributes
	if(ParentNode) {
		std::stringstream Buffer;

		Node->SetAttribute("id", Name.c_str());
		if(Texture)
			Node->SetAttribute("texture", Texture->Name.c_str());
		if(Style)
			Node->SetAttribute("style", Style->Name.c_str());
		if(HoverStyle)
			Node->SetAttribute("hover_style", HoverStyle->Name.c_str());
		if(DisabledStyle)
			Node->SetAttribute("disabled_style", DisabledStyle->Name.c_str());
		if(ColorName.size())
			Node->SetAttribute("color", ColorName.c_str());
		if(Font)
			Node->SetAttribute("font", Font->ID.c_str());
		if(Text.size())
			Node->SetAttribute("text", Text.c_str());
		if(AllowedCharacters.size())
			Node->SetAttribute("allowed", AllowedCharacters.c_str());
		if(BaseOffset.x != 0.0f)
			Node->SetAttribute("offset_x", BaseOffset.x);
		if(BaseOffset.y != 0.0f)
			Node->SetAttribute("offset_y", BaseOffset.y);
		if(BaseSize.x != 0.0f) {
			Buffer << BaseSize.x;
			if(SizePercent[0])
				Buffer << "%";
			Node->SetAttribute("size_x", Buffer.str().c_str());
			Buffer.str("");
		}
		if(BaseSize.y != 0.0f) {
			Buffer << BaseSize.y;
			if(SizePercent[1])
				Buffer << "%";
			Node->SetAttribute("size_y", Buffer.str().c_str());
			Buffer.str("");
		}
		if(Alignment.Horizontal != _Alignment::CENTER)
			Node->SetAttribute("alignment_x", Alignment.Horizontal);
		if(Alignment.Vertical != _Alignment::MIDDLE)
			Node->SetAttribute("alignment_y", Alignment.Vertical);
		if(MaxLength)
			Node->SetAttribute("maxlength", (uint32_t)MaxLength);
		if(!Stretch)
			Node->SetAttribute("stretch", Stretch);
		if(Wrap)
			Node->SetAttribute("wrap", Wrap);
		if(Format)
			Node->SetAttribute("format", Format);
		if(Clickable)
			Node->SetAttribute("clickable", Clickable);
		if(Draggable)
			Node->SetAttribute("draggable", Draggable);
		if(Enabled != 1)
			Node->SetAttribute("enabled", Enabled);
		if(Index != -1)
			Node->SetAttribute("index", Index);

		ParentNode->InsertEndChild(Node);
	}
	else {
		Node->SetAttribute("base_height", BaseHeight);
		Document.InsertEndChild(Node);
	}

	// Add children
	for(const auto &Child : Children)
		Child->SerializeElement(Document, Node);
}

// Handle key event, return true if handled
bool _Element::HandleKey(const _KeyEvent &KeyEvent) {
	if(!Active)
		return false;

	// Handle editable text fields
	if(MaxLength) {
		if(FocusedElement == this && Active && KeyEvent.Pressed) {
			LastKeyPressed = KeyEvent.Scancode;
			if(Text.length() < MaxLength && KeyEvent.Text[0] >= 32 && KeyEvent.Text[0] <= 126) {

				// Check against allowed characters regex
				if(AllowedCharacters.size()) {
					std::regex Regex(AllowedCharacters);
					if(!std::regex_match(std::string(1, KeyEvent.Text[0]), Regex))
						return false;
				}

				if(CursorPosition > Text.length())
					CursorPosition = Text.length();

				Text.insert(CursorPosition, 1, KeyEvent.Text[0]);
				CursorPosition++;
			}
			else if(KeyEvent.Scancode == SDL_SCANCODE_BACKSPACE && Text.length() > 0 && CursorPosition > 0) {
				Text.erase(CursorPosition - 1, 1);
				if(CursorPosition > 0)
					CursorPosition--;
			}
			else if(KeyEvent.Scancode == SDL_SCANCODE_DELETE) {
				Text.erase(CursorPosition, 1);
				if(CursorPosition >= Text.length())
					CursorPosition = Text.length();
			}
			else if(KeyEvent.Scancode == SDL_SCANCODE_LEFT) {
				if(Input.ModKeyDown(KMOD_ALT))
					CursorPosition = 0;
				else if(CursorPosition > 0)
					CursorPosition--;
			}
			else if(KeyEvent.Scancode == SDL_SCANCODE_RIGHT) {
				if(Input.ModKeyDown(KMOD_ALT))
					CursorPosition = Text.length();
				else if(CursorPosition < Text.length())
					CursorPosition++;
			}
			else if(KeyEvent.Scancode == SDL_SCANCODE_HOME) {
				CursorPosition = 0;
			}
			else if(KeyEvent.Scancode == SDL_SCANCODE_END) {
				CursorPosition = Text.length();
			}
			else {
				return false;
			}

			ResetCursor();
			return true;
		}

		return false;
	}

	// Pass event to children
	for(auto &Child : Children) {
		if(Child->HandleKey(KeyEvent))
			return true;
	}

	return false;
}

// Handle a press event
void _Element::HandleMouseButton(bool Pressed) {
	if(!Active)
		return;

	// Handle text boxes
	if(MaxLength && Enabled) {
		if(HitElement || (Parent && Parent->HitElement)) {
			ResetCursor();
			FocusedElement = this;
		}

		return;
	}

	// Pass event to children
	for(auto &Child : Children)
		Child->HandleMouseButton(Pressed);

	// Set pressed element
	if(Pressed) {
		PressedElement = HitElement;
		PressedOffset = HitOffset;
	}

	// Get released element
	if(!Pressed && PressedElement && HitElement)
		ReleasedElement = PressedElement;

	// Unset pressed element
	if(!Pressed)
		PressedElement = nullptr;
}

// Get the element that was clicked and released
_Element *_Element::GetClickedElement() {
	if(HitElement == ReleasedElement)
		return HitElement;

	return nullptr;
}

// Remove a child element
void _Element::RemoveChild(_Element *Element) {
	auto Iterator = std::find(Children.begin(), Children.end(), Element);
	if(Iterator != Children.end()) {
		if(Graphics.Element->HitElement == Element)
			Graphics.Element->HitElement = nullptr;
		Children.erase(Iterator);
	}
}

// Handle mouse movement
void _Element::Update(double FrameTime, const glm::vec2 &Mouse) {
	HitElement = nullptr;
	ReleasedElement = nullptr;

	// Handle dragging
	if(Draggable && PressedElement && Parent) {
		BaseOffset = (Mouse - Parent->Bounds.Start - PressedOffset) / GetUIScale();
		BaseOffset = glm::clamp(BaseOffset, glm::vec2(0), Parent->BaseSize - BaseSize);
		CalculateBounds();
	}

	// Test element first
	if(Bounds.Inside(Mouse) && Active && Clickable && Enabled) {
		HitElement = this;
		HitOffset = Mouse - Bounds.Start;
	}
	else if(MaskOutside) {
		HitElement = nullptr;
		return;
	}

	// Test children
	if(Active) {
		for(auto &Child : Children) {
			Child->Update(FrameTime, Mouse);
			if(Child->HitElement)
				HitElement = Child->HitElement;
		}
	}

	// Handle edit boxes
	if(MaxLength) {
		if(FocusedElement == this || FocusedElement == Parent) {
			CursorTimer += FrameTime;
			if(CursorTimer >= 1)
				CursorTimer = 0;
		}
	}
}

// Render the element
void _Element::Render() const {
	if(!Active)
		return;

	// Remove fractions
	_Bounds DrawBounds(glm::ivec2(Bounds.Start), glm::ivec2(Bounds.End));

	// Mask outside bounds of element
	if(MaskOutside) {
		Graphics.SetProgram(Assets.Programs["ortho_pos"]);
		Graphics.EnableStencilTest();
		Graphics.DrawMask(DrawBounds);
	}

	// Draw enabled state
	if(Enabled) {
		if(Style) {
			DrawStyle(Style);
		}
		else if(Texture) {
			Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
			Graphics.SetColor(Color);
			Graphics.DrawImage(DrawBounds, Texture, Stretch);
		}

		// Draw hover texture
		if(HoverStyle && (Checked || HitElement)) {
			DrawStyle(HoverStyle);
		}
	}
	else if(DisabledStyle) {

		// Draw disabled state
		DrawStyle(DisabledStyle);
	}

	// Draw textbox or label
	if(Font && (Texts.size() || Text != "" || MaxLength)) {

		// Set color
		glm::vec4 RenderColor(Color.r, Color.g, Color.b, Color.a*Fade);
		if(!Enabled)
			RenderColor.a *= 0.5f;

		// Draw multiple lines
		if(Texts.size()) {

			// Handle alignment
			float LineHeight = Font->MaxHeight + 2;
			float Y = Bounds.Start.y;
			switch(Alignment.Vertical) {
				case _Alignment::MIDDLE:
					Y -= (LineHeight * Texts.size() - LineHeight) / 2;
				break;
				case _Alignment::BOTTOM:
					Y -= LineHeight * Texts.size() - LineHeight;
				break;
			}

			// Draw text lines
			for(const auto &Token : Texts) {
				glm::ivec2 DrawPosition(Bounds.Start.x, Y);
				if(Format)
					Font->DrawTextFormatted(Token, DrawPosition, Alignment, Fade);
				else
					Font->DrawText(Token, DrawPosition, Alignment, RenderColor);

				Y += LineHeight;
			}
		}
		else {
			std::string RenderText = Password ? std::string(Text.length(), '*') : Text;

			// Draw textbox
			if(MaxLength) {

				// Get width at cursor position
				_TextBounds TextBounds;
				Font->GetStringDimensions(RenderText.substr(0, CursorPosition), TextBounds);

				// Draw text
				glm::ivec2 StartPosition = DrawBounds.Start;
				Font->DrawText(RenderText, StartPosition, Alignment, RenderColor);

				// Draw cursor
				if(CursorTimer < 0.5 && (FocusedElement == this || FocusedElement == Parent)) {
					Graphics.SetProgram(Assets.Programs["ortho_pos"]);
					Graphics.SetColor(glm::vec4(1.0f));
					Graphics.DrawRectangle(glm::vec2(StartPosition.x + TextBounds.Width+1, StartPosition.y - Font->MaxAbove - 1), glm::vec2(StartPosition.x + TextBounds.Width+2, StartPosition.y + Font->MaxBelow));
				}
			}
			else {

				// Draw label
				if(Format)
					Font->DrawTextFormatted(RenderText, glm::ivec2(DrawBounds.Start), Alignment, Fade);
				else
					Font->DrawText(RenderText, glm::ivec2(DrawBounds.Start), Alignment, RenderColor);
			}
		}
	}

	// Render all children
	for(auto &Child : Children) {
		Child->Render();
	}

	// Disable mask
	if(MaskOutside)
		Graphics.DisableStencilTest();

	// Draw debug info
	if(Debug && Debug-1 < DebugColorCount) {
		Graphics.SetProgram(Assets.Programs["ortho_pos"]);
		Graphics.SetColor(DebugColors[Debug-1]);
		Graphics.DrawRectangle(DrawBounds.Start, DrawBounds.End);
	}
}

// Draw an element using a style
void _Element::DrawStyle(const _Style *DrawStyle) const {

	// Remove fractions
	_Bounds DrawBounds(glm::ivec2(Bounds.Start), glm::ivec2(Bounds.End));

	// Render
	Graphics.SetProgram(DrawStyle->Program);
	if(DrawStyle->Texture) {
		Graphics.SetColor(DrawStyle->TextureColor);
		Graphics.DrawImage(DrawBounds, DrawStyle->Texture, DrawStyle->Stretch);
	}
	else {
		if(DrawStyle->HasBackgroundColor) {
			glm::vec4 RenderColor(DrawStyle->BackgroundColor);
			RenderColor.a *= Fade;
			Graphics.SetColor(RenderColor);
			Graphics.DrawRectangle(DrawBounds, true);
		}
		if(DrawStyle->HasBorderColor) {
			glm::vec4 RenderColor(DrawStyle->BorderColor);
			RenderColor.a *= Fade;
			Graphics.SetColor(RenderColor);
			Graphics.DrawRectangle(DrawBounds, false);
		}
	}
}

// Calculate the screen space bounds for the element
void _Element::CalculateBounds(bool Scale) {

	// Scale element
	if(Scale) {
		Offset = BaseOffset * GetUIScale();
		Size = BaseSize * GetUIScale();
	}

	// Handle percents
	if(Parent) {
		if(SizePercent[0])
			Size.x = Parent->Size.x * BaseSize[0] * 0.01f;
		if(SizePercent[1])
			Size.y = Parent->Size.y * BaseSize[1] * 0.01f;
	}

	// Set start position
	Bounds.Start = Offset;

	// Handle horizontal alignment
	switch(Alignment.Horizontal) {
		case _Alignment::CENTER:
			if(Parent)
				Bounds.Start.x += Parent->Size.x / 2;
			Bounds.Start.x -= Size.x / 2;
		break;
		case _Alignment::RIGHT:
			if(Parent)
				Bounds.Start.x += Parent->Size.x;
			Bounds.Start.x -= Size.x;
		break;
	}

	// Handle vertical alignment
	switch(Alignment.Vertical) {
		case _Alignment::MIDDLE:
			if(Parent)
				Bounds.Start.y += Parent->Size.y / 2;
			Bounds.Start.y -= Size.y / 2;
		break;
		case _Alignment::BOTTOM:
			if(Parent)
				Bounds.Start.y += Parent->Size.y;
			Bounds.Start.y -= Size.y;
		break;
	}

	// Offset from parent
	if(Parent)
		Bounds.Start += Parent->Bounds.Start + Parent->ChildrenOffset;

	// Set end position
	Bounds.End = Bounds.Start + Size;

	// Set text wrapping
	if(Wrap)
		SetWrap(Size.x);

	// Update children
	CalculateChildrenBounds();
}

// Update children bounds
void _Element::CalculateChildrenBounds(bool Scale) {

	// Update children
	for(auto &Child : Children)
		Child->CalculateBounds(Scale);
}

// Set the debug flag, and increment for children
void _Element::SetDebug(int Value) {
	Debug = Value;

	for(auto &Child : Children) {
		if(Value)
			Child->SetDebug(Value + 1);
	}
}

// Set clickable/hoverable flag of element and children. Depth=-1 is full recursion
void _Element::SetClickable(bool Value, int Depth) {
	if(Depth == 0)
		return;

	Clickable = Value;

	if(Depth != -1)
		Depth--;

	for(auto &Child : Children)
		Child->SetClickable(Value, Depth);
}

// Set active state of element and children
void _Element::SetActive(bool Value) {
	Active = Value;

	for(auto &Child : Children)
		Child->SetActive(Value);
}

// Set fade of element and children
void _Element::SetFade(float Value) {
	Fade = Value;

	for(auto &Child : Children)
		Child->SetFade(Value);
}

// Set enabled state of element
void _Element::SetEnabled(bool Value) {
	Enabled = Value;

	for(auto &Child : Children)
		Child->SetEnabled(Value);
}

// Break up text into multiple strings
void _Element::SetWrap(float Width) {
	Texts.clear();
	Font->BreakupString(Text, Width, Texts, Format);
}

// Assign a string from xml attribute
void _Element::AssignAttributeString(tinyxml2::XMLElement *Node, const char *Attribute, std::string &String) {
	const char *Value = Node->Attribute(Attribute);
	if(Value)
		String = Value;
}
