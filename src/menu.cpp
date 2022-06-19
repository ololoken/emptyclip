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
#include <menu.h>
#include <ae/input.h>
#include <ae/actions.h>
#include <ae/texture.h>
#include <ae/assets.h>
#include <actiontype.h>
#include <constants.h>
#include <ae/graphics.h>
#include <assets.h>
#include <ae/ui.h>
#include <objects/player.h>
#include <config.h>
#include <animation.h>
#include <framework.h>
#include <version.h>
#include <states/play.h>
#include <states/null.h>
#include <sstream>
#include <SDL_mouse.h>

_Menu Menu;

const std::string InputBoxPrefix = "button_options_input_";
const std::string PlayerButtonPrefix = "button_singleplayer_slot";
const std::string PlayerColorButtonPrefix = "button_new_color";

const int KeyBindings[] = {
	Action::GAME_UP,
	Action::GAME_DOWN,
	Action::GAME_LEFT,
	Action::GAME_RIGHT,
	Action::GAME_USE,
	Action::GAME_SPRINT,
	Action::GAME_FIRE,
	Action::GAME_AIM,
	Action::GAME_MELEE,
	Action::GAME_RELOAD,
	Action::GAME_WEAPONSWITCH,
	Action::GAME_HEAL,
	Action::GAME_INVENTORY,
};

const std::string KEYLABEL_IDENTIFIERS[] = {
	"label_options_config_up",
	"label_options_config_down",
	"label_options_config_left",
	"label_options_config_right",
	"label_options_config_use",
	"label_options_config_sprint",
	"label_options_config_fire",
	"label_options_config_aim",
	"label_options_config_melee",
	"label_options_config_reload",
	"label_options_config_weaponswitch",
	"label_options_config_medkit",
	"label_options_config_inventory",
};

const char *COLORS[] = {
	"black",
	"red",
	"green",
	"blue",
};

// Constructor
_Menu::_Menu() {
	State = STATE_NONE;
	CurrentLayout = nullptr;
	Background = nullptr;
	OptionsState = OPTION_NONE;
	SinglePlayerState = SINGLEPLAYER_NONE;
	PreviousClickTimer = 0.0;
}

// Initialize
void _Menu::InitTitle() {
	ae::Graphics.Element->SetActive(false);
	ae::Graphics.Element->Active = true;

	ChangeLayout("element_menu_title");

	std::string BuildVersion;
	if(std::string(BUILD_VERSION) != "")
		BuildVersion = std::string("-") + BUILD_VERSION;
	ae::Graphics.SetCursor(true);

	ae::Assets.Elements["label_game_version"]->Text = GAME_VERSION + BuildVersion;
	ae::Assets.Elements["label_game_version"]->SetActive(true);

	Background = ae::Assets.Elements["image_menu_bg"];
	Background->SetWidth(ae::Graphics.CurrentSize.x * ((float)Background->Texture->Size.y / Background->Texture->Size.x));
	Background->SetHeight(ae::Graphics.CurrentSize.y);
	Background->SetActive(true);

	State = STATE_TITLE;
}

// Init tutorial
void _Menu::InitTutorial() {
	Save.GetPlayer(_Save::SLOT_TUTORIAL)->Reset();
	PlayState.Player = Save.GetPlayer(_Save::SLOT_TUTORIAL);
	PlayState.Level = "tutorial0.map";
	PlayState.TestMode = false;
	PlayState.FromEditor = false;

	Framework.ChangeState(&PlayState);
	State = STATE_NONE;
}

// Init single player
void _Menu::InitSinglePlayer() {
	ChangeLayout("element_menu_singleplayer");

	RefreshSaveSlots();
	for(int i = 0; i <= _Save::SLOT_9; i++)
		SaveSlots[i]->Checked = false;;
	SelectedColor = 0;
	SelectedSlot = -1;

	SinglePlayerState = SINGLEPLAYER_NONE;
	State = STATE_SINGLEPLAYER;
}

// Options
void _Menu::InitOptions() {
	ChangeLayout("element_menu_options");

	RefreshInputLabels();
	CurrentAction = -1;

	OptionsState = OPTION_NONE;
	State = STATE_OPTIONS;
}

// In-game menu
void _Menu::InitInGame() {
	ChangeLayout("element_menu_ingame");

	ae::Graphics.SetCursor(true);
	Background = nullptr;

	State = STATE_INGAME;
}

// Return to play
void _Menu::InitPlay() {
	if(CurrentLayout)
		CurrentLayout->SetActive(false);
	CurrentLayout = nullptr;

	State = STATE_NONE;
}

