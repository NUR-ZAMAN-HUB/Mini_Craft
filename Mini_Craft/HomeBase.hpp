// =====================================================================
//  HomeBase.hpp
// =====================================================================
//  Home Base scene: pick one of 3 fighters (Guardian/Ranger/Alchemist),
//  walk around, attack, use your fighter's special ability, and gather
//  Stone/Wood/Iron/Water from resource nodes.
//
//  iMain.cpp already owns the single main()/iDraw()/iMouse()/
//  fixedUpdate() (iGraphics only allows one of each), so Home Base's
//  logic is exposed as four plain functions iMain.cpp calls into
//  whenever currentState == GameState::HOMEBASE:
//
//      HomeBase_Init()                  -> call once, after iInitialize()
//      HomeBase_FixedUpdate()           -> call every tick while in HOMEBASE
//      HomeBase_Draw()                  -> call every frame while in HOMEBASE
//      HomeBase_OnMouseDown(button,x,y) -> call from iMouse() while in HOMEBASE
// =====================================================================
#pragma once

#include "iGraphics.h"
#include "Menu.h"           // characterNumber / CharacterId - the hero picked on CHARACTER_SELECT
#include "HomeBaseConfig.hpp"
#include "Enemy.hpp"
#include "Fighters.hpp"
#include "GatherSystem.hpp"
#include "inventory.hpp"    // drawInventoryUI() - Home Base corner icon + ITEM_DB/INVENTORY
#include "CraftingEngine.hpp" // drawCraftingMenu()/craftingMenuOnClick() - the crafting book UI
#include "ArmorEngine.hpp"   // drawArmorMenu()/armorMenuOnClick() - the Armor icon's display-only panel
#include <stdio.h>          // fopen() - see fileExists() below

// True if the file can actually be opened - used to tell "art not added yet"
// apart from "art loaded fine" (see the comment on attackBtn.imgId in HomeBase_Init()).
inline bool fileExists(const char* path)
{
	FILE* f = fopen(path, "rb");
	if (f) { fclose(f); return true; }
	return false;
}

// ---------------------------------------------------------------
//  STATE
// ---------------------------------------------------------------
Fighter roster[3];                       // one Fighter per type, indexed by CHAR_GUARDIAN/CHAR_RANGER/CHAR_ALCHEMIST
unsigned int homeBaseTexture = 0;
unsigned int homeBaseNightTexture = 0;
unsigned int homeBaseNightBlurTexture = 0;
bool g_nightAlertActive = false;   // true while "THE NIGHT HAS COME" alert box is displayed
bool g_nightAlertShown = false;    // ensures the alert appears only once per night cycle
bool g_spaceWasDownForAlert = false;

// ---------------------------------------------------------------
//  NIGHT ZOMBIE DEFENSE EVENT
//  2 Waves: Wave 1 has 3 zombies, Wave 2 has 6 zombies.
// ---------------------------------------------------------------
#define HOME_MAX_ZOMBIES 6
Enemy g_homeZombies[HOME_MAX_ZOMBIES];
int   g_homeZombieCount = 0;
bool  g_zombieDefenseActive = false;
int   g_zombieWave = 0;              // 1 = Wave 1 (3 zombies), 2 = Wave 2 (6 zombies), 3 = finished
bool  g_baseSafeMessageActive = false;
unsigned long g_baseSafeMessageStartTime = 0;
bool  g_baseDefeatedMessageActive = false;
unsigned long g_baseDefeatedStartTime = 0;

// Set true once the first night (win or lose) resolves. While true, the
// countdown/night-alert checks are skipped entirely - the timer does not
// restart after the first win/loss, for now.
bool g_nightCycleStopped = false;

// Shared zombie textures (4 walk frames L/R, 3 attack frames L/R)
unsigned int zombieWalkTex[2][4];
unsigned int zombieAttackTex[2][3];
bool zombieSpritesLoaded = false;
int activeFighterIndex = CHAR_GUARDIAN;  // which roster[] slot the player currently controls - locked to the
                                          // character chosen on CHARACTER_SELECT, see HomeBase_Init()

// characterNumber (Menu.h: CHARACTER_ALCHEMIST=0, CHARACTER_RANGER=1, CHARACTER_GUARDIAN=2) uses a
// different order than FighterType (Fighters.hpp: CHAR_GUARDIAN=0, CHAR_RANGER=1, CHAR_ALCHEMIST=2),
// so this table maps one to the other, indexed by characterNumber.
static const FighterType kFighterForCharacterId[3] = { CHAR_ALCHEMIST, CHAR_RANGER, CHAR_GUARDIAN };

ResourceNode homeResourceNodes[4];

// 50x50 resource-count icons, shown top-right during Home Base (see drawHomeResourceHUD()).
unsigned int stoneIconID = 0;
unsigned int ironIconID = 0;
unsigned int woodIconID = 0;
unsigned int waterIconID = 0;
unsigned int coinIconID = 0;
unsigned int armorIconID = 0;
// Coins earned by defeating enemies (Zombie = +3, Skeleton = +2 - see
// awardCoinsForKill() below). Plain in-memory session total, intentionally
// NEVER written to a save file, so it goes back to 0 on its own the next
// time the game is launched (e.g. after clicking "Exit" on the main menu,
// which calls exit(0) in iMain.cpp).
int g_playerCoins = 0;
#include "SaveGame.hpp"   // SaveGame()/LoadGame()/ResetSaveGame() - needs g_playerCoins/activeFighterIndex above

// Call this right after applyDamageToEnemy() at an attack call site, but
// only for the tick where that call just killed the enemy (i.e. it was
// alive before the call and e.isDead is true after). Zombies are worth
// 3 coins, Skeletons 2; other types (e.g. the Monster boss) award nothing.
inline void awardCoinsForKill(EnemyType type)
{
	if (type == ENEMY_ZOMBIE)      g_playerCoins += 3;
	else if (type == ENEMY_SKELETON) g_playerCoins += 2;
}
// Bottom-left corner "Attack" button - clicking it sends the player into a
// BATTLE against an enemy (see Battle.hpp). imgId is set in HomeBase_Init().
Button attackBtn = { 15, 15, 15 + 85, 15 + 65, "", 0 };

