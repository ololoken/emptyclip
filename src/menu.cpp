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
#include <ae/console.h>
#include <actiontype.h>
#include <achievements.h>
#include <save.h>
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
	Action::GAME_MOREINFO,
	Action::GAME_FILTERGEAR,
	Action::GAME_FILTERMODS,
	Action::GAME_SWITCHOUTFIT,
	Action::MISC_CONSOLE,
	Action::MISC_MENU,
	Action::MISC_DEBUG,
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
	"label_menu_controls_config_moreinfo",
	"label_menu_controls_config_filtergear",
	"label_menu_controls_config_filtermods",
	"label_menu_controls_config_switchoutfit",
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
	int Index = 0;
	for(const auto &Achievement : Stats.Achievements) {

		ae::_Element *Button = new ae::_Element();
		Button->ID = Achievement.ID;
		Button->Parent = AchievementContainer;
		Button->BaseOffset = glm::vec2(-Size.x / 2 - Spacing.x, Offset.y);
		Button->BaseSize = Size;
		Button->Alignment = ae::_Alignment(ae::_Alignment::CENTER, ae::_Alignment::TOP);
		Button->Style = ae::Assets.Styles["style_menu_window"];
		Button->DisabledStyle = ae::Assets.Styles["style_menu_window_disabled"];
		Button->Index = Index++;
		if(Column & 1)
			Button->BaseOffset.x = -Button->BaseOffset.x;

		ae::_Element *Title = new ae::_Element();
		Title->Parent = Button;
		Title->Text = Achievement.Name;
		Title->BaseOffset = glm::vec2(20, 40);
		Title->Alignment = ae::LEFT_BASELINE;
		Title->Font = ae::Assets.Fonts["hud_medium"];
		Title->Format = true;
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
		Text->Format = true;
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
	ShowDefaultCursor(true);

	ae::Assets.Elements["label_game_version"]->Text = std::string("pre") + GAME_VERSION + BuildVersion;
	ae::Assets.Elements["label_game_version"]->SetActive(true);

	if(!Achievements.Enabled) {
		ae::Assets.Elements["button_menu_title_achievements"]->SetEnabled(false);
		ae::Assets.Elements["button_menu_ingame_achievements"]->SetEnabled(false);
	}

	Background = ae::Assets.Elements["image_menu_bg"];
	HandleResize();

	PlayState.Player = nullptr;

	State = STATE_TITLE;
}

// Init single player
void _Menu::InitSinglePlayer() {
	ChangeLayout("element_menu_singleplayer");

	Save.LoadSaves();
	RefreshSaveSlots();
	for(int i = 0; i < SAVE_SLOTS; i++)
		SaveSlots[i]->Checked = false;

	ae::Assets.Elements["button_menu_singleplayer_play"]->SetEnabled(true);
	ae::Assets.Elements["button_menu_singleplayer_delete"]->SetEnabled(false);

	SelectedColor = 0;
	SelectedSlot = -1;

	SinglePlayerState = SINGLEPLAYER_NONE;
	State = STATE_SINGLEPLAYER;
}

