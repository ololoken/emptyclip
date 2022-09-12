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
#include <states/play.h>
#include <states/editor.h>
#include <states/null.h>
#include <objects/entity.h>
#include <objects/player.h>
#include <objects/monster.h>
#include <objects/particle.h>
#include <ae/actions.h>
#include <ae/camera.h>
#include <ae/graphics.h>
#include <ae/graphics.h>
#include <ae/assets.h>
#include <ae/program.h>
#include <ae/console.h>
#include <ae/light.h>
#include <ae/audio.h>
#include <ae/font.h>
#include <ae/util.h>
#include <ae/random.h>
#include <ae/framebuffer.h>
#include <achievements.h>
#include <objectmanager.h>
#include <framework.h>
#include <menu.h>
#include <constants.h>
#include <gameassets.h>
#include <hud.h>
#include <map.h>
#include <events.h>
#include <config.h>
#include <particles.h>
#include <stats.h>
#include <actiontype.h>
#include <stdexcept>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

_PlayState PlayState;

// Load level and set up objects
void _PlayState::Init() {
	ae::Graphics.SetViewport(ae::Graphics.CurrentSize);
	ae::Graphics.Element->SetActive(false);
	ae::Graphics.Element->Active = true;

	WeaponsUsed.clear();
	CursorItem = nullptr;
	PreviousCursorItem = nullptr;

	// Check for player
	if(TestMode) {
		Player = new _Player(Stats.Objects.at("player"));
		Player->SavePath = Config.ConfigPath + "test.save";
		Save.LoadPlayer(Player);
	}

	// Bad player
	if(!Player)
		throw std::runtime_error("Player is nullptr");

	// Set checkpoint from editor
	if(FromEditor)
		Player->CheckpointIndex = CheckpointIndex;

	// Check for level override
	if(Level.empty())
		Level = Player->MapID;

	// Create framebuffer for mixing lights
	Framebuffer = new ae::_Framebuffer(ae::Graphics.CurrentSize);

	// Load level
	Map = new _Map(Level, Player->Clock, Player->Progression);
	Map->InitializeTiles();
	Player->Map = Map;
	Player->MapID = Map->Filename;

	// Set starting states
	Player->SetPosition(Map->GetStartingPositionByCheckpoint(Player->CheckpointIndex));
	Player->TileChanged = true;
	Map->AddObjectToGrid(Player, GRID_PLAYER);

	// Spawn objects
	for(const auto &ObjectSpawn : Map->ObjectSpawns)
		SpawnObject(ObjectSpawn, false, Map->GetAddedLevel());

	// Initialize camera
	ae::_CameraSettings CameraSettings;
	CameraSettings.UpdateDivisor = CAMERA_DIVISOR;
	CameraSettings.SnappingThreshold = CAMERA_SNAPPING_THRESHOLD;
	Camera = new ae::_Camera(CameraSettings);
	Camera->CalculateFrustum(ae::Graphics.AspectRatio);
	Camera->ForcePosition(glm::vec3(Player->Position, CAMERA_DISTANCE));
	Map->Camera = Camera;

	// Initialize particles
	Particles = new _Particles();
	Particles->Camera = Camera;
	Particles->Map = Map;

	// Initialize HUD
	HUD = new _HUD(Camera, Player);
	HUD->SetStats(Map->Monsters, Map->Crates, Map->Secrets);
	if(Player->CheckpointIndex == 0 && !Map->Name.empty())
		HUD->ShowLevelName(Map->Name, UI_LEVELNAME_TIME);

	Camera->ConvertScreenToWorld(ae::Input.GetMouse(), WorldCursor);
	PreviousWorldCursor = WorldCursor;
	ae::Graphics.SetCursor(false);

	ae::Actions.ResetState();
	ae::Audio.Stop();

	// Print stats
	if(DevMode)
		Framework.Console->AddMessage("TotalExperience=" + std::to_string(Map->TotalExperience));

	ActiveAI = 0;
	Timer = 0;
}

// Close map
void _PlayState::Close() {
	Save.SavePlayer(Player);

	DeleteMonsters();
	ActiveEvents.clear();

	Player->StopAudio();

	if(TestMode) {
		delete Player;
		Player = nullptr;
	}

	delete Particles;
	delete Camera;
	delete Map;
	delete HUD;
	delete Framebuffer;
}

// Action handler
bool _PlayState::HandleAction(int InputType, size_t Action, int Value) {
	if(Value == 0)
		return false;

	// Handle console toggling
	if(Action == Action::MISC_CONSOLE) {
		Framework.Console->Toggle();
		Framework.IgnoreNextInputEvent = true;
	}

	// Ignore actions when console is open
	if(Framework.Console->IsOpen())
		return false;

	// Handle console toggling
	if(Action == Action::MISC_DEBUG) {
		DebugMode = !DebugMode;
		return false;
	}

	if(!Player || IsPaused())
		return false;

	if(!Player->IsDying()) {
		switch(Action) {
			case Action::GAME_INVENTORY:
				HUD->SetInventoryOpen(!HUD->InventoryOpen);
				Player->SetAiming(false);
				Player->SetSprinting(false);
			break;
			case Action::GAME_SORTINVENTORY:
				if(HUD->InventoryOpen)
					Player->SortInventory();
			break;
			case Action::GAME_FIRE:
				if(!HUD->InventoryOpen && !Player->IsMeleeAttacking()) {

					// Use melee weapon if player has no main hand
					int AttackType = WEAPONATTACK_MAIN;
					if(!Player->HasMainHand())
						AttackType = WEAPONATTACK_MELEE;

					// Can reload
					if(Player->Reloading && Player->WeaponHasAmmo(AttackType))
						Player->CancelReloading();

					// Play sound
					if(Player->CanAttack(AttackType) && !Player->WeaponHasAmmo(AttackType))
						ae::Audio.PlaySound(Player->GetSound(SOUND_EMPTY, AttackType));

					if(Player->CheckAttackTimer(AttackType) && Player->FireRateType[AttackType] == FIRERATE_SEMI && (!Player->BurstRounds[AttackType] || (Player->BurstRounds[AttackType] && Player->BurstRoundsShot == 0))) {
						Player->BurstRoundsShot = 0;
						Player->RequestAttack(AttackType);
					}
				}
			break;
			case Action::GAME_MELEE:
				if(!HUD->InventoryOpen) {
					if(Player->Reloading)
						Player->CancelReloading();

					if(Player->FireRateType[WEAPONATTACK_MELEE] == FIRERATE_SEMI)
						Player->RequestAttack(WEAPONATTACK_MELEE);
				}
			break;
			case Action::GAME_RELOAD:
				if(!HUD->IsDragging())
					Player->StartReloading();
			break;
			case Action::GAME_WEAPONSWITCH:
				if(!HUD->IsDragging())
					Player->StartWeaponSwitch(INVENTORY_MAINHAND, INVENTORY_OFFHAND);
			break;
			case Action::GAME_FLASHLIGHT:
				Player->Flashlight = !Player->Flashlight;
				ae::Audio.PlaySound(ae::Assets.Sounds["game_flashlight0.ogg"]);
			break;
		}
	}
	else {
		if(Action == Action::GAME_USE)
			Player->Respawn();
	}

	return false;
}

// Key handler
bool _PlayState::HandleKey(const ae::_KeyEvent &KeyEvent) {
	bool Handled = ae::Graphics.Element->HandleKey(KeyEvent);

	bool SendAction = true;
	if(IsPaused()) {
		Player->StopAudio();
		if(!Handled)
			SendAction = Menu.HandleKey(KeyEvent);

		return SendAction;
	}

	if(KeyEvent.Pressed) {
		switch(KeyEvent.Scancode) {
			case SDL_SCANCODE_ESCAPE:
				if(Player->IsDead()) {
					Player->Respawn();
					Save.SavePlayer(Player);
				}
				else if(!Player->IsDying()) {
					if(HUD->InventoryOpen) {
						HUD->SetInventoryOpen(false);
					}
					else if(TestMode) {
						if(FromEditor)
							Framework.ChangeState(&EditorState);
						else
							Framework.Done = true;
					}
					else {
						Save.SavePlayer(Player);
						Menu.InitInGame();
					}
				}
			break;
			case SDL_SCANCODE_F1:
				HUD->SetInventoryOpen(false);
				Save.SavePlayer(Player);
				Menu.InitInGame();
			break;
		}
	}

	return SendAction;
}