// Init new player popup
void _Menu::InitNewPlayer() {
	CurrentLayout->SetClickable(false);

	CurrentLayout = ae::Assets.Elements["element_menu_new"];
	CurrentLayout->SetActive(true);

	ae::_Element *Name = ae::Assets.Elements["textbox_new_name_input"];
	ae::FocusedElement = Name;
	Name->Text.clear();
	Name->ResetCursor();

	// Deselect previous elements
	for(int i = 0; i < COLOR_COUNT; i++) {
		std::stringstream Buffer;
		Buffer << PlayerColorButtonPrefix << i;

		ColorButtons[i] = ae::Assets.Elements[Buffer.str()];
		ColorButtons[i]->Checked = false;
		ColorButtons[i]->Index = i;
	}

	SelectedColor = 0;
	ColorButtons[SelectedColor]->Checked = true;

	SinglePlayerState = SINGLEPLAYER_NEW_PLAYER;
}

// Play the game
void _Menu::LaunchGame() {
	Save.GetPlayer(SelectedSlot)->Load();
	PlayState.Player = Save.GetPlayer(SelectedSlot);
	PlayState.Level = "";
	PlayState.TestMode = false;
	PlayState.FromEditor = false;
	Framework.ChangeState(&PlayState);

	SaveSlots[SelectedSlot]->Checked = false;
	State = STATE_NONE;
}

// Shutdown
void _Menu::Close() {
}

// Handle key event
bool _Menu::HandleKey(const ae::_KeyEvent &KeyEvent) {
	if(CurrentLayout)
		CurrentLayout->HandleKey(KeyEvent);

	switch(State) {
		case STATE_TITLE: {
			if(KeyEvent.Pressed && KeyEvent.Scancode == SDL_SCANCODE_ESCAPE)
				Framework.Done = true;
		} break;
		case STATE_SINGLEPLAYER: {

			if(SinglePlayerState == SINGLEPLAYER_NONE) {
				if(KeyEvent.Pressed && KeyEvent.Scancode == SDL_SCANCODE_ESCAPE)
					InitTitle();
			}
			else {
				if(KeyEvent.Pressed) {
					if(KeyEvent.Scancode == SDL_SCANCODE_ESCAPE)
						CancelCreate();
					else if(KeyEvent.Scancode == SDL_SCANCODE_RETURN)
						CreatePlayer();
				}
			}
		} break;
		case STATE_OPTIONS: {
			if(OptionsState == OPTION_NONE) {
				if(KeyEvent.Pressed && KeyEvent.Scancode == SDL_SCANCODE_ESCAPE) {
					Config.Save();
					if(Framework.GetState() == &PlayState)
						InitInGame();
					else
						InitTitle();
				}
			}
			else {
				if(KeyEvent.Pressed) {
					RemapInput(ae::_Input::KEYBOARD, KeyEvent.Scancode);
					return false;
				}
			}
		} break;
		case STATE_INGAME: {
			if(KeyEvent.Pressed && KeyEvent.Scancode == SDL_SCANCODE_ESCAPE)
				InitPlay();
		} break;
		default:
		break;
	}

	return true;
}