// Options
void _Menu::InitOptions() {
	ChangeLayout("element_menu_options");

	// Update text values
	Stats.CreateTransformedText();

	// Set up MSAA values
	int MaxSamples = std::min(4, ae::Graphics.MaxSamples);

	// Keep custom MSAA value
	MaxSamples = std::max(MaxSamples, Config.MSAA);
	MSAAValues.clear();
	MSAAValues.push_back(0);
	int Samples = 2;
	while(Samples <= MaxSamples) {
		MSAAValues.push_back(Samples);
		Samples *= 2;
	}

	// Set up anisotropy values
	LastAnisotropy = Config.Anisotropy;
	AnisotropyValues.clear();
	int Anisotropy = 1;
	while(Anisotropy <= ae::Graphics.MaxAnisotropy) {
		AnisotropyValues.push_back(Anisotropy);
		Anisotropy *= 2;
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

	bool Warn = PlayState.Player && PlayState.Player->InCombat();
	ae::Assets.Elements["label_menu_ingame_exitwarning"]->SetActive(Warn);
	ae::Assets.Elements["label_menu_ingame_mainmenu_text"]->Text = "Main Menu";
	if(Warn && !PlayState.TestMode) {
		ae::Assets.Elements["button_menu_ingame_mainmenu"]->SetEnabled(false);
		WarnTimer = MENU_WARN_TIME;
	}
	else {
		ae::Assets.Elements["button_menu_ingame_mainmenu"]->SetEnabled(true);
		WarnTimer = 0.0;
	}

	ShowDefaultCursor(true);
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

	ShowDefaultCursor(true);

	Background = ae::Assets.Elements["image_menu_bg"];
	HandleResize();

	State = STATE_SCORE;
}

// Init achievements screen
void _Menu::InitAchievements() {
	ChangeLayout("element_menu_achievements");

	// Set enabled state
	ae::_Element *AchievementContainer = ae::Assets.Elements["element_menu_achievements_container"];
	for(const auto &Child : AchievementContainer->Children) {
		Child->SetEnabled(Achievements.Stats.find(Child->ID) != Achievements.Stats.end());

		// Check for failed achievements
		Child->Children[0]->Text = Stats.Achievements[(size_t)Child->Index].Name;
		if(!Child->Enabled && PlayState.Player) {
			bool Failed = false;
			if(Child->ID == "all" && !PlayState.Player->Stat100Percent)
				Failed = true;
			else if(Child->ID == "lonewolf" && !PlayState.Player->StatLoneWolf)
				Failed = true;
			else if(Child->ID == "fists" && !PlayState.Player->StatFistsOnly)
				Failed = true;
			else if(Child->ID == "smoked" && PlayState.Player->LavaTouches)
				Failed = true;
			else if(Child->ID == "p10" && !PlayState.Player->Hardcore)
				Failed = true;
			else if(Child->ID == "bleedrun" && (PlayState.Player->Progression > 1 || PlayState.Player->TotalDeaths || !PlayState.Player->Hardcore || PlayState.Player->PlayTime >= ACHIEVEMENTS_BLEEDRUN_TIME))
				Failed = true;

			if(Failed)
				Child->Children[0]->Text += "  [c red]FAILED";
		}
	}

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
	for(int i = 0; i < PLAYER_COLOR_COUNT; i++) {
		std::ostringstream Buffer;
		Buffer << PlayerColorButtonPrefix << i;

		ColorButtons[i] = ae::Assets.Elements[Buffer.str()];
		ColorButtons[i]->Checked = false;
		ColorButtons[i]->Index = i;
	}

	ae::_Element *Check = ae::Assets.Elements["label_menu_new_hardcore_check"];
	Check->Text = "";

	SelectedColor = 0;
	ColorButtons[SelectedColor]->Checked = true;

	SinglePlayerState = SINGLEPLAYER_NEW_PLAYER;
}

// Play the game
void _Menu::LaunchGame() {
	_Player *Player = Save.GetPlayer((size_t)SelectedSlot);
	if(Player->Hardcore && Player->Health <= 0)
		return;

	PlayState.Player = Player;
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
		for(size_t i = 0; i < AnisotropyValues.size(); i++) {
			if(Config.Anisotropy == AnisotropyValues[i]) {
				Offset = i * (1.0f / (AnisotropyValues.size() - 1));
				break;
			}
		}

		ae::Assets.Elements["button_menu_options_anisotropy"]->SetOffsetPercent(glm::vec2(Offset, 0));
		ae::Assets.Elements["label_menu_options_anisotropy_value"]->Text = std::to_string(Config.Anisotropy);
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
		size_t Index = std::clamp((size_t)(MSAAValues.size() * MSAAButton->GetOffsetPercent().x), (size_t)0, MSAAValues.size() - 1);

		Config.MSAA = MSAAValues[Index];

		std::ostringstream Buffer;
		Buffer << std::fixed << Config.MSAA;
		MSAAValue->Text = Buffer.str();
		Buffer.str("");
	}
}