// Mouse handler
void _PlayState::HandleMouseButton(const ae::_MouseEvent &MouseEvent) {
	HUD->MouseEvent(MouseEvent);

	if(IsPaused())
		Menu.HandleMouseButton(MouseEvent);
}

// Handle console commands
bool _PlayState::HandleCommand(ae::_Console *Console) {

	// Get parameters
	std::vector<std::string> Parameters;
	ae::TokenizeString(Console->Parameters, Parameters);

	// Handle normal commands
	if(Console->Command == "quit") {
		HandleQuit();
		return true;
	}
	else if(Console->Command == "suicide") {
		if(Player) {
			Player->UpdateHealth(-10000000);
			ae::Audio.PlaySound(ae::Assets.Sounds["player_die0.ogg"], ae::_SoundSettings(glm::vec3(Player->Position.x, 0.0f, Player->Position.y), 1.0f, AUDIO_REFERENCE_DISTANCE, AUDIO_MAX_DISTANCE, AUDIO_ROLL_OFF));
		}

		return true;
	}

	// Handle dev commands
	if(DevMode) {
		if(Console->Command == "ammo") {
			if(!Player)
				return true;

			for(const auto &Ammo : Stats.AmmoNames)
				Player->Ammo[Ammo] = Player->AmmoMax[Ammo];

			Console->AddMessage("ammo replenished");
		}
		else if(Console->Command == "clock") {
			if(!Player)
				return true;

			if(Parameters.size() == 1)
				Player->Clock = std::clamp(ae::ToNumber<double>(Parameters[0]), 0.0, MAP_DAY_LENGTH);
			else
				Console->AddMessage("clock = " + std::to_string(Player->Clock));
		}
		else if(Console->Command == "experience") {
			if(!Player)
				return true;

			if(Parameters.size() == 1) {
				bool Adjust = false;
				if(Parameters[0][0] == '+' || Parameters[0][0] == '-')
					Adjust = true;

				int64_t Change = ae::ToNumber<int64_t>(Parameters[0]);
				Player->Experience = std::max((int64_t)0, Adjust ? Player->Experience + Change : Change);
				Player->RecalculateStats();
			}
			else
				Console->AddMessage("usage: " + Console->Command + " [+-][amount]");
		}
		else if(Console->Command == "god") {
			GodMode = !GodMode;
			Console->AddMessage("god = " + std::to_string(GodMode));
		}
		else if(Console->Command == "health") {
			if(!Player)
				return true;

			if(Parameters.size() == 1) {
				bool Adjust = false;
				if(Parameters[0][0] == '+' || Parameters[0][0] == '-')
					Adjust = true;

				int64_t Change = ae::ToNumber<int64_t>(Parameters[0]);
				Player->Health = std::max((int64_t)0, Adjust ? Player->Health + Change : Change);
				Player->UpdateHealth(0);
				Player->RecalculateStats();
			}
			else
				Console->AddMessage("usage: " + Console->Command + " [+-][amount]");
		}
		else if(Console->Command == "keys") {
			if(!Player)
				return true;

			Player->Keys["key_red"] = 1;
			Player->Keys["key_green"] = 1;
			Player->Keys["key_blue"] = 1;
			Player->Keys["key_boss"] = 1;

			Console->AddMessage("keys acquired");
		}
		else if(Console->Command == "progression") {
			if(!Player)
				return true;

			if(Parameters.size() == 1) {
				Player->Progression = std::clamp(ae::ToNumber<int>(Parameters[0]), 0, GAME_MAX_PROGRESSION);
				Player->ProgressionTime = 0;
				Player->ProgressionKills = 0;
				Player->ProgressionCrates = 0;
				Player->ProgressionSecrets = 0;
				Player->ProgressionDeaths = 0;
				Player->RecalculateStats();
			}
			else
				Console->AddMessage("usage: " + Console->Command + " [level]");
		}
		else if(Console->Command == "reset") {
			if(!Player)
				return true;

			if(Parameters.size() == 1) {
				if(Parameters[0] == "skills") {
					for(int i = 0; i < SKILL_COUNT; i++)
						Player->Skills[i] = 0;

					Player->RecalculateStats();
					Console->AddMessage("skills reset");
				}
				else if(Parameters[0] == "keys") {
					Player->Keys.clear();
					Console->AddMessage("keys reset");
				}
			}
			else {
				Player->Reset(true);
				Console->AddMessage("player reset");
			}
		}
		else
			return false;
	}
	else
		return false;

	return true;
}

// Window size updates
void _PlayState::HandleWindow(uint8_t Event) {
	if(Event == SDL_WINDOWEVENT_SIZE_CHANGED) {
		if(Camera)
			Camera->CalculateFrustum(ae::Graphics.AspectRatio);

		Menu.HandleResize();

		if(Framebuffer) {
			Framebuffer->Resize(ae::Graphics.CurrentSize);
			ae::Graphics.ResetState();
		}
	}
}

// Handle quit events
void _PlayState::HandleQuit() {
	Framework.Done = true;
}