// Handle mouse event
void _Menu::HandleMouseButton(const ae::_MouseEvent &MouseEvent) {
	if(!CurrentLayout)
		return;

	// Accepting new action input
	switch(State) {
		case STATE_OPTIONS: {
			if(OptionsState == OPTION_ACCEPT_INPUT) {
				if(MouseEvent.Pressed) {
					RemapInput(ae::_Input::MOUSE_BUTTON, MouseEvent.Button);
					return;
				}
			}
		} break;
		default:
		break;
	}

	if(MouseEvent.Button == SDL_BUTTON_LEFT)
		CurrentLayout->HandleMouseButton(MouseEvent.Pressed);

	// Get clicked element
	ae::_Element *Clicked = CurrentLayout->GetClickedElement();
	if(Clicked) {
		bool DoubleClick = false;
		if(PreviousClick == Clicked && PreviousClickTimer < MENU_DOUBLECLICK_TIME) {
			PreviousClick = nullptr;
			DoubleClick = true;
		}
		else
			PreviousClick = Clicked;
		PreviousClickTimer = 0.0;

		switch(State) {
			case STATE_TITLE: {
				if(Clicked->Name == "button_title_tutorial") {
					InitTutorial();
				}
				else if(Clicked->Name == "button_title_single") {
					InitSinglePlayer();
				}
				else if(Clicked->Name == "button_title_options") {
					InitOptions();
				}
				else if(Clicked->Name == "button_title_exit") {
					Framework.Done = true;
				}
			} break;
			case STATE_SINGLEPLAYER: {
				if(SinglePlayerState == SINGLEPLAYER_NONE) {

					if(Clicked->Name == "button_singleplayer_delete") {
						if(SelectedSlot != -1) {
							Save.DeletePlayer(SelectedSlot);
							RefreshSaveSlots();

							SaveSlots[SelectedSlot]->Checked = false;
							SelectedSlot = -1;
						}
					}
					else if(Clicked->Name == "button_singleplayer_play") {
						if(SelectedSlot != -1 && Save.GetPlayer(SelectedSlot)) {
							LaunchGame();
						}
					}
					else if(Clicked->Name == "button_singleplayer_back") {
						InitTitle();
					}
					else if(Clicked->Name.substr(0, PlayerButtonPrefix.size()) == PlayerButtonPrefix) {

						// Deselect previous slot
						if(SelectedSlot != -1)
							SaveSlots[SelectedSlot]->Checked = false;

						// Set up create player screen
						if(!Save.GetPlayer(Clicked->Index)) {
							InitNewPlayer();
						}

						SelectedSlot = Clicked->Index;
						SaveSlots[SelectedSlot]->Checked = true;

						if(DoubleClick)
							LaunchGame();
					}
				}
				else {
					if(Clicked->Name.substr(0, PlayerColorButtonPrefix.size()) == PlayerColorButtonPrefix) {
						if(SelectedColor != -1)
							ColorButtons[SelectedColor]->Checked = false;

						SelectedColor = Clicked->Index;
						ColorButtons[SelectedColor]->Checked = true;
					}
					else if(Clicked->Name == "button_new_create") {
						CreatePlayer();
					}
					else if(Clicked->Name == "button_new_cancel") {
						CancelCreate();
					}
				}
			} break;
			case STATE_OPTIONS: {
				if(OptionsState == OPTION_NONE) {
					if(Clicked->Name == "button_options_defaults") {
						Config.LoadDefaultInputBindings(false);
						RefreshInputLabels();
					}
					else if(Clicked->Name == "button_options_save") {
						Config.Save();
						if(Framework.GetState() == &PlayState)
							InitInGame();
						else
							InitTitle();
					}
					else if(Clicked->Name == "button_options_cancel") {
						Config.Load();
						if(Framework.GetState() == &PlayState)
							InitInGame();
						else
							InitTitle();
					}
					else if(Clicked->Name.substr(0, InputBoxPrefix.size()) == InputBoxPrefix) {
						OptionsState = OPTION_ACCEPT_INPUT;
						CurrentAction = Clicked->Index;
						ae::Assets.Elements["label_menu_options_accept_text_action"]->Text = ae::Actions.GetInputNameForAction(CurrentAction, 0);
					}
				}
			} break;
			case STATE_INGAME: {
				if(Clicked->Name == "button_ingame_restart" && PlayState.Player) {
					InitPlay();
					PlayState.Player->SetCheckpointIndex(0);
					PlayState.Player->Save();
					PlayState.Player->Load();
					Framework.ChangeState(&PlayState);
				}
				else if(Clicked->Name == "button_ingame_resume") {
					InitPlay();
				}
				else if(Clicked->Name == "button_ingame_options") {
					InitOptions();
				}
				else if(Clicked->Name == "button_ingame_menu") {
					Framework.ChangeState(&NullState);
				}
			} break;
			default:
			break;
		}
	}
}

// Update phase
void _Menu::Update(double FrameTime) {
	PreviousClickTimer += FrameTime;

	switch(State) {
		case STATE_SINGLEPLAYER: {
			for(int i = 0; i <= _Save::SLOT_9; i++) {
				_Player *Player = Save.GetPlayer(i);
				if(Player) {
					Player->PositionChanged = true;
					Player->UpdateAnimation(FrameTime);
				}
			}
		} break;
		default:
		break;
	}
}

