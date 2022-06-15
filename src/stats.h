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
#include <objects/templates.h>
#include <unordered_map>
#include <vector>
#include <string>

// Forward Declarations

// Used for level information
struct _Level {
	int64_t Experience;
	int HealthBonus;
	int DamageBlockBonus;
	int SkillPoints;
};

// Holds skill information
struct _Skill {
	float Data[SKILL_COUNT];
};

// Classes
class _Stats {

	public:

		void Init();
		void Close();

		void LoadLevels(const std::string &Path);
		void LoadSkills(const std::string &Path);

		int GetLevel(int64_t Experience);
		int64_t GetValidExperience(int64_t Experience);
		int64_t GetExperienceForLevel(int Level);
		int GetLevelHealth(int Level) { return Levels[Level-1].HealthBonus; }
		int GetLevelDamageBlock(int Level) { return Levels[Level-1].DamageBlockBonus; }
		int GetSkillPointsRemaining(int Level) { return Levels[Level-1].SkillPoints; }

		int GetValidSkill(int Level);
		float GetSkill(int Level, int Type) const { return Skills[Level].Data[Type]; }
		float GetSkillPercentImprovement(int Level, int Type) const { return (Skills[Level].Data[Type] - 1.0f) * 100.0f; }

		std::vector<_Level> Levels;
		std::vector<_Skill> Skills;

	private:

};

extern _Stats Stats;
