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
#include <graphics.h>
#include <framework.h>
#include <menu.h>
#include <camera.h>
#include <constants.h>
#include <assets.h>
#include <hud.h>
#include <map.h>
#include <events.h>
#include <audio.h>
#include <config.h>
#include <actions.h>
#include <utils.h>
#include <particles.h>
#include <objects/entity.h>
#include <objects/player.h>
#include <objects/monster.h>
#include <objects/particle.h>
#include <objects/misc.h>
#include <objects/ammo.h>
#include <objects/upgrade.h>
#include <objects/weapon.h>
#include <objects/armor.h>
#include <states/editor.h>
#include <states/null.h>
#include <stdexcept>
#include <fstream>
#include <iostream>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/rotate_vector.hpp>

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
		Player->SetCheckpointIndex(CheckpointIndex);

	// Check for level override
	if(Level == "")
		Level = Player->GetMapIdentifier();

	// Load level
	Map = new _Map(Level);
	Map->Init();
	Player->SetMap(Map);
	Player->SetMapIdentifier(Map->GetFilename());

	// Set starting states
	Player->SetPosition(Map->GetStartingPositionByCheckpoint(Player->GetCheckpointIndex()));
	Player->TileChanged = true;
	Map->AddObjectToGrid(Player, GRID_PLAYER);

	// Get monster and item list
	std::vector<_ObjectSpawn *> Objects = Map->GetObjectsList();

	// Spawn objects
	for(size_t i = 0; i < Objects.size(); i++) {
		SpawnObject(Objects[i]);
	}

	// Initialize hud
	HUD = new _HUD(Player);

	// Set up graphics
	Camera = new _Camera(Player->Position, CAMERA_DISTANCE, CAMERA_DIVISOR);
	Map->SetCamera(Camera);

	Particles = new _Particles();
	Particles->SetCamera(Camera);

	Graphics.ChangeViewport(Graphics.GetScreenWidth(), Graphics.GetScreenHeight());
	Camera->CalculateFrustum(Graphics.GetAspectRatio());
	Graphics.ShowCursor(false);

	Actions.ResetState();
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
bool _PlayState::HandleAction(int InputType, int Action, int Value) {
	if(!Player || IsPaused())
		return false;

	if(Value) {
		if(!Player->IsDying()) {
			switch(Action) {
				case _Actions::INVENTORY:
					HUD->SetInventoryOpen(!HUD->GetInventoryOpen());
					Player->SetCrouching(false);
					Player->SetSprinting(false);
				break;
				case _Actions::FIRE:
					if(!HUD->GetInventoryOpen() && !Player->IsMeleeAttacking()) {
						if(Player->CanAttack(WEAPONATTACK_MAIN) && !Player->HasAmmo())
							Audio.Play(new _AudioSource(Audio.GetBuffer(Player->GetSample(SAMPLE_EMPTY))), Player->Position);

						if(Player->GetFireRate(WEAPONATTACK_MAIN) == FIRERATE_SEMI) {
							Player->AttackRequested = true;
							Player->AttackRequestType = WEAPONATTACK_MAIN;
						}
					}
				break;
				case _Actions::MELEE:
					if(!HUD->GetInventoryOpen()) {
						if(Player->GetFireRate(WEAPONATTACK_MELEE) == FIRERATE_SEMI) {
							Player->AttackRequested = true;
							Player->AttackRequestType = WEAPONATTACK_MELEE;
						}
					}
				break;
				case _Actions::RELOAD:
					if(!HUD->IsDragging()) {
						if(Player->IsReloading())
							Player->CancelReloading();
						else
							Player->StartReloading();
					}
				break;
				case _Actions::WEAPONSWITCH:
					if(!HUD->IsDragging())
						Player->StartWeaponSwitch(INVENTORY_MAINHAND, INVENTORY_OFFHAND);
				break;
				case _Actions::MEDKIT:
					Player->SetMedkitRequested(true);
				break;
			}
		}
		else {
			if(Action == _Actions::USE)
				RestartFromDeath();
		}
	}

	return false;
}

