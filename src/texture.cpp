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
#include <texture.h>
#include <graphics.h>
#include <SDL_image.h>
#include <stdexcept>


// Load from file
_Texture::_Texture(const std::string &Path, bool IsServer, bool Repeat, bool Mipmaps, bool Nearest) :
	_Texture(Path) {

	if(IsServer)
		return;

	// Open file
	SDL_Surface *Image = IMG_Load(Path.c_str());
	if(!Image)
		throw std::runtime_error("Error loading image: " + Path + " with error: " + IMG_GetError());

	Load(Image, Repeat, Mipmaps, Nearest);
	SDL_FreeSurface(Image);
}

// Load from file handle
_Texture::_Texture(const std::string &Path, FILE *FileHandle, bool IsServer, bool Repeat, bool Mipmaps, bool Nearest) :
	_Texture(Path) {

	if(IsServer)
		return;

	// Open file
	SDL_RWops *SDLBuffer = SDL_RWFromFP(FileHandle, SDL_FALSE);
	SDL_Surface *Image = IMG_Load_RW(SDLBuffer, SDL_FALSE);
	SDL_RWclose(SDLBuffer);
	if(!Image)
		throw std::runtime_error("Error loading image: " + Path + " with error: " + IMG_GetError());

	// Load texture
	Load(Image, Repeat, Mipmaps, Nearest);
	SDL_FreeSurface(Image);
}

// Load texture from SDL_Surface
void _Texture::Load(SDL_Surface *Image, bool Repeat, bool Mipmaps, bool Nearest) {
	Size.x = Image->w;
	Size.y = Image->h;

	// Determine OpenGL format
	GLint ColorFormat;
	switch(Image->format->BitsPerPixel) {
		case 24:
			ColorFormat = GL_RGB;
		break;
		case 32:
			ColorFormat = GL_RGBA;
		break;
		default:
			throw std::runtime_error("Unsupported bpp " + std::to_string(Image->format->BitsPerPixel) + " for texture " + Name);
	}

	// Create texture and upload to GPU
	glGenTextures(1, &ID);
	glBindTexture(GL_TEXTURE_2D, ID);
	if(Repeat) {
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	else {
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	// Set magnification filter
	GLfloat MagFilter = GL_LINEAR;
	if(Nearest)
		MagFilter = GL_NEAREST;

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, MagFilter);

	if(Mipmaps) {
		if(Graphics.Anisotropy > 0)
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, Graphics.Anisotropy);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	}
	else {
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}

	// Create texture
	glTexImage2D(GL_TEXTURE_2D, 0, ColorFormat, Size.x, Size.y, 0, (GLenum)ColorFormat, GL_UNSIGNED_BYTE, Image->pixels);
	if(Mipmaps)
		glGenerateMipmap(GL_TEXTURE_2D);
}

// Initialize from buffer
_Texture::_Texture(unsigned char *Data, const glm::ivec2 &Size, GLint InternalFormat, GLenum Format) :
	Size(Size) {

	// Create texture
	glGenTextures(1, &ID);
	glBindTexture(GL_TEXTURE_2D, ID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, Size.x, Size.y, 0, Format, GL_UNSIGNED_BYTE, Data);
}

// Destructor
_Texture::~_Texture() {
	if(ID)
		glDeleteTextures(1, &ID);
}
