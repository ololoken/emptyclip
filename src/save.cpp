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
#include <objects/inventory.h>
#include <states/play.h>
#include <ae/files.h>
#include <ae/buffer.h>
#include <config.h>
#include <stats.h>
#include <sstream>
#include <iostream>
#include <fstream>

enum SaveChunkTypes {
	CHUNK_SAVEVERSION,
	CHUNK_TEST,
	CHUNK_PLAYERNAME,
	CHUNK_COLOR,
	CHUNK_CLOCK,
	CHUNK_MAP,
	CHUNK_CHECKPOINT,
	CHUNK_PROGRESSION,
	CHUNK_HARDCORE,
	CHUNK_EXPERIENCE,
	CHUNK_HEALTH,
	CHUNK_SKILLS,
	CHUNK_INVENTORY,
	CHUNK_AMMO,
	CHUNK_KEYS,
	CHUNK_FILTERS,
	CHUNK_ACTIVE_OUTFIT,
	CHUNK_ACTIVE_BACKPACK,
	CHUNK_UNUSED3,
	CHUNK_UNUSED4,
	CHUNK_UNUSED5,
	CHUNK_UNUSED6,
	CHUNK_UNUSED7,
	CHUNK_UNUSED8,
	CHUNK_UNUSED9,
	CHUNK_STAT_PLAYTIME,
	CHUNK_STAT_KILLS,
	CHUNK_STAT_DEATHS,
	CHUNK_STAT_LAVATOUCHES,
	CHUNK_PROGRESSION_TIME,
	CHUNK_PROGRESSION_KILLS,
	CHUNK_PROGRESSION_CRATES,
	CHUNK_PROGRESSION_SECRETS,
	CHUNK_PROGRESSION_DEATHS,
	CHUNK_PROGRESSION_UNUSED0,
	CHUNK_PROGRESSION_UNUSED1,
	CHUNK_PROGRESSION_UNUSED2,
	CHUNK_PROGRESSION_UNUSED3,
	CHUNK_PROGRESSION_UNUSED4,
	CHUNK_ACHIEVEMENT_100PERCENT,
	CHUNK_ACHIEVEMENT_FISTSONLY,
	CHUNK_ACHIEVEMENT_LONEWOLF,
	CHUNK_ACHIEVEMENT_UNUSED0,
	CHUNK_ACHIEVEMENT_UNUSED1,
	CHUNK_ACHIEVEMENT_UNUSED2,
	CHUNK_ACHIEVEMENT_UNUSED3,
	CHUNK_ACHIEVEMENT_UNUSED4,
	CHUNK_ACHIEVEMENT_UNUSED5,
	CHUNK_ACHIEVEMENT_UNUSED6,
	CHUNK_ACHIEVEMENT_UNUSED7,
	CHUNK_ACHIEVEMENT_UNUSED8,
	CHUNK_ACHIEVEMENT_UNUSED9,
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
	Players.insert(Players.begin(), SAVE_SLOTS, nullptr);
}

// Destructor
_Save::~_Save() {
	for(size_t i = 0; i < Players.size(); i++)
		delete Players[i];
}

// Get a save path for a slot
std::string _Save::GetConfigPath(size_t Slot) {
	if(Slot >= SAVE_SLOTS)
		return "";

	std::ostringstream Buffer;
	Buffer << Config.ConfigPath << (Slot + 1) << ".save";
	return Buffer.str();
}

// Create new player
void _Save::CreateNewPlayer(size_t Slot, const std::string &Name, const std::string &ColorID, bool Hardcore) {
	if(Slot >= SAVE_SLOTS)
		return;

	Players[Slot] = new _Player(Stats.Objects.at("player"));
	Players[Slot]->SavePath = GetConfigPath(Slot);
	Players[Slot]->Name = Name;
	Players[Slot]->Health = Players[Slot]->MaxHealth * PLAYER_STARTING_HEALTH_FACTOR;
	Players[Slot]->ResetAchievementTracking();
	Players[Slot]->SetColorID(ColorID);
	Players[Slot]->Hardcore = Hardcore;

	SavePlayer(Players[Slot]);
}

// Deletes a player
void _Save::DeletePlayer(size_t Slot) {
	if(Slot >= SAVE_SLOTS)
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
		size_t ExtensionPosition = Files.Nodes[i].find(".save");
		if(ExtensionPosition != Files.Nodes[i].size() - 5)
			continue;

		std::string SlotIndexString = Files.Nodes[i].substr(0, ExtensionPosition);
		size_t SlotIndex = (size_t)(atoi(SlotIndexString.c_str()) - 1);
		if(SlotIndex >= SAVE_SLOTS)
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

		// Remove test saves
		if(Players[SlotIndex] && Players[SlotIndex]->TestSave) {
			delete Players[SlotIndex];
			Players[SlotIndex] = nullptr;
		}
	}
}

