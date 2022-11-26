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
#include <objects/player.h>
#include <objects/monster.h>
#include <objects/inventory.h>
#include <states/play.h>
#include <ae/texture.h>
#include <ae/graphics.h>
#include <ae/assets.h>
#include <ae/animation.h>
#include <ae/program.h>
#include <ae/audio.h>
#include <ae/ui.h>
#include <ae/random.h>
#include <config.h>
#include <hud.h>
#include <framework.h>
#include <gameassets.h>
#include <stats.h>
#include <map.h>
#include <stdexcept>
#include <algorithm>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtx/rotate_vector.hpp>

// Function for sorting by item stats
inline bool CompareItemStats(_Item *First, _Item *Second, bool CompareMods) {
	if(First->ID == Second->ID) {
		if(First->Level == Second->Level) {
			if(CompareMods && First->Quality == Second->Quality)
				return First->Attributes.at("max_mods").Float > Second->Attributes.at("max_mods").Float;

			return First->Quality > Second->Quality;
		}

		return First->Level > Second->Level;
	}

	return First->ID < Second->ID;
}

// Function for sorting items
inline bool CompareItem(_Item *First, _Item *Second) {
	if(First->Type == Second->Type) {
		if(First->Type == _Object::WEAPON) {
			if(First->Template.Attributes.at("weapon_type").Int == Second->Template.Attributes.at("weapon_type").Int)
				return CompareItemStats(First, Second, true);

			return First->Template.Attributes.at("weapon_type").Int < Second->Template.Attributes.at("weapon_type").Int;
		}
		else if(First->Type == _Object::MOD) {
			if(First->Template.Attributes.at("object_type").Int == Second->Template.Attributes.at("object_type").Int) {
				if(First->GetModType() == Second->GetModType()) {
					if(First->Attributes.at("bonus").Float == Second->Attributes.at("bonus").Float)
						return First->Quality > Second->Quality;

					if(First->Template.Attributes.at("negative").Int)
						return First->Attributes.at("bonus").Float < Second->Attributes.at("bonus").Float;
					else
						return First->Attributes.at("bonus").Float > Second->Attributes.at("bonus").Float;
				}

				return First->GetModType() < Second->GetModType();
			}

			return First->Template.Attributes.at("object_type").Int < Second->Template.Attributes.at("object_type").Int;
		}
		else if(First->Type == _Object::USABLE) {
			if(First->Template.Attributes.at("usable_type").Int == Second->Template.Attributes.at("usable_type").Int)
				return First->Quality > Second->Quality;

			return First->Template.Attributes.at("usable_type").Int < Second->Template.Attributes.at("usable_type").Int;
		}

		return CompareItemStats(First, Second, true);
	}

	return First->Type < Second->Type;
}

// Constructor
_Player::_Player(const _ObjectTemplate &PlayerTemplate) :
	_Entity(PlayerTemplate) {

	// Set object properties
	Radius = PLAYER_RADIUS;
	Mass = PLAYER_MASS;

	// Set animation
	Animation->Reels = ae::Assets.Animations["player"];
	Animation->CalculateTextureCoords();

	// Set up animations
	LegAnimation = new ae::_Animation(nullptr);
	LegAnimation->Reels.push_back(ae::Assets.Reels["player_legs"]);
	LegAnimation->CalculateTextureCoords();

	WalkingAnimation = PLAYER_ANIMATIONWALKINGONEHAND;
	MeleeAnimation = PLAYER_ANIMATIONMELEE;
	ShootingOnehandAnimation = PLAYER_ANIMATIONSHOOTONEHAND;
	ShootingTwohandAnimation = PLAYER_ANIMATIONSHOOTTWOHAND;
	DyingAnimation = PLAYER_ANIMATIONDYING;

	// Weapon offsets
	WeaponOffset[0] = glm::vec2(0, 0);
	WeaponOffset[1] = PLAYER_PISTOLOFFSET;
	for(int i = 2; i < WEAPON_COUNT; i++)
		WeaponOffset[i] = PLAYER_WEAPONOFFSET;

	// Allow attacks when player loads
	for(int i = 0; i < WEAPONATTACK_COUNT; i++)
		AttackTimer[i] = 100.0;

	// Inventory
	Inventory = new _Inventory();

	// Skills
	for(int i = 0; i < SKILL_COUNT; i++)
		Skills[i] = 0;

	// Filters
	for(int i = 0; i < FILTER_COUNT; i++) {
		Filters[i] = 0;
		LastFilters[i] = 0;
	}

	// Initialize ammo needed array
	AmmoNeeded.reserve(Stats.AmmoNames.size());
	for(size_t i = 0; i < Stats.AmmoNames.size(); i++)
		AmmoNeeded.push_back(false);

	// Set sounds
	_SoundGroup &SoundGroup = GameAssets.SoundGroups.at("player");
	for(int i = 0; i < SOUND_COUNT; i++)
		Sounds[i] = SoundGroup.SoundID[i];

	// Set up stats
	RecalculateStats();
	ResetWeaponAnimation();

	// Set animation state
	Animation->Play(0);
	Animation->Stop();
	LegAnimation->Stop();

	Health = MaxHealth;
}

// Destructor
_Player::~_Player() {
	delete Inventory;
	delete LegAnimation;
}

// Reset flags associated with achievements
void _Player::ResetAchievementTracking() {
	LavaTouches = 0;
	Stat100Percent = true;
	if(Progression == 1) {
		StatLoneWolf = true;
		StatFistsOnly = true;
	}
	else {
		StatLoneWolf = false;
		StatFistsOnly = false;
	}
}

