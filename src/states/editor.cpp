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
#include <states/editor.h>
#include <states/play.h>
#include <ae/camera.h>
#include <ae/texture.h>
#include <ae/mesh.h>
#include <ae/graphics.h>
#include <ae/font.h>
#include <ae/ui.h>
#include <ae/assets.h>
#include <ae/program.h>
#include <ae/animation.h>
#include <ae/framebuffer.h>
#include <ae/util.h>
#include <objects/monster.h>
#include <objects/player.h>
#include <gameassets.h>
#include <framework.h>
#include <map.h>
#include <events.h>
#include <menu.h>
#include <config.h>
#include <constants.h>
#include <stats.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>
#include <algorithm>

_EditorState EditorState;

inline bool CompareBrush(_Brush &First, _Brush &Second) {
	return First.ObjectType < Second.ObjectType || (First.ObjectType == Second.ObjectType && First.ID < Second.ID);
}

// Input box
static const char *InputBoxStrings[EDITINPUT_COUNT] = {
	"Load map",
	"Save map",
	"Set item",
	"Set monster",
	"Set particle",
	"Set sound",
	"Color",
};

// Initialize
void _EditorState::Init() {
	ae::Graphics.Element->SetActive(false);
	ae::Graphics.Element->Active = true;
	ae::FocusedElement = nullptr;
	//ae::_Mesh::ConvertOBJ("meshes/test.obj", true, true);

	// Load command buttons
	MainFont = ae::Assets.Fonts["editor"];
	CommandElement = ae::Assets.Elements["element_editor_command"];
	BlockElement = ae::Assets.Elements["element_editor_blocks"];
	EventElement = ae::Assets.Elements["element_editor_events"];
	InputBox = ae::Assets.Elements["element_editor_input"];
	CommandElement->SetActive(true);
	BlockElement->SetActive(true);
	EventElement->SetActive(false);
	InputBox->SetActive(false);

	// Create button groups
	PaletteElement[EDITMODE_BLOCKS] = ae::Assets.Elements["element_editor_palette_block"];
	PaletteElement[EDITMODE_EVENTS] = ae::Assets.Elements["element_editor_palette_events"];
	PaletteElement[EDITMODE_ITEMS] = ae::Assets.Elements["element_editor_palette_items"];
	PaletteElement[EDITMODE_MONSTERS] = ae::Assets.Elements["element_editor_palette_monsters"];
	PaletteElement[EDITMODE_PROPS] = ae::Assets.Elements["element_editor_palette_props"];

	// Assign layer buttons
	LayerButtons[MAPLAYER_BASE] = ae::Assets.Elements["button_editor_layer_base"];
	LayerButtons[MAPLAYER_FLOOR0] = ae::Assets.Elements["button_editor_layer_floor0"];
	LayerButtons[MAPLAYER_FLOOR1] = ae::Assets.Elements["button_editor_layer_floor1"];
	LayerButtons[MAPLAYER_FLOOR2] = ae::Assets.Elements["button_editor_layer_floor2"];
	LayerButtons[MAPLAYER_FLAT] = ae::Assets.Elements["button_editor_layer_flat"];
	LayerButtons[MAPLAYER_WALL] = ae::Assets.Elements["button_editor_layer_wall"];
	LayerButtons[MAPLAYER_FORE] = ae::Assets.Elements["button_editor_layer_fore"];

	// Assign palette buttons
	ModeButtons[EDITMODE_BLOCKS] = ae::Assets.Elements["button_editor_mode_block"];
	ModeButtons[EDITMODE_EVENTS] = ae::Assets.Elements["button_editor_mode_event"];
	ModeButtons[EDITMODE_ITEMS] = ae::Assets.Elements["button_editor_mode_item"];
	ModeButtons[EDITMODE_MONSTERS] = ae::Assets.Elements["button_editor_mode_mons"];
	ModeButtons[EDITMODE_PROPS] = ae::Assets.Elements["button_editor_mode_prop"];

	// Reset state
	ResetEditorState();

	// Create camera
	ae::_CameraSettings CameraSettings;
	CameraSettings.UpdateDivisor = CAMERA_EDITOR_DIVISOR;
	Camera = new ae::_Camera(CameraSettings);
	Camera->ForcePosition(glm::vec3(0, 0, CAMERA_DISTANCE));

	// Load level
	if(PlayState.FromEditor)
		MapFilename = EDITOR_TESTLEVEL;

	LoadMap(MapFilename, PlayState.FromEditor);

	// Set up graphics
	ae::Graphics.SetViewport(ae::Graphics.CurrentSize - EDITOR_VIEWPORT_OFFSET);
	Camera->CalculateFrustum(ae::Graphics.AspectRatio);
	ae::Graphics.SetCursor(true);
	Framebuffer = new ae::_Framebuffer(ae::Graphics.ViewportSize);

	// Adjust UI
	for(int i = 0; i < EDITMODE_COUNT; i++)
		PaletteElement[i]->SetHeight(ae::Graphics.ViewportSize.y - 30);

	// Set saved state
	ExecuteUpdateLayer(SavedLayer, false);
	ExecuteSwitchMode(SavedPalette);
	CheckpointIndex = SavedCheckpointIndex;
	GridMode = SavedGridMode;
	HighlightBlocks = SavedHighlightBlocks;
	MinZ = SavedMinZ;
	MaxZ = SavedMaxZ;
	if(SavedBrushIndex != -1)
		Brush[EDITMODE_BLOCKS] = PaletteElement[EDITMODE_BLOCKS]->Children[SavedBrushIndex];
}

// Close
void _EditorState::Close() {
	Camera->GetDrawPosition(0, SavedCameraPosition);
	SavedLayer = EditLayer;
	SavedPalette = EditMode;
	SavedHighlightBlocks = HighlightBlocks;
	SavedGridMode = GridMode;
	SavedCheckpointIndex = CheckpointIndex;
	SavedMinZ = MinZ;
	SavedMaxZ = MaxZ;
	if(Brush[EDITMODE_BLOCKS])
		SavedBrushIndex = Brush[EDITMODE_BLOCKS]->Index;

	for(int i = 0; i < EDITMODE_COUNT; i++)
		ClearPalette(i);

	delete Camera;
	delete Map;
	delete Framebuffer;

	Camera = nullptr;
	Map = nullptr;
	Framebuffer = nullptr;
}

// Load a level
bool _EditorState::LoadMap(const std::string &File, bool UseSavedCameraPosition) {
	bool Success = false;

	if(Map)
		delete Map;

	try {
		Map = new _Map(File);
		Success = true;
	}
	catch(std::exception &Error) {
		std::cout << Error.what() << std::endl;
		Map = new _Map();
	}

	Map->Camera = Camera;
	ResetEditorState();

	// Set camera
	if(UseSavedCameraPosition)
		Camera->ForcePosition(SavedCameraPosition);
	else
		Camera->ForcePosition(glm::vec3(Map->GetStartingPositionByCheckpoint(0), CAMERA_DISTANCE));

	return Success;
}

void _EditorState::ResetEditorState() {
	WorldCursor = glm::vec2(0, 0);
	SelectedEventIndex = -1;
	SelectedEvent = nullptr;
	ClipboardEvent = nullptr;
	MinZ = 0.0f;
	MaxZ = 0.0f;
	Rotation = 0.0f;
	ScaleX = 1.0f;
	EventActivationPeriod = 0;
	EventActive = 1;
	EventLevel = 0;
	EventSpawnLevel = Map ? Map->Level : 1;
	ObjectLevel = Map ? Map->Level : 1;
	AltTextureID = "";
	AltTexture = nullptr;
	EditorInput = -1;
	CheckpointIndex = 0;
	ClickedPosition = glm::vec2(0, 0);
	CopiedPosition = glm::vec2(0, 0);
	MoveDelta = glm::vec2(0, 0);
	EditLayer = EDITOR_DEFAULT_LAYER;
	EditMode = EDITMODE_BLOCKS;
	GridMode = EDITOR_DEFAULT_GRIDMODE;
	Walkable = true;
	HighlightBlocks = false;
	SelectedBlocks.clear();
	SelectedObjects.clear();
	ClipboardObjects.clear();

	IsShiftDown = false;
	IsCtrlDown = false;
	IsAltDown = false;
	DraggingBox = false;
	IsDrawing = false;
	IsMoving = false;
	FinishDrawing = false;

	WorldCursorIndex.x = 0;
	WorldCursorIndex.y = 0;
	DrawStart.x = 0;
	DrawStart.y = 0;
	DrawEnd.x = 0;
	DrawEnd.y = 0;
	OldStart.x = 0;
	OldStart.y = 0;
	OldEnd.x = 0;
	OldEnd.y = 0;
	SavedWorldCursorIndex.x = 0;
	SavedWorldCursorIndex.y = 0;

	// Enable default button
	for(int i = 0; i < MAPLAYER_COUNT; i++)
		LayerButtons[i]->Checked = false;

	for(int i = 0; i < EDITMODE_COUNT; i++) {
		ModeButtons[i]->Checked = false;
		Brush[i] = nullptr;
	}

	// Load palettes
	LoadPalettes();
	LayerButtons[EditLayer]->Checked = true;
	ModeButtons[EditMode]->Checked = true;
	ae::FocusedElement = nullptr;
}

