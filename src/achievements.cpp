/******************************************************************************
* Empty Clip
* Copyright (C) 2024 Alan Witkowski
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
#include <achievements.h>
#include <ae/buffer.h>
#include <constants.h>
#include <config.h>
#include <fstream>

static const char *FILENAME = "stats.db";

enum AchievementChunkTypes {
	CHUNK_VERSION,
	CHUNK_STAT_ID,
	CHUNK_STAT_VERSION,
	CHUNK_STAT_TIME,
	CHUNK_STAT_VALUE,
};

// Write a chunk to a stream
static void WriteChunk(std::ofstream &File, int Type, const char *Data, int Size) {
	File.write((char *)&Type, sizeof(Type));
	File.write((char *)&Size, sizeof(Size));
	File.write(Data, Size);
}

_Achievements Achievements;

// Set up system
void _Achievements::Init() {
	Path = Config.SavePath + FILENAME;
}

void _Achievements::Load() {
	if(!Enabled || Path.empty())
		return;

	Stats.clear();

	// Open file
	std::ifstream File(Path.c_str(), std::ios::in | std::ios::binary);
	if(!File)
		throw std::runtime_error("Cannot load file: " + Path);

	// Read file
	std::string CurrentID = "";
	while(!File.eof() && File.peek() != EOF) {

		// Get chunk type
		int Type;
		File.read((char *)&Type, sizeof(Type));

		// Get chunk size
		int Size;
		File.read((char *)&Size, sizeof(Size));

		// Read chunks
		switch(Type) {
			case CHUNK_VERSION: {
				int Version;
				File.read((char *)&Version, sizeof(Version));
				if(Version != ACHIEVEMENTS_VERSION) {
					std::string OldVersionPath = Path + "." + std::to_string(Version);
					std::remove(OldVersionPath.c_str());
					std::rename(Path.c_str(), OldVersionPath.c_str());
					throw std::runtime_error("Achievements version mismatch");
				}
			} break;
			case CHUNK_STAT_ID: {
				ae::_Buffer Buffer((size_t)Size);
				File.read(&Buffer[0], Size);
				CurrentID = Buffer.ReadString();
			} break;
			case CHUNK_STAT_VERSION: {
				ae::_Buffer Buffer((size_t)Size);
				File.read(&Buffer[0], Size);
				std::string BuildVersion = Buffer.ReadString();
				if(CurrentID.size())
					Stats[CurrentID].Version = BuildVersion;
			} break;
			case CHUNK_STAT_TIME: {
				ae::_Buffer Buffer((size_t)Size);
				File.read(&Buffer[0], Size);
				int64_t Time = Buffer.Read<int64_t>();
				if(CurrentID.size())
					Stats[CurrentID].Time = Time;
			} break;
			case CHUNK_STAT_VALUE: {
				ae::_Buffer Buffer((size_t)Size);
				File.read(&Buffer[0], Size);
				int Value = Buffer.Read<int>();
				if(CurrentID.size())
					Stats[CurrentID].Value = Value;
			} break;
			default:
				File.ignore(Size);
			break;
		}
	}

	File.close();
}

// Save achievement stats
void _Achievements::Save() {
	if(!Enabled || Path.empty())
		return;

	// Open file
	std::string TempPath = Config.SavePath + "_stats.db";
	std::ofstream File(TempPath.c_str(), std::ios::out | std::ios::binary);
	if(!File.is_open())
		throw std::runtime_error("Cannot create file: " + TempPath);

	// Write header
	WriteChunk(File, CHUNK_VERSION, (const char *)&ACHIEVEMENTS_VERSION, sizeof(ACHIEVEMENTS_VERSION));

	// Write stats
	for(const auto &Stat : Stats) {
		{
			ae::_Buffer Buffer;
			Buffer.WriteString(Stat.first.c_str());
			WriteChunk(File, CHUNK_STAT_ID, &Buffer[0], (int)Buffer.GetCurrentSize());
		}
		{
			ae::_Buffer Buffer;
			Buffer.WriteString(Stat.second.Version.c_str());
			WriteChunk(File, CHUNK_STAT_VERSION, &Buffer[0], (int)Buffer.GetCurrentSize());
		}
		{
			ae::_Buffer Buffer;
			Buffer.Write<int64_t>(Stat.second.Time);
			WriteChunk(File, CHUNK_STAT_TIME, &Buffer[0], (int)Buffer.GetCurrentSize());
		}
		{
			ae::_Buffer Buffer;
			Buffer.Write<int>(Stat.second.Value);
			WriteChunk(File, CHUNK_STAT_VALUE, &Buffer[0], (int)Buffer.GetCurrentSize());
		}
	}

	File.close();

	// Rename temp file
	std::remove(Path.c_str());
	std::rename(TempPath.c_str(), Path.c_str());
}

// Backup stats file
void _Achievements::Backup() {
	std::string NewPath = Path + "." + std::to_string(time(nullptr));
	std::remove(NewPath.c_str());
	std::rename(Path.c_str(), NewPath.c_str());
}

