// =====================================================================
//  PotionSystem.hpp
// =====================================================================
//  Shared potion hotkeys for Home Base AND Battle:
//
//      1 = Health         (+30 HP, instant)
//      2 = Movement       (+50% speed, timed)
//      3 = Power          (+75% attack damage, timed)
//      4 = Reinforcement  (halves incoming damage, timed)
//
//  Pressing a key consumes ONE crafted potion from the inventory
//  (removeItem()), turns its effect on, and pops up a banner reading
//  "<Potion> Potion activated". If none are in stock the banner says so
//  instead and nothing is spent.
//
//  Slot numbers below stay in the ItemID enum order (Health, Power,
//  Movement, Reinforcement) so "ITEM_HEALTH_POTION + slot" keeps working;
//  POTION_HOTKEYS maps each slot to the key that triggers it.
//
//  Home Base calls Potion_HotkeysUpdate() from HomeBase_FixedUpdate() and
//  Potion_DrawHUD() from HomeBase_Draw(); Battle.hpp does the same in its
//  own update/draw. State is shared, so an active buff simply keeps
//  ticking down when you move between the two screens.
// =====================================================================
#pragma once

#include "iGraphics.h"
#include "HomeBaseConfig.hpp"
#include "Fighters.hpp"
#include "inventory.hpp"
#include <windows.h>   // GetTickCount()
#include <stdio.h>     // sprintf()
#include <string.h>    // strlen()

#define POTION_SLOT_HEALTH         0
#define POTION_SLOT_POWER          1
#define POTION_SLOT_MOVEMENT       2
#define POTION_SLOT_REINFORCEMENT  3
#define POTION_SLOT_COUNT          4

#define POTION_HEALTH_HEAL          30
#define POTION_POWER_DAMAGE_MULT    1.75f
#define POTION_MOVEMENT_SPEED_MULT  1.5f
#define POTION_DEFENSE_MULT         2.0f
#define POTION_BUFF_DURATION_MS     10000UL  // every potion's icon/effect runs for 10s
#define POTION_POPUP_DURATION_MS    2000UL   // "<Potion> activated" banner lifetime

// slot -> key that uses it (1 Health, 2 Movement, 3 Power, 4 Reinforcement)
static const int POTION_HOTKEYS[POTION_SLOT_COUNT] = { '1', '3', '2', '4' };

static bool          g_potionActive[POTION_SLOT_COUNT]     = { false, false, false, false };
static unsigned long g_potionExpiresAt[POTION_SLOT_COUNT]  = { 0, 0, 0, 0 };
static bool          g_potionKeyWasDown[POTION_SLOT_COUNT] = { false, false, false, false };

// Un-buffed stats, so Power/Movement always compute "base * multiplier".
static float g_potionBaseDamage = 0;
static float g_potionBaseSpeed  = 0;

// Banner state
static char          g_potionPopupText[64] = "";
static bool          g_potionPopupActive   = false;
static unsigned long g_potionPopupStart    = 0;

inline void Potion_ShowPopup(const char* text)
{
	strncpy(g_potionPopupText, text, sizeof(g_potionPopupText) - 1);
	g_potionPopupText[sizeof(g_potionPopupText) - 1] = '\0';
	g_potionPopupActive = true;
	g_potionPopupStart = GetTickCount();
}

// Puts damage/speed back to normal and clears every active potion.
// Call before a scene captures the fighter's stats (e.g. Battle_Init()).
inline void Potion_Reset(Fighter &player)
{
	if (g_potionActive[POTION_SLOT_POWER] && g_potionBaseDamage > 0)
		player.damage = (int)g_potionBaseDamage;
	if (g_potionActive[POTION_SLOT_MOVEMENT] && g_potionBaseSpeed > 0)
		player.moveSpeed = g_potionBaseSpeed;

	for (int i = 0; i < POTION_SLOT_COUNT; i++)
	{
		g_potionActive[i] = false;
		g_potionExpiresAt[i] = 0;
		g_potionKeyWasDown[i] = false;
	}
	g_potionPopupActive = false;
}

// Reinforcement Potion: damage reduction, use wherever the hero takes a hit.
inline int Potion_AdjustedDamage(int rawDamage)
{
	if (!g_potionActive[POTION_SLOT_REINFORCEMENT]) return rawDamage;
	int reduced = (int)(rawDamage / POTION_DEFENSE_MULT);
	return (reduced < 0) ? 0 : reduced;
}

