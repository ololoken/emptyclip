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
#include <ae/state.h>
#include <map.h>
#include <string>
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

// Forward Declarations
class _Event;
class _Map;
struct _EventTile;
struct _Brush;
struct _ObjectSpawn;
struct _Block;

namespace ae {
	class _Camera;
	class _Element;
	class _Font;
	class _Framebuffer;
	struct _Reel;
}

// Enumerations
enum EditorIconTypes {
	ICON_LAYER1,
	ICON_LAYER2,
	ICON_LAYER3,
	ICON_LAYER4,
	ICON_LAYER5,
	ICON_WALL,
	ICON_FORE,
	ICON_UP,
	ICON_DOWN,
	ICON_BLOCK,
	ICON_EVENT,
	ICON_ITEM,
	ICON_MONSTER,
	ICON_NONE,
	ICON_DELETE,
	ICON_SPLIT,
	ICON_COPY,
	ICON_PASTE,
	ICON_SHOW,
	ICON_GRID,
	ICON_NEW,
	ICON_LOAD,
	ICON_SAVE,
	ICON_TEST
};

enum EditorModeType {
	EDITMODE_BLOCKS,
	EDITMODE_EVENTS,
	EDITMODE_ITEMS,
	EDITMODE_MONSTERS,
	EDITMODE_PROPS,
	EDITMODE_COUNT
};

enum EditorBlockIconTypes {
	ICON_COLOR,
	ICON_WALK,
	ICON_ROTATE,
	ICON_MIRROR,
	ICON_RAISE,
	ICON_LOWER
};

enum EditorEventIconTypes {
	ICON_TILE,
	ICON_LEVELUP,
	ICON_LEVELDOWN,
	ICON_PERIODUP,
	ICON_PERIODDOWN,
	ICON_ACTIVE,
	ICON_ITEMIDENTIFIER,
	ICON_MONSTERIDENTIFIER,
	ICON_PARTICLEIDENTIFIER
};

enum EditorInputTypes {
	EDITINPUT_LOAD,
	EDITINPUT_SAVE,
	EDITINPUT_ITEMIDENTIFIER,
	EDITINPUT_MONSTERIDENTIFIER,
	EDITINPUT_PARTICLEIDENTIFIER,
	EDITINPUT_COLOR,
	EDITINPUT_COUNT
};

// Used for the map editor
struct _Brush {
	_Brush() {}
	_Brush(const std::string &ID, const std::string &Text, const ae::_Texture *Texture, const glm::vec4 &Color, int ObjectType=-1) :
		ID(ID),
		Text(Text),
		Texture(Texture),
		Color(Color),
		ObjectType(ObjectType) {}

	std::string ID;
	std::string Text;
	const ae::_Texture *Texture;
	glm::vec4 Color;
	int ObjectType;
};

// Editor state
class _EditorState : public ae::_State {

	public:

		void Init() override;
		void Close() override;

		// Input
		bool HandleKey(const ae::_KeyEvent &KeyEvent) override;
		void HandleMouseButton(const ae::_MouseEvent &MouseEvent) override;
		void HandleMouseWheel(int Direction) override;
		void HandleWindow(uint8_t Event) override;
		void HandleQuit() override;

		// Update
		void Update(double FrameTime) override;
		void Render(double BlendFactor) override;

		// State parameters
		void SetMapFilename(const std::string &Filename) { MapFilename = Filename; SavedText[EDITINPUT_SAVE] = MapFilename; }

	protected:

		bool LoadMap(const std::string &File, bool UseSavedCameraPosition=false);
		void ResetEditorState();

		void DrawObject(float OffsetX, float OffsetY, const _ObjectSpawn *ObjectSpawn, float Alpha);
		void DrawBrush();
		void DrawEventTiles(_Event *Event, const glm::vec4 &Color);
		void ProcessIcons(int Index, int Type);
		void ProcessBlockIcons(int Index, int Type);
		void ProcessEventIcons(int Index, int Type);

		void LoadPalettes();
		void LoadPaletteButtons(std::vector<_Brush> &Icons, int Type);
		void ClearPalette(int Type);

		void AddEvent(int Type);
		void UpdateEventID(int Type, const std::string &ID);
		void SpawnObject(const glm::vec2 &Position, float Rotation, float Scale, int Type, const std::string &ID, int Level, bool Align);
		void SelectBlocks();
		void SelectObject();
		void SelectObjects();
		void UpdateSelectionBounds();
		void DeselectBlocks() { SelectedBlocks.clear(); }
		void DeselectEvent() { SelectedEventIndex = -1; SelectedEvent = nullptr; }
		void DeselectObjects() { SelectedObjects.clear(); }
		void ClearClipboard();
		bool EventSelected() { return SelectedEventIndex != -1; }
		bool ObjectsSelected() { return SelectedObjects.size() != 0; }