// Set once each grunt type has been beaten in a full (non-boss) battle - see
// Battle.hpp's Battle_FixedUpdate(), where these flip to true on a BATTLE_WON
// against a Skeleton/Zombie round. Zombie is no longer part of Arena_1's grunt
// rotation (regular Attack-button battles never spawn one - see Battle.hpp's
// Battle_Init()), so g_hasWonZombieRound is kept only so old save files still
// load; the Boss button below now unlocks off g_hasWonSkeletonRound alone.
bool g_hasWonSkeletonRound = false;
bool g_hasWonZombieRound = false;

// Set right before EnterLoading(GameState::BATTLE) when the Boss button is
// clicked - tells Battle_Init() to force a boss fight instead of rolling one
// off currentLevel. Consumed (reset to false) the moment Battle_Init() reads it.
bool g_forceBossFight = false;

// Boss button - next to Attack, only drawn/clickable once both round flags
// above are true (see HomeBase_Draw()/HomeBase_OnMouseDown()). Same height as
// attackBtn, sitting just to its right.
Button bossBtn = { attackBtn.x2 + 10, attackBtn.y1, attackBtn.x2 + 10 + 85, attackBtn.y2, "", 0 };

// Set for one tick when the player clicks (iMouse() runs separately
// from fixedUpdate()), then read + cleared inside HomeBase_FixedUpdate().
bool g_homeLeftClickPending = false;
bool g_homeRightClickPending = false;
// Set by HomeBase_OnMouseDown() when the player left-clicks while the night alert / base-safe /
// base-defeated box is showing; read + cleared inside HomeBase_FixedUpdate() to dismiss it.
bool g_alertClickPending = false;

// Controls panel is hidden by default and toggles on/off each time ESC
// is pressed. g_escKeyWasDown remembers last frame's ESC state so it
// only flips showControls on the frame ESC goes down, not on every
// fixedUpdate() tick while it's held.
bool showControls = false;
bool g_escKeyWasDown = false;

// ---------------------------------------------------------------
//  NIGHT TIMER - counts down from NIGHT_TIMER_DURATION_MS.
//  Tracked as an explicit "remaining ms" value that only advances
//  from inside updateNightTimer(), called once per HomeBase_FixedUpdate()
//  tick (see below). Because that function only runs while
//  currentState == HOMEBASE, the countdown naturally PAUSES the instant
//  the player enters the Battle arena (fixedUpdate() stops calling it)
//  and RESUMES exactly where it left off when HomeBase_ResumeNightTimer()
//  re-stamps g_nightTimerLastTick on the way back - see EnterLoading()'s
//  LOADING->HOMEBASE handoff in iMain.cpp, which chooses Resume vs Start.
// ---------------------------------------------------------------
unsigned long g_nightTimerRemainingMs = NIGHT_TIMER_DURATION_MS;
unsigned long g_nightTimerLastTick = 0;   // 0 = "not currently ticking" (paused or not started)
bool g_isNightTime = false;   // flips true once the countdown reaches 0:00

// Call this once, right when Home Base is entered FRESH (new game, portal
// arrival, loaded save) to reset the countdown to the full duration and
// clear out any leftover night-event state from a previous cycle.
inline void HomeBase_StartNightTimer()
{
	g_nightTimerRemainingMs = NIGHT_TIMER_DURATION_MS;
	g_nightTimerLastTick = GetTickCount();
	g_isNightTime = false;
	g_nightAlertActive = false;
	g_nightAlertShown = false;
	g_spaceWasDownForAlert = false;
	g_zombieDefenseActive = false;
	g_zombieWave = 0;
	g_homeZombieCount = 0;
	g_baseSafeMessageActive = false;
	g_baseDefeatedMessageActive = false;
}

// Call this when RETURNING to Home Base from the Battle arena - resumes the
// countdown from whatever it was paused at, without touching remaining time
// or any night-event state (g_isNightTime, wave progress, etc. all carry over).
inline void HomeBase_ResumeNightTimer()
{
	g_nightTimerLastTick = GetTickCount();
}

// Advances the countdown by however much time has passed since the last
// tick. Call exactly once per HomeBase_FixedUpdate() - never from Battle.
inline void updateNightTimer()
{
	if (g_nightCycleStopped) return;   // first night's already resolved - leave remaining as-is

	unsigned long now = GetTickCount();
	unsigned long delta = now - g_nightTimerLastTick;
	g_nightTimerLastTick = now;

	if (delta >= g_nightTimerRemainingMs)
		g_nightTimerRemainingMs = 0;
	else
		g_nightTimerRemainingMs -= delta;
}

// Milliseconds left until night, clamped to 0 (never negative).
inline unsigned long getNightTimeRemainingMs()
{
	return g_nightTimerRemainingMs;
}

inline bool IsNightTime() { return g_isNightTime; }

// ---------------------------------------------------------------
//  UI HELPER: rounded black HUD panel (iGraphics only draws straight
//  edges, so a rounded rectangle = a quarter-circle stitched onto each corner)
// ---------------------------------------------------------------
inline int buildRoundedRectPoints(double x, double y, double w, double h, double radius, double outX[], double outY[])
{
	const int segs = 8;
	const double PI = 3.14159265358979323846;
	int n = 0;

	struct { double cx, cy, startAngle; } corners[4] = {
		{ x + w - radius, y + radius,       -PI / 2.0 },
		{ x + w - radius, y + h - radius,    0.0 },
		{ x + radius,     y + h - radius,    PI / 2.0 },
		{ x + radius,     y + radius,        PI },
	};

	for (int c = 0; c < 4; c++)
		for (int s = 0; s <= segs; s++)
		{
			double t = corners[c].startAngle + (PI / 2.0) * (s / (double)segs);
			outX[n] = corners[c].cx + radius * cos(t);
			outY[n] = corners[c].cy + radius * sin(t);
			n++;
		}
	return n;
}

inline void drawSolidBlackPanel(double x, double y, double w, double h, double radius)
{
	double px[64], py[64];
	int n = buildRoundedRectPoints(x, y, w, h, radius, px, py);

	iSetColor(0, 0, 0);
	glBegin(GL_POLYGON);
	for (int i = 0; i < n; i++) glVertex2d(px[i], py[i]);
	glEnd();
}