// Calculates the player's stats from weapons and skills
void _Player::RecalculateStats(bool SoftReset) {
	CalculateExperienceStats();
	CalculateSkillsRemaining();

	// Reset base stats
	Mass = 1.0f;
	DamageBlock = 0;
	SelfDamageResist = 0.0f;
	DamageResist = 0.0f;
	SelfHealPercent = PLAYER_HEAL_PERCENT;
	BaseMoveSpeed = 100.0f;
	float HealthBonus = 100.0f;
	float WeaponDamage[WEAPON_COUNT];
	for(int i = 0; i < WEAPON_COUNT; i++)
		WeaponDamage[i] = 100.0f;

	std::unordered_map<std::string, _Value> WeaponAttributes[WEAPONATTACK_COUNT];
	for(int i = 0; i < WEAPONATTACK_COUNT; i++) {
		Projectiles[i] = nullptr;
		Weapons[i] = nullptr;
	}

	// Set default melee to fists
	Stats.WeaponFists->Level = Level;
	Stats.WeaponFists->RecalculateStats();
	WeaponAttributes[WEAPONATTACK_MELEE] = Stats.WeaponFists->Attributes;
	Weapons[WEAPONATTACK_MELEE] = &Stats.WeaponFists->Template;

	// See if the player is using a weapon
	if(GetMainHand()) {
		WeaponAttributes[WEAPONATTACK_MAIN] = GetMainHand()->Attributes;
		Weapons[WEAPONATTACK_MAIN] = &GetMainHand()->Template;
		MainWeaponType = Weapons[WEAPONATTACK_MAIN]->Attributes.at("weapon_type").Int;
		if(!GetMainHand()->Template.ProjectileID.empty()) {
			Projectiles[WEAPONATTACK_MAIN] = &Stats.Objects.at(GetMainHand()->Template.ProjectileID);
			ProjectileSpeed[WEAPONATTACK_MAIN] = GetMainHand()->Template.Attributes.at("projectile_speed").Float;
		}

		ZoomScale = WeaponAttributes[WEAPONATTACK_MAIN]["zoom_scale"].Float;
	}
	else
		MainWeaponType = WEAPON_MELEE;

	// Get stats of melee weapon
	if(GetMelee()) {
		WeaponAttributes[WEAPONATTACK_MELEE] = GetMelee()->Attributes;
		Weapons[WEAPONATTACK_MELEE] = &GetMelee()->Template;
		MeleeTexture = ae::Assets.Textures[GetMelee()->Template.MeleeID];
		if(!GetMelee()->Template.ProjectileID.empty()) {
			Projectiles[WEAPONATTACK_MELEE] = &Stats.Objects.at(GetMelee()->Template.ProjectileID);
			ProjectileSpeed[WEAPONATTACK_MELEE] = GetMainHand()->Template.Attributes.at("projectile_speed").Float;
		}
	}
	else
		MeleeTexture = ae::Assets.Textures[Stats.WeaponFists->Template.MeleeID];

	// Set up main stats based on weapon
	Recoil = 0;
	AccuracyRegen = 0;
	MoveRecoil = 0.0f;
	AttackRange[WEAPONATTACK_MAIN] = WeaponAttributes[WEAPONATTACK_MAIN]["range"].Float;
	AttackRange[WEAPONATTACK_MELEE] = WeaponAttributes[WEAPONATTACK_MELEE]["range"].Float;
	CurrentAccuracyNormal = 0;
	MinAccuracyNormal = 0;
	MaxAccuracyNormal = 0;
	if(MainWeaponType != WEAPON_MELEE) {
		float StrengthSkillMultiplier = Stats.GetSkillBonusMultiplier(Skills[SKILL_STRENGTH], SKILL_STRENGTH);
		float AccuracySkillMultiplier = 1.0f / Stats.GetSkillBonusMultiplier(Skills[SKILL_PERCEPTION], SKILL_PERCEPTION);
		CurrentAccuracyNormal = MinAccuracyNormal = std::min(360.0f, WeaponAttributes[WEAPONATTACK_MAIN].at("accuracy_min").Float * AccuracySkillMultiplier);
		MaxAccuracyNormal = std::min(360.0f, WeaponAttributes[WEAPONATTACK_MAIN].at("accuracy_max").Float * AccuracySkillMultiplier);
		Recoil = WeaponAttributes[WEAPONATTACK_MAIN]["recoil"].Float / StrengthSkillMultiplier;
		AccuracyRegen = WeaponAttributes[WEAPONATTACK_MAIN]["accuracy_regen"].Float * StrengthSkillMultiplier;
		MoveRecoil = WeaponAttributes[WEAPONATTACK_MAIN]["move_recoil"].Float / StrengthSkillMultiplier;
	}

	// Set accuracy
	ResetAccuracy(!SoftReset);

	// Set skill stats
	BaseMoveSpeed += Stats.GetSkill(Skills[SKILL_CUNNING], SKILL_CUNNING);
	MaxStamina = Stats.GetSkillBonusMultiplier(Skills[SKILL_ENDURANCE], SKILL_ENDURANCE);
	WeaponSwitchPeriod = PLAYER_WEAPONSWITCHPERIOD / Stats.GetSkillBonusMultiplier(Skills[SKILL_DEXTERITY], SKILL_DEXTERITY);
	OutfitSwitchPeriod = PLAYER_OUTFITSWITCHPERIOD / Stats.GetSkillBonusMultiplier(Skills[SKILL_DEXTERITY], SKILL_DEXTERITY);
	HealthBonus += Stats.GetSkill(Skills[SKILL_VITALITY], SKILL_VITALITY);
	HealModifier = Stats.GetSkillBonusMultiplier(Skills[SKILL_VITALITY], SKILL_VITALITY, 1);
	SelfHealStartTime = PLAYER_HEAL_STARTTIME / Stats.GetSkillBonusMultiplier(Skills[SKILL_CUNNING], SKILL_CUNNING, 1);
	SelfHealPeriod = PLAYER_HEAL_PERIOD / Stats.GetSkillBonusMultiplier(Skills[SKILL_CUNNING], SKILL_CUNNING, 1);

	// Add armor bonuses
	DamageResist += Stats.GetSkill(Skills[SKILL_FORTITUDE], SKILL_FORTITUDE, 0);
	SelfDamageResist += Stats.GetSkill(Skills[SKILL_FORTITUDE], SKILL_FORTITUDE, 1);
	Attributes["max_ammo"].Float = 100.0f + Stats.GetSkill(Skills[SKILL_ENDURANCE], SKILL_ENDURANCE, 1);
	if(GetArmor()) {
		Mass += GetArmor()->Template.Attributes.at("mass").Float;
		DamageBlock += std::round(GetArmor()->Attributes.at("damage_block").Float);
		DamageResist += GetArmor()->Attributes.at("damage_resist").Float;
		BaseMoveSpeed += GetArmor()->Attributes.at("move_speed").Float;
		Attributes["max_ammo"].Float += GetArmor()->Attributes.at("max_ammo").Float;
		MaxStamina += GetArmor()->Attributes.at("max_stamina").Float * 0.01f;
		HealthBonus += GetArmor()->Attributes.at("max_health").Float;
		WeaponDamage[WEAPON_MELEE] += GetArmor()->Attributes.at("melee_damage").Float;
		WeaponDamage[WEAPON_PISTOL] += GetArmor()->Attributes.at("pistol_damage").Float;
		WeaponDamage[WEAPON_SHOTGUN] += GetArmor()->Attributes.at("shotgun_damage").Float;
		WeaponDamage[WEAPON_RIFLE] += GetArmor()->Attributes.at("rifle_damage").Float;
		WeaponDamage[WEAPON_HEAVY] += GetArmor()->Attributes.at("heavy_damage").Float;
	}

	// Set attack stats
	for(int i = 0; i < WEAPONATTACK_COUNT; i++) {
		float WeaponDamageModifier = 1.0f;

		// Handle melee damage stat
		if((i == WEAPONATTACK_MAIN && MainWeaponType == WEAPON_MELEE) || i == WEAPONATTACK_MELEE)
			WeaponDamageModifier = (WeaponDamage[WEAPON_MELEE] + Stats.GetSkill(Skills[SKILL_STRENGTH], SKILL_STRENGTH)) * 0.01f;
		// Handle other weapon types
		else if(i == WEAPONATTACK_MAIN)
			WeaponDamageModifier = WeaponDamage[MainWeaponType] * 0.01f;

		FireRateType[i] = WeaponAttributes[i]["fire_rate"].Int;
		AttackPeriod[i] = std::max(WeaponAttributes[i]["fire_period"].Double / Stats.GetSkillBonusMultiplier(Skills[SKILL_AGILITY], SKILL_AGILITY), WEAPON_MINFIREPERIOD);
		MinDamage[i] = std::round(WeaponAttributes[i]["min_damage"].Int * WeaponDamageModifier);
		MaxDamage[i] = std::round(WeaponAttributes[i]["max_damage"].Int * WeaponDamageModifier);
		AttackMoveSpeed[i] = WeaponAttributes[i]["attack_movespeed"].Float;
		ShootPeriod[i] = WeaponAttributes[i]["shoot_period"].Double / Stats.GetSkillBonusMultiplier(Skills[SKILL_STRENGTH], SKILL_STRENGTH, 1);
		Penetration[i] = std::round(WeaponAttributes[i]["penetration"].Float);
		PenetrationDamage[i] = WeaponAttributes[i]["penetration_damage"].Float;
		StartingBounces[i] = std::round(WeaponAttributes[i]["bounces"].Float);
		AttackCount[i] = std::round(WeaponAttributes[i]["attack_count"].Float);
		FireAllRounds[i] = WeaponAttributes[i]["fire_allrounds"].Int;
		CritChance[i] = std::round(WeaponAttributes[i]["crit_chance"].Float);
		CritDamage[i] = PLAYER_CRIT_DAMAGE + Stats.GetSkill(Skills[SKILL_PERCEPTION], SKILL_PERCEPTION, 1);
		BurstRounds[i] = std::round(WeaponAttributes[i]["burst_rounds"].Float);
		if(BurstRounds[i])
			BurstPeriod[i] = std::max(WeaponAttributes[i]["burst_period"].Double / Stats.GetSkillBonusMultiplier(Skills[SKILL_AGILITY], SKILL_AGILITY), WEAPON_MINFIREPERIOD);
		else
			BurstPeriod[i] = AttackPeriod[i];
		ExplosionSize[i] = WeaponAttributes[i]["explosion_size"].Float;
		if(Weapons[i]) {
			AttackWidth[i] = Weapons[i]->Attributes.at("melee_width").Float;
			MeleeScale[i].x = Weapons[i]->Attributes.at("scale_x").Float;
			MeleeScale[i].y = Weapons[i]->Attributes.at("scale_y").Float;
			MeleeOffset[i] = Weapons[i]->Attributes.at("melee_offset").Float;
			MeleeSwitchOffset[i] = Weapons[i]->Attributes.at("melee_switch").Int;
			Push[i] = Weapons[i]->Attributes.at("push").Float;
			Force[i] = Weapons[i]->Attributes.at("force").Float;
		}
	}
	ReloadDelay = AttackPeriod[WEAPONATTACK_MAIN] / Stats.GetSkillBonusMultiplier(Skills[SKILL_DEXTERITY], SKILL_DEXTERITY);
	ReloadPeriod = WeaponAttributes[WEAPONATTACK_MAIN]["reload_period"].Double / Stats.GetSkillBonusMultiplier(Skills[SKILL_DEXTERITY], SKILL_DEXTERITY);

	// Set final stats
	MaxHealth = std::round(Stats.GetLevelHealth(Level) * HealthBonus * 0.01f);
	Health = std::clamp(Health, (int64_t)0, MaxHealth);
	Stamina = std::clamp(Stamina, 0.0f, MaxStamina);
	MoveSpeed = BaseMoveSpeed * 0.01f * PLAYER_MOVESPEED;
	DamageResist = std::min(DamageResist, ENTITY_MAX_DAMAGE_RESIST);
	SelfHealPercent *= HealModifier;

	// Handle max ammo
	AmmoMax.clear();
	for(const auto &AmmoType : Stats.AmmoNames) {
		AmmoMax[AmmoType] = Stats.Ammo.at(AmmoType).Max * Attributes.at("max_ammo").Float * 0.01f + 0.5f;
		if(Ammo.find(AmmoType) != Ammo.end())
			Ammo[AmmoType] = std::min(Ammo[AmmoType], AmmoMax[AmmoType]);
	}
	UpdateAmmoNeeded();

	// Skills
	DropRate = 100 + Stats.GetSkill(Skills[SKILL_LUCK], SKILL_LUCK, 0);
	PickupModifier = Stats.GetSkillBonusMultiplier(Skills[SKILL_LUCK], SKILL_LUCK, 1);
	ExperienceModifier = Stats.GetSkillBonusMultiplier(Skills[SKILL_INTELLIGENCE], SKILL_INTELLIGENCE, 0);
	ExtraMods = Stats.GetSkill(Skills[SKILL_INTELLIGENCE], SKILL_INTELLIGENCE, 1);
}

