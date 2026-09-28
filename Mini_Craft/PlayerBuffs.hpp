// =====================================================================
//  PlayerBuffs.hpp
// =====================================================================
//  Player stat modifiers / buffs for Mini Craft: what happens to the
//  player's numbers when a crafted potion or scroll is consumed.
//
//  Builds on the per-character base numbers already defined in
//  Player.hpp's CH[3] table (health, moveSpeed, resourceCostMultiplier,
//  potionConsumeSpeed, ...) so the three classes keep feeling different
//  even though they all run through the same buff functions.
//
//  Covers all 4 crafted potions from CraftingEngine.hpp's RECIPE_BOOK:
//    - Movement Potion      -> applyMovementPotion()      (temporary, timed)
//    - Power Potion         -> applyPowerPotion()         (temporary, timed)
//    - Reinforcement Potion -> applyReinforcementPotion()  (temporary, timed)
//    - Health Potion        -> applyHealthPotion()         (temporary, ticks HP back over time)
//    - Damage Scroll        -> applyDamageScroll()          (permanent)
//
//  Self-contained: does not touch iMain.cpp or any other existing file.
//  Call updatePlayerBuffs() once per tick (e.g. from your fixedUpdate())
//  so timed buffs count down and the Health Potion's heal-over-time
//  actually ticks in.
//
//  Written for Visual Studio 2013 (no C++11 features used) + iGraphics.
// =====================================================================
#pragma once

#include "iGraphics.h"
#include "Player.hpp"   // CH[3] - per-character base stats
#include <windows.h>    // GetTickCount()
#include <stdio.h>      // sprintf()

// ---------------------------------------------------------------
//  BUFF NUMBERS  (tweak here, not inside the functions below - each
//  comment gives the design range from the spec; the #define picks
//  one concrete value from inside that range)
// ---------------------------------------------------------------

// Movement Potion (Swiftness Potion): +40% to +60% speed, 15-20s.
#define MOVEMENT_POTION_SPEED_MULT      1.5f      // +50% speed (mid of 1.4x-1.6x)
#define MOVEMENT_POTION_DURATION_MS     18000UL   // 18s (mid of 15-20s)

// Power Potion: +50% to +100% attack damage, 10-15s.
#define POWER_POTION_DAMAGE_MULT        1.75f     // +75% damage (mid of 1.5x-2.0x)
#define POWER_POTION_DURATION_MS        12000UL   // 12s (mid of 10-15s)

// Reinforcement Potion: up to 2x armor/defense, 12s exactly.
#define REINFORCEMENT_POTION_DEFENSE_MULT  2.0f
#define REINFORCEMENT_POTION_DURATION_MS   12000UL

// Health Potion: +30 HP, applied gradually (heal-over-time) across 12s.
#define HEALTH_POTION_TOTAL_HEAL        30
#define HEALTH_POTION_DURATION_MS       12000UL

// Damage Scroll: permanent, stacks multiplicatively.
#define DAMAGE_SCROLL_BONUS_PERCENT     0.20f     // +20%, permanent

// ---------------------------------------------------------------
//  BuffTimer  - small reusable "is this temporary effect still on"
//  helper so every timed buff below shares the same start/expire logic
//  instead of re-deriving it four separate times.
// ---------------------------------------------------------------
struct BuffTimer
{
	bool          active;
	unsigned long startTime;
	unsigned long durationMs;
};

inline void startBuffTimer(BuffTimer &t, unsigned long durationMs)
{
	t.active = true;
	t.startTime = GetTickCount();
	t.durationMs = durationMs;
}

// True if the timer's duration has elapsed (call once per tick from
// updatePlayerBuffs() - does NOT clear t.active itself, so the caller
// can still run "buff just expired" cleanup, e.g. reset speed to base).
inline bool buffTimerExpired(const BuffTimer &t)
{
	if (!t.active) return false;
	return (GetTickCount() - t.startTime) >= t.durationMs;
}

