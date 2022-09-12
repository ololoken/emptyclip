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
#include <states/play.h>
#include <states/null.h>
#include <objects/player.h>
#include <ae/input.h>
#include <ae/actions.h>
#include <ae/texture.h>
#include <ae/assets.h>
#include <ae/graphics.h>
#include <ae/animation.h>
#include <ae/util.h>
#include <ae/ui.h>
#include <ae/audio.h>
#include <ae/font.h>
#include <actiontype.h>
#include <achievements.h>
#include <hud.h>
#include <stats.h>
#include <constants.h>
#include <gameassets.h>
#include <config.h>
#include <framework.h>
#include <version.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <SDL_mouse.h>

_Menu Menu;

static const std::string InputBoxPrefix = "button_menu_controls_input_";
static const std::string PlayerButtonPrefix = "button_menu_singleplayer_slot";
static const std::string PlayerColorButtonPrefix = "button_menu_new_color";

static const int KeyBindings[] = {
	Action::GAME_UP,
	Action::GAME_DOWN,
	Action::GAME_LEFT,
	Action::GAME_RIGHT,
	Action::GAME_USE,
	Action::GAME_SPRINT,
	Action::GAME_MAP,
	Action::GAME_FLASHLIGHT,
	Action::GAME_FIRE,
	Action::GAME_AIM,
	Action::GAME_MELEE,
	Action::GAME_RELOAD,
	Action::GAME_WEAPONSWITCH,
	Action::GAME_INVENTORY,
	Action::GAME_SORTINVENTORY,
};

static const std::string KEYLABELS[] = {
	"label_menu_controls_config_up",
	"label_menu_controls_config_down",
	"label_menu_controls_config_left",
	"label_menu_controls_config_right",
	"label_menu_controls_config_use",
	"label_menu_controls_config_sprint",
	"label_menu_controls_config_map",
	"label_menu_controls_config_flashlight",
	"label_menu_controls_config_fire",
	"label_menu_controls_config_aim",
	"label_menu_controls_config_melee",
	"label_menu_controls_config_reload",
	"label_menu_controls_config_weaponswitch",
	"label_menu_controls_config_inventory",
	"label_menu_controls_config_sortinventory",
};

static const char *COLORS[] = {
	"black",
	"red",
	"green",
	"blue",
};

// Initialize
void _Menu::Init() {

	// Create achievement buttons
	ae::_Element *AchievementContainer = ae::Assets.Elements["element_menu_achievements_container"];
	glm::vec2 Size = glm::vec2(500, 120);
	glm::vec2 Spacing = glm::vec2(30, 140);
	glm::vec2 Offset = glm::vec2(0, 0);
	int Column = 0;
	for(const auto &Achievement : Stats.Achievements) {

		ae::_Element *Button = new ae::_Element();
		Button->Name = Achievement.ID;
		Button->Parent = AchievementContainer;
		Button->BaseOffset = glm::vec2(-Size.x / 2 - Spacing.x, Offset.y);
		Button->BaseSize = Size;
		Button->Alignment = ae::_Alignment(ae::_Alignment::CENTER, ae::_Alignment::TOP);
		Button->Style = ae::Assets.Styles["style_menu_window"];
		Button->DisabledStyle = ae::Assets.Styles["style_menu_window_disabled"];
		if(Column & 1)
			Button->BaseOffset.x = -Button->BaseOffset.x;

		ae::_Element *Title = new ae::_Element();
		Title->Parent = Button;
		Title->Text = Achievement.Name;
		Title->BaseOffset = glm::vec2(20, 40);
		Title->Alignment = ae::LEFT_BASELINE;
		Title->Font = ae::Assets.Fonts["hud_medium"];
		Button->Children.push_back(Title);

		ae::_Element *Text = new ae::_Element();
		Text->Parent = Title;
		Text->Text = Achievement.Text;
		Text->Color = glm::vec4(0.85f, 0.85f, 0.85f, 1.0f);
		Text->BaseOffset = glm::vec2(0, 30);
		Text->Alignment = ae::LEFT_BASELINE;
		Text->Font = ae::Assets.Fonts["hud_small"];
		Text->BaseSize = Size - glm::vec2(40, 20);
		Text->Wrap = true;
		Title->Children.push_back(Text);

		AchievementContainer->Children.push_back(Button);
		Column++;
		if(Column > 1) {
			Offset.y += Spacing.y;
			Column = 0;
		}
	}

	AchievementContainer->CalculateBounds();
}