// Consume one potion from `slot` and apply it. Returns true if used.
inline bool Potion_Use(int slot, Fighter &player)
{
	char msg[64];
	const char* name = ITEM_DB[ITEM_HEALTH_POTION + slot].name;

	if (getItemCount(ITEM_HEALTH_POTION + slot) <= 0)
	{
		sprintf(msg, "No %s Potion left", name);
		Potion_ShowPopup(msg);
		return false;
	}

	if (slot == POTION_SLOT_HEALTH && player.currentHealth >= player.maxHealth)
	{
		Potion_ShowPopup("Health is already full");
		return false;
	}

	if (!removeItem(ITEM_HEALTH_POTION + slot, 1)) return false;

	// capture the un-buffed stat before the buff flips on
	if (slot == POTION_SLOT_POWER && !g_potionActive[POTION_SLOT_POWER])       g_potionBaseDamage = (float)player.damage;
	if (slot == POTION_SLOT_MOVEMENT && !g_potionActive[POTION_SLOT_MOVEMENT]) g_potionBaseSpeed  = player.moveSpeed;

	g_potionActive[slot] = true;
	g_potionExpiresAt[slot] = GetTickCount() + POTION_BUFF_DURATION_MS;

	if (slot == POTION_SLOT_HEALTH)
	{
		player.currentHealth += POTION_HEALTH_HEAL;
		if (player.currentHealth > player.maxHealth) player.currentHealth = player.maxHealth;
	}

	sprintf(msg, "%s Potion activated", name);
	Potion_ShowPopup(msg);
	return true;
}

// Once per tick: read keys, expire buffs, keep damage/speed in sync.
inline void Potion_HotkeysUpdate(Fighter &player)
{
	unsigned long now = GetTickCount();

	// While no buff is on, the fighter's current numbers ARE the base.
	if (!g_potionActive[POTION_SLOT_POWER])    g_potionBaseDamage = (float)player.damage;
	if (!g_potionActive[POTION_SLOT_MOVEMENT]) g_potionBaseSpeed  = player.moveSpeed;

	for (int i = 0; i < POTION_SLOT_COUNT; i++)
	{
		bool down = (isKeyPressed(POTION_HOTKEYS[i]) != 0);
		if (down && !g_potionKeyWasDown[i])   // edge-triggered: one potion per press
		{
			Potion_Use(i, player);
		}
		g_potionKeyWasDown[i] = down;

		if (g_potionActive[i] && now >= g_potionExpiresAt[i])
			g_potionActive[i] = false;
	}

	player.damage    = (int)(g_potionBaseDamage * (g_potionActive[POTION_SLOT_POWER] ? POTION_POWER_DAMAGE_MULT : 1.0f));
	player.moveSpeed = g_potionBaseSpeed * (g_potionActive[POTION_SLOT_MOVEMENT] ? POTION_MOVEMENT_SPEED_MULT : 1.0f);
}

// Small row of active-potion icons under the Stone/Iron/Wood/Water column.
inline void Potion_DrawIcons()
{
	const int iconSize = 25;
	const int iconX = 730;
	const int iconY = HOME_AREA_H - 65 - 4 * 35 - 15;
	const int gap = iconSize + 8;

	iSetColor(255, 255, 255);
	int drawn = 0;
	for (int i = 0; i < POTION_SLOT_COUNT; i++)
	{
		if (!g_potionActive[i]) continue;
		iShowImage(iconX + drawn * gap, iconY, iconSize, iconSize, ITEM_DB[ITEM_HEALTH_POTION + i].iconTex);
		drawn++;
	}
}

// "<Potion> Potion activated" banner, top-centre, fades out at the end.
inline void Potion_DrawPopup()
{
	if (!g_potionPopupActive) return;
	unsigned long elapsed = GetTickCount() - g_potionPopupStart;
	if (elapsed >= POTION_POPUP_DURATION_MS) { g_potionPopupActive = false; return; }

	void* font = GLUT_BITMAP_HELVETICA_18;
	int textW = glutBitmapLength(font, (const unsigned char*)g_potionPopupText);
	int boxW = textW + 40, boxH = 44;
	int boxX = (HOME_AREA_W - boxW) / 2;
	int boxY = HOME_AREA_H - 110;

	iSetColor(20, 20, 30);
	iFilledRectangle(boxX, boxY, boxW, boxH);
	iSetColor(120, 230, 120);
	iRectangle(boxX, boxY, boxW, boxH);
	iRectangle(boxX + 1, boxY + 1, boxW - 2, boxH - 2);
	iSetColor(255, 255, 255);
	iText(boxX + 20, boxY + 15, g_potionPopupText, font);
}

// Call from a scene's draw function.
inline void Potion_DrawHUD()
{
	Potion_DrawIcons();
	Potion_DrawPopup();
}