// Update
void _PlayState::Update(double FrameTime) {
	ae::Graphics.Element->Update(FrameTime, ae::Input.GetMouse());
	//if(ae::Graphics.Element->HitElement)
	//	std::cout << ae::Graphics.Element->HitElement->Name << std::endl;

	int OldLevel = Player->Level;
	Timer += FrameTime;
	FlashTimer = std::max(0.0, FlashTimer - FrameTime);
	Menu.Update(FrameTime);

	// Handle pause
	if(IsPaused()) {
		ae::Graphics.SetCursor(true);
		HUD->CursorOverItem = nullptr;
		HUD->CursorOverWorld = false;

		return;
	}

	// Get world cursor
	PreviousWorldCursor = WorldCursor;
	Camera->ConvertScreenToWorld(ae::Input.GetMouse(), WorldCursor);

	// Handle input
	if(!Player->IsDying() && ae::FocusedElement == nullptr) {

		// Turn character to face the world cursor
		if(Player->Action != ACTION_MELEE)
			Player->FacePosition(WorldCursor);

		// Move types
		if(ae::Actions.State[Action::GAME_UP].Value > 0.0f && ae::Actions.State[Action::GAME_LEFT].Value > 0.0f)
			Player->MoveState = MOVE_FORWARDLEFT;
		else if(ae::Actions.State[Action::GAME_UP].Value > 0.0f && ae::Actions.State[Action::GAME_RIGHT].Value > 0.0f)
			Player->MoveState = MOVE_FORWARDRIGHT;
		else if(ae::Actions.State[Action::GAME_DOWN].Value > 0.0f && ae::Actions.State[Action::GAME_LEFT].Value > 0.0f)
			Player->MoveState = MOVE_BACKWARDLEFT;
		else if(ae::Actions.State[Action::GAME_DOWN].Value > 0.0f && ae::Actions.State[Action::GAME_RIGHT].Value > 0.0f)
			Player->MoveState = MOVE_BACKWARDRIGHT;
		else if(ae::Actions.State[Action::GAME_LEFT].Value > 0.0f)
			Player->MoveState = MOVE_LEFT;
		else if(ae::Actions.State[Action::GAME_RIGHT].Value > 0.0f)
			Player->MoveState = MOVE_RIGHT;
		else if(ae::Actions.State[Action::GAME_UP].Value > 0.0f)
			Player->MoveState = MOVE_FORWARD;
		else if(ae::Actions.State[Action::GAME_DOWN].Value > 0.0f)
			Player->MoveState = MOVE_BACKWARD;
		else
			Player->MoveState = MOVE_NONE;

		// Attack or aim
		if(!HUD->InventoryOpen) {

			// Use melee weapon if player has no main hand
			int AttackType = WEAPONATTACK_MAIN;
			if(!Player->HasMainHand())
				AttackType = WEAPONATTACK_MELEE;

			// Check holding down fire button to attack
			if(!Player->IsMeleeAttacking() && Player->FireRateType[AttackType] == FIRERATE_AUTO && ae::Actions.State[Action::GAME_FIRE].Value > 0.0f)
				Player->RequestAttack(AttackType);

			// Check holding down melee button to attack
			else if(!Player->IsMeleeAttacking() && Player->FireRateType[WEAPONATTACK_MELEE] == FIRERATE_AUTO && ae::Actions.State[Action::GAME_MELEE].Value > 0.0f)
				Player->RequestAttack(WEAPONATTACK_MELEE);

			// Aim
			Player->SetAiming(ae::Actions.State[Action::GAME_AIM].Value > 0.0f && !Player->Reloading && !Player->SwitchingWeapons);
			Player->SetSprinting(ae::Actions.State[Action::GAME_SPRINT].Value > 0.0f);
		}

		Player->UseRequested = ae::Actions.State[Action::GAME_USE].Value;
	}
	else
		HUD->SetInventoryOpen(false);

	int PlayerHealth = Player->Health;

	// Update player
	Player->Update(FrameTime);
	if(Player->PositionChanged)
		IgnoreItems.clear();

	if(GodMode)
		Player->Stamina = Player->MaxStamina;

	// Handle gun flashes
	if(Player->Action == ACTION_STARTSHOOT && Player->HasMainHand() && Player->GetMainHand()->Template.Attributes.at("flash").Int)
		FlashTimer = LIGHT_FLASH_TIME;

	// Check for events
	if(Player->TileChanged)
		CheckEvents(Player);

	// Activate events
	if(Player->UseRequested && Player->CanUse()) {
		if(ActivateEvent())
			Player->UseRequested = false;
	}

	// Handle auto and manually picking up items
	HandlePickup();

	// Update objects
	Map->Update(FrameTime, Player->Clock);
	Map->ObjectManager->RenderList[_ObjectManager::RENDER_PLAYER].push_back(Player);

	// Update monsters
	UpdateMonsters(FrameTime);

	// Check for player dying
	if(Player->Health == 0 && PlayerHealth > 0)
		PlayerDied();

	// Update particles
	Particles->Update(FrameTime);

	// Add player to minimap
	_MinimapLayer MinimapLayer;
	MinimapLayer.Color = HUD_MINIMAP_PLAYER_COLOR;
	MinimapLayer.Bounds = glm::vec4(
		Player->Position.x - Player->Scale * 0.25f, Player->Position.y - Player->Scale * 0.25f,
		Player->Position.x + Player->Scale * 0.25f, Player->Position.y + Player->Scale * 0.25f
	);
	Map->MinimapLayers.push_back(MinimapLayer);

	// Update events
	UpdateEvents(FrameTime);

	// Apply the damage
	if(Player->AttackMade)
		ResolveAttack(Player, GRID_MONSTER);

	// Level up screen
	if(Player->Level > OldLevel)
		HUD->ShowTextMessage("LEVEL UP! YOU HAVE UNSPENT SKILL POINTS", 5.0);

	// Update camera
	Camera->Set2DPosition(Player->Position);

	// Get zoom state
	if(Player->Aiming) {
		glm::vec2 CursorVector = WorldCursor - Player->Position;
		if(CursorVector.x != 0 && CursorVector.y != 0) {
			Map->CollisionHits.clear();
			Map->CheckBulletCollisions(Player, glm::normalize(CursorVector), Map->CollisionHits, GRID_MONSTER, true, 1, _Tile::VISION);
			if(Map->CollisionHits.size()) {
				glm::vec2 HitVector = Map->CollisionHits.front().Position - Player->Position;
				if(glm::dot(CursorVector, CursorVector) < glm::dot(HitVector, HitVector))
					Camera->UpdatePosition(CursorVector / Player->ZoomScale);
				else
					Camera->UpdatePosition(HitVector / Player->ZoomScale);
			}
		}

		Camera->SetDistance(CAMERA_DISTANCE_AIMED);
	}
	else
		Camera->SetDistance(CAMERA_DISTANCE);

	Camera->Update(FrameTime);

	// Get item at cursor
	PreviousCursorItem = CursorItem;
	CursorItem = (_Item *)(Map->GetCloseObject(WorldCursor, 0.05f, GRID_ITEM));
	if(CursorItem && CursorItem == PreviousCursorItem)
		CursorItemTimer += FrameTime;
	else
		CursorItemTimer = 0;

	// Update the HUD
	HUD->Update(FrameTime, Player->GetCrosshairRadius(WorldCursor), Player->Clock);

	// Reset timer if player isn't idle
	if(Player->Action != ACTION_IDLE)
		ClosestItemTimer = 0.0;

	// Show item tooltip when standing over item
	if(!HUD->InventoryOpen && ClosestItem && ClosestItem == LastClosestItem && ClosestItem->Type != _Object::AMMO) {
		ClosestItemTimer += FrameTime;
		if(!HUD->CursorOverItem && ClosestItemTimer >= HUD_STANDOVER_TIME) {
			HUD->CursorOverItem = (_Item *)ClosestItem;
			HUD->CursorOverWorld = true;
		}
	}
	else
		ClosestItemTimer = 0.0;

	LastClosestItem = ClosestItem;

	// Set cursor item
	if(!ae::Graphics.Element->HitElement && CursorItem && (!HUD->CursorOverItem || ClosestItem == HUD->CursorOverItem) && (HUD->InventoryOpen || CursorItemTimer > HUD_CURSOR_ITEM_WAIT || ClosestItemTimer >= HUD_STANDOVER_TIME)) {
		HUD->CursorOverItem = CursorItem;
		HUD->CursorOverWorld = false;
	}

	ae::Audio.SetPosition(glm::vec3(Player->Position.x, 10, Player->Position.y));
}