// Loads information from a file
void _Save::LoadPlayer(_Player *Player) {

	// Open file
	std::ifstream File(Player->SavePath.c_str(), std::ios::in | std::ios::binary);
	if(!File)
		throw std::invalid_argument("Cannot load save file: " + Player->SavePath);

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
				if(SaveVersion != SAVE_VERSION) {
					std::string OldVersionPath = Player->SavePath + "." + std::to_string(SaveVersion);
					std::remove(OldVersionPath.c_str());
					std::rename(Player->SavePath.c_str(), OldVersionPath.c_str());
					throw std::runtime_error("Save version mismatch");
				}
			} break;
			case CHUNK_TEST:
				File.read((char *)&Player->TestSave, sizeof(Player->TestSave));
			break;
			case CHUNK_PLAYERNAME: {
				if(Size >= 1024)
					throw std::runtime_error("Bad chunk size!");

				char Buffer[1024];
				File.read(Buffer, Size);
				Buffer[Size] = 0;
				Player->Name = Buffer;
			} break;
			case CHUNK_COLOR: {
				if(Size >= 1024)
					throw std::runtime_error("Bad chunk size!");

				char Buffer[1024];
				File.read(Buffer, Size);
				Buffer[Size] = 0;
				Player->ColorID = Buffer;
			} break;
			case CHUNK_MAP: {
				if(Size >= 1024)
					throw std::runtime_error("Bad chunk size!");

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
			case CHUNK_HARDCORE:
				File.read((char *)&Player->Hardcore, sizeof(Player->Hardcore));
			break;
			case CHUNK_EXPERIENCE:
				File.read((char *)&Player->Experience, sizeof(Player->Experience));
			break;
			case CHUNK_HEALTH:
				File.read((char *)&Player->Health, sizeof(Player->Health));
			break;
			case CHUNK_STAT_PLAYTIME:
				File.read((char *)&Player->PlayTime, sizeof(Player->PlayTime));
			break;
			case CHUNK_PROGRESSION_TIME:
				File.read((char *)&Player->ProgressionTime, sizeof(Player->ProgressionTime));
			break;
			case CHUNK_PROGRESSION_KILLS:
				File.read((char *)&Player->ProgressionKills, sizeof(Player->ProgressionKills));
			break;
			case CHUNK_PROGRESSION_CRATES:
				File.read((char *)&Player->ProgressionCrates, sizeof(Player->ProgressionCrates));
			break;
			case CHUNK_PROGRESSION_SECRETS:
				File.read((char *)&Player->ProgressionSecrets, sizeof(Player->ProgressionSecrets));
			break;
			case CHUNK_PROGRESSION_DEATHS:
				File.read((char *)&Player->ProgressionDeaths, sizeof(Player->ProgressionDeaths));
			break;
			case CHUNK_STAT_KILLS:
				File.read((char *)&Player->TotalKills, sizeof(Player->TotalKills));
			break;
			case CHUNK_STAT_DEATHS:
				File.read((char *)&Player->TotalDeaths, sizeof(Player->TotalDeaths));
			break;
			case CHUNK_SKILLS: {
				int SkillCount;
				File.read((char *)&SkillCount, sizeof(SkillCount));
				SkillCount = std::min(SkillCount, (int)SKILL_COUNT);
				for(int i = 0; i < SkillCount; i++)
					File.read((char *)&Player->Skills[i], sizeof(Player->Skills[i]));
			} break;
			case CHUNK_CLOCK:
				File.read((char *)&Player->Clock, sizeof(Player->Clock));
			break;
			case CHUNK_STAT_LAVATOUCHES:
				File.read((char *)&Player->LavaTouches, sizeof(Player->LavaTouches));
			break;
			case CHUNK_ACHIEVEMENT_100PERCENT:
				File.read((char *)&Player->Stat100Percent, sizeof(Player->Stat100Percent));
			break;
			case CHUNK_ACHIEVEMENT_FISTSONLY:
				File.read((char *)&Player->StatFistsOnly, sizeof(Player->StatFistsOnly));
			break;
			case CHUNK_ACHIEVEMENT_LONEWOLF:
				File.read((char *)&Player->StatLoneWolf, sizeof(Player->StatLoneWolf));
			break;
			case CHUNK_AMMO: {
				ae::_Buffer Buffer((size_t)Size);
				File.read(&Buffer[0], Size);
				LoadAmmo(Player, Buffer);
			} break;
			case CHUNK_KEYS: {
				ae::_Buffer Buffer((size_t)Size);
				File.read(&Buffer[0], Size);
				LoadKeys(Player, Buffer);
			} break;
			case CHUNK_FILTERS: {
				int FilterCount;
				File.read((char *)&FilterCount, sizeof(FilterCount));
				FilterCount = std::min(FilterCount, (int)FILTER_COUNT);
				for(int i = 0; i < FilterCount; i++)
					File.read((char *)&Player->Filters[i], sizeof(Player->Filters[i]));
			} break;
			case CHUNK_INVENTORY: {
				ae::_Buffer Buffer((size_t)Size);
				File.read(&Buffer[0], Size);
				LoadInventory(Player, Buffer);
			} break;
			case CHUNK_ACTIVE_OUTFIT:
				File.read((char *)&Player->ActiveOutfit, sizeof(Player->ActiveOutfit));
			break;
			case CHUNK_ACTIVE_BACKPACK:
				File.read((char *)&Player->ActiveBackpack, sizeof(Player->ActiveBackpack));
			break;
			default:
				File.ignore(Size);
			break;
		}
	}

	File.close();

	// Initialize
	Player->AddMissingBackpacks();
	Player->ActiveBackpack = std::min(Player->ActiveBackpack, Player->Inventory->Containers[(size_t)BagType::BACKPACK].size() - 1);
	Player->CheckpointIndex = 0;
	Player->CalculateExperienceStats();
	Player->CalculateSkillsRemaining();
	Player->UpdateColor();
	Player->RecalculateStats();
	Player->ResetWeaponAnimation();
	Player->Stamina = Player->MaxStamina;
	Player->SelfHealTimer = Player->SelfHealDelay;

	// Handle exiting during combat
	Player->Health = std::min(Player->Health, Player->MaxHealth);
	if(!Player->TestSave && !Player->Hardcore && Player->Health <= 0) {
		Player->ApplyDeathPenalty();
		Player->Health = 1;
	}
}

