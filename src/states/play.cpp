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
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <fstream>
#include <iostream>
#include <algorithm>

_PlayState PlayState;

// Constructor
_PlayState::_PlayState() {
	Framebuffer = nullptr;
	Player = nullptr;
	Level = "";
	TestMode = false;
	DevMode = false;
	GodMode = false;
	DebugMode = false;
	FromEditor = false;
	LastClosestItem = nullptr;
	ClosestItem = nullptr;
	ClosestItemTimer = 0.0;
	FlashTimer = 0.0;
}

// Load level and set up objects
void _PlayState::Init() {
	ae::Graphics.SetViewport(ae::Graphics.CurrentSize);
	ae::Graphics.Element->SetActive(false);
	ae::Graphics.Element->Active = true;

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
	if(Level == "")
		Level = Player->MapID;

	// Create framebuffer for mixing lights
	Framebuffer = new ae::_Framebuffer(ae::Graphics.CurrentSize);

	// Load level
	Map = new _Map(Level, Player->Progression+1);
	Map->InitializeTiles();
	Player->Map = Map;
	Player->MapID = Map->Filename;

	// Set starting states
	Player->SetPosition(Map->GetStartingPositionByCheckpoint(Player->CheckpointIndex));
	Player->TileChanged = true;
	Map->AddObjectToGrid(Player, GRID_PLAYER);

	// Spawn objects
	for(const auto &ObjectSpawn : Map->ObjectSpawns)
		SpawnObject(ObjectSpawn, false, Player->GetAddedLevel());

	// Initialize objects
	HUD = new _HUD(Player);
	HUD->SetStats(Map->Monsters, Map->Crates, Map->Secrets);
	Particles = new _Particles();

	// Set up camera
	ae::_CameraSettings CameraSettings;
	CameraSettings.UpdateDivisor = CAMERA_DIVISOR;
	Camera = new ae::_Camera(CameraSettings);
	Camera->CalculateFrustum(ae::Graphics.AspectRatio);
	Camera->ForcePosition(glm::vec3(Player->Position, CAMERA_DISTANCE));

	Map->Camera = Camera;
	Particles->Camera = Camera;
	Particles->Map = Map;
	Camera->ConvertScreenToWorld(ae::Input.GetMouse(), WorldCursor);
	PreviousWorldCursor = WorldCursor;

	ae::Graphics.SetCursor(false);

	ae::Actions.ResetState();
	ae::Audio.Stop();

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
		if(!Player->SelfHealing) {
			switch(Action) {
				case Action::GAME_INVENTORY:
					HUD->SetInventoryOpen(!HUD->InventoryOpen);
					Player->SetAiming(false);
					Player->SetSprinting(false);
				break;
				case Action::GAME_FIRE:
					if(!HUD->InventoryOpen && !Player->IsMeleeAttacking()) {

						// Use melee weapon if player has no main hand
						int AttackType = WEAPONATTACK_MAIN;
						if(!Player->HasMainHand() && Player->HasMelee())
							AttackType = WEAPONATTACK_MELEE;

						// Can reload
						if(Player->Reloading && Player->WeaponHasAmmo(AttackType))
							Player->CancelReloading();

						// Play sound
						if(Player->CanAttack(AttackType) && !Player->WeaponHasAmmo(AttackType))
							ae::Audio.PlaySound(Player->GetSound(SOUND_EMPTY, AttackType));

						if(Player->CheckAttackTimer(AttackType) && Player->FireRateType[AttackType] == FIRERATE_SEMI && (!Player->BurstRounds[AttackType] || (Player->BurstRounds[AttackType] && Player->BurstRoundsShot == 0))) {
							Player->BurstRoundsShot = 0;
							Player->AttackRequested = true;
							Player->AttackRequestType = AttackType;
						}
					}
				break;
				case Action::GAME_MELEE:
					if(!HUD->InventoryOpen) {
						if(Player->Reloading)
							Player->CancelReloading();

						if(Player->FireRateType[WEAPONATTACK_MELEE] == FIRERATE_SEMI) {
							Player->AttackRequested = true;
							Player->AttackRequestType = WEAPONATTACK_MELEE;
						}
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
					ae::Audio.PlaySound(ae::Assets.Sounds["game_flashlight0"]);
				break;
			}
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
				else if(!Player->IsDying() && !Player->SelfHealing) {
					if(HUD->InventoryOpen) {
						HUD->SetInventoryOpen(false);
					}
					else if(TestMode) {
						if(FromEditor)
							Framework.ChangeState(&EditorState);
						else
							Framework.Done = true;

						Save.SavePlayer(Player);
					}
					else
						Menu.InitInGame();
				}
			break;
			case SDL_SCANCODE_F1:
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
			ae::Audio.PlaySound(Player->GetSound(SOUND_DEATH, -1), ae::_SoundSettings(glm::vec3(Player->Position.x, 0.0f, Player->Position.y)));
		}

		return true;
	}

	// Handle dev commands
	if(DevMode) {
		if(Console->Command == "experience") {
			if(Parameters.size() == 1) {
				if(!Player)
					return true;

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
			if(Parameters.size() == 1) {
				if(!Player)
					return true;

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

		else if(Console->Command == "reset") {
			if(!Player)
				return true;

			Player->Reset();
			Console->AddMessage("player reset");
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

	Timer += FrameTime;
	FlashTimer = std::max(0.0, FlashTimer - FrameTime);

	// Handle pause
	if(IsPaused()) {
		Menu.Update(FrameTime);
		ae::Graphics.SetCursor(true);
		if(HUD) {
			HUD->CursorOverItem = nullptr;
			HUD->CursorOverWorld = false;
		}

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

		// Handle healing
		Player->SelfHealing = Player->CanSelfHeal() && ae::Actions.State[Action::GAME_HEAL].Value > 0.0f;
		if(!Player->SelfHealing) {

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

				// Attack again
				if(!Player->IsMeleeAttacking() && Player->FireRateType[WEAPONATTACK_MAIN] == FIRERATE_AUTO && ae::Actions.State[Action::GAME_FIRE].Value > 0.0f) {
					Player->AttackRequested = true;
					Player->AttackRequestType = WEAPONATTACK_MAIN;
				}
				if(Player->FireRateType[WEAPONATTACK_MELEE] == FIRERATE_AUTO && ae::Actions.State[Action::GAME_MELEE].Value > 0.0f) {
					Player->AttackRequested = true;
					Player->AttackRequestType = WEAPONATTACK_MELEE;
				}

				// Aim
				Player->SetAiming(ae::Actions.State[Action::GAME_AIM].Value > 0.0f && !Player->Reloading && !Player->SwitchingWeapons);
				Player->SetSprinting(ae::Actions.State[Action::GAME_SPRINT].Value > 0.0f);
			}

			Player->UseRequested = ae::Actions.State[Action::GAME_USE].Value;
			Player->SelfHealTimer = 0.0;
		}
		else {
			HUD->CursorItem = nullptr;
			Player->MoveState = MOVE_NONE;
			Player->SetAiming(false);
			Player->SetSprinting(false);
			Player->UseRequested = false;
			Player->AttackRequested = false;
		}
	}
	else
		HUD->SetInventoryOpen(false);

	// Update player
	Player->Update(FrameTime);
	if(Player->PositionChanged)
		IgnoreItems.clear();

	// Handle gun flashes
	if(Player->Action == ACTION_STARTSHOOT)
		FlashTimer = LIGHT_FLASH_TIME;

	// Check for events
	if(Player->TileChanged)
		CheckEvents(Player);

	// Handle auto and manually picking up items
	HandlePickup();

	// Activate events
	if(Player->UseRequested && Player->CanUse()) {
		ActivateEvent();
		Player->UseRequested = false;
	}

	// Update objects
	Map->Update(FrameTime);
	Map->ObjectManager->RenderList[1].push_back(Player);

	// Update monsters
	int PlayerHealth = Player->Health;
	UpdateMonsters(FrameTime);

	// Check for player dying
	if(Player->Health == 0 && PlayerHealth > 0)
		PlayerDied();

	// Update particles
	Particles->Update(FrameTime);

	// Add player to minimap
	_MinimapLayer MinimapLayer;
	MinimapLayer.Color = COLOR_DARK;
	MinimapLayer.Bounds = glm::vec4(
		Player->Position.x - Player->Scale * 0.25f, Player->Position.y - Player->Scale * 0.25f,
		Player->Position.x + Player->Scale * 0.25f, Player->Position.y + Player->Scale * 0.25f
	);
	Map->MinimapLayers.push_back(MinimapLayer);

	// Update events
	UpdateEvents(FrameTime);

	// Apply the damage
	int OldLevel = Player->Level;
	if(Player->AttackMade)
		ResolveAttack(Player, GRID_MONSTER);

	// Level up screen
	if(Player->Level > OldLevel)
		HUD->ShowTextMessage("LEVEL UP! YOU HAVE UNSPENT SKILL POINTS", 5.0);

	// Update camera
	Camera->Set2DPosition(Player->Position);

	// Get zoom state
	if(Player->Aiming) {
		if(Map->IsVisible(Player->Position, WorldCursor, _Tile::BULLET)) {
			Camera->UpdatePosition((WorldCursor - Player->Position) / Player->ZoomScale);
		}
		else {
			std::vector<_Hit> Hits;
			Map->CheckBulletCollisions(Player->Position, WorldCursor - Player->Position, Hits, 0, false, 1, _Tile::BULLET);
			if(Hits.size())
				Camera->UpdatePosition((Hits.front().Position - Player->Position) / Player->ZoomScale);
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
	HUD->Update(FrameTime, Player->GetCrosshairRadius(WorldCursor));

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
	ae::Assets.Programs["map"]->LightCount = 1;
	ae::Assets.Programs["map"]->Lights[0].Color = PlayerLightColor;
	ae::Assets.Programs["map"]->Lights[0].Position = LightPosition;
	ae::Assets.Programs["map"]->Lights[0].Attenuation = LightAttenuantion;
	ae::Assets.Programs["map"]->AmbientLight = Map->GetAmbientLight();
	ae::Assets.Programs["map_norm"]->LightCount = 1;
	ae::Assets.Programs["map_norm"]->Lights[0].Color = PlayerLightColor;
	ae::Assets.Programs["map_norm"]->Lights[0].Position = LightPosition;
	ae::Assets.Programs["map_norm"]->Lights[0].Attenuation = LightAttenuantion;
	ae::Assets.Programs["map_norm"]->AmbientLight = Map->GetAmbientLight();

	// Setup the viewing matrix
	ae::Graphics.Setup3D();
	Camera->Set3DProjection(BlendFactor);

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
	ae::Graphics.DisableParticleBlending();
	ae::_Framebuffer::Unbind();

	// Set framebuffer texture
	if(Framebuffer) {
		ae::Assets.Programs["map"]->Use();
		ae::Graphics.SetActiveTexture(1);
		Framebuffer->BindTexture();
		ae::Graphics.SetActiveTexture(0);
		ae::Assets.Programs["map_norm"]->Use();
		ae::Graphics.SetActiveTexture(1);
		Framebuffer->BindTexture();
		ae::Graphics.SetActiveTexture(0);
	}

	// Draw the floor
	int BlockRenderCount = Map->RenderFloors();

	// Draw floor decals
	ae::Graphics.SetProgram(ae::Assets.Programs["map"]);
	ae::Assets.Programs["map"]->ResetTextureTransform();
	ae::Graphics.SetDepthMask(false);
	ae::Graphics.SetDepthTest(false);
	int ParticleRenderCount = Map->RenderParticles(_Particles::FLOOR_DECALS);

	// Draw walls clipped with MaxZ=OBJECT_Z
	BlockRenderCount += Map->RenderWalls();

	// Draw objects
	Map->ObjectManager->Render(BlendFactor);

	// Draw the rest of the walls
	BlockRenderCount += Map->RenderWalls();
	BlockRenderCount += Map->RenderFlatWalls();

	// Draw wall decals
	ae::Graphics.SetProgram(ae::Assets.Programs["map"]);
	ae::Assets.Programs["map"]->ResetTextureTransform();
	ae::Graphics.SetDepthMask(false);
	ParticleRenderCount += Map->RenderParticles(_Particles::WALL_DECALS);

	// Draw particles
	ae::Graphics.EnableParticleBlending();
	ae::Graphics.SetProgram(ae::Assets.Programs["map"]);
	ae::Assets.Programs["map"]->ResetTextureTransform();
	Particles->Render(_Particles::NORMAL);
	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
	ae::Assets.Programs["pos_uv"]->ResetTextureTransform();
	Particles->Render(_Particles::EMISSIVE);
	ae::Graphics.DisableParticleBlending();

	// Draw the foreground tiles
	BlockRenderCount += Map->RenderForeground();

	// Draw the crosshair
	if(!Player->IsDying())
		HUD->DrawCrosshair(WorldCursor * (float)BlendFactor + PreviousWorldCursor * (float)(1.0f - BlendFactor));

	// Debug
	if(GodMode && DevMode && DebugMode) {
		ae::Graphics.SetDepthTest(false);

		// Draw monster target positions
		ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
		ae::Graphics.SetColor(glm::vec4(1, 0, 0, 1));
		for(const auto &Object : Map->ObjectManager->RenderList[_ObjectManager::RENDER_MONSTER]) {
			ae::Graphics.DrawCircle(glm::vec3(((_Entity *)Object)->TargetPosition, 0), 0.1f);
		}

		// Draw weapon ranges
		for(int i = 0; i < WEAPONATTACK_COUNT; i++) {
			glm::vec4 Color = COLOR_WHITE;
			if(i == 1)
				Color = COLOR_GREEN;

			float Range = Player->AttackRange[i];
			if(Range == 0.0f)
				Range = 100.0f;

			ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
			ae::Graphics.SetColor(Color);
			ae::Graphics.DrawCircle(glm::vec3(Player->Position, 0), Range);

			glm::vec2 Direction = Player->GetDirectionVector();
			glm::vec2 NormalDirection(-Direction.y, Direction.x);

			glm::vec2 LeftLineStart = Player->Position - NormalDirection * Player->AttackWidth[i];
			glm::vec2 LeftLineEnd = LeftLineStart + Direction * Range;
			ae::Graphics.DrawLine(LeftLineStart, LeftLineEnd);

			glm::vec2 RightLineStart = Player->Position + NormalDirection * Player->AttackWidth[i];
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

	// Draw damage text numbers
	Particles->Render(_Particles::TEXT);

	/*
	glm::ivec2 Start(Camera->GetAABB()[0], Camera->GetAABB()[1]);
	glm::ivec2 End(Camera->GetAABB()[2], Camera->GetAABB()[3]);

	for(int X = Start.x; X < End.x; X++) {
		for(int Y = Start.y; Y < End.y; Y++) {
			if(X > 0 && Y > 0) {
				glm::ivec2 P;
				Camera->ConvertWorldToScreen(glm::vec2(X-0.5f, Y-0.5f), P);
				std::ostringstream Buffer;
				size_t Count = 0;
				std::vector<_Event *> &Events = Map->GetEventList(glm::ivec2(X, Y));
				for(auto Event : Events) {
					if(Event->Active)
						Count++;

				}
				Buffer << Count << "/" << Events.size();
				Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), P.x, P.y);
				Buffer.str("");
			}
		}
	}*/

	// Render HUD
	HUD->Render(Camera, ae::FocusedElement == nullptr && ae::Actions.State[Action::GAME_MAP].Value > 0.0f);

	// Debug mode
	if(DebugMode) {
		glm::vec2 DrawPosition = glm::vec2(10, 200) * ae::_Element::GetUIScale();
		glm::vec2 Spacing(0, 16 * ae::_Element::GetUIScale());
		std::stringstream Buffer;
		Buffer << ae::Graphics.FramesPerSecond << " FPS";
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition);
		Buffer.str("");

		DrawPosition.y += Spacing.y;
		Buffer << BlockRenderCount << " blocks rendered";
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition);
		Buffer.str("");

		DrawPosition.y += Spacing.y;
		Buffer << Map->ObjectManager->RenderList[0].size() << " items rendered";
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition);
		Buffer.str("");

		DrawPosition.y += Spacing.y;
		Buffer << Map->ObjectManager->RenderList[2].size() << " monsters rendered";
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

	// Draw in-game menu
	if(IsPaused()) {
		Menu.Render();
	}
	// Draw death screen
	else if(Player->IsDead()) {
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

	// Reduce ammo
	if(!GodMode)
		Attacker->ReduceAmmo(1);

	// Weapon type specific code
	int WeaponType = WEAPON_MELEE;
	if(Attacker->AttackRequestType == WEAPONATTACK_MAIN)
		WeaponType = Attacker->MainWeaponType;

	// Play fire sound and generate fire/smoke particles
	if(WeaponType != WEAPON_MELEE) {
		_Hit Hit(HIT_NONE);
		GenerateHitEffects(Attacker, -1, Hit);
		if(Attacker->Type == _Object::PLAYER)
			ae::Audio.PlaySound(Attacker->GetSound(SOUND_FIRE, WEAPONATTACK_MAIN));
		else
			ae::Audio.PlaySound(Attacker->GetSound(SOUND_FIRE, WEAPONATTACK_MAIN), ae::_SoundSettings(glm::vec3(Attacker->Position.x, 0.0f, Attacker->Position.y)));
	}

	Attacker->StartTriggerDownAudio();

	// For each bullet that the weapon fires
	bool PlayedHitWallSound = false;
	std::unordered_map<_Object *, int> DecalObjects;
	std::vector<_Hit> Hits;
	Hits.reserve(Attacker->Penetration[Attacker->AttackRequestType]);
	for(int i = 0; i < Attacker->AttackCount[Attacker->AttackRequestType]; i++) {
		Hits.clear();

		// Check if gun was at min accuracy
		bool Steady = false;
		if(Attacker->AttackRequestType == WEAPONATTACK_MAIN && Attacker->IsSteady())
			Steady = true;

		// Check weapon type
		if(WeaponType == WEAPON_MELEE) {
			Map->CheckMeleeCollisions(Attacker, GridType, Attacker->Penetration[Attacker->AttackRequestType], Hits);
		}
		else {

			// Check distance to the wall
			float ShotDirection = Attacker->GenerateShotDirection();
			Map->CheckBulletCollisions(Attacker->Position, glm::rotate(glm::vec2(0, -1), glm::radians(ShotDirection)), Hits, GridType, true, Attacker->Penetration[Attacker->AttackRequestType], _Tile::BULLET);

			// Generate tracer particle
			_ParticleTemplate *Template = &GameAssets.Particles["tracer0"];
			glm::vec2 ParticleStart = Attacker->Position + glm::rotate(glm::vec2(0, -Template->Size.y * 0.5f) + Attacker->WeaponOffset[Attacker->MainWeaponType], glm::radians(ShotDirection));
			_Particle *Tracer = new _Particle(_ParticleSpawn(Template, glm::vec2(0), ParticleStart, OBJECT_Z, ShotDirection));
			float Distance = glm::length(Hits.front().Position - Attacker->Position) - Template->Size.y;
			Tracer->Lifetime = Distance * Template->VelocityScale.y * GAME_FPS;
			Particles->Add(Tracer);
		}

		// Generate particle effects and reduce health
		for(const auto &Hit : Hits) {
			switch(Hit.Type) {
				case HIT_NONE:
				break;
				case HIT_WALL:
					if(!PlayedHitWallSound) {
						ae::Audio.PlaySound(Attacker->GetSound(SOUND_RICOCHET, WEAPONATTACK_MAIN), ae::_SoundSettings(glm::vec3(Hit.Position.x, 0.0f, Hit.Position.y)));
						PlayedHitWallSound = true;
					}

					GenerateHitEffects(Attacker, HIT_WALL, Hit);
				break;
				case HIT_OBJECT: {

					// Generate damage
					bool Crit = false;
					int Damage = Attacker->GenerateDamage(Attacker->AttackRequestType, Hit.Object->DamageBlock, Hit.Object->DamageResist, Steady, Crit);
					if(GodMode && Hit.Object->Type == _Object::PLAYER)
						Damage = 0;

					// Create damage number particles
					glm::vec2 DamagePosition = Hit.Position;
					if(Hit.Object->Type ==  _Object::PLAYER)
						DamagePosition += _Map::GenerateRandomPointInCircle(0.3f);
					_Particle *DamageParticle = new _Particle(_ParticleSpawn(GameAssets.GetParticleTemplate("text0"), glm::vec2(0), DamagePosition, OBJECT_Z, 0));
					DamageParticle->Text = std::to_string(Damage);
					if(Hit.Object->Type ==  _Object::PLAYER)
						DamageParticle->Color = COLOR_RED;

					if(Crit)
						DamageParticle->Color = COLOR_YELLOW;
					Particles->Add(DamageParticle);

					// Update health
					Hit.Object->UpdateHealth(-Damage);
					if(Hit.Object->IsDying()) {

						// Handle item drops
						CreateItemDrop(Hit.Object, Player->DropRate * 0.01f);

						// Dying sound
						ae::Audio.PlaySound(Hit.Object->GetSound(SOUND_DEATH, -1), ae::_SoundSettings(glm::vec3(Hit.Position.x, 0.0f, Hit.Position.y)));

						// Update stats
						if(Attacker->Type == _Object::PLAYER) {
							_Monster *Monster = (_Monster *)Hit.Object;
							if(Monster->IsCrate()) {
								HUD->Crates[0]++;
							}
							else {
								HUD->Kills[0]++;
								Attacker->UpdateKillCount(1);
							}

							Attacker->UpdateExperience(Hit.Object->ExperienceGiven);
						}
					}

					// Generate bullet effects once for each hit object
					if(DecalObjects.find(Hit.Object) == DecalObjects.end()) {
						GenerateHitEffects(Attacker, HIT_OBJECT, Hit);
						DecalObjects[Hit.Object] = 1;
					}

					// Callback functions
					Attacker->OnAttack(Hit.Object, Hit);
					Hit.Object->OnHit(Attacker, Hit);

					// Set HUD last hit object
					if(Hit.Object->Type == _Object::MONSTER) {
						HUD->SetLastEntityHit(Hit.Object);
					}
				} break;
			}
		}
	}
}

// Handle pickup
void _PlayState::HandlePickup() {
	ClosestItem = nullptr;

	// Get nearby items
	std::unordered_map<_Object *, int> NearbyItems;
	Map->GetCloseObjects(Player->Position, Player->Radius, GRID_ITEM, NearbyItems, &ClosestItem);
	for(auto &Iterator : NearbyItems) {
		_Item *NearbyItem = (_Item *)Iterator.first;

		// Automatically pickup ammo
		if(NearbyItem && NearbyItem->IsAutoPickup()) {
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

	// Update monsters
	for(const auto &Entity : Monsters) {
		_Monster *Monster = (_Monster *)Entity;
		Monster->OnPlayerDeath();
	}
}

// Places an item into the player's inventory
void _PlayState::PickupObject(_Item *Item, int &AmountAdded) {
	if(!Item)
		return;

	// Attempt to add item
	int AddResult = Player->AddItem(Item, AmountAdded);
	if(AddResult) {
		if(Item == ClosestItem)
			ClosestItem = nullptr;

		Player->ResetUseTimer();
		Map->RemoveItem(Item);
		if(AddResult == 2) {
			delete Item;
			CursorItemTimer = 0;
		}
	}
	else {
		if(IgnoreItems.find(Item) == IgnoreItems.end()) {
			if(Item->Type == _Object::AMMO)
				HUD->ShowTextMessage("AMMO FULL", HUD_INVENTORYFULLTIME, false);
			else if(Item->Type == _Object::MEDKIT)
				HUD->ShowTextMessage("HEALTH FULL", HUD_INVENTORYFULLTIME, false);
			else
				HUD->ShowTextMessage("INVENTORY FULL", HUD_INVENTORYFULLTIME);
		}
	}
}

// Processes the use key to open doors, hit switches, and pickup items
void _PlayState::UseObject(_Item *Item) {
	if(!Player->CanPickup())
		return;

	// Pick up an item if available
	int AmountAdded = 0;
	PickupObject(Item, AmountAdded);
}

// Open door or handle switches
void _PlayState::ActivateEvent() {

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
				HUD->ShowMessageBox("You need the " + Stats.Objects.at(Event->ItemID).Name, HUD_KEY_MESSAGETIME);
				return;
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

		Player->ResetUseTimer();
	}
}

// Creates a random item from an entity
void _PlayState::CreateItemDrop(const _Entity *Entity, float DropRate) {
	if(Entity->Type != _Object::MONSTER)
		return;

	// Get monster
	_Monster *Monster = (_Monster *)Entity;
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
			if(Monster->AttackMade)
				ResolveAttack(Monster, GRID_PLAYER);

			// Add to render list
			if(Camera->IsAABBInView(Bounds))
				Map->ObjectManager->RenderList[2].push_back(Monster);

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
			case EVENT_ENDLEVEL:
				Level = Event->ItemID;

				// End of the game
				if(Level == "") {
					Level = GAME_FIRSTLEVEL;
					Player->Progression++;
					Framework.ChangeState(&NullState);
				}
				// Next level
				else
					Framework.ChangeState(&PlayState);

				Player->CheckpointIndex = Event->Level;
				Player->MapID = Level;
				if(Map->MapType == MAPTYPE_CAMPAIGN)
					Player->Keys.clear();
				Save.SavePlayer(Player);
			break;
			case EVENT_TEXT: {
				bool ShowMessage = true;
				bool IsTutorial = Event->ItemID.find("tutorial_") == 0;
				if(IsTutorial && (!Config.Tutorial || Player->Progression))
					ShowMessage = false;

				if(ShowMessage) {
					if(IsTutorial)
						ae::Audio.PlaySound(ae::Assets.Sounds["game_message0"]);
					HUD->ShowMessageBox(Stats.Strings[Event->ItemID], Event->ActivationPeriod);
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
				else {
					Map->SetAmbientLight(ae::Assets.Colors[Event->ItemID]);
					Map->SetAmbientLightChangePeriod(LIGHT_CHANGE_PERIOD);
				}
			} break;
			case EVENT_SECRET: {
				ae::Audio.PlaySound(ae::Assets.Sounds["game_secret0"]);
				HUD->ShowMessageBox("You have found a secret!", HUD_SECRET_MESSAGETIME);
				HUD->Secrets[0]++;
				Event->Active = false;
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
					for(int j = 0; j < Event->SpawnMultiplier; j++) {
						Position.x = Tiles[i].Coord.x + 0.5f + ae::GetRandomReal(-0.25f, 0.25f);
						Position.y = Tiles[i].Coord.y + 0.5f + ae::GetRandomReal(-0.25f, 0.25f);
						_Monster *Monster = Stats.CreateMonster(Event->MonsterID, Event->SpawnLevel + Player->GetAddedLevel(), Position);
						Monster->Player = Player;
						AddMonster(Monster);
						Particles->Create(_ParticleSpawn(GameAssets.GetParticleTemplate(Event->ParticleID), glm::vec2(0), Position, OBJECT_Z, 0));
					}
				}

				Decrement = true;
			} break;
			case EVENT_SOUND: {
				const std::vector<_EventTile> &Tiles = Event->Tiles;
				if(Tiles.size())
					ae::Audio.PlaySound(ae::Assets.Sounds[Event->ItemID], ae::_SoundSettings(glm::vec3(Tiles.front().Coord.x, 0, Tiles.front().Coord.y)));
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
				Map->SetAmbientLight(ae::Assets.Colors[Event->ItemID]);
				Map->SetAmbientLightChangePeriod(LIGHT_CHANGE_PERIOD);
				Decrement = true;
			} break;
			default:
			break;
		}

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
		_Monster *Monster = Stats.CreateMonster(ObjectSpawn->ID, ObjectSpawn->Level + Player->GetAddedLevel(), ObjectSpawn->Position);
		Monster->Player = Player;
		AddMonster(Monster);
		if(Monster->IsCrate())
			Map->Crates++;
		else
			Map->Monsters++;
	}
	else
		Map->AddItem(Stats.CreateItem(ObjectSpawn->ID, ObjectSpawn->Level + AddedLevel, 0, 1, ObjectSpawn->Position, GenerateStats));
}

// Adds a monster to the monster list and collision grid
void _PlayState::AddMonster(_Monster *Monster) {
	Monster->Map = Map;

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
void _PlayState::GenerateHitEffects(_Entity *Attacker, const int Type, const _Hit &Hit) {
	if(Type == -1) {
		glm::vec2 ParticlePosition = Attacker->Position + glm::rotate(Attacker->WeaponOffset[Attacker->MainWeaponType], glm::radians(Attacker->Rotation));
		Particles->Create(_ParticleSpawn(Attacker->GetParticle(PARTICLE_FIRE), glm::vec2(0), ParticlePosition, OBJECT_Z, Attacker->Rotation));
		Particles->Create(_ParticleSpawn(Attacker->GetParticle(PARTICLE_SMOKE), glm::vec2(0), ParticlePosition, OBJECT_Z, 0));
	}
	else if(Type == HIT_WALL) {
		Particles->Create(_ParticleSpawn(Attacker->GetParticle(PARTICLE_RICOCHET), Hit.Normal, Hit.Position, OBJECT_Z, Attacker->Rotation));
		Particles->Create(_ParticleSpawn(Attacker->GetParticle(PARTICLE_BULLETHOLE), Hit.Normal, Hit.Position, OBJECT_Z, Attacker->Rotation));
	}
	else if(Type == HIT_OBJECT) {
		glm::vec2 ParticlePosition = _Map::GenerateRandomPointInCircle(0.2f) + Hit.Object->Position;
		Particles->Create(_ParticleSpawn(Hit.Object->GetParticle(PARTICLE_HIT), Hit.Normal, Hit.Position, OBJECT_Z, Attacker->Rotation));
		Particles->Create(_ParticleSpawn(Hit.Object->GetParticle(PARTICLE_FLOORDECAL), Hit.Normal, ParticlePosition, 0.06f, Attacker->Rotation));
	}
}

// Determine if game is paused
bool _PlayState::IsPaused() {
	return Menu.GetState() != _Menu::STATE_NONE;
}