// Update the player
void _Player::Update(double FrameTime) {
	_Entity::Update(FrameTime);

	// Update timers
	PlayTime += FrameTime;
	ProgressionTime += FrameTime;
	LevelTime += FrameTime;
	UseTimer += FrameTime;
	CombatTimer += FrameTime;
	TakeDamageSoundTimer -= FrameTime;
	if(SwitchingOutfits)
		OutfitSwitchTimer += FrameTime;
	if(Reloading)
		ReloadTimer += FrameTime;
	if(SwitchingWeapons)
		WeaponSwitchTimer += FrameTime;
	if(CombatTimer >= PLAYER_HEAL_STARTTIME && Health < MaxHealth * PLAYER_HEAL_THRESHOLD) {
		SelfHealTimer -= FrameTime;
		if(SelfHealTimer <= 0) {
			SelfHealTimer += SelfHealPeriod;
			int HealAmount = std::max(1, (int)(SelfHealPercent * MaxHealth * 0.01f));
			UpdateHealth(HealAmount);
		}
	}

	for(int i = 0; i < WEAPONATTACK_COUNT; i++)
		AttackTimer[i] += FrameTime;

	if(InvulnerableTimer > 0.0) {
		InvulnerableTimer -= FrameTime;
		if(InvulnerableTimer < 0.0)
			InvulnerableTimer = 0.0;
	}

	if(PoisonTimer > 0.0) {
		PoisonTimer -= FrameTime;
		if(PoisonTimer < 0.0)
			PoisonTimer = 0.0;
	}

	// Update clock
	Clock += FrameTime;
	if(Clock >= MAP_DAY_LENGTH)
		Clock -= MAP_DAY_LENGTH;

	// Update stamina
	if(!IsDying() && !Sprinting)
		Stamina += MaxStamina * PLAYER_STAMINAREGEN * FrameTime;

	if(Stamina > MaxStamina)
		Stamina = MaxStamina;
	if(Tired && Stamina > PLAYER_TIREDTHRESHOLD)
		Tired = false;

	// Update states
	UpdateAnimation(FrameTime);
	UpdateRecoil(FrameTime);
	UpdateReloading();
	UpdateWeaponSwitch();
	UpdateOutfitSwitch();

	// Stop trigger down audio
	if(TriggerDownAudio && (!AttackRequested || !WeaponHasAmmo(WEAPONATTACK_MAIN) || IsDying() || IsSwitching() || Reloading))
		StopAudio();

	// Make an attack
	if(AttackRequested && StartAttack()) {
		AttackRequested = false;
		BurstRoundsShot = 0;
	}

	// Move player
	Move(FrameTime);

	// Check for updated position
	if(PositionChanged) {

		// Warp out of walls
		glm::ivec2 Coord = Map->GetValidCoord(Position);
		if(!Map->CheckCollisionFlag(Coord, _Tile::ENTITY))
			WarpPosition(glm::vec2(LastGoodCoord) + glm::vec2(0.5f));
		else
			LastGoodCoord = Coord;

		// Update stamina
		if(Stamina > 0.0f && Sprinting) {
			Stamina -= PLAYER_SPRINTSTAMINA * FrameTime;
			if(Stamina < 0.0f) {
				Stamina = 0.0f;
				SetSprinting(false);
				Tired = true;
			}
		}
	}
}

// Updates the leg's animation and direction
void _Player::UpdateAnimation(double FrameTime, bool PlaySound) {
	_Entity::UpdateAnimation(FrameTime, false);

	int LastFrame = LegAnimation->Frame;
	LegAnimation->Update(FrameTime);

	// Play move sound on first and last frame of leg animation
	if(PlaySound && LastFrame != LegAnimation->Frame && (LegAnimation->Frame == 0 || LegAnimation->Frame == LegAnimation->Reels[LegAnimation->Reel]->EndFrame))
		ae::Audio.PlaySound(GetSound(SOUND_MOVE, -1));

	switch(MoveState) {
		case MOVE_FORWARD:
			AdjustLegDirection(0);
		break;
		case MOVE_BACKWARD:
			AdjustLegDirection(180);
		break;
		case MOVE_LEFT:
			AdjustLegDirection(270);
		break;
		case MOVE_RIGHT:
			AdjustLegDirection(90);
		break;
		case MOVE_FORWARDLEFT:
			AdjustLegDirection(315);
		break;
		case MOVE_FORWARDRIGHT:
			AdjustLegDirection(45);
		break;
		case MOVE_BACKWARDLEFT:
			AdjustLegDirection(225);
		break;
		case MOVE_BACKWARDRIGHT:
			AdjustLegDirection(135);
		break;
		default:
			AdjustLegDirection(Rotation);
		break;
	}
}