// Only drawn while showControls is true (toggled by ESC).
inline void drawHomeControlsBox()
{
	const double boxX = 15, boxY = 90, boxW = 250, boxH = 165;   // moved up from y=15 so it clears the Attack button
	drawSolidBlackPanel(boxX, boxY, boxW, boxH, 12.0);

	const char* lines[] = {
		"CONTROLS", "Move    : WASD / Arrows", "Attack  : Space / R-Click", "Shield  : E",
		"Dash    : Shift + D", "Gather  : Left-Click", "Menu    : M"
	};

	double textX = boxX + 15, lineY = boxY + boxH - 24;
	iSetColor(255, 255, 255);
	for (int i = 0; i < 7; i++)
	{
		iText(textX, lineY, (char*)lines[i]);
		lineY -= (i == 0) ? 22.0 : 18.0;   // extra gap right after the "CONTROLS" title
	}
}

// Top-left health bar for the active fighter: an outline rectangle
// that fills left-to-right in proportion to currentHealth/maxHealth,
// colored green/yellow/red by how low that fraction is, plus the
// current health number next to it.
inline void drawHomeHealthBar()
{
	const double barW = 260, barH = 22;
	const double barX = 20;               // anchored to the left edge
	const double barY = HOME_AREA_H - 40; // near the top of the screen

	Fighter &active = roster[activeFighterIndex];

	double pct = (active.maxHealth > 0) ? (double)active.currentHealth / active.maxHealth : 0.0;
	if (pct < 0.0) pct = 0.0;
	if (pct > 1.0) pct = 1.0;

	// filled portion first, so the outline below sits cleanly on top of it
	if (pct > 0.0)
	{
		if (pct > 0.5)       iSetColor(60, 200, 70);   // healthy: green
		else if (pct > 0.25) iSetColor(230, 200, 40);  // medium: yellow
		else                 iSetColor(220, 60, 60);   // low: red

		double fillW = barW * pct;
		glBegin(GL_POLYGON);
			glVertex2d(barX, barY);
			glVertex2d(barX + fillW, barY);
			glVertex2d(barX + fillW, barY + barH);
			glVertex2d(barX, barY + barH);
		glEnd();
	}

	// outline, always full barW - this is what makes it read as a container
	iSetColor(255, 255, 255);
	glBegin(GL_LINE_LOOP);
		glVertex2d(barX, barY);
		glVertex2d(barX + barW, barY);
		glVertex2d(barX + barW, barY + barH);
		glVertex2d(barX, barY + barH);
	glEnd();

	// Just the number now (no "HP:" label, no "/maxHealth").
	char buf[16];
	sprintf(buf, "%d", active.currentHealth);
	iText(barX + barW + 12, barY + 6, buf);
}

// Top-right resource HUD: a 25x25 icon plus the current count for each of
// Stone / Iron / Wood / Water, stacked vertically at x=730 down the right edge of the screen.
// Counts come straight from GatherSystem.hpp's g_inventory (via the getter
// functions), so this is purely a display of state that already exists.
inline void drawHomeResourceHUD()
{
	const double iconSize = 25;
	const double iconX = 730;                 // left edge of the icon column
	const double textX = iconX + iconSize + 10;
	double iconY = HOME_AREA_H - 65;          // top slot, near the top of the screen
	const double rowGap = 35;

	struct { unsigned int tex; int count; } rows[4] = {
		{ stoneIconID, getStoneCount() },
		{ ironIconID, getIronCount() },
		{ woodIconID, getWoodCount() },
		{ waterIconID, getWaterCount() },
	};

	iSetColor(255, 255, 255);
	for (int i = 0; i < 4; i++)
	{
		iShowImage((int)iconX, (int)iconY, (int)iconSize, (int)iconSize, rows[i].tex);

		char buf[16];
		sprintf(buf, "%d", rows[i].count);
		iText(textX, iconY + iconSize / 2 - 5, buf);

		iconY -= rowGap;
	}
}

// Coin HUD: a small icon + the current coin count, top-right corner - sits
// just above the Stone/Iron/Wood/Water column (drawHomeResourceHUD() starts
// at y = HOME_AREA_H - 65, this sits above that at HOME_AREA_H - 30) so the
// two never overlap. Shared by Home Base and Battle (see Battle_Draw()).
inline void drawCoinHUD()
{
	const double iconSize = 25;
	const double iconX = 730;
	const double iconY = HOME_AREA_H - 30;
	const double textX = iconX + iconSize + 10;

	iSetColor(255, 255, 255);
	iShowImage((int)iconX, (int)iconY, (int)iconSize, (int)iconSize, coinIconID);

	char buf[16];
	sprintf(buf, "%d", g_playerCoins);
	iText(textX, iconY + iconSize / 2 - 5, buf);
}

inline int getLivingHomeZombieCount()
{
	int count = 0;
	for (int i = 0; i < g_homeZombieCount; i++)
	{
		if (!g_homeZombies[i].isDead) count++;
	}
	return count;
}

// Right-edge "Night time in: MM:SS" readout inside a color box for clear visibility.
// Sits above the resource HUD (which occupies x=730 down from y=HOME_AREA_H-65) so the two never overlap.
inline void drawNightTimer()
{
	unsigned long remainingMs = getNightTimeRemainingMs();
	int minutes = (int)(remainingMs / 60000);
	int seconds = (int)((remainingMs / 1000) % 60);

	char buf[32];
	if (g_zombieDefenseActive)
		sprintf(buf, "Wave %d: %d Left", g_zombieWave, getLivingHomeZombieCount());
	else
		sprintf(buf, "Night time in: %d:%02d", minutes, seconds);

	const double boxX = 525;
	const double boxY = HOME_AREA_H - 44;
	const double boxW = 195;
	const double boxH = 34;

	if (remainingMs == 0 || g_isNightTime || g_zombieDefenseActive)
	{
		// Timer stopped at 0 / Zombie invasion: Red alert color box
		iSetColor(80, 20, 25);
		iFilledRectangle(boxX, boxY, boxW, boxH);
		iSetColor(255, 60, 60);
		iRectangle(boxX, boxY, boxW, boxH);
		iRectangle(boxX + 1, boxY + 1, boxW - 2, boxH - 2);
	}
	else if (remainingMs <= 30000)
	{
		// Last 30 seconds: Warning amber/orange color box
		iSetColor(70, 35, 15);
		iFilledRectangle(boxX, boxY, boxW, boxH);
		iSetColor(255, 150, 30);
		iRectangle(boxX, boxY, boxW, boxH);
		iRectangle(boxX + 1, boxY + 1, boxW - 2, boxH - 2);
	}
	else
	{
		// Daytime countdown: Deep navy color box with bright gold border
		iSetColor(25, 45, 80);
		iFilledRectangle(boxX, boxY, boxW, boxH);
		iSetColor(255, 215, 0);
		iRectangle(boxX, boxY, boxW, boxH);
		iRectangle(boxX + 1, boxY + 1, boxW - 2, boxH - 2);
	}

	const double textX = boxX + 22;
	const double textY = boxY + 10;

	iSetColor(255, 255, 255);
	iText(textX, textY, buf, GLUT_BITMAP_HELVETICA_18);
}

