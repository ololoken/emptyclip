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
#include <utils.h>
#include <random.h>
#include <cmath>
#include <glm/gtx/rotate_vector.hpp>

// Loads a file into a string
const char *LoadFileIntoMemory(const char *Path) {

	// Open file
	std::ifstream File(Path, std::ios::binary);
	if(!File)
		return nullptr;

	// Get file size
	File.seekg(0, std::ios::end);
	std::ifstream::pos_type Size = File.tellg();
	if(!Size)
		return nullptr;

	File.seekg(0, std::ios::beg);

	// Read data
	char *Data = new char[(std::size_t)Size + 1];
	File.read(Data, Size);
	File.close();
	Data[(std::size_t)Size] = 0;

	return Data;
}

// Reads in a string that is CSV formatted
std::string GetCSVText(std::ifstream &Stream) {
	std::string Text;
	char Char;

	// Ignore the first space
	if(Stream.peek() == ' ')
		Stream.ignore(1, ' ');

	// If there's a quote, ignore spaces until another quote is read
	if(Stream.peek() == '\"') {
		Stream.ignore(1);
		Stream.get(Char);
		while(Char != '\"') {
			Text += Char;
			Stream.get(Char);
		}
	}
	else
		return "";

	return Text;
}

// Write a chunk to a stream
void WriteChunk(std::ofstream &File, int Type, const char *Data, size_t Size) {
	File.write((char *)&Type, sizeof(Type));
	File.write((char *)&Size, sizeof(Size));
	File.write(Data, Size);
}

// Generates a random point inside of a circle
glm::vec2 GenerateRandomPointInCircle(float Radius) {

	return glm::rotate(glm::vec2(0, -1), glm::radians((float)Random.Generate() * 360.0f)) * Radius * (float)sqrt(Random.Generate());
}