		void SetEventProperties(double ActivationPeriod, int Level, int Active, const std::string &ParticleID);
		std::string GetEventID(int Type);
		glm::vec2 GetValidObjectPosition(const glm::vec2 &Position) const;
		bool ObjectInSelectedList(_ObjectSpawn *Object);
		glm::vec2 AlignToGrid(const glm::vec2 &Position) const { return glm::vec2((int)Position.x + 0.5f, (int)Position.y + 0.5f); }

		glm::vec2 GetMoveDeltaPosition(const glm::vec2 &Position);
		ae::_Element *GetBrushFromTexture(size_t PaletteType, const ae::_Texture *Texture);

		void ExecuteWalkable();
		void ExecuteRotate();
		void ExecuteMirror();
		void ExecuteToggleTile();
		void ExecuteIOCommand(int Type);
		void ExecuteClear();
		void ExecuteTest();
		void ExecuteDelete();
		void ExecuteCopy();
		void ExecutePaste(int PasteMode=0);
		void ExecuteSplit();
		void ExecuteDeselect();
		void ExecuteChangeZ(float Change, int Type);
		void ExecuteChangeLevel(int Change);
		void ExecuteChangePeriod(double Value);
		void ExecuteChangeActive();
		void ExecuteUpdateCheckpointIndex(int Value);
		void ExecuteSelectPalette(ae::_Element *Button, int ClickType);
		void ExecuteUpdateSelectedPalette(int Change);
		void ExecuteUpdateGridMode(int Change);
		void ExecuteHighlightBlocks();
		void ExecuteSwitchMode(int State);
		void ExecuteUpdateLayer(int Layer, bool Move);
		void ExecuteShiftLayer(int Change);
		void ExecuteUpdateBlockSize(int Direction, bool Expand);
		void ExecuteUpdateMapLevel(int Change);

		// Parameters
		glm::vec3 SavedCameraPosition{0.0f, 0.0f, 6.5f};
		std::string MapFilename;
		int CheckpointIndex{0};
		int SavedCheckpointIndex{0};
		int SavedLayer{0};
		int SavedPalette{0};
		int SavedGridMode{5};
		int SavedBrushIndex{-1};
		int SavedMinZ{0};
		int SavedMaxZ{0};
		bool SavedHighlightBlocks{false};

		// Graphics
		ae::_Framebuffer *Framebuffer;

		// Map editing
		ae::_Camera *Camera;
		_Map *Map;
		glm::vec2 WorldCursor;
		glm::ivec2 WorldCursorIndex;
		int GridMode;
		bool IsDrawing;
		bool IsMoving;
		bool IsShiftDown;
		bool IsCtrlDown;
		bool IsAltDown;
		bool DraggingBox;

		// Text input
		std::string SavedText[EDITINPUT_COUNT];
		int EditorInput;

		// UI
		int EditLayer;
		int EditMode;
		std::vector<const ae::_Texture *> EventTextures;
		ae::_Font *MainFont;
		ae::_Element *LayerButtons[MAPLAYER_COUNT];
		ae::_Element *ModeButtons[EDITMODE_COUNT];
		ae::_Element *Brush[EDITMODE_COUNT];
		ae::_Element *CommandElement;
		ae::_Element *BlockElement;
		ae::_Element *EventElement;
		ae::_Element *PaletteElement[EDITMODE_COUNT];
		ae::_Element *InputBox;

		// Blocks
		std::vector<size_t> SelectedBlocks;
		glm::ivec4 SelectionBounds;
		std::vector<_Block> ClipboardBlocks[MAPLAYER_COUNT];
		std::string AltTextureID;
		float MinZ;
		float MaxZ;
		float ScaleX;
		float Rotation;
		int SelectedEventIndex;
		glm::ivec2 DrawStart;
		glm::ivec2 DrawEnd;
		glm::ivec2 OldStart;
		glm::ivec2 OldEnd;
		glm::ivec2 SavedWorldCursorIndex;
		bool FinishDrawing;
		bool HighlightBlocks;
		bool Walkable;
		const ae::_Texture *AltTexture;

		// Events
		_Event *SelectedEvent;
		_Event *ClipboardEvent;
		double EventActivationPeriod;
		int EventActive;
		int EventLevel;
		int EventSpawnLevel;

		// Objects
		std::unordered_map<std::string, int> ObjectCounts;
		std::vector<_ObjectSpawn *> SelectedObjects;
		std::vector<_ObjectSpawn *> ClipboardObjects;
		glm::vec2 ClickedPosition;
		glm::vec2 CopiedPosition;
		glm::vec2 MoveDelta;
		int ObjectLevel;
};

extern _EditorState EditorState;