// ---------------------------------------------------------------
//  NIGHT ALERT OVERLAY: Shown once when the countdown timer hits 0.
//  Uses homeBaseBackground_nightblur with a prominent message box:
//  "THE NIGHT HAS COME DEFEAT THE ZOMBIES TO SAVE THE BASE"
//  and a keyboard interaction prompt: "Press [Space] to continue".
// ---------------------------------------------------------------
inline void drawNightAlertBox()
{
	const double boxW = 660;
	const double boxH = 140;
	const double boxX = (HOME_AREA_W - boxW) / 2.0;   // 70
	const double boxY = (HOME_AREA_H - boxH) / 2.0;   // 230

	// Dark atmospheric background fill
	iSetColor(16, 14, 24);
	iFilledRectangle(boxX, boxY, boxW, boxH);

	// Multi-layer ominous red alert border
	iSetColor(220, 45, 45);
	iRectangle(boxX, boxY, boxW, boxH);
	iRectangle(boxX + 1, boxY + 1, boxW - 2, boxH - 2);
	iSetColor(160, 30, 30);
	iRectangle(boxX + 2, boxY + 2, boxW - 4, boxH - 4);

	// Main warning message
	iSetColor(255, 255, 255);
	const double msgX = boxX + 50;
	const double msgY = boxY + 80;
	iText(msgX, msgY, const_cast<char*>("THE NIGHT HAS COME DEFEAT THE ZOMBIES TO SAVE THE BASE"), GLUT_BITMAP_HELVETICA_18);

	// Keyboard interaction message with small text
	iSetColor(240, 210, 100);
	const char* hintText = "Left click anywhere to continue";
	const double hintX = boxX + (boxW - glutBitmapLength(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)hintText)) / 2.0;
	const double hintY = boxY + 30;
	iText(hintX, hintY, const_cast<char*>(hintText), GLUT_BITMAP_HELVETICA_12);
}

// ---------------------------------------------------------------
//  VICTORY MESSAGE BOX: "THE BASE IS SAFE"
// ---------------------------------------------------------------
inline void drawBaseSafeBox()
{
	const double boxW = 560;   // widened to fit "YOU WON! THE BASE IS SAVED"
	const double boxH = 130;
	const double boxX = (HOME_AREA_W - boxW) / 2.0;
	const double boxY = (HOME_AREA_H - boxH) / 2.0;

	// Dark emerald background fill
	iSetColor(12, 24, 18);
	iFilledRectangle(boxX, boxY, boxW, boxH);

	// Triumphant green and gold borders
	iSetColor(40, 190, 70);
	iRectangle(boxX, boxY, boxW, boxH);
	iRectangle(boxX + 1, boxY + 1, boxW - 2, boxH - 2);
	iSetColor(255, 215, 0);
	iRectangle(boxX + 2, boxY + 2, boxW - 4, boxH - 4);

	// Victory message: "YOU WON! THE BASE IS SAVED"
	iSetColor(255, 255, 255);
	const double msgX = boxX + 95;
	const double msgY = boxY + 75;
	iText(msgX, msgY, const_cast<char*>("YOU WON! THE BASE IS SAVED"), GLUT_BITMAP_HELVETICA_18);

	// Interaction prompt: "Press [Space] to continue"
	iSetColor(240, 220, 120);
	const char* hintText = "Left click anywhere to continue";
	const double hintX = boxX + (boxW - glutBitmapLength(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)hintText)) / 2.0;
	const double hintY = boxY + 28;
	iText(hintX, hintY, const_cast<char*>(hintText), GLUT_BITMAP_HELVETICA_12);
}

// ---------------------------------------------------------------
//  DEFEAT MESSAGE BOX: "DEFEATED! THE ZOMBIES DESTROYED THE BASE"
// ---------------------------------------------------------------
inline void drawBaseDefeatedBox()
{
	const double boxW = 580;
	const double boxH = 130;
	const double boxX = (HOME_AREA_W - boxW) / 2.0;   // 110
	const double boxY = (HOME_AREA_H - boxH) / 2.0;   // 235

	// Dark ominous blood-red background fill
	iSetColor(26, 8, 12);
	iFilledRectangle(boxX, boxY, boxW, boxH);

	// Menacing multi-layer red borders
	iSetColor(220, 30, 30);
	iRectangle(boxX, boxY, boxW, boxH);
	iRectangle(boxX + 1, boxY + 1, boxW - 2, boxH - 2);
	iSetColor(150, 20, 20);
	iRectangle(boxX + 2, boxY + 2, boxW - 4, boxH - 4);

	// Defeat message: "DEFEAT! THE ZOMBIES DESTROYED THE BASE"
	iSetColor(255, 60, 60);
	const double msgX = boxX + 65;
	const double msgY = boxY + 75;
	iText(msgX, msgY, const_cast<char*>("DEFEAT! THE ZOMBIES DESTROYED THE BASE"), GLUT_BITMAP_HELVETICA_18);

	// Interaction prompt: "Press [Space] to continue"
	iSetColor(220, 200, 100);
	const char* hintText = "Left click anywhere to continue";
	const double hintX = boxX + (boxW - glutBitmapLength(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)hintText)) / 2.0;
	const double hintY = boxY + 28;
	iText(hintX, hintY, const_cast<char*>(hintText), GLUT_BITMAP_HELVETICA_12);
}