// 0.0 (just started) to 1.0 (finished) - handy for HUD bars/heal-over-time math.
inline float buffTimerProgress(const BuffTimer &t)
{
	if (!t.active || t.durationMs == 0) return 1.0f;
	float p = (float)(GetTickCount() - t.startTime) / (float)t.durationMs;
	if (p < 0.0f) p = 0.0f;
	if (p > 1.0f) p = 1.0f;
	return p;
}

// Seconds left, for HUD text ("Swiftness 7s" etc). 0 if not active.
inline int buffTimerSecondsLeft(const BuffTimer &t)
{
	if (!t.active) return 0;
	unsigned long elapsed = GetTickCount() - t.startTime;
	if (elapsed >= t.durationMs) return 0;
	return (int)((t.durationMs - elapsed) / 1000UL) + 1;
}

// ---------------------------------------------------------------
//  Player  - the live, in-game stat block (as opposed to Character in
//  Player.hpp, which just holds each class's fixed starting numbers)
// ---------------------------------------------------------------
struct Player
{
	int characterIndex;   // 0 = Alchemist, 1 = Ranger, 2 = Guardian (matches CH[] order)

	// --- required base stats ---
	float speed;                     // current moveSpeed (base * any active speed buff)
	int   maxHealth;
	int   currentHealth;
	float attackDamageMultiplier;    // PERMANENT damage multiplier - raised by Damage Scrolls only

	// --- un-buffed baselines, so buffs always compute "base * multiplier"
	//     instead of compounding on top of an already-buffed number ---
	float baseSpeed;

	// --- temporary combat multipliers, separate from the permanent
	//     attackDamageMultiplier above so a Power Potion wearing off
	//     never accidentally erases a Damage Scroll's permanent bonus ---
	float powerPotionMultiplier;     // 1.0 normally, POWER_POTION_DAMAGE_MULT while active
	float defenseMultiplier;         // 1.0 normally, REINFORCEMENT_POTION_DEFENSE_MULT while active

	// --- timers, one per temporary buff ---
	BuffTimer speedBuff;
	BuffTimer powerBuff;
	BuffTimer reinforcementBuff;

	// --- Health Potion is a heal-over-time, so it needs its own timer
	//     PLUS bookkeeping for how much of the total heal has landed so
	//     far (see applyHealthPotionTick() in updatePlayerBuffs() below) ---
	BuffTimer healthRegenBuff;
	int       healthRegenTotalAmount;
	int       healthRegenAppliedSoFar;
};

// ---------------------------------------------------------------
//  SETUP - pulls starting numbers from CH[characterIndex]
// ---------------------------------------------------------------
inline void initPlayerStats(Player &player, int characterIndex)
{
	player.characterIndex = characterIndex;

	player.baseSpeed = CH[characterIndex].moveSpeed;
	player.speed = player.baseSpeed;

	player.maxHealth = (int)CH[characterIndex].maxHealth;
	player.currentHealth = (int)CH[characterIndex].health;

	player.attackDamageMultiplier = 1.0f;
	player.powerPotionMultiplier = 1.0f;
	player.defenseMultiplier = 1.0f;

	player.speedBuff.active = false;
	player.powerBuff.active = false;
	player.reinforcementBuff.active = false;
	player.healthRegenBuff.active = false;
	player.healthRegenTotalAmount = 0;
	player.healthRegenAppliedSoFar = 0;
}

// ---------------------------------------------------------------
//  READ-ONLY HELPERS  -  what combat/movement code should actually
//  call, so it never needs to know which buffs are active right now.
// ---------------------------------------------------------------
// Combined attack multiplier: permanent scroll bonus * temporary potion bonus.
inline float getEffectiveAttackMultiplier(const Player &player)
{
	return player.attackDamageMultiplier * player.powerPotionMultiplier;
}

