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
#include <state.h>
#include <map.h>
#include <string>
#include <vector>
#include <list>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

// Forward Declarations
class _Font;
class _Texture;
class _Event;
class _Element;
class _Button;
class _Map;
class _TextBox;
class _Camera;
struct _EventTile;
struct _Brush;
struct _ObjectSpawn;
struct _Block;

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
	ICON_MONSTER,
	ICON_ITEM,
	ICON_AMMO,
	ICON_UPGRADE,
	ICON_WEAPON,
	ICON_ARMOR,
	ICON_NONE,
	ICON_DELETE,
	ICON_COPY,
	ICON_PASTE,
	ICON_SHOW,
	ICON_UNDO,
	ICON_CLEAR,
	ICON_GRID,
	ICON_MSET,
	ICON_LOAD,
	ICON_SAVE,
	ICON_TEST
};

enum EditorModeType {
	EDITMODE_BLOCKS,
	EDITMODE_EVENTS,
	EDITMODE_MONSTERS,
	EDITMODE_ITEMS,
	EDITMODE_AMMO,
	EDITMODE_UPGRADES,
	EDITMODE_WEAPONS,
	EDITMODE_ARMOR,
	EDITMODE_COUNT
};

enum EditorBlockIconTypes {
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
	EDITINPUT_LOADMONSTERSET,
	EDITINPUT_LOAD,
	EDITINPUT_SAVE,
	EDITINPUT_ITEMIDENTIFIER,
	EDITINPUT_MONSTERIDENTIFIER,
	EDITINPUT_PARTICLEIDENTIFIER,
	EDITINPUT_COUNT
};

// Editor state
class _EditorState : public _State {

	public:

		// Setup
		_EditorState();
		virtual ~_EditorState() { }

		void Init() override;
		void Close() override;

		// Input
		bool HandleKey(const _KeyEvent &KeyEvent) override;
		void HandleMouseButton(const _MouseEvent &MouseEvent) override;
		void HandleMouseWheel(int Direction) override;

		// Update
		void Update(double FrameTime) override;
		void Render(double BlendFactor) override;

		// State parameters
		void SetMapFilename(const std::string &Filename) { MapFilename = Filename; }

	protected:

		bool LoadMap(const std::string &File, bool UseSavedCameraPosition=false);
		void ResetEditorState();

		void DrawObject(float OffsetX, float OffsetY, const _ObjectSpawn *Object, float Alpha);
		void DrawBrush();
		void ProcessIcons(int Index, int Type);
		void ProcessBlockIcons(int Index, int Type);
		void ProcessEventIcons(int Index, int Type);

		void LoadPalettes();
		void LoadMonsterButtons();
		void LoadPaletteButtons(std::vector<_Brush> &Icons, int Type);
		void ClearPalette(int Type);

		void AddEvent(int Type);
		void UpdateEventIdentifier(int Type, const std::string &Identifier);
		void SpawnObject(const glm::vec2 &Position, int Type, const std::string &Identifier, bool Align);
		void SelectObject();
		void SelectObjects();
		void DeselectBlock() { SelectedBlockIndex = -1, SelectedBlock = nullptr; }
		void DeselectEvent() { SelectedEventIndex = -1, SelectedEvent = nullptr; }
		void DeselectObjects();
		void ClearClipboard();
		bool BlockSelected() { return SelectedBlockIndex != -1; }
		bool EventSelected() { return SelectedEventIndex != -1; }
		bool ObjectsSelected();

		void SetEventProperties(double ActivationPeriod, int Level, int Active, const std::string &ParticleIdentifier);
		std::string GetEventIdentifier(int Type);
		glm::vec2 GetValidObjectPosition(const glm::vec2 &Position) const;
		bool ObjectInSelectedList(_ObjectSpawn *Object);
		glm::vec2 AlignToGrid(const glm::vec2 &Position) const;

		glm::vec2 GetMoveDeltaPosition(const glm::vec2 &Position);

		void ExecuteWalkable();
		void ExecuteRotate();
		void ExecuteMirror();
		void ExecuteToggleTile();
		void ExecuteIOCommand(int Type);
		void ExecuteClear();
		void ExecuteTest();
		void ExecuteDelete();
		void ExecuteCopy();
		void ExecutePaste(bool Viewport);
		void ExecuteDeselect();
		void ExecuteUndo();
		void ExecuteChangeZ(float Change, int Type);
		void ExecuteChangeLevel(int Change);
		void ExecuteChangePeriod(double Value);
		void ExecuteChangeActive();
		void ExecuteUpdateCheckpointIndex(int Value);
		void ExecuteSelectPalette(_Element *Button, int ClickType);
		void ExecuteUpdateSelectedPalette(int Change);
		void ExecuteUpdateGridMode(int Change);
		void ExecuteHighlightBlocks();
		void ExecuteSwitchMode(int State);
		void ExecuteUpdateLayer(int Layer, bool Move);
		void ExecuteShiftLayer(int Change);
		void ExecuteUpdateBlockLimits(int Direction, bool Expand);

		// Parameters
		glm::vec3 SavedCameraPosition;
		int CheckpointIndex;
		int SavedCheckpointIndex;
		std::string MapFilename;
		int SavedLayer;
		int SavedPalette;

		// Map editing
		_Camera *Camera;
		_Map *Map;
		glm::vec2 WorldCursor;
		_Coord WorldCursorIndex;
		int GridMode;
		int UndoNumber[MAPLAYER_COUNT];
		bool IsDrawing;
		bool IsMoving;
		bool IsShiftDown;
		bool IsCtrlDown;
		bool DraggingBox;

		// Text input
		std::string SavedText[EDITINPUT_COUNT];
		int EditorInput;
		bool BlockTextEvent;

		// UI
		int CurrentLayer;
		int CurrentPalette;
		std::vector<const _Texture *> EventTextures;
		_Font *MainFont;
		_Element *LayerButtons[MAPLAYER_COUNT];
		_Element *ModeButtons[EDITMODE_COUNT];
		_Element *Brush[EDITMODE_COUNT];
		_Element *CommandElement;
		_Element *BlockElement;
		_Element *EventElement;
		_Element *PaletteElement[EDITMODE_COUNT];
		_Element *InputBox;

		// Blocks
		_Block *SelectedBlock;
		_Block ClipboardBlock;
		std::string AltTextureIdentifier;
		float MinZ;
		float MaxZ;
		float ScaleX;
		float Rotation;
		int SelectedBlockIndex;
		int SelectedEventIndex;
		_Coord DrawStart;
		_Coord DrawEnd;
		_Coord OldStart;
		_Coord OldEnd;
		_Coord SavedIndex;
		bool FinishDrawing;
		bool HighlightBlocks;
		bool Walkable;
		bool BlockCopied;
		const _Texture *AltTexture;

		// Events
		_Event *SelectedEvent;
		_Event *ClipboardEvent;
		double EventActivationPeriod;
		int EventActive;
		int EventLevel;

		// Objects
		std::list<_ObjectSpawn *> SelectedObjects;
		std::list<_ObjectSpawn *> ClipboardObjects;
		glm::vec2 ClickedPosition;
		glm::vec2 CopiedPosition;
		glm::vec2 MoveDelta;
};

extern _EditorState EditorState;