// Saves information to a file
void _Save::SavePlayer(_Player *Player) {
	if(Player->SavePath.empty())
		return;

	// Open file
	std::string SavePath = Config.ConfigPath + "_temp.save";
	std::ofstream File(SavePath.c_str(), std::ios::out | std::ios::binary);
	if(!File.is_open())
		throw std::runtime_error("Cannot create save file: " + Player->SavePath);

	int64_t Health = Player->Health;
	if(!Player->TestSave && Player->InCombat())
		Health = 0;

	WriteChunk(File, CHUNK_SAVEVERSION, (const char *)&SAVE_VERSION_NEW, sizeof(SAVE_VERSION_NEW));
	WriteChunk(File, CHUNK_TEST, (char *)&Player->TestSave, sizeof(Player->TestSave));
	WriteChunk(File, CHUNK_PLAYERNAME, Player->Name.c_str(), (int)Player->Name.length());
	WriteChunk(File, CHUNK_COLOR, Player->ColorID.c_str(), (int)Player->ColorID.length());
	WriteChunk(File, CHUNK_CLOCK, (char *)&Player->Clock, sizeof(Player->Clock));
	if(Player->Map) {
		WriteChunk(File, CHUNK_MAP, Player->MapID.c_str(), (int)Player->MapID.length());
		WriteChunk(File, CHUNK_CHECKPOINT, (char *)&Player->CheckpointIndex, sizeof(Player->CheckpointIndex));
	}
	WriteChunk(File, CHUNK_MAP, Player->MapID.c_str(), (int)Player->MapID.length());
	WriteChunk(File, CHUNK_PROGRESSION, (char *)&Player->Progression, sizeof(Player->Progression));
	WriteChunk(File, CHUNK_HARDCORE, (char *)&Player->Hardcore, sizeof(Player->Hardcore));
	WriteChunk(File, CHUNK_EXPERIENCE, (char *)&Player->Experience, sizeof(Player->Experience));
	WriteChunk(File, CHUNK_HEALTH, (char *)&Health, sizeof(Health));
	WriteChunk(File, CHUNK_ACTIVE_OUTFIT, (char *)&Player->ActiveOutfit, sizeof(Player->ActiveOutfit));
	WriteChunk(File, CHUNK_ACTIVE_BACKPACK, (char *)&Player->ActiveBackpack, sizeof(Player->ActiveBackpack));
	WriteChunk(File, CHUNK_STAT_PLAYTIME, (char *)&Player->PlayTime, sizeof(Player->PlayTime));
	WriteChunk(File, CHUNK_STAT_KILLS, (char *)&Player->TotalKills, sizeof(Player->TotalKills));
	WriteChunk(File, CHUNK_STAT_DEATHS, (char *)&Player->TotalDeaths, sizeof(Player->TotalDeaths));
	WriteChunk(File, CHUNK_STAT_LAVATOUCHES, (char *)&Player->LavaTouches, sizeof(Player->LavaTouches));
	WriteChunk(File, CHUNK_PROGRESSION_TIME, (char *)&Player->ProgressionTime, sizeof(Player->ProgressionTime));
	WriteChunk(File, CHUNK_PROGRESSION_KILLS, (char *)&Player->ProgressionKills, sizeof(Player->ProgressionKills));
	WriteChunk(File, CHUNK_PROGRESSION_CRATES, (char *)&Player->ProgressionCrates, sizeof(Player->ProgressionCrates));
	WriteChunk(File, CHUNK_PROGRESSION_SECRETS, (char *)&Player->ProgressionSecrets, sizeof(Player->ProgressionSecrets));
	WriteChunk(File, CHUNK_PROGRESSION_DEATHS, (char *)&Player->ProgressionDeaths, sizeof(Player->ProgressionDeaths));
	WriteChunk(File, CHUNK_ACHIEVEMENT_100PERCENT, (char *)&Player->Stat100Percent, sizeof(Player->Stat100Percent));
	WriteChunk(File, CHUNK_ACHIEVEMENT_FISTSONLY, (char *)&Player->StatFistsOnly, sizeof(Player->StatFistsOnly));
	WriteChunk(File, CHUNK_ACHIEVEMENT_LONEWOLF, (char *)&Player->StatLoneWolf, sizeof(Player->StatLoneWolf));

	SaveInventory(Player, File);
	SaveAmmo(Player, File);
	SaveKeys(Player, File);
	SaveSkills(Player, File);
	SaveFilters(Player, File);

	File.close();

	// Rename temp file
	std::remove(Player->SavePath.c_str());
	std::rename(SavePath.c_str(), Player->SavePath.c_str());
}