// Render the state
void _PlayState::Render(double BlendFactor) {
	if(IsPaused())
		BlendFactor = 0;

	// Set up programs
	ae::_Program *MapProgram = ae::Assets.Programs["map"];
	ae::_Program *MapNormProgram = ae::Assets.Programs["map_norm"];

	// Get player light
	glm::vec4 PlayerLightColor;
	glm::vec3 LightAttenuantion;
	if(Config.WeaponFlashes && FlashTimer > 0.0) {
		PlayerLightColor = LIGHT_FLASH_COLOR;
		LightAttenuantion = LIGHT_FLASH_ATTENUATION;
	}
	else {
		PlayerLightColor = PLAYER_LIGHT;
		LightAttenuantion = LIGHT_ATTENUATION;
	}

	// Set up lights
	glm::vec3 LightPosition(glm::vec2(Player->Position), 1.0f);
	MapProgram->LightCount = 1;
	MapProgram->Lights[0].Color = PlayerLightColor;
	MapProgram->Lights[0].Position = LightPosition;
	MapProgram->Lights[0].Attenuation = LightAttenuantion;
	MapProgram->AmbientLight = Map->AmbientLight;
	MapNormProgram->LightCount = 1;
	MapNormProgram->Lights[0].Color = PlayerLightColor;
	MapNormProgram->Lights[0].Position = LightPosition;
	MapNormProgram->Lights[0].Attenuation = LightAttenuantion;
	MapNormProgram->AmbientLight = Map->AmbientLight;

	// Setup the viewing matrix
	ae::Graphics.Setup3D();
	Camera->Set3DProjection(BlendFactor);

	// Setup the viewing matrix
	ae::Graphics.SetProgram(MapProgram);
	MapProgram->SetUniformMat4("view_projection_transform", Camera->Transform);
	ae::Graphics.SetProgram(MapNormProgram);
	MapNormProgram->SetUniformMat4("view_projection_transform", Camera->Transform);
	ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
	ae::Assets.Programs["pos"]->SetUniformMat4("view_projection_transform", Camera->Transform);
	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
	ae::Assets.Programs["pos_uv"]->SetUniformMat4("view_projection_transform", Camera->Transform);
	ae::Graphics.SetProgram(ae::Assets.Programs["text"]);
	ae::Assets.Programs["text"]->SetUniformMat4("view_projection_transform", Camera->Transform);

	// Update minimap
	if(ae::Actions.State[Action::GAME_MAP].Value > 0.0f) {
		Map->MinimapCaptureSize = HUD_MINIMAP_FULL_CAPTURE_SIZE;
		Map->MinimapCaptureSize.x *= ae::Graphics.AspectRatio;
	}
	else
		Map->MinimapCaptureSize = HUD_MINIMAP_CAPTURE_SIZE;

	// Add lights
	Framebuffer->Clear();
	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
	ae::Graphics.EnableParticleBlending();
	if(Player->Flashlight) {
		ae::Graphics.SetColor(glm::vec4(1.0f));
		ae::Graphics.DrawSprite(glm::vec3(Player->Position + glm::vec2(5) * Player->GetDirectionVector(), 0), ae::Assets.Textures["textures/lights/flashlight.png"], Player->Rotation, glm::vec2(5, 10));
	}
	Map->ObjectManager->RenderLights(_ObjectManager::RENDER_LIGHTS, BlendFactor);
	ae::Graphics.DisableParticleBlending();
	ae::_Framebuffer::Unbind();

	// Set framebuffer texture
	if(Framebuffer) {
		MapProgram->Use();
		ae::Graphics.SetActiveTexture(1);
		Framebuffer->BindTexture();
		ae::Graphics.SetActiveTexture(0);
		MapNormProgram->Use();
		ae::Graphics.SetActiveTexture(1);
		Framebuffer->BindTexture();
		ae::Graphics.SetActiveTexture(0);
	}

	int BlockRenderCount = 0;
	int ParticleRenderCount = 0;
	int PropRenderCount = 0;

	// Draw the floor
	BlockRenderCount += Map->RenderFloors();

	// Draw floor decals
	ae::Graphics.SetProgram(MapProgram);
	MapProgram->ResetTransform(MapProgram->TextureTransformID);
	MapProgram->ResetTransform(MapProgram->NormalTransformID);
	ae::Graphics.SetDepthMask(false);
	ae::Graphics.SetDepthTest(false);
	if(Config.FloorDecals)
		ParticleRenderCount += Map->RenderParticles(_Particles::FLOOR_DECALS, BlendFactor);

	// Draw walls and props below objects
	BlockRenderCount += Map->RenderWalls(true);
	PropRenderCount += Map->RenderProps();

	// Draw objects
	ae::Graphics.SetProgram(MapProgram);
	MapProgram->ResetTransform(MapProgram->TextureTransformID);
	ae::Graphics.SetDepthMask(false);
	ae::Graphics.SetDepthTest(true);
	Map->ObjectManager->Render(_ObjectManager::RENDER_ITEMS, BlendFactor);
	Map->ObjectManager->Render(_ObjectManager::RENDER_PROJECTILES, BlendFactor);
	Map->ObjectManager->Render(_ObjectManager::RENDER_PLAYER, BlendFactor);
	Map->ObjectManager->Render(_ObjectManager::RENDER_MONSTER, BlendFactor);

	// Draw walls and props again on top of objects
	BlockRenderCount += Map->RenderWalls();
	BlockRenderCount += Map->RenderFlatWalls();
	PropRenderCount += Map->RenderProps();

	// Draw wall decals
	ae::Graphics.SetProgram(MapNormProgram);
	MapProgram->ResetTransform(MapNormProgram->TextureTransformID);
	MapProgram->ResetTransform(MapNormProgram->NormalTransformID);
	ae::Graphics.SetDepthMask(false);
	if(Config.WallDecals)
		ParticleRenderCount += Map->RenderParticles(_Particles::WALL_DECALS, BlendFactor);

	// Draw particles
	ae::Graphics.EnableParticleBlending();
	ae::Graphics.SetProgram(MapProgram);
	MapProgram->ResetTransform(MapProgram->TextureTransformID);
	MapProgram->ResetTransform(MapProgram->NormalTransformID);
	Particles->Render(_Particles::NORMAL, BlendFactor);
	ae::Graphics.DisableParticleBlending();

	// Emissive animating particles
	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
	Particles->Render(_Particles::EMISSIVE_ANIMATION, BlendFactor);

	// Normal emissive particles
	ae::Graphics.EnableParticleBlending();
	ae::Assets.Programs["pos_uv"]->ResetTransform(ae::Assets.Programs["pos_uv"]->TextureTransformID);
	Particles->Render(_Particles::EMISSIVE, BlendFactor);
	ae::Graphics.DisableParticleBlending();

	// Draw the foreground tiles
	BlockRenderCount += Map->RenderForeground(Player->Position, false);

	// Get cursor position
	glm::vec2 CursorDrawPosition = (WorldCursor == PreviousWorldCursor) ? WorldCursor : WorldCursor * (float)BlendFactor + PreviousWorldCursor * (float)(1.0f - BlendFactor);
	if(!Player->IsDying())
		HUD->DrawCrosshair(CursorDrawPosition);

	// Debug
	if(GodMode && DevMode && DebugMode) {
		ae::Graphics.SetDepthTest(false);

		// Draw monster target positions
		ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
		ae::Graphics.SetColor(COLOR_RED);
		for(const auto &Object : Map->ObjectManager->RenderList[_ObjectManager::RENDER_MONSTER]) {
			_Entity *Entity = (_Entity *)Object;
			ae::Graphics.DrawCircle(glm::vec3(Entity->TargetPosition, 0), Entity->TargetRadius);
		}

		// Draw weapon ranges
		glm::vec2 DrawPosition;
		Player->GetDrawPosition(DrawPosition, BlendFactor);
		for(int i = 0; i < WEAPONATTACK_COUNT; i++) {
			glm::vec4 Color = COLOR_WHITE;
			if(i == 1)
				Color = COLOR_GREEN;

			float Range = Player->AttackRange[i];
			if(Range == 0.0f)
				Range = 100.0f;

			ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
			ae::Graphics.SetColor(Color);
			ae::Graphics.DrawCircle(glm::vec3(DrawPosition, 0), Range);

			glm::vec2 Direction = Player->GetDirectionVector();
			glm::vec2 NormalDirection(-Direction.y, Direction.x);

			glm::vec2 LeftLineStart = DrawPosition - NormalDirection * (Player->AttackWidth[i] - Player->MeleeOffset[i]);
			glm::vec2 LeftLineEnd = LeftLineStart + Direction * Range;
			ae::Graphics.DrawLine(LeftLineStart, LeftLineEnd);

			glm::vec2 RightLineStart = DrawPosition + NormalDirection * (Player->AttackWidth[i] + Player->MeleeOffset[i]);
			glm::vec2 RightLineEnd = RightLineStart + Direction * Range;
			ae::Graphics.DrawLine(RightLineStart, RightLineEnd);
			//glm::vec2 LeftLine = Player->Position + Player->GetDirectionVector(-Player->MaxAccuracy[i] * 0.5f) * Range;
			//glm::vec2 RightLine = Player->Position + Player->GetDirectionVector(Player->MaxAccuracy[i] * 0.5f) * Range;
			//ae::Graphics.DrawLine(Player->Position, LeftLine);
			//ae::Graphics.DrawLine(Player->Position, RightLine);
		}

		ae::Graphics.SetDepthTest(true);
	}

	// Setup OpenGL for drawing the HUD
	ae::Graphics.Setup2D();
	ae::Graphics.SetStaticUniforms();
	ae::Graphics.SetDepthTest(false);
	ae::Graphics.SetDepthMask(false);

	// Show accuracy in degrees
	if(ae::Input.ModKeyDown(KMOD_ALT)) {
		std::ostringstream Buffer;
		Buffer << ae::Round1(Player->CurrentAccuracy);
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), ae::Input.GetMouse() + glm::ivec2(7, 7), ae::LEFT_BASELINE);
		Buffer.str("");
	}

	// Draw damage text numbers
	Particles->Render(_Particles::TEXT, BlendFactor);

	// Render HUD
	HUD->Render(ae::FocusedElement == nullptr && ae::Actions.State[Action::GAME_MAP].Value > 0.0f);

	// Debug mode
	if(DebugMode) {
		glm::vec2 DrawPosition = glm::vec2(10, 200) * ae::_Element::GetUIScale();
		glm::vec2 Spacing(0, 16 * ae::_Element::GetUIScale());
		std::ostringstream Buffer;
		Buffer << ae::Graphics.FramesPerSecond << " FPS";
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition);
		Buffer.str("");

		DrawPosition.y += Spacing.y;
		Buffer << BlockRenderCount << " blocks rendered";
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition);
		Buffer.str("");

		DrawPosition.y += Spacing.y;
		Buffer << PropRenderCount << " props rendered";
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition);
		Buffer.str("");

		DrawPosition.y += Spacing.y;
		Buffer << Map->ObjectManager->RenderList[_ObjectManager::RENDER_ITEMS].size() << " items rendered";
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition);
		Buffer.str("");

		DrawPosition.y += Spacing.y;
		Buffer << Map->ObjectManager->RenderList[_ObjectManager::RENDER_MONSTER].size() << " monsters rendered";
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition);
		Buffer.str("");

		DrawPosition.y += Spacing.y;
		Buffer << ParticleRenderCount << " decals rendered";
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition);
		Buffer.str("");

		DrawPosition.y += Spacing.y;
		Buffer << ActiveAI << " active ai";
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition);
		Buffer.str("");
	}

	// Show red overlay when player's health is low
	if(Player->GetHealthPercentage() <= HUD_PLAYER_HEALTH_WARNING) {
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos"]);
		float FadePower = (HUD_PLAYER_HEALTH_WARNING - Player->GetHealthPercentage()) / HUD_PLAYER_HEALTH_WARNING;
		float FadeAmount = HUD_PLAYER_HEALTH_FADE + sin(Timer * HUD_PLAYER_HEALTH_PULSE_FACTOR * FadePower) * HUD_PLAYER_HEALTH_PULSE_AMOUNT;
		ae::Graphics.SetColor(glm::vec4(1.0f, 0.0f, 0.0f, FadeAmount * FadePower));
		ae::Graphics.DrawRectangle(glm::vec2(0, 0), ae::Graphics.CurrentSize, true);
	}

	// Fade screen when paused
	if(IsPaused() || Player->IsDead())
		ae::Graphics.FadeScreen(ae::Assets.Programs["ortho_pos"], GAME_PAUSE_FADEAMOUNT);

	// Render menu
	Menu.Render();

	// Draw death screen
	if(Player->IsDead()) {
		ae::Graphics.SetCursor(1);
		HUD->DrawDeathScreen();
	}
}