// ---------------------------------------------------------------
//  ZOMBIE SPRITE LOADING & INSTANTIATION
// ---------------------------------------------------------------
inline void loadZombieSpritesOnce()
{
	if (zombieSpritesLoaded) return;
	zombieSpritesLoaded = true;

	char path[160];
	for (int frame = 1; frame <= 4; frame++)
	{
		sprintf(path, "Images/zombie_walkL%d.png", frame);
		zombieWalkTex[ENEMY_FACE_LEFT][frame - 1] = iLoadImage(path);

		sprintf(path, "Images/zombie_walkR%d.png", frame);
		zombieWalkTex[ENEMY_FACE_RIGHT][frame - 1] = iLoadImage(path);
	}
	for (int frame = 1; frame <= 3; frame++)
	{
		sprintf(path, "Images/zombie_attackL%d.png", frame);
		zombieAttackTex[ENEMY_FACE_LEFT][frame - 1] = iLoadImage(path);

		sprintf(path, "Images/zombie_attackR%d.png", frame);
		zombieAttackTex[ENEMY_FACE_RIGHT][frame - 1] = iLoadImage(path);
	}
}

inline void applyZombieSprites(Enemy &e)
{
	e.walkFrameCount = 4;
	e.attackFrameCount = 3;
	for (int f = 0; f < 4; f++)
	{
		e.walkTex[ENEMY_FACE_LEFT][f]  = zombieWalkTex[ENEMY_FACE_LEFT][f];
		e.walkTex[ENEMY_FACE_RIGHT][f] = zombieWalkTex[ENEMY_FACE_RIGHT][f];
	}
	for (int f = 0; f < 3; f++)
	{
		e.attackTex[ENEMY_FACE_LEFT][f]  = zombieAttackTex[ENEMY_FACE_LEFT][f];
		e.attackTex[ENEMY_FACE_RIGHT][f] = zombieAttackTex[ENEMY_FACE_RIGHT][f];
	}
	e.idleTex[ENEMY_FACE_LEFT]  = zombieWalkTex[ENEMY_FACE_LEFT][0];
	e.idleTex[ENEMY_FACE_RIGHT] = zombieWalkTex[ENEMY_FACE_RIGHT][0];
}

inline void spawnHomeZombieWave(int count)
{
	if (count > HOME_MAX_ZOMBIES) count = HOME_MAX_ZOMBIES;
	g_homeZombieCount = count;

	Fighter &hero = roster[activeFighterIndex];

	for (int i = 0; i < count; i++)
	{
		float sx = 0, sy = 0;
		for (int attempt = 0; attempt < 10; attempt++)
		{
			int side = rand() % 4;
			if (side == 0)      { sx = (float)(60 + rand() % 680); sy = 550.0f; } // top
			else if (side == 1) { sx = (float)(60 + rand() % 680); sy = 60.0f;  } // bottom
			else if (side == 2) { sx = 40.0f;  sy = (float)(80 + rand() % 440); } // left
			else                { sx = 760.0f; sy = (float)(80 + rand() % 440); } // right

			if (enemyDistanceTo(sx, sy, hero.x, hero.y) >= 120.0f)
				break;
		}

		initEnemy(g_homeZombies[i], ENEMY_ZOMBIE, sx, sy);
		applyZombieSprites(g_homeZombies[i]);
		g_homeZombies[i].maxHealth = g_homeZombies[i].currentHealth = ZOMBIE_MAX_HEALTH;
		g_homeZombies[i].moveSpeed = 1.6f;
		g_homeZombies[i].damage = 6;
	}
}

inline void homePlayerAttackZombies(Fighter &attacker, Enemy enemies[], int count, bool triggerRequested)
{
	if (!triggerRequested) return;

	unsigned long now = GetTickCount();
	if (now - attacker.lastAttackTriggerTime < ATTACK_COOLDOWN_MS) return;

	attacker.lastAttackTriggerTime = now;
	attacker.isAttacking = true;
	attacker.animState = ANIM_ATTACK;
	attacker.frameIndex = 0;
	attacker.attackAnimStartTime = now;

	int nearest = -1;
	float nearestDist = 0;
	for (int i = 0; i < count; i++)
	{
		if (enemies[i].isDead) continue;
		float d = distanceBetween(attacker.x, attacker.y, enemies[i].x, enemies[i].y);
		if (nearest == -1 || d < nearestDist)
		{
			nearest = i;
			nearestDist = d;
		}
	}
	if (nearest == -1) return;

	attacker.facing = (enemies[nearest].x >= attacker.x) ? FACE_RIGHT : FACE_LEFT;

	for (int i = 0; i < count; i++)
	{
		if (enemies[i].isDead) continue;
		float d = distanceBetween(attacker.x, attacker.y, enemies[i].x, enemies[i].y);
		if (d <= attacker.meleeRange)
		{
			applyDamageToEnemy(enemies[i], attacker.damage);
			if (enemies[i].isDead) awardCoinsForKill(enemies[i].type);   // just died this tick -> pay out
		}
	}
}