// Load inventory
void _Save::LoadInventory(_Player *Player, ae::_Buffer &Buffer) {
	Player->Inventory->Unserialize(Buffer);
}

// Load ammo
void _Save::LoadAmmo(_Player *Player, ae::_Buffer &Buffer) {

	// Read count
	int AmmoTypeCount = Buffer.Read<int>();

	// Read data
	for(int i = 0; i < AmmoTypeCount; i++) {
		std::string ID = Buffer.ReadString();
		int Count = Buffer.Read<int>();
		if(Stats.Ammo.find(ID) != Stats.Ammo.end())
			Player->Ammo[ID] = Count;
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

// Save inventory items
void _Save::SaveInventory(_Player *Player, std::ofstream &File) {
	ae::_Buffer Buffer;
	Player->Inventory->Serialize(Buffer);
	WriteChunk(File, CHUNK_INVENTORY, &Buffer[0], (int)Buffer.GetCurrentSize());
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
	WriteChunk(File, CHUNK_AMMO, &Buffer[0], (int)Buffer.GetCurrentSize());
}

// Save keys
void _Save::SaveKeys(_Player *Player, std::ofstream &File) {
	ae::_Buffer Buffer;

	// Only save first two keys when boss key is found
	if(!PlayState.TestMode && Player->Keys.find("key_boss") != Player->Keys.end()) {
		Buffer.Write<int>(2);
		Buffer.WriteString("key_green");
		Buffer.WriteString("key_red");
	}
	else {

		// Write keys
		Buffer.Write<int>((int)Player->Keys.size());
		for(const auto &Key : Player->Keys)
			Buffer.WriteString(Key.first.c_str());
	}

	// Write chunk
	WriteChunk(File, CHUNK_KEYS, &Buffer[0], (int)Buffer.GetCurrentSize());
}

// Save skills
void _Save::SaveSkills(_Player *Player, std::ofstream &File) {
	ae::_Buffer Buffer;

	Buffer.Write<int>(SKILL_COUNT);
	for(int i = 0; i < SKILL_COUNT; i++)
		Buffer.Write<int>(Player->Skills[i]);

	// Write chunk
	WriteChunk(File, CHUNK_SKILLS, &Buffer[0], (int)Buffer.GetCurrentSize());
}

// Save loot filter settings
void _Save::SaveFilters(_Player *Player, std::ofstream &File) {
	ae::_Buffer Buffer;

	Buffer.Write<int>(FILTER_COUNT);
	for(int i = 0; i < FILTER_COUNT; i++)
		Buffer.Write<int>(Player->Filters[i]);

	// Write chunk
	WriteChunk(File, CHUNK_FILTERS, &Buffer[0], (int)Buffer.GetCurrentSize());
}