// Key handler
void _PlayState::KeyEvent(const _KeyEvent &KeyEvent) {
	if(IsPaused()) {
		Player->StopAudio();
		Menu.KeyEvent(KeyEvent);
		return;
	}

	if(KeyEvent.Pressed) {
		switch(KeyEvent.Key) {
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
							Framework.SetDone(true);

						Player->Save();
					}
					else
						Menu.InitInGame();
				}
			break;
			case SDL_SCANCODE_F1:
				Menu.InitInGame();
			break;
			case SDL_SCANCODE_GRAVE:
				//WorldCursor.Print();
				//IsFiring = !IsFiring;
				//Audio.Play(new _AudioSource(Audio.GetBuffer("player_hit0")), WorldCursor);
				//HUD->ShowTextMessage("CHECKPOINT REACHED", 5.0f);
				//_ParticleSpawn
				//Particles->Create(_ParticleSpawn(Assets.GetParticleTemplate("tracer0"), WorldCursor, OBJECT_Z, Player->GetDirection()));
			break;
		}
	}
}

// Mouse handler
void _PlayState::MouseEvent(const _MouseEvent &MouseEvent) {
	HUD->MouseEvent(MouseEvent);

	if(IsPaused())
		Menu.MouseEvent(MouseEvent);
}

// Update
void _PlayState::Update(double FrameTime) {

	// Handle pause
	if(IsPaused()) {
		Menu.Update(FrameTime);
		Graphics.ShowCursor(true);
		if(HUD)
			HUD->SetCursorOverItem(nullptr);

		return;
	}

	// Get world cursor
	PreviousWorldCursor = WorldCursor;
	Camera->ConvertScreenToWorld(Input.GetMouse(), WorldCursor);

	// Update save timer
	SaveGameTimer += FrameTime;

	// Handle input
	if(!Player->IsDying()) {

		// Turn character to face the world cursor
		Player->FacePosition(WorldCursor);

		// Move types
		if(Actions.GetState(_Actions::UP) && Actions.GetState(_Actions::LEFT))
			Player->SetMoveState(MOVE_FORWARDLEFT);
		else if(Actions.GetState(_Actions::UP) && Actions.GetState(_Actions::RIGHT))
			Player->SetMoveState(MOVE_FORWARDRIGHT);
		else if(Actions.GetState(_Actions::DOWN) && Actions.GetState(_Actions::LEFT))
			Player->SetMoveState(MOVE_BACKWARDLEFT);
		else if(Actions.GetState(_Actions::DOWN) && Actions.GetState(_Actions::RIGHT))
			Player->SetMoveState(MOVE_BACKWARDRIGHT);
		else if(Actions.GetState(_Actions::LEFT))
			Player->SetMoveState(MOVE_LEFT);
		else if(Actions.GetState(_Actions::RIGHT))
			Player->SetMoveState(MOVE_RIGHT);
		else if(Actions.GetState(_Actions::UP))
			Player->SetMoveState(MOVE_FORWARD);
		else if(Actions.GetState(_Actions::DOWN))
			Player->SetMoveState(MOVE_BACKWARD);
		else
			Player->SetMoveState(MOVE_NONE);

		// Attack or aim
		if(!HUD->GetInventoryOpen()) {

			// Attack again
			if(!Player->IsMeleeAttacking() && Player->GetFireRate(WEAPONATTACK_MAIN) == FIRERATE_AUTO && Actions.GetState(_Actions::FIRE)) {
				Player->AttackRequested = true;
				Player->AttackRequestType = WEAPONATTACK_MAIN;
			}
			if(Player->GetFireRate(WEAPONATTACK_MELEE) == FIRERATE_AUTO && Actions.GetState(_Actions::MELEE)) {
				Player->AttackRequested = true;
				Player->AttackRequestType = WEAPONATTACK_MELEE;
			}

			// Aim
			Player->SetCrouching(Actions.GetState(_Actions::AIM));
			Player->SetSprinting(Actions.GetState(_Actions::SPRINT));
		}

		Player->SetUseRequested(Actions.GetState(_Actions::USE));
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

	// Pickup up an object
	if(Player->GetUseRequested()) {
		UseObject();

		Player->SetUseRequested(false);
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
	Camera->SetPosition(Player->Position);

	// Get zoom state
	if(Player->IsCrouching()) {
		if(Map->IsVisible(Player->Position, WorldCursor)) {
			Camera->UpdatePosition((WorldCursor - Player->Position) / Player->GetZoomScale());
		}
		else {
			glm::vec2 Direction = WorldCursor - Player->Position;

			glm::vec2 NewPosition;
			Map->CheckBulletCollisions(Player->Position, Direction, nullptr, &NewPosition, 0, false);
			Camera->UpdatePosition((NewPosition - Player->Position) / Player->GetZoomScale());
		}
		Camera->SetDistance(CAMERA_DISTANCE_AIMED);
	}
	else
		Camera->SetDistance(CAMERA_DISTANCE);

	Camera->Update(FrameTime);

	// Get item at cursor
	PreviousCursorItem = CursorItem;
	CursorItem = static_cast<_Item *>(Map->CheckCollisionsInGrid(WorldCursor, 0.05f, GRID_ITEM, nullptr));
	if(CursorItem && CursorItem == PreviousCursorItem)
		CursorItemTimer += FrameTime;
	else
		CursorItemTimer = 0;

	// Update the HUD
	HUD->Update(FrameTime, Player->GetCrosshairRadius(WorldCursor));

	// Set cursor item
	if(CursorItem && !HUD->GetCursorOverItem() && (HUD->GetInventoryOpen() || CursorItemTimer > HUD_CURSOR_ITEM_WAIT))
		HUD->SetCursorOverItem(CursorItem);

	Audio.SetPosition(Player->Position);
}

// Render the state
void _PlayState::Render(double BlendFactor) {
	if(IsPaused())
		BlendFactor = 0;

	// Setup the viewing matrix
	Graphics.Setup3DViewport();
	Camera->Set3DProjection(BlendFactor);
	Graphics.EnableDepthTest();

	// Draw the floor
	Map->RenderFloors();

	Graphics.SetDepthMask(false);

	// Enable VBOs
	Graphics.EnableVBO(VBO_QUAD);
	Graphics.DisableDepthTest();

	// Draw floor decals
	Particles->Render(_Particles::FLOOR_DECALS);

	// Draw objects
	Map->RenderObjects(BlendFactor);

	// Disable VBOs
	Graphics.EnableDepthTest();
	Graphics.DisableVBO(VBO_QUAD);
	Graphics.SetDepthMask(true);

	// Draw the walls
	Map->RenderWalls();

	Graphics.SetDepthMask(false);

	Graphics.EnableVBO(VBO_QUAD);

	// Draw wall decals
	Particles->Render(_Particles::WALL_DECALS);

	// Draw particles
	Graphics.EnableParticleBlending();
	Particles->Render(_Particles::NORMAL);
	Graphics.DisableParticleBlending();
	Graphics.DisableVBO(VBO_QUAD);

	Graphics.DisableDepthTest();
	Particles->Render(_Particles::TEXT);
	Graphics.EnableDepthTest();

	Graphics.SetDepthMask(true);

	// Draw the foreground tiles
	Map->RenderForeground();

	// Draw the crosshair
	if(!Player->IsDying())
		HUD->RenderCrosshair(WorldCursor * (float)BlendFactor + PreviousWorldCursor * (float)(1.0f - BlendFactor));

	// Debug
	if(0) {
		Graphics.DisableDepthTest();

		// Draw melee hit range
		for(int i = 0; i < WEAPONATTACK_COUNT; i++) {
			_Color Color = COLOR_WHITE;
			if(i == 1)
				Color = COLOR_GREEN;

			float Range = Player->GetWeaponRange(i);
			if(Range == 0.0f)
				Range = 100.0f;
			Graphics.EnableVBO(VBO_CIRCLE);
			Graphics.DrawCircle(Player->Position.x, Player->Position.y, 0.2f, Range, Color);
			Graphics.DisableVBO(VBO_CIRCLE);
			glm::vec2 t1, t2;
			t1 = Player->Position + Player->GetDirectionVector(- Player->GetMaxAccuracy(i) / 2) * Range;
			t2 = Player->Position + Player->GetDirectionVector(+ Player->GetMaxAccuracy(i) / 2) * Range;
			glBegin(GL_LINES);
			glVertex2f(Player->Position.x, Player->Position.y);
			glVertex2f(t1.x, t1.y);
			glEnd();
			glBegin(GL_LINES);
			glVertex2f(Player->Position.x, Player->Position.y);
			glVertex2f(t2.x, t2.y);
			glEnd();
		}

		Graphics.EnableDepthTest();
	}

	// Setup OpenGL for drawing the HUD
	Graphics.Setup2DProjectionMatrix();
	/*
	_Coord Start(Camera->GetAABB()[0], Camera->GetAABB()[1]);
	_Coord End(Camera->GetAABB()[2], Camera->GetAABB()[3]);

	for(int X = Start.x; X < End.x; X++) {
		for(int Y = Start.y; Y < End.y; Y++) {
			if(X > 0 && Y > 0) {
				_Point P;
				Camera->ConvertWorldToScreen(glm::vec2(X-0.5f, Y-0.5f), P);
				std::ostringstream Buffer;
				size_t Count = 0;
				std::list<_Event *> &Events = Map->GetEventList(_Coord(X, Y));
				for(auto Event : Events) {
					if(Event->Active)
						Count++;

				}
				Buffer << Count << "/" << Events.size();
				Assets.GetFont("hud_tiny")->DrawText(Buffer.str(), P.x, P.y);
				Buffer.str("");
			}
		}
	}*/

	HUD->Render();

	if(IsPaused() || (Player && Player->IsDead()))
		Graphics.DrawRectangle(0, 0, Graphics.GetScreenWidth(), Graphics.GetScreenHeight(), _Color(0, 0, 0, GAME_PAUSE_FADEAMOUNT), true);

	// Draw in-game menu
	if(IsPaused()) {
		Menu.Render();
	}
	else if(Player && Player->IsDead()) {
		Graphics.ShowCursor(1);
		HUD->RenderDeathScreen();
	}

	Graphics.SetDepthMask(true);
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
	if(!Attacker->HasAmmo())
		return;

	// Reduce ammo
	Attacker->ReduceAmmo();

	// Weapon type specific code
	int WeaponType = WEAPON_MELEE;
	if(Attacker->AttackRequestType == 0)
		WeaponType = Attacker->GetWeaponType();

	// Play fire sound and generate fire/smoke particles
	HitStruct HitInformation;
	if(WeaponType != WEAPON_MELEE) {
		GenerateBulletEffects(Attacker, -1, HitInformation.Position);
		Audio.Play(new _AudioSource(Audio.GetBuffer(Attacker->GetSample(SAMPLE_FIRE))), Attacker->Position);
	}

	Attacker->StartTriggerDownAudio();

	// For each bullet that the weapon fires
	bool PlayedHitWallSound = false;
	for(int i = 0; i < Attacker->BulletsShot; i++) {
		HitInformation.Type = HIT_NONE;

		// Check weapon type
		if(WeaponType == WEAPON_MELEE) {

			HitInformation.Object = Map->CheckMeleeCollisions(Attacker, Attacker->GetDirectionVector(), GridType);
			if(HitInformation.Object != nullptr) {
				HitInformation.Type = HIT_OBJECT;
				HitInformation.Position = HitInformation.Object->Position;
			}
		}
		else {

			// Get bullet direction
			float ShotDirection = Attacker->GenerateShotDirection();

			// Check distance to the wall
			Map->CheckBulletCollisions(Attacker->Position, glm::rotate(glm::vec2(0, -1), glm::radians(ShotDirection)), &HitInformation.Object, &HitInformation.Position, GridType, true);
			if(HitInformation.Object != nullptr)
				HitInformation.Type = HIT_OBJECT;
			else
				HitInformation.Type = HIT_WALL;

			_ParticleTemplate *Template = Assets.GetParticleTemplate("tracer0");
			glm::vec2 ParticleStart = Attacker->Position + glm::rotate(glm::vec2(0, -Template->Size.y * 0.5f) + Attacker->GetWeaponOffset(Attacker->GetWeaponType()), glm::radians(ShotDirection));

			float Distance = glm::length(HitInformation.Position - Attacker->Position) - Template->Size.y;

			_Particle *Tracer = new _Particle(_ParticleSpawn(Template, ParticleStart, OBJECT_Z, ShotDirection));
			Tracer->Lifetime = Distance * Template->VelocityScale.y * GAME_FPS;
			Particles->Add(Tracer);
		}

		// Generate particle effects and reduce health
		switch(HitInformation.Type) {
			case HIT_NONE:
			break;
			case HIT_WALL:
				if(!PlayedHitWallSound) {
					Audio.Play(new _AudioSource(Audio.GetBuffer(Attacker->GetSample(SAMPLE_RICOCHET))), HitInformation.Position);
					PlayedHitWallSound = true;
				}

				GenerateBulletEffects(Attacker, HIT_WALL, HitInformation.Position);
			break;
			case HIT_OBJECT:
				GenerateBulletEffects(Attacker, HIT_OBJECT, HitInformation.Position);

				// Generate damage
				int Damage = Attacker->GenerateDamage(Attacker->AttackRequestType, HitInformation.Object->DamageBlock, HitInformation.Object->DamageResist);

				// Create damage number particles
				glm::vec2 DamagePosition = HitInformation.Position;
				if(HitInformation.Object->Type ==  _Object::PLAYER)
					DamagePosition += GenerateRandomPointInCircle(0.3f);
				_Particle *DamageParticle = new _Particle(_ParticleSpawn(Assets.GetParticleTemplate("damage0"), DamagePosition, OBJECT_Z, 0));
				DamageParticle->Text = std::to_string(Damage);
				if(HitInformation.Object->Type ==  _Object::PLAYER)
					DamageParticle->Color = COLOR_RED;
				Particles->Add(DamageParticle);

				// Update health
				HitInformation.Object->UpdateHealth(-Damage);
				if(HitInformation.Object->IsDying()) {
					Attacker->UpdateExperience(HitInformation.Object->ExperienceGiven);
					CreateItemDrop(HitInformation.Object);

					// Dying sound
					Audio.Play(new _AudioSource(Audio.GetBuffer(HitInformation.Object->GetSample(SAMPLE_DEATH))), HitInformation.Position);

					if(Attacker->Type == _Object::PLAYER)
						Attacker->UpdateKillCount(1);
				}

				// Weapon hit sound
				Audio.Play(new _AudioSource(Audio.GetBuffer(Attacker->GetSample(SAMPLE_HIT))), HitInformation.Position);

				// Entity hit sound
				Audio.Play(new _AudioSource(Audio.GetBuffer(HitInformation.Object->GetSample(SAMPLE_TAKEDAMAGE))), HitInformation.Position);

				// Set HUD last hit object
				if(HitInformation.Object->Type == _Object::MONSTER)
					HUD->SetLastEntityHit(HitInformation.Object);
			break;
		}
	}

	Attacker->AttackMade = false;
}

// Places an item into the player's inventory
void _PlayState::PickupObject() {
	_Item *HitItem;

	// Loop through the objects
	_TileBounds TB;
	Map->GetTileBounds(glm::vec2(0.8, 0.8), 0.5, TB);
	HitItem = (_Item *)Map->CheckCollisionsInGrid(Player->Position, Player->Radius, GRID_ITEM, nullptr);

	if(HitItem != nullptr) {
		int AddResult = Player->AddItem(HitItem);
		if(AddResult) {
			Map->RemoveItem(HitItem);
			if(AddResult == 2) {
				delete HitItem;
				CursorItemTimer = 0;
			}
			Player->ResetUseTimer();
		}
		else {
			HUD->ShowTextMessage(HUD_INVENTORYFULLMESSAGE, HUD_INVENTORYFULLTIME);
		}
	}
}

// Processes the use key to open doors and hit switches
void _PlayState::UseObject() {
	if(!Player->CanUse())
		return;

	// Pick up an item if available
	if(Player->CanPickup())
		PickupObject();

	// Open a door if possible
	_Coord Position;
	Map->GetAdjacentTile(Player->Position, Player->GetDirection(), Position);

	// Check for events
	std::list<_Event *> &Events = Map->GetEventList(Position);
	for(auto Event : Events) {

		// Check for doors or switches
		if(Event->Active && (Event->Type == EVENT_DOOR || Event->Type == EVENT_WSWITCH) && Map->CanChangeMapState(Event)) {

			// Check for key in inventory and use it
			if(Event->ItemIdentifier != "") {
				int ItemIndex = Player->FindItem(Event->ItemIdentifier);
				if(ItemIndex == -1) {
					if(Assets.IsMiscItemLoaded(Event->ItemIdentifier))
						HUD->ShowMessageBox("You need a " + Assets.GetMiscItemTemplate(Event->ItemIdentifier)->Name, HUD_KEYMESSAGETIME);

					return;
				}

				if(Player->UseItem(ItemIndex, true)) {
					HUD->ShowTextMessage("KEY USED", 2.0f);
				}
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

	_ItemGroup *ItemGroup = Assets.GetItemGroup(Entity->ItemGroupIdentifier);
	for(int i = 0; i < ItemGroup->Quantity; i++) {

		// Spawn random item
		_ObjectSpawn ObjectSpawn;
		ObjectSpawn.Position = GenerateRandomPointInCircle(PLAYER_RADIUS) + Entity->Position;

		// Roll for drop
		Assets.GetRandomDrop(ItemGroup, &ObjectSpawn);
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
	_Coord Position = Map->GetValidCoord(Entity->Position);

	// Check for events triggered by walking
	std::list<_Event *> &Events = Map->GetEventList(Position);
	for(auto Event : Events) {

		// Perform action
		if(Event->Active) {
			switch(Event->Type) {
				case EVENT_SPAWN:
					if(Assets.IsMonsterLoaded(Event->MonsterIdentifier)) {
						Event->StartTimer();
						ActiveEvents.push_back(Event);
					}
					Event->Active = false;
				break;
				case EVENT_CHECK:
					switch(Map->GetMapType()) {
						case MAPTYPE_SINGLE:
							if(Event->Level > Player->GetCheckpointIndex()) {
								Player->SetCheckpointIndex(Event->Level);
								HUD->ShowTextMessage(HUD_CHECKPOINTMESSAGE, HUD_CHECKPOINTTIME);
								Player->Save();
								SaveGameTimer = 0;
							}
							Event->Active = false;
						break;
						case MAPTYPE_TUTORIAL:
							if(Event->Level > Player->GetCheckpointIndex()) {
								Player->SetCheckpointIndex(Event->Level);
								HUD->ShowTextMessage(HUD_CHECKPOINTMESSAGE, HUD_CHECKPOINTTIME);
								Player->Save();
							}
							Event->Active = false;
						break;
						case MAPTYPE_ADVENTURE:
							if(Event->Level != Player->GetCheckpointIndex()) {
								Player->SetCheckpointIndex(Event->Level);
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

					Player->SetCheckpointIndex(Event->Level);
					Player->SetMapIdentifier(Level);
					Player->Save();
				break;
				case EVENT_TEXT:
					if(Assets.IsStringLoaded(Event->ItemIdentifier))
						HUD->ShowMessageBox(Assets.GetString(Event->ItemIdentifier), Event->ActivationPeriod);

					// Message with level of 0 = infinite
					if(Event->Level != 0)
						Event->Active = false;
				break;
				case EVENT_SOUND:
					if(Audio.GetBuffer(Event->ItemIdentifier)) {
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
						Particles->Create(_ParticleSpawn(Assets.GetParticleTemplate(Event->ParticleIdentifier), NewPosition, OBJECT_Z, 0));

						Map->RemoveObjectFromGrid(Player, GRID_PLAYER);
						Player->SetPosition(NewPosition);
						Map->AddObjectToGrid(Player, GRID_PLAYER);
					}
				} break;
				case EVENT_LIGHT: {
					if(LastLightEvent != Event) {
						Map->SetAmbientLight(Assets.GetColor(Event->ItemIdentifier));
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

		if(Event->TimerExpired()) {
			glm::vec2 Position;
			bool Decrement = false;
			switch(Event->Type) {
				case EVENT_SPAWN: {

					const std::vector<_EventTile> &Tiles = Event->Tiles;
					for(size_t i = 0; i < Tiles.size(); i++) {
						Position.x = static_cast<float>(Tiles[i].Coord.x) + 0.5f;
						Position.y = static_cast<float>(Tiles[i].Coord.y) + 0.5f;
						AddMonster(Assets.CreateMonster(Event->MonsterIdentifier, Position));
						Particles->Create(_ParticleSpawn(Assets.GetParticleTemplate(Event->ParticleIdentifier), Position, OBJECT_Z, 0));
					}

					Decrement = true;
				} break;
				case EVENT_SOUND:
					Audio.Play(new _AudioSource(Audio.GetBuffer(Event->ItemIdentifier), true));
					Decrement = true;
				break;
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
			AddMonster(Assets.CreateMonster(ObjectSpawn->Identifier, ObjectSpawn->Position));
		break;
		case _Object::MISCITEM:
			Map->AddItem(Assets.CreateMiscItem(ObjectSpawn->Identifier, 1, ObjectSpawn->Position));
		break;
		case _Object::AMMO:
			Map->AddItem(Assets.CreateAmmoItem(ObjectSpawn->Identifier, 1, ObjectSpawn->Position));
		break;
		case _Object::UPGRADE:
			Map->AddItem(Assets.CreateUpgradeItem(ObjectSpawn->Identifier, 1, ObjectSpawn->Position));
		break;
		case _Object::WEAPON:
			Map->AddItem(Assets.CreateWeapon(ObjectSpawn->Identifier, 1, ObjectSpawn->Position, GenerateStats));
		break;
		case _Object::ARMOR:
			Map->AddItem(Assets.CreateArmor(ObjectSpawn->Identifier, 1, ObjectSpawn->Position));
		break;
	}
}

// Adds a monster to the monster list and collision grid
void _PlayState::AddMonster(_Monster *Monster) {
	Monster->SetMap(Map);

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

void _PlayState::GenerateBulletEffects(_Entity *Attacker, const int Type, const glm::vec2 &Position) {
	glm::vec2 ParticlePosition;

	if(Type == -1) {

		// Particle position
		ParticlePosition = Attacker->Position + glm::rotate(Attacker->GetWeaponOffset(Attacker->GetWeaponType()), glm::radians(Attacker->GetDirection()));

		// Particles
		Particles->Create(_ParticleSpawn(Attacker->GetWeaponParticle(WEAPONPARTICLE_FIRE), ParticlePosition, OBJECT_Z, Attacker->GetDirection()));
		Particles->Create(_ParticleSpawn(Attacker->GetWeaponParticle(WEAPONPARTICLE_SMOKE), ParticlePosition, OBJECT_Z, 0));
	}
	else if(Type == HIT_WALL) {
		Particles->Create(_ParticleSpawn(Attacker->GetWeaponParticle(WEAPONPARTICLE_RICOCHET), Position, OBJECT_Z, Attacker->GetDirection()));
		Particles->Create(_ParticleSpawn(Attacker->GetWeaponParticle(WEAPONPARTICLE_BULLETHOLE), Position, OBJECT_Z, Attacker->GetDirection()));
	}
	else if(Type == HIT_OBJECT) {
		ParticlePosition = GenerateRandomPointInCircle(0.7f) + Position;

		// Blood
		Particles->Create(_ParticleSpawn(Assets.GetParticleTemplate("bloodspurt0"), Position, OBJECT_Z, Attacker->GetDirection()));
		Particles->Create(_ParticleSpawn(Assets.GetParticleTemplate("blood0"), ParticlePosition, 0.06f, Attacker->GetDirection()));
	}
}

bool _PlayState::IsPaused() { return Menu.GetState() != _Menu::STATE_NONE; }