// Draws the player
void _Player::Render(double BlendFactor) const {
	glm::vec2 DrawPosition;
	GetDrawPosition(DrawPosition, BlendFactor);

	float Alpha = 1.0f;
	if(IsInvulnerable())
		Alpha = std::abs(std::fmod(InvulnerableTimer * 5, 1.0)) >= 0.5 ? 0.25f : 0.75f;

	// Draw legs
	ae::Graphics.SetColor(glm::vec4(Color.r, Color.g, Color.b, Color.a * Alpha));
	ae::Graphics.DrawAnimationFrame(
		glm::vec3(DrawPosition, PositionZ),
		LegAnimation->Reels[LegAnimation->Reel]->Texture,
		glm::vec4(LegAnimation->TextureCoords),
		LegDirection,
		glm::vec2(Scale)
	);

	// Draw melee thrust animation
	ae::Graphics.SetColor(glm::vec4(1.0f, 1.0f, 1.0f, Alpha));
	if(Action == ACTION_MELEE && MeleeTexture) {
		float MeleePercent = std::clamp(AttackTimer[AttackRequestType] / AttackPeriod[AttackRequestType], 0.0, 1.0);
		float MeleeMagnitude = std::sin(MeleePercent * glm::pi<double>());
		glm::vec2 NormalDirection(-Direction.y, Direction.x);
		glm::vec2 TextureCoordX = MeleeOffset[AttackRequestType] < 0 ? glm::vec2(1.0f, 0.0f) : glm::vec2(0.0f, 1.0f);

		//TODO fix when range is < 0.5
		// Start position of melee frame behind the player and shift position proportional to magnitude if range is bigger than the melee texture
		glm::vec2 MeleePosition = DrawPosition + NormalDirection * MeleeOffset[AttackRequestType] + Direction * (MeleeScale[AttackRequestType].y * 0.5f + (AttackRange[AttackRequestType] - MeleeScale[AttackRequestType].y) * MeleeMagnitude);
		ae::Graphics.DrawAnimationFrame(
			glm::vec3(MeleePosition, PositionZ + 0.005f),
			MeleeTexture,
			glm::vec4(TextureCoordX[0], MeleeMagnitude - 1.0f, TextureCoordX[1], MeleeMagnitude),
			Rotation,
			MeleeScale[AttackRequestType]
		);
	}

	/*
	// Draw melee swing
	if(Action == ACTION_MELEE) {
		double MeleePercent = std::clamp(AttackTimer[1] / AttackPeriod[1], 0.0, 1.0);
		float MeleeRotation = Rotation + (0.5 - MeleePercent) * MaxAccuracy[WEAPONATTACK_MELEE];
		if(MeleeRotation < 0)
			MeleeRotation += 360;
		else if(MeleeRotation > 360)
			MeleeRotation -= 360;

		//std::cout << MeleeRotation << " " << AttackTimer[1] << " " << AttackPeriod[1] << std::endl;
		glm::vec2 MeleeDirection = glm::rotate(glm::vec2(0, -AttackRange[WEAPONATTACK_MELEE]), glm::radians(MeleeRotation));
		ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
		ae::Graphics.SetDepthMask(false);
		ae::Graphics.SetDepthTest(false);
		ae::Graphics.SetColor(COLOR_WHITE);
		ae::Graphics.DrawLine(Position, Position + MeleeDirection);
		ae::Graphics.SetDepthTest(true);
	}
	*/

	glm::vec4 StateColor(1.0f, 1.0f, 1.0f, Alpha);
	if(PoisonTimer > 0.0) {
		float RedGreen = 1.0f - GetPoisonIntensity();
		StateColor = glm::vec4(RedGreen, 1.0f, RedGreen, Alpha);
	}

	// Draw torso
	ae::Graphics.SetColor(StateColor);
	ae::Graphics.DrawAnimationFrame(
		glm::vec3(DrawPosition, PositionZ + 0.01f),
		Animation->Reels[Animation->Reel]->Texture,
		glm::vec4(Animation->TextureCoords),
		Rotation,
		glm::vec2(Scale)
	);

	//RenderRadius(DrawPosition);
}

// Draws the player in screen space
void _Player::Render2D(const glm::ivec2 &DrawPosition) {
	ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);

	// Draw legs
	const ae::_Reel *LegTemplate = LegAnimation->Reels[LegAnimation->Reel];
	ae::Graphics.SetColor(Color);
	ae::Graphics.DrawAnimationFrame(
		glm::vec3(DrawPosition, 0),
		LegTemplate->Texture,
		glm::vec4(LegAnimation->TextureCoords),
		Rotation,
		glm::vec2(LegTemplate->FrameSize) * ae::_Element::GetUIScale()
	);

	// Draw torso
	const ae::_Reel *WalkTemplate = Animation->Reels[Animation->Reel];
	ae::Graphics.SetColor(COLOR_WHITE);
	ae::Graphics.DrawAnimationFrame(
		glm::vec3(DrawPosition, 0.01f),
		WalkTemplate->Texture,
		glm::vec4(Animation->TextureCoords),
		Rotation,
		glm::vec2(WalkTemplate->FrameSize) * ae::_Element::GetUIScale()
	);
}

// Determines how much to move the leg direction
void _Player::AdjustLegDirection(float Destination) {
	float Distance, Adjust;

	Distance = Destination - LegDirection;

	// Get deltas
	if(Distance < -180.0f)
		Adjust = -(Distance + 180.0f) * PLAYER_LEGCHANGEFACTOR;
	else if(Distance > 180.0f)
		Adjust = -(Distance - 180.0f) * PLAYER_LEGCHANGEFACTOR;
	else
		Adjust = Distance * PLAYER_LEGCHANGEFACTOR;

	// Update leg
	if(std::abs(Adjust) < 0.1f)
		LegDirection = Destination;
	else
		LegDirection += Adjust;

	// Cap direction
	if(LegDirection < 0.0f)
		LegDirection += 360.0f;
	else if(LegDirection >= 360.0f)
		LegDirection -= 360.0f;
}

// Updates the player's experience, leveling up if needed
void _Player::UpdateExperience(int64_t ExperienceGained) {
	Experience = std::clamp(Experience + (int64_t)std::round(ExperienceGained * ExperienceModifier), (int64_t)0, Stats.Levels.back().Experience);

	int OldLevel = Level;
	CalculateExperienceStats();

	// Check for new level
	if(Level > OldLevel) {
		RecalculateStats(true);
		if(Health)
			Health = MaxHealth;
		ae::Audio.PlaySound(ae::Assets.Sounds["game_levelup0.ogg"]);
	}
}

// Updates a skill
void _Player::UpdateSkill(int Index, int Value) {

	Value = std::min(Value, SkillPointsRemaining);
	Value = std::min(Value, Stats.GetMaxSkillLevel(Level) - Skills[Index]);
	int TentativeSum = Value;
	for(int i = 0; i < SKILL_COUNT; i++)
		TentativeSum += Skills[i];

	// Check to make sure skill points don't exceed level
	if(TentativeSum > Stats.GetSkillPointsRemaining(Level))
		return;

	Skills[Index] = Stats.GetValidSkillLevel(Skills[Index] + Value);
	CalculateSkillsRemaining();

	// Update player stats
	RecalculateStats();
}

// Calculates the level and experience variables
void _Player::CalculateExperienceStats() {
	const _Level &LevelStat = Stats.FindLevel(Experience);
	Level = LevelStat.Level;
	ExperienceNextLevel = LevelStat.NextLevel;

	int64_t ExperienceThisLevel = Experience - LevelStat.Experience;
	ExperienceNeeded = (Level == Stats.GetMaxLevel()) ? 0 : LevelStat.NextLevel - ExperienceThisLevel;
	ExperienceLost = std::min(ExperienceThisLevel, (int64_t)(LevelStat.NextLevel * Stats.Progressions[(size_t)Progression].ExperienceLost * 0.01));
}