// Applies the Reinforcement Potion's damage reduction to an incoming hit.
// Call this from wherever damage is currently subtracted from currentHealth,
// e.g.: player.currentHealth -= applyIncomingDamage(player, rawDamage);
inline int applyIncomingDamage(const Player &player, int rawDamage)
{
	if (player.defenseMultiplier <= 0.0f) return rawDamage;
	int reduced = (int)(rawDamage / player.defenseMultiplier);
	if (reduced < 0) reduced = 0;
	return reduced;
}

// ---------------------------------------------------------------
//  POTIONS / SCROLLS  -  one function per crafted item, matching
//  ITEM_HEALTH_POTION / ITEM_POWER_POTION / ITEM_MOVEMENT_POTION /
//  ITEM_REINFORCEMENT_POTION / ITEM_DAMAGE_SCROLL from inventory.hpp.
//  Call these right after craftingEngine's craftItem()/removeItem()
//  has already taken the potion OUT of the inventory (consuming it),
//  e.g.:  if (removeItem(ITEM_HEALTH_POTION, 1)) applyHealthPotion(player);
// ---------------------------------------------------------------

// Movement Potion (Swiftness Potion): +40%-60% move/sprint speed for
// 15-20s. Re-drinking one refreshes the timer instead of stacking, so
// speed never compounds past MOVEMENT_POTION_SPEED_MULT.
inline void applyMovementPotion(Player &player)
{
	unsigned long duration =
		(unsigned long)(MOVEMENT_POTION_DURATION_MS * CH[player.characterIndex].potionConsumeSpeed);

	startBuffTimer(player.speedBuff, duration);
	player.speed = player.baseSpeed * MOVEMENT_POTION_SPEED_MULT;
}

// Power Potion: +50%-100% physical/spell attack damage for 10-15s.
// Multiplicative on top of attackDamageMultiplier - see getEffectiveAttackMultiplier().
inline void applyPowerPotion(Player &player)
{
	unsigned long duration =
		(unsigned long)(POWER_POTION_DURATION_MS * CH[player.characterIndex].potionConsumeSpeed);

	startBuffTimer(player.powerBuff, duration);
	player.powerPotionMultiplier = POWER_POTION_DAMAGE_MULT;
}

// Reinforcement Potion: up to 2x armor/defense for 12s - halves incoming
// damage via applyIncomingDamage() while active.
inline void applyReinforcementPotion(Player &player)
{
	unsigned long duration =
		(unsigned long)(REINFORCEMENT_POTION_DURATION_MS * CH[player.characterIndex].potionConsumeSpeed);

	startBuffTimer(player.reinforcementBuff, duration);
	player.defenseMultiplier = REINFORCEMENT_POTION_DEFENSE_MULT;
}

// Health Potion: +30 HP, NOT instant - ticks in gradually over 12s (a
// heal-over-time). Drinking a second one while the first is still
// ticking tops up the remaining heal rather than resetting/stacking
// the full +30 again, so chain-chugging potions can't out-heal the item.
inline void applyHealthPotion(Player &player)
{
	unsigned long duration =
		(unsigned long)(HEALTH_POTION_DURATION_MS * CH[player.characterIndex].potionConsumeSpeed);

	int remainingFromBefore = player.healthRegenBuff.active
		? (player.healthRegenTotalAmount - player.healthRegenAppliedSoFar)
		: 0;

	startBuffTimer(player.healthRegenBuff, duration);
	player.healthRegenTotalAmount = remainingFromBefore + HEALTH_POTION_TOTAL_HEAL;
	player.healthRegenAppliedSoFar = 0;
}

// Damage Scroll: permanently increases attackDamageMultiplier by 20%.
// Stacks multiplicatively (2 scrolls = 1.44x, not 1.4x) since each one
// represents a real upgrade, not a temporary buff.
inline void applyDamageScroll(Player &player)
{
	player.attackDamageMultiplier *= (1.0f + DAMAGE_SCROLL_BONUS_PERCENT);
}

