#include "iGraphics.h"
#include "Menu.h"
#include <cstdlib>
#include <cstdio>
#include "Player.hpp"

GameState currentState = GameState::MENU;

Button startBtn = { 326, 235, 466, 275, "START" };
Button levelBtn = { 326, 185, 466, 225, "LEVEL" };
Button settingsBtn = { 326, 135, 466, 175, "SETTINGS" };
Button exitBtn = { 326, 85, 466, 125, "EXIT" };

// Shared "back to menu" button, reused on the LEVEL and SETTINGS screens
Button backBtn = { 30, 30, 150, 70, "BACK" };

// LEVEL_SELECT screen
Button resetLevelBtn = { 275, 220, 525, 265, "RESET LEVEL" };

// SETTINGS screen (difficulty showcase)
Button easyBtn = { 100, 300, 260, 345, "EASY" };
Button mediumBtn = { 320, 300, 480, 345, "MEDIUM" };
Button hardBtn = { 540, 300, 700, 345, "HARD" };

// CHARACTER_SELECT screen
// "Next >" cycles cards. Clicking the card picture selects it, which spawns
// "Confirm" underneath it - clicking Confirm is what actually moves on.
Button nextCharBtn = { 650, 15, 770, 55, "Next >" };
Button confirmCharBtn = { 300, 15, 500, 55, "Confirm" };

int currentLevel = 1;
int difficulty = 0;   // 0 = Easy, 1 = Medium, 2 = Hard

int x = 300;
int y = 330;

int backgroundImg1;
int backgroundImg2;
int selectCharacterImg;   // "select character.jpg" banner shown at the top of CHARACTER_SELECT

// Card images are 842x576. On the 800x600 screen we scale that down to
// 614x420 (same aspect ratio) so the banner still fits above and the
// Confirm/Next buttons still fit below.
CharacterBox charBoxes[3] = {
	{ 93, 70, 707, 490, -1 },
	{ 93, 70, 707, 490, -1 },
	{ 93, 70, 707, 490, -1 }
};

int characterNumber = -1;    // 0 = Alchemist, 1 = Ranger, 2 = Guardian (see CharacterId in Menu.h). Set when the player clicks a card picture.
int currentCardIndex = 0;    // which of the 3 cards is currently displayed

bool IsInsideButton(const Button& b, int mx, int my)
{
	return mx >= b.x1 && mx <= b.x2 && my >= b.y1 && my <= b.y2;
}

bool IsInsideBox(const CharacterBox& b, int mx, int my)
{
	return mx >= b.x1 && mx <= b.x2 && my >= b.y1 && my <= b.y2;
}

void DrawButton(const Button& b, bool highlighted)
{
	// All buttons share the same fill color now (60, 191, 4).
	// "highlighted" (the currently-selected option, e.g. on SETTINGS) is
	// still shown, but via a thicker yellow outline instead of a different fill.
	iSetColor(60, 191, 4);
	iFilledRectangle(b.x1, b.y1, b.x2 - b.x1, b.y2 - b.y1);

	if (highlighted)
	{
		iSetColor(255, 255, 0);
		iRectangle(b.x1 - 1, b.y1 - 1, b.x2 - b.x1 + 2, b.y2 - b.y1 + 2);
		iRectangle(b.x1, b.y1, b.x2 - b.x1, b.y2 - b.y1);
	}

	iSetColor(255, 255, 255);
	iRectangle(b.x1, b.y1, b.x2 - b.x1, b.y2 - b.y1);
	iText(b.x1 + 15, (b.y1 + b.y2) / 2 - 5, const_cast<char*>(b.label), GLUT_BITMAP_HELVETICA_18);
}

void DrawMenu()
{
	DrawButton(startBtn);
	DrawButton(levelBtn);
	DrawButton(settingsBtn);
	DrawButton(exitBtn);
}

void DrawCharacterSelect()
{
	// "Select Character" banner across the top of the screen
	iShowImage(150, 500, 500, 60, selectCharacterImg);

	// Only the current card in the cycle (0, 1, 2 -> back to 0 ...) is shown
	const CharacterBox& box = charBoxes[currentCardIndex];
	iShowImage(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1, box.imgId);

	// "Next >" always cycles to the next character card.
	DrawButton(nextCharBtn);

	// "Confirm" only spawns once this card has been clicked (selected).
	// Clicking Confirm is what actually advances to the next scene.
	if (characterNumber == currentCardIndex)
	{
		DrawButton(confirmCharBtn);
	}

	// Lets the player back out to the main menu
	DrawButton(backBtn);
}

void DrawLoading()
{
	// Plain blank screen (no background image) with a loading message
	iSetColor(0, 0, 0);
	iFilledRectangle(0, 0, 800, 600);

	iSetColor(255, 255, 255);
	iText(280, 300, const_cast<char*>("Next scene is loading..."), GLUT_BITMAP_TIMES_ROMAN_24);
}

void DrawLevelSelect()
{
	// Level indicator "showcase" - just displays the level currently reached
	iSetColor(255, 255, 255);
	char levelText[32];
	sprintf_s(levelText, "Current Level: %d", currentLevel);
	iText(300, 420, levelText, GLUT_BITMAP_TIMES_ROMAN_24);

	DrawButton(resetLevelBtn);

	// Warning shown under the reset button
	iSetColor(220, 60, 60);
	iText(190, 190, const_cast<char*>("Warning: resetting will take you back to Level 1."), GLUT_BITMAP_HELVETICA_18);

	DrawButton(backBtn);
}