// Calculates the number of skills points remaining
void _Player::CalculateSkillsRemaining() {
	SkillPointsRemaining = Stats.GetSkillPointsRemaining(Level) - SpentSkillPoints();
}

// Returns the number of skill points the player has spent
int _Player::SpentSkillPoints() const {
	int Sum = 0;
	for(int i = 0; i < SKILL_COUNT; i++)
		Sum += Skills[i];

	return Sum;
}

// Adds an item to the player's possession
int _Player::AddItem(_Item *Item, int &AmountAdded, bool UseOnFull) {

	switch(Item->Type) {
		case _Object::WEAPON: {
			if(Item->IsMelee()) {
				if(!GetMelee() && Config.AutoEquip) {
					GetActiveOutfitBag().Slots[GearType::MELEE] = Item;
					RecalculateStats();
					ResetWeaponAnimation();
					return ADD_REMOVE;
				}
			}
			else {
				if(!GetMainHand() && Config.AutoEquip) {
					GetActiveOutfitBag().Slots[GearType::MAINHAND] = Item;
					RecalculateStats();
					ResetWeaponAnimation();
					return ADD_REMOVE;
				}
				else if(!GetOffHand() && Config.AutoEquip) {
					GetActiveOutfitBag().Slots[GearType::OFFHAND] = Item;
					return ADD_REMOVE;
				}
			}
		} break;
		case _Object::ARMOR: {
			if(!GetArmor() && Config.AutoEquip) {
				GetActiveOutfitBag().Slots[GearType::ARMOR] = Item;
				RecalculateStats();
				return ADD_REMOVE;
			}
		} break;
		case _Object::AMMO: {
			if(Ammo[Item->Template.AmmoID] == AmmoMax[Item->Template.AmmoID])
				return (UseOnFull ? ADD_DELETE : ADD_QUIETFULL);

			int AmountToMax = AmmoMax[Item->Template.AmmoID] - Ammo[Item->Template.AmmoID];

			// Check for unique ammo
			if(Item->Unique)
				AmountAdded = AmountToMax;
			else
				AmountAdded = std::min(AmountToMax, GetPickupAmount(Item));

			Ammo[Item->Template.AmmoID] += AmountAdded;
			UpdateAmmoNeeded();

			return ADD_DELETE;
		}
		case _Object::CONSUMABLE: {
			double CurrentValue = 0;
			double MaxValue = 0;
			int UpdateType = 0;
			if(Item->Template.Attributes.at("health").Float) {
				CurrentValue = Health;
				MaxValue = MaxHealth;
				UpdateType = 1;
			}
			else if(Item->Template.Attributes.at("stamina").Float) {
				CurrentValue = Stamina;
				MaxValue = MaxStamina;
				UpdateType = 2;
			}

			if(!UpdateType)
				throw std::runtime_error(std::string(__func__) + " bad consumable UpdateType");

			// Already full
			if(CurrentValue == MaxValue)
				return (UseOnFull ? ADD_DELETE : ADD_QUIETFULL);

			// Get update amounts
			double AmountToMax = MaxValue - CurrentValue;
			double UpdateAmount = Item->Unique ? AmountToMax : Item->GetConsumableValue(this) * 0.01 * MaxValue;

			// Update stats
			switch(UpdateType) {
				case 1:
					UpdateAmount = std::round(UpdateAmount);
					UpdateHealth(UpdateAmount);
					AmountAdded = std::min(AmountToMax, UpdateAmount);
				break;
				case 2:
					Stamina = std::clamp((double)Stamina + UpdateAmount, 0.0, (double)MaxStamina);
					AmountAdded = std::round(100 * std::min(AmountToMax, UpdateAmount));
				break;
			}

			return ADD_DELETE;
		} break;
		case _Object::KEY: {
			Keys[Item->ID] = 1;
			AmountAdded = 1;
			return ADD_DELETE;
		}
		default:
		break;
	}

	// Update bag stats
	Inventory->UpdateTypeCount();

	// Find bag
	size_t BagIndex = Inventory->FindSuitableBackpackBag(Item, ActiveBackpack, false, PlayState.HUD->InventoryOpen || !Config.AutoOrganize);

	// Add item
	return AddItemToBackpack(Item, BagIndex);
}

// Drop an item from the player's inventory
void _Player::DropItem(const _Slot &Slot, const glm::vec2 &DropPosition) {
	if(!CanDropItems() || !Slot.IsValidIndex())
		return;

	// Get item
	_Item *Item = Slot.GetItem();
	if(!Item)
		return;

	// Remove item from inventory
	Slot.RemoveItem();

	// Check if the item was equipped
	if(Slot.IsGearSlot()) {
		PlayEquipSound(Slot.Index);

		RecalculateStats();
		ResetWeaponAnimation();
	}

	// Add item to map
	if(DropPosition.x < 0.0f)
		Item->SetPosition(Position + _Map::GenerateRandomPointInCircle(PLAYER_RADIUS));
	else
		Item->SetPosition(DropPosition);

	Item->Visible = true;
	Map->AddObject(Item, GRID_ITEM);
}

// Sort inventory
void _Player::SortInventory() {
	ae::Audio.PlaySound(ae::Assets.Sounds["game_click0.ogg"]);

	// Add items to sortable array
	_Bag &Bag = GetActiveBackpackBag();
	std::vector<_Item *> SortBag;
	SortBag.reserve(Bag.Slots.size());
	for(size_t i = 0; i < Bag.Slots.size(); i++) {
		if(!Bag.Slots[i])
			continue;

		SortBag.push_back(Bag.Slots[i]);
		Bag.Slots[i] = nullptr;
	}

	// Sort
	std::sort(SortBag.begin(), SortBag.end(), CompareItem);

	// Add items back in
	for(size_t i = 0; i < SortBag.size(); i++)
		Bag.Slots[i] = SortBag[i];
}

// Determine if an item can be equipped in a slot
bool _Player::CanEquipItem(const _Item *Item, size_t Slot) const {
	if(!Item)
		return true;

	switch(Slot) {
		case GearType::ARMOR: {
			if(Item->Type != _Object::ARMOR)
				return false;

			return true;
		} break;
		case GearType::MAINHAND:
		case GearType::OFFHAND:
		case GearType::MELEE: {
			if(Item->Type == _Object::WEAPON) {
				if(Slot == GearType::MELEE) {
					if(Item->IsMelee())
						return true;
				}
				else
					return !Item->IsMelee();
			}
		} break;
	}

	return false;
}