// ---------------------------------------------------------------
//  SCENE LIFECYCLE
// ---------------------------------------------------------------
inline void HomeBase_Init()
{
	homeBaseTexture = iLoadImage("Images/homeBaseBackground.png");
	homeBaseNightTexture = fileExists("Images/homeBaseBackground_night.png") ? iLoadImage("Images/homeBaseBackground_night.png") : 0;
	homeBaseNightBlurTexture = fileExists("Images/homeBaseBackground_nightblur.png") ? iLoadImage("Images/homeBaseBackground_nightblur.png") : 0;
	loadZombieSpritesOnce();
	if (g_nightTimerLastTick == 0) HomeBase_StartNightTimer();

	// All 3 fighters start where the portal drops the player.
	for (int i = 0; i < 3; i++)
		initFighter(roster[i], (FighterType)i, 380.0f, 160.0f);

	// Lock Home Base to whichever hero was picked on CHARACTER_SELECT - no more
	// switching fighters with 1/2/3. Falls back to Guardian if somehow nothing
	// was confirmed (characterNumber == -1) so activeFighterIndex is never invalid.
	if (characterNumber >= 0 && characterNumber < 3)
		activeFighterIndex = kFighterForCharacterId[characterNumber];
	else
		activeFighterIndex = CHAR_GUARDIAN;

	initAllResourceNodes(homeResourceNodes);

	stoneIconID = iLoadImage("Images/Stone.jpg");
	ironIconID = iLoadImage("Images/Iron.jpg");
	woodIconID = iLoadImage("Images/Wood.jpg");
	waterIconID = iLoadImage("Images/water.png");
	coinIconID = iLoadImage("Images/coin_icon.png");
	armorIconID = iLoadImage("Images/Armor_icon.png");

	// fileExists() check first: iLoadImage() below always returns a valid (non-zero)
	// OpenGL texture handle even when the file is missing - it just uploads
	// uninitialized/garbage image data, which usually ends up fully transparent
	// (invisible) rather than erroring. So imgId being non-zero does NOT mean the
	// art loaded - checking the file directly is the only reliable signal, and is
	// what lets HomeBase_Draw() below fall back to a plain rectangle button when
	// Images/Attack_button.png hasn't been added yet.
	attackBtn.imgId = fileExists("Images/Attack_button.png") ? iLoadImage("Images/Attack_button.png") : 0;
	bossBtn.imgId = fileExists("Images/Boss_button.png") ? iLoadImage("Images/Boss_button.png") : 0;

	initInventory();          // zero out item quantities
	loadInventoryTextures();  // item icons + the corner icon / panel art
	loadCraftingTextures();   // craftingbook.png + task_box.png for the crafting UI
}

inline void HomeBase_Draw()
{
	// Check if the night countdown just finished (skipped once the cycle has
	// been stopped after the first win/loss - see g_nightCycleStopped)
	if (!g_nightCycleStopped && !g_isNightTime && getNightTimeRemainingMs() == 0)
	{
		g_isNightTime = true;
		if (!g_nightAlertShown)
		{
			g_nightAlertActive = true;
			g_nightAlertShown = true;
		}
	}

	// 1. When the timer stops to 0, homeBaseBackground_nightblur comes first with the message box
	if (g_nightAlertActive)
	{
		if (homeBaseNightBlurTexture != 0)
			iShowImage(0, 0, HOME_AREA_W, HOME_AREA_H, homeBaseNightBlurTexture);
		else if (homeBaseNightTexture != 0)
			iShowImage(0, 0, HOME_AREA_W, HOME_AREA_H, homeBaseNightTexture);
		else
			iShowImage(0, 0, HOME_AREA_W, HOME_AREA_H, homeBaseTexture);

		drawNightAlertBox();
		return;
	}

	// 2. Once the player presses [Space], the alert dismisses and homeBaseBackground_night comes.
	// Gated on !g_nightCycleStopped too - once win/loss has resolved, the 2-minute
	// timer is stale (elapsed will read past its duration forever), so without this
	// guard getNightTimeRemainingMs() == 0 would keep this permanently true and the
	// background would never go back to day.
	
	
	if (!g_nightCycleStopped && (g_isNightTime || getNightTimeRemainingMs() == 0) && homeBaseNightTexture != 0)
		iShowImage(0, 0, HOME_AREA_W, HOME_AREA_H, homeBaseNightTexture);
	else
		iShowImage(0, 0, HOME_AREA_W, HOME_AREA_H, homeBaseTexture);

	for (int i = 0; i < 4; i++)
		drawResourceNode(homeResourceNodes[i]);

	// Draw active zombies during night defense
	if (g_zombieDefenseActive)
	{
		for (int i = 0; i < g_homeZombieCount; i++)
		{
			if (!g_homeZombies[i].isDead)
			{
				drawEnemy(g_homeZombies[i]);
				drawEnemyHealthBar(g_homeZombies[i], (int)(g_homeZombies[i].x - 18), (int)(g_homeZombies[i].y + 35), 36, 4);
			}
		}
	}

	drawFighter(roster[activeFighterIndex]);

	drawHomeHealthBar();
	drawHomeResourceHUD();
	drawCoinHUD();
	if (!g_nightCycleStopped) drawNightTimer();

	if (attackBtn.imgId != 0)
	{
		DrawButton(attackBtn);
	}
	else
	{
		// Images/Attack_button.png hasn't been added yet - draw a plain
		// rectangle button instead so Attack is still visible and clickable.
		// Swap to the real art automatically once that file exists (see
		// fileExists() in HomeBase_Init()) - no other code changes needed.
		iSetColor(150, 30, 30);
		iFilledRectangle(attackBtn.x1, attackBtn.y1, attackBtn.x2 - attackBtn.x1, attackBtn.y2 - attackBtn.y1);
		iSetColor(255, 255, 255);
		iRectangle(attackBtn.x1, attackBtn.y1, attackBtn.x2 - attackBtn.x1, attackBtn.y2 - attackBtn.y1);
		iText(attackBtn.x1 + 14, attackBtn.y1 + 25, const_cast<char*>("ATTACK"));
	}

	// Boss button - only spawns once a Skeleton round has been won at least
	// once (see g_hasWonSkeletonRound's comment above - Zombie no longer
	// factors in, since Arena_1 never spawns one anymore).
	if (g_hasWonSkeletonRound)
	{
		if (bossBtn.imgId != 0)
		{
			DrawButton(bossBtn);
		}
		else
		{
			// Images/Boss_button.png hasn't been added yet - same plain
			// rectangle fallback as the Attack button above.
			iSetColor(90, 20, 110);
			iFilledRectangle(bossBtn.x1, bossBtn.y1, bossBtn.x2 - bossBtn.x1, bossBtn.y2 - bossBtn.y1);
			iSetColor(255, 255, 255);
			iRectangle(bossBtn.x1, bossBtn.y1, bossBtn.x2 - bossBtn.x1, bossBtn.y2 - bossBtn.y1);
			iText(bossBtn.x1 + 20, bossBtn.y1 + 25, const_cast<char*>("BOSS"));
		}
	}

	if (showControls)
		drawHomeControlsBox();

	// 3. Victory announcement when all 6 zombies are killed
	if (g_baseSafeMessageActive)
		drawBaseSafeBox();

	// 4. Defeat announcement when the hero is downed during the defense
	if (g_baseDefeatedMessageActive)
		drawBaseDefeatedBox();


	if (armorIconID != 0) {
		iShowImage(715, 185, 70, 70, armorIconID);
	}

	iShowImage(715, 105, 70, 70, iLoadImage("Images/craftingbook.png"));

	drawInventoryUI();   // corner icon (always shown, bottom-right)
	drawCraftingMenu();  // book + task box, only while isCraftingMenuOpen is true
	drawArmorMenu();     // armor list, only while isArmorMenuOpen is true
	drawCraftPopup();    // "+1" popup - only while a craft just happened
}

