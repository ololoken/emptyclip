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
#include <ae/actions.h>
#include <ae/camera.h>
#include <ae/graphics.h>
#include <ae/graphics.h>
#include <ae/assets.h>
#include <ae/program.h>
#include <ae/console.h>
#include <ae/light.h>
#include <ae/audio.h>
#include <objects/entity.h>
#include <objects/player.h>
#include <objects/monster.h>
#include <objects/particle.h>
#include <objects/weapon.h>
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

_PlayState PlayState;

// Constructor
_PlayState::_PlayState() {
	Player = nullptr;
	Level = "";
	TestMode = false;
	FromEditor = false;
}

// Load level and set up objects
void _PlayState::Init() {
	ae::Graphics.SetViewport(ae::Graphics.CurrentSize);
	ae::Graphics.Element->SetActive(false);
	ae::Graphics.Element->Active = true;

	CursorItem = nullptr;
	PreviousCursorItem = nullptr;
	LastLightEvent = nullptr;
	SaveGameTimer = 0;

	// Check for player
	if(TestMode) {
		Player = Save.GetPlayer(_Save::SLOT_TEST);
	}

	// Bad player
	if(!Player)
		throw std::runtime_error("Player is nullptr");

	// Set checkpoint from editor
	if(FromEditor)
		Player->CheckpointIndex = CheckpointIndex;

	// Check for level override
	if(Level == "")
		Level = Player->MapIdentifier;

	// Load level
	Map = new _Map(Level);
	Map->Init();
	Player->Map = Map;
	Player->MapIdentifier = Map->GetFilename();

	// Set starting states
	Player->SetPosition(Map->GetStartingPositionByCheckpoint(Player->CheckpointIndex));
	Player->TileChanged = true;
	Map->AddObjectToGrid(Player, GRID_PLAYER);

	// Get monster and item list
	std::vector<_ObjectSpawn *> Objects = Map->GetObjectsList();

	// Spawn objects
	for(size_t i = 0; i < Objects.size(); i++) {
		SpawnObject(Objects[i]);
	}

	// Initialize objects
	HUD = new _HUD(Player);
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
}

// Close map
void _PlayState::Close() {

	DeleteMonsters();
	DeleteActiveEvents();

	Player->StopAudio();

	delete Particles;
	delete Camera;
	delete Map;
	delete HUD;
}

