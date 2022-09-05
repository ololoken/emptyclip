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
#include <save.h>
#include <objects/player.h>
#include <ae/files.h>
#include <ae/buffer.h>
#include <config.h>
#include <stats.h>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>

enum SaveChunkTypes {
	CHUNK_SAVEVERSION,
	CHUNK_PLAYERNAME,
	CHUNK_COLOR,
	CHUNK_MAP,
	CHUNK_PROGRESSION,
	CHUNK_CHECKPOINT,
	CHUNK_EXPERIENCE,
	CHUNK_GOLD,
	CHUNK_HEALTH,
	CHUNK_PLAYTIME,
	CHUNK_KILLS,
	CHUNK_SKILLS,
	CHUNK_ITEMS,
	CHUNK_AMMO,
	CHUNK_KEYS,
	CHUNK_DEATHS,
	CHUNK_PROGRESSIONTIME,
	CHUNK_CLOCK,
	CHUNK_PROGRESSIONKILLS,
	CHUNK_PROGRESSIONCRATES,
	CHUNK_PROGRESSIONSECRETS,
	CHUNK_PROGRESSIONDEATHS,
};

// Write a chunk to a stream
static void WriteChunk(std::ofstream &File, int Type, const char *Data, int Size) {
	File.write((char *)&Type, sizeof(Type));
	File.write((char *)&Size, sizeof(Size));
	File.write(Data, Size);
}

_Save Save;

// Constructor
_Save::_Save() {
	Players.insert(Players.begin(), SLOT_COUNT, nullptr);
}

// Destructor
_Save::~_Save() {
	for(size_t i = 0; i < Players.size(); i++)
		delete Players[i];
}

// Get a save path for a slot
std::string _Save::GetConfigPath(size_t Slot) {
	if(Slot >= SLOT_COUNT)
		return "";

	std::ostringstream Buffer;
	Buffer << Config.ConfigPath << (Slot + 1) << ".save";
	return Buffer.str();
}

// Create new player
void _Save::CreateNewPlayer(size_t Slot, const std::string &Name, const std::string &ColorID) {
	if(Slot >= SLOT_COUNT)
		return;

	Players[Slot] = new _Player(Stats.Objects.at("player"));
	Players[Slot]->Reset(true);
	Players[Slot]->SavePath = GetConfigPath(Slot);
	Players[Slot]->Name = Name;
	Players[Slot]->Health = Players[Slot]->MaxHealth * PLAYER_STARTING_HEALTH_FACTOR;
	Players[Slot]->SetColorID(ColorID);

	SavePlayer(Players[Slot]);
}

// Deletes a player
void _Save::DeletePlayer(size_t Slot) {
	if(Slot >= SLOT_COUNT)
		return;

	// Build save name
	remove(GetConfigPath(Slot).c_str());

	delete Players[Slot];
	Players[Slot] = nullptr;
}

// Load save files
void _Save::LoadSaves() {

	// Clear previous saves
	for(size_t i = 0; i < Players.size(); i++) {
		delete Players[i];
		Players[i] = nullptr;
	}

	// Get directory contents
	ae::_Files Files(Config.ConfigPath);

	// Load slots with player names
	for(size_t i = 0; i < Files.Nodes.size(); i++) {
		size_t Extension = Files.Nodes[i].find(".save");
		std::string SlotIndexString = Files.Nodes[i].substr(0, Extension);
		size_t SlotIndex = atoi(SlotIndexString.c_str()) - 1;
		if(SlotIndex > SLOT_9)
			continue;

		try {
			Players[SlotIndex] = new _Player(Stats.Objects.at("player"));
			Players[SlotIndex]->SavePath = Config.ConfigPath + Files.Nodes[i];
			LoadPlayer(Players[SlotIndex]);
		}
		catch(std::exception &Error) {
			std::cout << Error.what() << std::endl;
			delete Players[SlotIndex];
			Players[SlotIndex] = nullptr;
		}
	}
}

