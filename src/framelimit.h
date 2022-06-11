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
#include <chrono>
#include <thread>

class _FrameLimit {

	public:

		// Constructor
		_FrameLimit(double FrameRate) : FrameRate(FrameRate), ExtraTime(0.0) { Reset(); }

		// Reset timer
		void Reset() {
			Timer = std::chrono::high_resolution_clock::now();
		}

		// Update frame rate
		void SetFrameRate(double Value) {
			FrameRate = Value;
			ExtraTime = 0.0;
			Reset();
		}

		// Limit frame rate
		void Update() {
			if(FrameRate <= 0.0)
				return;

			// Account for extra time from last frame
			double Elapsed = std::chrono::duration<int64_t, std::nano>(std::chrono::high_resolution_clock::now() - Timer).count() - ExtraTime;
			Reset();

			// Get sleep duration
			ExtraTime = 1000000000 / FrameRate - Elapsed;
			if(ExtraTime > 0)
				std::this_thread::sleep_for(std::chrono::nanoseconds((int64_t)(ExtraTime)));
			else
				ExtraTime = 0;
		}

	private:

		std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> Timer;
		double FrameRate;
		double ExtraTime;

};
