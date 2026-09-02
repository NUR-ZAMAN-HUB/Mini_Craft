// =====================================================================
//  HomeBase.hpp
// =====================================================================
//  This is the "Home Base" scene: pick one of 3 fighters (Guardian /
//  Ranger / Alchemist), walk around, attack, use your fighter's
//  special ability, and gather Stone/Wood/Iron/Water from resource
//  nodes.
//
//  It's everything that used to live in part_maria's own iMain.cpp
//  (which had its own main(), iDraw(), iMouse(), fixedUpdate()). Since
//  this project already has ONE main()/iDraw()/iMouse()/fixedUpdate()
//  (in the project's iMain.cpp, driving the menu + "Save the Witch"
//  map), Home Base can't have a second set of those - iGraphics only
//  calls one of each. So the same logic is exposed here as four plain
//  functions that the project's iMain.cpp calls into whenever
//  currentState == GameState::HOMEBASE:
//
//      HomeBase_Init()      -> call once, after iInitialize()
//      HomeBase_FixedUpdate() -> call every tick while in HOMEBASE
//      HomeBase_Draw()        -> call every frame while in HOMEBASE
//      HomeBase_OnMouseDown(button) -> call from iMouse() while in HOMEBASE
//
//  Nothing about the gameplay itself changed - same controls, same
//  stats, same abilities.
// =====================================================================
#pragma once

#include "iGraphics.h"
#include "HomeBaseConfig.hpp"
#include "Fighters.hpp"
#include "GatherSystem.hpp"

// ---------------------------------------------------------------
//  STATE
// ---------------------------------------------------------------
Fighter roster[3];
unsigned int homeBaseTexture = 0;
int activeFighterIndex = CHAR_GUARDIAN;

ResourceNode homeResourceNodes[4];

bool g_homeLeftClickPending = false;
bool g_homeRightClickPending = false;

