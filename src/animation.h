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
#include <string>
#include <vector>

// Forward Declarations
namespace ae {
	class _Texture;
}

// Enumerations
enum PlayType {
	PLAYING,
	STOPPED,
};

enum RepeatType {
	STOP,
	WRAP,
	BOUNCE
};

// Used for storing information about a template reel
struct _ReelTemplate {
	_ReelTemplate() { }

	std::vector<std::string> TextureFiles;
	float PlaybackSpeed;
	int RepeatMode, StartPosition;
};

// Used for storing information about an animation sequence
struct _Reel {
	std::vector<ae::_Texture *> Textures;
	double FramePeriod;
	RepeatType RepeatMode;
	int StartPosition;
};

// Classes
class _Animation {

	public:

		_Animation();
		_Animation &operator=(const _Animation &Animation);

		void Update(double FrameTime);
		void ChangeReel(int Index);

		void SetFramePeriod(double Value);
		void SetPlaybackSpeedFactor(double Value) { FramePeriod = Reels[CurrentReel]->FramePeriod * Value; }
		void SetPlayMode(int Mode);
		void SetAllowUpdate(bool Value) { AllowUpdate = Value; }

		ae::_Texture *GetCurrentFrame() const;
		ae::_Texture *GetStartPositionFrame() const;

		std::vector<const _Reel *> Reels;
		int PlayMode;
		int PlayDirection;
		int CurrentReel;
		int Position;
		double Timer;
		double FramePeriod;
		bool AllowUpdate;

};
