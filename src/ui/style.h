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

// Libraries
#include <color.h>
#include <string>

// Forward Declarations
class _Texture;

// Classes
class _Style {

	public:

		_Style(const std::string &Identifier, bool HasBackgroundColor, bool HasBorderColor, const glm::vec4 &BackgroundColor, const glm::vec4 &BorderColor, const _Texture *Texture, const glm::vec4 &TextureColor, bool Stretch);
		~_Style();

		void SetIdentifier(const std::string &Identifier) { this->Identifier = Identifier; }
		const std::string &GetIdentifier() const { return Identifier; }

		void SetHasBackgroundColor(bool HasBackgroundColor) { this->HasBackgroundColor = HasBackgroundColor; }
		bool GetHasBackgroundColor() const { return HasBackgroundColor; }

		void SetHasBorderColor(bool HasBorderColor) { this->HasBorderColor = HasBorderColor; }
		bool GetHasBorderColor() const { return HasBorderColor; }

		void SetBackgroundColor(const glm::vec4 &BackgroundColor) { this->BackgroundColor = BackgroundColor; }
		const glm::vec4 &GetBackgroundColor() const { return BackgroundColor; }

		void SetBorderColor(const glm::vec4 &BorderColor) { this->BorderColor = BorderColor; }
		const glm::vec4 &GetBorderColor() const { return BorderColor; }

		void SetTexture(_Texture *Texture) { this->Texture = Texture; }

		void SetTextureColor(const glm::vec4 &TextureColor) { this->TextureColor = TextureColor; }
		const glm::vec4 &GetTextureColor() const { return TextureColor; }

		void SetStretch(bool Stretch) { this->Stretch = Stretch; }
		bool GetStretch() const { return Stretch; }

		std::string Identifier;
		const _Texture *Texture;
		glm::vec4 BackgroundColor;
		glm::vec4 BorderColor;
		glm::vec4 TextureColor;
		bool HasBackgroundColor;
		bool HasBorderColor;
		bool Stretch;

};