// Init title screen
void _Menu::InitTitle() {
	ChangeLayout("element_menu_title");

	std::string BuildVersion;
	if(std::string(BUILD_VERSION) != "")
		BuildVersion = std::string("-") + BUILD_VERSION;
	ae::Graphics.SetCursor(true);

	ae::Assets.Elements["label_game_version"]->Text = GAME_VERSION + BuildVersion;
	ae::Assets.Elements["label_game_version"]->SetActive(true);

	if(!Achievements.Enabled) {
		ae::Assets.Elements["button_menu_title_achievements"]->SetEnabled(false);
		ae::Assets.Elements["button_menu_ingame_achievements"]->SetEnabled(false);
	}

	Background = ae::Assets.Elements["image_menu_bg"];
	HandleResize();

	State = STATE_TITLE;
}

// Init single player
void _Menu::InitSinglePlayer() {
	ChangeLayout("element_menu_singleplayer");

	Save.LoadSaves();
	RefreshSaveSlots();
	for(int i = 0; i <= _Save::SLOT_9; i++)
		SaveSlots[i]->Checked = false;

	SelectedColor = 0;
	SelectedSlot = -1;

	SinglePlayerState = SINGLEPLAYER_NONE;
	State = STATE_SINGLEPLAYER;
}

// Options
void _Menu::InitOptions() {
	ChangeLayout("element_menu_options");

	// Set up MSAA values
	MSAAValues.clear();
	MSAAValues.push_back(0);
	int Samples = 2;
	while(Samples <= ae::Graphics.MaxSamples) {
		MSAAValues.push_back(Samples);
		Samples *= 2;
	}

	// Set up Aniso values
	AnisoValues.clear();
	AnisoValues.push_back(0);
	int Aniso = 2;
	while(Aniso <= ae::Graphics.MaxAnisotropy) {
		AnisoValues.push_back(Aniso);
		Aniso *= 2;
	}

	UpdateOptions();

	State = STATE_OPTIONS;
}

// Controls
void _Menu::InitControls() {
	ChangeLayout("element_menu_controls");

	RefreshInputLabels();
	CurrentAction = -1;

	OptionsState = OPTION_NONE;
	State = STATE_CONTROLS;
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
	Background = nullptr;

	State = STATE_NONE;
}

// Init the end of level score screen
void _Menu::InitScore() {
	ChangeLayout("element_menu_score");

	ae::Graphics.SetCursor(true);

	Background = ae::Assets.Elements["image_menu_bg"];
	HandleResize();

	State = STATE_SCORE;
}

// Init achievements screen
void _Menu::InitAchievements() {
	ChangeLayout("element_menu_achievements");

	// Set enabled state
	ae::_Element *AchievementContainer = ae::Assets.Elements["element_menu_achievements_container"];
	for(const auto &Child : AchievementContainer->Children)
		Child->SetEnabled(Achievements.Stats.find(Child->Name) != Achievements.Stats.end());

	State = STATE_ACHIEVEMENTS;
}