// Resolve an entity attacking
void _PlayState::ResolveAttack(_Entity *Attacker, int GridType) {
	Attacker->AttackMade = false;

	// Check for ammo
	if(!Attacker->WeaponHasAmmo(Attacker->AttackRequestType))
		return;

	// Keep track of weapon used
	if(Attacker->Type == _Object::PLAYER) {
		const char *WeaponID = Attacker->GetWeaponID(Attacker->AttackRequestType);
		if(WeaponID)
			WeaponsUsed[WeaponID]++;
	}

	// Get number of rounds to fire
	int RoundsShot = 1;
	if(Attacker->FireAllRounds[Attacker->AttackRequestType])
		RoundsShot = Attacker->GetWeaponAmmo();

	// Reduce ammo
	if(!GodMode)
		Attacker->ReduceAmmo(RoundsShot);

	// Weapon type specific code
	int WeaponType = WEAPON_MELEE;
	if(Attacker->AttackRequestType == WEAPONATTACK_MAIN)
		WeaponType = Attacker->MainWeaponType;

	// Play fire sound and generate fire/smoke particles
	if(WeaponType != WEAPON_MELEE) {
		_Hit Hit(HIT_NONE);
		GenerateHitEffects(Attacker, -1, Hit);
		if(Attacker->FireSoundTimer == 0.0) {
			if(Attacker->Type == _Object::PLAYER)
				ae::Audio.PlaySound(Attacker->GetSound(SOUND_FIRE, WEAPONATTACK_MAIN));
			else
				ae::Audio.PlaySound(Attacker->GetSound(SOUND_FIRE, WEAPONATTACK_MAIN), ae::_SoundSettings(glm::vec3(Attacker->Position.x, 0.0f, Attacker->Position.y), 1.0f, AUDIO_REFERENCE_DISTANCE, AUDIO_MAX_DISTANCE, AUDIO_ROLL_OFF));

			Attacker->FireSoundTimer = ENTITY_MAX_FIRESOUND_PERIOD;
		}
	}

	Attacker->StartTriggerDownAudio();

	// For each bullet that the weapon fires
	bool PlayedHitWallSound = false;
	std::unordered_map<_Object *, int> DecalObjects;
	std::vector<_Hit> Hits;
	Hits.reserve(Attacker->Penetration[Attacker->AttackRequestType]);
	int AttackCount = RoundsShot * Attacker->AttackCount[Attacker->AttackRequestType];
	for(int i = 0; i < AttackCount; i++) {
		Hits.clear();

		// Check if gun was at min accuracy
		bool Steady = false;
		if(Attacker->AttackRequestType == WEAPONATTACK_MAIN && Attacker->IsSteady())
			Steady = true;

		// Check for projectile weapons
		if(Attacker->Projectiles[Attacker->AttackRequestType]) {

			// Create projectile
			_Object *Projectile = Stats.CreateProjectile(*Attacker->Projectiles[Attacker->AttackRequestType], Attacker->Position);
			Projectile->GridCheckType = (Attacker->Type == _Object::PLAYER) ? GRID_MONSTER : GRID_PLAYER;
			Projectile->Map = Map;
			Projectile->Owner = Attacker;
			Projectile->Rotation = Attacker->GenerateShotDirection();
			Projectile->Direction = glm::rotate(glm::vec2(0, -1), glm::radians(Projectile->Rotation));
			Projectile->Velocity = Projectile->Direction * Attacker->ProjectileSpeed[Attacker->AttackRequestType];
			Projectile->ProjectileMinDamage = Attacker->MinDamage[Attacker->AttackRequestType];
			Projectile->ProjectileMaxDamage = Attacker->MaxDamage[Attacker->AttackRequestType];
			Projectile->Depth = Attacker->Penetration[Attacker->AttackRequestType];
			Projectile->ProjectileCritChance = Attacker->CritChance[Attacker->AttackRequestType];
			Projectile->ProjectileCritDamage = Attacker->CritDamage[Attacker->AttackRequestType];
			Projectile->ProjectilePenetrationDamage = Attacker->PenetrationDamage[Attacker->AttackRequestType];
			Projectile->ProjectileExplosionSize = Attacker->ExplosionSize[Attacker->AttackRequestType];
			if(Steady)
				Projectile->ProjectileCritChance *= PLAYER_STEADY_CRIT_FACTOR;

			Map->ObjectManager->AddObject(Projectile);
		}
		else {

			// Check weapon type
			if(WeaponType == WEAPON_MELEE) {
				Map->CheckMeleeCollisions(Attacker, GridType, Attacker->Penetration[Attacker->AttackRequestType], Hits);
			}
			else {

				// Check distance to the wall
				float ShotDirection = Attacker->GenerateShotDirection();
				Map->CheckBulletCollisions(Attacker, glm::rotate(glm::vec2(0, -1), glm::radians(ShotDirection)), Hits, GridType, true, Attacker->Penetration[Attacker->AttackRequestType], _Tile::BULLET);

				// Generate tracer particle
				_ParticleTemplate *Template = &GameAssets.Particles["tracer0"];
				glm::vec2 ParticleStart = Attacker->Position + glm::rotate(glm::vec2(0, -Template->Size.y * 0.5f) + Attacker->WeaponOffset[Attacker->MainWeaponType], glm::radians(ShotDirection));
				_Particle *Tracer = new _Particle(_ParticleSpawn(Template, glm::vec2(0), ParticleStart, OBJECT_Z, ShotDirection));
				float Distance = glm::length(Hits.front().Position - Attacker->Position) - Template->Size.y;
				Tracer->Lifetime = Distance * Template->VelocityScale.y * GAME_FPS;
				Particles->Add(Tracer);
			}

			// Generate particle effects and reduce health
			float PenetrationDamage = 1.0f;
			for(const auto &Hit : Hits) {
				switch(Hit.Type) {
					case HIT_NONE:
					break;
					case HIT_WALL: {
						if(!PlayedHitWallSound) {
							ae::Audio.PlaySound(Attacker->GetSound(SOUND_RICOCHET, WEAPONATTACK_MAIN), ae::_SoundSettings(glm::vec3(Hit.Position.x, 0.0f, Hit.Position.y), 1.0f, AUDIO_REFERENCE_DISTANCE, AUDIO_MAX_DISTANCE, AUDIO_ROLL_OFF));
							PlayedHitWallSound = true;
						}

						bool CreateWallDecal = (Hit.Object && Hit.Object->Type == _Object::PROP) ? false : true;
						GenerateHitEffects(Attacker, HIT_WALL, Hit, CreateWallDecal);
					} break;
					case HIT_OBJECT: {
						_Entity *HitEntity = (_Entity *)Hit.Object;
						bool HitPlayer = Hit.Object->Type == _Object::PLAYER;

						// Generate damage
						bool Crit = false;
						int Damage = Attacker->GenerateDamage(Attacker->AttackRequestType, PenetrationDamage, Steady, Crit);
						Damage = HitEntity->ReduceDamage(Damage);
						if(GodMode && HitPlayer)
							Damage = 0;

						// Generate damage particles
						GenerateDamageText(Hit.Position, Damage, Crit, HitPlayer);

						// Update health
						HitEntity->UpdateHealth(-Damage);

						// Generate bullet effects once for each hit object
						if(DecalObjects.find(Hit.Object) == DecalObjects.end()) {
							GenerateHitEffects(Attacker, HIT_OBJECT, Hit);
							DecalObjects[Hit.Object] = 1;
						}

						// Callback functions
						Attacker->OnAttack(HitEntity, Hit);
						HitEntity->OnHit(Attacker, Hit);

						// Apply penetration
						PenetrationDamage *= Attacker->PenetrationDamage[Attacker->AttackRequestType];
					} break;
				}
			}
		}
	}

	// Update accuracy
	Attacker->ApplyRecoil();
}