// Draw phase
void _Menu::Render() {
	ae::Graphics.Setup2D();
	ae::Graphics.SetStaticUniforms();

	if(Background)
		Background->Render();

	switch(State) {
		case STATE_TITLE: {
			if(CurrentLayout)
				CurrentLayout->Render();
			ae::Assets.Elements["label_game_version"]->Render();
		} break;
		case STATE_OPTIONS: {
			if(CurrentLayout)
				CurrentLayout->Render();

			if(OptionsState == OPTION_ACCEPT_INPUT) {
				ae::Graphics.FadeScreen(ae::Assets.Programs["ortho_pos"], MENU_ACCEPTINPUT_FADE);
				ae::Assets.Elements["element_menu_popup"]->SetActive(true);
				ae::Assets.Elements["element_menu_popup"]->Render();
			}
		} break;
		case STATE_SINGLEPLAYER: {
			ae::Assets.Elements["element_menu_singleplayer"]->Render();

			ae::Graphics.SetVBO(ae::VBO_QUAD);
			for(int i = 0; i <= _Save::SLOT_9; i++) {
				_Player *Player = Save.GetPlayer(i);
				if(Player)
					Player->Render2D(SaveSlots[i]->Bounds.GetCenter());
			}

			if(SinglePlayerState == SINGLEPLAYER_NEW_PLAYER) {
				ae::Graphics.FadeScreen(ae::Assets.Programs["ortho_pos"], MENU_ACCEPTINPUT_FADE);
				if(CurrentLayout)
					CurrentLayout->Render();
			}

		} break;
		case STATE_INGAME: {
			if(CurrentLayout)
				CurrentLayout->Render();
		} break;
		default:
		break;
	}
}

// Change menu layout
void _Menu::ChangeLayout(const std::string &ElementName) {
	ae::Assets.Elements["label_game_version"]->SetActive(false);

	if(CurrentLayout) {
		CurrentLayout->SetActive(false);
	}

	CurrentLayout = ae::Assets.Elements[ElementName];
	CurrentLayout->SetActive(true);
}

// Refreshes the save slots after player creation
void _Menu::RefreshSaveSlots() {
	CurrentLayout->SetClickable(true);

	// Load save slots
	for(int i = 0; i <= _Save::SLOT_9; i++) {
		std::stringstream Buffer;
		Buffer << "label_menu_singleplayer_slot" << i << "_text";
		ae::_Element *SlotLabel = ae::Assets.Elements[Buffer.str()];
		Buffer.str("");

		_Player *Player = Save.GetPlayer(i);
		if(Player) {
			Player->SetLegAnimationPlayMode(PLAYING);
			Player->Animation->SetPlayMode(PLAYING);
			SlotLabel->Text = Player->GetName();
		}
		else
			SlotLabel->Text = "Empty Slot";

		Buffer << PlayerButtonPrefix << i;
		SaveSlots[i] = ae::Assets.Elements[Buffer.str()];
		SaveSlots[i]->Index = i;
	}
}

// Refreshes the input map labels
void _Menu::RefreshInputLabels() {
	for(size_t i = 0; i < LABEL_COUNT; i++) {
		InputLabels[i] = ae::Assets.Elements[KEYLABEL_IDENTIFIERS[i]];
		InputLabels[i]->Text = ae::Actions.GetInputNameForAction(i);
		InputLabels[i]->Parent->Index = i;
	}
}

// Cancel create screen
void _Menu::CancelCreate() {
	CurrentLayout = ae::Assets.Elements["element_menu_singleplayer"];
	CurrentLayout->SetClickable(true);
	SinglePlayerState = SINGLEPLAYER_NONE;

	SaveSlots[SelectedSlot]->Checked = false;
}

// Handle player creation
void _Menu::CreatePlayer() {
	if(ae::Assets.Elements["textbox_new_name_input"]->Text.length() == 0)
		return;

	CurrentLayout = ae::Assets.Elements["element_menu_singleplayer"];
	SinglePlayerState = SINGLEPLAYER_NONE;

	if(SelectedSlot != -1) {
		Save.CreateNewPlayer(SelectedSlot, ae::Assets.Elements["textbox_new_name_input"]->Text, COLORS[SelectedColor]);
		RefreshSaveSlots();
	}
}

// Clear action on keybinding page
void _Menu::ClearAction(int Action, int Type) {
	for(int i = 0; i < ae::_Input::INPUT_COUNT; i++) {
		if(ae::Actions.GetInputForAction(i, Action, Type) != -1)
			ae::Actions.ClearMappingsForAction(i, Action, Type);
	}
}

// Remap a key/button
void _Menu::RemapInput(int InputType, int Input) {
	OptionsState = OPTION_NONE;
	if(InputType == ae::_Input::KEYBOARD) {
		if(Input == SDL_SCANCODE_ESCAPE || Input == SDL_SCANCODE_RETURN || Input == SDL_SCANCODE_KP_ENTER)
			return;
	}

	// Remove duplicate keys/buttons
	for(const auto &Action : KeyBindings)
		ae::Actions.ClearMappingForInputAction(InputType, Input, Action);

	// Clear out existing action
	ClearAction(CurrentAction, 0);

	// Add new binding
	ae::Actions.AddInputMap(0, InputType, Input, CurrentAction, 1.0f, -1.0f, false);

	// Update menu labels
	RefreshInputLabels();
}
