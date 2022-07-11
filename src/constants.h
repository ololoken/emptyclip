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
const  glm::ivec2   DEFAULT_WINDOW_SIZE            =  glm::ivec2(1440,900);
const  int          DEFAULT_FULLSCREEN             =  1;
const  int          DEFAULT_AUDIOENABLED           =  1;
const  int          DEFAULT_VSYNC                  =  1;
const  double       DEFAULT_MAXFPS                 =  240.0;
//     Config
const  int          CONFIG_VERSION                 =  3;
//     Game
const  std::string  GAME_WINDOWTITLE               =  "Empty Clip";
const  double       GAME_FPS                       =  100.0;
const  double       GAME_TIMESTEP                  =  1.0/GAME_FPS;
const  float        GAME_PAUSE_FADEAMOUNT          =  0.7f;
const  std::string  GAME_STARTLEVEL                =  "mansion0.map";
const  std::string  GAME_FIRSTLEVEL                =  "mansion0.map";
const  int          GAME_SKILLLEVELS               =  100;
const  int          GAME_MAX_SKILL_PERLEVEL        =  3;
const  int          GAME_WIN_PROGRESSION_POINTS    =  20;
const  double       GAME_EXPERIENCE_LOST           =  0.1;
const  double       GAME_INVULNERABLE_TIME         =  3.0;
//     Camera
const  float        CAMERA_DISTANCE                =  6.5f;
const  float        CAMERA_DISTANCE_AIMED          =  7.0f;
const  float        CAMERA_DIVISOR                 =  0.2f;
const  float        CAMERA_EDITOR_DIVISOR          =  0.05f;
const  float        CAMERA_FOVY                    =  90.0f;
const  float        CAMERA_NEAR                    =  0.1f;
const  float        CAMERA_FAR                     =  500.0f;
//     Weapons
const  double       WEAPON_MINFIREPERIOD           =  GAME_TIMESTEP;
//     Audio
const  float        AUDIO_MAX_DISTANCE             =  30.0f;
//     Entities
const  float        ENTITY_MOVESOUNDDELAYFACTOR    =  0.02625f;
const  int          ENTITY_MINDAMAGEPOINTS         =  1;
const  float        ENTITY_STOP_THRESHOLD          =  0.005f;
const  double       ENTITY_STATIC_TIME             =  2.0;
const  float        ENTITY_MAX_ACTIVE_RANGE        =  30.0f;
const  int          ENTITY_MAX_DAMAGE_RESIST       =  90;
//     Player
const  int          PLAYER_SAVEVERSION             =  5;
const  float        PLAYER_RADIUS                  =  0.35f;
const  double       PLAYER_MEDKITPERIOD            =  0.5;
const  float        PLAYER_STARTING_HEALTH_FACTOR  =  0.85f;
const  float        PLAYER_LEGCHANGEFACTOR         =  0.2f;
const  float        PLAYER_MOVESPEED               =  4.5f;
const  float        PLAYER_BACKWARDS_SPEEDFACTOR   =  0.5f;
const  float        PLAYER_AIM_MOVESPEEDFACTOR     =  0.333333f;
const  float        PLAYER_AIM_RECOIL_MODIFIER     =  0.5f;
const  float        PLAYER_SPRINT_RECOIL_MODIFIER  =  2.0f;
const  float        PLAYER_SPRINT_SPEEDFACTOR      =  1.65f;
const  glm::vec2    PLAYER_PISTOLOFFSET            =  glm::vec2(36.0f/64.0f-0.5f,-0.5f);
const  glm::vec2    PLAYER_WEAPONOFFSET            =  glm::vec2(30.0f/64.0f-0.5f,-0.5f);
const  float        PLAYER_STAMINAREGEN            =  0.1f;
const  float        PLAYER_SPRINTSTAMINA           =  0.333333f;
const  float        PLAYER_TIREDTHRESHOLD          =  0.3f;
const  float        PLAYER_ZOOMSCALE               =  15.0f;
const  float        PLAYER_MAXACCURACY             =  170.0f;
const  double       PLAYER_WEAPONSWITCHPERIOD      =  0.5;
const  double       PLAYER_USEPERIOD               =  0.2;
const  glm::vec4    PLAYER_LIGHT                   =  glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
//     Inventory
const  int          INVENTORY_BAGSIZE              =  16;
const  int          INVENTORY_MAX_STACK            =  99;
//     Items
const  float        ITEM_SCALE                     =  0.5f;
const  float        ITEM_Z                         =  0.05f;
const  int          ITEM_QUALITY_RANGE             =  15;
const  int          ITEM_MAX_MOVESPEED             =  90;
//     Objects
const  float        OBJECT_Z                       =  0.3f;
const  int          OBJECT_MAX_LEVEL               =  1000;
//     Map
const  int          MAP_FILEVERSION                =  2;
const  std::string  MAP_TEXTURE_PATH               =  "textures/map/";
const  float        MAP_MINZ                       =  0.0f;
const  float        MAP_FLATZ                      =  1.0f;
const  float        MAP_WALLZ                      =  2.0f;
const  float        MAP_FOREGROUNDZ                =  3.0f;
const  float        MAP_LAYEROFFSET                =  0.01f;
const  int          MAP_WIDTH                      =  100;
const  int          MAP_HEIGHT                     =  100;
const  float        MAP_EPSILON                    =  0.0001f;
//     Editor
const  std::string  EDITOR_TESTLEVEL               =  "test.map";
const  float        EDITOR_OBJECTRADIUS            =  0.3f;
const  double       EDITOR_PERIODADJUST            =  0.1;
const  int          EDITOR_DEFAULT_LAYER           =  1;
const  int          EDITOR_DEFAULT_GRIDMODE        =  5;
const  glm::ivec2   EDITOR_VIEWPORT_OFFSET         =  glm::ivec2(256,168);
const  int          EDITOR_PALETTE_SELECTEDSIZE    =  32;
//     Menu
const  float        MENU_ACCEPTINPUT_FADE          =  0.7f;
const  double       MENU_DOUBLECLICK_TIME          =  0.250;
//     UI
const  glm::vec2    UI_INVENTORY_ITEM_SIZE         =  glm::vec2(64,64);
const  glm::vec2    UI_HUD_AMMO_SIZE               =  glm::vec2(32,32);
//     HUD
const  double       HUD_ENTITYHEALTHDISPLAYPERIOD  =  5.0;
const  float        HUD_PLAYER_HEALTH_WARNING      =  0.5f;
const  float        HUD_PLAYER_HEALTH_FADE         =  0.4f;
const  float        HUD_PLAYER_HEALTH_PULSE_AMOUNT =  0.025f;
const  double       HUD_PLAYER_HEALTH_PULSE_FACTOR =  10.0;
const  double       HUD_CURSOR_ITEM_WAIT           =  0.5;
const  double       HUD_STANDOVER_TIME             =  1.0;
const  float        HUD_CROSSHAIRDIVISOR           =  5.0f;
const  float        HUD_MINCROSSHAIRSCALE          =  0.0f;
const  double       HUD_KEYMESSAGETIME             =  3.0;
const  double       HUD_CHECKPOINTTIME             =  5.0;
const  std::string  HUD_CHECKPOINTMESSAGE          =  "CHECKPOINT REACHED";
const  double       HUD_INVENTORYFULLTIME          =  2.0;
const  glm::vec2    HUD_MINIMAP_CAPTURE_SIZE       =  glm::vec2(20.0f,20.0f);
const  glm::vec2    HUD_MINIMAP_FULL_CAPTURE_SIZE  =  glm::vec2(50.0f,50.0f);
const  glm::vec2    HUD_MINIMAP_SIZE               =  glm::vec2(200,200);
const  glm::vec2    HUD_MINIMAP_PADDING            =  glm::vec2(10,10);
const  glm::vec4    HUD_MINIMAP_BACKGROUND_COLOR   =  glm::vec4(0.0f, 0.0f,0.0f, 0.8f);
const  glm::vec4    HUD_MINIMAP_ENEMY_COLOR        =  glm::vec4(1.0f,0.0f,0.0f,1.0f);
const  glm::vec4    HUD_MINIMAP_PLAYER_COLOR       =  glm::vec4(1.0f,0.0f,0.0f,1.0f);
const  glm::vec4    HUD_MINIMAP_EQUIPMENT_COLOR    =  glm::vec4(0.0f,1.0f,0.0f,1.0f);
const  glm::vec4    HUD_MINIMAP_KEY_COLOR          =  glm::vec4(1.0f,1.0f,0.0f,1.0f);
const  glm::vec4    HUD_MINIMAP_AMMO_COLOR         =  glm::vec4(0.0f,1.0f,1.0f,1.0f);
const  glm::vec4    HUD_MINIMAP_MEDKIT_COLOR       =  glm::vec4(1.0f,1.0f,1.0f,1.0f);
const  glm::vec4    HUD_MINIMAP_CRATE_COLOR        =  glm::vec4(1.0f,0.0f,1.0f,1.0f);
const  glm::vec4    HUD_MINIMAP_WALL_COLOR         =  glm::vec4(0.40f,0.36f,0.29f,0.4f);
const  glm::vec4    HUD_MINIMAP_DOOR_COLOR         =  glm::vec4(0.28f,0.28f,0.28f,1.0f);
const  glm::vec4    HUD_MINIMAP_TOGGLED_COLOR      =  glm::vec4(0.035f,0.035f,0.035f,1.0f);
//     Light
const  glm::vec3    LIGHT_ATTENUATION              =  glm::vec3(1.0f, 0.2f, 0.1f);
const  glm::vec3    LIGHT_FLASH_ATTENUATION        =  glm::vec3(1.0f, 0.1f, 0.05f);
const  double       LIGHT_FLASH_TIME               =  GAME_TIMESTEP * 2;
const  glm::vec4    LIGHT_FLASH_COLOR              =  glm::vec4(4.0f, 4.0f, 4.0f, 1.0f);
const  double       LIGHT_CHANGE_PERIOD            =  0.5;
