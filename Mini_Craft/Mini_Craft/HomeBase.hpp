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
#include "Fighters.hpp"
#include "GatherSystem.hpp"
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
int activeFighterIndex = CHAR_GUARDIAN;  // which roster[] slot the player currently controls - locked to the
                                          // character chosen on CHARACTER_SELECT, see HomeBase_Init()

// A "world" = 3 fights (see FIGHTS_PER_WORLD) at the current currentLevel's enemy
// mix (see Battle_Init()'s currentLevel-based enemy pick). g_worldFightsWon counts
// wins toward that within the current world; once it hits FIGHTS_PER_WORLD, iMain.cpp's
// fixedUpdate() resets it to 0 and bumps currentLevel - i.e. a new world starts.
#define FIGHTS_PER_WORLD 3
int g_worldFightsWon = 0;

// characterNumber (Menu.h: CHARACTER_ALCHEMIST=0, CHARACTER_RANGER=1, CHARACTER_GUARDIAN=2) uses a
// different order than FighterType (Fighters.hpp: CHAR_GUARDIAN=0, CHAR_RANGER=1, CHAR_ALCHEMIST=2),
// so this table maps one to the other, indexed by characterNumber.
static const FighterType kFighterForCharacterId[3] = { CHAR_ALCHEMIST, CHAR_RANGER, CHAR_GUARDIAN };

ResourceNode homeResourceNodes[4];

// 50x50 resource-count icons, shown top-right during Home Base (see drawHomeResourceHUD()).
unsigned int stoneIconID = 0;
unsigned int ironIconID = 0;
unsigned int woodIconID = 0;

// Bottom-left corner "Attack" button - clicking it sends the player into a
// BATTLE against an enemy (see Battle.hpp). imgId is set in HomeBase_Init().
Button attackBtn = { 15, 15, 15 + 85, 15 + 65, "", 0 };

// Set for one tick when the player clicks (iMouse() runs separately
// from fixedUpdate()), then read + cleared inside HomeBase_FixedUpdate().
bool g_homeLeftClickPending = false;
bool g_homeRightClickPending = false;

// Controls panel is hidden by default and toggles on/off each time ESC
// is pressed. g_escKeyWasDown remembers last frame's ESC state so it
// only flips showControls on the frame ESC goes down, not on every
// fixedUpdate() tick while it's held.
bool showControls = false;
bool g_escKeyWasDown = false;

// ---------------------------------------------------------------
//  NIGHT TIMER - counts down from NIGHT_TIMER_DURATION_MS (2 min),
//  starting the moment Home Base is entered. Uses GetTickCount()
//  directly (same pattern as the cooldowns in Fighters.hpp/
//  GatherSystem.hpp) rather than counting fixedUpdate() ticks, so
//  it stays accurate regardless of frame rate.
// ---------------------------------------------------------------
unsigned long g_nightTimerStartTick = 0;
bool g_isNightTime = false;   // flips true once the countdown reaches 0:00

// Call this once, right when Home Base is entered (see EnterLoading()'s
// LOADING->HOMEBASE handoff in iMain.cpp) to (re)start the 2-minute countdown.
inline void HomeBase_StartNightTimer()
{
	g_nightTimerStartTick = GetTickCount();
	g_isNightTime = false;
}