// Update anisotropic filtering slider
void _Menu::UpdateAnisotropy() {
	ae::_Element *AnisotropySlider = ae::Assets.Elements["element_menu_options_anisotropy"];
	ae::_Element *AnisotropyValue = ae::Assets.Elements["label_menu_options_anisotropy_value"];
	ae::_Element *AnisotropyButton = ae::Assets.Elements["button_menu_options_anisotropy"];

	// Handle clicking inside slider elements
	if(!AnisotropyButton->PressedElement && AnisotropySlider->PressedElement) {
		AnisotropyButton->PressedOffset = AnisotropyButton->Size / 2.0f;
		AnisotropyButton->PressedElement = AnisotropyButton;
	}

	// Update value
	if(AnisotropyButton->PressedElement) {
		size_t Index = std::clamp((size_t)(AnisotropyValues.size() * AnisotropyButton->GetOffsetPercent().x), (size_t)0, AnisotropyValues.size() - 1);

		Config.Anisotropy = AnisotropyValues[Index];
		if(LastAnisotropy != Config.Anisotropy) {
			UpdateTextures();
			LastAnisotropy = Config.Anisotropy;
		}

		std::ostringstream Buffer;
		Buffer << std::fixed << Config.Anisotropy;
		AnisotropyValue->Text = Buffer.str();
		Buffer.str("");
	}
}