// Swap inventory
void _Player::SwapInventory(const _Slot &SlotFrom, const _Slot &SlotTo) {
	if(!SlotFrom.IsValidIndex() || !SlotTo.IsValidIndex())
		return;

	_Item *ItemFrom = SlotFrom.GetItem();
	_Item *ItemTo = SlotTo.GetItem();
	if(!ItemFrom)
		return;

	if(SlotFrom == SlotTo)
		return;

	// Try mod
	if(AddMod(SlotFrom, SlotTo))
		return;

	// Try usable
	if(ApplyUsable(SlotFrom, SlotTo))
		return;

	// Prevent swap with gear and non-gear
	if(ItemFrom && !ItemFrom->CanEquip() && ItemTo && ItemTo->CanEquip())
		return;

	// Check for simple swap
	bool CanSwap = false;
	if(!SlotFrom.IsGearSlot() && !SlotTo.IsGearSlot()) {
		CanSwap = true;
	}
	// Gear swap
	else if((SlotFrom.IsGearSlot() && !SlotTo.IsGearSlot()) || (SlotTo.IsGearSlot() && !SlotFrom.IsGearSlot()) || (SlotFrom.IsGearSlot() && SlotTo.IsGearSlot())) {
		if(SlotTo.IsGearSlot())
			CanSwap = CanEquipItem(ItemFrom, SlotTo.Index);
		else
			CanSwap = CanEquipItem(ItemTo, SlotFrom.Index);
	}

	if(!CanSwap)
		return;

	if((SlotTo.IsGearSlot() && SlotTo.Index == GearType::MAINHAND) || (SlotFrom.IsGearSlot() && SlotFrom.Index == GearType::MAINHAND) || (SlotFrom.IsGearSlot() && SlotTo.IsGearSlot() && SlotFrom.IsHandIndex() && SlotTo.IsHandIndex()) ) {
		StartWeaponSwitch(SlotFrom, SlotTo);
	}
	else {

		// Try to combine items
		int CombineResult = CombineItems(ItemFrom, ItemTo);
		if(CombineResult == 0) {
			if(!PlayEquipSound(SlotFrom.Index))
				PlayEquipSound(SlotTo.Index);

			// Swap
			SlotFrom.SetItem(ItemTo);
			SlotTo.SetItem(ItemFrom);
		}
		else if(CombineResult == 2) {
			delete ItemFrom;
			SlotFrom.RemoveItem();
		}
	}

	RecalculateStats();
	ResetWeaponAnimation();
}

// Attempts to combine two items and deletes FromItem if successful
// Return 0 when item can't be combined
// Return 1 when item was combined but still has count left
// Return 2 when item was combined and fromitem needs deletion
int _Player::CombineItems(_Item *FromItem, _Item *ToItem) {

	if(FromItem && ToItem && FromItem->CanStack() && ToItem->CanStack() && FromItem->ID == ToItem->ID && FromItem->Level == ToItem->Level) {
		ToItem->UpdateCount(FromItem->Count);
		if(ToItem->Count > GetInventoryMaxStack()) {
			FromItem->Count = ToItem->Count - GetInventoryMaxStack();
			ToItem->Count = GetInventoryMaxStack();

			return 1;
		}
		else
			return 2;
	}

	return 0;
}

// Add an item to an empty backpack slot
int _Player::AddItemToBackpack(_Item *Item, size_t StartingBagIndex, bool OneBagOnly) {
	if(!Item)
		return ADD_FULL;

	_Container &Container = Inventory->Containers[(size_t)BagType::BACKPACK];
	size_t BagIndex = StartingBagIndex;
	if(BagIndex >= Container.size())
		BagIndex = 0;

	size_t EmptySlot = (size_t)-1;
	for(size_t i = 0; i < Container.size(); i++) {

		// Search for an existing item or empty slot
		_Bag &Bag = Container[BagIndex];
		for(size_t j = 0; j < Bag.Slots.size(); j++) {
			if(CombineItems(Item, Bag.Slots[j]) == 2)
				return ADD_DELETE;

			if(Bag.Slots[j] == nullptr && EmptySlot == (size_t)-1)
				EmptySlot = j;
		}

		// Add item to empty slot
		if(EmptySlot != (size_t)-1) {
			Bag.Slots[EmptySlot] = Item;
			return ADD_REMOVE;
		}

		// Stop searching after one
		if(OneBagOnly)
			return ADD_FULL;

		// Wrap around
		BagIndex++;
		if(BagIndex >= Container.size())
			BagIndex = 0;
	}

	return ADD_FULL;
}

// Add a mod to a weapon
bool _Player::AddMod(const _Slot &SlotFrom, const _Slot &SlotTo) {
	if(!SlotFrom.IsValidIndex() || !SlotTo.IsValidIndex())
		return false;

	_Item *ItemFrom = SlotFrom.GetItem();
	_Item *ItemTo = SlotTo.GetItem();
	if(!ItemFrom || !ItemTo)
		return false;

	if(ItemFrom->Type != _Object::MOD)
		return false;

	if(ItemTo->AddMod(ItemFrom)) {
		ConsumeInventory(SlotFrom, false);
		RecalculateStats();
		return true;
	}

	return false;
}

// Apply usable item to another
bool _Player::ApplyUsable(const _Slot &SlotFrom, const _Slot &SlotTo) {
	if(!SlotFrom.IsValidIndex() || !SlotTo.IsValidIndex())
		return false;

	_Item *ItemFrom = SlotFrom.GetItem();
	_Item *ItemTo = SlotTo.GetItem();
	if(!ItemFrom || !ItemTo)
		return false;

	if(ItemFrom->Type != _Object::USABLE)
		return false;

	if(ItemTo->ApplyUsable(ItemFrom)) {
		ConsumeInventory(SlotFrom, true);
		RecalculateStats();
		return true;
	}

	return false;
}

// Calculates the radius of the crosshair
float _Player::GetCrosshairRadius(const glm::vec2 &Cursor) {

	// Check bounds
	float Accuracy = CurrentAccuracy;
	if(Accuracy < 0.0f)
		Accuracy = 0.0f;
	else if(Accuracy > 180.0f)
		return -1.0f;

	// Get distance to cursor
	float Distance = glm::length(Cursor - Position);

	return tan(glm::radians(Accuracy * 0.5f)) * Distance;
}

// Checks if the player's weapon has ammo
bool _Player::WeaponHasAmmo(int AttackType) const {
	if(AttackType == WEAPONATTACK_MAIN) {
		if(!GetMainHand() || GetMainHand()->Template.AmmoID.empty())
			return true;

		return GetMainHand()->Attributes.at("ammo").Int > 0;
	}
	else if(AttackType == WEAPONATTACK_MELEE) {
		return true;
	}

	return false;
}

// Get number of rounds in main weapon
int _Player::GetWeaponAmmo() const {
	if(!GetMainHand())
		return 0;

	return GetMainHand()->Attributes.at("ammo").Int;
}

// Return active outfit bag
_Bag &_Player::GetActiveOutfitBag() const {
	return Inventory->Containers[(size_t)BagType::OUTFIT][(size_t)ActiveOutfit];
}

// Return active backpack bag
_Bag &_Player::GetActiveBackpackBag() const {
	return Inventory->Containers[(size_t)BagType::BACKPACK][(size_t)ActiveBackpack];
}

// Find a similar equipped item and return its slot
void _Player::GetEquippedCompareSlot(const _Item *Item, bool Offhand, _Slot &Slot) {
	switch(Item->Type) {
		case _Object::WEAPON:
			if(Item->IsMelee()) {
				if(GetMelee()) {
					Slot.Bag = &GetActiveOutfitBag();
					Slot.Index = GearType::MELEE;
				}
			}
			else {
				if(Offhand) {
					if(GetOffHand()) {
						Slot.Bag = &GetActiveOutfitBag();
						Slot.Index = GearType::OFFHAND;
					}
					else if(GetMainHand()) {
						Slot.Bag = &GetActiveOutfitBag();
						Slot.Index = GearType::MAINHAND;
					}
				}
				else {
					if(GetMainHand()) {
						Slot.Bag = &GetActiveOutfitBag();
						Slot.Index = GearType::MAINHAND;
					}
					else if(GetOffHand()) {
						Slot.Bag = &GetActiveOutfitBag();
						Slot.Index = GearType::OFFHAND;
					}
				}
			}
		break;
		case _Object::ARMOR:
			if(GetArmor()) {
				Slot.Bag = &GetActiveOutfitBag();
				Slot.Index = GearType::ARMOR;
			}
		break;
	}
}

// Update ammo needed by the player
void _Player::UpdateAmmoNeeded() {

	for(size_t i = 0; i < Stats.AmmoNames.size(); i++) {
		const auto Iterator = Ammo.find(Stats.AmmoNames[i]);
		if(Iterator == Ammo.end())
			AmmoNeeded[i] = true;
		else
			AmmoNeeded[i] = Iterator->second < AmmoMax.at(Stats.AmmoNames[i]);
	}
}