inline void HomeBase_OnMouseDown(int button, int mx, int my)
{
	if (g_nightAlertActive || g_baseSafeMessageActive || g_baseDefeatedMessageActive)
	{
		if (button == GLUT_LEFT_BUTTON) g_alertClickPending = true;
		return;
	}

	// Only one of the three panels (Inventory / Crafting Book / Armor) can be
	// open at a time - they all reuse the same panel rect on screen, so
	// opening one closes the other two first.
	if (button == GLUT_LEFT_BUTTON)
	{
		// ১. Armor Icon Click Detection (X: 715 to 785, Y: 185 to 255)
		if (mx >= 715 && mx <= 785 && my >= 185 && my <= 255)
		{
			closeCraftingMenu();
			g_inventoryPanelOpen = false;
			toggleArmorMenu();
			return;
		}

		// ২. Crafting Book Icon Click Detection (X: 715 to 785, Y: 105 to 175)
		if (mx >= 715 && mx <= 785 && my >= 105 && my <= 175)
		{
			closeArmorMenu();
			g_inventoryPanelOpen = false;
			toggleCraftingMenu();
			return;
		}
	}

	// The inventory icon opens/closes the plain item list again.
	// Crafting now lives only on the dedicated Crafting Book icon above it -
	// swallow the click first, same way attackBtn already does below, so
	// opening it never also fires a gather-click on whatever resource node
	// happens to be nearest the player.
	if (button == GLUT_LEFT_BUTTON && IsInsideButton(g_inventoryIconBtn, mx, my))
	{
		closeCraftingMenu();
		closeArmorMenu();
		handleInventoryClick(button, mx, my);
		return;
	}

	// While the inventory panel is open, every other left-click belongs to
	// it (clicking an armor slot to equip/un-equip it) - same idea as the
	// crafting-book/armor-panel swallow blocks below, so a click meant for
	// an armor slot never falls through and fires a gather-click/attack
	// instead.
	if (button == GLUT_LEFT_BUTTON && g_inventoryPanelOpen)
	{
		handleInventoryClick(button, mx, my);
		return;
	}

	// While the crafting book is open, every other left-click belongs to
	// it (selecting a potion slot, hitting CRAFT, or clicking outside to
	// close it) - never falls through to gathering/attacking underneath.
	if (button == GLUT_LEFT_BUTTON && isCraftingMenuOpen)
	{
		craftingMenuOnClick(mx, my);
		return;
	}

	// Same idea while the (display-only) Armor panel is open - swallow
	// every other click; clicking outside it just closes it.
	if (button == GLUT_LEFT_BUTTON && isArmorMenuOpen)
	{
		armorMenuOnClick(mx, my);
		return;
	}

	if (button == GLUT_LEFT_BUTTON)  g_homeLeftClickPending = true;
	if (button == GLUT_RIGHT_BUTTON) g_homeRightClickPending = true;

	if (button == GLUT_LEFT_BUTTON && IsInsideButton(attackBtn, mx, my))
	{
		g_homeLeftClickPending = false;   // this click opened Battle, not a gather-click
		EnterLoading(GameState::BATTLE);
	}

	// Boss button - only live once it's actually showing (Skeleton round won).
	if (button == GLUT_LEFT_BUTTON && g_hasWonSkeletonRound
		&& IsInsideButton(bossBtn, mx, my))
	{
		g_homeLeftClickPending = false;   // this click opened Battle, not a gather-click
		g_forceBossFight = true;          // tells Battle_Init() to skip the level roll
		EnterLoading(GameState::BATTLE);
	}
}

