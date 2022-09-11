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
#include <achievements.h>
#include <ae/buffer.h>
#include <constants.h>
#include <config.h>
#include <fstream>

static const char *FILENAME = "stats.db";

enum AchievementChunkTypes {
	CHUNK_VERSION,
	CHUNK_STATS,
};

// Write a chunk to a stream
static void WriteChunk(std::ofstream &File, int Type, const char *Data, int Size) {
	File.write((char *)&Type, sizeof(Type));
	File.write((char *)&Size, sizeof(Size));
	File.write(Data, Size);
}

_Achievements Achievements;

// Load achievement stats
void _Achievements::Load() {
	if(!Enabled)
		return;

	std::string Path = Config.ConfigPath + FILENAME;

	// Open file
	std::ifstream File(Path.c_str(), std::ios::in | std::ios::binary);
	if(!File)
		throw std::runtime_error("Cannot load file: " + Path);

	// Read file
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
				}
			} break;
			case CHUNK_STATS: {
				Stats.clear();

				// Load buffer
				ae::_Buffer Buffer(Size);
				File.read(&Buffer[0], Size);

				// Load stats
				int Count = Buffer.Read<int>();
				for(int i = 0; i < Count; i++) {
					const char *ID = Buffer.ReadString();
					int Value = Buffer.Read<int>();
					Stats[ID] = Value;
				}
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
	if(!Enabled)
		return;

	// Open file
	std::string TempPath = Config.ConfigPath + "_stats.db";
	std::ofstream File(TempPath.c_str(), std::ios::out | std::ios::binary);
	if(!File.is_open())
		throw std::runtime_error("Cannot create file: " + TempPath);

	// Write header
	WriteChunk(File, CHUNK_VERSION, (const char *)&ACHIEVEMENTS_VERSION, sizeof(ACHIEVEMENTS_VERSION));

	// Build stats buffer
	ae::_Buffer Buffer;
	Buffer.Write<int>(Stats.size());
	for(const auto &Stat : Stats) {
		Buffer.WriteString(Stat.first.c_str());
		Buffer.Write<int>(Stat.second);
	}

	// Write stats
	WriteChunk(File, CHUNK_STATS, &Buffer[0], Buffer.GetCurrentSize());

	File.close();

	// Rename temp file
	std::string Path = Config.ConfigPath + FILENAME;
	std::rename(TempPath.c_str(), Path.c_str());
}