// Handle pickup
void _PlayState::HandlePickup() {
	ClosestItem = nullptr;

	// Get nearby items
	std::unordered_map<_Object *, int> NearbyItems;
	Map->GetCloseObjects(Player->Position, Player->Radius, GRID_ITEM, NearbyItems, &ClosestItem);
	for(auto &Iterator : NearbyItems) {
		_Item *NearbyItem = (_Item *)Iterator.first;
		if(!NearbyItem->Visible)
			continue;

		// Automatically pickup ammo
		if(NearbyItem->IsAutoPickup()) {
			int AmountAdded = 0;
			int Type = NearbyItem->Type;
			std::string Name = NearbyItem->Name;
			PickupObject(NearbyItem, AmountAdded);
			if(!AmountAdded) {
				IgnoreItems[NearbyItem] = 1;
				continue;
			}

			// Set up particle
			glm::vec2 ParticlePosition(Player->Position.x, Player->Position.y - 0.5);
			std::string ParticleText = "+";
			glm::vec4 ParticleColor = COLOR_WHITE;
			if(Type == _Object::AMMO)
				ParticleText += std::to_string(AmountAdded);
			else if(Type == _Object::MEDKIT) {
				ParticleText += std::to_string(AmountAdded) + "HP";
				ParticleColor = COLOR_GREEN;
			}
			else
				ParticleText += Name;

			// Add particle
			_Particle *Particle = new _Particle(_ParticleSpawn(GameAssets.GetParticleTemplate("text0"), glm::vec2(0), ParticlePosition, OBJECT_Z, 0));
			Particle->Text = ParticleText;
			Particle->Color = ParticleColor;
			Particles->Add(Particle);
		}
		// Manually pickup up an item
		else if(Player->UseRequested)
			UseObject(NearbyItem);
	}
}

// Called when the player dies
void _PlayState::PlayerDied() {

	// Dying sound
	ae::Audio.PlaySound(ae::Assets.Sounds["player_die0.ogg"], ae::_SoundSettings(glm::vec3(Player->Position.x, 0.0f, Player->Position.y), 1.0f, AUDIO_REFERENCE_DISTANCE, AUDIO_MAX_DISTANCE, AUDIO_ROLL_OFF));

	// Update monsters
	for(const auto &Entity : Monsters) {
		_Monster *Monster = (_Monster *)Entity;
		Monster->OnPlayerDeath();
	}
}

// Places an item into the player's inventory
void _PlayState::PickupObject(_Item *Item, int &AmountAdded) {
	if(!Item || !Item->Visible)
		return;

	// Attempt to add item
	int AddResult = Player->AddItem(Item, AmountAdded);
	if(AddResult) {
		if(Item == ClosestItem)
			ClosestItem = nullptr;

		Player->UseTimer = 0.0;

		// Remove item from map
		Map->RemoveObject(Item, GRID_ITEM);

		// Delete item
		if(AddResult == 2) {
			delete Item;
			CursorItemTimer = 0;
		}
	}
	else
		HUD->ShowTextMessage("INVENTORY FULL", HUD_INVENTORYFULLTIME);
}

// Processes the use key to open doors, hit switches, and pickup items
void _PlayState::UseObject(_Item *Item) {
	if(!Player->CanPickup())
		return;

	// Pick up an item if available
	int AmountAdded = 0;
	PickupObject(Item, AmountAdded);
}

// Open door or handle switches, return true on success
bool _PlayState::ActivateEvent() {
	bool Success = false;

	// Open a door if possible
	glm::ivec2 Position;
	Map->GetAdjacentTile(Player->Position, Player->Rotation, Position);

	// Check for events
	std::vector<_Event *> &Events = Map->GetEventList(Position);
	for(auto Event : Events) {
		if(!Event->Active)
			continue;

		if(!Map->CanChangeMapState(Event))
			continue;

		// Check for doors or switches
		if(Event->Type != EVENT_DOOR && Event->Type != EVENT_WALLSWITCH)
			continue;

		// Check for key in inventory and use it
		if(!Event->ItemID.empty()) {
			if(Player->Keys.find(Event->ItemID) == Player->Keys.end()) {
				HUD->ShowMessageBox("You need the " + Stats.Objects.at(Event->ItemID).Name, HUD_KEY_MESSAGETIME, UI_MESSAGE_SMALL_SIZE);
				return Success;
			}

			std::string KeyName = Stats.Objects.at(Event->ItemID).Name;
			std::transform(KeyName.begin(), KeyName.end(), KeyName.begin(), ::toupper);
			HUD->ShowTextMessage(KeyName + " USED", 2.0f);
		}

		// Change map
		Map->ChangeMapState(Event);

		// Decrement level
		if(Event->Level > 0) {
			Event->Decrement();
			if(Event->Level == 0)
				Event->Active = false;
		}

		// Toggle event
		Event->Switched = !Event->Switched;

		// Reset player's use timer
		Player->UseTimer = 0.0;

		Success = true;
	}

	return Success;
}

// Creates a random item from an entity
void _PlayState::CreateItemDrop(const _Entity *Entity, float DropRate) {
	if(Entity->Type != _Object::MONSTER)
		return;

	// Get monster
	const _Monster *Monster = (const _Monster *)Entity;
	if(!Monster->ItemDrop)
		return;

	// Roll for items
	for(int i = 0; i < Monster->Template.Attributes.at("drop_count").Int; i++) {
		int Rolls = (int)DropRate;

		// Check for extra roll
		double MultiOdds = DropRate - (int)DropRate;
		double MultiRoll = ae::GetRandomReal(0, 1);
		if(MultiRoll <= MultiOdds)
			Rolls++;

		// Roll for each item
		for(int j = 0; j < Rolls; j++) {

			// Roll for drop
			_ObjectSpawn ObjectSpawn;
			Stats.GetRandomDrop(Monster->ItemDrop, &ObjectSpawn);
			if(ObjectSpawn.Type) {
				ObjectSpawn.Position = _Map::GenerateRandomPointInCircle(PLAYER_RADIUS) + Monster->Position;

				// Spawn object on player if item can't be reached
				if(!Map->CheckCollisionFlag(Map->GetValidCoord(ObjectSpawn.Position), _Tile::ENTITY) || (Monster->FreePathing && Monster->Template.ItemDropID == "boss"))
					ObjectSpawn.Position = Player->Position;

				ObjectSpawn.Level = Monster->Level;
				SpawnObject(&ObjectSpawn, true);
			}
		}
	}
}

// Updates the monsters
void _PlayState::UpdateMonsters(double FrameTime) {
	bool HasBossKey = Player->Keys.find("key_boss") != Player->Keys.end();

	// Loop through monsters
	ActiveAI = 0;
	for(auto MonsterIterator = Monsters.begin(); MonsterIterator != Monsters.end();) {
		_Monster *Monster = (_Monster *)*MonsterIterator;

		if(!Monster->Active) {
			Map->RemoveObjectFromGrid(Monster, GRID_MONSTER);
			delete Monster;
			MonsterIterator = Monsters.erase(MonsterIterator);
		}
		else {
			Monster->Update(FrameTime);
			if(Monster->MoveState)
				ActiveAI++;

			// Get bounds
			glm::vec4 Bounds;
			Monster->GetRenderBounds(Bounds);

			// Add to minimap
			if((Monster->MoveState || Monster->IsCrate() || HasBossKey) && Monster->Health > 0 && Map->CheckMinimapBounds(Bounds)) {
				_MinimapLayer MinimapLayer;
				MinimapLayer.Bounds = Bounds;
				MinimapLayer.Color = Monster->IsCrate() ? HUD_MINIMAP_CRATE_COLOR : HUD_MINIMAP_ENEMY_COLOR;
				Map->MinimapLayers.push_back(MinimapLayer);
			}

			// Attack
			if(Monster->AttackMade) {
				Monster->FacePosition(Player->Position);
				ResolveAttack(Monster, GRID_PLAYER);
			}

			// Add to render list
			if(Camera->IsAABBInView(Bounds)) {
				int RenderListType = Monster->IsCrate() ? _ObjectManager::RENDER_PROP : _ObjectManager::RENDER_MONSTER;
				Map->ObjectManager->RenderList[RenderListType].push_back(Monster);
			}

			++MonsterIterator;
		}
	}
}