// Returns true if the player pressed 'M' to leave for the main menu.
inline bool HomeBase_FixedUpdate()
{
	// Advance the night countdown by real elapsed time. This is the ONLY place
	// it ticks, which is what makes it pause automatically while in Battle.
	updateNightTimer();

	// Check if the night countdown just finished (skipped once the cycle has
	// been stopped after the first win/loss - see g_nightCycleStopped)
	if (!g_nightCycleStopped && !g_isNightTime && getNightTimeRemainingMs() == 0)
	{
		g_isNightTime = true;
		if (!g_nightAlertShown)
		{
			g_nightAlertActive = true;
			g_nightAlertShown = true;
		}
	}

	// While the night alert message is active:
	// Gameplay is paused, and pressing [Space] continues to the night base + starts zombie defense!
	if (g_nightAlertActive)
	{
		bool spaceDown = (isKeyPressed(' ') != 0);
		if (g_alertClickPending)
		{
			g_nightAlertActive = false;
			g_spaceWasDownForAlert = true;

			// Start Wave 1 (3 zombies)
			g_zombieDefenseActive = true;
			g_zombieWave = 1;
			spawnHomeZombieWave(3);
		}
		else if (!spaceDown)
		{
			g_spaceWasDownForAlert = false;
		}

		g_homeLeftClickPending = false;
		g_homeRightClickPending = false;
		g_alertClickPending = false;

		return isKeyPressed('m') || isKeyPressed('M');
	}
	else if (g_baseSafeMessageActive)
	{
		bool spaceDown = (isKeyPressed(' ') != 0);
		bool autoTimeout = (GetTickCount() - g_baseSafeMessageStartTime > 4000);

		if (g_alertClickPending || autoTimeout)
		{
			g_baseSafeMessageActive = false;
			g_spaceWasDownForAlert = true;

			// Day time comes again - but the night timer stays off for now
			// (see g_nightCycleStopped), so it won't count down and re-trigger.
			g_isNightTime = false;
			g_nightAlertActive = false;
			g_zombieDefenseActive = false;
			g_zombieWave = 0;
			g_homeZombieCount = 0;
			g_nightCycleStopped = true;

			// Restore hero's health for the new day
			roster[activeFighterIndex].currentHealth = roster[activeFighterIndex].maxHealth;
		}
		else if (!spaceDown)
		{
			g_spaceWasDownForAlert = false;
		}

		g_homeLeftClickPending = false;
		g_homeRightClickPending = false;
		g_alertClickPending = false;

		return isKeyPressed('m') || isKeyPressed('M');
	}
	else if (g_baseDefeatedMessageActive)
	{
		// Same "message box, then move on" shape as the other alerts, but
		// what it moves on TO is different: the hero died, so ResetSaveGame()
		// already wiped everything back to defaults (see the currentHealth
		// <= 0 check above) - once this box is dismissed, leave for the Menu
		// instead of continuing this now-reset Home Base session.
		bool spaceDown = (isKeyPressed(' ') != 0);
		bool autoTimeout = (GetTickCount() - g_baseDefeatedStartTime > 4000);

		if (g_alertClickPending || autoTimeout)
		{
			g_baseDefeatedMessageActive = false;
			g_spaceWasDownForAlert = true;

			g_homeLeftClickPending = false;
			g_homeRightClickPending = false;
			g_alertClickPending = false;

			return true;   // -> iMain.cpp's HOMEBASE handler calls EnterLoading(GameState::MENU)
		}
		else if (!spaceDown)
		{
			g_spaceWasDownForAlert = false;
		}

		g_homeLeftClickPending = false;
		g_homeRightClickPending = false;
		g_alertClickPending = false;

		return isKeyPressed('m') || isKeyPressed('M');
	}
	else
	{
		bool spaceDown = (isKeyPressed(' ') != 0);
		if (!spaceDown)
		{
			g_spaceWasDownForAlert = false;
		}
	}

	// ESC (ASCII 27) toggles the controls panel. fixedUpdate() runs every
	// tick while a key is held, so the "was it already down last tick"
	// check makes this flip once per press instead of dozens of times a second.
	bool escDown = (isKeyPressed(27) != 0);
	if (escDown && !g_escKeyWasDown) showControls = !showControls;
	g_escKeyWasDown = escDown;

	// activeFighterIndex is no longer switchable at runtime - it's locked once,
	// in HomeBase_Init(), to whichever hero was confirmed on CHARACTER_SELECT.
	Fighter &active = roster[activeFighterIndex];

	bool up = isKeyPressed('w') || isSpecialKeyPressed(GLUT_KEY_UP);
	bool down = isKeyPressed('s') || isSpecialKeyPressed(GLUT_KEY_DOWN);
	bool left = isKeyPressed('a') || isSpecialKeyPressed(GLUT_KEY_LEFT);
	bool right = isKeyPressed('d') || isSpecialKeyPressed(GLUT_KEY_RIGHT);
	handleMovement(active, up, down, left, right);

	// Zombie defense update (movement, attacks, wave progression)
	if (g_zombieDefenseActive)
	{
		for (int i = 0; i < g_homeZombieCount; i++)
		{
			if (g_homeZombies[i].isDead) continue;

			moveEnemyToward(g_homeZombies[i], active.x, active.y);

			if (tryEnemyAttack(g_homeZombies[i], active.x, active.y))
			{
				applyDamageToFighter(active, g_homeZombies[i].damage);
			}

			updateEnemy(g_homeZombies[i]);
		}

		// Hero downed during the zombie defense -> the base is lost.
		if (active.currentHealth <= 0)
		{
			g_zombieDefenseActive = false;
			g_baseDefeatedMessageActive = true;
			g_baseDefeatedStartTime = GetTickCount();
			active.currentHealth = active.maxHealth;   // don't leave the fighter sitting at 0 HP

			// Hero died -> the WHOLE game resets to its initial state, same
			// as a brand-new install: wipes the save file and every global
			// (level, coins, resources, inventory, AND character selection)
			// back to defaults. See the g_baseDefeatedMessageActive block
			// above, which sends the player to the Menu once this message
			// is dismissed.
			ResetSaveGame();

			// Not part of the save file, so ResetSaveGame() leaves these
			// alone on its own - reset them here too so a round win from
			// before death doesn't keep the Boss button unlocked afterward.
			g_hasWonSkeletonRound = false;
			g_hasWonZombieRound = false;
		}
		else
		{
			// Check wave completion: Wave 1 (3 zombies) -> Wave 2 (6 zombies) -> Base is Safe!
			int living = getLivingHomeZombieCount();
			if (living == 0)
			{
				if (g_zombieWave == 1)
				{
					g_zombieWave = 2;
					spawnHomeZombieWave(6);
				}
				else if (g_zombieWave == 2)
				{
					g_zombieWave = 3;
					g_zombieDefenseActive = false;
					g_baseSafeMessageActive = true;
					g_baseSafeMessageStartTime = GetTickCount();
				}
			}
		}
	}

	bool attackTrigger = (isKeyPressed(' ') && !g_spaceWasDownForAlert) || g_homeRightClickPending;

	if (g_zombieDefenseActive)
	{
		homePlayerAttackZombies(active, g_homeZombies, g_homeZombieCount, attackTrigger);
	}
	else
	{
		Fighter* allFighters[3] = { &roster[0], &roster[1], &roster[2] };
		tryAttack(active, attackTrigger, allFighters, 3);
	}
	g_homeRightClickPending = false;

	if (active.type == CHAR_GUARDIAN) tryActivateShield(active, isKeyPressed('e') != 0);
	if (active.type == CHAR_RANGER)   tryDash(active, isKeyPressed('D') != 0);

	for (int i = 0; i < 3; i++) updateFighter(roster[i]);

	if (g_homeLeftClickPending)
	{
		tryGatherNearestNode(active, homeResourceNodes, 4);
		g_homeLeftClickPending = false;
	}
	for (int i = 0; i < 4; i++) updateResourceNode(homeResourceNodes[i]);

	return isKeyPressed('m') || isKeyPressed('M');
}
