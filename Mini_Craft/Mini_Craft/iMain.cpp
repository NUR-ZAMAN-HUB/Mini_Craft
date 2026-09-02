
#define _CRT_SECURE_NO_WARNINGS   // silence sprintf/etc. "unsafe" warnings

#include "iGraphics.h"
#include "GameConfig.hpp"
#include "Player.hpp"
#include "ResourceSystem.hpp"

// =====================================================================
//  GLOBAL STATE
// =====================================================================

Character roster[3];
unsigned int homeBaseTexture;
int activeIndex = CHAR_GUARDIAN;

ResourceNode resourceNodes[4];

bool g_rightClickPending = false;
bool g_leftClickPending = false;

// =====================================================================
//  UI HELPERS
// =====================================================================

static int buildRoundedRectPoints(double x, double y, double w, double h,
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

static void drawSolidBlackPanel(double x, double y, double w, double h, double radius)
{
	double px[64], py[64];
	int n = buildRoundedRectPoints(x, y, w, h, radius, px, py);

	iSetColor(0, 0, 0);
	glBegin(GL_POLYGON);
	for (int i = 0; i < n; i++) glVertex2d(px[i], py[i]);
	glEnd();
}

static void drawControlsBox()
{
	const double boxX = 15, boxY = 15;
	const double boxW = 250, boxH = 150;

	drawSolidBlackPanel(boxX, boxY, boxW, boxH, 12.0);

	double textX = boxX + 15;
	double lineY = boxY + boxH - 24;
	const double lineGap = 18;

	iSetColor(255, 255, 255);
	iText(textX, lineY, "CONTROLS");
	lineY -= lineGap + 4;

	iText(textX, lineY, "Move    : WASD / Arrows");   lineY -= lineGap;
	iText(textX, lineY, "Attack  : Space / R-Click");  lineY -= lineGap;
	iText(textX, lineY, "Shield  : E");                lineY -= lineGap;
	iText(textX, lineY, "Dash    : Shift + D");         lineY -= lineGap;
	iText(textX, lineY, "Gather  : Left-Click");        lineY -= lineGap;
	iText(textX, lineY, "Switch  : 1 / 2 / 3");
}

// =====================================================================
//  DRAW
// =====================================================================
void iDraw()
{
	iClear();
	iShowImage(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, homeBaseTexture);

	for (int i = 0; i < 4; i++)
		drawResourceNode(resourceNodes[i]);

	// Active character drawing
	drawCharacter(roster[activeIndex]);

	
	double hudX = 15;
	double hudY = 175;
	double hudW = 250;
	double hudH = 65;

	drawSolidBlackPanel(hudX, hudY, hudW, hudH, 10.0);

	char buf[128];
	iSetColor(255, 255, 255);

	// Line 1: Active Character
	sprintf(buf, "Active: %s", characterName(roster[activeIndex].type));
	iText(hudX + 14, hudY + hudH - 23, buf);

	// Line 2: HP 
	sprintf(buf, "HP: %d/%d", roster[activeIndex].currentHealth, roster[activeIndex].maxHealth);
	iText(hudX + 14, hudY + 18, buf);

	drawControlsBox();
}

void iMouseMove(int mx, int my) { }
void iPassiveMouseMove(int mx, int my) { }

void iMouse(int button, int state, int mx, int my)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		g_leftClickPending = true;
	}

	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
	{
		g_rightClickPending = true;
	}
}

// =====================================================================
//  FIXED UPDATE
// =====================================================================
void fixedUpdate()
{
	if (isKeyPressed('1')) activeIndex = CHAR_GUARDIAN;
	if (isKeyPressed('2')) activeIndex = CHAR_RANGER;
	if (isKeyPressed('3')) activeIndex = CHAR_ALCHEMIST;

	Character &active = roster[activeIndex];

	bool up = (isKeyPressed('w') != 0) || (isSpecialKeyPressed(GLUT_KEY_UP) != 0);
	bool down = (isKeyPressed('s') != 0) || (isSpecialKeyPressed(GLUT_KEY_DOWN) != 0);
	bool left = (isKeyPressed('a') != 0) || (isSpecialKeyPressed(GLUT_KEY_LEFT) != 0);
	bool right = (isKeyPressed('d') != 0) || (isSpecialKeyPressed(GLUT_KEY_RIGHT) != 0);
	handleMovement(active, up, down, left, right);

	bool attackTrigger = (isKeyPressed(' ') != 0) || g_rightClickPending;
	Character* allCharacters[3] = { &roster[0], &roster[1], &roster[2] };
	tryAttack(active, attackTrigger, allCharacters, 3);
	g_rightClickPending = false;

	if (active.type == CHAR_GUARDIAN)
	{
		bool shieldTrigger = (isKeyPressed('e') != 0);
		tryActivateShield(active, shieldTrigger);
	}

	if (active.type == CHAR_RANGER)
	{
		bool dashTrigger = (isKeyPressed('D') != 0);
		tryDash(active, dashTrigger);
	}

	for (int i = 0; i < 3; i++)
		updateCharacter(roster[i]);

	if (g_leftClickPending)
	{
		tryGatherNearestNode(active, resourceNodes, 4);
		g_leftClickPending = false;
	}
	for (int i = 0; i < 4; i++)
		updateResourceNode(resourceNodes[i]);
}

// =====================================================================
//  MAIN
// =====================================================================
int main()
{
	mciSendString("open \"Audios//background.mp3\" alias bgsong", NULL, 0, NULL);
	mciSendString("open \"Audios//gameover.mp3\" alias ggsong", NULL, 0, NULL);

	mciSendString("play bgsong repeat", NULL, 0, NULL);

	iInitialize(SCREEN_WIDTH, SCREEN_HEIGHT, "Mini Craft - Checkpoint 1");

	homeBaseTexture = iLoadImage("Images/homeBaseBackground.png");

	
	double portalSpotX = 380;
	double portalSpotY = 160;

	initGuardian(roster[CHAR_GUARDIAN], portalSpotX, portalSpotY);
	initRanger(roster[CHAR_RANGER], portalSpotX, portalSpotY);
	initAlchemist(roster[CHAR_ALCHEMIST], portalSpotX, portalSpotY);

	initAllResourceNodes(resourceNodes);

	iStart();
	return 0;
}