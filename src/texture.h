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

#include <ae/opengl.h>
#include <glm/vec2.hpp>
#include <string>

struct SDL_Surface;

// Classes
class _Texture {

	public:

		_Texture(const std::string &Path) : Name(Path), ID(0) { }
		_Texture(const std::string &Path, bool IsServer, bool Repeat, bool Mipmaps, bool Nearest);
		_Texture(const std::string &Path, FILE *FileHandle, bool IsServer, bool Repeat, bool Mipmaps, bool Nearest);
		_Texture(unsigned char *Data, const glm::ivec2 &Size, int InternalFormat, GLenum Format);
		~_Texture();

		// Info
		std::string Name;
		GLuint ID;

		// Dimensions
		glm::ivec2 Size;

	private:

		void Load(SDL_Surface *Image, bool Repeat, bool Mipmaps, bool Nearest);

};