// Add missing backpack bags based on progression
void _Player::AddMissingBackpacks() {
	int Missing = Stats.Progressions[(size_t)Progression].Backpacks - (int)Inventory->Containers[(size_t)BagType::BACKPACK].size();
	for(int i = 0; i < Missing; i++)
		Inventory->AddBag(BagType::BACKPACK);
}

// Checks if the player has ammo for the main weapon
bool _Player::HasAmmoForMain() const {
	if(!GetMainHand())
		return false;

	const std::string &AmmoType = GetMainHand()->Template.AmmoID;
	if(Ammo.find(AmmoType) == Ammo.end())
		return false;

	return Ammo.at(AmmoType) > 0;
}

// Reduces the player's mainhand weapon ammo and returns the amount reduced
int _Player::ReduceAmmo(int Amount) {
	if(GetMainHand() && AttackRequestType == WEAPONATTACK_MAIN) {
		Amount = std::min(GetMainHand()->Attributes["ammo"].Int, Amount);
		GetMainHand()->Attributes["ammo"].Int = std::max(GetMainHand()->Attributes["ammo"].Int - Amount, 0);

		UpdateAmmoNeeded();
	}

	return Amount;
}

// Begins the reloading process
void _Player::StartReloading() {

	// Test conditions
	if(!CanReload())
		return;

	// Stop reloading sound
	CancelReloading();

	// Play sound
	ReloadSound = ae::Audio.PlaySound(GetSound(SOUND_RELOAD, WEAPONATTACK_MAIN));

	// Start timer
	ReloadTimer = 0.0;
	Reloading = true;
}

// Cancel the reload process
void _Player::CancelReloading() {

	// Stop existing sound
	if(ReloadSound)
		ReloadSound->Stop();

	ReloadSound = nullptr;
	Reloading = false;
}

// Begins the weapon switch process
void _Player::StartWeaponSwitch(const _Slot &SlotFrom, const _Slot &SlotTo) {

	// Test conditions
	if(!CanSwitchWeapons())
		return;

	// Test for empty hands
	if(SlotFrom.IsHandIndex() && SlotTo.IsHandIndex() && !GetMainHand() && !GetOffHand())
		return;

	CancelReloading();

	// Start timer
	WeaponSwitchFrom = SlotFrom;
	WeaponSwitchTo = SlotTo;
	WeaponSwitchTimer = 0.0;
	SwitchingWeapons = true;

	ae::Audio.PlaySound(ae::Assets.Sounds["equip_gun0.ogg"]);
}

// Start outfit switch process
void _Player::StartOutfitSwitch(size_t Outfit) {
	if(Outfit == ActiveOutfit)
		return;

	// Check index
	if(Outfit >= Inventory->Containers[(size_t)BagType::OUTFIT].size())
		return;

	// Test conditions
	if(!CanSwitchOutfits())
		return;

	CancelReloading();

	// Start timer
	OutfitSwitchTo = Outfit;
	OutfitSwitchTimer = 0.0;
	SwitchingOutfits = true;

	ae::Audio.PlaySound(ae::Assets.Sounds["equip_armor0.ogg"]);
}

// Reloads the weapon when the timer goes off
void _Player::UpdateReloading() {
	if(!Reloading || ReloadTimer <= ReloadPeriod)
		return;

	// Stop reload sound
	CancelReloading();

	// Check weapon type
	if(!CanReload())
		return;

	// Get amounts
	const std::string &AmmoType = GetMainHand()->Template.AmmoID;
	int AmountNeeded = std::round(GetMainHand()->Attributes["rounds"].Float) - GetMainHand()->Attributes["ammo"].Int;
	int AmmoLoadAmount = std::min(Ammo[AmmoType], AmountNeeded);

	// Handle different reload amounts
	bool ReloadAgain = false;
	if(GetMainHand()->Attributes["reload_amount"].Float) {
		AmmoLoadAmount = std::min(std::max(1, (int)std::round(GetMainHand()->Attributes["reload_amount"].Float)), AmmoLoadAmount);
		ReloadAgain = true;
	}

	// Update ammo
	GetMainHand()->Attributes["ammo"].Int += AmmoLoadAmount;
	Ammo[AmmoType] -= AmmoLoadAmount;
	UpdateAmmoNeeded();

	// Reset player
	ResetAccuracy(false);
	ResetWeaponAnimation();
	if(ReloadAgain)
		StartReloading();
}

// Switches weapons when timer expires
void _Player::UpdateWeaponSwitch() {
	if(!SwitchingWeapons || WeaponSwitchTimer <= WeaponSwitchPeriod)
		return;

	SwitchingWeapons = false;

	// Check weapon type
	if(!CanSwitchWeapons())
		return;

	if(!WeaponSwitchFrom.IsValidIndex() || !WeaponSwitchTo.IsValidIndex())
		return;

	_Item *ItemFrom = WeaponSwitchFrom.GetItem();
	_Item *ItemTo = WeaponSwitchTo.GetItem();

	WeaponSwitchFrom.SetItem(ItemTo);
	WeaponSwitchTo.SetItem(ItemFrom);

	RecalculateStats();
	ResetWeaponAnimation();
}

// Switch outfits when timer expires
void _Player::UpdateOutfitSwitch() {
	if(!SwitchingOutfits || OutfitSwitchTimer <= OutfitSwitchPeriod)
		return;

	SwitchingOutfits = false;

	// Test condition
	if(!CanSwitchOutfits())
		return;

	ActiveOutfit = OutfitSwitchTo;
	RecalculateStats();
	ResetWeaponAnimation();
}

// Updates the move speed modifier for aiming and running
void _Player::UpdateSpeed(float Factor) {
	_Entity::UpdateSpeed(Factor);

	if(Aiming)
		MoveModifier *= PLAYER_AIM_MOVESPEEDFACTOR;
	else if(Sprinting)
		MoveModifier *= PLAYER_SPRINT_SPEEDFACTOR;

	MoveModifier *= Factor;

	LegAnimation->FramePeriod = LegAnimation->Reels[0]->FramePeriod / MoveModifier;
	LegAnimation->Timer = std::min(LegAnimation->Timer, LegAnimation->FramePeriod);
	if(Animation->Reel == PLAYER_ANIMATIONWALKINGONEHAND || Animation->Reel == PLAYER_ANIMATIONWALKINGTWOHAND)
		SetAnimationPlaybackSpeedFactor();
}

// Updates the states for aiming
void _Player::SetAiming(bool State) {

	// Update state
	if(Aiming != State) {
		Aiming = State;
		ResetAccuracy(false);
	}

	// Disable sprint
	if(Aiming)
		SetSprinting(false);
}

// Update state for sprinting
void _Player::SetSprinting(bool State) {
	if(State && Tired)
		return;

	// Update state
	if(Sprinting != State) {
		Sprinting = State;
		ResetAccuracy(false);
	}

	if(Sprinting)
		SetAiming(false);
}

// Resets the accuracy depending on aiming state
void _Player::ResetAccuracy(bool CompleteReset) {

	if(Aiming && !IsMelee())
		RecoilModifier = PLAYER_AIM_RECOIL_MODIFIER;
	else if(Sprinting && !IsMelee())
		RecoilModifier = PLAYER_SPRINT_RECOIL_MODIFIER;
	else
		RecoilModifier = 1.0f;

	if(CompleteReset) {
		CurrentAccuracy = 0;
		if(!IsMelee())
			CurrentAccuracy = MaxAccuracyNormal;
	}

	MinAccuracy = MinAccuracyNormal;
	MaxAccuracy[WEAPONATTACK_MAIN] = MaxAccuracyNormal;
}