// Loads information from a file
void _Save::LoadPlayer(_Player *Player) {
	Player->Reset();

	// Open file
	std::ifstream File(Player->SavePath.c_str(), std::ios::in | std::ios::binary);
	if(!File)
		throw std::runtime_error("Cannot load save file: " + Player->SavePath);

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		// Get chunk type
		int Type;
		File.read((char *)&Type, sizeof(Type));

		// Get chunk size
		int Size;
		File.read((char *)&Size, sizeof(Size));

		switch(Type) {
			case CHUNK_SAVEVERSION: {
				int SaveVersion;
				File.read((char *)&SaveVersion, sizeof(SaveVersion));
				if(SaveVersion != PLAYER_SAVEVERSION) {
					std::rename(Player->SavePath.c_str(), (Player->SavePath + "." + std::to_string(SaveVersion)).c_str());
					throw std::runtime_error("Save version mismatch");
				}
			} break;
			case CHUNK_PLAYERNAME: {
				char Buffer[1024];
				File.read(Buffer, Size);
				Buffer[Size] = 0;
				Player->Name = Buffer;
			} break;
			case CHUNK_COLOR: {
				char Buffer[1024];
				File.read(Buffer, Size);
				Buffer[Size] = 0;
				Player->ColorID = Buffer;
			} break;
			case CHUNK_MAP: {
				char Buffer[1024];
				File.read(Buffer, Size);
				Buffer[Size] = 0;
				Player->MapID = Buffer;
			} break;
			case CHUNK_CHECKPOINT:
				File.read((char *)&Player->CheckpointIndex, sizeof(Player->CheckpointIndex));
			break;
			case CHUNK_PROGRESSION:
				File.read((char *)&Player->Progression, sizeof(Player->Progression));
			break;
			case CHUNK_GOLD:
				File.read((char *)&Player->Gold, sizeof(Player->Gold));
			break;
			case CHUNK_EXPERIENCE:
				File.read((char *)&Player->Experience, sizeof(Player->Experience));
			break;
			case CHUNK_HEALTH:
				File.read((char *)&Player->Health, sizeof(Player->Health));
				if(Player->Health <= 0)
					Player->Health = 1;
			break;
			case CHUNK_PLAYTIME:
				File.read((char *)&Player->PlayTime, sizeof(Player->PlayTime));
			break;
			case CHUNK_PROGRESSIONTIME:
				File.read((char *)&Player->ProgressionTime, sizeof(Player->ProgressionTime));
			break;
			case CHUNK_PROGRESSIONKILLS:
				File.read((char *)&Player->ProgressionKills, sizeof(Player->ProgressionKills));
			break;
			case CHUNK_PROGRESSIONCRATES:
				File.read((char *)&Player->ProgressionCrates, sizeof(Player->ProgressionCrates));
			break;
			case CHUNK_PROGRESSIONSECRETS:
				File.read((char *)&Player->ProgressionSecrets, sizeof(Player->ProgressionSecrets));
			break;
			case CHUNK_PROGRESSIONDEATHS:
				File.read((char *)&Player->ProgressionDeaths, sizeof(Player->ProgressionDeaths));
			break;
			case CHUNK_KILLS:
				File.read((char *)&Player->TotalKills, sizeof(Player->TotalKills));
			break;
			case CHUNK_DEATHS:
				File.read((char *)&Player->TotalDeaths, sizeof(Player->TotalDeaths));
			break;
			case CHUNK_SKILLS:
				File.read((char *)&Player->Skills, sizeof(Player->Skills));
			break;
			case CHUNK_CLOCK:
				File.read((char *)&Player->Clock, sizeof(Player->Clock));
			break;
			case CHUNK_ITEMS: {
				ae::_Buffer Buffer(Size);
				File.read(&Buffer[0], Size);
				LoadItems(Player, Buffer);
			} break;
			case CHUNK_AMMO: {
				ae::_Buffer Buffer(Size);
				File.read(&Buffer[0], Size);
				LoadAmmo(Player, Buffer);
			} break;
			case CHUNK_KEYS: {
				ae::_Buffer Buffer(Size);
				File.read(&Buffer[0], Size);
				LoadKeys(Player, Buffer);
			} break;
			default:
				File.ignore(Size);
			break;
		}
	}

	File.close();

	Player->CheckpointIndex = 0;
	Player->CalculateExperienceStats();
	Player->CalculateSkillsRemaining();
	Player->UpdateColor();
	Player->RecalculateStats();
	Player->ResetWeaponAnimation();
	Player->UpdateHealth(0);
}

