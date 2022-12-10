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

// Includes
#include <string>
#include <SDL_keycode.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

//     Defaults
const  glm::ivec2   DEFAULT_WINDOW_SIZE                  =  glm::ivec2(1440,900);
const  int          DEFAULT_FULLSCREEN                   =  1;
const  int          DEFAULT_AUDIOENABLED                 =  1;
const  int          DEFAULT_VSYNC                        =  1;
const  int          DEFAULT_MSAA                         =  0;
const  int          DEFAULT_ANISOTROPY                   =  1.0f;
const  double       DEFAULT_MAXFPS                       =  240.0;
//     Config
const  int          CONFIG_VERSION                       =  3;
//     Game
const  std::string  GAME_WINDOWTITLE                     =  "Empty Clip";
const  double       GAME_FPS                             =  100.0;
const  double       GAME_TIMESTEP                        =  1.0/GAME_FPS;
const  double       GAME_MAX_FRAMETIME                   =  1.0/30.0;
const  float        GAME_PAUSE_FADEAMOUNT                =  0.7f;
const  std::string  GAME_FIRSTLEVEL                      =  "c01.map";
const  int          GAME_PLAYERLEVEL_SOFTCAP             =  201;
const  int          GAME_SKILL_SOFTCAP                   =  100;
const  int          GAME_MAX_SKILL_PERLEVEL              =  3;
const  double       GAME_INVULNERABLE_TIME               =  3.0;
const  int          GAME_MAX_PROGRESSION                 =  1000;
const  double       GAME_DEFAULT_CLOCK                   =  720.0;
const  float        GAME_TILE_SIZE                       =  64.0f;
const  double       GAME_COMBAT_TIMER                    =  5.0;
const  int          GAME_QUALITY_RANGE                   =  15;
//     Save
const  int          SAVE_SLOTS                           =  10;
const  int          SAVE_VERSION                         =  8;
const  int          SAVE_VERSION_NEW                     =  8;
const  double       SAVE_TIME                            =  60.0;
//     Benchmark
const  double       BENCH_TIME                           =  10.0;
//     Camera
const  float        CAMERA_DISTANCE                      =  6.5f;
const  float        CAMERA_DISTANCE_AIMED                =  CAMERA_DISTANCE;
const  float        CAMERA_DIVISOR                       =  0.2f;
const  float        CAMERA_SNAPPING_THRESHOLD            =  0.0001f;
const  float        CAMERA_EDITOR_DIVISOR                =  0.05f;
const  float        CAMERA_FOVY                          =  90.0f;
const  float        CAMERA_NEAR                          =  0.1f;
const  float        CAMERA_FAR                           =  500.0f;
//     Weapons
const  double       WEAPON_MINFIREPERIOD                 =  GAME_TIMESTEP;
//     Audio
const  float        AUDIO_REFERENCE_DISTANCE             =  10.0f;
const  float        AUDIO_MAX_DISTANCE                   =  500.0f;
const  float        AUDIO_ROLL_OFF                       =  5.0f;
const  float        AUDIO_POSITION_Z                     =  10.0f;
//     Entities
const  float        ENTITY_PUSH_FACTOR                   =  0.4f;
const  float        ENTITY_MOVESOUNDDELAYFACTOR          =  0.02625f;
const  int          ENTITY_MINDAMAGEPOINTS               =  1;
const  float        ENTITY_STOP_THRESHOLD                =  0.005f;
const  double       ENTITY_STATIC_TIME                   =  2.0;
const  float        ENTITY_MAX_ACTIVE_RANGE              =  30.0f;
const  float        ENTITY_MAX_DAMAGE_RESIST             =  90.0f;
const  int          ENTITY_MAX_MOVESPEED_LEVEL           =  100;
const  double       ENTITY_FREEPATHING_TIMER_INCREMENT   =  0.1;
const  double       ENTITY_MAX_FIRESOUND_PERIOD          =  0.03;
const  float        ENTITY_FRICTION_FACTOR               =  0.003f;
const  float        ENTITY_VELOCITY_THRESHOLD            =  0.00001f;
const  double       ENTITY_CRATE_COMBATTIME              =  1.0;
//     Player
const  float        PLAYER_RADIUS                        =  0.35f;
const  float        PLAYER_MASS                          =  1.0f;
const  double       PLAYER_HEAL_DELAY                    =  5.0;
const  double       PLAYER_HEAL_PERIOD                   =  1.0;
const  float        PLAYER_HEAL_THRESHOLD                =  0.75;
const  float        PLAYER_HEAL_PERCENT                  =  2.0f;
const  float        PLAYER_STARTING_HEALTH_FACTOR        =  0.5f;
const  float        PLAYER_LEGCHANGEFACTOR               =  0.2f;
const  float        PLAYER_MOVESPEED                     =  4.5f;
const  float        PLAYER_BACKWARDS_SPEEDFACTOR         =  0.5f;
const  float        PLAYER_PUSH_FACTOR                   =  0.5f;
const  float        PLAYER_REACH_DISTANCE                =  1.3f;
const  float        PLAYER_REACH_DISTANCE_SQUARED        =  PLAYER_REACH_DISTANCE*PLAYER_REACH_DISTANCE;
const  int          PLAYER_CRIT_DAMAGE                   =  200;
const  int          PLAYER_STEADY_CRIT_FACTOR            =  2;
const  double       PLAYER_SHOOT_PERIOD                  =  0.5;
const  double       PLAYER_TAKEDAMAGE_SOUND_COOLDOWN     =  1.0;
const  float        PLAYER_AIM_MOVESPEEDFACTOR           =  0.333333f;
const  float        PLAYER_AIM_RECOIL_MODIFIER           =  0.5f;
const  float        PLAYER_SPRINT_RECOIL_MODIFIER        =  2.0f;
const  float        PLAYER_SPRINT_SPEEDFACTOR            =  1.65f;
const  glm::vec2    PLAYER_PISTOLOFFSET                  =  glm::vec2(36.0f/GAME_TILE_SIZE-0.5f,-0.5f);
const  glm::vec2    PLAYER_WEAPONOFFSET                  =  glm::vec2(30.0f/GAME_TILE_SIZE-0.5f,-0.5f);
const  float        PLAYER_STAMINAREGEN                  =  0.1f;
const  float        PLAYER_SPRINTSTAMINA                 =  0.25f;
const  float        PLAYER_TIREDTHRESHOLD                =  0.3f;
const  float        PLAYER_ZOOMSCALE                     =  15.0f;
const  double       PLAYER_WEAPONSWITCHPERIOD            =  0.5;
const  double       PLAYER_OUTFITSWITCHPERIOD            =  1.0;
const  double       PLAYER_USEPERIOD                     =  0.2;
const  glm::vec4    PLAYER_LIGHT                         =  glm::vec4(0.5f,0.5f,0.5f,1.0f);
const  float        PLAYER_MELEE_OFFSET                  =  0.15f;
const  glm::vec2    PLAYER_FLASHLIGHT_SIZE               =  glm::vec2(5,10);
//     Achievements
const  int          ACHIEVEMENTS_VERSION                 =  2;
const  int          ACHIEVEMENTS_LINE_EM_UP_HITS         =  30;
const  int          ACHIEVEMENTS_BLEEDRUN_TIME           =  2*60*60;
const  double       ACHIEVEMENTS_MESSAGE_TIMEOUT         =  5.0;
const  double       ACHIEVEMENTS_MESSAGE_FADETIME        =  1.0;
//     Inventory
const  int          INVENTORY_BAGSIZE                    =  20;
const  int          INVENTORY_MAX_STACK                  =  99;
const  int          INVENTORY_MAX_OUTFITS                =  4;
const  int          INVENTORY_MAX_BACKPACKS              =  4;
const  int          INVENTORY_ICONTYPE_THRESHOLD         =  1;
//     Items
const  float        ITEM_SCALE                           =  0.5f;
const  float        ITEM_Z                               =  0.01f;
const  int          ITEM_QUALITY_MIN                     =  -100;
const  int          ITEM_QUALITY_FILTER_PROGRESSION      =  25;
const  float        ITEM_MIN_MOVESPEED                   =  -90.0f;
const  float        ITEM_MAX_MOVESPEED                   =  100.0f;
const  int          ITEM_PLACEMENT_ATTEMPTS              =  30;
const  float        ITEM_PLACEMENT_RADIUS                =  0.5f;
const  float        ITEM_PLACEMENT_RADIUS_REDUCTION      =  0.9f;
const  float        ITEM_RADIUS                          =  0.25f;
const  glm::vec4    ITEM_QUALITY_GOOD_COLOR              =  glm::vec4(1.0f,0.2f,0.2f,1.0f);
const  glm::vec4    ITEM_QUALITY_BAD_COLOR               =  glm::vec4(0.2f,1.0f,0.2f,1.0f);
const  int          ITEM_UNIQUE_LIGHTS                   =  10;
const  float        ITEM_HIGHLIGHT_SCALE                 =  1.3f;
const  float        ITEM_HIGHLIGHT_ALPHA                 =  0.4f;
const  float        ITEM_FILTERED_ALPHA                  =  0.1f;
const  double       ITEM_FILTERED_FADETIME               =  2.0;
const  double       ITEM_FILTERED_FADESTART              =  0.5;
const  int          ITEM_DYNAMITE_VALUE                  =  25;
const  float        ITEM_DYNAMITE_MOD_FACTOR             =  0.2f;
const  double       ITEM_ADRENALINE_USE_THRESHOLD        =  0.95;
//     Mods
const  double       MOD_BURST_FIREPERIOD_FACTOR          =  2.0;
const  float        MOD_SECONDARY_BONUS                  =  25.0f;
//     Objects
const  float        OBJECT_Z                             =  0.3f;
const  int          OBJECT_MAX_LEVEL                     =  1000;
const  float        OBJECT_MAX_SPEED                     =  15.0f;
const  int          OBJECT_MAX_RENDERLIST                =  10;
//     Projectiles
const  float        PROJECTILE_MIN_ALPHA                 =  0.35f;
//     AI
const  double       AI_REACTION_TIME_MIN                 =  GAME_TIMESTEP;
const  double       AI_REACTION_TIME_MAX                 =  0.3;
const  double       AI_RETURN_TIME                       =  10.0;
const  double       AI_RETURN_TIME_BOSS                  =  3.0;
const  double       AI_RETREAT_TIME                      =  4.0;
const  float        AI_RETREAT_DISTANCE                  =  20.0f;
const  double       AI_SHOOT_PERIOD                      =  0.5;
//     Map
const  int          MAP_FILEVERSION                      =  3;
const  std::string  MAP_TEXTURE_PATH                     =  "textures/map/";
const  float        MAP_MINZ                             =  0.0f;
const  float        MAP_FLATZ                            =  1.0f;
const  float        MAP_WALLZ                            =  2.0f;
const  float        MAP_FOREGROUNDZ                      =  3.0f;
const  float        MAP_FOREGROUND_FADE                  =  0.2f;
const  float        MAP_LAYEROFFSET                      =  0.001f;
const  int          MAP_WIDTH                            =  100;
const  int          MAP_HEIGHT                           =  100;
const  float        MAP_EPSILON                          =  0.0001f;
const  double       MAP_DAY_LENGTH                       =  24.0*60.0;
//     Editor
const  std::string  EDITOR_TESTLEVEL                     =  "test.map";
const  float        EDITOR_OBJECTRADIUS                  =  0.3f;
const  double       EDITOR_PERIODADJUST                  =  0.1;
const  int          EDITOR_DEFAULT_LAYER                 =  1;
const  int          EDITOR_DEFAULT_GRIDMODE              =  5;
const  glm::ivec2   EDITOR_VIEWPORT_OFFSET               =  glm::ivec2(256,168);
const  int          EDITOR_PALETTE_SIZE                  =  64;
const  int          EDITOR_PALETTE_SELECTEDSIZE          =  32;
const  float        EDITOR_LEVEL_Z                       =  20.0f;
//     Menu
const  float        MENU_ACCEPTINPUT_FADE                =  0.7f;
const  double       MENU_DOUBLECLICK_TIME                =  0.250;
const  double       MENU_WARN_TIME                       =  3.0;
//     UI
const  glm::vec2    UI_INVENTORY_ITEM_SIZE               =  glm::vec2(64,64);
const  glm::vec2    UI_HUD_AMMO_SIZE                     =  glm::vec2(32,32);
const  glm::vec2    UI_MESSAGE_SIZE                      =  glm::vec2(550,200);
const  glm::vec2    UI_MESSAGE_SMALL_SIZE                =  glm::vec2(425,125);
const  double       UI_LEVELNAME_TIME                    =  4;
//     HUD
const  int          HUD_BACKPACK_INDEX                   =  10;
const  float        HUD_PLAYER_HEALTH_WARNING            =  0.5f;
const  float        HUD_PLAYER_HEALTH_FADE               =  0.4f;
const  float        HUD_PLAYER_HEALTH_PULSE_AMOUNT       =  0.025f;
const  double       HUD_PLAYER_HEALTH_PULSE_FACTOR       =  10.0;
const  double       HUD_CURSOR_ITEM_WAIT                 =  0.5;
const  double       HUD_STANDOVER_TIME                   =  1.0;
const  float        HUD_CROSSHAIRDIVISOR                 =  5.0f;
const  float        HUD_MINCROSSHAIRSCALE                =  0.0f;
const  double       HUD_KEY_MESSAGETIME                  =  3.0;
const  double       HUD_SECRET_MESSAGETIME               =  5.0;
const  double       HUD_CHECKPOINTTIME                   =  5.0;
const  std::string  HUD_CHECKPOINTMESSAGE                =  "CHECKPOINT REACHED";
const  std::string  HUD_INVENTORYFULLMESSAGE             =  "INVENTORY FULL";
const  std::string  HUD_BACKPACKFULLMESSAGE              =  "BACKPACK FULL";
const  double       HUD_INVENTORYFULLTIME                =  2.0;
//     Minimap
const  glm::vec2    MINIMAP_CAPTURE_SIZE                 =  glm::vec2(20.0f,20.0f);
const  glm::vec2    MINIMAP_FULL_CAPTURE_SIZE            =  glm::vec2(50.0f,50.0f);
const  glm::vec2    MINIMAP_SIZE                         =  glm::vec2(200,200);
const  glm::vec2    MINIMAP_PADDING                      =  glm::vec2(10,10);
const  glm::vec4    MINIMAP_BACKGROUND_COLOR             =  glm::vec4(0.0f,0.0f,0.0f,0.8f);
const  glm::vec4    MINIMAP_BACKGROUND_COLOR_ICONS       =  glm::vec4(0.0f,0.0f,0.0f,1.0f);
const  glm::vec4    MINIMAP_ENEMY_COLOR                  =  glm::vec4(1.0f,0.0f,0.0f,1.0f);
const  glm::vec4    MINIMAP_PLAYER_COLOR                 =  glm::vec4(0.3f,0.3f,0.3f,1.0f);
const  glm::vec4    MINIMAP_GEAR_COLOR                   =  glm::vec4(0.0f,1.0f,0.0f,1.0f);
const  glm::vec4    MINIMAP_MOD_COLOR                    =  glm::vec4(0.0f,1.0f,1.0f,1.0f);
const  glm::vec4    MINIMAP_UNIQUE_COLOR                 =  glm::vec4(0.76f,0.73f,0.173f,1.0f);
const  glm::vec4    MINIMAP_KEY_COLOR                    =  glm::vec4(1.0f,1.0f,0.0f,1.0f);
const  glm::vec4    MINIMAP_AMMO_COLOR                   =  glm::vec4(0.0f,0.0f,1.0f,1.0f);
const  glm::vec4    MINIMAP_CONSUMABLE_COLOR             =  glm::vec4(1.0f,1.0f,1.0f,1.0f);
const  glm::vec4    MINIMAP_USABLE_COLOR                 =  glm::vec4(1.0f,1.0f,1.0f,1.0f);
const  glm::vec4    MINIMAP_CRATE_COLOR                  =  glm::vec4(1.0f,0.0f,1.0f,1.0f);
const  glm::vec4    MINIMAP_WALL_COLOR                   =  glm::vec4(0.40f,0.36f,0.29f,0.4f);
const  glm::vec4    MINIMAP_PROJECTILE_COLOR             =  glm::vec4(0.5f,0.5f,0.5f,1.0f);
const  glm::vec4    MINIMAP_DOOR_COLOR                   =  glm::vec4(0.28f,0.28f,0.28f,1.0f);
const  glm::vec4    MINIMAP_DOOR_REDCOLOR                =  glm::vec4(1.0f,0.0f,0.0f,1.0f);
const  glm::vec4    MINIMAP_DOOR_GREENCOLOR              =  glm::vec4(0.0f,1.0f,0.0f,1.0f);
const  glm::vec4    MINIMAP_DOOR_BLUECOLOR               =  glm::vec4(0.0f,0.0f,1.0f,1.0f);
const  glm::vec4    MINIMAP_DOOR_BOSSCOLOR               =  glm::vec4(1.0f,1.0f,0.0f,1.0f);
const  glm::vec4    MINIMAP_TOGGLED_COLOR                =  glm::vec4(0.035f,0.035f,0.035f,1.0f);
const  float        MINIMAP_IMAGE_SIZE                   =  1.0f;
const  float        MINIMAP_ICON_FADE                    =  0.25f;
//     Light
const  glm::vec3    LIGHT_ATTENUATION                    =  glm::vec3(1.0f,0.2f,0.1f);
const  glm::vec3    LIGHT_FLASH_ATTENUATION              =  glm::vec3(1.0f,0.1f,0.05f);
const  double       LIGHT_FLASH_TIME                     =  GAME_TIMESTEP*2;
const  glm::vec4    LIGHT_FLASH_COLOR                    =  glm::vec4(4.0f,4.0f,4.0f,1.0f);
const  float        LIGHT_CHANGE_SPEED                   =  2.0f;
const  float        LIGHT_CRATE_SCALE                    =  1.2f;