// ---------------------------------------------------------------
//  PER-TICK UPDATE - call every frame/tick so timed buffs expire and
//  the Health Potion's heal-over-time actually applies.
// ---------------------------------------------------------------
inline void updatePlayerBuffs(Player &player)
{
	// Movement Potion
	if (player.speedBuff.active && buffTimerExpired(player.speedBuff))
	{
		player.speedBuff.active = false;
		player.speed = player.baseSpeed;
	}

	// Power Potion
	if (player.powerBuff.active && buffTimerExpired(player.powerBuff))
	{
		player.powerBuff.active = false;
		player.powerPotionMultiplier = 1.0f;
	}

	// Reinforcement Potion
	if (player.reinforcementBuff.active && buffTimerExpired(player.reinforcementBuff))
	{
		player.reinforcementBuff.active = false;
		player.defenseMultiplier = 1.0f;
	}

	// Health Potion - heal-over-time: work out how much of the total
	// SHOULD have landed by now (linear over the duration) and apply
	// just the difference since last tick, so it's frame-rate independent.
	if (player.healthRegenBuff.active)
	{
		float progress = buffTimerProgress(player.healthRegenBuff);
		int targetHealed = (int)(player.healthRegenTotalAmount * progress);

		int delta = targetHealed - player.healthRegenAppliedSoFar;
		if (delta > 0)
		{
			player.currentHealth += delta;
			if (player.currentHealth > player.maxHealth)
				player.currentHealth = player.maxHealth;   // extra heal is simply wasted, not banked
			player.healthRegenAppliedSoFar += delta;
		}

		if (buffTimerExpired(player.healthRegenBuff))
			player.healthRegenBuff.active = false;
	}
}

// ---------------------------------------------------------------
//  CHARACTER-SPECIFIC PASSIVE BENEFITS
// ---------------------------------------------------------------
// Alchemist: crafting costs 25% fewer resources (CH[].resourceCostMultiplier).
// Use this wherever a recipe's ingredient counts are checked/spent, e.g.:
//   int actualCost = (int)ceil(baseCost * getCraftResourceMultiplier(player));
inline float getCraftResourceMultiplier(const Player &player)
{
	return CH[player.characterIndex].resourceCostMultiplier;
}

// Ranger: short-range dash that costs energy (CH[].dashDistance / energyCost).
// Returns false if the Ranger doesn't have enough energy to dash right now.
inline bool tryRangerDash(const Player &player, float &currentEnergy)
{
	if (player.characterIndex != 1) return false;              // Ranger only
	if (currentEnergy < CH[player.characterIndex].energyCost) return false;

	currentEnergy -= CH[player.characterIndex].energyCost;
	return true;                                                // caller moves the
	                                                             // player by dashDistance
}

// Guardian: temporary defensive shield (CH[].shieldDuration / shieldStrength).
// Returns the shield's strength (damage it can absorb) if the Guardian can
// raise a shield, or 0 if this isn't the Guardian.
inline float tryGuardianShield(const Player &player)
{
	if (player.characterIndex != 2) return 0.0f;                // Guardian only
	return CH[player.characterIndex].shieldStrength;            // lasts shieldDuration seconds
}

// ---------------------------------------------------------------
//  ACTIVE-BUFF HUD  (optional - purely visual, safe to ignore if you
//  already have your own buff HUD)
// ---------------------------------------------------------------
//  One wide badge per active timed buff, stacked top-to-bottom, using
//  the buff icon art that already exists under Images/. Call
//  loadBuffIconTextures() once after iInitialize(), then
//  drawActiveBuffIcons(player, x, y) every frame from your HUD draw code.
// ---------------------------------------------------------------
static unsigned int g_buffIconSpeedTex = 0;          // Images/Movement_Speed_icon.png
static unsigned int g_buffIconPowerTex = 0;          // Images/Power_Boost_icon.png
static unsigned int g_buffIconReinforcementTex = 0;  // Images/Reinforcement_icon.png
static unsigned int g_buffIconHealthRegenTex = 0;    // Images/Health_Regen_icon.png