// Milliseconds left until night, clamped to 0 (never negative).
inline unsigned long getNightTimeRemainingMs()
{
	if (g_nightTimerStartTick == 0) return NIGHT_TIMER_DURATION_MS;
	unsigned long elapsed = GetTickCount() - g_nightTimerStartTick;
	if (elapsed >= NIGHT_TIMER_DURATION_MS) return 0;
	return NIGHT_TIMER_DURATION_MS - elapsed;
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
// Stone / Iron / Wood, stacked vertically at x=730 down the right edge of the screen.
// Counts come straight from GatherSystem.hpp's g_inventory (via the getter
// functions), so this is purely a display of state that already exists.
inline void drawHomeResourceHUD()
{
	const double iconSize = 25;
	const double iconX = 730;                 // left edge of the icon column
	const double textX = iconX + iconSize + 10;
	double iconY = HOME_AREA_H - 65;          // top slot, near the top of the screen
	const double rowGap = 35;

	struct { unsigned int tex; int count; } rows[3] = {
		{ stoneIconID, getStoneCount() },
		{ ironIconID,  getIronCount()  },
		{ woodIconID,  getWoodCount()  },
	};

	iSetColor(255, 255, 255);
	for (int i = 0; i < 3; i++)
	{
		iShowImage((int)iconX, (int)iconY, (int)iconSize, (int)iconSize, rows[i].tex);

		char buf[16];
		sprintf(buf, "%d", rows[i].count);
		iText(textX, iconY + iconSize / 2 - 5, buf);

		iconY -= rowGap;
	}
}

// Right-edge "Night time in: MM:SS" readout inside a color box for clear visibility.
// Sits above the resource HUD (which occupies x=730 down from y=HOME_AREA_H-65) so the two never overlap.
inline void drawNightTimer()
{
	unsigned long remainingMs = getNightTimeRemainingMs();
	int minutes = (int)(remainingMs / 60000);
	int seconds = (int)((remainingMs / 1000) % 60);

	char buf[32];
	sprintf(buf, "Night time in: %d:%02d", minutes, seconds);

	const double boxX = 525;
	const double boxY = HOME_AREA_H - 44;
	const double boxW = 195;
	const double boxH = 34;

	if (remainingMs == 0)
	{
		// Timer stopped at 0: Red alert color box
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
//  SCENE LIFECYCLE
// ---------------------------------------------------------------
inline void HomeBase_Init()
{
	homeBaseTexture = iLoadImage("Images/homeBaseBackground.png");
	homeBaseNightTexture = fileExists("Images/homeBaseBackground_night.png") ? iLoadImage("Images/homeBaseBackground_night.png") : 0;
	if (g_nightTimerStartTick == 0) HomeBase_StartNightTimer();

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

	// fileExists() check first: iLoadImage() below always returns a valid (non-zero)
	// OpenGL texture handle even when the file is missing - it just uploads
	// uninitialized/garbage image data, which usually ends up fully transparent
	// (invisible) rather than erroring. So imgId being non-zero does NOT mean the
	// art loaded - checking the file directly is the only reliable signal, and is
	// what lets HomeBase_Draw() below fall back to a plain rectangle button when
	// Images/Attack_button.png hasn't been added yet.
	attackBtn.imgId = fileExists("Images/Attack_button.png") ? iLoadImage("Images/Attack_button.png") : 0;
}

// ---------------------------------------------------------------
//  SAVE / RESUME
//  A "world" is currentLevel + how far into it the player is
//  (g_worldFightsWon) + which hero they picked (characterNumber) +
//  the shared resource pool (g_inventory[], GatherSystem.hpp) + that
//  hero's current health. SaveGame() is called both when a new world
//  starts (the two currentLevel++ sites in iMain.cpp) AND whenever the
//  player leaves Home Base for the main MENU ('M' - see iMain.cpp's
//  fixedUpdate()), so progress made mid-world (fights won so far,
//  resources gathered, damage taken) isn't lost just by backing out to
//  the menu. Clicking Start on the main MENU then checks
//  HasSaveFile()/LoadGame() to jump straight back into all of that
//  instead of going through CHARACTER_SELECT again.
// ---------------------------------------------------------------
const char* SAVE_FILE_PATH = "savegame.txt";

// Saved health of the fighter this save was taken with is restored in two
// steps: LoadGame() reads it into here, and the Start button code (iMain.cpp)
// applies it to roster[activeFighterIndex] AFTER HomeBase_Init() runs (which
// would otherwise reset every fighter back to full health).
int g_savedPlayerHealth = -1;   // -1 = no saved health to apply (fresh game)

inline bool HasSaveFile()
{
	return fileExists(SAVE_FILE_PATH);
}

inline void SaveGame()
{
	FILE* f = fopen(SAVE_FILE_PATH, "w");
	if (!f) return;   // couldn't open (e.g. read-only folder) - just skip saving silently

	fprintf(f, "%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n",
		characterNumber, currentLevel, g_worldFightsWon,
		g_inventory[RES_STONE], g_inventory[RES_WOOD], g_inventory[RES_IRON], g_inventory[RES_WATER],
		roster[activeFighterIndex].currentHealth);

	fclose(f);
}

// Reads the save file back into characterNumber/currentLevel/g_worldFightsWon/
// g_inventory[]/g_savedPlayerHealth. Returns false (and leaves everything
// untouched) if there's no save file or it doesn't parse - callers should
// fall back to a fresh game.
inline bool LoadGame()
{
	FILE* f = fopen(SAVE_FILE_PATH, "r");
	if (!f) return false;

	int savedCharacter, savedLevel, savedFightsWon, savedStone, savedWood, savedIron, savedWater, savedHealth;
	int fieldsRead = fscanf(f, "%d %d %d %d %d %d %d %d",
		&savedCharacter, &savedLevel, &savedFightsWon,
		&savedStone, &savedWood, &savedIron, &savedWater, &savedHealth);
	fclose(f);

	if (fieldsRead != 8) return false;   // malformed/partial file - don't apply a half-read save

	characterNumber = savedCharacter;
	currentLevel = savedLevel;
	g_worldFightsWon = savedFightsWon;
	g_inventory[RES_STONE] = savedStone;
	g_inventory[RES_WOOD] = savedWood;
	g_inventory[RES_IRON] = savedIron;
	g_inventory[RES_WATER] = savedWater;
	g_savedPlayerHealth = savedHealth;

	return true;
}

inline void HomeBase_Draw()
{
	if ((g_isNightTime || getNightTimeRemainingMs() == 0) && homeBaseNightTexture != 0)
		iShowImage(0, 0, HOME_AREA_W, HOME_AREA_H, homeBaseNightTexture);
	else
		iShowImage(0, 0, HOME_AREA_W, HOME_AREA_H, homeBaseTexture);

	for (int i = 0; i < 4; i++)
		drawResourceNode(homeResourceNodes[i]);

	drawFighter(roster[activeFighterIndex]);

	drawHomeHealthBar();
	drawHomeResourceHUD();
	drawNightTimer();

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

	if (showControls)
		drawHomeControlsBox();
}

inline void HomeBase_OnMouseDown(int button, int mx, int my)
{
	if (button == GLUT_LEFT_BUTTON)  g_homeLeftClickPending = true;
	if (button == GLUT_RIGHT_BUTTON) g_homeRightClickPending = true;

	if (button == GLUT_LEFT_BUTTON && IsInsideButton(attackBtn, mx, my))
	{
		g_homeLeftClickPending = false;   // this click opened Battle, not a gather-click
		EnterLoading(GameState::BATTLE);
	}
}

// Returns true if the player pressed 'M' to leave for the main menu.
inline bool HomeBase_FixedUpdate()
{
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

	bool attackTrigger = isKeyPressed(' ') || g_homeRightClickPending;
	Fighter* allFighters[3] = { &roster[0], &roster[1], &roster[2] };
	tryAttack(active, attackTrigger, allFighters, 3);
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

	updateRandomResourceBonus();   // free +5 to a random resource every 5s (GatherSystem.hpp)

	if (!g_isNightTime && getNightTimeRemainingMs() == 0) g_isNightTime = true;

	return isKeyPressed('m') || isKeyPressed('M');
}