// Key handler
bool _EditorState::HandleKey(const ae::_KeyEvent &KeyEvent) {
	if(IsMoving || IsDrawing || !KeyEvent.Pressed)
		return false;

	// See if the user is entering in text
	if(EditorInput != -1) {
		switch(KeyEvent.Scancode) {
			case SDL_SCANCODE_RETURN: {
				const std::string InputText = InputBox->Children.front()->Text;
				switch(EditorInput) {
					case EDITINPUT_LOAD: {
						if(InputText.empty())
							break;

						if(LoadMap(InputText, false))
							SavedText[EDITINPUT_SAVE] = InputText;

					} break;
					case EDITINPUT_SAVE:
						if(InputText.empty() || !Map->Save(InputText))
							SavedText[EditorInput] = "";
						else {
							SavedText[EditorInput] = InputText;
						}
					break;
					case EDITINPUT_ITEMID:
					case EDITINPUT_MONSTERID:
					case EDITINPUT_PARTICLEID:
					case EDITINPUT_SOUNDID:
						UpdateEventID(EditorInput, InputText);

						if(!EventSelected())
							SavedText[EditorInput] = InputText;
					break;
					case EDITINPUT_COLOR:
						if(ae::Assets.Colors.find(InputText) != ae::Assets.Colors.end()) {
							if(SelectedBlocks.size()) {
								for(const auto &Index : SelectedBlocks) {
									Map->GetBlock(EditLayer, Index)->Color = ae::Assets.Colors[InputText];
								}
							}
							else
								BrushColor = ae::Assets.Colors[InputText];
						}
					break;
				}
				ae::FocusedElement = nullptr;
				EditorInput = -1;
				InputBox->SetActive(false);
			} break;
			case SDL_SCANCODE_ESCAPE:
				ae::FocusedElement = nullptr;
				EditorInput = -1;
				InputBox->SetActive(false);
			break;
			default:
				InputBox->HandleKey(KeyEvent);
			break;
		}
	}
	else {

		// Command keys
		switch(KeyEvent.Scancode) {

			// Exit
			case SDL_SCANCODE_ESCAPE:
				Framework.Done = true;
			break;
			case SDL_SCANCODE_F1:
				ExecuteUpdateLayer(0, false);
			break;
			case SDL_SCANCODE_F2:
				ExecuteUpdateLayer(1, false);
			break;
			case SDL_SCANCODE_F3:
				ExecuteUpdateLayer(2, false);
			break;
			case SDL_SCANCODE_F4:
				ExecuteUpdateLayer(3, false);
			break;
			case SDL_SCANCODE_F5:
				ExecuteUpdateLayer(4, false);
			break;
			case SDL_SCANCODE_W:
				ExecuteUpdateLayer(5, false);
			break;
			case SDL_SCANCODE_F:
				ExecuteUpdateLayer(6, false);
			break;
			case SDL_SCANCODE_PAGEUP:
				ExecuteUpdateMapLevel(1);
			break;
			case SDL_SCANCODE_PAGEDOWN:
				ExecuteUpdateMapLevel(-1);
			break;
			case SDL_SCANCODE_MINUS:
				ExecuteUpdateCheckpointIndex(-1);
			break;
			case SDL_SCANCODE_EQUALS:
				ExecuteUpdateCheckpointIndex(1);
			break;
			case SDL_SCANCODE_1:
				ExecuteSwitchMode(EDITMODE_BLOCKS);
			break;
			case SDL_SCANCODE_2:
				ExecuteSwitchMode(EDITMODE_EVENTS);
			break;
			case SDL_SCANCODE_3:
				ExecuteSwitchMode(EDITMODE_ITEMS);
			break;
			case SDL_SCANCODE_4:
				ExecuteSwitchMode(EDITMODE_MONSTERS);
			break;
			case SDL_SCANCODE_5:
				ExecuteSwitchMode(EDITMODE_PROPS);
			break;
			case SDL_SCANCODE_GRAVE:
			    ExecuteDeselect();
			break;
			case SDL_SCANCODE_D:
				ExecuteDelete();
			break;
			case SDL_SCANCODE_X:
				ExecuteSplit();
			break;
			case SDL_SCANCODE_C:
				if(IsShiftDown) {
					ExecuteShowInput(EDITINPUT_COLOR);
					Framework.IgnoreNextInputEvent = true;
				}
				else
					ExecuteCopy();
			break;
			case SDL_SCANCODE_V: {
				int PasteMode = 0;
				if(IsShiftDown)
					PasteMode = 1;
				else if(IsCtrlDown)
					PasteMode = 2;

				ExecutePaste(PasteMode);
			} break;
			case SDL_SCANCODE_G:
				if(IsShiftDown)
					ExecuteUpdateGridMode(-1);
				else
					ExecuteUpdateGridMode(1);
			break;
			case SDL_SCANCODE_B:
				ExecuteHighlightBlocks();
			break;
			case SDL_SCANCODE_A:
				if(EditMode == EDITMODE_BLOCKS)
					ExecuteWalkable();
				else if(EditMode == EDITMODE_EVENTS)
					ExecuteChangeActive();
			break;
			case SDL_SCANCODE_R:
				ExecuteRotate();
			break;
			case SDL_SCANCODE_M:
				if(IsShiftDown) {
					ExecuteShowInput(EDITINPUT_MONSTERID);
					Framework.IgnoreNextInputEvent = true;
				}
				else
					ExecuteMirror();
			break;
			case SDL_SCANCODE_KP_MINUS:
				ExecuteChangeZ(-0.5f, !IsShiftDown);
			break;
			case SDL_SCANCODE_KP_PLUS:
				ExecuteChangeZ(0.5f, !IsShiftDown);
			break;
			case SDL_SCANCODE_SPACE:
				ExecuteToggleTile();
			break;
			case SDL_SCANCODE_P:
				if(IsShiftDown) {
					ExecuteShowInput(EDITINPUT_PARTICLEID);
					Framework.IgnoreNextInputEvent = true;
				}
			break;
			case SDL_SCANCODE_N:
				if(IsCtrlDown)
					ExecuteClear();
			break;
			case SDL_SCANCODE_I:
				if(IsShiftDown) {
					ExecuteShowInput(EDITINPUT_ITEMID);
					Framework.IgnoreNextInputEvent = true;
				}
			break;
			case SDL_SCANCODE_L:
				ExecuteShowInput(EDITINPUT_LOAD);
				Framework.IgnoreNextInputEvent = true;
			break;
			case SDL_SCANCODE_S:
				if(IsShiftDown) {
					ExecuteShowInput(EDITINPUT_SOUNDID);
					Framework.IgnoreNextInputEvent = true;
				}
				else {
					ExecuteShowInput(EDITINPUT_SAVE);
					Framework.IgnoreNextInputEvent = true;
				}
			break;
			case SDL_SCANCODE_T:
				ExecuteTest();
			break;
			case SDL_SCANCODE_TAB:
				if(IsShiftDown)
					ExecuteUpdateSelectedPalette(-1);
				else
					ExecuteUpdateSelectedPalette(1);
			break;
			case SDL_SCANCODE_LEFT:
				ExecuteUpdateBlockSize(0, !IsShiftDown);
			break;
			case SDL_SCANCODE_UP:
				ExecuteUpdateBlockSize(1, !IsShiftDown);
			break;
			case SDL_SCANCODE_RIGHT:
				ExecuteUpdateBlockSize(2, !IsShiftDown);
			break;
			case SDL_SCANCODE_DOWN:
				ExecuteUpdateBlockSize(3, !IsShiftDown);
			break;
		}
	}

	return false;
}

// Mouse handler
void _EditorState::HandleMouseButton(const ae::_MouseEvent &MouseEvent) {
	ae::FocusedElement = nullptr;
	ae::Graphics.Element->HandleMouseButton(MouseEvent.Pressed);

	// Handle command group clicks
	ae::_Element *Clicked = CommandElement->GetClickedElement();
	if(Clicked && Clicked->Index != -1) {
		ProcessIcons(Clicked->Index, MouseEvent.Button == SDL_BUTTON_RIGHT);
	}

	if(EditMode == EDITMODE_BLOCKS) {
		ae::_Element *Clicked = BlockElement->GetClickedElement();
		if(Clicked && Clicked->Index != -1) {
			ProcessBlockIcons(Clicked->Index, MouseEvent.Button == SDL_BUTTON_RIGHT);
		}
	}

	if(EditMode == EDITMODE_EVENTS) {
		ae::_Element *Clicked = EventElement->GetClickedElement();
		if(Clicked && Clicked->Index != -1) {
			ProcessEventIcons(Clicked->Index, MouseEvent.Button == SDL_BUTTON_RIGHT);
		}
	}

	// Distinguish between interface and viewport clicks
	if(ae::Input.GetMouse().x < ae::Graphics.ViewportSize.x && ae::Input.GetMouse().y < ae::Graphics.ViewportSize.y) {
		if(MouseEvent.Pressed) {

			// Mouse press
			switch(MouseEvent.Button) {
				case SDL_BUTTON_LEFT:
					if(!IsMoving && !Clicked) {
						switch(EditMode) {
							case EDITMODE_BLOCKS:
							case EDITMODE_EVENTS:
								DeselectBlocks();
								DeselectEvent();

								// Save start position
								DrawStart = SavedWorldCursorIndex = WorldCursorIndex;
								DrawEnd = DrawStart + 1;

								IsDrawing = true;
								FinishDrawing = false;
							break;
							default: {
								ae::_Element *Button = Brush[EditMode];
								if(Button)
									SpawnObject(Map->GetValidPosition(WorldCursor), Rotation, 1.0f, (intptr_t)Button->UserData, Button->Name, ObjectLevel, IsShiftDown);
							} break;
						}
					}
				break;
				case SDL_BUTTON_RIGHT:
					Camera->Set2DPosition(WorldCursor);
				break;
				case SDL_BUTTON_MIDDLE:
					if(!IsDrawing) {
						switch(EditMode) {
							case EDITMODE_BLOCKS: {
								ClickedPosition = WorldCursor;

								// See if click hit a block
								_Block *SelectedBlock = nullptr;
								size_t SelectedBlockIndex = Map->GetSelectedBlock(EditLayer, WorldCursorIndex, &SelectedBlock);
								if(SelectedBlock) {
									SavedWorldCursorIndex = WorldCursorIndex;
									IsMoving = true;
									DraggingBox = false;
									SelectedBlock->MoveStart = SelectedBlock->Start;
									SelectedBlock->MoveEnd = SelectedBlock->End;

									// See if block was part of an existing selection
									for(const auto &Index : SelectedBlocks) {
										if(Index == SelectedBlockIndex)
											return;
									}

									// Shift adds to selection
									if(IsShiftDown)
										IsMoving = false;
									else
										SelectedBlocks.clear();
									SelectedBlocks.push_back(SelectedBlockIndex);
									UpdateSelectionBounds();
								}
								else
									DraggingBox = true;
							} break;
							case EDITMODE_EVENTS:

								// Get the event
								SelectedEventIndex = Map->GetSelectedEvent(WorldCursorIndex, &SelectedEvent);
								if(EventSelected()) {

									// Save old states
									OldStart = SelectedEvent->Start;
									OldEnd = SelectedEvent->End;
									SavedWorldCursorIndex = WorldCursorIndex;
									IsMoving = true;
								}
							break;
							default:
								SelectObject();
							break;
						}
					}
				break;
			}
		}
	}
	else {

		// Get button click for palette
		ae::_Element *Button = PaletteElement[EditMode]->GetClickedElement();
		if(Button)
			ExecuteSelectPalette(Button, MouseEvent.Button == SDL_BUTTON_RIGHT);
	}

	// Mouse Release
	if(!MouseEvent.Pressed) {
		switch(MouseEvent.Button) {
			case SDL_BUTTON_LEFT:
				if(IsDrawing)
					FinishDrawing = true;
			break;
			case SDL_BUTTON_MIDDLE:
				if(IsMoving) {
					IsMoving = false;
					for(auto Iterator : SelectedObjects)
						Iterator->Position = GetMoveDeltaPosition(Iterator->Position);

					// Reset move position
					for(const auto &Index : SelectedBlocks) {
						_Block *Block = Map->GetBlock(EditLayer, (size_t)Index);
						Block->MoveStart = Block->Start;
						Block->MoveEnd = Block->End;
					}
					UpdateSelectionBounds();

					MoveDelta = glm::vec2(0, 0);
				}

				if(DraggingBox) {
					DraggingBox = false;
					switch(EditMode) {
						case EDITMODE_BLOCKS:
							SelectBlocks();
						break;
						default:
							SelectObjects();
						break;
					}
				}
			break;
		}
	}
}

// Mouse wheel handler
void _EditorState::HandleMouseWheel(int Direction) {

	// Inside viewport
	if(ae::Input.GetMouse().x < ae::Graphics.ViewportSize.x && ae::Input.GetMouse().y < ae::Graphics.ViewportSize.y) {
		float Multiplier = 1.0f * Direction;
		if(IsShiftDown)
			Multiplier = 10.0f * Direction;

		if(IsCtrlDown) {
			if(SelectedBlocks.size()) {
				ExecuteChangeZ(Direction * 0.5f, !IsShiftDown);
				return;
			}

			if(SelectedObjects.size()) {
				for(const auto &Object : SelectedObjects)
					Object->Scale += Multiplier * 0.05f;

				return;
			}
		}

		// Zoom
		Camera->UpdateDistance(-Multiplier);
	}
	else {

		// Inside palette
		if(ae::Input.GetMouse().x >= ae::Graphics.ViewportSize.x) {
			if(Direction > 0)
				PaletteElement[EditMode]->UpdateChildrenOffset(glm::ivec2(0, EDITOR_PALETTE_SIZE));
			else
				PaletteElement[EditMode]->UpdateChildrenOffset(glm::ivec2(0, -EDITOR_PALETTE_SIZE));
		}
		// Inside controls
		else {
			if(IsShiftDown)
				Direction *= 5;

			if(EditMode == EDITMODE_EVENTS) {
				if(SelectedEvent)
					SelectedEvent->SpawnLevel = std::clamp(SelectedEvent->SpawnLevel + Direction, 1, OBJECT_MAX_LEVEL);
				else
					EventSpawnLevel = std::clamp(EventSpawnLevel + Direction, 1, OBJECT_MAX_LEVEL);
			}
			else {
				if(SelectedObjects.size()) {
					for(const auto &Object : SelectedObjects)
						Object->Level = std::clamp(Object->Level + Direction, 1, OBJECT_MAX_LEVEL);
				}
				else
					ObjectLevel = std::clamp(ObjectLevel + Direction, 1, OBJECT_MAX_LEVEL);
			}
		}
	}
}

// Window size updates
void _EditorState::HandleWindow(uint8_t Event) {
	if(Event == SDL_WINDOWEVENT_SIZE_CHANGED) {
		if(Camera)
			Camera->CalculateFrustum(ae::Graphics.AspectRatio);
	}
}