// Consume an item from the inventory
void _Player::ConsumeInventory(const _Slot &Slot, bool Delete) {
	if(!Slot.IsValidIndex())
		return;

	_Item *Item = Slot.GetItem();
	if(!Item)
		return;

	// Delete item
	if(Item->UpdateCount(-1) <= 0) {
		if(Delete)
			delete Item;

		Slot.RemoveItem();
	}
}

// Get amount of ammo picked up
int _Player::GetPickupAmount(const _Item *Item) const {
	if(Item->Template.Attributes.at("pickup_bonus").Int)
		return std::round(Item->Template.Attributes.at("amount").Int * PickupModifier);

	return Item->Template.Attributes.at("amount").Int;
}

// Reset after death
void _Player::Respawn() {
	if(Hardcore)
		return;

	Active = true;
	Health = MaxHealth;
	Action = ACTION_IDLE;
	Animation->Play(0);
	Animation->Stop();
	LegAnimation->Stop();

	RecalculateStats();
	ResetWeaponAnimation();
	StopAudio();
	WarpPosition(Map->GetStartingPositionByCheckpoint(CheckpointIndex));

	Map->AmbientClock = Map->BaseAmbientClock;
	Map->TargetAmbientLight = Map->BaseAmbientLight;

	InvulnerableTimer = GAME_INVULNERABLE_TIME;
	CombatTimer = GAME_COMBAT_TIMER;
}

// Change player position and update grid
void _Player::WarpPosition(const glm::vec2 &NewPosition) {
	Map->RemoveObjectFromGrid(this, GRID_PLAYER);
	SetPosition(NewPosition);
	Map->AddObjectToGrid(this, GRID_PLAYER);

	LastGoodCoord = Position;
	TileChanged = true;
}

// Sets the weapon animation for the player
void _Player::ResetWeaponAnimation() {
	if(IsDying())
		return;

	// Get walking animation
	switch(MainWeaponType) {
		case WEAPON_MELEE:
		case WEAPON_PISTOL:
			WalkingAnimation = PLAYER_ANIMATIONWALKINGONEHAND;
		break;
		default:
			WalkingAnimation = PLAYER_ANIMATIONWALKINGTWOHAND;
		break;
	}

	Action = ACTION_IDLE;
	Animation->Stop();
	Animation->Play(WalkingAnimation, MoveSpeed);
	Animation->CalculateTextureCoords();
	SetAnimationPlaybackSpeedFactor();
}

// Applies the death penalty
void _Player::ApplyDeathPenalty() {
	Reloading = false;
	SwitchingWeapons = false;
	SwitchingOutfits = false;
	TotalDeaths++;
	ProgressionDeaths++;

	Experience -= ExperienceLost;
	CalculateExperienceStats();
}

// Returns a sound index
const ae::_Sound *_Player::GetSound(int SoundType, int AttackType) const {
	if(AttackType == WEAPONATTACK_MAIN && GetMainHand())
		return GetMainHand()->GetSound(SoundType);
	else if(AttackType == WEAPONATTACK_MELEE && GetMelee())
		return GetMelee()->GetSound(SoundType);

	return _Entity::GetSound(SoundType, AttackType);
}

// Get a particle from either the main weapon or player group
const _ParticleTemplate *_Player::GetParticle(int ParticleType) const {
	if(ParticleType == PARTICLE_HIT || ParticleType == PARTICLE_FLOORDECAL) {
		const auto &Template = GameAssets.ParticleGroups.at("player").ParticleTemplates[ParticleType];
		if(Template.empty())
			return nullptr;

		return Template[ae::GetRandomInt((size_t)0, Template.size()-1)];
	}

	if(GetMainHand()) {
		const auto &Template = GetMainHand()->Template.ParticleGroup->ParticleTemplates[ParticleType];
		if(Template.empty())
			return nullptr;

		return Template[ae::GetRandomInt((size_t)0, Template.size()-1)];
	}

	return nullptr;
}

// Get current main hand weapon
_Item *_Player::GetMainHand() const {
	return GetActiveOutfitBag().Slots[GearType::MAINHAND];
}

// Get current offhand weapon
_Item *_Player::GetOffHand() const {
	return GetActiveOutfitBag().Slots[GearType::OFFHAND];
}

// Get current melee weapon
_Item *_Player::GetMelee() const {
	return GetActiveOutfitBag().Slots[GearType::MELEE];
}

// Get current armor
_Item *_Player::GetArmor() const {
	return GetActiveOutfitBag().Slots[GearType::ARMOR];
}

// Set attack requested
void _Player::RequestAttack(int RequestType) {
	AttackRequested = true;
	AttackRequestType = RequestType;
	if(MeleeSwitchOffset[RequestType])
		MeleeOffset[RequestType] = -MeleeOffset[RequestType];

	AttackWasSteady = false;
	if(RequestType == WEAPONATTACK_MAIN)
		AttackWasSteady = IsSteady();
}

// Sets the color string and color of the player
void _Player::UpdateColor() {
	Color = ae::Assets.Colors[ColorID];
}

// Called when the player gets hit
void _Player::OnHit(_Entity *Attacker, const _Hit &Hit, bool PlaySound) {
	if(IsInvulnerable())
		return;

	if(!PlayState.HUD->LastHitMaxHealth)
		PlayState.HUD->SetLastHit(Attacker);

	PlaySound = false;
	if(TakeDamageSoundTimer <= 0.0) {
		PlaySound = true;
		TakeDamageSoundTimer = PLAYER_TAKEDAMAGE_SOUND_COOLDOWN * ae::GetRandomReal(1.0, 1.5);
	}

	_Entity::OnHit(Attacker, Hit, PlaySound);
	SelfHealTimer = SelfHealPeriod;
}

// Play equip sounds
bool _Player::PlayEquipSound(size_t Slot) const {

	switch(Slot) {
		case GearType::MAINHAND:
		case GearType::OFFHAND:
			ae::Audio.PlaySound(ae::Assets.Sounds["equip_gun0.ogg"]);
			return true;
		break;
		case GearType::MELEE:
			ae::Audio.PlaySound(ae::Assets.Sounds["equip_melee0.ogg"]);
			return true;
		break;
		case GearType::ARMOR:
			ae::Audio.PlaySound(ae::Assets.Sounds["equip_armor0.ogg"]);
			return true;
		break;
	}

	return false;
}

int _Player::GetInventoryMaxStack() const {
	return INVENTORY_MAX_STACK;
}

bool _Player::CanReload() const {
	return GetMainHand() && AttackTimer[WEAPONATTACK_MAIN] >= ReloadDelay && !Reloading && !IsSwitching() && !IsMeleeAttacking() && GetMainHand()->Attributes.at("ammo").Int != std::round(GetMainHand()->Attributes.at("rounds").Float) && HasAmmoForMain();
}

bool _Player::IsMelee() const {
	return GetMainHand() == nullptr || GetMainHand()->IsMelee();
}

// Get weapon id from attack type
const char *_Player::GetWeaponID(int AttackType) {
	if(AttackType == WEAPONATTACK_MAIN) {
		if(GetMainHand())
			return GetMainHand()->Template.ID.c_str();
		else if(GetMelee())
			return GetMelee()->Template.ID.c_str();
	}
	else if(GetMelee())
		return GetMelee()->Template.ID.c_str();

	return "weapon_fists";
}

void _Player::SetLegAnimationPlayMode(int Mode) {
	if(Mode == ae::_Animation::PLAYING)
		LegAnimation->Play(0);
	else if(Mode == ae::_Animation::STOPPED)
		LegAnimation->Stop();
}

void _Player::SetAnimationPlaybackSpeedFactor() {
	Animation->FramePeriod = Animation->Reels[Animation->Reel]->FramePeriod / MoveModifier;
}