// Init new player popup
void _Menu::InitNewPlayer() {
	CurrentLayout->SetClickable(false);

	CurrentLayout = ae::Assets.Elements["element_menu_new"];
	CurrentLayout->SetActive(true);

	ae::_Element *Name = ae::Assets.Elements["textbox_menu_new_name_input"];
	ae::FocusedElement = Name;
	Name->Text.clear();
	Name->ResetCursor();

	// Deselect previous elements
	for(int i = 0; i < COLOR_COUNT; i++) {
		std::ostringstream Buffer;
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
	Save.LoadPlayer(Save.GetPlayer(SelectedSlot));
	PlayState.Player = Save.GetPlayer(SelectedSlot);
	PlayState.Level = "";
	PlayState.TestMode = false;
	PlayState.FromEditor = false;
	Framework.ChangeState(&PlayState);

	SaveSlots[SelectedSlot]->Checked = false;
	Background = nullptr;
	State = STATE_NONE;
}

// Update option elements
void _Menu::UpdateOptions() {

	// Set checkboxes
	ae::Assets.Elements["label_menu_options_fullscreen_check"]->Text = Config.Fullscreen ? "X" : "";
	ae::Assets.Elements["label_menu_options_gunflashes_check"]->Text = Config.WeaponFlashes ? "X" : "";
	ae::Assets.Elements["label_menu_options_walldecals_check"]->Text = Config.WallDecals ? "X" : "";
	ae::Assets.Elements["label_menu_options_floordecals_check"]->Text = Config.FloorDecals ? "X" : "";
	ae::Assets.Elements["label_menu_options_autoequip_check"]->Text = Config.AutoEquip ? "X" : "";
	ae::Assets.Elements["label_menu_options_tutorial_check"]->Text = Config.Tutorial ? "X" : "";

	// Set sound volume
	{
		std::ostringstream Buffer;
		Buffer << std::fixed << std::setprecision(2) << Config.SoundVolume;
		ae::Assets.Elements["label_menu_options_soundvolume_value"]->Text = Buffer.str();
		Buffer.str("");

		ae::Assets.Elements["button_menu_options_soundvolume"]->SetOffsetPercent(glm::vec2(Config.SoundVolume, 0));
	}

	// Set MSAA
	{
		float Offset = 0.0f;
		for(size_t i = 0; i < MSAAValues.size(); i++) {
			if(Config.MSAA == MSAAValues[i]) {
				Offset = i * (1.0f / (MSAAValues.size() - 1));
				break;
			}
		}

		ae::Assets.Elements["button_menu_options_msaa"]->SetOffsetPercent(glm::vec2(Offset, 0));
		ae::Assets.Elements["label_menu_options_msaa_value"]->Text = std::to_string(Config.MSAA);
	}

	// Set anisotropic filtering
	{
		float Offset = 0.0f;
		for(size_t i = 0; i < AnisoValues.size(); i++) {
			if(Config.Anisotrophy == AnisoValues[i]) {
				Offset = i * (1.0f / (AnisoValues.size() - 1));
				break;
			}
		}

		ae::Assets.Elements["button_menu_options_aniso"]->SetOffsetPercent(glm::vec2(Offset, 0));
		ae::Assets.Elements["label_menu_options_aniso_value"]->Text = std::to_string(Config.Anisotrophy);
	}
}

// Update config and audio volumes from options
void _Menu::UpdateVolume() {
	ae::_Element *SoundSlider = ae::Assets.Elements["element_menu_options_soundvolume"];
	ae::_Element *SoundVolume = ae::Assets.Elements["label_menu_options_soundvolume_value"];
	ae::_Element *SoundButton = ae::Assets.Elements["button_menu_options_soundvolume"];

	// Handle clicking inside slider elements
	if(!SoundButton->PressedElement && SoundSlider->PressedElement) {
		SoundButton->PressedOffset = SoundButton->Size / 2.0f;
		SoundButton->PressedElement = SoundButton;
	}

	// Update volume
	if(SoundButton->PressedElement) {

		// Convert slider percent to number
		std::ostringstream Buffer;
		Buffer << std::fixed << std::setprecision(2) << SoundButton->GetOffsetPercent().x;
		SoundVolume->Text = Buffer.str();
		Buffer.str("");

		// Set volumes
		Config.SoundVolume = ae::ToNumber<float>(SoundVolume->Text);
		ae::Audio.SetSoundVolume(Config.SoundVolume);
	}
}

// Update MSAA slider
void _Menu::UpdateMSAA() {
	ae::_Element *MSAASlider = ae::Assets.Elements["element_menu_options_msaa"];
	ae::_Element *MSAAValue = ae::Assets.Elements["label_menu_options_msaa_value"];
	ae::_Element *MSAAButton = ae::Assets.Elements["button_menu_options_msaa"];

	// Handle clicking inside slider elements
	if(!MSAAButton->PressedElement && MSAASlider->PressedElement) {
		MSAAButton->PressedOffset = MSAAButton->Size / 2.0f;
		MSAAButton->PressedElement = MSAAButton;
	}

	// Update value
	if(MSAAButton->PressedElement) {
		int Index = std::clamp((int)(MSAAValues.size() * MSAAButton->GetOffsetPercent().x), 0, (int)MSAAValues.size() - 1);

		Config.MSAA = MSAAValues[Index];

		std::ostringstream Buffer;
		Buffer << std::fixed << Config.MSAA;
		MSAAValue->Text = Buffer.str();
		Buffer.str("");
	}
}

// Update anisotropic filtering slider
void _Menu::UpdateAniso() {
	ae::_Element *AnisoSlider = ae::Assets.Elements["element_menu_options_aniso"];
	ae::_Element *AnisoValue = ae::Assets.Elements["label_menu_options_aniso_value"];
	ae::_Element *AnisoButton = ae::Assets.Elements["button_menu_options_aniso"];

	// Handle clicking inside slider elements
	if(!AnisoButton->PressedElement && AnisoSlider->PressedElement) {
		AnisoButton->PressedOffset = AnisoButton->Size / 2.0f;
		AnisoButton->PressedElement = AnisoButton;
	}

	// Update value
	if(AnisoButton->PressedElement) {
		int Index = std::clamp((int)(AnisoValues.size() * AnisoButton->GetOffsetPercent().x), 0, (int)AnisoValues.size() - 1);

		Config.Anisotrophy = AnisoValues[Index];

		std::ostringstream Buffer;
		Buffer << std::fixed << Config.Anisotrophy;
		AnisoValue->Text = Buffer.str();
		Buffer.str("");
	}
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
			if(KeyEvent.Pressed) {
				if(KeyEvent.Scancode == SDL_SCANCODE_ESCAPE)
					Framework.Done = true;
				else if(KeyEvent.Scancode == SDL_SCANCODE_RETURN)
					InitSinglePlayer();
			}
		} break;
		case STATE_SINGLEPLAYER: {
			if(SinglePlayerState == SINGLEPLAYER_NONE) {
				if(KeyEvent.Pressed) {
					if(KeyEvent.Scancode == SDL_SCANCODE_ESCAPE)
						InitTitle();
					else if(KeyEvent.Scancode == SDL_SCANCODE_RETURN) {
						if(SelectedSlot == -1) {
							for(int i = 0; i < _Save::SLOT_COUNT; i++) {
								if(Save.GetPlayer(i)) {
									SelectedSlot = i;
									break;
								}
							}
						}

						if(SelectedSlot >= 0 && Save.GetPlayer(SelectedSlot))
							LaunchGame();
					}
				}
			}
			else {
				if(KeyEvent.Pressed) {
					if(KeyEvent.Scancode == SDL_SCANCODE_ESCAPE)
						SinglePlayerCancel();
					else if(KeyEvent.Scancode == SDL_SCANCODE_RETURN)
						CreatePlayer();
				}
			}
		} break;
		case STATE_OPTIONS: {
			if(KeyEvent.Pressed && KeyEvent.Scancode == SDL_SCANCODE_ESCAPE) {
				Config.Save();
				if(Framework.GetState() == &PlayState)
					InitInGame();
				else
					InitTitle();
			}
		} break;
		case STATE_CONTROLS: {
			if(OptionsState == OPTION_NONE) {
				if(KeyEvent.Pressed && KeyEvent.Scancode == SDL_SCANCODE_ESCAPE) {
					Config.Save();
					InitOptions();
				}
			}
			else {
				if(KeyEvent.Pressed) {
					RemapInput(ae::_Input::KEYBOARD, KeyEvent.Scancode);
					if(CurrentAction == Action::GAME_UP || CurrentAction == Action::GAME_DOWN || CurrentAction == Action::GAME_LEFT || CurrentAction == Action::GAME_RIGHT) {
						int Up = ae::Actions.GetInputForAction(ae::_Input::KEYBOARD, Action::GAME_UP, 0);
						int Down = ae::Actions.GetInputForAction(ae::_Input::KEYBOARD, Action::GAME_DOWN, 0);
						int Left = ae::Actions.GetInputForAction(ae::_Input::KEYBOARD, Action::GAME_LEFT, 0);
						int Right = ae::Actions.GetInputForAction(ae::_Input::KEYBOARD, Action::GAME_RIGHT, 0);
						if(Up == SDL_SCANCODE_W && Down == SDL_SCANCODE_S && Left == SDL_SCANCODE_A && Right == SDL_SCANCODE_D)
							ae::Audio.PlaySound(ae::Assets.Sounds["player_hit0.ogg"]);
					}
					return false;
				}
			}
		} break;
		case STATE_ACHIEVEMENTS: {
			if(KeyEvent.Pressed && KeyEvent.Scancode == SDL_SCANCODE_ESCAPE) {
				if(Framework.GetState() == &PlayState)
					InitInGame();
				else
					InitTitle();
			}
		} break;
		case STATE_INGAME: {
			if(KeyEvent.Pressed && KeyEvent.Scancode == SDL_SCANCODE_ESCAPE)
				InitPlay();
		} break;
		case STATE_SCORE: {
			if(KeyEvent.Pressed && KeyEvent.Scancode == SDL_SCANCODE_ESCAPE) {
				InitPlay();
				Framework.ChangeState(&PlayState);
			}
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
		case STATE_CONTROLS: {
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
				if(Clicked->Name == "button_menu_title_single") {
					InitSinglePlayer();
				}
				else if(Clicked->Name == "button_menu_title_options") {
					InitOptions();
				}
				else if(Clicked->Name == "button_menu_title_achievements") {
					InitAchievements();
				}
				else if(Clicked->Name == "button_menu_title_exit") {
					Framework.Done = true;
				}
			} break;
			case STATE_SINGLEPLAYER: {
				switch(SinglePlayerState) {
					case SINGLEPLAYER_NONE:
						if(Clicked->Name == "button_menu_singleplayer_delete") {
							if(SelectedSlot != -1) {
								CurrentLayout->SetClickable(false);
								SinglePlayerState = SINGLEPLAYER_DELETE;
								ConfirmAction();
							}
						}
						else if(Clicked->Name == "button_menu_singleplayer_play") {
							if(SelectedSlot != -1 && Save.GetPlayer(SelectedSlot)) {
								LaunchGame();
							}
						}
						else if(Clicked->Name == "button_menu_singleplayer_back") {
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
					break;
					case SINGLEPLAYER_NEW_PLAYER:
						if(Clicked->Name.substr(0, PlayerColorButtonPrefix.size()) == PlayerColorButtonPrefix) {
							if(SelectedColor != -1)
								ColorButtons[SelectedColor]->Checked = false;

							SelectedColor = Clicked->Index;
							ColorButtons[SelectedColor]->Checked = true;
						}
						else if(Clicked->Name == "button_menu_new_create") {
							CreatePlayer();
						}
						else if(Clicked->Name == "button_menu_new_cancel") {
							SinglePlayerCancel();
						}
					break;
					case SINGLEPLAYER_DELETE:
						if(Clicked->Name == "button_confirm_ok") {
							if(SelectedSlot != -1) {
								Save.DeletePlayer(SelectedSlot);
								InitSinglePlayer();
							}
						}
						else if(Clicked->Name == "button_confirm_cancel") {
							SinglePlayerCancel();
						}
					break;
				}
			} break;
			case STATE_OPTIONS: {
				if(OptionsState == OPTION_NONE) {
					if(Clicked->Name == "button_menu_options_fullscreen") {
						SetFullscreen(!Config.Fullscreen);
						UpdateOptions();
					}
					else if(Clicked->Name == "button_menu_options_gunflashes") {
						Config.WeaponFlashes = !Config.WeaponFlashes;
						UpdateOptions();
					}
					else if(Clicked->Name == "button_menu_options_walldecals") {
						Config.WallDecals = !Config.WallDecals;
						UpdateOptions();
					}
					else if(Clicked->Name == "button_menu_options_floordecals") {
						Config.FloorDecals = !Config.FloorDecals;
						UpdateOptions();
					}
					else if(Clicked->Name == "button_menu_options_autoequip") {
						Config.AutoEquip = !Config.AutoEquip;
						UpdateOptions();
					}
					else if(Clicked->Name == "button_menu_options_tutorial") {
						Config.Tutorial = !Config.Tutorial;
						UpdateOptions();
					}
					else if(Clicked->Name == "button_menu_options_controls") {
						InitControls();
					}
					else if(Clicked->Name == "button_menu_options_defaults") {
						Config.SetDefaults(true);
						ae::Audio.SetSoundVolume(Config.SoundVolume);
						UpdateOptions();
					}
					else if(Clicked->Name == "button_menu_options_save") {
						Config.Save();
						if(Framework.GetState() == &PlayState)
							InitInGame();
						else
							InitTitle();
					}
					else if(Clicked->Name == "button_menu_options_cancel") {
						Config.Load();
						if(Framework.GetState() == &PlayState)
							InitInGame();
						else
							InitTitle();
					}
					else if(Clicked->Name.substr(0, InputBoxPrefix.size()) == InputBoxPrefix) {
						OptionsState = OPTION_ACCEPT_INPUT;
						CurrentAction = Clicked->Index;
						ae::Assets.Elements["label_menu_controls_accept_text_action"]->Text = Clicked->Children.front()->Text;
					}
				}
			} break;
			case STATE_CONTROLS: {
				if(Clicked->Name == "button_menu_controls_defaults") {
					Config.LoadDefaultInputBindings(false);
					RefreshInputLabels();
				}
				else if(Clicked->Name == "button_menu_controls_save") {
					Config.Save();
					InitOptions();
				}
				else if(Clicked->Name == "button_menu_controls_cancel") {
					Config.Load();
					InitOptions();
				}
				else if(Clicked->Name.substr(0, InputBoxPrefix.size()) == InputBoxPrefix) {
					OptionsState = OPTION_ACCEPT_INPUT;
					CurrentAction = Clicked->Index;
					ae::Assets.Elements["label_menu_controls_accept_text_action"]->Text = Clicked->Children.front()->Text;
				}
			} break;
			case STATE_ACHIEVEMENTS: {
				if(Clicked->Name == "button_menu_achievements_back") {
					if(Framework.GetState() == &PlayState)
						InitInGame();
					else
						InitTitle();
				}
			} break;
			case STATE_INGAME: {
				if(Clicked->Name == "button_menu_ingame_resume") {
					InitPlay();
				}
				else if(Clicked->Name == "button_menu_ingame_options") {
					InitOptions();
				}
				else if(Clicked->Name == "button_menu_ingame_achievements") {
					InitAchievements();
				}
				else if(Clicked->Name == "button_menu_ingame_mainmenu") {
					Framework.ChangeState(&NullState);
				}
			} break;
			case STATE_SCORE: {
				if(Clicked->Name == "button_menu_score_continue") {
					InitPlay();
					Framework.ChangeState(&PlayState);
				}
				else if(Clicked->Name == "button_menu_score_mainmenu") {
					InitTitle();
				}
			} break;
			default:
			break;
		}
	}
}