// Update filtering for textures
void _Menu::UpdateTextures() {
	for(const auto &Texture : ae::Assets.Textures) {
		if(Texture.second && Texture.second->Mipmaps)
			Texture.second->UpdateAnisotropicFiltering(Config.Anisotropy);
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
							for(int i = 0; i < SAVE_SLOTS; i++) {
								if(Save.GetPlayer((size_t)i)) {
									SelectedSlot = i;
									break;
								}
							}
						}

						if(SelectedSlot >= 0 && Save.GetPlayer((size_t)SelectedSlot))
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
				if(Clicked->ID == "button_menu_title_single") {
					InitSinglePlayer();
				}
				else if(Clicked->ID == "button_menu_title_options") {
					InitOptions();
				}
				else if(Clicked->ID == "button_menu_title_achievements") {
					InitAchievements();
				}
				else if(Clicked->ID == "button_menu_title_exit") {
					Framework.Done = true;
				}
			} break;
			case STATE_SINGLEPLAYER: {
				switch(SinglePlayerState) {
					case SINGLEPLAYER_NONE:
						if(Clicked->ID == "button_menu_singleplayer_delete") {
							if(SelectedSlot != -1) {
								CurrentLayout->SetClickable(false);
								SinglePlayerState = SINGLEPLAYER_DELETE;
								ConfirmAction();
							}
						}
						else if(Clicked->ID == "button_menu_singleplayer_play") {
							if(SelectedSlot != -1 && Save.GetPlayer((size_t)SelectedSlot)) {
								LaunchGame();
							}
						}
						else if(Clicked->ID == "button_menu_singleplayer_back") {
							InitTitle();
						}
						else if(Clicked->ID.substr(0, PlayerButtonPrefix.size()) == PlayerButtonPrefix) {

							// Deselect previous slot
							if(SelectedSlot != -1)
								SaveSlots[SelectedSlot]->Checked = false;

							// Set up create player screen
							_Player *Player = Save.GetPlayer((size_t)Clicked->Index);
							if(Player) {
								ae::Assets.Elements["button_menu_singleplayer_play"]->SetEnabled(!(Player->Hardcore && Player->Health <= 0));
								ae::Assets.Elements["button_menu_singleplayer_delete"]->SetEnabled(true);
							}
							else
								InitNewPlayer();

							SelectedSlot = Clicked->Index;
							SaveSlots[SelectedSlot]->Checked = true;

							if(DoubleClick)
								LaunchGame();
						}
					break;
					case SINGLEPLAYER_NEW_PLAYER:
						if(Clicked->ID.substr(0, PlayerColorButtonPrefix.size()) == PlayerColorButtonPrefix) {
							if(SelectedColor != -1)
								ColorButtons[SelectedColor]->Checked = false;

							SelectedColor = Clicked->Index;
							ColorButtons[SelectedColor]->Checked = true;
						}
						else if(Clicked->ID == "button_menu_new_hardcore") {
							ae::_Element *Check = ae::Assets.Elements["label_menu_new_hardcore_check"];
							Check->Text = Check->Text == "" ? "X" : "";
						}
						else if(Clicked->ID == "button_menu_new_create") {
							CreatePlayer();
						}
						else if(Clicked->ID == "button_menu_new_cancel") {
							SinglePlayerCancel();
						}
					break;
					case SINGLEPLAYER_DELETE:
						if(Clicked->ID == "button_confirm_ok") {
							if(SelectedSlot != -1) {
								Save.DeletePlayer((size_t)SelectedSlot);
								InitSinglePlayer();
							}
						}
						else if(Clicked->ID == "button_confirm_cancel") {
							SinglePlayerCancel();
						}
					break;
				}
			} break;
			case STATE_OPTIONS: {
				if(OptionsState == OPTION_NONE) {
					if(Clicked->ID == "button_menu_options_fullscreen") {
						SetFullscreen(!Config.Fullscreen);
						UpdateOptions();
					}
					else if(Clicked->ID == "button_menu_options_gunflashes") {
						Config.WeaponFlashes = !Config.WeaponFlashes;
						UpdateOptions();
					}
					else if(Clicked->ID == "button_menu_options_walldecals") {
						Config.WallDecals = !Config.WallDecals;
						UpdateOptions();
					}
					else if(Clicked->ID == "button_menu_options_floordecals") {
						Config.FloorDecals = !Config.FloorDecals;
						UpdateOptions();
					}
					else if(Clicked->ID == "button_menu_options_autoequip") {
						Config.AutoEquip = !Config.AutoEquip;
						UpdateOptions();
					}
					else if(Clicked->ID == "button_menu_options_tutorial") {
						Config.Tutorial = !Config.Tutorial;
						UpdateOptions();
					}
					else if(Clicked->ID == "button_menu_options_controls") {
						InitControls();
					}
					else if(Clicked->ID == "button_menu_options_defaults") {
						Config.SetDefaults(true);
						ae::Audio.SetSoundVolume(Config.SoundVolume);
						UpdateOptions();
					}
					else if(Clicked->ID == "button_menu_options_save") {
						Config.Save();
						if(Framework.GetState() == &PlayState)
							InitInGame();
						else
							InitTitle();
					}
					else if(Clicked->ID == "button_menu_options_cancel") {
						Config.Load();
						UpdateTextures();
						if(Framework.GetState() == &PlayState)
							InitInGame();
						else
							InitTitle();
					}
					else if(Clicked->ID.substr(0, InputBoxPrefix.size()) == InputBoxPrefix) {
						OptionsState = OPTION_ACCEPT_INPUT;
						CurrentAction = Clicked->Index;
						ae::Assets.Elements["label_menu_controls_accept_text_action"]->Text = Clicked->Children.front()->Text;
					}
				}
			} break;
			case STATE_CONTROLS: {
				if(Clicked->ID == "button_menu_controls_defaults") {
					Config.LoadDefaultInputBindings(false);
					RefreshInputLabels();
				}
				else if(Clicked->ID == "button_menu_controls_save") {
					Config.Save();
					InitOptions();
				}
				else if(Clicked->ID == "button_menu_controls_cancel") {
					Config.Load();
					InitOptions();
				}
				else if(Clicked->ID.substr(0, InputBoxPrefix.size()) == InputBoxPrefix) {
					OptionsState = OPTION_ACCEPT_INPUT;
					CurrentAction = Clicked->Index;
					ae::Assets.Elements["label_menu_controls_accept_text_action"]->Text = Clicked->Children.front()->Text;
				}
			} break;
			case STATE_ACHIEVEMENTS: {
				if(Clicked->ID == "button_menu_achievements_back") {
					if(Framework.GetState() == &PlayState)
						InitInGame();
					else
						InitTitle();
				}
			} break;
			case STATE_INGAME: {
				if(Clicked->ID == "button_menu_ingame_resume") {
					InitPlay();
				}
				else if(Clicked->ID == "button_menu_ingame_options") {
					InitOptions();
				}
				else if(Clicked->ID == "button_menu_ingame_achievements") {
					InitAchievements();
				}
				else if(Clicked->ID == "button_menu_ingame_mainmenu") {
					Framework.ChangeState(&NullState);
				}
			} break;
			case STATE_SCORE: {
				if(Clicked->ID == "button_menu_score_continue") {
					InitPlay();
					Framework.ChangeState(&PlayState);
				}
				else if(Clicked->ID == "button_menu_score_mainmenu") {
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

	if(Framework.Console)
		Framework.Console->UpdateSize();

	// Reload fonts
	ae::Assets.LoadFonts("ui/fonts.tsv");
	ae::Graphics.ResetState();
}

// Show menu cursor or game cursor
void _Menu::ShowDefaultCursor(bool Value) {
	if(Value)
		ae::Graphics.SetCursor(ae::CURSOR_MAIN);
	else
		ae::Graphics.SetCursor(ae::Assets.Cursors["game"]);
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
			for(size_t i = 0; i < SAVE_SLOTS; i++) {
				_Player *Player = Save.GetPlayer(i);
				if(Player) {
					Player->PositionChanged = true;
					Player->MoveState = MOVE_FORWARD;
					Player->UpdateAnimation(FrameTime, false);
				}
			}
		} break;
		case STATE_OPTIONS:
			UpdateVolume();
			UpdateMSAA();
			UpdateAnisotropy();
		break;
		case STATE_INGAME:
			if(WarnTimer > 0.0) {
				std::ostringstream Buffer;
				Buffer << "Main Menu " << std::ceil(WarnTimer);
				ae::Assets.Elements["label_menu_ingame_mainmenu_text"]->Text = Buffer.str();

				WarnTimer -= FrameTime;
				if(WarnTimer <= 0) {
					WarnTimer = 0.0;
					ae::Assets.Elements["label_menu_ingame_mainmenu_text"]->Text = "Main Menu";
					ae::Assets.Elements["button_menu_ingame_mainmenu"]->SetEnabled(true);
				}
			}
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
			for(size_t i = 0; i < SAVE_SLOTS; i++) {
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
		Achievements.Stats[ID].Version = BUILD_VERSION;
		Achievements.Stats[ID].Time = std::time(nullptr);
		Achievements.Stats[ID].Value = 1;
		Achievements.Save();

		return;
	}
}