// Handle quit events
void _EditorState::HandleQuit() {
	Framework.Done = true;
}

// Update
void _EditorState::Update(double FrameTime) {
	ae::Graphics.Element->Update(FrameTime, ae::Input.GetMouse());
	//if(ae::Graphics.Element->HitElement)
	//	std::cout << ae::Graphics.Element->HitElement->Name << std::endl;

	// Get modifier key status
	IsShiftDown = ae::Input.ModKeyDown(KMOD_SHIFT) ? true : false;
	IsCtrlDown = ae::Input.ModKeyDown(KMOD_CTRL) ? true : false;
	IsAltDown = ae::Input.ModKeyDown(KMOD_ALT) ? true : false;

	// Get world cursor
	Camera->ConvertScreenToWorld(ae::Input.GetMouse(), WorldCursor);

	// Get tile indices for later usage
	WorldCursorIndex = Map->GetValidCoord(WorldCursor);

	// Set camera position
	Camera->Update(FrameTime);

	// Count object ids
	ObjectCounts.clear();
	for(const auto &ObjectSpawn : Map->ObjectSpawns) {
		ObjectCounts[ObjectSpawn->ID]++;
	}

	// Drawing a block or event
	if(IsDrawing) {

		// Get start positions
		DrawStart = SavedWorldCursorIndex;

		// Check bounds
		DrawEnd = WorldCursorIndex + 1;

		// Reverse X
		if(DrawEnd.x <= DrawStart.x) {
			std::swap(DrawStart.x, DrawEnd.x);
			DrawStart.x--;
			DrawEnd.x++;
		}

		// Reverse Y
		if(DrawEnd.y <= DrawStart.y) {
			std::swap(DrawStart.y, DrawEnd.y);
			DrawStart.y--;
			DrawEnd.y++;
		}
	}

	// Update based on editor state
	switch(EditMode) {
		case EDITMODE_BLOCKS:

			// Finish drawing a block and add it to the list
			if(FinishDrawing) {
				if(Brush[EDITMODE_BLOCKS]) {
					_Block Block;
					Block.Start = DrawStart;
					Block.End = DrawEnd-1;
					Block.MinZ = MinZ;
					Block.MaxZ = MaxZ;
					Block.Texture = Brush[EDITMODE_BLOCKS]->Style->Texture;
					Block.AltTexture = AltTexture;
					Block.Rotation = Rotation;
					Block.ScaleX = ScaleX;
					Block.Walkable = Walkable || (EditLayer == MAPLAYER_FORE);
					Block.Color = BrushColor;

					Map->AddBlock(EditLayer, Block);
				}

				FinishDrawing = IsDrawing = false;
			}
			else if(IsMoving) {
				glm::ivec2 Offset = WorldCursorIndex - SavedWorldCursorIndex;
				if(Offset.x + SelectionBounds[0] < 0)
					Offset.x = -SelectionBounds[0];
				else if(Offset.x + SelectionBounds[2] >= Map->Size.x)
					Offset.x = Map->Size.x - SelectionBounds[2] - 1;
				if(Offset.y + SelectionBounds[1] < 0)
					Offset.y = -SelectionBounds[1];
				else if(Offset.y + SelectionBounds[3] >= Map->Size.y)
					Offset.y = Map->Size.y - SelectionBounds[3] - 1;

				for(const auto &Index : SelectedBlocks) {
					_Block *Block = Map->GetBlock(EditLayer, (size_t)Index);
					Block->Start = Block->MoveStart + Offset;
					Block->End = Block->MoveEnd + Offset;
				}
			}
		break;
		case EDITMODE_EVENTS:

			// Moving an event
			if(IsMoving) {

				// Get offsets
				glm::ivec2 Offset = WorldCursorIndex - SavedWorldCursorIndex;

				// Check x bounds
				if(Offset.x + OldStart.x < 0)
					Offset.x = -OldStart.x;
				else if(Offset.x + OldEnd.x >= Map->Size.x)
					Offset.x = Map->Size.x - OldEnd.x - 1;

				// Check y bounds
				if(Offset.y + OldStart.y < 0)
					Offset.y = -OldStart.y;
				else if(Offset.y + OldEnd.y >= Map->Size.y)
					Offset.y = Map->Size.y - OldEnd.y - 1;

				// Get start positions
				DrawStart = OldStart + Offset;

				// Check bounds
				DrawEnd.x = OldEnd.x + Offset.x + 1;
				DrawEnd.y = OldEnd.y + Offset.y + 1;
				SelectedEvent->Start = DrawStart;
				SelectedEvent->End = DrawEnd - 1;
			}

			if(FinishDrawing) {
				if(Brush[EDITMODE_EVENTS])
					AddEvent(Brush[EDITMODE_EVENTS]->Index);

				FinishDrawing = IsDrawing = false;
			}
		break;
		default:
			if(IsMoving)
				MoveDelta = WorldCursor - ClickedPosition;
		break;
	}
}