// Saves information to a file
void _Save::SavePlayer(_Player *Player) {

	// Open file
	std::string SavePath = Config.ConfigPath + "_temp.save";
	std::ofstream File(SavePath.c_str(), std::ios::out | std::ios::binary);
	if(!File.is_open())
		throw std::runtime_error("Cannot create save file: " + Player->SavePath);

	WriteChunk(File, CHUNK_SAVEVERSION, (const char *)&PLAYER_SAVEVERSION, sizeof(PLAYER_SAVEVERSION));
	WriteChunk(File, CHUNK_PLAYERNAME, Player->Name.c_str(), (int)Player->Name.length());
	WriteChunk(File, CHUNK_COLOR, Player->ColorID.c_str(), (int)Player->ColorID.length());
	if(Player->Map) {
		WriteChunk(File, CHUNK_MAP, Player->MapID.c_str(), (int)Player->MapID.length());
		WriteChunk(File, CHUNK_CHECKPOINT, (char *)&Player->CheckpointIndex, sizeof(Player->CheckpointIndex));
	}
	WriteChunk(File, CHUNK_PROGRESSION, (char *)&Player->Progression, sizeof(Player->Progression));
	WriteChunk(File, CHUNK_EXPERIENCE, (char *)&Player->Experience, sizeof(Player->Experience));
	WriteChunk(File, CHUNK_GOLD, (char *)&Player->Gold, sizeof(Player->Gold));
	WriteChunk(File, CHUNK_HEALTH, (char *)&Player->Health, sizeof(Player->Health));
	WriteChunk(File, CHUNK_PLAYTIME, (char *)&Player->PlayTime, sizeof(Player->PlayTime));
	WriteChunk(File, CHUNK_PROGRESSIONTIME, (char *)&Player->ProgressionTime, sizeof(Player->ProgressionTime));
	WriteChunk(File, CHUNK_CLOCK, (char *)&Player->Clock, sizeof(Player->Clock));
	WriteChunk(File, CHUNK_KILLS, (char *)&Player->TotalKills, sizeof(Player->TotalKills));
	WriteChunk(File, CHUNK_DEATHS, (char *)&Player->TotalDeaths, sizeof(Player->TotalDeaths));
	WriteChunk(File, CHUNK_SKILLS, (char *)&Player->Skills, sizeof(Player->Skills));
	WriteChunk(File, CHUNK_PROGRESSIONKILLS, (char *)&Player->ProgressionKills, sizeof(Player->ProgressionKills));
	WriteChunk(File, CHUNK_PROGRESSIONCRATES, (char *)&Player->ProgressionCrates, sizeof(Player->ProgressionCrates));
	WriteChunk(File, CHUNK_PROGRESSIONSECRETS, (char *)&Player->ProgressionSecrets, sizeof(Player->ProgressionSecrets));
	WriteChunk(File, CHUNK_PROGRESSIONDEATHS, (char *)&Player->ProgressionDeaths, sizeof(Player->ProgressionDeaths));

	SaveItems(Player, File);
	SaveAmmo(Player, File);
	SaveKeys(Player, File);

	File.close();

	// Rename temp file
	std::rename(SavePath.c_str(), Player->SavePath.c_str());
}