// Checks for the player triggering events
void _PlayState::CheckEvents(const _Entity *Entity) {
	Player->TileChanged = false;

	// Check for events triggered by walking
	glm::ivec2 Position = Map->GetValidCoord(Entity->Position);
	std::vector<_Event *> &Events = Map->GetEventList(Position);
	for(auto Event : Events) {
		if(!Event->Active)
			continue;

		// Perform action
		switch(Event->Type) {
			case EVENT_SPAWN:
				if(Stats.Objects.find(Event->MonsterID) != Stats.Objects.end()) {
					Event->StartTimer();
					ActiveEvents.push_back(Event);
				}
				Event->Active = false;
			break;
			case EVENT_CHECK:
				switch(Map->MapType) {
					case MAPTYPE_CAMPAIGN:
						if(Event->Level > Player->CheckpointIndex) {
							Player->CheckpointIndex = Event->Level;
							HUD->ShowTextMessage(HUD_CHECKPOINTMESSAGE, HUD_CHECKPOINTTIME);
							Save.SavePlayer(Player);
						}
						Event->Active = false;
					break;
					case MAPTYPE_ADVENTURE:
						if(Event->Level != Player->CheckpointIndex) {
							Player->CheckpointIndex = Event->Level;
							HUD->ShowTextMessage(HUD_CHECKPOINTMESSAGE, HUD_CHECKPOINTTIME);
							Save.SavePlayer(Player);
						}
					break;
					default:
					break;
				}
			break;
			case EVENT_ENDLEVEL: {
				Level = Event->ItemID;
				bool GotOneHundredPercent = HUD->Kills[0] == HUD->Kills[1] && HUD->Crates[0] == HUD->Crates[1] && HUD->Secrets[0] == HUD->Secrets[1];
				if(!GotOneHundredPercent)
					Player->Stat100Percent = false;

				// Build weapons used string
				std::string WeaponsUsedString = "";
				for(const auto &WeaponID : WeaponsUsed) {
					if(Stats.Objects.find(WeaponID.first) == Stats.Objects.end())
						continue;

					if(!WeaponsUsedString.empty())
						WeaponsUsedString += ", ";

					WeaponsUsedString += Stats.Objects.at(WeaponID.first).Name;

					// Check achievements
					if(WeaponID.first != "weapon_fists")
						Player->StatFistsOnly = false;

					if(WeaponID.first != "weapon_knife" && WeaponID.first != "weapon_boltrifle")
						Player->StatLoneWolf = false;
				}

				// End of the game
				if(Level.empty()) {
					Menu.SetScoreStats(true, Player->LevelTime, HUD->Kills, HUD->Crates, HUD->Secrets, Player->Progression + 1, WeaponsUsedString);
					Level = GAME_FIRSTLEVEL;

					// Check achievements
					if(Player->Progression == 0) {
						if(Player->StatFistsOnly)
							Menu.UnlockAchievement("fists");

						if(Player->StatLoneWolf)
							Menu.UnlockAchievement("lonewolf");
					}

					if(Player->Stat100Percent)
						Menu.UnlockAchievement("all");

					if(Player->Progression >= 10)
						Menu.UnlockAchievement("p10");

					if(Player->LavaTouches == 0)
						Menu.UnlockAchievement("smoked");

					// Reset stats
					Player->Progression++;
					Player->ProgressionTime = 0;
					Player->ProgressionKills = 0;
					Player->ProgressionCrates = 0;
					Player->ProgressionSecrets = 0;
					Player->ProgressionDeaths = 0;
					Player->ResetAchievementTracking();
				}
				else {
					Player->ProgressionKills += HUD->Kills[0];
					Player->ProgressionCrates += HUD->Crates[0];
					Player->ProgressionSecrets += HUD->Secrets[0];
					Menu.SetScoreStats(false, Player->LevelTime, HUD->Kills, HUD->Crates, HUD->Secrets, 0, WeaponsUsedString);
				}

				Player->LevelTime = 0;
				Player->CheckpointIndex = Event->Level;
				Player->MapID = Level;
				if(Map->MapType == MAPTYPE_CAMPAIGN)
					Player->Keys.clear();
				Save.SavePlayer(Player);

				NullState.LevelComplete = true;
				Framework.ChangeState(&NullState);
			} break;
			case EVENT_TEXT: {

				// Check for tutorial messages
				bool IsTutorial = Event->ItemID.find("tutorial_") == 0;
				if(!IsTutorial || (Config.Tutorial && !Player->Progression)) {
					if(IsTutorial)
						ae::Audio.PlaySound(ae::Assets.Sounds["game_message0.ogg"]);
					HUD->ShowMessageBox(Stats.Strings[Event->ItemID], Event->ActivationPeriod, UI_MESSAGE_SIZE);
				}

				if(Event->Level != 0)
					Event->Active = false;
			} break;
			case EVENT_SOUND:
				if(ae::Assets.Sounds[Event->ItemID]) {
					Event->StartTimer();
					ActiveEvents.push_back(Event);
				}
				Event->Active = false;
			break;
			case EVENT_FLOORSWITCH:
			case EVENT_ENABLE:
				Event->StartTimer();
				ActiveEvents.push_back(Event);
				Event->Active = false;
			break;
			case EVENT_TELEPORT: {
				if(Event->Level > 0) {
					Event->Decrement();
					if(Event->Level == 0)
						Event->Active = false;
				}

				if(Event->Tiles.size() > 0) {
					glm::vec2 NewPosition(Event->Tiles[0].Coord.x + 0.5f, Event->Tiles[0].Coord.y + 0.5f);
					Particles->Create(_ParticleSpawn(GameAssets.GetParticleTemplate(Event->ParticleID), glm::vec2(0), NewPosition, OBJECT_Z, 0));

					Map->RemoveObjectFromGrid(Player, GRID_PLAYER);
					Player->SetPosition(NewPosition);
					Map->AddObjectToGrid(Player, GRID_PLAYER);
					Player->TileChanged = true;
				}
			} break;
			case EVENT_LIGHT: {
				if(Event->Level > 0) {
					Event->Active = false;
					Event->StartTimer();
					ActiveEvents.push_back(Event);
				}
				else
					Map->SetAmbientLight(Event->ItemID);
			} break;
			case EVENT_SECRET: {
				ae::Audio.PlaySound(ae::Assets.Sounds["game_secret0.ogg"]);
				HUD->ShowMessageBox("You have found a secret!", HUD_SECRET_MESSAGETIME, UI_MESSAGE_SMALL_SIZE);
				HUD->Secrets[0]++;
				Event->Active = false;
			} break;
			case EVENT_LAVA: {
				ae::Audio.PlaySound(ae::Assets.Sounds["game_lava0.ogg"]);
				if(!GodMode)
					Player->UpdateHealth(-GAME_LAVA_DAMAGE * (Event->Level + Player->Progression));
				Particles->Create(_ParticleSpawn(GameAssets.GetParticleTemplate(Event->ParticleID), glm::vec2(0), Player->Position, OBJECT_Z, 0));
				Player->LavaTouches++;
			} break;
			default:
			break;
		}
	}
}