// Action handler
bool _PlayState::HandleAction(int InputType, std::size_t Action, int Value) {
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

	if(!Player || IsPaused())
		return false;

	if(!Player->IsDying()) {
		switch(Action) {
			case Action::GAME_INVENTORY:
				HUD->SetInventoryOpen(!HUD->GetInventoryOpen());
				Player->SetCrouching(false);
				Player->SetSprinting(false);
			break;
			case Action::GAME_FIRE:
				if(!HUD->GetInventoryOpen() && !Player->IsMeleeAttacking()) {

					// Play sound
					if(Player->CanAttack(WEAPONATTACK_MAIN) && !Player->WeaponHasAmmo())
						ae::Audio.PlaySound(ae::Assets.Sounds[Player->GetSample(SOUND_EMPTY)]);

					if(Player->GetFireRate(WEAPONATTACK_MAIN) == FIRERATE_SEMI) {
						Player->AttackRequested = true;
						Player->AttackRequestType = WEAPONATTACK_MAIN;
					}
				}
			break;
			case Action::GAME_MELEE:
				if(!HUD->GetInventoryOpen()) {
					if(Player->GetFireRate(WEAPONATTACK_MELEE) == FIRERATE_SEMI) {
						Player->AttackRequested = true;
						Player->AttackRequestType = WEAPONATTACK_MELEE;
					}
				}
			break;
			case Action::GAME_RELOAD:
				if(!HUD->IsDragging()) {
					if(Player->Reloading)
						Player->CancelReloading();
					else
						Player->StartReloading();
				}
			break;
			case Action::GAME_WEAPONSWITCH:
				if(!HUD->IsDragging())
					Player->StartWeaponSwitch(INVENTORY_MAINHAND, INVENTORY_OFFHAND);
			break;
			case Action::GAME_HEAL:
				Player->MedkitRequested = true;
			break;
		}
	}
	else {
		if(Action == Action::GAME_USE)
			RestartFromDeath();
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
					RestartFromDeath();
				}
				else if(!Player->IsDying()) {
					if(HUD->GetInventoryOpen()) {
						HUD->SetInventoryOpen(false);
					}
					else if(TestMode) {
						if(FromEditor)
							Framework.ChangeState(&EditorState);
						else
							Framework.Done = true;

						Player->Save();
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

	// Handle normal commands
	if(Console->Command == "quit") {
		HandleQuit();
		return true;
	}

	return false;
}

// Window size updates
void _PlayState::HandleWindow(uint8_t Event) {
	if(Event == SDL_WINDOWEVENT_SIZE_CHANGED) {
		if(Camera)
			Camera->CalculateFrustum(ae::Graphics.AspectRatio);

		Menu.HandleResize();
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

	// Handle pause
	if(IsPaused()) {
		Menu.Update(FrameTime);
		ae::Graphics.SetCursor(true);
		if(HUD)
			HUD->CursorOverItem = nullptr;

		return;
	}

	// Get world cursor
	PreviousWorldCursor = WorldCursor;
	Camera->ConvertScreenToWorld(ae::Input.GetMouse(), WorldCursor);

	// Update save timer
	SaveGameTimer += FrameTime;

	// Handle input
	if(!Player->IsDying() && ae::FocusedElement == nullptr) {

		// Turn character to face the world cursor
		Player->FacePosition(WorldCursor);

		// Move types
		if(ae::Actions.State[Action::GAME_UP].Value > 0.0f && ae::Actions.State[Action::GAME_LEFT].Value > 0.0f)
			Player->SetMoveState(MOVE_FORWARDLEFT);
		else if(ae::Actions.State[Action::GAME_UP].Value > 0.0f && ae::Actions.State[Action::GAME_RIGHT].Value > 0.0f)
			Player->SetMoveState(MOVE_FORWARDRIGHT);
		else if(ae::Actions.State[Action::GAME_DOWN].Value > 0.0f && ae::Actions.State[Action::GAME_LEFT].Value > 0.0f)
			Player->SetMoveState(MOVE_BACKWARDLEFT);
		else if(ae::Actions.State[Action::GAME_DOWN].Value > 0.0f && ae::Actions.State[Action::GAME_RIGHT].Value > 0.0f)
			Player->SetMoveState(MOVE_BACKWARDRIGHT);
		else if(ae::Actions.State[Action::GAME_LEFT].Value > 0.0f)
			Player->SetMoveState(MOVE_LEFT);
		else if(ae::Actions.State[Action::GAME_RIGHT].Value > 0.0f)
			Player->SetMoveState(MOVE_RIGHT);
		else if(ae::Actions.State[Action::GAME_UP].Value > 0.0f)
			Player->SetMoveState(MOVE_FORWARD);
		else if(ae::Actions.State[Action::GAME_DOWN].Value > 0.0f)
			Player->SetMoveState(MOVE_BACKWARD);
		else
			Player->SetMoveState(MOVE_NONE);

		// Attack or aim
		if(!HUD->GetInventoryOpen()) {

			// Attack again
			if(!Player->IsMeleeAttacking() && Player->GetFireRate(WEAPONATTACK_MAIN) == FIRERATE_AUTO && ae::Actions.State[Action::GAME_FIRE].Value > 0.0f) {
				Player->AttackRequested = true;
				Player->AttackRequestType = WEAPONATTACK_MAIN;
			}
			if(Player->GetFireRate(WEAPONATTACK_MELEE) == FIRERATE_AUTO && ae::Actions.State[Action::GAME_MELEE].Value > 0.0f) {
				Player->AttackRequested = true;
				Player->AttackRequestType = WEAPONATTACK_MELEE;
			}

			// Aim
			Player->SetCrouching(ae::Actions.State[Action::GAME_AIM].Value > 0.0f);
			Player->SetSprinting(ae::Actions.State[Action::GAME_SPRINT].Value > 0.0f);
		}

		Player->UseRequested = ae::Actions.State[Action::GAME_USE].Value;
	}
	else {
		HUD->SetInventoryOpen(false);
	}

	// Update the player's states
	Player->Update(FrameTime);

	// Check for events
	if(Player->TileChanged)
		CheckEvents(Player);
	Player->TileChanged = false;

	// Find nearest item
	_Item *NearbyItem = (_Item *)Map->CheckCollisionsInGrid(Player->Position, Player->Radius, GRID_ITEM, nullptr);

	// Automatically pickup ammo
	if(NearbyItem && NearbyItem->Type == _Object::AMMO) {
		PickupObject(NearbyItem);
	}
	// Manually pickup up an item
	else if(Player->UseRequested) {
		UseObject(NearbyItem);

		Player->UseRequested = false;
	}

	// Update objects
	Map->Update(FrameTime);
	UpdateMonsters(FrameTime);
	Particles->Update(FrameTime);
	Map->AddRenderList(Player, 1);

	// Update events
	UpdateEvents(FrameTime);

	// Apply the damage
	if(Player->AttackMade)
		EntityAttack(Player, GRID_MONSTER);

	// Update camera
	Camera->Set2DPosition(Player->Position);

	// Get zoom state
	if(Player->Crouching) {
		if(Map->IsVisible(Player->Position, WorldCursor)) {
			Camera->UpdatePosition((WorldCursor - Player->Position) / Player->ZoomScale);
		}
		else {
			_Hit Hit;
			Map->CheckBulletCollisions(Player->Position, WorldCursor - Player->Position, Hit, 0, false);
			Camera->UpdatePosition((Hit.Position - Player->Position) / Player->ZoomScale);
		}
		Camera->SetDistance(CAMERA_DISTANCE_AIMED);
	}
	else
		Camera->SetDistance(CAMERA_DISTANCE);

	Camera->Update(FrameTime);

	// Get item at cursor
	PreviousCursorItem = CursorItem;
	CursorItem = (_Item *)(Map->CheckCollisionsInGrid(WorldCursor, 0.05f, GRID_ITEM, nullptr));
	if(CursorItem && CursorItem == PreviousCursorItem)
		CursorItemTimer += FrameTime;
	else
		CursorItemTimer = 0;

	// Update the HUD
	HUD->Update(FrameTime, Player->GetCrosshairRadius(WorldCursor));

	// Set cursor item
	if(!ae::Graphics.Element->HitElement && CursorItem && !HUD->CursorOverItem && (HUD->GetInventoryOpen() || CursorItemTimer > HUD_CURSOR_ITEM_WAIT))
		HUD->CursorOverItem = CursorItem;

	ae::Audio.SetPosition(glm::vec3(Player->Position.x, 10, Player->Position.y));
}

// Render the state
void _PlayState::Render(double BlendFactor) {
	if(IsPaused())
		BlendFactor = 0;

	glm::vec3 LightPosition(glm::vec2(Player->Position), 1.0f);
	glm::vec4 AmbientLight(0.4f, 0.4f, 0.4f, 1.0f);

	ae::Assets.Programs["pos_uv"]->LightCount = 1;
	ae::Assets.Programs["pos_uv"]->Lights[0].Position = LightPosition;
	ae::Assets.Programs["pos_uv"]->Lights[0].Color = glm::vec4(0.5);
	ae::Assets.Programs["pos_uv"]->AmbientLight = Map->GetAmbientLight();
	ae::Assets.Programs["pos_uv_norm"]->LightCount = 1;
	ae::Assets.Programs["pos_uv_norm"]->Lights[0].Position = LightPosition;
	ae::Assets.Programs["pos_uv_norm"]->Lights[0].Color = glm::vec4(0.5);
	ae::Assets.Programs["pos_uv_norm"]->AmbientLight = AmbientLight;

	// Setup the viewing matrix
	ae::Graphics.Setup3D();
	Camera->Set3DProjection(BlendFactor);

	// Setup the viewing matrix
	ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
	glUniformMatrix4fv(ae::Assets.Programs["pos"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
	glUniformMatrix4fv(ae::Assets.Programs["pos_uv"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv_norm"]);
	glUniformMatrix4fv(ae::Assets.Programs["pos_uv_norm"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	ae::Graphics.SetProgram(ae::Assets.Programs["text"]);
	glUniformMatrix4fv(ae::Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));

	// Draw the floor
	Map->RenderFloors();

	// Draw floor decals
	ae::Assets.Programs["pos_uv"]->ResetTextureTransform();
	ae::Graphics.SetDepthMask(false);
	ae::Graphics.SetDepthTest(false);
	int ParticleRenderCount = Map->RenderParticles(_Particles::FLOOR_DECALS);

	// Draw walls clipped with MaxZ=OBJECT_Z
	Map->RenderWalls();

	// Draw objects
	Map->RenderObjects(BlendFactor);

	// Draw the rest of the walls
	Map->RenderWalls();
	Map->RenderFlatWalls();

	// Draw wall decals
	ae::Assets.Programs["pos_uv"]->ResetTextureTransform();
	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
	ae::Graphics.SetDepthMask(false);
	ParticleRenderCount += Map->RenderParticles(_Particles::WALL_DECALS);
	//std::cout << ParticleRenderCount << std::endl;

	// Draw particles
	ae::Graphics.EnableParticleBlending();
	Particles->Render(_Particles::NORMAL);

	// Draw damage text numbers
	ae::Graphics.DisableParticleBlending();
	ae::Graphics.SetDepthTest(false);
	Particles->Render(_Particles::TEXT);

	// Draw the foreground tiles
	Map->RenderForeground();

	// Draw the crosshair
	if(!Player->IsDying())
		HUD->RenderCrosshair(WorldCursor * (float)BlendFactor + PreviousWorldCursor * (float)(1.0f - BlendFactor));

	// Debug
	if(0) {
		ae::Graphics.SetDepthTest(false);

		// Draw melee hit range
		for(int i = 0; i < WEAPONATTACK_COUNT; i++) {
			glm::vec4 Color = COLOR_WHITE;
			if(i == 1)
				Color = COLOR_GREEN;

			float Range = Player->GetWeaponRange(i);
			if(Range == 0.0f)
				Range = 100.0f;
			ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
			ae::Graphics.SetColor(Color);
			ae::Graphics.DrawCircle(glm::vec3(Player->Position, 0), Range);
			glm::vec2 t1, t2;
			t1 = Player->Position + Player->GetDirectionVector(-Player->GetMaxAccuracy(i) * 0.5f) * Range;
			t2 = Player->Position + Player->GetDirectionVector(Player->GetMaxAccuracy(i) * 0.5f) * Range;
			ae::Graphics.DrawLine(Player->Position, t1);
			ae::Graphics.DrawLine(Player->Position, t2);
		}
	}

	// Setup OpenGL for drawing the HUD
	ae::Graphics.Setup2D();
	ae::Graphics.SetStaticUniforms();
	ae::Graphics.SetDepthTest(false);
	ae::Graphics.SetDepthMask(false);

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
				std::list<_Event *> &Events = Map->GetEventList(glm::ivec2(X, Y));
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

	HUD->Render();

	if(IsPaused() || (Player && Player->IsDead()))
		ae::Graphics.FadeScreen(ae::Assets.Programs["ortho_pos"], GAME_PAUSE_FADEAMOUNT);

	// Draw in-game menu
	if(IsPaused()) {
		Menu.Render();
	}
	else if(Player && Player->IsDead()) {
		ae::Graphics.SetCursor(1);
		HUD->RenderDeathScreen();
	}
}

// Restart the level after death
void _PlayState::RestartFromDeath() {
	try {
		Player->Load();
	}
	catch(std::exception &Error) {
	}

	Framework.ChangeState(&PlayState);
}

// Fires a gun or swings a weapon
void _PlayState::EntityAttack(_Entity *Attacker, int GridType) {

	// Check for ammo
	if(!Attacker->WeaponHasAmmo())
		return;

	// Reduce ammo
	Attacker->ReduceAmmo();

	// Weapon type specific code
	int WeaponType = WEAPON_MELEE;
	if(Attacker->AttackRequestType == 0)
		WeaponType = Attacker->GetWeaponType();

	// Play fire sound and generate fire/smoke particles
	_Hit Hit;
	if(WeaponType != WEAPON_MELEE) {
		GenerateBulletEffects(Attacker, -1, Hit);
		ae::Audio.PlaySound(ae::Assets.Sounds[Attacker->GetSample(SOUND_FIRE)], glm::vec3(Attacker->Position.x, 0.0f, Attacker->Position.y));
	}

	Attacker->StartTriggerDownAudio();

	// For each bullet that the weapon fires
	bool PlayedHitWallSound = false;
	for(int i = 0; i < Attacker->AttackCount; i++) {
		Hit.Type = HIT_NONE;

		// Check weapon type
		if(WeaponType == WEAPON_MELEE) {

			Hit.Object = Map->CheckMeleeCollisions(Attacker, Attacker->GetDirectionVector(), GridType);
			if(Hit.Object != nullptr) {
				Hit.Type = HIT_OBJECT;
				Hit.Position = Hit.Object->Position;
			}
		}
		else {

			// Get bullet direction
			float ShotDirection = Attacker->GenerateShotDirection();

			// Check distance to the wall
			Map->CheckBulletCollisions(Attacker->Position, glm::rotate(glm::vec2(0, -1), glm::radians(ShotDirection)), Hit, GridType, true);
			if(Hit.Object != nullptr)
				Hit.Type = HIT_OBJECT;
			else
				Hit.Type = HIT_WALL;

			_ParticleTemplate *Template = GameAssets.GetParticleTemplate("tracer0");
			glm::vec2 ParticleStart = Attacker->Position + glm::rotate(glm::vec2(0, -Template->Size.y * 0.5f) + Attacker->GetWeaponOffset(Attacker->GetWeaponType()), glm::radians(ShotDirection));

			float Distance = glm::length(Hit.Position - Attacker->Position) - Template->Size.y;

			_Particle *Tracer = new _Particle(_ParticleSpawn(Template, glm::vec2(0), ParticleStart, OBJECT_Z, ShotDirection));
			Tracer->Lifetime = Distance * Template->VelocityScale.y * GAME_FPS;
			Particles->Add(Tracer);
		}

		// Generate particle effects and reduce health
		switch(Hit.Type) {
			case HIT_NONE:
			break;
			case HIT_WALL:
				if(!PlayedHitWallSound) {
					ae::Audio.PlaySound(ae::Assets.Sounds[Attacker->GetSample(SOUND_RICOCHET)], glm::vec3(Hit.Position.x, 0.0f, Hit.Position.y));
					PlayedHitWallSound = true;
				}

				GenerateBulletEffects(Attacker, HIT_WALL, Hit);
			break;
			case HIT_OBJECT:
				GenerateBulletEffects(Attacker, HIT_OBJECT, Hit);

				// Generate damage
				int Damage = Attacker->GenerateDamage(Attacker->AttackRequestType, Hit.Object->DamageBlock, Hit.Object->DamageResist);

				// Create damage number particles
				glm::vec2 DamagePosition = Hit.Position;
				if(Hit.Object->Type ==  _Object::PLAYER)
					DamagePosition += _Map::GenerateRandomPointInCircle(0.3f);
				_Particle *DamageParticle = new _Particle(_ParticleSpawn(GameAssets.GetParticleTemplate("damage0"), glm::vec2(0), DamagePosition, OBJECT_Z, 0));
				DamageParticle->Text = std::to_string(Damage);
				if(Hit.Object->Type ==  _Object::PLAYER)
					DamageParticle->Color = COLOR_RED;
				Particles->Add(DamageParticle);

				// Update health
				Hit.Object->UpdateHealth(-Damage);
				if(Hit.Object->IsDying()) {

					// Handle item drops
					CreateItemDrop(Hit.Object);

					// Dying sound
					ae::Audio.PlaySound(ae::Assets.Sounds[Hit.Object->GetSample(SOUND_DEATH)], glm::vec3(Hit.Position.x, 0.0f, Hit.Position.y));

					// Update stats
					if(Attacker->Type == _Object::PLAYER) {
						Attacker->UpdateKillCount(1);
						Attacker->UpdateExperience(Hit.Object->ExperienceGiven);
					}
				}

				// Weapon hit sound
				ae::Audio.PlaySound(ae::Assets.Sounds[Attacker->GetSample(SOUND_HIT)], glm::vec3(Hit.Position.x, 0.0f, Hit.Position.y));

				// Entity hit sound
				ae::Audio.PlaySound(ae::Assets.Sounds[Hit.Object->GetSample(SOUND_TAKEDAMAGE)], glm::vec3(Hit.Position.x, 0.0f, Hit.Position.y));

				// Set HUD last hit object
				if(Hit.Object->Type == _Object::MONSTER)
					HUD->SetLastEntityHit(Hit.Object);
			break;
		}
	}

	Attacker->AttackMade = false;
}

// Places an item into the player's inventory
void _PlayState::PickupObject(_Item *NearbyItem) {
	if(!NearbyItem)
		return;

	// Attempt to add item
	int AddResult = Player->AddItem(NearbyItem);
	if(AddResult) {
		Player->ResetUseTimer();
		Map->RemoveItem(NearbyItem);
		if(AddResult == 2) {
			delete NearbyItem;
			CursorItemTimer = 0;
		}
	}
	else
		HUD->ShowTextMessage(HUD_INVENTORYFULLMESSAGE, HUD_INVENTORYFULLTIME);
}

// Processes the use key to open doors, hit switches, and pickup items
void _PlayState::UseObject(_Item *NearbyItem) {
	if(!Player->CanUse())
		return;

	// Pick up an item if available
	if(Player->CanPickup())
		PickupObject(NearbyItem);

	// Open a door if possible
	glm::ivec2 Position;
	Map->GetAdjacentTile(Player->Position, Player->Rotation, Position);

	// Check for events
	std::list<_Event *> &Events = Map->GetEventList(Position);
	for(auto Event : Events) {

		// Check for doors or switches
		if(Event->Active && (Event->Type == EVENT_DOOR || Event->Type == EVENT_WSWITCH) && Map->CanChangeMapState(Event)) {

			// Check for key in inventory and use it
			if(Event->ItemIdentifier != "") {
				int ItemIndex = Player->FindItem(Event->ItemIdentifier);
				if(ItemIndex == -1) {
					HUD->ShowMessageBox("You need the " + Stats.Items[Event->ItemIdentifier].Name, HUD_KEYMESSAGETIME);
					return;
				}

				if(Player->UseItem(ItemIndex, true))
					HUD->ShowTextMessage("KEY USED", 2.0f);
			}

			// Change map
			Map->ChangeMapState(Event);

			// Decrement level
			if(Event->Level > 0) {
				Event->Decrement();
				if(Event->Level == 0)
					Event->Active = false;
			}

			Player->ResetUseTimer();
		}
	}
}

// Creates a random item from an entity
void _PlayState::CreateItemDrop(const _Entity *Entity) {
	if(Entity->ItemGroupIdentifier == "")
		return;

	_ItemGroup *ItemGroup = &Stats.ItemGroups[Entity->ItemGroupIdentifier];
	for(int i = 0; i < ItemGroup->Quantity; i++) {

		// Spawn random item
		_ObjectSpawn ObjectSpawn;
		ObjectSpawn.Position = _Map::GenerateRandomPointInCircle(PLAYER_RADIUS) + Entity->Position;

		// Roll for drop
		Stats.GetRandomDrop(ItemGroup, &ObjectSpawn);
		SpawnObject(&ObjectSpawn, true);
	}
}

// Updates the monsters
void _PlayState::UpdateMonsters(double FrameTime) {

	// Loop through monsters
	for(auto MonsterIterator = Monsters.begin(); MonsterIterator != Monsters.end();) {
		_Monster *Monster = (_Monster *)*MonsterIterator;

		if(!Monster->Active) {
			Map->RemoveObjectFromGrid(Monster, GRID_MONSTER);
			delete Monster;
			MonsterIterator = Monsters.erase(MonsterIterator);
		}
		else {
			Monster->UpdateMonster(FrameTime, Player);

			if(Monster->AttackMade) {
				EntityAttack(Monster, GRID_PLAYER);
			}

			if(Camera->IsCircleInView(Monster->Position, Monster->Scale)) {
				Map->AddRenderList(Monster, 2);
			}

			++MonsterIterator;
		}
	}
}

// Checks for the player triggering events
void _PlayState::CheckEvents(const _Entity *Entity) {
	glm::ivec2 Position = Map->GetValidCoord(Entity->Position);

	// Check for events triggered by walking
	std::list<_Event *> &Events = Map->GetEventList(Position);
	for(auto Event : Events) {

		// Perform action
		if(Event->Active) {
			switch(Event->Type) {
				case EVENT_SPAWN:
					if(Stats.Monsters.find(Event->MonsterIdentifier) != Stats.Monsters.end()) {
						Event->StartTimer();
						ActiveEvents.push_back(Event);
					}
					Event->Active = false;
				break;
				case EVENT_CHECK:
					switch(Map->GetMapType()) {
						case MAPTYPE_CAMPAIGN:
							if(Event->Level > Player->CheckpointIndex) {
								Player->CheckpointIndex = Event->Level;
								HUD->ShowTextMessage(HUD_CHECKPOINTMESSAGE, HUD_CHECKPOINTTIME);
								Player->Save();
								SaveGameTimer = 0;
							}
							Event->Active = false;
						break;
						case MAPTYPE_ADVENTURE:
							if(Event->Level != Player->CheckpointIndex) {
								Player->CheckpointIndex = Event->Level;
								HUD->ShowTextMessage(HUD_CHECKPOINTMESSAGE, HUD_CHECKPOINTTIME);
								Player->Save();
							}
						break;
						default:
						break;
					}
				break;
				case EVENT_END:
					Level = Event->ItemIdentifier;

					// End of the game
					if(Level == "") {
						Level = GAME_FIRSTLEVEL;
						Framework.ChangeState(&NullState);
					}
					// Next level
					else
						Framework.ChangeState(&PlayState);

					Player->CheckpointIndex = Event->Level;
					Player->MapIdentifier = Level;
					Player->Save();
				break;
				case EVENT_TEXT:
					HUD->ShowMessageBox(Stats.Strings[Event->ItemIdentifier], Event->ActivationPeriod);
					if(Event->Level != 0)
						Event->Active = false;
				break;
				case EVENT_SOUND:
					if(ae::Assets.Sounds[Event->ItemIdentifier]) {
						Event->StartTimer();
						ActiveEvents.push_back(Event);
					}
					Event->Active = false;
				break;
				case EVENT_FSWITCH:
				case EVENT_ENABLE:
					Event->StartTimer();
					ActiveEvents.push_back(Event);
					Event->Active = false;
				break;
				case EVENT_TELE: {
					if(Event->Level > 0) {
						Event->Decrement();
						if(Event->Level == 0)
							Event->Active = false;
					}

					if(Event->Tiles.size() > 0) {
						glm::vec2 NewPosition(Event->Tiles[0].Coord.x + 0.5f, Event->Tiles[0].Coord.y + 0.5f);
						Particles->Create(_ParticleSpawn(GameAssets.GetParticleTemplate(Event->ParticleIdentifier), glm::vec2(0), NewPosition, OBJECT_Z, 0));

						Map->RemoveObjectFromGrid(Player, GRID_PLAYER);
						Player->SetPosition(NewPosition);
						Map->AddObjectToGrid(Player, GRID_PLAYER);
					}
				} break;
				case EVENT_LIGHT: {
					if(LastLightEvent != Event) {
						Map->SetAmbientLight(ae::Assets.Colors[Event->ItemIdentifier]);
						Map->SetAmbientLightChangePeriod(Event->ActivationPeriod);
						LastLightEvent = Event;
					}
				} break;
				default:
				break;
			}
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
					Position.x = Tiles[i].Coord.x + 0.5f;
					Position.y = Tiles[i].Coord.y + 0.5f;
					AddMonster(Stats.CreateMonster(Event->MonsterIdentifier, Position));
					Particles->Create(_ParticleSpawn(GameAssets.GetParticleTemplate(Event->ParticleIdentifier), glm::vec2(0), Position, OBJECT_Z, 0));
				}

				Decrement = true;
			} break;
			case EVENT_SOUND: {
				ae::Audio.PlaySound(ae::Assets.Sounds[Event->ItemIdentifier]);
				Decrement = true;
			} break;
			case EVENT_FSWITCH:
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

	for(auto Iterator : Monsters)
		if(Iterator)
			delete Iterator;

	Monsters.clear();
}

// Deletes the active events
void _PlayState::DeleteActiveEvents() {

	ActiveEvents.clear();
}

// Spawn an object in the map
void _PlayState::SpawnObject(_ObjectSpawn *ObjectSpawn, bool GenerateStats) {
	switch(ObjectSpawn->Type) {
		case _Object::MONSTER:
			AddMonster(Stats.CreateMonster(ObjectSpawn->Identifier, ObjectSpawn->Position));
		break;
		case _Object::KEY:
		case _Object::AMMO:
		case _Object::UPGRADE:
		case _Object::ARMOR:
		case _Object::MEDKIT:
			Map->AddItem(Stats.CreateItem(ObjectSpawn->Identifier, 1, ObjectSpawn->Position));
		break;
		case _Object::WEAPON:
			Map->AddItem(Stats.CreateWeapon(ObjectSpawn->Identifier, 1, ObjectSpawn->Position, GenerateStats));
		break;
	}
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
void _PlayState::GenerateBulletEffects(_Entity *Attacker, const int Type, const _Hit &Hit) {
	if(Type == -1) {
		glm::vec2 ParticlePosition = Attacker->Position + glm::rotate(Attacker->GetWeaponOffset(Attacker->GetWeaponType()), glm::radians(Attacker->Rotation));
		Particles->Create(_ParticleSpawn(Attacker->GetWeaponParticle(WEAPONPARTICLE_FIRE), glm::vec2(0), ParticlePosition, OBJECT_Z, Attacker->Rotation));
		Particles->Create(_ParticleSpawn(Attacker->GetWeaponParticle(WEAPONPARTICLE_SMOKE), glm::vec2(0), ParticlePosition, OBJECT_Z, 0));
	}
	else if(Type == HIT_WALL) {
		Particles->Create(_ParticleSpawn(Attacker->GetWeaponParticle(WEAPONPARTICLE_RICOCHET), Hit.Normal, Hit.Position, OBJECT_Z, Attacker->Rotation));
		Particles->Create(_ParticleSpawn(Attacker->GetWeaponParticle(WEAPONPARTICLE_BULLETHOLE), Hit.Normal, Hit.Position, OBJECT_Z, Attacker->Rotation));
	}
	else if(Type == HIT_OBJECT) {
		glm::vec2 ParticlePosition = _Map::GenerateRandomPointInCircle(0.7f) + Hit.Position;
		Particles->Create(_ParticleSpawn(GameAssets.GetParticleTemplate("bloodspurt0"), Hit.Normal, Hit.Position, OBJECT_Z, Attacker->Rotation));
		Particles->Create(_ParticleSpawn(GameAssets.GetParticleTemplate("blood0"), Hit.Normal, ParticlePosition, 0.06f, Attacker->Rotation));
	}
}

bool _PlayState::IsPaused() { return Menu.GetState() != _Menu::STATE_NONE; }
