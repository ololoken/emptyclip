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
#include <ae/graphics.h>
#include <ae/graphics.h>
#include <ae/font.h>
#include <ae/ui.h>
#include <ae/assets.h>
#include <ae/program.h>
#include <objects/monster.h>
#include <objects/weapon.h>
#include <objects/player.h>
#include <assets.h>
#include <framework.h>
#include <map.h>
#include <events.h>
#include <menu.h>
#include <animation.h>
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
	return First.ObjectType < Second.ObjectType || (First.ObjectType == Second.ObjectType && First.Identifier < Second.Identifier);
}

// Input box
const char *InputBoxStrings[EDITINPUT_COUNT] = {
	"Load monster set",
	"Load map",
	"Save map",
	"Set item",
	"Set monster",
	"Set particle",
};

// Input box
const int PaletteSizes[EDITMODE_COUNT] = {
	32,
	64,
	64,
	32,
	32,
	32,
	32,
	32,
};

// Constructor
_EditorState::_EditorState() :
	SavedCameraPosition(0, 0, CAMERA_DISTANCE),
	SavedCheckpointIndex(-1),
	MapFilename(""),
	SavedLayer(-1),
	SavedPalette(-1) {

}

void _EditorState::Init() {
	ae::Graphics.Element->SetActive(false);
	ae::Graphics.Element->Active = true;

	// Load command buttons
	MainFont = ae::Assets.Fonts["menu_buttons"];
	CommandElement = ae::Assets.Elements["element_editor_command"];
	BlockElement = ae::Assets.Elements["element_editor_blocks"];
	EventElement = ae::Assets.Elements["element_editor_events"];
	InputBox = ae::Assets.Elements["element_editor_input"];
	CommandElement->SetActive(true);
	BlockElement->SetActive(true);
	EventElement->SetActive(false);
	InputBox->SetActive(false);

	// Create button groups
	PaletteElement[0] = ae::Assets.Elements["element_editor_palette_block"];
	PaletteElement[1] = ae::Assets.Elements["element_editor_palette_events"];
	PaletteElement[2] = ae::Assets.Elements["element_editor_palette_monsters"];
	PaletteElement[3] = ae::Assets.Elements["element_editor_palette_items"];
	PaletteElement[4] = ae::Assets.Elements["element_editor_palette_ammo"];
	PaletteElement[5] = ae::Assets.Elements["element_editor_palette_upgrades"];
	PaletteElement[6] = ae::Assets.Elements["element_editor_palette_weapons"];
	PaletteElement[7] = ae::Assets.Elements["element_editor_palette_armors"];

	// Assign layer buttons
	LayerButtons[0] = ae::Assets.Elements["button_editor_layer_base"];
	LayerButtons[1] = ae::Assets.Elements["button_editor_layer_floor0"];
	LayerButtons[2] = ae::Assets.Elements["button_editor_layer_floor1"];
	LayerButtons[3] = ae::Assets.Elements["button_editor_layer_floor2"];
	LayerButtons[4] = ae::Assets.Elements["button_editor_layer_flat"];
	LayerButtons[5] = ae::Assets.Elements["button_editor_layer_wall"];
	LayerButtons[6] = ae::Assets.Elements["button_editor_layer_fore"];

	// Assign palette buttons
	ModeButtons[0] = ae::Assets.Elements["button_editor_mode_block"];
	ModeButtons[1] = ae::Assets.Elements["button_editor_mode_event"];
	ModeButtons[2] = ae::Assets.Elements["button_editor_mode_mons"];
	ModeButtons[3] = ae::Assets.Elements["button_editor_mode_item"];
	ModeButtons[4] = ae::Assets.Elements["button_editor_mode_ammo"];
	ModeButtons[5] = ae::Assets.Elements["button_editor_mode_mod"];
	ModeButtons[6] = ae::Assets.Elements["button_editor_mode_weap"];
	ModeButtons[7] = ae::Assets.Elements["button_editor_mode_arm"];

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

	// Adjust UI
	for(int i = 0; i < EDITMODE_COUNT; i++)
		PaletteElement[i]->SetHeight(ae::Graphics.ViewportSize.y - 30);

	if(SavedLayer != -1)
		ExecuteUpdateLayer(SavedLayer, false);

	if(SavedPalette != -1)
		ExecuteSwitchMode(SavedPalette);

	if(SavedCheckpointIndex != -1)
		CheckpointIndex = SavedCheckpointIndex;
}