void DrawSettings()
{
	iSetColor(255, 255, 255);
	iText(340, 420, const_cast<char*>("Difficulty"), GLUT_BITMAP_TIMES_ROMAN_24);

	// Showcase only - highlights whichever difficulty is currently selected
	DrawButton(easyBtn, difficulty == 0);
	DrawButton(mediumBtn, difficulty == 1);
	DrawButton(hardBtn, difficulty == 2);

	DrawButton(backBtn);
}

void iDraw()
{
	iClear();

	// The MENU screen keeps the original background; the moment the player
	// leaves the menu the background switches to Game_background2.jpg.
	// LOADING is a plain blank screen, so it skips both background images
	// (DrawLoading fills the screen itself).
	if (currentState == GameState::MENU)
		iShowImage(0, 0, 800, 600, backgroundImg1);
	else if (currentState != GameState::LOADING)
		iShowImage(0, 0, 800, 600, backgroundImg2);

	switch (currentState)
	{
	case GameState::MENU:
		DrawMenu();
		break;
	case GameState::CHARACTER_SELECT:
		DrawCharacterSelect();
		break;
	case GameState::LOADING:
		DrawLoading();
		break;
	case GameState::LEVEL_SELECT:
		DrawLevelSelect();
		break;
	case GameState::SETTINGS:
		DrawSettings();
		break;
	case GameState::PLAYING:
		break;
	}
}

void iMouseMove(int mx, int my)
{

}

void iPassiveMouseMove(int mx, int my)
{

}

void iMouse(int button, int state, int mx, int my)
{

	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && currentState == GameState::MENU)
	{
		if (IsInsideButton(startBtn, mx, my))
		{
			currentState = GameState::CHARACTER_SELECT;
		}
		else if (IsInsideButton(levelBtn, mx, my))
		{
			currentState = GameState::LEVEL_SELECT;
		}
		else if (IsInsideButton(settingsBtn, mx, my))
		{
			currentState = GameState::SETTINGS;
		}
		else if (IsInsideButton(exitBtn, mx, my))
		{
			exit(0);
		}
	}
	else if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && currentState == GameState::CHARACTER_SELECT)
	{
		if (IsInsideButton(nextCharBtn, mx, my))
		{
			// Cycle 0 -> 1 -> 2 -> 0 ...
			currentCardIndex = (currentCardIndex + 1) % 3;
		}
		else if (characterNumber == currentCardIndex && IsInsideButton(confirmCharBtn, mx, my))
		{
			// Confirm only exists once a card is selected - clicking it moves on
			currentState = GameState::LOADING;
		}
		else if (IsInsideButton(backBtn, mx, my))
		{
			currentState = GameState::MENU;
		}
		else if (IsInsideBox(charBoxes[currentCardIndex], mx, my))
		{
			// Clicking the picture itself selects that character, which
			// spawns the Confirm button under it
			characterNumber = currentCardIndex;
		}
	}
	else if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && currentState == GameState::LEVEL_SELECT)
	{
		if (IsInsideButton(resetLevelBtn, mx, my))
		{
			currentLevel = 1;   // reset always sends the player back to level 1
		}
		else if (IsInsideButton(backBtn, mx, my))
		{
			currentState = GameState::MENU;
		}
	}
	else if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && currentState == GameState::SETTINGS)
	{
		if (IsInsideButton(easyBtn, mx, my))
		{
			difficulty = 0;
		}
		else if (IsInsideButton(mediumBtn, mx, my))
		{
			difficulty = 1;
		}
		else if (IsInsideButton(hardBtn, mx, my))
		{
			difficulty = 2;
		}
		else if (IsInsideButton(backBtn, mx, my))
		{
			currentState = GameState::MENU;
		}
	}

	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
	{

	}
}

// Special Keys:
// GLUT_KEY_F1, GLUT_KEY_F2, GLUT_KEY_F3, GLUT_KEY_F4, GLUT_KEY_F5, GLUT_KEY_F6, GLUT_KEY_F7, GLUT_KEY_F8, GLUT_KEY_F9, GLUT_KEY_F10, GLUT_KEY_F11, GLUT_KEY_F12, 
// GLUT_KEY_LEFT, GLUT_KEY_UP, GLUT_KEY_RIGHT, GLUT_KEY_DOWN, GLUT_KEY_PAGE UP, GLUT_KEY_PAGE DOWN, GLUT_KEY_HOME, GLUT_KEY_END, GLUT_KEY_INSERT

void fixedUpdate()
{
	if (isKeyPressed('w') || isSpecialKeyPressed(GLUT_KEY_UP))
	{
		y++;
	}
	if (isKeyPressed('a') || isSpecialKeyPressed(GLUT_KEY_LEFT))
	{
		x--;
	}
	if (isKeyPressed('s') || isSpecialKeyPressed(GLUT_KEY_DOWN))
	{
		y--;
	}
	if (isKeyPressed('d') || isSpecialKeyPressed(GLUT_KEY_RIGHT))
	{
		x++;
	}

	if (isKeyPressed(' ')) {

	}
}


int main()
{
	iInitialize(800, 600, "Project Title");
	backgroundImg1 = iLoadImage("Images//Game_background1.jpg");
	backgroundImg2 = iLoadImage("Images//Game_background2.jpg");
	selectCharacterImg = iLoadImage("Images//select character.jpg");

	// These read CH[i].fightStand from Player.hpp, which points at
	// "Alchemist card.jpg", "Ranger card.jpg", and "Guardian card.jpg" (all 842x576).
	charBoxes[0].imgId = iLoadImage(CH[0].fightStand.c_str());
	charBoxes[1].imgId = iLoadImage(CH[1].fightStand.c_str());
	charBoxes[2].imgId = iLoadImage(CH[2].fightStand.c_str());

	iStart();
	return 0;
}