// Update score screen label values
void _Menu::SetScoreStats(const _HUD *HUD, const _Player *Player, bool EndOfGame, int Progression, bool GotOneHundredPercent) {
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

	// Set level time
	char TimeBuffer[256];
	_HUD::FormatTime(TimeBuffer, Player->LevelTime);
	ae::Assets.Elements["label_menu_score_time_value"]->Text = TimeBuffer;

	// Set level kills
	Buffer << HUD->Kills[0] << "/" << HUD->Kills[1];
	ae::Assets.Elements["label_menu_score_kills_value"]->Text = Buffer.str();
	Buffer.str("");

	// Set level crates
	Buffer << HUD->Crates[0] << "/" << HUD->Crates[1];
	ae::Assets.Elements["label_menu_score_crates_value"]->Text = Buffer.str();
	Buffer.str("");

	// Set level secrets
	Buffer << HUD->Secrets[0] << "/" << HUD->Secrets[1];
	ae::Assets.Elements["label_menu_score_secrets_value"]->Text = Buffer.str();
	Buffer.str("");

	// Set progression stats
	_HUD::FormatTime(TimeBuffer, Player->ProgressionTime);
	ae::Assets.Elements["label_menu_score_progression_time_value"]->Text = TimeBuffer;
	ae::Assets.Elements["label_menu_score_progression_kills_value"]->Text = std::to_string(Player->ProgressionKills);
	ae::Assets.Elements["label_menu_score_progression_crates_value"]->Text = std::to_string(Player->ProgressionCrates);
	ae::Assets.Elements["label_menu_score_progression_secrets_value"]->Text = std::to_string(Player->ProgressionSecrets);

	// Set completion text
	ae::Assets.Elements["label_menu_score_completion"]->Color.a = GotOneHundredPercent ? 1.0f : 0.0f;
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
	for(size_t i = 0; i < SAVE_SLOTS; i++) {
		std::ostringstream Buffer;
		Buffer << "label_menu_singleplayer_slot" << i << "_text";
		ae::_Element *SlotLabel = ae::Assets.Elements[Buffer.str()];
		Buffer.str("");

		Buffer << "label_menu_singleplayer_slot" << i << "_hardcore";
		ae::_Element *SlotHardcore = ae::Assets.Elements[Buffer.str()];
		Buffer.str("");

		_Player *Player = Save.GetPlayer(i);
		if(Player) {
			Player->UpdateSpeed(1.0f);
			Player->SetLegAnimationPlayMode(ae::_Animation::PLAYING);
			Player->Animation->Play(0);
			SlotLabel->Text = Player->Name;
			if(Player->Progression > 1)
				SlotLabel->Text += "  [c gold]P" + std::to_string(Player->Progression) + "[c white]";
			SlotHardcore->Text = Player->Hardcore ? (Player->Health == 0 ? "Dead" : "Hardcore") : "";
		}
		else {
			SlotLabel->Text = "Empty Slot";
			SlotHardcore->Text = "";
		}

		Buffer << PlayerButtonPrefix << i;
		SaveSlots[i] = ae::Assets.Elements[Buffer.str()];
		SaveSlots[i]->Index = (int)i;
	}
}