inline void loadBuffIconTextures()
{
	g_buffIconSpeedTex = iLoadImage((char*)"Images/Movement_Speed_icon.png");
	g_buffIconPowerTex = iLoadImage((char*)"Images/Power_Boost_icon.png");
	g_buffIconReinforcementTex = iLoadImage((char*)"Images/Reinforcement_icon.png");
	g_buffIconHealthRegenTex = iLoadImage((char*)"Images/Health_Regen_icon.png");
}

#define BUFF_BADGE_W  150
#define BUFF_BADGE_H  28
#define BUFF_BADGE_GAP 6

// Draws a badge (icon + seconds-left) for each currently-active timed
// buff, stacked downward starting at (x, y). Returns how many were drawn.
inline int drawActiveBuffIcons(const Player &player, int x, int y)
{
	int drawn = 0;
	iSetColor(1, 1, 1);

	if (player.speedBuff.active)
	{
		iShowImage(x, y, BUFF_BADGE_W, BUFF_BADGE_H, g_buffIconSpeedTex);
		char buf[8]; sprintf(buf, "%ds", buffTimerSecondsLeft(player.speedBuff));
		iText(x + BUFF_BADGE_W - 26, y + 9, buf);
		y -= (BUFF_BADGE_H + BUFF_BADGE_GAP); drawn++;
	}
	if (player.powerBuff.active)
	{
		iShowImage(x, y, BUFF_BADGE_W, BUFF_BADGE_H, g_buffIconPowerTex);
		char buf[8]; sprintf(buf, "%ds", buffTimerSecondsLeft(player.powerBuff));
		iText(x + BUFF_BADGE_W - 26, y + 9, buf);
		y -= (BUFF_BADGE_H + BUFF_BADGE_GAP); drawn++;
	}
	if (player.reinforcementBuff.active)
	{
		iShowImage(x, y, BUFF_BADGE_W, BUFF_BADGE_H, g_buffIconReinforcementTex);
		char buf[8]; sprintf(buf, "%ds", buffTimerSecondsLeft(player.reinforcementBuff));
		iText(x + BUFF_BADGE_W - 26, y + 9, buf);
		y -= (BUFF_BADGE_H + BUFF_BADGE_GAP); drawn++;
	}
	if (player.healthRegenBuff.active)
	{
		iShowImage(x, y, BUFF_BADGE_W, BUFF_BADGE_H, g_buffIconHealthRegenTex);
		char buf[8]; sprintf(buf, "%ds", buffTimerSecondsLeft(player.healthRegenBuff));
		iText(x + BUFF_BADGE_W - 26, y + 9, buf);
		y -= (BUFF_BADGE_H + BUFF_BADGE_GAP); drawn++;
	}

	return drawn;
}

// =====================================================================
//  WIRING THIS INTO YOUR CONSUME-POTION CODE  (reference only - this
//  file doesn't call into inventory.hpp/CraftingEngine.hpp itself, so
//  it stays usable even if you haven't added a "use item" click yet)
// =====================================================================
//  Player player;
//  initPlayerStats(player, characterIndex);     // once, e.g. HomeBase_Init()
//  loadBuffIconTextures();                      // once, after iInitialize()
//
//  // wherever a potion slot gets clicked in the inventory:
//  if (removeItem(ITEM_HEALTH_POTION, 1))        applyHealthPotion(player);
//  if (removeItem(ITEM_POWER_POTION, 1))         applyPowerPotion(player);
//  if (removeItem(ITEM_MOVEMENT_POTION, 1))      applyMovementPotion(player);
//  if (removeItem(ITEM_REINFORCEMENT_POTION, 1)) applyReinforcementPotion(player);
//  if (removeItem(ITEM_DAMAGE_SCROLL, 1))        applyDamageScroll(player);
//
//  // every tick:
//  updatePlayerBuffs(player);
//
//  // every frame, in your HUD draw code:
//  drawActiveBuffIcons(player, hudX, hudY);
//
//  // wherever damage is dealt/taken:
//  int dealt = (int)(baseDamage * getEffectiveAttackMultiplier(player));
//  player.currentHealth -= applyIncomingDamage(player, incomingDamage);
// =====================================================================