// Update the active events
void _PlayState::UpdateEvents(double FrameTime) {

	// Activate events
	for(auto ActiveEventIterator = ActiveEvents.begin(); ActiveEventIterator != ActiveEvents.end(); ++ActiveEventIterator) {
		_Event *Event = *ActiveEventIterator;
		Event->Update(FrameTime);
		if(!Event->TimerExpired())
			continue;

		glm::vec2 Position;
		bool Decrement = false;
		switch(Event->Type) {
			case EVENT_SPAWN: {
				const std::vector<_EventTile> &Tiles = Event->Tiles;
				for(size_t i = 0; i < Tiles.size(); i++) {

					// Chance for special monster
					int SpecialType = 0;
					if(Player->Progression && ae::GetRandomInt(1, 100) <= Player->Progression * GAME_PROGRESSION_SPECIAL_CHANCE)
						SpecialType = ae::GetRandomInt((size_t)1, Stats.Specials.size() - 1);

					// Spawn monsters
					for(int j = 0; j < Event->SpawnMultiplier; j++) {
						Position.x = Tiles[i].Coord.x + 0.5f;
						Position.y = Tiles[i].Coord.y + 0.5f;
						_Monster *Monster = Stats.CreateMonster(Event->MonsterID, Event->SpawnLevel + Map->GetAddedLevel(), Position, SpecialType);
						Monster->Player = Player;
						Monster->FreePathingTimer = ENTITY_FREEPATHING_TIMER_INCREMENT * j;
						AddMonster(Monster);
						Particles->Create(_ParticleSpawn(GameAssets.GetParticleTemplate(Event->ParticleID), glm::vec2(0), Position, OBJECT_Z, 0));
					}
				}

				Decrement = true;
			} break;
			case EVENT_SOUND: {
				const std::vector<_EventTile> &Tiles = Event->Tiles;
				if(Tiles.size())
					ae::Audio.PlaySound(ae::Assets.Sounds[Event->ItemID], ae::_SoundSettings(glm::vec3(Tiles.front().Coord.x, 0, Tiles.front().Coord.y), 1.0f, AUDIO_REFERENCE_DISTANCE, AUDIO_MAX_DISTANCE, AUDIO_ROLL_OFF));
				else
					ae::Audio.PlaySound(ae::Assets.Sounds[Event->ItemID]);
				Decrement = true;
			} break;
			case EVENT_FLOORSWITCH:
				if(Map->CanChangeMapState(Event)) {
					Map->ChangeMapState(Event);
					Decrement = true;
				}
			break;
			case EVENT_ENABLE: {
				const std::vector<_EventTile> &Tiles = Event->Tiles;
				for(size_t i = 0; i < Tiles.size(); i++)
					Map->ToggleEventActive(Tiles[i].BlockID);

				Decrement = true;
			} break;
			case EVENT_LIGHT: {
				Map->SetAmbientLight(Event->ItemID);
				Decrement = true;
			} break;
			default:
			break;
		}

		// Play sound
		if(!Event->SoundID.empty())
			ae::Audio.PlaySound(ae::Assets.Sounds[Event->SoundID]);

		// Decrease the event level
		if(Decrement) {
			Event->StartTimer();
			if(Event->Level != -1)
				Event->Decrement();

			if(Event->Level == 0) {
				ActiveEventIterator = ActiveEvents.erase(ActiveEventIterator);
				if(ActiveEventIterator == ActiveEvents.end())
					break;
			}
		}
	}
}

// Deletes the monsters
void _PlayState::DeleteMonsters() {
	for(auto Iterator : Monsters) {
		if(Iterator)
			delete Iterator;
	}

	Monsters.clear();
}

// Spawn an object in the map
void _PlayState::SpawnObject(_ObjectSpawn *ObjectSpawn, bool GenerateStats, int AddedLevel) {
	if(ObjectSpawn->Type == _Object::MONSTER) {
		_Monster *Monster = Stats.CreateMonster(ObjectSpawn->ID, ObjectSpawn->Level + Map->GetAddedLevel(), ObjectSpawn->Position);
		Monster->Player = Player;
		AddMonster(Monster);
		if(Monster->IsCrate())
			Map->Crates++;
		else
			Map->Monsters++;
	}
	else if(ObjectSpawn->Type == _Object::PROP)
		Map->AddObject(Stats.CreateProp(ObjectSpawn->ID, ObjectSpawn->Position, ObjectSpawn->Rotation, ObjectSpawn->Scale), GRID_MONSTER);
	else
		Map->AddObject(Stats.CreateItem(ObjectSpawn->ID, ObjectSpawn->Level + AddedLevel, 0, 1, ObjectSpawn->Position, GenerateStats), GRID_ITEM);
}

// Adds a monster to the monster list and collision grid
void _PlayState::AddMonster(_Monster *Monster) {
	Monster->Map = Map;
	if(Map->SimpleAI)
		Monster->AIType = AI_SIMPLE;
	Monsters.push_back(Monster);
	Map->AddObjectToGrid(Monster, GRID_MONSTER);
}

// Removes a monster from object list and collision grid
void _PlayState::RemoveMonster(_Monster *Monster) {
	for(auto Iterator = Monsters.begin(); Iterator != Monsters.end(); ++Iterator) {
		if(*Iterator == Monster) {
			Monsters.erase(Iterator);
			break;
		}
	}

	Map->RemoveObjectFromGrid(Monster, GRID_MONSTER);
}

// Generate particles depending on hit type
void _PlayState::GenerateHitEffects(_Entity *Attacker, const int Type, const _Hit &Hit, bool CreateWallDecal) {
	if(Type == -1) {
		glm::vec2 ParticlePosition = Attacker->Position + glm::rotate(Attacker->WeaponOffset[Attacker->MainWeaponType], glm::radians(Attacker->Rotation));
		Particles->Create(_ParticleSpawn(Attacker->GetParticle(PARTICLE_FIRE), glm::vec2(0), ParticlePosition, OBJECT_Z, Attacker->Rotation));
		Particles->Create(_ParticleSpawn(Attacker->GetParticle(PARTICLE_SMOKE), glm::vec2(0), ParticlePosition, OBJECT_Z, 0));
	}
	else if(Type == HIT_WALL) {
		Particles->Create(_ParticleSpawn(Attacker->GetParticle(PARTICLE_RICOCHET), Hit.Normal, Hit.Position, OBJECT_Z, Attacker->Rotation));
		if(CreateWallDecal && Config.WallDecals)
			Particles->Create(_ParticleSpawn(Attacker->GetParticle(PARTICLE_BULLETHOLE), Hit.Normal, Hit.Position, OBJECT_Z, Attacker->Rotation));
	}
	else if(Type == HIT_OBJECT) {
		glm::vec2 ParticlePosition = _Map::GenerateRandomPointInCircle(0.2f) + Hit.Object->Position;
		Particles->Create(_ParticleSpawn(Hit.Object->GetParticle(PARTICLE_HIT), Hit.Normal, Hit.Position, OBJECT_Z, Attacker->Rotation));
		if(Config.FloorDecals)
			Particles->Create(_ParticleSpawn(Hit.Object->GetParticle(PARTICLE_FLOORDECAL), Hit.Normal, ParticlePosition, 0.06f, Attacker->Rotation));
	}
}

// Create damage number particles
void _PlayState::GenerateDamageText(glm::vec2 Position, int Value, bool Crit, bool HitPlayer) {
	Position += _Map::GenerateRandomPointInCircle(0.2f);

	// Create particle
	_Particle *DamageParticle = new _Particle(_ParticleSpawn(GameAssets.GetParticleTemplate("text0"), glm::vec2(0), Position, OBJECT_Z, 0));
	DamageParticle->Text = std::to_string(Value);

	// Set color
	if(HitPlayer)
		DamageParticle->Color = COLOR_RED;
	if(Crit)
		DamageParticle->Color = COLOR_YELLOW;

	Particles->Add(DamageParticle);
}

// Generate explosion particles
void _PlayState::GenerateExplosion(const _ParticleTemplate *ParticleTemplate, const glm::vec2 &Position, const glm::vec2 &Scale) {
	if(!Particles->Create(_ParticleSpawn(ParticleTemplate, glm::vec2(0), Position, OBJECT_Z, 0)))
		return;

	// Set scale
	_Particle *Particle = Particles->Particles.back();
	Particle->Scale = Scale;
}

// Generate projectile particle effects
void _PlayState::GenerateProjectileEffects(const _ParticleTemplate *ParticleTemplate, const glm::vec2 &Position) {
	Particles->Create(_ParticleSpawn(ParticleTemplate, glm::vec2(0), Position, OBJECT_Z, 0));
}

// Determine if game is paused
bool _PlayState::IsPaused() {
	return Menu.GetState() != _Menu::STATE_NONE;
}