// ---------------------------------------------------------------
//  SMALL UI HELPERS (rounded black HUD panels)
// ---------------------------------------------------------------
inline int buildRoundedRectPoints(double x, double y, double w, double h,
	double radius, double outX[], double outY[])
{
	const int segs = 8;
	const double PI = 3.14159265358979323846;
	int n = 0;

	struct { double cx, cy, startAngle; } corners[4] = {
		{ x + w - radius, y + radius, -PI / 2.0 },
		{ x + w - radius, y + h - radius, 0.0 },
		{ x + radius, y + h - radius, PI / 2.0 },
		{ x + radius, y + radius, PI }
	};

	for (int c = 0; c < 4; c++)
	{
		for (int s = 0; s <= segs; s++)
		{
			double t = corners[c].startAngle + (PI / 2.0) * (s / (double)segs);
			outX[n] = corners[c].cx + radius * cos(t);
			outY[n] = corners[c].cy + radius * sin(t);
			n++;
		}
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

inline void drawHomeControlsBox()
{
	const double boxX = 15, boxY = 15;
	const double boxW = 250, boxH = 165;

	drawSolidBlackPanel(boxX, boxY, boxW, boxH, 12.0);

	double textX = boxX + 15;
	double lineY = boxY + boxH - 24;
	const double lineGap = 18;

	iSetColor(255, 255, 255);
	iText(textX, lineY, (char*)"CONTROLS"); lineY -= lineGap + 4;

	iText(textX, lineY, (char*)"Move    : WASD / Arrows");  lineY -= lineGap;
	iText(textX, lineY, (char*)"Attack  : Space / R-Click"); lineY -= lineGap;
	iText(textX, lineY, (char*)"Shield  : E");               lineY -= lineGap;
	iText(textX, lineY, (char*)"Dash    : Shift + D");       lineY -= lineGap;
	iText(textX, lineY, (char*)"Gather  : Left-Click");      lineY -= lineGap;
	iText(textX, lineY, (char*)"Switch  : 1 / 2 / 3");       lineY -= lineGap;
	iText(textX, lineY, (char*)"Menu    : M");
}

inline void drawHomeHud()
{
	double hudX = 15, hudY = 190, hudW = 250, hudH = 65;
	drawSolidBlackPanel(hudX, hudY, hudW, hudH, 10.0);

	char buf[128];
	iSetColor(255, 255, 255);

	sprintf(buf, "Active: %s", fighterTypeName(roster[activeFighterIndex].type));
	iText(hudX + 14, hudY + hudH - 23, buf);

	sprintf(buf, "HP: %d/%d", roster[activeFighterIndex].currentHealth, roster[activeFighterIndex].maxHealth);
	iText(hudX + 14, hudY + 18, buf);
}

// ---------------------------------------------------------------
//  SCENE LIFECYCLE
// ---------------------------------------------------------------

// Loads all Home Base art + sets up the roster and resource nodes.
// Call this once, right after iInitialize() in main().
inline void HomeBase_Init()
{
	homeBaseTexture = iLoadImage("Images/homeBaseBackground.png");

	double portalSpotX = 380;
	double portalSpotY = 160;

	initGuardian(roster[CHAR_GUARDIAN], (float)portalSpotX, (float)portalSpotY);
	initRanger(roster[CHAR_RANGER], (float)portalSpotX, (float)portalSpotY);
	initAlchemist(roster[CHAR_ALCHEMIST], (float)portalSpotX, (float)portalSpotY);

	initAllResourceNodes(homeResourceNodes);
}

// Draws the whole scene: background, resource nodes, active fighter, HUD.
inline void HomeBase_Draw()
{
	iShowImage(0, 0, HOME_AREA_W, HOME_AREA_H, homeBaseTexture);

	for (int i = 0; i < 4; i++)
		drawResourceNode(homeResourceNodes[i]);

	drawFighter(roster[activeFighterIndex]);

	drawHomeHud();
	drawHomeControlsBox();
}

// Set from iMouse() while in the HOMEBASE state.
inline void HomeBase_OnMouseDown(int button)
{
	if (button == GLUT_LEFT_BUTTON)  g_homeLeftClickPending = true;
	if (button == GLUT_RIGHT_BUTTON) g_homeRightClickPending = true;
}

// Movement / combat / gathering tick. Call every fixedUpdate() while
// in the HOMEBASE state. Returns true if the player pressed 'M' to
// leave for the main menu (caller decides what that means).
inline bool HomeBase_FixedUpdate()
{
	if (isKeyPressed('1')) activeFighterIndex = CHAR_GUARDIAN;
	if (isKeyPressed('2')) activeFighterIndex = CHAR_RANGER;
	if (isKeyPressed('3')) activeFighterIndex = CHAR_ALCHEMIST;

	Fighter &active = roster[activeFighterIndex];

	bool up = (isKeyPressed('w') != 0) || (isSpecialKeyPressed(GLUT_KEY_UP) != 0);
	bool down = (isKeyPressed('s') != 0) || (isSpecialKeyPressed(GLUT_KEY_DOWN) != 0);
	bool left = (isKeyPressed('a') != 0) || (isSpecialKeyPressed(GLUT_KEY_LEFT) != 0);
	bool right = (isKeyPressed('d') != 0) || (isSpecialKeyPressed(GLUT_KEY_RIGHT) != 0);
	handleMovement(active, up, down, left, right);

	bool attackTrigger = (isKeyPressed(' ') != 0) || g_homeRightClickPending;
	Fighter* allFighters[3] = { &roster[0], &roster[1], &roster[2] };
	tryAttack(active, attackTrigger, allFighters, 3);
	g_homeRightClickPending = false;

	if (active.type == CHAR_GUARDIAN)
		tryActivateShield(active, isKeyPressed('e') != 0);

	if (active.type == CHAR_RANGER)
		tryDash(active, isKeyPressed('D') != 0);

	for (int i = 0; i < 3; i++)
		updateFighter(roster[i]);

	if (g_homeLeftClickPending)
	{
		tryGatherNearestNode(active, homeResourceNodes, 4);
		g_homeLeftClickPending = false;
	}
	for (int i = 0; i < 4; i++)
		updateResourceNode(homeResourceNodes[i]);

	return (isKeyPressed('m') != 0) || (isKeyPressed('M') != 0);
}