// Refreshes the input map labels
void _Menu::RefreshInputLabels() {
	for(size_t i = 0; i < LABEL_COUNT; i++) {
		InputLabels[i] = ae::Assets.Elements[KEYLABELS[i]];
		InputLabels[i]->Text = ae::Actions.GetInputNameForAction(i);
		InputLabels[i]->Parent->Index = (int)i;
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

	if(SelectedSlot == -1)
		return;

	ae::_Element *Check = ae::Assets.Elements["label_menu_new_hardcore_check"];

	Save.CreateNewPlayer((size_t)SelectedSlot, ae::Assets.Elements["textbox_menu_new_name_input"]->Text, COLORS[SelectedColor], Check->Text != "");
	ae::Assets.Elements["button_menu_singleplayer_play"]->SetEnabled(true);
	ae::Assets.Elements["button_menu_singleplayer_delete"]->SetEnabled(true);

	RefreshSaveSlots();
	ae::FocusedElement = nullptr;
}

// Clear action on keybinding page
void _Menu::ClearAction(size_t Action, int Type) {
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
		ae::Actions.ClearMappingForInputAction(InputType, Input, (size_t)Action);

	// Clear out existing action
	ClearAction((size_t)CurrentAction, 0);

	// Add new binding
	ae::Actions.AddInputMap(0, InputType, Input, (size_t)CurrentAction, 1.0f, -1.0f, false);

	// Update menu labels
	RefreshInputLabels();
}

// Show the confirm screen
void _Menu::ConfirmAction() {
	CurrentLayout = ae::Assets.Elements["element_confirm"];
	ae::Assets.Elements["label_confirm_warning"]->Text = "Are you sure?";

	CurrentLayout->SetActive(true);
}