// Handle window resize
void _Menu::HandleResize() {
	if(Background) {
		Background->SetWidth(Background->Texture->Size.x * (ae::Graphics.CurrentSize.y / (float)Background->Texture->Size.y));
		Background->SetHeight(ae::Graphics.CurrentSize.y);
		Background->SetActive(true);
	}

	ae::Assets.Elements["element_menu_achievements_container"]->CalculateBounds();
}

// Set fullscreen state of game
void _Menu::SetFullscreen(bool Fullscreen) {
	if(!ae::Graphics.SetFullscreen(Fullscreen))
		return;

	Config.Fullscreen = Fullscreen;
	Config.Save();

	// Reload fonts
	ae::Assets.LoadFonts("ui/fonts.tsv");
	ae::Graphics.ResetState();
}

// Update phase
void _Menu::Update(double FrameTime) {
	PreviousClickTimer += FrameTime;

	// Update messages
	for(auto Iterator = AchievementMessages.begin(); Iterator != AchievementMessages.end(); ) {
		_Message &Message = *Iterator;

		// Update timer
		Message.Time += FrameTime;
		if(Message.Time >= ACHIEVEMENTS_MESSAGE_TIMEOUT) {
			Iterator = AchievementMessages.erase(Iterator);
		}
		else
			++Iterator;
	}

	// Update states
	switch(State) {
		case STATE_SINGLEPLAYER: {
			for(int i = 0; i <= _Save::SLOT_9; i++) {
				_Player *Player = Save.GetPlayer(i);
				if(Player) {
					Player->PositionChanged = true;
					Player->UpdateAnimation(FrameTime, false);
				}
			}
		} break;
		case STATE_OPTIONS:
			UpdateVolume();
			UpdateMSAA();
			UpdateAniso();
		break;
		default:
		break;
	}
}