// Loads items from a stream
void _Save::LoadItems(_Player *Player, ae::_Buffer &Buffer) {

	// Get inventory size
	int ItemCount = Buffer.Read<int>();
	if(ItemCount > INVENTORY_SIZE) {
		throw std::runtime_error("Too many items");
	}

	// Get items
	for(int i = 0; i < ItemCount; i++) {
		int Slot = Buffer.Read<int>();
		int Type = Buffer.Read<int>();
		int Count = Buffer.Read<int>();
		std::string ID = Buffer.ReadString();
		int Level = Buffer.Read<int>();
		int Quality = Buffer.Read<int>();

		// Create item
		_Item *Item = Stats.CreateItem(ID, Level, Quality, Count, glm::vec2(0, 0), false);

		// Read mods
		if(Type == _Object::WEAPON || Type == _Object::ARMOR) {
			Item->Attributes["max_mods"].Int = Buffer.Read<int>();
			LoadMods(Buffer, Item);
			Item->RecalculateStats();
		}

		// Read ammo
		if(Type == _Object::WEAPON) {
			Item->SetAmmo(Buffer.Read<int>());
		}

		Player->Inventory[Slot] = Item;
	}
}

// Load mods from a stream
void _Save::LoadMods(ae::_Buffer &Buffer, _Item *Item) {

	// Get size header
	int Mods = Buffer.Read<int>();

	// Read data
	for(int i = 0; i < Mods; i++) {
		std::string ID = Buffer.ReadString();
		int Level = Buffer.Read<int>();
		int Quality = Buffer.Read<int>();
		_Item *Mod = Stats.CreateItem(ID, Level, Quality, 0, glm::vec2(0, 0), false);
		if(!Item->AddMod(Mod))
			delete Mod;
	}
}

// Load ammo
void _Save::LoadAmmo(_Player *Player, ae::_Buffer &Buffer) {

	// Read count
	int AmmoTypeCount = Buffer.Read<int>();

	// Read data
	for(int i = 0; i < AmmoTypeCount; i++) {
		std::string ID = Buffer.ReadString();
		Player->Ammo[ID] = Buffer.Read<int>();
	}
}

// Load keys
void _Save::LoadKeys(_Player *Player, ae::_Buffer &Buffer) {

	// Read count
	int KeyTypeCount = Buffer.Read<int>();

	// Read data
	for(int i = 0; i < KeyTypeCount; i++) {
		std::string ID = Buffer.ReadString();
		Player->Keys[ID] = 1;
	}
}

// Saves items to a stream
void _Save::SaveItems(_Player *Player, std::ofstream &File) {

	// Get item count
	int ItemCount = 0;
	for(int i = 0; i < INVENTORY_SIZE; i++) {
		if(Player->HasInventory(i))
			ItemCount++;
	}

	// Write item count
	ae::_Buffer Buffer;
	Buffer.Write<int>(ItemCount);

	// Write items
	for(int i = 0; i < INVENTORY_SIZE; i++) {
		if(!Player->HasInventory(i))
			continue;

		Buffer.Write(i);
		Buffer.Write(Player->Inventory[i]->Type);
		Buffer.Write(Player->Inventory[i]->Count);
		Player->Inventory[i]->Serialize(Buffer);
	}

	// Write chunk
	WriteChunk(File, CHUNK_ITEMS, &Buffer[0], Buffer.GetCurrentSize());
}

// Save ammo to a stream
void _Save::SaveAmmo(_Player *Player, std::ofstream &File) {

	// Write ammo type count
	ae::_Buffer Buffer;
	Buffer.Write<int>((int)Player->Ammo.size());

	// Write ammo types
	for(const auto &AmmoType : Player->Ammo) {
		Buffer.WriteString(AmmoType.first.c_str());
		Buffer.Write(AmmoType.second);
	}

	// Write chunk
	WriteChunk(File, CHUNK_AMMO, &Buffer[0], Buffer.GetCurrentSize());
}

// Save keys
void _Save::SaveKeys(_Player *Player, std::ofstream &File) {

	// Write key type count
	ae::_Buffer Buffer;
	Buffer.Write<int>((int)Player->Keys.size());

	// Write keys
	for(const auto &Key : Player->Keys)
		Buffer.WriteString(Key.first.c_str());

	// Write chunk
	WriteChunk(File, CHUNK_KEYS, &Buffer[0], Buffer.GetCurrentSize());
}