void _EditorState::Close() {
	Camera->GetDrawPosition(0, SavedCameraPosition);
	SavedLayer = CurrentLayer;
	SavedPalette = CurrentPalette;
	SavedCheckpointIndex = CheckpointIndex;

	for(int i = 0; i < EDITMODE_COUNT; i++)
		ClearPalette(i);

	delete Camera;
	delete Map;

	Camera = nullptr;
	Map = nullptr;
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
		Map->LoadMonsterSet(MAP_DEFAULTMONSTERSET);
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
	SelectedBlockIndex = -1;
	SelectedEventIndex = -1;
	SelectedBlock = nullptr;
	SelectedEvent = nullptr;
	ClipboardEvent = nullptr;
	MinZ = 0.0f;
	MaxZ = 0.0f;
	Rotation = 0.0f;
	ScaleX = 1.0f;
	EventActivationPeriod = 0;
	EventActive = 1;
	EventLevel = 0;
	AltTextureIdentifier = "";
	AltTexture = nullptr;
	EditorInput = -1;
	CheckpointIndex = 0;
	ClickedPosition = glm::vec2(0, 0);
	CopiedPosition = glm::vec2(0, 0);
	MoveDelta = glm::vec2(0, 0);
	CurrentLayer = EDITOR_DEFAULT_LAYER;
	CurrentPalette = EDITMODE_BLOCKS;
	GridMode = EDITOR_DEFAULT_GRIDMODE;
	Walkable = true;
	HighlightBlocks = false;
	SelectedObjects.clear();
	ClipboardObjects.clear();

	IsShiftDown = false;
	IsCtrlDown = false;
	DraggingBox = false;
	BlockCopied = false;
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
	SavedIndex.x = 0;
	SavedIndex.y = 0;

	for(int i = 0; i < MAPLAYER_COUNT; i++)
		UndoNumber[i] = 0;

	// Enable default button
	for(int i = 0; i < MAPLAYER_COUNT; i++)
		LayerButtons[i]->Checked = false;

	for(int i = 0; i < EDITMODE_COUNT; i++) {
		ModeButtons[i]->Checked = false;
		Brush[i] = nullptr;
	}

	// Load palettes
	LoadPalettes();
	LayerButtons[CurrentLayer]->Checked = true;
	ModeButtons[CurrentPalette]->Checked = true;
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
					case EDITINPUT_LOADMONSTERSET:

						if(!Map->LoadMonsterSet(InputText))
							SavedText[EditorInput] = "";
						else {
							SavedText[EditorInput] = InputText;
							LoadMonsterButtons();
						}

						ExecuteDeselect();
					break;
					case EDITINPUT_LOAD: {
						if(InputText == "")
							break;

						if(LoadMap(InputText, false))
							SavedText[EDITINPUT_SAVE] = InputText;

					} break;
					case EDITINPUT_SAVE:
						if(InputText == "" || !Map->SaveLevel(InputText))
							SavedText[EditorInput] = "";
						else {
							SavedText[EditorInput] = InputText;
						}

						ExecuteDeselect();
					break;
					case EDITINPUT_ITEMIDENTIFIER:
					case EDITINPUT_MONSTERIDENTIFIER:
					case EDITINPUT_PARTICLEIDENTIFIER:
						UpdateEventIdentifier(EditorInput, InputText);

						if(!EventSelected())
							SavedText[EditorInput] = InputText;
					break;
				}
				EditorInput = -1;
				InputBox->SetActive(false);
			} break;
			case SDL_SCANCODE_ESCAPE:
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
				ExecuteSwitchMode(EDITMODE_MONSTERS);
			break;
			case SDL_SCANCODE_4:
				ExecuteSwitchMode(EDITMODE_ITEMS);
			break;
			case SDL_SCANCODE_5:
				ExecuteSwitchMode(EDITMODE_AMMO);
			break;
			case SDL_SCANCODE_6:
				ExecuteSwitchMode(EDITMODE_UPGRADES);
			break;
			case SDL_SCANCODE_7:
				ExecuteSwitchMode(EDITMODE_WEAPONS);
			break;
			case SDL_SCANCODE_8:
				ExecuteSwitchMode(EDITMODE_ARMOR);
			break;
			case SDL_SCANCODE_GRAVE:
				ExecuteDeselect();
			break;
			case SDL_SCANCODE_D:
				ExecuteDelete();
			break;
			case SDL_SCANCODE_C:
				ExecuteCopy();
			break;
			case SDL_SCANCODE_V:
				ExecutePaste(true);
			break;
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
				ExecuteWalkable();
			break;
			case SDL_SCANCODE_R:
				ExecuteRotate();
			break;
			case SDL_SCANCODE_M:
				if(IsShiftDown) {
					ExecuteIOCommand(EDITINPUT_MONSTERIDENTIFIER);
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
					ExecuteIOCommand(EDITINPUT_PARTICLEIDENTIFIER);
					Framework.IgnoreNextInputEvent = true;
				}
			break;
			case SDL_SCANCODE_Z:
				if(IsCtrlDown)
					ExecuteUndo();
			break;
			case SDL_SCANCODE_N:
				if(IsCtrlDown)
					ExecuteClear();
			break;
			case SDL_SCANCODE_I:
				if(IsShiftDown) {
					ExecuteIOCommand(EDITINPUT_ITEMIDENTIFIER);
					Framework.IgnoreNextInputEvent = true;
				}
			break;
			case SDL_SCANCODE_O:
				ExecuteIOCommand(EDITINPUT_LOADMONSTERSET);
				Framework.IgnoreNextInputEvent = true;
			break;
			case SDL_SCANCODE_L:
				ExecuteIOCommand(EDITINPUT_LOAD);
				Framework.IgnoreNextInputEvent = true;
			break;
			case SDL_SCANCODE_S:
				ExecuteIOCommand(EDITINPUT_SAVE);
				Framework.IgnoreNextInputEvent = true;
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
				ExecuteUpdateBlockLimits(0, !IsShiftDown);
			break;
			case SDL_SCANCODE_UP:
				ExecuteUpdateBlockLimits(1, !IsShiftDown);
			break;
			case SDL_SCANCODE_RIGHT:
				ExecuteUpdateBlockLimits(2, !IsShiftDown);
			break;
			case SDL_SCANCODE_DOWN:
				ExecuteUpdateBlockLimits(3, !IsShiftDown);
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

	if(CurrentPalette == EDITMODE_BLOCKS) {
		ae::_Element *Clicked = BlockElement->GetClickedElement();
		if(Clicked && Clicked->Index != -1) {
			ProcessBlockIcons(Clicked->Index, MouseEvent.Button == SDL_BUTTON_RIGHT);
		}
	}

	if(CurrentPalette == EDITMODE_EVENTS) {
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
						switch(CurrentPalette) {
							case EDITMODE_BLOCKS:
							case EDITMODE_EVENTS:

								DeselectBlock();
								DeselectEvent();

								// Save the indices
								SavedIndex = WorldCursorIndex;
								IsDrawing = true;
								FinishDrawing = false;

							break;
							default: {
								ae::_Element *Button = Brush[CurrentPalette];
								if(Button)
									SpawnObject(Map->GetValidPosition(WorldCursor), (intptr_t)Button->UserData, Button->Name, IsShiftDown);
							} break;
						}
					}
				break;
				case SDL_BUTTON_RIGHT:

					// Move the camera
					Camera->Set2DPosition(WorldCursor);

				break;
				case SDL_BUTTON_MIDDLE:
					if(!IsDrawing) {
						switch(CurrentPalette) {
							case EDITMODE_BLOCKS:

								// Get the block
								SelectedBlockIndex = Map->GetSelectedBlock(CurrentLayer, WorldCursorIndex, &SelectedBlock);
								if(BlockSelected()) {

									// Save old states
									OldStart = SelectedBlock->Start;
									OldEnd = SelectedBlock->End;
									SavedIndex = WorldCursorIndex;

									IsMoving = true;
								}

							break;
							case EDITMODE_EVENTS:

								// Get the event
								SelectedEventIndex = Map->GetSelectedEvent(WorldCursorIndex, &SelectedEvent);
								if(EventSelected()) {

									// Save old states
									OldStart = SelectedEvent->Start;
									OldEnd = SelectedEvent->End;
									SavedIndex = WorldCursorIndex;

									// Remove bad tiles
									std::vector<_EventTile> &Tiles = SelectedEvent->Tiles;
									for(size_t i = 0; i < Tiles.size(); i++) {
										int LayerSize = Map->GetLayerSize(Tiles[i].Layer);
										if(Tiles[i].BlockID != -1 && LayerSize != -1 && Tiles[i].BlockID >= LayerSize) {
											std::cout << "Bad BlockID cleansed in layer" << Tiles[i].Layer << ": " << Tiles[i].BlockID << " vs " << LayerSize << std::endl;
											Tiles[i].BlockID = -1;
										}
									}
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
		ae::_Element *Button = PaletteElement[CurrentPalette]->GetClickedElement();
		if(Button)
			ExecuteSelectPalette(Button, MouseEvent.Button == SDL_BUTTON_RIGHT);
	}

	// Mouse Release
	if(!MouseEvent.Pressed) {
		switch(MouseEvent.Button) {
			case SDL_BUTTON_LEFT:
				if(IsDrawing) {
					FinishDrawing = true;
					UndoNumber[CurrentLayer]++;
				}
			break;
			case SDL_BUTTON_MIDDLE:
				if(IsMoving) {
					IsMoving = false;
					for(auto Iterator : SelectedObjects) {
						Iterator->Position = GetMoveDeltaPosition(Iterator->Position);
					}
					MoveDelta = glm::vec2(0, 0);
				}

				if(DraggingBox) {
					DraggingBox = false;
					SelectObjects();
				}
			break;
		}
	}
}

// Mouse wheel handler
void _EditorState::HandleMouseWheel(int Direction) {

	if(ae::Input.GetMouse().x < ae::Graphics.ViewportSize.x && ae::Input.GetMouse().y < ae::Graphics.ViewportSize.y) {
		float Multiplier = 1.0f * Direction;
		if(IsShiftDown)
			Multiplier = 10.0f * Direction;

		// Zoom
		Camera->UpdateDistance(-Multiplier);
	}
	else {
		if(Direction > 0)
			PaletteElement[CurrentPalette]->UpdateChildrenOffset(glm::ivec2(0, PaletteSizes[CurrentPalette]));
		else
			PaletteElement[CurrentPalette]->UpdateChildrenOffset(glm::ivec2(0, -PaletteSizes[CurrentPalette]));
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

	// Get world cursor
	Camera->ConvertScreenToWorld(ae::Input.GetMouse(), WorldCursor);

	// Get tile indices for later usage
	WorldCursorIndex = Map->GetValidCoord(WorldCursor);

	// Set camera position
	Camera->Update(FrameTime);

	// Drawing a block or event
	if(IsDrawing) {

		// Get start positions
		DrawStart = SavedIndex;

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

	// Moving a block or event
	if(IsMoving) {
		_Coord Offset;

		// Get offsets
		Offset = WorldCursorIndex - SavedIndex;

		// Check x bounds
		if(Offset.x + OldStart.x < 0)
			Offset.x = -OldStart.x;
		else if(Offset.x + OldEnd.x >= Map->GetWidth())
			Offset.x = Map->GetWidth() - OldEnd.x - 1;

		// Check y bounds
		if(Offset.y + OldStart.y < 0)
			Offset.y = -OldStart.y;
		else if(Offset.y + OldEnd.y >= Map->GetHeight())
			Offset.y = Map->GetHeight() - OldEnd.y - 1;

		// Get start positions
		DrawStart = OldStart + Offset;

		// Check bounds
		DrawEnd.x = OldEnd.x + Offset.x + 1;
		DrawEnd.y = OldEnd.y + Offset.y + 1;

	}

	// Update based on editor state
	switch(CurrentPalette) {
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
					Block.Wall = (CurrentLayer == EDITOR_WALL_LAYER);
					Block.Walkable = Walkable;

					Map->AddBlock(CurrentLayer, Block);
				}

				FinishDrawing = IsDrawing = false;
			}

			if(IsMoving) {
				SelectedBlock->Start = DrawStart;
				SelectedBlock->End = DrawEnd-1;
			}
		break;
		case EDITMODE_EVENTS:
			if(FinishDrawing) {
				if(Brush[EDITMODE_EVENTS])
					AddEvent(Brush[EDITMODE_EVENTS]->Index);
				FinishDrawing = IsDrawing = false;
			}

			if(IsMoving) {
				SelectedEvent->Start = DrawStart;
				SelectedEvent->End = DrawEnd - 1;
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
	ae::Assets.Programs["pos_uv"]->AmbientLight = glm::vec4(1);

	// Setup the viewing matrix
	ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
	glUniformMatrix4fv(ae::Assets.Programs["pos"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
	glUniformMatrix4fv(ae::Assets.Programs["pos_uv"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	ae::Graphics.SetProgram(ae::Assets.Programs["text"]);
	glUniformMatrix4fv(ae::Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));

	// Draw floors
	Map->RenderFloors();

	// Draw tentative block
	if(IsDrawing) {
		if(Brush[CurrentPalette]) {
			if(CurrentPalette == EDITMODE_EVENTS) {
				ae::Graphics.SetDepthTest(false);
				ae::Graphics.DrawRepeatable(glm::vec3(DrawStart.x, DrawStart.y, MAP_LAYEROFFSET), glm::vec3(DrawEnd.x, DrawEnd.y, MAP_LAYEROFFSET), Brush[CurrentPalette]->Style->Texture, 0, 1.0f);
				ae::Graphics.SetDepthTest(true);
			}
			else {
				if(CurrentLayer == MAPLAYER_FORE)
					ae::Graphics.DrawRepeatable(glm::vec3(DrawStart.x, DrawStart.y, MaxZ + MAP_LAYEROFFSET * CurrentLayer), glm::vec3(DrawEnd.x, DrawEnd.y, MaxZ + MAP_LAYEROFFSET * CurrentLayer), Brush[CurrentPalette]->Style->Texture, Rotation, ScaleX);
				else if(CurrentLayer == MAPLAYER_FLAT) {
					ae::Graphics.SetVBO(ae::VBO_CUBE);
					ae::Graphics.DrawWall(glm::vec3(DrawStart.x, DrawStart.y, MinZ), glm::vec3(DrawEnd.x - DrawStart.x, DrawEnd.y - DrawStart.y, MaxZ - MinZ), Rotation, Brush[CurrentPalette]->Style->Texture);
				}
				else {
					if(MaxZ == MinZ) {
						ae::Graphics.DrawRepeatable(glm::vec3(DrawStart.x, DrawStart.y, MinZ + MAP_LAYEROFFSET * CurrentLayer), glm::vec3(DrawEnd.x, DrawEnd.y, MinZ + MAP_LAYEROFFSET * CurrentLayer), Brush[CurrentPalette]->Style->Texture, Rotation, ScaleX);
					}
					else {
						ae::Graphics.SetVBO(ae::VBO_CUBE);
						ae::Graphics.DrawCube(glm::vec3(DrawStart.x, DrawStart.y, MinZ), glm::vec3(DrawEnd.x - DrawStart.x, DrawEnd.y - DrawStart.y, MaxZ - MinZ), Brush[CurrentPalette]->Style->Texture);
					}
				}
			}
		}
	}

	// Draw walls clipped with MaxZ=OBJECT_Z
	Map->RenderWalls();

	// Draw objects
	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
	ae::Graphics.SetDepthMask(false);
	ae::Graphics.SetVBO(ae::VBO_QUAD);
	const std::vector<_ObjectSpawn *> &Objects = Map->GetObjectsList();
	for(size_t i = 0; i < Objects.size(); i++) {
		DrawObject(0.0f, 0.0f, Objects[i], 1.0f);
	}

	// Outline selected item
	ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
	ae::Graphics.SetColor(COLOR_WHITE);
	for(auto Iterator : SelectedObjects) {
		glm::vec2 Position = GetMoveDeltaPosition(Iterator->Position);
		ae::Graphics.DrawCircle(glm::vec3(Position, ITEM_Z + 0.05f), EDITOR_OBJECTRADIUS);
	}

	// Draw faded items while moving
	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
	ae::Graphics.SetVBO(ae::VBO_QUAD);
	for(auto Iterator : SelectedObjects) {
		DrawObject(MoveDelta.x, MoveDelta.y, Iterator, 0.5f);
	}
	ae::Graphics.SetDepthMask(true);

	// Draw walls
	Map->RenderWalls();
	Map->RenderFlatWalls();

	// Draw the foreground tiles
	Map->RenderForeground();

	// Draw the events
	Map->RenderEvents(EventTextures);

	ae::Graphics.SetDepthMask(false);
	ae::Graphics.SetDepthTest(false);

	// Draw map boundaries
	ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
	ae::Graphics.SetColor(COLOR_RED);
	ae::Graphics.DrawRectangle3D(glm::vec2(-0.01f, -0.01f), glm::vec2(Map->GetWidth() + 0.01f, Map->GetHeight() + 0.01f), false);

	// Draw grid
	Map->RenderGrid(GridMode);

	// Outline the blocks
	if(HighlightBlocks)
		Map->HighlightBlocks(CurrentLayer);

	// Outline selected block
	if(BlockSelected()) {
		ae::Graphics.SetColor(COLOR_WHITE);
		ae::Graphics.DrawRectangle3D(glm::vec2(SelectedBlock->Start.x, SelectedBlock->Start.y), glm::vec2(SelectedBlock->End.x + 1.0f, SelectedBlock->End.y + 1.0f), false);
	}

	// Outline selected event
	if(EventSelected()) {
		ae::Graphics.SetColor(COLOR_CYAN);
		ae::Graphics.DrawRectangle3D(glm::vec2(SelectedEvent->Start.x + 0.02f, SelectedEvent->Start.y + 0.02f), glm::vec2(SelectedEvent->End.x + 0.98f, SelectedEvent->End.y + 0.98f), false);

		// Outline affected tiles and blocks
		const std::vector<_EventTile> &Tiles = SelectedEvent->Tiles;
		for(size_t i = 0; i < Tiles.size(); i++) {
			ae::Graphics.SetColor(COLOR_RED);
			ae::Graphics.DrawRectangle3D(glm::vec2(Tiles[i].Coord.x + 0.2f, Tiles[i].Coord.y + 0.2f), glm::vec2(Tiles[i].Coord.x + 0.8f, Tiles[i].Coord.y + 0.8f), false);

			if(Tiles[i].BlockID != -1) {
				if(SelectedEvent->Type == EVENT_ENABLE) {
					const _Event *Event = Map->GetEvent(Tiles[i].BlockID);
					ae::Graphics.SetColor(COLOR_YELLOW);
					ae::Graphics.DrawRectangle3D(glm::vec2(Event->Start.x, Event->Start.y), glm::vec2(Event->End.x + 1.0f, Event->End.y + 1.0f), false);
				}
				else {
					const _Block *Block = Map->GetBlock(Tiles[i].Layer, Tiles[i].BlockID);
					ae::Graphics.SetColor(COLOR_GREEN);
					ae::Graphics.DrawRectangle3D(glm::vec2(Block->Start.x, Block->Start.y), glm::vec2(Block->End.x + 1.0f, Block->End.y + 1.0f), false);
				}
			}
		}
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

	// Draw viewport outline
	ae::Graphics.SetColor(COLOR_DARK);
	ae::Graphics.DrawRectangle(glm::vec2(0, 0), ae::Graphics.ViewportSize);

	// Draw text
	if(EditorInput != -1)
		InputBox->Render();

	// Draw filename
	std::ostringstream Buffer;
	Buffer << Map->GetFilename();
	MainFont->DrawText(Buffer.str(), glm::vec2(25, 25));
	Buffer.str("");

	// Draw cursor position
	int X = 16;
	int Y = ae::Graphics.ViewportSize.y - 25;
	Buffer << std::fixed << WorldCursor.x << ", " << WorldCursor.y;
	MainFont->DrawText(Buffer.str(), glm::vec2(X, Y));
	Buffer.str("");

	// Draw FPS
	X = ae::Graphics.ViewportSize.x - 45;
	Y = 25;
	Buffer << ae::Graphics.FramesPerSecond << " FPS";
	MainFont->DrawText(Buffer.str(), glm::vec2(X, Y), ae::RIGHT_BASELINE);
	Buffer.str("");

	// Draw selection count
	Buffer << SelectedObjects.size() << " selected";
	MainFont->DrawText(Buffer.str(), glm::vec2(X, Y + 20), ae::RIGHT_BASELINE);
	Buffer.str("");

	// Draw checkpoint info
	X = ae::Graphics.ViewportSize.x - 45;
	Y = ae::Graphics.ViewportSize.y - 40;
	Buffer << CheckpointIndex;
	MainFont->DrawText("Checkpoint:", glm::vec2(X, Y), ae::RIGHT_BASELINE);
	MainFont->DrawText(Buffer.str(), glm::vec2(X + 5, Y));
	Buffer.str("");

	// Draw grid size
	Y += 20;
	Buffer << GridMode;
	MainFont->DrawText("Grid:", glm::vec2(X, Y), ae::RIGHT_BASELINE);
	MainFont->DrawText(Buffer.str(), glm::vec2(X + 5, Y));
	Buffer.str("");

	// Draw command buttons
	CommandElement->Render();
	if(CurrentPalette == EDITMODE_BLOCKS) {
		BlockElement->Render();
	}
	else if(CurrentPalette == EDITMODE_EVENTS) {
		EventElement->Render();
	}

	// Draw current brush
	DrawBrush();

	// Draw Palette
	PaletteElement[CurrentPalette]->Render();

	ae::Graphics.SetDepthMask(true);
}

// Load palette buttons
void _EditorState::LoadPalettes() {
	std::vector<_Brush> Icons;

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
	Icons.push_back(_Brush("text", "Event Message", ae::Assets.Textures["textures/editor_repeat/event_text.png"], COLOR_WHITE));
	Icons.push_back(_Brush("sound", "Event Sound", ae::Assets.Textures["textures/editor_repeat/event_sound.png"], COLOR_WHITE));
	Icons.push_back(_Brush("fswitch", "Floor Switch", ae::Assets.Textures["textures/editor_repeat/event_fswitch.png"], COLOR_WHITE));
	Icons.push_back(_Brush("enable", "Event Enabler", ae::Assets.Textures["textures/editor_repeat/event_enable.png"], COLOR_WHITE));
	Icons.push_back(_Brush("tele", "Teleporter", ae::Assets.Textures["textures/editor_repeat/event_tele.png"], COLOR_WHITE));
	Icons.push_back(_Brush("light", "Lights", ae::Assets.Textures["textures/editor_repeat/event_light.png"], COLOR_WHITE));
	LoadPaletteButtons(Icons, EDITMODE_EVENTS);
	for(size_t i = 0; i < Icons.size(); i++)
		EventTextures.push_back(Icons[i].Texture);
	Icons.clear();

	// Load monsters
	LoadMonsterButtons();

	// Load items
	for(const auto &Item : Stats.Items) {
		if(Item.second.Type == _Object::MEDKIT || Item.second.Type == _Object::KEY)
			Icons.push_back(_Brush(Item.first, Item.second.Name, ae::Assets.Textures[Item.second.IconID], Item.second.Color, Item.second.Type));
	}
	LoadPaletteButtons(Icons, EDITMODE_ITEMS);
	Icons.clear();

	// Load ammo
	for(const auto &Item : Stats.Items) {
		if(Item.second.Type == _Object::AMMO)
			Icons.push_back(_Brush(Item.first, Item.second.Name, ae::Assets.Textures[Item.second.IconID], Item.second.Color, Item.second.Type));
	}
	LoadPaletteButtons(Icons, EDITMODE_AMMO);
	Icons.clear();

	// Load upgrades
	for(const auto &Item : Stats.Items) {
		if(Item.second.Type == _Object::UPGRADE)
			Icons.push_back(_Brush(Item.first, Item.second.Name, ae::Assets.Textures[Item.second.IconID], Item.second.Color, Item.second.Type));
	}
	LoadPaletteButtons(Icons, EDITMODE_UPGRADES);
	Icons.clear();

	// Load weapons
	for(const auto &Weapon : Stats.Weapons) {
		const ae::_Texture *Texture = ae::Assets.Textures[Weapon.second.IconIdentifier];
		if(Texture)
			Icons.push_back(_Brush(Weapon.first, Weapon.second.Name, Texture, Weapon.second.Color, _Object::WEAPON));
	}
	LoadPaletteButtons(Icons, EDITMODE_WEAPONS);
	Icons.clear();

	// Load armor
	for(const auto &Item : Stats.Items) {
		if(Item.second.Type == _Object::ARMOR)
			Icons.push_back(_Brush(Item.first, Item.second.Name, ae::Assets.Textures[Item.second.IconID], Item.second.Color, Item.second.Type));
	}
	LoadPaletteButtons(Icons, EDITMODE_ARMOR);
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

// Loads the palette buttons from the map's monster set
void _EditorState::LoadMonsterButtons() {
	if(!Map)
		return;

	std::vector<_Brush> Icons;
	for(size_t i = 0; i < Map->MonsterSet.size(); i++) {
		if(Stats.Monsters.find(Map->MonsterSet[i]) == Stats.Monsters.end()) {
			throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " - Cannot find monster: " + Map->MonsterSet[i]);
		}
		else {
			_MonsterTemplate &MonsterTemplate = Stats.Monsters.at(Map->MonsterSet[i]);
			Icons.push_back(_Brush(Map->MonsterSet[i], MonsterTemplate.Name, OldAssets.Animations[MonsterTemplate.AnimationIdentifier]->GetStartPositionFrame(), MonsterTemplate.Color, _Object::MONSTER));
		}
	}

	LoadPaletteButtons(Icons, EDITMODE_MONSTERS);
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
		Button->Name = Icons[i].Identifier;
		Button->Parent = PaletteElement[Type];
		Button->BaseOffset = Offset;
		Button->BaseSize = glm::ivec2(PaletteSizes[Type], PaletteSizes[Type]);
		Button->Alignment = ae::LEFT_TOP;
		Button->Style = Style;
		Button->HoverStyle = ae::Assets.Styles["style_editor_button_selected"];
		Button->UserData = (void *)(intptr_t)Icons[i].ObjectType;
		Button->Index = i;

		PaletteElement[Type]->Children.push_back(Button);

		Offset.x += PaletteSizes[Type];
		if(Offset.x > Width - PaletteSizes[Type]) {
			Offset.y += PaletteSizes[Type];
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
	std::string IconText = "", IconIdentifier = "";
	glm::vec4 IconColor = COLOR_WHITE;
	const ae::_Texture *IconTexture = nullptr;
	if(Brush[CurrentPalette]) {
		IconIdentifier = Brush[CurrentPalette]->Name;
		IconText = Brush[CurrentPalette]->Style->Name;
		IconTexture = Brush[CurrentPalette]->Style->Texture;
		IconColor = Brush[CurrentPalette]->Style->TextureColor;
	}

	// Edit mode specific text
	float IconRotation = 0;
	float IconScaleX = 1.0f;
	switch(CurrentPalette) {
		case EDITMODE_BLOCKS: {

			// See if there's a selected block
			std::string BlockAltTextureIdentifier;
			float BlockMinZ, BlockMaxZ;
			bool BlockWalkable;
			if(BlockSelected()) {
				IconTexture = SelectedBlock->Texture;
				if(IconTexture)
					IconText = IconTexture->Name;
				IconRotation = SelectedBlock->Rotation;
				IconScaleX = SelectedBlock->ScaleX;
				BlockMinZ = SelectedBlock->MinZ;
				BlockMaxZ = SelectedBlock->MaxZ;
				BlockWalkable = SelectedBlock->Walkable;
				if(SelectedBlock->AltTexture)
					BlockAltTextureIdentifier = SelectedBlock->AltTexture->Name;
			}
			else {
				if(Brush[CurrentPalette])
					IconText = Brush[CurrentPalette]->Name;
				IconRotation = Rotation;
				IconScaleX = ScaleX;
				BlockMinZ = MinZ;
				BlockMaxZ = MaxZ;
				BlockWalkable = Walkable;
				BlockAltTextureIdentifier = AltTextureIdentifier;
			}
			IconText.erase(0, std::string(MAP_TEXTURE_PATH).length());
			BlockAltTextureIdentifier.erase(0, std::string(MAP_TEXTURE_PATH).length());

			IconIdentifier = "";

			int X = (float)ae::Graphics.ViewportSize.x + 100;
			int Y = (float)ae::Graphics.ViewportSize.y + 5;
			std::ostringstream Buffer;
			Buffer << IconRotation;
			MainFont->DrawText("Rotation:", glm::vec2(X, Y), ae::RIGHT_BASELINE);
			MainFont->DrawText(Buffer.str(), glm::vec2(X + 5, Y));
			Buffer.str("");

			Buffer << BlockMinZ;
			MainFont->DrawText("Min Z:", glm::vec2(X + 85, Y), ae::RIGHT_BASELINE);
			MainFont->DrawText(Buffer.str(), glm::vec2(X + 90, Y));
			Buffer.str("");

			Y += 15;
			Buffer << IconScaleX;
			MainFont->DrawText("ScaleX:", glm::vec2(X, Y), ae::RIGHT_BASELINE);
			MainFont->DrawText(Buffer.str(), glm::vec2(X + 5, Y));
			Buffer.str("");

			Buffer << BlockMaxZ;
			MainFont->DrawText("Max Z:", glm::vec2(X + 85, Y), ae::RIGHT_BASELINE);
			MainFont->DrawText(Buffer.str(), glm::vec2(X + 90, Y));
			Buffer.str("");

			Y += 15;
			Buffer << BlockWalkable;
			MainFont->DrawText("Walk:", glm::vec2(X, Y), ae::RIGHT_BASELINE);
			MainFont->DrawText(Buffer.str(), glm::vec2(X + 5, Y));
			Buffer.str("");

			MainFont->DrawText(BlockAltTextureIdentifier, glm::vec2(ae::Graphics.ViewportSize.x + 112.0f, ae::Graphics.ViewportSize.y + 145.0f), ae::CENTER_MIDDLE);
		} break;
		case EDITMODE_EVENTS: {

			// Get object identifiers
			std::string ItemIdentifier, MonsterIdentifier, ParticleIdentifier;
			double ActivationPeriod;
			int Active, Level;
			if(EventSelected()) {
				ae::_Element *Button = PaletteElement[EDITMODE_EVENTS]->Children[SelectedEvent->Type];
				IconTexture = Button->Style->Texture;
				IconIdentifier = Button->Name;
				IconText = Button->Style->Name;

				ItemIdentifier = SelectedEvent->ItemIdentifier;
				MonsterIdentifier = SelectedEvent->MonsterIdentifier;
				ParticleIdentifier = SelectedEvent->ParticleIdentifier;
				Active = SelectedEvent->Active;
				Level = SelectedEvent->Level;
				ActivationPeriod = SelectedEvent->ActivationPeriod;
			}
			else {
				ItemIdentifier = SavedText[EDITINPUT_ITEMIDENTIFIER];
				MonsterIdentifier = SavedText[EDITINPUT_MONSTERIDENTIFIER];
				ParticleIdentifier = SavedText[EDITINPUT_PARTICLEIDENTIFIER];
				Active = EventActive;
				Level = EventLevel;
				ActivationPeriod = EventActivationPeriod;
			}

			int X = ae::Graphics.ViewportSize.x + 75;
			int Y = ae::Graphics.ViewportSize.y - 15;

			std::ostringstream Buffer;
			Buffer << Active;
			MainFont->DrawText("Active:", glm::vec2(X, Y), ae::RIGHT_BASELINE);
			MainFont->DrawText(Buffer.str(), glm::vec2(X + 5, Y));
			Buffer.str("");

			Y += 15;
			MainFont->DrawText("Item:", glm::vec2(X, Y), ae::RIGHT_BASELINE);
			MainFont->DrawText(ItemIdentifier, glm::vec2(X + 5, Y));

			Y += 15;
			MainFont->DrawText("Monster:", glm::vec2(X, Y), ae::RIGHT_BASELINE);
			MainFont->DrawText(MonsterIdentifier, glm::vec2(X + 5, Y));

			Y += 15;
			MainFont->DrawText("Particle:", glm::vec2(X, Y), ae::RIGHT_BASELINE);
			MainFont->DrawText(ParticleIdentifier, glm::vec2(X + 5, Y));

			Y += 15;
			Buffer << Level << ":" << ActivationPeriod;
			MainFont->DrawText("Level:", glm::vec2(X, Y), ae::RIGHT_BASELINE);
			MainFont->DrawText(Buffer.str(), glm::vec2(X + 5, Y));
			Buffer.str("");
		} break;
		default:

			// See if there's a selected object
			if(SelectedObjects.size() > 0) {
				auto Iterator = SelectedObjects.begin();
				IconIdentifier = (*Iterator)->Identifier;
				IconText = "";
				IconTexture = nullptr;
			}
		break;
	}

	// Bottom information box
	if(IconText != "")
		MainFont->DrawText(IconText, glm::vec2(ae::Graphics.ViewportSize.x + 112, ae::Graphics.ViewportSize.y + 130), ae::CENTER_MIDDLE);

	if(IconIdentifier != "")
		MainFont->DrawText(IconIdentifier, glm::vec2(ae::Graphics.ViewportSize.x + 112, ae::Graphics.ViewportSize.y + 145), ae::CENTER_MIDDLE);

	if(IconTexture) {
		ae::Assets.Programs["ortho_pos_uv"]->ResetTextureTransform();
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
		ae::Graphics.SetColor(IconColor);
		ae::Graphics.DrawSprite(glm::vec3((float)ae::Graphics.CurrentSize.x - 112, (float)ae::Graphics.CurrentSize.y - 84, 0.0f), IconTexture, IconRotation * IconScaleX, glm::vec2(IconScaleX * EDITOR_PALETTE_SELECTEDSIZE * 2, EDITOR_PALETTE_SELECTEDSIZE * 2));
	}
}

// Draws an object
void _EditorState::DrawObject(float OffsetX, float OffsetY, const _ObjectSpawn *Object, float Alpha) {
	float Scale = ITEM_SCALE;
	float Depth = ITEM_Z;
	glm::vec4 Color;
	const ae::_Texture *Texture = nullptr;
	switch(Object->Type) {
		case _Object::MONSTER: {
			_MonsterTemplate &Monster = Stats.Monsters.at(Object->Identifier);
			Texture = OldAssets.GetAnimation(Monster.AnimationIdentifier)->GetStartPositionFrame();
			Color = Monster.Color;
			Scale = Monster.Scale;
			Depth = OBJECT_Z;
		} break;
		case _Object::KEY:
		case _Object::AMMO:
		case _Object::UPGRADE:
		case _Object::ARMOR:
		case _Object::MEDKIT: {
			_ItemTemplate &Ammo = Stats.Items[Object->Identifier];
			Texture = ae::Assets.Textures[Ammo.IconID];
			Color = Ammo.Color;
		} break;
		case _Object::WEAPON: {
			_WeaponTemplate &Weapon = Stats.Weapons[Object->Identifier];
			Texture = ae::Assets.Textures[Weapon.IconIdentifier];
			Color = Weapon.Color;
		} break;
	}

	glm::vec2 DrawPosition(Object->Position.x + OffsetX, Object->Position.y + OffsetY);
	if(!Camera->IsCircleInView(DrawPosition, Scale)) {
		return;
	}

	Color.a *= Alpha;
	if(Texture != nullptr) {
		ae::Assets.Programs["pos_uv"]->ResetTextureTransform();
		ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
		ae::Graphics.SetColor(Color);
		ae::Graphics.DrawSprite(glm::vec3(DrawPosition, Depth), Texture, 0.0f, glm::vec2(Scale));
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
		case ICON_AMMO:
			ExecuteSwitchMode(EDITMODE_AMMO);
		break;
		case ICON_UPGRADE:
			ExecuteSwitchMode(EDITMODE_UPGRADES);
		break;
		case ICON_MONSTER:
			ExecuteSwitchMode(EDITMODE_MONSTERS);
		break;
		case ICON_WEAPON:
			ExecuteSwitchMode(EDITMODE_WEAPONS);
		break;
		case ICON_ARMOR:
			ExecuteSwitchMode(EDITMODE_ARMOR);
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
		case ICON_PASTE:
			ExecutePaste(false);
		break;
		case ICON_SHOW:
			ExecuteHighlightBlocks();
		break;
		case ICON_UNDO:
			ExecuteUndo();
		break;
		case ICON_CLEAR:
			ExecuteClear();
		break;
		case ICON_GRID:
			if(IsShiftDown)
				ExecuteUpdateGridMode(-1);
			else
				ExecuteUpdateGridMode(1);
		break;
		case ICON_MSET:
			ExecuteIOCommand(EDITINPUT_LOADMONSTERSET);
		break;
		case ICON_LOAD:
			ExecuteIOCommand(EDITINPUT_LOAD);
		break;
		case ICON_SAVE:
			ExecuteIOCommand(EDITINPUT_SAVE);
		break;
		case ICON_TEST:
			ExecuteTest();
		break;
	}
}

// Processes clicks on the block buttons
void _EditorState::ProcessBlockIcons(int Index, int Type) {

	switch(Index) {
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
		case ICON_ITEMIDENTIFIER:
			ExecuteIOCommand(EDITINPUT_ITEMIDENTIFIER);
		break;
		case ICON_MONSTERIDENTIFIER:
			ExecuteIOCommand(EDITINPUT_MONSTERIDENTIFIER);
		break;
		case ICON_PARTICLEIDENTIFIER:
			ExecuteIOCommand(EDITINPUT_PARTICLEIDENTIFIER);
		break;
	}
}

// Adds an object to the list
void _EditorState::SpawnObject(const glm::vec2 &Position, int Type, const std::string &Identifier, bool Align) {
	glm::vec2 SpawnPosition;

	if(Align)
		SpawnPosition = AlignToGrid(Position);
	else
		SpawnPosition = Position;

	_ObjectSpawn *Object = new _ObjectSpawn(Identifier, SpawnPosition, Type);
	Map->AddObject(Object);
}

// Adds an event to the list
void _EditorState::AddEvent(int Type) {

	// Get object identifiers
	std::string ItemIdentifier = SavedText[EDITINPUT_ITEMIDENTIFIER];
	std::string MonsterIdentifier = SavedText[EDITINPUT_MONSTERIDENTIFIER];
	std::string ParticleIdentifier = SavedText[EDITINPUT_PARTICLEIDENTIFIER];

	// Get selected object's identifier
	if(ObjectsSelected()) {
		_ObjectSpawn *SelectedObject = *SelectedObjects.begin();
		switch(SelectedObject->Type) {
			case _Object::MONSTER:
				MonsterIdentifier = SelectedObject->Identifier;
			break;
			default:
				ItemIdentifier = SelectedObject->Identifier;
			break;
		}
	}

	int TileLayer = -1;
	_Coord Start = DrawStart;
	_Coord End = DrawEnd - 1;
	std::string EventItemIdentifier = "";
	std::string EventMonsterIdentifier = "";
	std::string EventParticleIdentifier = "";

	// Setup the event
	switch(Type) {
		case EVENT_DOOR:
			EventItemIdentifier = ItemIdentifier;
			TileLayer = MAPLAYER_FLAT;
		break;
		case EVENT_WSWITCH:
			EventItemIdentifier = ItemIdentifier;
			TileLayer = CurrentLayer;
		break;
		case EVENT_SPAWN:
			EventMonsterIdentifier = MonsterIdentifier;
			EventParticleIdentifier = ParticleIdentifier;
			TileLayer = CurrentLayer;
		break;
		default:
		break;
	}

	if(Type != -1) {
		_Event *Event = new _Event(Type, EventActive, Start, End, EventLevel, EventActivationPeriod, EventItemIdentifier, EventMonsterIdentifier, EventParticleIdentifier);
		if(TileLayer != -1) {
			int BlockIndex = Map->GetSelectedBlock(TileLayer, Start);
			Event->AddTile(_EventTile(Start, TileLayer, BlockIndex));
		}

		Map->AddEvent(Event);
	}
}

// Updates the selected event's object identifier
void _EditorState::UpdateEventIdentifier(int Type, const std::string &Identifier) {

	if(EventSelected()) {
		switch(Type) {
			case EDITINPUT_ITEMIDENTIFIER:
				SelectedEvent->ItemIdentifier = Identifier;
			break;
			case EDITINPUT_MONSTERIDENTIFIER:
				SelectedEvent->MonsterIdentifier = Identifier;
			break;
			case EDITINPUT_PARTICLEIDENTIFIER:
				SelectedEvent->ParticleIdentifier = Identifier;
			break;
		}
	}
}

// Gets the selected event's object identifier
std::string _EditorState::GetEventIdentifier(int Type) {

	if(EventSelected()) {
		switch(Type) {
			case EDITINPUT_ITEMIDENTIFIER:
				return SelectedEvent->ItemIdentifier;
			break;
			case EDITINPUT_MONSTERIDENTIFIER:
				return SelectedEvent->MonsterIdentifier;
			break;
			case EDITINPUT_PARTICLEIDENTIFIER:
				return SelectedEvent->ParticleIdentifier;
			break;
		}
	}

	return "";
}

// Returns a valid position for the object
glm::vec2 _EditorState::GetValidObjectPosition(const glm::vec2 &Position) const {
	glm::vec2 NewPosition;

	if(Position.x < 0)
		NewPosition.x = 0;
	else if(Position.x >= Map->GetWidth())
		NewPosition.x = (float)Map->GetWidth();
	else
		NewPosition.x = Position.x;

	if(Position.y < 0)
		NewPosition.y = 0;
	else if(Position.y >= Map->GetHeight())
		NewPosition.y = (float)Map->GetHeight();
	else
		NewPosition.y = Position.y;

	return NewPosition;
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
	if(BlockSelected())
		SelectedBlock->Walkable = !SelectedBlock->Walkable;
	else
		Walkable = !Walkable;
}

// Executes the rotate command
void _EditorState::ExecuteRotate() {
	if(BlockSelected()) {
		SelectedBlock->Rotation += 90;
		if(SelectedBlock->Rotation > 359)
			SelectedBlock->Rotation = 0;
	}
	else {
		Rotation += 90;
		if(Rotation > 359)
			Rotation = 0;
	}
}

// Executes the mirror texture command
void _EditorState::ExecuteMirror() {
	if(BlockSelected())
		SelectedBlock->ScaleX = -SelectedBlock->ScaleX;
	else
		ScaleX = -ScaleX;
}

// Executes the mirror texture command
void _EditorState::ExecuteToggleTile() {

	if(EventSelected()) {
		auto Iterator = SelectedEvent->FindTile(WorldCursorIndex.x, WorldCursorIndex.y);
		if(Iterator != SelectedEvent->Tiles.end()) {
			SelectedEvent->RemoveTile(Iterator);
		}
		else {
			switch(SelectedEvent->Type) {
				case EVENT_DOOR:
				case EVENT_WSWITCH:
				case EVENT_FSWITCH: {
					_Block *Block;
					int BlockIndex = Map->GetSelectedBlock(CurrentLayer, WorldCursorIndex, &Block);
					SelectedEvent->AddTile(_EventTile(WorldCursorIndex, CurrentLayer, BlockIndex));
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
	}
}

// Executes the undo command
void _EditorState::ExecuteChangeZ(float Change, int Type) {
	if(Type == 0) {
		if(BlockSelected())
			SelectedBlock->MinZ += Change;
		else
			MinZ += Change;
	}
	else {
		if(BlockSelected())
			SelectedBlock->MaxZ += Change;
		else
			MaxZ += Change;
	}
}

// Executes the change level command
void _EditorState::ExecuteChangeLevel(int Change) {
	if(EventSelected()) {
		SelectedEvent->Level = SelectedEvent->Level + Change;
		if(SelectedEvent->Level < 0)
			SelectedEvent->Level = 0;

	}
	else {
		EventLevel += Change;
		if(EventLevel < 0)
			EventLevel = 0;
	}
}

// Executes the change activation period command
void _EditorState::ExecuteChangePeriod(double Value) {
	if(EventSelected()) {
		SelectedEvent->ActivationPeriod = SelectedEvent->ActivationPeriod + Value;
		if(SelectedEvent->ActivationPeriod < 0.0)
			SelectedEvent->ActivationPeriod = 0.0;

	}
	else {
		EventActivationPeriod += Value;
		if(EventActivationPeriod < 0.0)
			EventActivationPeriod = 0.0;
	}
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
	CheckpointIndex += Value;

	if(CheckpointIndex < 0)
		CheckpointIndex = 0;
}

// Executes the an I/O command
void _EditorState::ExecuteIOCommand(int Type) {
	EditorInput = Type;
	InputBox->SetActive(true);
	ae::_Element *TextBox = InputBox->Children.front();
	ae::_Element *Label = TextBox->Children.front();
	Label->Text = InputBoxStrings[Type];
	if(Type >= EDITINPUT_ITEMIDENTIFIER && Type <= EDITINPUT_PARTICLEIDENTIFIER && EventSelected())
		InputBox->Text = GetEventIdentifier(Type);
	else
		InputBox->Text = SavedText[Type];

	ae::FocusedElement = TextBox;
}

// Executes the clear map command
void _EditorState::ExecuteClear() {
	LoadMap("", false);
	SavedText[EDITINPUT_SAVE] = "";
}

// Executes the test command
void _EditorState::ExecuteTest() {

	// TODO catch exception
	Map->SaveLevel(EDITOR_TESTLEVEL);

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

	switch(CurrentPalette) {
		case EDITMODE_BLOCKS:
			if(BlockSelected()) {
				Map->RemoveBlock(CurrentLayer, SelectedBlockIndex);
				DeselectBlock();
			}
		break;
		case EDITMODE_EVENTS:
			if(EventSelected()) {
				Map->RemoveEvent(SelectedEventIndex);
				DeselectEvent();
				ClipboardEvent = nullptr;
			}
		break;
		default:
			if(ObjectsSelected()) {
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

	switch(CurrentPalette) {
		case EDITMODE_BLOCKS:
			if(BlockSelected()) {
				ClipboardBlock = *SelectedBlock;
				DeselectBlock();
				BlockCopied = true;
			}
		break;
		case EDITMODE_EVENTS:
			if(EventSelected()) {
				ClipboardEvent = SelectedEvent;
			}
		break;
		default:
			if(ObjectsSelected()) {
				CopiedPosition = WorldCursor;
				ClipboardObjects = SelectedObjects;
			}
		break;
	}
}

// Executes the paste command
void _EditorState::ExecutePaste(bool Viewport) {
	glm::vec2 StartPosition;

	if(Viewport)
		StartPosition = WorldCursor;
	else
		StartPosition = glm::vec2(Camera->GetPosition().x, Camera->GetPosition().y);

	switch(CurrentPalette) {
		case EDITMODE_BLOCKS:
			if(BlockCopied) {
				int Width = ClipboardBlock.End.x - ClipboardBlock.Start.x;
				int Height = ClipboardBlock.End.y - ClipboardBlock.Start.y;
				ClipboardBlock.Start = Map->GetValidCoord(_Coord(StartPosition));
				ClipboardBlock.End = Map->GetValidCoord(_Coord(StartPosition.x + Width, StartPosition.y + Height));

				UndoNumber[CurrentLayer]++;
				Map->AddBlock(CurrentLayer, ClipboardBlock);
			}
		break;
		case EDITMODE_EVENTS:
			if(ClipboardEvent != nullptr) {
				DrawStart = Map->GetValidCoord(StartPosition);
				DrawEnd = Map->GetValidCoord(ClipboardEvent->End - ClipboardEvent->Start + StartPosition);

				_Event *Event = new _Event(ClipboardEvent->Type, ClipboardEvent->Active, DrawStart, DrawEnd,
										ClipboardEvent->Level, ClipboardEvent->ActivationPeriod, ClipboardEvent->ItemIdentifier,
										ClipboardEvent->MonsterIdentifier, ClipboardEvent->ParticleIdentifier);

				Map->AddEvent(Event);
			}
		break;
		default:
			for(auto Iterator : ClipboardObjects) {
				SpawnObject(GetValidObjectPosition(StartPosition - CopiedPosition + Iterator->Position), Iterator->Type, Iterator->Identifier, IsShiftDown);
			}
		break;
	}
}

// Executes the deselect command
void _EditorState::ExecuteDeselect() {
	DeselectBlock();
	DeselectEvent();
	DeselectObjects();
}

// Executes the undo command
void _EditorState::ExecuteUndo() {
	switch(CurrentPalette) {
		case EDITMODE_BLOCKS:
			if(UndoNumber[CurrentLayer] > 0) {
				Map->RemoveLastBlock(CurrentLayer);
				DeselectBlock();
				UndoNumber[CurrentLayer]--;
			}
		break;
	}
}

// Executes the update selected palette command
void _EditorState::ExecuteUpdateSelectedPalette(int Change) {
	std::vector<ae::_Element *> &Children = PaletteElement[CurrentPalette]->Children;
	if(!Brush[CurrentPalette]) {
		Brush[CurrentPalette] = Children[0];
		return;
	}

	int CurrentIndex = Brush[CurrentPalette]->Index;
	CurrentIndex += Change;
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

		// Deselect alternate texture
		if(ClickType == 1 && CurrentPalette == EDITMODE_BLOCKS) {
			if(BlockSelected()) {
				SelectedBlock->AltTexture = nullptr;
			}
			else {
				AltTextureIdentifier = "";
				AltTexture = nullptr;
			}
		}

		return;
	}

	switch(CurrentPalette) {
		case EDITMODE_BLOCKS:
			if(!Button)
				return;

			if(ClickType == 1) {
				if(BlockSelected()) {
					SelectedBlock->AltTexture = Button->Style->Texture;
				}
				else {
					AltTextureIdentifier = Button->Name;
					AltTexture = Button->Style->Texture;
				}
			}
			else {
				if(BlockSelected()) {
					SelectedBlock->Texture = Button->Style->Texture;
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
					case EVENT_SOUND:
					case EVENT_FSWITCH:
					case EVENT_ENABLE:
						SetEventProperties(0, 1, 1, "");
					break;
					case EVENT_TELE:
						SetEventProperties(0, 0, 1, "smoke0");
					break;
					case EVENT_LIGHT:
						SetEventProperties(1, 100, 1, "smoke0");
					break;
					default:
						SetEventProperties(0, 0, 1, "");
					break;
				}
			}
		break;
		default:
			if(ClickType == 1 && EventSelected()) {
				switch(CurrentPalette) {
					case EDITMODE_MONSTERS:
						SelectedEvent->MonsterIdentifier = Button->Name;
						ExecuteSwitchMode(EDITMODE_EVENTS);
					break;
					case EDITMODE_ITEMS:
						SelectedEvent->ItemIdentifier = Button->Name;
						ExecuteSwitchMode(EDITMODE_EVENTS);
					break;
					default:
					break;
				}
			}
		break;
	}

	if(ClickType == 0)
		Brush[CurrentPalette] = Button;
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

	ae::Assets.Elements["editor_show"]->Checked = HighlightBlocks;
}

// Executes the toggle editor mode
void _EditorState::ExecuteSwitchMode(int State) {

	// Toggle icons
	if(CurrentPalette != State) {
		ModeButtons[CurrentPalette]->Checked = false;
		ModeButtons[State]->Checked = true;

		// Set state
		CurrentPalette = State;
		BlockElement->SetActive(false);
		EventElement->SetActive(false);
		switch(CurrentPalette) {
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

	if(CurrentLayer != Layer) {
		if(Move && BlockSelected()) {
			Map->ChangeLayer(CurrentLayer, Layer, SelectedBlockIndex);
			SelectedBlockIndex = Map->GetLastBlock(Layer, &SelectedBlock);

			// Change block properties
			if(Layer == MAPLAYER_WALL) {
				SelectedBlock->Wall = true;
				SelectedBlock->Walkable = false;
			}
			else {
				SelectedBlock->Wall = false;
				SelectedBlock->Walkable = true;
			}
		}
		else
			DeselectBlock();

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
		LayerButtons[CurrentLayer]->Checked = false;
		LayerButtons[Layer]->Checked = true;
		CurrentLayer = Layer;
	}
}

// Executes the shift layer command
void _EditorState::ExecuteShiftLayer(int Change) {

	// Shift layers
	int NewLayer = CurrentLayer + Change;
	if(NewLayer > MAPLAYER_COUNT - 1)
		NewLayer = MAPLAYER_COUNT - 1;
	else if(NewLayer < 0)
		NewLayer = 0;

	ExecuteUpdateLayer(NewLayer, true);
}

// Executes the update block limit command
void _EditorState::ExecuteUpdateBlockLimits(int Direction, bool Expand) {
	_Coord Start, End;
	bool Change = false;
	if(CurrentPalette == EDITMODE_BLOCKS && BlockSelected()) {
		Start = SelectedBlock->Start;
		End = SelectedBlock->End;
		Change = true;
	}
	else if(CurrentPalette == EDITMODE_EVENTS && EventSelected()) {
		Start = SelectedEvent->Start;
		End = SelectedEvent->End;
		Change = true;
	}

	if(Change) {
		switch(Direction) {
			case 0:
				if(Expand)
					Start.x--;
				else
					End.x--;
			break;
			case 1:
				if(Expand)
					Start.y--;
				else
					End.y--;
			break;
			case 2:
				if(Expand)
					End.x++;
				else
					Start.x++;
			break;
			case 3:
				if(Expand)
					End.y++;
				else
					Start.y++;
			break;
		}

		// Check limits
		if(Start.x > End.x)
			Start.x = End.x;

		if(Start.y > End.y)
			Start.y = End.y;

		if(End.x < Start.x)
			End.x = Start.x;

		if(End.y < Start.y)
			End.y = Start.y;

		if(CurrentPalette == EDITMODE_BLOCKS && BlockSelected()) {
			SelectedBlock->Start = Map->GetValidCoord(Start);
			SelectedBlock->End = Map->GetValidCoord(End);
		}
		else if(CurrentPalette == EDITMODE_EVENTS && EventSelected()) {
			SelectedEvent->Start = Map->GetValidCoord(Start);
			SelectedEvent->End = Map->GetValidCoord(End);
		}
	}
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
			if(EventSelected()) {
				switch(SelectedObject->Type) {
					case _Object::MONSTER:
						UpdateEventIdentifier(EDITINPUT_MONSTERIDENTIFIER, SelectedObject->Identifier);
					break;
					default:
						UpdateEventIdentifier(EDITINPUT_ITEMIDENTIFIER, SelectedObject->Identifier);
					break;
				}
			}
		}
	}
	else {
		DeselectObjects();
		DraggingBox = true;
	}
}

// Selects objects
void _EditorState::SelectObjects() {
	DeselectObjects();
	Map->GetSelectedObjects(ClickedPosition, WorldCursor, &SelectedObjects);
}

// Aligns an object to the grid
glm::vec2 _EditorState::AlignToGrid(const glm::vec2 &Position) const {
	return glm::vec2((int)Position.x + 0.5f, (int)Position.y + 0.5f);
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

// Sets event properties
void _EditorState::SetEventProperties(double ActivationPeriod, int Level, int Active, const std::string &ParticleIdentifier) {
	EventActivationPeriod = ActivationPeriod;
	EventLevel = Level;
	EventActive = Active;
	SavedText[EDITINPUT_PARTICLEIDENTIFIER] = ParticleIdentifier;
}

// Clears all the objects in the clipboard
void _EditorState::ClearClipboard() {
	BlockCopied = false;
	ClipboardEvent = nullptr;
	ClipboardObjects.clear();
}

// Clear object selection
void _EditorState::DeselectObjects() {
	SelectedObjects.clear();
}

// Determine if any objects are selected
bool _EditorState::ObjectsSelected() {
	return SelectedObjects.size() != 0;
}