// Draw phase
void _Menu::Render() {
	ae::Graphics.Setup2D();
	ae::Graphics.SetStaticUniforms();

	if(State != STATE_NONE && Background)
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
		} break;
		case STATE_ACHIEVEMENTS: {
			if(CurrentLayout)
				CurrentLayout->Render();
		} break;
		case STATE_CONTROLS: {
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

			if(SinglePlayerState == SINGLEPLAYER_NEW_PLAYER || SinglePlayerState == SINGLEPLAYER_DELETE) {
				ae::Graphics.FadeScreen(ae::Assets.Programs["ortho_pos"], MENU_ACCEPTINPUT_FADE);
				if(CurrentLayout)
					CurrentLayout->Render();
			}
		} break;
		case STATE_INGAME: {
			if(CurrentLayout)
				CurrentLayout->Render();
		} break;
		case STATE_SCORE: {
			if(CurrentLayout)
				CurrentLayout->Render();
		} break;
		default:
		break;
	}

	DrawMessages();
}

// Draw achievement messages
void _Menu::DrawMessages() {

	glm::vec2 DrawSize = glm::vec2(320, 100) * ae::_Element::GetUIScale();
	glm::vec2 DrawPosition(ae::Graphics.CurrentSize.x - DrawSize.x, ae::Graphics.CurrentSize.y);
	for(auto &Message : AchievementMessages) {
		DrawPosition.y -= DrawSize.y;
		ae::_Bounds Bounds(DrawPosition, DrawPosition + DrawSize);

		// Get alpha
		glm::vec4 Color;
		double TimeLeft = ACHIEVEMENTS_MESSAGE_TIMEOUT - Message.Time;
		float Fade = 1.0f;
		if(TimeLeft < ACHIEVEMENTS_MESSAGE_FADETIME)
			Fade = (float)(TimeLeft / ACHIEVEMENTS_MESSAGE_FADETIME);

		// Draw box
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos"]);
		Color = ae::Assets.Colors["menu_red_opaque"];
		Color.a = Fade;
		ae::Graphics.SetColor(Color);
		ae::Graphics.DrawRectangle(Bounds, true);

		// Draw outline
		Color = ae::Assets.Colors["menu_red_border"];
		Color.a = Fade;
		ae::Graphics.SetColor(Color);
		ae::Graphics.DrawRectangle(Bounds, false);

		// Draw text
		Color = glm::vec4(1.0f);
		Color.a = Fade;
		ae::Assets.Fonts["hud_medium"]->DrawText("Achievement Unlocked!", DrawPosition + glm::vec2(15, 35) * ae::_Element::GetUIScale(), ae::LEFT_BASELINE, Color);

		// Draw text
		Color = glm::vec4(0.85f, 0.85f, 0.85f, 1.0f);
		Color.a = Fade;
		ae::Assets.Fonts["hud_small"]->DrawText(Message.Name, DrawPosition + glm::vec2(15, 65) * ae::_Element::GetUIScale(), ae::LEFT_BASELINE, Color);
	}
}