// Render the state
void _EditorState::Render(double BlendFactor) {

	// Setup 3D transformation
	ae::Graphics.Setup3D();
	Camera->Set3DProjection(BlendFactor);
	ae::Assets.Programs["map"]->LightCount = 0;
	ae::Assets.Programs["map"]->AmbientLight = glm::vec4(1);
	ae::Assets.Programs["map_norm"]->LightCount = 0;
	ae::Assets.Programs["map_norm"]->AmbientLight = glm::vec4(1);

	// Setup the viewing matrix
	ae::Graphics.SetProgram(ae::Assets.Programs["map"]);
	ae::Assets.Programs["map"]->SetUniformMat4("view_projection_transform", Camera->Transform);
	ae::Graphics.SetProgram(ae::Assets.Programs["map_norm"]);
	ae::Assets.Programs["map_norm"]->SetUniformMat4("view_projection_transform", Camera->Transform);
	ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
	ae::Assets.Programs["pos"]->SetUniformMat4("view_projection_transform", Camera->Transform);
	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
	ae::Assets.Programs["pos_uv"]->SetUniformMat4("view_projection_transform", Camera->Transform);
	ae::Graphics.SetProgram(ae::Assets.Programs["text"]);
	ae::Assets.Programs["text"]->SetUniformMat4("view_projection_transform", Camera->Transform);

	// Draw floors
	Map->RenderFloors();

	// Draw tentative block
	if(IsDrawing && Brush[EditMode]) {
		if(EditMode == EDITMODE_EVENTS) {
			ae::Graphics.SetColor(COLOR_WHITE);
			ae::Graphics.SetDepthTest(false);
			ae::Graphics.DrawRepeatable(glm::vec3(DrawStart.x, DrawStart.y, MAP_LAYEROFFSET), glm::vec3(DrawEnd.x, DrawEnd.y, MAP_LAYEROFFSET), Brush[EditMode]->Style->Texture, 0, 1.0f);
			ae::Graphics.SetDepthTest(true);
		}
		else {
			ae::Graphics.SetColor(BrushColor);
			if(EditLayer == MAPLAYER_FORE) {
				ae::Graphics.DrawRepeatable(glm::vec3(DrawStart.x, DrawStart.y, MaxZ + MAP_LAYEROFFSET), glm::vec3(DrawEnd.x, DrawEnd.y, MaxZ + MAP_LAYEROFFSET), Brush[EditMode]->Style->Texture, Rotation, ScaleX);
			}
			else if(EditLayer == MAPLAYER_FLAT) {
				glm::vec2 Offset;
				int Side;
				if(Rotation == 0.0f || Rotation == 180.0f) {
					Side = 3;
					Offset.y = 0.5f;
				}
				else {
					Side = 2;
					Offset.x = 0.5f;
				}
				ae::Graphics.DrawWall(glm::vec3(glm::vec2(DrawStart) + Offset, MinZ), glm::vec3(DrawEnd.x - DrawStart.x, DrawEnd.y - DrawStart.y, MaxZ - MinZ), Brush[EditMode]->Style->Texture, Side);
			}
			else {
				if(MaxZ == MinZ) {
					ae::Graphics.DrawRepeatable(glm::vec3(DrawStart.x, DrawStart.y, MinZ + MAP_LAYEROFFSET * EditLayer), glm::vec3(DrawEnd.x, DrawEnd.y, MinZ + MAP_LAYEROFFSET * EditLayer), Brush[EditMode]->Style->Texture, Rotation, ScaleX);
				}
				else {
					ae::Graphics.SetVBO(ae::VBO_CUBE);
					ae::Graphics.DrawCube(glm::vec3(DrawStart.x, DrawStart.y, MinZ), glm::vec3(DrawEnd.x - DrawStart.x, DrawEnd.y - DrawStart.y, MaxZ - MinZ), Brush[EditMode]->Style->Texture);
				}
			}
		}
	}

	// Draw walls below objects
	Map->RenderWalls(true);

	// Draw objects
	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
	ae::Graphics.SetDepthTest(true);
	for(const auto &ObjectSpawn : Map->ObjectSpawns)
		DrawObject(0.0f, 0.0f, ObjectSpawn, 1.0f);

	// Outline selected item
	ae::Graphics.SetDepthTest(false);
	ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
	ae::Graphics.SetColor(COLOR_WHITE);
	for(auto Iterator : SelectedObjects) {
		glm::vec2 Position = GetMoveDeltaPosition(Iterator->Position);
		ae::Graphics.DrawCircle(glm::vec3(Position, ITEM_Z + 0.05f), EDITOR_OBJECTRADIUS);
	}

	// Draw faded items while moving
	ae::Graphics.SetDepthTest(true);
	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
	ae::Graphics.SetVBO(ae::VBO_QUAD);
	for(auto Iterator : SelectedObjects)
		DrawObject(MoveDelta.x, MoveDelta.y, Iterator, 0.5f);
	ae::Graphics.SetDepthMask(true);

	// Draw walls
	Map->RenderWalls();
	Map->RenderFlatWalls();

	// Draw the foreground tiles
	Map->RenderForeground(glm::vec2(-1), EditLayer != MAPLAYER_FORE);

	// Draw the events
	if(EditMode == EDITMODE_EVENTS)
		Map->RenderEvents(EventTextures);

	ae::Graphics.SetDepthMask(false);
	ae::Graphics.SetDepthTest(false);

	// Draw map boundaries
	ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
	ae::Graphics.SetColor(COLOR_RED);
	ae::Graphics.DrawRectangle3D(glm::vec2(-0.01f, -0.01f), glm::vec2(Map->Size.x + 0.01f, Map->Size.y + 0.01f), false);

	// Draw grid
	Map->RenderGrid(GridMode);

	// Outline the blocks
	if(HighlightBlocks)
		Map->HighlightBlocks(EditLayer);

	// Outline selected blocks
	for(const auto &Index : SelectedBlocks) {
		_Block *Block = Map->GetBlock(EditLayer, (size_t)Index);
		ae::Graphics.SetColor(COLOR_WHITE);
		ae::Graphics.DrawRectangle3D(glm::vec2(Block->Start.x, Block->Start.y), glm::vec2(Block->End.x + 1.0f, Block->End.y + 1.0f), false);
	}

	// Outline selected event
	if(EventSelected()) {
		ae::Graphics.SetColor(COLOR_CYAN);
		ae::Graphics.DrawRectangle3D(glm::vec2(SelectedEvent->Start.x + 0.02f, SelectedEvent->Start.y + 0.02f), glm::vec2(SelectedEvent->End.x + 0.98f, SelectedEvent->End.y + 0.98f), false);

		// Draw tiles for all spawn events
		if(SelectedEvent->Type == EVENT_SPAWN || SelectedEvent->Type == EVENT_TELEPORT) {
			for(const auto &Event : Map->Events) {
				if(Event != SelectedEvent && Event->Type == SelectedEvent->Type)
					DrawEventTiles(Event, glm::vec4(0.5f, 0.5f, 0.5f, 0.5));
			}
		}

		// Outline affected tiles and blocks
		DrawEventTiles(SelectedEvent, glm::vec4(1.0f, 0.0f, 0.0f, 0.5f));
	}

	// Dragging a box around object
	if(DraggingBox) {
		ae::Graphics.SetColor(COLOR_WHITE);
		ae::Graphics.DrawRectangle3D(ClickedPosition, WorldCursor, false);
	}

	// Draw a block
	if(IsDrawing) {
		ae::Graphics.SetColor(COLOR_GREEN);
		ae::Graphics.DrawRectangle3D(glm::vec2(DrawStart.x, DrawStart.y), glm::vec2(DrawEnd.x, DrawEnd.y), false);
	}

	// Setup for drawing the HUD
	ae::Graphics.Setup2D();
	ae::Graphics.SetStaticUniforms();
	ae::Graphics.SetDepthTest(false);
	ae::Graphics.SetDepthMask(false);

	// Draw object levels
	if(Camera->GetPosition().z <= EDITOR_LEVEL_Z) {
		for(const auto &Object : Map->ObjectSpawns) {
			if(Object->Type == _Object::AMMO || Object->Type == _Object::MEDKIT ||  Object->Type == _Object::PROP || Object->Type == _Object::KEY)
				continue;

			glm::vec2 TextPosition;
			Camera->ConvertWorldToScreen(Object->Position + glm::vec2(0.25, 0.25), TextPosition);
			ae::Assets.Fonts["hud_small"]->DrawText(std::to_string(Object->Level), TextPosition, ae::CENTER_BASELINE);
		}
	}

	// Draw viewport outline
	ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos"]);
	ae::Graphics.SetColor(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
	ae::Graphics.DrawRectangle(glm::vec2(0, 0), ae::Graphics.ViewportSize);

	// Draw text
	if(EditorInput != -1)
		InputBox->Render();

	// Top left
	glm::vec2 DrawPosition = glm::vec2(15, 25);
	glm::vec2 DrawSpacing = glm::vec2(0, 20);
	std::ostringstream Buffer;

	// Draw filename
	Buffer << Map->Filename;
	MainFont->DrawText(Buffer.str(), DrawPosition);
	Buffer.str("");
	DrawPosition += DrawSpacing;

	// Draw map level
	Buffer << "Level " << Map->Level;
	MainFont->DrawText(Buffer.str(), DrawPosition);
	Buffer.str("");

	// Bottom Left

	// Draw cursor position
	DrawPosition = glm::vec2(15, ae::Graphics.ViewportSize.y - 25);
	Buffer << std::fixed << WorldCursor.x << ", " << WorldCursor.y;
	MainFont->DrawText(Buffer.str(), DrawPosition);
	Buffer.str("");

	// Top right
	DrawPosition.x = ae::Graphics.ViewportSize.x - 15;
	DrawPosition.y = 25;

	// Draw FPS
	Buffer << ae::Graphics.FramesPerSecond << " FPS";
	MainFont->DrawText(Buffer.str(), DrawPosition, ae::RIGHT_BASELINE);
	Buffer.str("");
	DrawPosition += DrawSpacing;

	// Draw selection count
	if(EditMode == EDITMODE_BLOCKS)
		Buffer << SelectedBlocks.size() << " selected";
	else
		Buffer << SelectedObjects.size() << " selected";
	MainFont->DrawText(Buffer.str(), DrawPosition, ae::RIGHT_BASELINE);
	Buffer.str("");

	// Bottom right
	DrawPosition.x = ae::Graphics.ViewportSize.x - 30;
	DrawPosition.y = ae::Graphics.ViewportSize.y - 45;
	glm::vec2 DrawOffset(5, 0);

	// Draw grid size
	Buffer << GridMode;
	MainFont->DrawText("Grid:", DrawPosition, ae::RIGHT_BASELINE);
	MainFont->DrawText(Buffer.str(), DrawPosition + DrawOffset);
	Buffer.str("");
	DrawPosition += DrawSpacing;

	// Draw checkpoint info
	Buffer << CheckpointIndex;
	MainFont->DrawText("Checkpoint:",  DrawPosition, ae::RIGHT_BASELINE);
	MainFont->DrawText(Buffer.str(), DrawPosition + DrawOffset);
	Buffer.str("");
	DrawPosition += DrawSpacing;

	// Event tile count
	if(SelectedEvent) {
		Buffer << SelectedEvent->Tiles.size();
		MainFont->DrawText("Event tiles:",  DrawPosition, ae::RIGHT_BASELINE);
		MainFont->DrawText(Buffer.str(), DrawPosition + DrawOffset);
		Buffer.str("");
	}

	// Draw command buttons
	CommandElement->Render();
	if(EditMode == EDITMODE_BLOCKS) {
		BlockElement->Render();
	}
	else if(EditMode == EDITMODE_EVENTS) {
		EventElement->Render();
	}

	// Draw current brush
	DrawBrush();

	// Draw Palette
	PaletteElement[EditMode]->Render();

	ae::Graphics.SetDepthMask(true);
}

// Load palette buttons
void _EditorState::LoadPalettes() {
	std::vector<_Brush> Icons;
	Icons.reserve(100);

	// Load map textures
	for(const auto &Texture : ae::Assets.Textures) {
		if(Texture.second && Texture.second->Name.find(MAP_TEXTURE_PATH) != std::string::npos)
			Icons.push_back(_Brush(Texture.first, Texture.second->Name, Texture.second, COLOR_WHITE));
	}
	LoadPaletteButtons(Icons, EDITMODE_BLOCKS);
	Icons.clear();

	// Load events
	Icons.push_back(_Brush("door", "Door", ae::Assets.Textures["textures/editor_repeat/event_door.png"], COLOR_WHITE));
	Icons.push_back(_Brush("wswitch", "Wall Switch", ae::Assets.Textures["textures/editor_repeat/event_wswitch.png"], COLOR_WHITE));
	Icons.push_back(_Brush("spawn", "Spawn", ae::Assets.Textures["textures/editor_repeat/event_spawn.png"], COLOR_WHITE));
	Icons.push_back(_Brush("check", "Checkpoint", ae::Assets.Textures["textures/editor_repeat/event_check.png"], COLOR_WHITE));
	Icons.push_back(_Brush("end", "End of Level", ae::Assets.Textures["textures/editor_repeat/event_end.png"], COLOR_WHITE));
	Icons.push_back(_Brush("text", "Message", ae::Assets.Textures["textures/editor_repeat/event_text.png"], COLOR_WHITE));
	Icons.push_back(_Brush("sound", "Sound", ae::Assets.Textures["textures/editor_repeat/event_sound.png"], COLOR_WHITE));
	Icons.push_back(_Brush("fswitch", "Floor Switch", ae::Assets.Textures["textures/editor_repeat/event_fswitch.png"], COLOR_WHITE));
	Icons.push_back(_Brush("enable", "Event Enabler", ae::Assets.Textures["textures/editor_repeat/event_enable.png"], COLOR_WHITE));
	Icons.push_back(_Brush("tele", "Teleporter", ae::Assets.Textures["textures/editor_repeat/event_tele.png"], COLOR_WHITE));
	Icons.push_back(_Brush("light", "Lights", ae::Assets.Textures["textures/editor_repeat/event_light.png"], COLOR_WHITE));
	Icons.push_back(_Brush("secret", "Secret", ae::Assets.Textures["textures/editor_repeat/event_secret.png"], COLOR_WHITE));
	Icons.push_back(_Brush("lava", "Lava", ae::Assets.Textures["textures/editor_repeat/event_lava.png"], COLOR_WHITE));
	LoadPaletteButtons(Icons, EDITMODE_EVENTS);
	for(size_t i = 0; i < Icons.size(); i++)
		EventTextures.push_back(Icons[i].Texture);
	Icons.clear();

	// Load items
	for(const auto &Item : Stats.Objects) {
		if(Item.second.IsItem() && Item.second.IconID != "")
			Icons.push_back(_Brush(Item.first, Item.second.Name, ae::Assets.Textures[Item.second.IconID], Item.second.Color, Item.second.Type));
	}
	LoadPaletteButtons(Icons, EDITMODE_ITEMS);
	Icons.clear();

	// Load monsters
	for(const auto &Monster : Stats.Objects) {
		if(Monster.second.Type != _Object::MONSTER)
			continue;

		const ae::_Texture *MonsterIcon = ae::Assets.Textures["textures/icons/" + Monster.second.AnimationID + ".png"];
		Icons.push_back(_Brush(Monster.first, Monster.second.Name, MonsterIcon, Monster.second.Color, _Object::MONSTER));
	}
	LoadPaletteButtons(Icons, EDITMODE_MONSTERS);
	Icons.clear();

	// Load props
	for(const auto &Prop : Stats.Objects) {
		if(Prop.second.Type != _Object::PROP)
			continue;

		const ae::_Texture *PropIcon = ae::Assets.Textures["textures/icons/" + Prop.first + ".png"];
		Icons.push_back(_Brush(Prop.first, Prop.second.Name, PropIcon, Prop.second.Color, Prop.second.Type));
	}
	LoadPaletteButtons(Icons, EDITMODE_PROPS);
	Icons.clear();
}

// Free memory used by palette
void _EditorState::ClearPalette(int Type) {
	std::vector<ae::_Element *> &Children = PaletteElement[Type]->Children;
	for(size_t i = 0; i < Children.size(); i++) {
		delete Children[i]->Style;
		delete Children[i];
	}
	Children.clear();
}

// Loads the palette
void _EditorState::LoadPaletteButtons(std::vector<_Brush> &Icons, int Type) {
	ClearPalette(Type);

	// Sort icons
	if(Type != EDITMODE_EVENTS)
		std::sort(Icons.begin(), Icons.end(), CompareBrush);

	// Loop through textures
	glm::ivec2 Offset(0, 0);
	int Width = PaletteElement[Type]->Size.x;
	for(size_t i = 0; i < Icons.size(); i++) {

		ae::_Style *Style = new ae::_Style();
		Style->Name = Icons[i].Text;
		Style->HasBackgroundColor = false;
		Style->HasBorderColor = false;
		Style->BackgroundColor = COLOR_WHITE;
		Style->BorderColor = COLOR_WHITE;
		Style->Program = ae::Assets.Programs["ortho_pos_uv"];
		Style->Texture = Icons[i].Texture;
		Style->TextureColor = Icons[i].Color;
		Style->Stretch = true;

		ae::_Element *Button = new ae::_Element();
		Button->Name = Icons[i].ID;
		Button->Parent = PaletteElement[Type];
		Button->BaseOffset = Offset;
		Button->BaseSize = glm::ivec2(EDITOR_PALETTE_SIZE, EDITOR_PALETTE_SIZE);
		Button->Alignment = ae::LEFT_TOP;
		Button->Style = Style;
		Button->HoverStyle = ae::Assets.Styles["style_editor_button_selected"];
		Button->UserData = (void *)(intptr_t)Icons[i].ObjectType;
		Button->Index = i;
		Button->Scaled = false;

		PaletteElement[Type]->Children.push_back(Button);

		Offset.x += EDITOR_PALETTE_SIZE;
		if(Offset.x > Width - EDITOR_PALETTE_SIZE) {
			Offset.y += EDITOR_PALETTE_SIZE;
			Offset.x = 0;
		}
	}

	PaletteElement[Type]->SetClickable(true);
	PaletteElement[Type]->SetActive(true);
	PaletteElement[Type]->CalculateBounds();
}

// Draws the current brush
void _EditorState::DrawBrush() {

	// Get selected palette
	std::string IconText;
	std::string IconID;
	std::string IconTotal;
	const ae::_Texture *IconTexture = nullptr;
	glm::vec4 IconColor = COLOR_WHITE;
	if(Brush[EditMode]) {
		IconID = Brush[EditMode]->Name;
		IconText = Brush[EditMode]->Style->Name;
		IconTexture = Brush[EditMode]->Style->Texture;
		IconColor = Brush[EditMode]->Style->TextureColor;
	}

	// Get brush icon/texture position
	glm::vec2 IconPosition(382 + EDITOR_PALETTE_SELECTEDSIZE, ae::Graphics.CurrentSize.y - EDITOR_PALETTE_SELECTEDSIZE - 8);
	glm::vec2 NamePosition = glm::vec2(EDITOR_PALETTE_SELECTEDSIZE + 16, -EDITOR_PALETTE_SELECTEDSIZE / 2);
	float IconRotation = 0;
	float IconScale = 1.0f;
	float IconScaleX = 1.0f;
	float TextSpacingY = 18;
	int SelectedObjectLevel = 1;

	// Edit mode specific text
	switch(EditMode) {
		case EDITMODE_BLOCKS: {

			// See if there's a selected block
			std::string BlockAltTextureID;
			float BlockMinZ;
			float BlockMaxZ;
			bool BlockWalkable;
			float TextRotation;
			glm::vec4 BlockColor(BrushColor);
			glm::vec2 BlockSize(0.0f);
			if(SelectedBlocks.size() == 1) {
				_Block *SelectedBlock = Map->GetBlock(EditLayer, SelectedBlocks[0]);
				IconText = "";
				BlockColor = SelectedBlock->Color;
				BlockSize = SelectedBlock->End - SelectedBlock->Start + glm::ivec2(1);
				IconTexture = SelectedBlock->Texture;
				if(IconTexture)
					IconText = IconTexture->Name;
				if(EditLayer != MAPLAYER_FLAT)
					IconRotation = SelectedBlock->Rotation;
				TextRotation = SelectedBlock->Rotation;
				IconScaleX = SelectedBlock->ScaleX;
				BlockMinZ = SelectedBlock->MinZ;
				BlockMaxZ = SelectedBlock->MaxZ;
				BlockWalkable = SelectedBlock->Walkable;
				if(SelectedBlock->AltTexture)
					BlockAltTextureID = SelectedBlock->AltTexture->Name;
			}
			else {
				if(Brush[EditMode])
					IconText = Brush[EditMode]->Name;
				if(EditLayer != MAPLAYER_FLAT)
					IconRotation = Rotation;
				TextRotation = Rotation;
				IconScaleX = ScaleX;
				BlockMinZ = MinZ;
				BlockMaxZ = MaxZ;
				BlockWalkable = Walkable;
				BlockAltTextureID = AltTextureID;
			}
			IconText.erase(0, std::string(MAP_TEXTURE_PATH).length());
			BlockAltTextureID.erase(0, std::string(MAP_TEXTURE_PATH).length());

			IconID = "";

			glm::vec2 TextPosition(IconPosition.x + EDITOR_PALETTE_SELECTEDSIZE + 290, ae::Graphics.ViewportSize.y + 30);
			glm::vec2 ValueOffset(5, 0);
			std::ostringstream Buffer;

			if(BlockSize.x != 0 && BlockSize.y != 0) {
				Buffer << BlockSize.x << "x" << BlockSize.y;
				MainFont->DrawText("Size:", glm::ivec2(TextPosition), ae::RIGHT_BASELINE);
				MainFont->DrawText(Buffer.str(), glm::ivec2(TextPosition + ValueOffset));
				Buffer.str("");
			}
			TextPosition.y += TextSpacingY;

			Buffer << ae::Round2(BlockColor.r) << ","  << ae::Round2(BlockColor.g) << "," << ae::Round2(BlockColor.b);
			MainFont->DrawText("Color:", glm::ivec2(TextPosition), ae::RIGHT_BASELINE);
			MainFont->DrawText(Buffer.str(), glm::ivec2(TextPosition + ValueOffset));
			Buffer.str("");
			TextPosition.y += TextSpacingY;

			Buffer << TextRotation;
			MainFont->DrawText("Rotation:", glm::ivec2(TextPosition), ae::RIGHT_BASELINE);
			MainFont->DrawText(Buffer.str(), glm::ivec2(TextPosition + ValueOffset));
			Buffer.str("");
			TextPosition.y += TextSpacingY;

			Buffer << BlockMinZ;
			MainFont->DrawText("Min Z:", glm::ivec2(TextPosition), ae::RIGHT_BASELINE);
			MainFont->DrawText(Buffer.str(), glm::ivec2(TextPosition + ValueOffset));
			Buffer.str("");
			TextPosition.y += TextSpacingY;

			Buffer << BlockMaxZ;
			MainFont->DrawText("Max Z:", glm::ivec2(TextPosition), ae::RIGHT_BASELINE);
			MainFont->DrawText(Buffer.str(), glm::ivec2(TextPosition + ValueOffset));
			Buffer.str("");
			TextPosition.y += TextSpacingY;

			Buffer << IconScaleX;
			MainFont->DrawText("ScaleX:", glm::ivec2(TextPosition), ae::RIGHT_BASELINE);
			MainFont->DrawText(Buffer.str(), glm::ivec2(TextPosition + ValueOffset));
			Buffer.str("");
			TextPosition.y += TextSpacingY;

			Buffer << BlockWalkable;
			MainFont->DrawText("Walk:", glm::ivec2(TextPosition), ae::RIGHT_BASELINE);
			MainFont->DrawText(Buffer.str(), glm::ivec2(TextPosition + ValueOffset));
			Buffer.str("");

			IconID = BlockAltTextureID;
		} break;
		case EDITMODE_EVENTS: {

			// Get object identifiers
			std::string ItemID;
			std::string MonsterID;
			std::string ParticleID;
			std::string SoundID;
			double ActivationPeriod;
			int Active;
			int Level;
			int SpawnLevel;
			if(EventSelected()) {
				ae::_Element *Button = PaletteElement[EDITMODE_EVENTS]->Children[SelectedEvent->Type];
				IconTexture = Button->Style->Texture;
				IconID = Button->Name;
				IconText = Button->Style->Name;

				ItemID = SelectedEvent->ItemID;
				MonsterID = SelectedEvent->MonsterID;
				ParticleID = SelectedEvent->ParticleID;
				SoundID = SelectedEvent->SoundID;
				Active = SelectedEvent->Active;
				Level = SelectedEvent->Level;
				SpawnLevel = SelectedEvent->SpawnLevel;
				ActivationPeriod = SelectedEvent->ActivationPeriod;
			}
			else {
				ItemID = SavedText[EDITINPUT_ITEMID];
				MonsterID = SavedText[EDITINPUT_MONSTERID];
				ParticleID = SavedText[EDITINPUT_PARTICLEID];
				SoundID = SavedText[EDITINPUT_SOUNDID];
				Active = EventActive;
				Level = EventLevel;
				SpawnLevel = EventSpawnLevel;
				ActivationPeriod = EventActivationPeriod;
			}

			glm::vec2 TextPosition(IconPosition.x + EDITOR_PALETTE_SELECTEDSIZE + 260, IconPosition.y - EDITOR_PALETTE_SELECTEDSIZE/2);
			glm::vec2 ValueOffset(5, 0);

			std::ostringstream Buffer;
			Buffer << Active;
			MainFont->DrawText("Active:", glm::ivec2(TextPosition), ae::RIGHT_BASELINE);
			MainFont->DrawText(Buffer.str(), glm::ivec2(TextPosition + ValueOffset));
			Buffer.str("");

			TextPosition.y += TextSpacingY;
			Buffer << Level << ":" << ActivationPeriod;
			MainFont->DrawText("Level:", glm::ivec2(TextPosition), ae::RIGHT_BASELINE);
			MainFont->DrawText(Buffer.str(), glm::ivec2(TextPosition + ValueOffset));
			Buffer.str("");

			TextPosition.y += TextSpacingY;
			Buffer << SpawnLevel;
			MainFont->DrawText("Spawn Level:", glm::ivec2(TextPosition), ae::RIGHT_BASELINE);
			MainFont->DrawText(Buffer.str(), glm::ivec2(TextPosition + ValueOffset));
			Buffer.str("");

			TextPosition.x += 150;
			TextPosition.y -= TextSpacingY * 3;
			MainFont->DrawText("Item:", glm::ivec2(TextPosition), ae::RIGHT_BASELINE);
			MainFont->DrawText(ItemID, glm::ivec2(TextPosition + ValueOffset));

			TextPosition.y += TextSpacingY;
			MainFont->DrawText("Monster:", glm::ivec2(TextPosition), ae::RIGHT_BASELINE);
			MainFont->DrawText(MonsterID, glm::ivec2(TextPosition + ValueOffset));

			TextPosition.y += TextSpacingY;
			MainFont->DrawText("Particle:", glm::ivec2(TextPosition), ae::RIGHT_BASELINE);
			MainFont->DrawText(ParticleID, glm::ivec2(TextPosition + ValueOffset));

			TextPosition.y += TextSpacingY;
			MainFont->DrawText("Sound:", glm::ivec2(TextPosition), ae::RIGHT_BASELINE);
			MainFont->DrawText(SoundID, glm::ivec2(TextPosition + ValueOffset));

			IconID = "";
		} break;
		default: {

			// See if there's a selected object
			SelectedObjectLevel = ObjectLevel;
			if(SelectedObjects.size() > 0) {
				auto Iterator = SelectedObjects.begin();
				IconID = (*Iterator)->ID;
				IconText = "";
				IconTexture = nullptr;
				IconRotation = (*Iterator)->Rotation;
				IconScale = (*Iterator)->Scale;
				SelectedObjectLevel = (*Iterator)->Level;
			}

			IconTotal = std::to_string(ObjectCounts[IconID]);
		} break;
	}

	// Brush information name and id
	if(IconText != "")
		MainFont->DrawText(IconText, glm::ivec2(IconPosition + NamePosition), ae::LEFT_BASELINE);

	NamePosition.y += TextSpacingY;
	if(IconID != "") {
		MainFont->DrawText(IconID, glm::ivec2(IconPosition + NamePosition), ae::LEFT_BASELINE);

		if(EditMode == EDITMODE_ITEMS || EditMode == EDITMODE_MONSTERS ) {

			// Draw object level
			NamePosition.y += TextSpacingY;
			MainFont->DrawText("Level: " + std::to_string(SelectedObjectLevel), glm::ivec2(IconPosition + NamePosition), ae::LEFT_BASELINE);

			// Draw object total in level
			NamePosition.y += TextSpacingY;
			MainFont->DrawText("Total: " + IconTotal, glm::ivec2(IconPosition + NamePosition), ae::LEFT_BASELINE);
		}
		else if(EditMode == EDITMODE_PROPS) {

			// Draw object rotation
			NamePosition.y += TextSpacingY;
			MainFont->DrawText("Rotation: " + std::to_string(IconRotation), glm::ivec2(IconPosition + NamePosition), ae::LEFT_BASELINE);

			// Draw object scale
			NamePosition.y += TextSpacingY;
			MainFont->DrawText("Scale: " + std::to_string(IconScale), glm::ivec2(IconPosition + NamePosition), ae::LEFT_BASELINE);
		}
	}

	if(IconTexture) {
		 ae::_Program *Program = ae::Assets.Programs["ortho_pos_uv"];
		ae::Graphics.SetProgram(Program);
		ae::Graphics.SetColor(IconColor);
		Program->ResetTransform(Program->TextureTransformID);
		glm::vec3 DrawPosition = glm::vec3(IconPosition, 0.0f);
		glm::vec2 IconScale = glm::vec2(IconScaleX * EDITOR_PALETTE_SELECTEDSIZE * 2, EDITOR_PALETTE_SELECTEDSIZE * 2);
		ae::Graphics.DrawSprite(DrawPosition, IconTexture, IconRotation * IconScaleX, IconScale);
	}
}

// Draw tiles for an event
void _EditorState::DrawEventTiles(_Event *Event, const glm::vec4 &Color) {
	const std::vector<_EventTile> &Tiles = Event->Tiles;
	for(size_t i = 0; i < Tiles.size(); i++) {
		ae::Graphics.SetColor(Color);
		ae::Graphics.DrawRectangle3D(glm::vec2(Tiles[i].Coord.x + 0.2f, Tiles[i].Coord.y + 0.2f), glm::vec2(Tiles[i].Coord.x + 0.8f, Tiles[i].Coord.y + 0.8f), false);

		if(Tiles[i].BlockID == -1)
			continue;

		if(SelectedEvent->Type == EVENT_ENABLE) {
			const _Event *Event = Map->Events[Tiles[i].BlockID];
			ae::Graphics.SetColor(COLOR_YELLOW);
			ae::Graphics.DrawRectangle3D(glm::vec2(Event->Start.x, Event->Start.y), glm::vec2(Event->End.x + 1.0f, Event->End.y + 1.0f), false);
		}
		else {
			_Block *Block = Map->GetBlock(Tiles[i].Layer, Tiles[i].BlockID);
			ae::Graphics.SetColor(COLOR_GREEN);
			ae::Graphics.DrawRectangle3D(glm::vec2(Block->Start.x, Block->Start.y), glm::vec2(Block->End.x + 1.0f, Block->End.y + 1.0f), false);
		}
	}
}

// Draws an object
void _EditorState::DrawObject(float OffsetX, float OffsetY, const _ObjectSpawn *ObjectSpawn, float Alpha) {
	float Rotation = 0.0f;
	float Scale = ITEM_SCALE;
	float Depth = 0.0f;
	glm::vec4 Color;
	const ae::_Texture *Texture = nullptr;
	const ae::_Mesh *Mesh = nullptr;
	switch(ObjectSpawn->Type) {
		case _Object::MONSTER: {
			_ObjectTemplate &Monster = Stats.Objects.at(ObjectSpawn->ID);
			Texture = ae::Assets.Textures["textures/icons/" + Monster.AnimationID + ".png"];
			Mesh = ae::Assets.Meshes[Monster.MeshID];
			Color = Monster.Color;
			Scale = Monster.Attributes.at("scale").Float;
			Depth = Mesh ? 0.0f : OBJECT_Z;
		} break;
		case _Object::KEY:
		case _Object::AMMO:
		case _Object::MOD:
		case _Object::ARMOR:
		case _Object::WEAPON:
		case _Object::MEDKIT: {
			_ObjectTemplate &Item = Stats.Objects.at(ObjectSpawn->ID);
			Texture = ae::Assets.Textures[Item.IconID];
			Color = Item.Color;
			Depth = ITEM_Z;
		} break;
		case _Object::PROP: {
			_ObjectTemplate &ObjectTemplate = Stats.Objects.at(ObjectSpawn->ID);
			Texture = ae::Assets.Textures[ObjectTemplate.IconID];
			Mesh = ae::Assets.Meshes[ObjectTemplate.MeshID];
			Rotation = ObjectSpawn->Rotation;
			Color = ObjectTemplate.Color;
			Scale = ObjectTemplate.Attributes.at("scale").Float * ObjectSpawn->Scale;
		} break;
	}

	if(!Texture)
		return;

	glm::vec2 DrawPosition(ObjectSpawn->Position.x + OffsetX, ObjectSpawn->Position.y + OffsetY);
	if(!Camera->IsCircleInView(DrawPosition, Scale))
		return;

	Color.a *= Alpha;
	ae::_Program *Program;
	if(Mesh) {
		Program = ae::Assets.Programs["map_norm"];
		ae::Graphics.SetProgram(Program);
		Program->ResetTransform(Program->TextureTransformID);

		ae::Graphics.SetColor(Color);
		ae::Graphics.SetDepthMask(true);
		ae::Graphics.SetCullFace(true);
		ae::Graphics.DrawMesh(glm::vec3(DrawPosition, Depth), Mesh, Texture, Rotation, glm::vec3(Scale));
		ae::Graphics.SetCullFace(false);
	}
	else {
		Program = ae::Assets.Programs["pos_uv"];
		ae::Graphics.SetProgram(Program);
		Program->ResetTransform(Program->TextureTransformID);

		ae::Graphics.SetColor(Color);
		ae::Graphics.SetDepthMask(false);
		ae::Graphics.DrawSprite(glm::vec3(DrawPosition, Depth), Texture, Rotation, glm::vec2(Scale));
	}
}

// Processes clicks on the buttons
void _EditorState::ProcessIcons(int Index, int Type) {
	switch(Index) {
		case ICON_LAYER1:
			ExecuteUpdateLayer(0, false);
		break;
		case ICON_LAYER2:
			ExecuteUpdateLayer(1, false);
		break;
		case ICON_LAYER3:
			ExecuteUpdateLayer(2, false);
		break;
		case ICON_LAYER4:
			ExecuteUpdateLayer(3, false);
		break;
		case ICON_LAYER5:
			ExecuteUpdateLayer(4, false);
		break;
		case ICON_WALL:
			ExecuteUpdateLayer(5, false);
		break;
		case ICON_FORE:
			ExecuteUpdateLayer(6, false);
		break;
		case ICON_DOWN:
			ExecuteShiftLayer(-1);
		break;
		case ICON_UP:
			ExecuteShiftLayer(1);
		break;
		case ICON_BLOCK:
			ExecuteSwitchMode(EDITMODE_BLOCKS);
		break;
		case ICON_EVENT:
			ExecuteSwitchMode(EDITMODE_EVENTS);
		break;
		case ICON_ITEM:
			ExecuteSwitchMode(EDITMODE_ITEMS);
		break;
		case ICON_MONSTER:
			ExecuteSwitchMode(EDITMODE_MONSTERS);
		break;
		case ICON_NONE:
			ExecuteDeselect();
		break;
		case ICON_DELETE:
			ExecuteDelete();
		break;
		case ICON_COPY:
			ExecuteCopy();
		break;
		case ICON_SHOW:
			ExecuteHighlightBlocks();
		break;
		case ICON_NEW:
			ExecuteClear();
		break;
		case ICON_GRID:
			if(IsShiftDown)
				ExecuteUpdateGridMode(-1);
			else
				ExecuteUpdateGridMode(1);
		break;
		case ICON_LOAD:
			ExecuteShowInput(EDITINPUT_LOAD);
		break;
		case ICON_SAVE:
			ExecuteShowInput(EDITINPUT_SAVE);
		break;
		case ICON_TEST:
			ExecuteTest();
		break;
		default:
		break;
	}
}

// Processes clicks on the block buttons
void _EditorState::ProcessBlockIcons(int Index, int Type) {
	switch(Index) {
		case ICON_COLOR:
			if(SelectedBlocks.size())
				ExecuteShowInput(EDITINPUT_COLOR);
		break;
		case ICON_WALK:
			ExecuteWalkable();
		break;
		case ICON_ROTATE:
			ExecuteRotate();
		break;
		case ICON_MIRROR:
			ExecuteMirror();
		break;
		case ICON_RAISE:
			ExecuteChangeZ(0.5f, !IsShiftDown);
		break;
		case ICON_LOWER:
			ExecuteChangeZ(-0.5f, !IsShiftDown);
		break;
	}
}

// Processes clicks on the event buttons
void _EditorState::ProcessEventIcons(int Index, int Type) {

	switch(Index) {
		case ICON_TILE:
		break;
		case ICON_LEVELUP:
			if(Type)
				ExecuteChangeLevel(5);
			else
				ExecuteChangeLevel(1);
		break;
		case ICON_LEVELDOWN:
			if(Type)
				ExecuteChangeLevel(-5);
			else
				ExecuteChangeLevel(-1);
		break;
		case ICON_PERIODUP:
			if(Type)
				ExecuteChangePeriod(EDITOR_PERIODADJUST);
			else
				ExecuteChangePeriod(EDITOR_PERIODADJUST * 10);
		break;
		case ICON_PERIODDOWN:
			if(Type)
				ExecuteChangePeriod(-EDITOR_PERIODADJUST);
			else
				ExecuteChangePeriod(-EDITOR_PERIODADJUST * 10);
		break;
		case ICON_ACTIVE:
			ExecuteChangeActive();
		break;
		case ICON_ITEMID:
			ExecuteShowInput(EDITINPUT_ITEMID);
		break;
		case ICON_MONSTERID:
			ExecuteShowInput(EDITINPUT_MONSTERID);
		break;
		case ICON_PARTICLEID:
			ExecuteShowInput(EDITINPUT_PARTICLEID);
		break;
		case ICON_SOUNDID:
			ExecuteShowInput(EDITINPUT_SOUNDID);
		break;
	}
}

// Adds an object to the list
void _EditorState::SpawnObject(const glm::vec2 &Position, float Rotation, float Scale, int Type, const std::string &ID, int Level, bool Align) {
	glm::vec2 SpawnPosition = Align ? AlignToGrid(Position) : Position;

	_ObjectSpawn *ObjectSpawn = new _ObjectSpawn(ID, SpawnPosition, Type, Level);
	ObjectSpawn->Rotation = Rotation;
	ObjectSpawn->Scale = Scale;
	Map->ObjectSpawns.push_back(ObjectSpawn);
}

// Adds an event to the list
void _EditorState::AddEvent(int Type) {
	if(Type == -1)
		return;

	// Set up event
	bool AddTile = false;
	int TileLayer = -1;
	glm::ivec2 Start = DrawStart;
	glm::ivec2 End = DrawEnd - 1;
	switch(Type) {
		case EVENT_DOOR:
			TileLayer = MAPLAYER_FLAT;
			AddTile = true;
		break;
		case EVENT_WALLSWITCH:
			TileLayer = EditLayer;
			AddTile = true;
		break;
		case EVENT_SPAWN:
			EventLevel = std::max(1, EventLevel);
		break;
		case EVENT_TELEPORT:
		break;
		default:
		break;
	}

	_Event *Event = new _Event();
	Event->Type = Type;
	Event->Active = EventActive;
	Event->Start = Start;
	Event->End = End;
	Event->Level = EventLevel;
	Event->SpawnLevel = EventSpawnLevel;
	Event->ActivationPeriod = EventActivationPeriod;
	Event->ItemID = SavedText[EDITINPUT_ITEMID];
	Event->MonsterID = SavedText[EDITINPUT_MONSTERID];
	Event->ParticleID = SavedText[EDITINPUT_PARTICLEID];
	Event->SoundID = SavedText[EDITINPUT_SOUNDID];
	if(AddTile) {
		int BlockIndex = Map->GetSelectedBlock(TileLayer, Start);
		Event->AddTile(_EventTile(Start, TileLayer, BlockIndex));
	}

	Map->AddEvent(Event);
}

// Updates the selected event's object id
void _EditorState::UpdateEventID(int Type, const std::string &ID) {
	if(!EventSelected())
		return;

	switch(Type) {
		case EDITINPUT_ITEMID:
			SelectedEvent->ItemID = ID;
		break;
		case EDITINPUT_MONSTERID:
			SelectedEvent->MonsterID = ID;
		break;
		case EDITINPUT_PARTICLEID:
			SelectedEvent->ParticleID = ID;
		break;
		case EDITINPUT_SOUNDID:
			SelectedEvent->SoundID = ID;
		break;
	}
}

// Gets the selected event's object id
std::string _EditorState::GetEventID(int Type) {
	if(!EventSelected())
		return "";

	switch(Type) {
		case EDITINPUT_ITEMID:
			return SelectedEvent->ItemID;
		break;
		case EDITINPUT_MONSTERID:
			return SelectedEvent->MonsterID;
		break;
		case EDITINPUT_PARTICLEID:
			return SelectedEvent->ParticleID;
		break;
		case EDITINPUT_SOUNDID:
			return SelectedEvent->SoundID;
		break;
	}

	return "";
}

// Returns a valid position for the object
glm::vec2 _EditorState::GetValidObjectPosition(const glm::vec2 &Position) const {
	return glm::clamp(Position, glm::vec2(0.0f), glm::vec2(Map->Size) - MAP_EPSILON);
}

// Determines if an object is part of the selected objects list
bool _EditorState::ObjectInSelectedList(_ObjectSpawn *Object) {
	for(auto Iterator : SelectedObjects) {
		if(Object == Iterator)
			return true;
	}

	return false;
}

// Executes the walkable command
void _EditorState::ExecuteWalkable() {
	if(SelectedBlocks.size()) {
		for(const auto &Index : SelectedBlocks) {
			_Block *Block = Map->GetBlock(EditLayer, (size_t)Index);
			Block->Walkable = !Block->Walkable || (EditLayer == MAPLAYER_FORE);
		}
	}
	else
		Walkable = !Walkable || (EditLayer == MAPLAYER_FORE);
}

// Executes the rotate command
void _EditorState::ExecuteRotate() {
	if(SelectedBlocks.size()) {
		for(const auto &Index : SelectedBlocks) {
			_Block *Block = Map->GetBlock(EditLayer, (size_t)Index);
			Block->Rotation += 90.0f;
			if(Block->Rotation > 359.0f)
				Block->Rotation = 0.0f;
		}
	}
	else if(SelectedObjects.size()) {
		for(const auto &Object : SelectedObjects) {
			Object->Rotation += 90.0f;
			if(Object->Rotation > 359.0f)
				Object->Rotation = 0.0f;
		}
	}
	else {
		Rotation += 90.0f;
		if(Rotation > 359.0f)
			Rotation = 0.0f;
	}
}

// Executes the mirror texture command
void _EditorState::ExecuteMirror() {
	if(SelectedBlocks.size()) {
		for(const auto &Index : SelectedBlocks) {
			_Block *Block = Map->GetBlock(EditLayer, (size_t)Index);
			Block->ScaleX = -Block->ScaleX;
		}
	}
	else
		ScaleX = -ScaleX;
}

// Executes the mirror texture command
void _EditorState::ExecuteToggleTile() {
	if(!EventSelected())
		return;

	auto Iterator = SelectedEvent->FindTile(WorldCursorIndex);
	if(Iterator == SelectedEvent->Tiles.end()) {
		switch(SelectedEvent->Type) {
			case EVENT_DOOR:
			case EVENT_WALLSWITCH:
			case EVENT_FLOORSWITCH: {
				_Block *Block;
				int BlockIndex = Map->GetSelectedBlock(EditLayer, WorldCursorIndex, &Block);
				SelectedEvent->AddTile(_EventTile(WorldCursorIndex, EditLayer, BlockIndex));
			} break;
			case EVENT_ENABLE: {
				_Event *Event;
				int EventIndex = Map->GetSelectedEvent(WorldCursorIndex, &Event);
				SelectedEvent->AddTile(_EventTile(WorldCursorIndex, -1, EventIndex));
			} break;
			default:
				SelectedEvent->AddTile(_EventTile(WorldCursorIndex, -1, -1));
			break;
		}
	}
	else if(!IsShiftDown)
		SelectedEvent->RemoveTile(Iterator);
}

// Executes the change Z command
void _EditorState::ExecuteChangeZ(float Change, int Type) {
	if(Type == 0) {
		if(SelectedBlocks.size()) {
			for(const auto &Index : SelectedBlocks) {
				_Block *Block = Map->GetBlock(EditLayer, (size_t)Index);
				Block->MinZ += Change;
			}
		}
		else
			MinZ += Change;
	}
	else {
		if(SelectedBlocks.size()) {
			for(const auto &Index : SelectedBlocks) {
				_Block *Block = Map->GetBlock(EditLayer, (size_t)Index);
				Block->MaxZ += Change;
			}
		}
		else
			MaxZ += Change;
	}
}

// Executes the change level command
void _EditorState::ExecuteChangeLevel(int Change) {
	if(EventSelected()) {
		int MinLevel = SelectedEvent->Type == EVENT_SPAWN ? 1 : 0;

		// Change level for all the next checkpoints if control is held
		if(SelectedEvent->Type == EVENT_CHECK && IsCtrlDown && Change) {
			for(auto &Event : Map->Events) {
				if(Event->Type == SelectedEvent->Type && Event->Level > SelectedEvent->Level)
					Event->Level = std::max(MinLevel, Event->Level + Change);
			}
		}
		SelectedEvent->Level = std::max(MinLevel, SelectedEvent->Level + Change);
	}
	else
		EventLevel = std::max(0, EventLevel + Change);
}

// Executes the change activation period command
void _EditorState::ExecuteChangePeriod(double Value) {
	if(IsShiftDown)
		Value *= 5;

	if(EventSelected())
		SelectedEvent->ActivationPeriod = std::max(0.0, SelectedEvent->ActivationPeriod + Value);
	else
		EventActivationPeriod  = std::max(0.0, EventActivationPeriod + Value);
}

// Executes the change active command
void _EditorState::ExecuteChangeActive() {
	if(EventSelected())
		SelectedEvent->Active = !SelectedEvent->Active;
	else
		EventActive = !EventActive;
}

// Executes the change checkpoint command
void _EditorState::ExecuteUpdateCheckpointIndex(int Value) {
	CheckpointIndex = std::max(0, CheckpointIndex + Value);
}

// Executes the show input command
void _EditorState::ExecuteShowInput(int Type) {
	EditorInput = Type;
	InputBox->SetActive(true);
	ae::_Element *TextBox = InputBox->Children.front();
	ae::_Element *Label = TextBox->Children.front();
	Label->Text = InputBoxStrings[Type];
	if(Type >= EDITINPUT_ITEMID && Type <= EDITINPUT_SOUNDID && EventSelected())
		TextBox->SetText(GetEventID(Type));
	else
		TextBox->SetText(SavedText[Type]);

	ae::FocusedElement = TextBox;
}

// Executes the clear map command
void _EditorState::ExecuteClear() {
	LoadMap("", false);
	SavedText[EDITINPUT_SAVE] = "";
}

// Executes the test command
void _EditorState::ExecuteTest() {

	try {
		Map->Save(EDITOR_TESTLEVEL);
	}
	catch(std::exception &Error) {
		std::cout << Error.what() << std::endl;
		return;
	}

	ExecuteDeselect();
	ClearClipboard();

	PlayState.TestMode = true;
	PlayState.FromEditor = true;
	PlayState.Level = EDITOR_TESTLEVEL;
	PlayState.CheckpointIndex = CheckpointIndex;
	Framework.ChangeState(&PlayState);
}

// Executes the delete command
void _EditorState::ExecuteDelete() {

	switch(EditMode) {
		case EDITMODE_BLOCKS:
			Map->DeleteBlocks(EditLayer, SelectedBlocks);
			DeselectBlocks();
		break;
		case EDITMODE_EVENTS:
			if(EventSelected()) {
				Map->RemoveEvent(SelectedEventIndex);
				DeselectEvent();
				ClipboardEvent = nullptr;
			}
		break;
		default:
			if(SelectedObjects.size()) {
				for(auto &Object : SelectedObjects)
					Object->Deleted = true;

				Map->CleanObjectSpawns();
				DeselectObjects();
				ClipboardObjects.clear();
			}
		break;
	}
}

// Executes the copy command
void _EditorState::ExecuteCopy() {

	switch(EditMode) {
		case EDITMODE_BLOCKS:
			if(SelectedBlocks.empty())
				return;

			// Copy block into brush
			if(SelectedBlocks.size() == 1) {
				_Block *Block = Map->GetBlock(EditLayer, SelectedBlocks[0]);
				MinZ = Block->MinZ;
				MaxZ = Block->MaxZ;
				Rotation = Block->Rotation;
				ScaleX = Block->ScaleX;
				Walkable = Block->Walkable;
				BrushColor = Block->Color;
				Brush[EDITMODE_BLOCKS] = GetBrushFromTexture(EDITMODE_BLOCKS, Block->Texture);
				ae::_Element *AltBrush = GetBrushFromTexture(EDITMODE_BLOCKS, Block->AltTexture);
				if(AltBrush) {
					AltTextureID = AltBrush->Name;
					AltTexture = AltBrush->Style->Texture;
				}
				else {
					AltTextureID = "";
					AltTexture = nullptr;
				}
			}

			ClipboardBlocks[EditLayer].clear();
			for(const auto &Index : SelectedBlocks) {
				_Block Block = *Map->GetBlock(EditLayer, (size_t)Index);
				Block.Start.x -= SelectionBounds[0];
				Block.Start.y -= SelectionBounds[1];
				Block.End.x -= SelectionBounds[0];
				Block.End.y -= SelectionBounds[1];
				ClipboardBlocks[EditLayer].push_back(Block);
			}
			DeselectBlocks();
		break;
		case EDITMODE_EVENTS:
			if(EventSelected()) {
				ClipboardEvent = SelectedEvent;
				DeselectEvent();
			}
		break;
		default:
			if(SelectedObjects.size()) {
				CopiedPosition = WorldCursor;
				ClipboardObjects = SelectedObjects;
			}
		break;
	}
}

// Executes the paste command
void _EditorState::ExecutePaste(int PasteMode) {

	glm::vec2 StartPosition = WorldCursor;
	switch(EditMode) {
		case EDITMODE_BLOCKS:
			if(SelectedBlocks.size() && ClipboardBlocks[EditLayer].size() == 1) {
				_Block &ClipboardBlock = ClipboardBlocks[EditLayer][0];
				for(const auto &Index : SelectedBlocks) {
					_Block *Block = Map->GetBlock(EditLayer, (size_t)Index);
					if(PasteMode == 0) {
						Block->Color = ClipboardBlock.Color;
						Block->Texture = ClipboardBlock.Texture;
					}
					else if(PasteMode == 1)
						Block->Color = ClipboardBlock.Color;
					else if(PasteMode == 2)
						Block->Texture = ClipboardBlock.Texture;
				}
			}
			else {
				for(auto Block : ClipboardBlocks[EditLayer]) {
					Block.Start = Map->GetValidCoord(glm::ivec2(StartPosition) + Block.Start);
					Block.End = Map->GetValidCoord(glm::ivec2(StartPosition) + Block.End);
					Map->AddBlock(EditLayer, Block);
				}
			}
		break;
		case EDITMODE_EVENTS: {
			if(!ClipboardEvent)
				break;

			if(SelectedEvent) {
				SelectedEvent->Level = ClipboardEvent->Level;
				SelectedEvent->SpawnLevel = ClipboardEvent->SpawnLevel;
				SelectedEvent->ActivationPeriod = ClipboardEvent->ActivationPeriod;
				SelectedEvent->ItemID = ClipboardEvent->ItemID;
				SelectedEvent->MonsterID = ClipboardEvent->MonsterID;
				SelectedEvent->ParticleID = ClipboardEvent->ParticleID;
				SelectedEvent->SoundID = ClipboardEvent->SoundID;
			}
			else {
				DrawStart = Map->GetValidCoord(StartPosition);
				DrawEnd = Map->GetValidCoord(ClipboardEvent->End - ClipboardEvent->Start + glm::ivec2(StartPosition));

				_Event *Event = new _Event();
				Event->Type = ClipboardEvent->Type;
				Event->Active = ClipboardEvent->Active;
				Event->Start = DrawStart;
				Event->End = DrawEnd;
				Event->Level = ClipboardEvent->Level;
				Event->SpawnLevel = ClipboardEvent->SpawnLevel;
				Event->ActivationPeriod = ClipboardEvent->ActivationPeriod;
				Event->ItemID = ClipboardEvent->ItemID;
				Event->MonsterID = ClipboardEvent->MonsterID;
				Event->ParticleID = ClipboardEvent->ParticleID;
				Event->SoundID = ClipboardEvent->SoundID;

				Map->AddEvent(Event);
			}
		} break;
		default:
			for(auto Iterator : ClipboardObjects)
				SpawnObject(GetValidObjectPosition(StartPosition - CopiedPosition + Iterator->Position), Iterator->Rotation, Iterator->Scale, Iterator->Type, Iterator->ID, Iterator->Level, IsShiftDown);
		break;
	}
}

// Split block into two
void _EditorState::ExecuteSplit() {

	// Can only split one block at a time
	if(SelectedBlocks.size() != 1)
		return;

	// Get single selected block
	_Block *Block = Map->GetBlock(EditLayer, SelectedBlocks[0]);

	// Check for square blocks
	glm::vec2 Size = Block->End - Block->Start;
	if(Size.x == Size.y)
		return;

	// Cut horizontally
	_Block NewBlock = *Block;
	if(Block->GetLargestAxis()) {

		// Check cut point
		int CutPoint = (int)WorldCursor.y - 1;
		if(CutPoint < Block->Start.y || CutPoint >= Block->End.y)
			return;

		// Get new block bounds
		NewBlock.Start = glm::vec2(Block->Start.x, CutPoint + 1);

		// Resize first block
		Block->End = glm::vec2(Block->End.x, CutPoint);
	}
	// Cut vertically
	else {

		// Check cut point
		int CutPoint = (int)WorldCursor.x - 1;
		if(CutPoint < Block->Start.x || CutPoint >= Block->End.x)
			return;

		// Get new block bounds
		NewBlock.Start = glm::vec2(CutPoint + 1, Block->Start.y);

		// Resize first block
		Block->End = glm::vec2(CutPoint, Block->End.y);
	}
	DeselectBlocks();

	// Create new half
	Map->AddBlock(EditLayer, NewBlock);
}

// Executes the deselect command
void _EditorState::ExecuteDeselect() {
	DeselectBlocks();
	DeselectEvent();
	DeselectObjects();
}

// Moves to the previous/next icon in the palette
void _EditorState::ExecuteUpdateSelectedPalette(int Change) {
	std::vector<ae::_Element *> &Children = PaletteElement[EditMode]->Children;
	if(!Brush[EditMode]) {
		Brush[EditMode] = Children[0];
		return;
	}

	int CurrentIndex = Brush[EditMode]->Index;
	CurrentIndex += Change;

	// Wrap around
	if(CurrentIndex >= (int)Children.size())
		CurrentIndex = 0;
	else if(CurrentIndex < 0)
		CurrentIndex = Children.size() - 1;

	ExecuteSelectPalette(Children[CurrentIndex], 0);
}

// Executes the select palette command
void _EditorState::ExecuteSelectPalette(ae::_Element *Button, int ClickType) {
	if(!Button)
		return;

	if(Button->Index == -1) {

		// Deselect texture
		if(EditMode == EDITMODE_BLOCKS) {
			if(SelectedBlocks.size()) {
				for(const auto &Index : SelectedBlocks) {
					_Block *Block = Map->GetBlock(EditLayer, (size_t)Index);
					if(ClickType)
						Block->AltTexture = nullptr;
					else
						Block->Texture = nullptr;
				}
			}
			else {
				if(ClickType) {
					AltTextureID = "";
					AltTexture = nullptr;
				}
			}
		}

		return;
	}

	switch(EditMode) {
		case EDITMODE_BLOCKS:
			if(!Button)
				return;

			if(ClickType == 1) {
				if(SelectedBlocks.size()) {
					for(const auto &Index : SelectedBlocks) {
						_Block *Block = Map->GetBlock(EditLayer, (size_t)Index);
						Block->AltTexture = Button->Style->Texture;
					}
				}
				else {
					AltTextureID = Button->Name;
					AltTexture = Button->Style->Texture;
				}
			}
			else {
				if(SelectedBlocks.size()) {
					for(const auto &Index : SelectedBlocks) {
						_Block *Block = Map->GetBlock(EditLayer, (size_t)Index);
						Block->Texture = Button->Style->Texture;
					}
				}
			}
		break;
		case EDITMODE_EVENTS:
			if(!EventSelected()) {
				switch(Button->Index) {
					case EVENT_SPAWN:
						SetEventProperties(0, 1, 1, "smoke0");
					break;
					case EVENT_TEXT:
						SetEventProperties(5, 1, 1, "");
					break;
					case EVENT_DOOR:
					case EVENT_SOUND:
					case EVENT_WALLSWITCH:
					case EVENT_FLOORSWITCH:
					case EVENT_ENABLE:
						SetEventProperties(0, 1, 1, "");
					break;
					case EVENT_TELEPORT:
						SetEventProperties(0, 0, 1, "smoke0");
					break;
					case EVENT_LIGHT:
						SetEventProperties(0, 0, 1, "");
					break;
					case EVENT_LAVA:
						SetEventProperties(0, 1, 1, "smoke0");
					break;
					default:
						SetEventProperties(0, 0, 1, "");
					break;
				}
			}
		break;
		default:
			if(ClickType == 1 && EventSelected()) {
				switch(EditMode) {
					case EDITMODE_ITEMS:
						SelectedEvent->ItemID = Button->Name;
						ExecuteSwitchMode(EDITMODE_EVENTS);
					break;
					case EDITMODE_MONSTERS:
						SelectedEvent->MonsterID = Button->Name;
						ExecuteSwitchMode(EDITMODE_EVENTS);
					break;
					default:
					break;
				}
			}
		break;
	}

	if(ClickType == 0)
		Brush[EditMode] = Button;
}

// Executes the update grid command
void _EditorState::ExecuteUpdateGridMode(int Change) {
	GridMode += Change;
	if(GridMode > 10)
		GridMode = 0;
	else if(GridMode < 0)
		GridMode = 10;
}

// Executes the highlight command
void _EditorState::ExecuteHighlightBlocks() {
	HighlightBlocks = !HighlightBlocks;

	ae::Assets.Elements["button_editor_show"]->Checked = HighlightBlocks;
}

// Executes the toggle editor mode
void _EditorState::ExecuteSwitchMode(int State) {

	// Toggle icons
	if(EditMode != State) {
		ModeButtons[EditMode]->Checked = false;
		ModeButtons[State]->Checked = true;

		// Set state
		EditMode = State;
		BlockElement->SetActive(false);
		EventElement->SetActive(false);
		switch(EditMode) {
			case EDITMODE_BLOCKS:
				BlockElement->SetActive(true);
			break;
			case EDITMODE_EVENTS:
				EventElement->SetActive(true);
			break;
		}
	}
}

// Executes the update layer command
void _EditorState::ExecuteUpdateLayer(int Layer, bool Move) {
	if(EditLayer == Layer)
		return;

	// Changing layers for a selection of blocks
	if(Move && SelectedBlocks.size()) {

		// Sort ids descending so erasing starts from the end
		std::sort(SelectedBlocks.begin(), SelectedBlocks.end(), std::greater<int>());

		// Create new selection array
		std::vector<size_t> NewSelection;
		NewSelection.reserve(SelectedBlocks.size());
		for(const auto &Index : SelectedBlocks) {

			// Change layers and add to new selection
			size_t NewIndex = Map->ChangeLayer(EditLayer, Layer, Index);
			NewSelection.push_back(NewIndex);

			// Change block properties
			_Block *Block =	Map->GetBlock(Layer, NewIndex);
			Block->Walkable = (Layer == MAPLAYER_FLAT || Layer == MAPLAYER_WALL) ? false : true;
		}

		// Assign new selection
		SelectedBlocks = NewSelection;
	}
	else
		SelectedBlocks.clear();

	if(Layer == MAPLAYER_FLAT) {
		MaxZ = MAP_FLATZ;
		Walkable = false;
	}
	else if(Layer == MAPLAYER_WALL) {
		MaxZ = MAP_WALLZ;
		Walkable = false;
	}
	else if(Layer == MAPLAYER_FORE) {
		MaxZ = MAP_FOREGROUNDZ;
		Walkable = true;
	}
	else {
		MaxZ = 0.0f;
		Walkable = true;
	}
	MinZ = MAP_MINZ;

	// Toggle icons
	LayerButtons[EditLayer]->Checked = false;
	LayerButtons[Layer]->Checked = true;
	EditLayer = Layer;
}

// Executes the shift layer command
void _EditorState::ExecuteShiftLayer(int Change) {
	int NewLayer = EditLayer + Change;
	if(NewLayer > MAPLAYER_COUNT - 1)
		NewLayer = MAPLAYER_COUNT - 1;
	else if(NewLayer < 0)
		NewLayer = 0;

	ExecuteUpdateLayer(NewLayer, true);
}

// Executes the update block size command
void _EditorState::ExecuteUpdateBlockSize(int Direction, bool Expand) {
	if(EditMode == EDITMODE_BLOCKS) {
		for(const auto &Index : SelectedBlocks) {
			_Block *SelectedBlock = Map->GetBlock(EditLayer, Index);
			GetNewBlockSize(SelectedBlock->Start, SelectedBlock->End, Direction, Expand);
		}
	}
	else if(EditMode == EDITMODE_EVENTS && EventSelected()) {
		GetNewBlockSize(SelectedEvent->Start, SelectedEvent->End, Direction, Expand);
	}
}

// Update map level
void _EditorState::ExecuteUpdateMapLevel(int Change) {
	Map->Level = std::max(0, Map->Level + Change);
}

// Selects an object
void _EditorState::SelectObject() {
	ClickedPosition = WorldCursor;

	_ObjectSpawn *SelectedObject;
	size_t Index;
	Map->GetSelectedObject(WorldCursor, EDITOR_OBJECTRADIUS * EDITOR_OBJECTRADIUS, &SelectedObject, &Index);
	if(SelectedObject != nullptr) {
		IsMoving = true;

		// Single object selected
		if(!ObjectInSelectedList(SelectedObject)) {
			DeselectObjects();
			SelectedObjects.push_back(SelectedObject);
		}
	}
	else {
		DeselectObjects();
		DraggingBox = true;
	}
}

// Select multiple blocks
void _EditorState::SelectBlocks() {
	DeselectBlocks();
	Map->GetSelectedBlocks(ClickedPosition, WorldCursor, EditLayer, SelectedBlocks, SelectionBounds);
}

// Selects objects
void _EditorState::SelectObjects() {
	DeselectObjects();
	Map->GetSelectedObjects(ClickedPosition, WorldCursor, &SelectedObjects, EditMode == EDITMODE_MONSTERS);
	UpdateSelectionBounds();
}

// Update bounding box of selection
void _EditorState::UpdateSelectionBounds() {
	SelectionBounds[0] = Map->Size.x;
	SelectionBounds[1] = Map->Size.y;
	SelectionBounds[2] = -1;
	SelectionBounds[3] = -1;
	for(const auto &Index : SelectedBlocks) {
		_Block *Block = Map->GetBlock(EditLayer, Index);
		SelectionBounds[0] = std::min(SelectionBounds[0], Block->Start.x);
		SelectionBounds[1] = std::min(SelectionBounds[1], Block->Start.y);
		SelectionBounds[2] = std::max(SelectionBounds[2], Block->End.x);
		SelectionBounds[3] = std::max(SelectionBounds[3], Block->End.y);
	}
}

// Get tentative position
glm::vec2 _EditorState::GetMoveDeltaPosition(const glm::vec2 &Position) {
	glm::vec2 NewPosition;
	if(IsShiftDown)
		NewPosition = AlignToGrid(GetValidObjectPosition(Position + MoveDelta));
	else
		NewPosition = GetValidObjectPosition(Position + MoveDelta);

	return NewPosition;
}

// Find the brush that matches the given texture
ae::_Element *_EditorState::GetBrushFromTexture(size_t PaletteType, const ae::_Texture *Texture) {
	if(PaletteType >= EDITMODE_COUNT)
		return nullptr;

	for(const auto &Element : PaletteElement[PaletteType]->Children) {
		if(Texture == Element->Style->Texture)
			return Element;
	}

	return nullptr;
}

// Sets event properties
void _EditorState::SetEventProperties(double ActivationPeriod, int Level, int Active, const std::string &ParticleID) {
	EventActivationPeriod = ActivationPeriod;
	EventLevel = Level;
	EventActive = Active;
	SavedText[EDITINPUT_PARTICLEID] = ParticleID;
}

// Clears all the objects in the clipboard
void _EditorState::ClearClipboard() {
	ClipboardEvent = nullptr;
	ClipboardObjects.clear();
}

// Adjust start and end position of block
void _EditorState::GetNewBlockSize(glm::ivec2 &Start, glm::ivec2 &End, int Direction, bool Expand) {

	// Change side
	if(Expand) {
		switch(Direction) {
			case 0:
				if(Start.x > 0)
					Start.x--;
			break;
			case 1:
				if(Start.y > 0)
					Start.y--;
			break;
			case 2:
				if(End.x < Map->Size.x - 1)
					End.x++;
			break;
			case 3:
				if(End.y < Map->Size.y - 1)
					End.y++;
			break;
		}
	}
	else {
		switch(Direction) {
			case 0:
				if(End.x > Start.x)
					End.x--;
			break;
			case 1:
				if(End.y > Start.y)
					End.y--;
			break;
			case 2:
				if(Start.x < End.x)
					Start.x++;
			break;
			case 3:
				if(Start.y < End.y)
					Start.y++;
			break;
		}
	}
}