// Show achievement message
void _Menu::UnlockAchievement(const std::string &ID) {
	if(PlayState.TestMode || PlayState.DevMode || !Achievements.Enabled)
		return;

	if(Achievements.Stats.find(ID) != Achievements.Stats.end())
		return;

	// Find ID
	for(const auto &Achievement : Stats.Achievements) {
		if(Achievement.ID != ID)
			continue;

		// Add message
		_Message Message;
		Message.Name = Achievement.Name;
		Message.Time = 0.0;
		AchievementMessages.push_back(Message);

		// Save stats
		Achievements.Stats[ID] = 1;
		Achievements.Save();

		return;
	}
}

// Update score screen label values
void _Menu::SetScoreStats(bool EndOfGame, double LevelTime, int *Kills, int *Crates, int *Secrets, int Progression, const std::string &WeaponsUsed) {
	std::ostringstream Buffer;

	// Set title
	if(EndOfGame) {
		ae::Assets.Elements["label_menu_score_title"]->Text = "Campaign Completed!";
		ae::Assets.Elements["label_menu_score_continue"]->Text = "Start Progression " + std::to_string(Progression);
		ae::Assets.Elements["button_menu_score_continue"]->BaseOffset.x = -210;
		ae::Assets.Elements["button_menu_score_continue"]->BaseSize.x = 380;
		ae::Assets.Elements["button_menu_score_continue"]->CalculateBounds();
	}
	else {
		ae::Assets.Elements["label_menu_score_title"]->Text = "Level Completed!";
		ae::Assets.Elements["label_menu_score_continue"]->Text = "Continue";
		ae::Assets.Elements["button_menu_score_continue"]->BaseOffset.x = -120;
		ae::Assets.Elements["button_menu_score_continue"]->BaseSize.x = 200;
		ae::Assets.Elements["button_menu_score_continue"]->CalculateBounds();
	}

	// Set time
	char TimeBuffer[256];
	_HUD::FormatTime(TimeBuffer, LevelTime);
	ae::Assets.Elements["label_menu_score_time_value"]->Text = TimeBuffer;

	// Set kills
	Buffer << Kills[0] << "/" << Kills[1];
	ae::Assets.Elements["label_menu_score_kills_value"]->Text = Buffer.str();
	Buffer.str("");

	// Set crates
	Buffer << Crates[0] << "/" << Crates[1];
	ae::Assets.Elements["label_menu_score_crates_value"]->Text = Buffer.str();
	Buffer.str("");

	// Set secrets
	Buffer << Secrets[0] << "/" << Secrets[1];
	ae::Assets.Elements["label_menu_score_secrets_value"]->Text = Buffer.str();
	Buffer.str("");

	// Set weapons used
	ae::_Element *WeaponsElement = ae::Assets.Elements["label_menu_score_weapons_value"];
	WeaponsElement->Text = WeaponsUsed;
	WeaponsElement->SetWrap(WeaponsElement->Size.x);
}

// Change menu layout
void _Menu::ChangeLayout(const std::string &ElementName) {
	ae::Assets.Elements["label_game_version"]->SetActive(false);

	if(CurrentLayout)
		CurrentLayout->SetActive(false);

	CurrentLayout = ae::Assets.Elements[ElementName];
	CurrentLayout->SetActive(true);
}

// Refreshes the save slots after player creation
void _Menu::RefreshSaveSlots() {
	CurrentLayout->SetClickable(true);

	// Load save slots
	for(int i = 0; i <= _Save::SLOT_9; i++) {
		std::ostringstream Buffer;
		Buffer << "label_menu_singleplayer_slot" << i << "_text";
		ae::_Element *SlotLabel = ae::Assets.Elements[Buffer.str()];
		Buffer.str("");

		_Player *Player = Save.GetPlayer(i);
		if(Player) {
			Player->UpdateSpeed(1.0f);
			Player->SetLegAnimationPlayMode(ae::_Animation::PLAYING);
			Player->Animation->Play(0);
			SlotLabel->Text = Player->Name;
			if(Player->Progression)
				SlotLabel->Text += " ([c gold]" + std::to_string(Player->Progression) + "[c white])";
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
		InputLabels[i] = ae::Assets.Elements[KEYLABELS[i]];
		InputLabels[i]->Text = ae::Actions.GetInputNameForAction(i);
		InputLabels[i]->Parent->Index = i;
	}
}

// Cancel on single player screen
void _Menu::SinglePlayerCancel() {
	CurrentLayout = ae::Assets.Elements["element_menu_singleplayer"];
	CurrentLayout->SetClickable(true);
	SinglePlayerState = SINGLEPLAYER_NONE;
	ae::FocusedElement = nullptr;

	SaveSlots[SelectedSlot]->Checked = false;
	SelectedSlot = -1;
}

// Handle player creation
void _Menu::CreatePlayer() {
	if(ae::Assets.Elements["textbox_menu_new_name_input"]->Text.length() == 0)
		return;

	CurrentLayout = ae::Assets.Elements["element_menu_singleplayer"];
	SinglePlayerState = SINGLEPLAYER_NONE;

	if(SelectedSlot != -1) {
		Save.CreateNewPlayer(SelectedSlot, ae::Assets.Elements["textbox_menu_new_name_input"]->Text, COLORS[SelectedColor]);
		RefreshSaveSlots();
		ae::FocusedElement = nullptr;
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

// Show the confirm screen
void _Menu::ConfirmAction() {
	CurrentLayout = ae::Assets.Elements["element_confirm"];
	ae::Assets.Elements["label_confirm_warning"]->Text = "Are you sure?";

	CurrentLayout->SetActive(true);
}
