#include "iGraphics.h"
#include "Menu.h"
#include <cstdlib> 
#include "Player.hpp"

GameState currentState = GameState::MENU;

Button startBtn = { 326, 235, 466, 275, "START" };
Button levelBtn = { 326, 185, 466, 225, "LEVEL" };
Button exitBtn = { 326, 135, 466, 175, "EXIT" };

int x = 300;
int y = 330;

int backgroundImg1;

bool IsInsideButton(const Button& b, int mx, int my)
{
	return mx >= b.x1 && mx <= b.x2 && my >= b.y1 && my <= b.y2;
}

void DrawButton(const Button& b)
{
	iSetColor(70, 130, 180);
	iFilledRectangle(b.x1, b.y1, b.x2 - b.x1, b.y2 - b.y1);

	iSetColor(255, 255, 255);
	iRectangle(b.x1, b.y1, b.x2 - b.x1, b.y2 - b.y1);
	iText(b.x1 + 45, (b.y1 + b.y2) / 2 - 5, const_cast<char*>(b.label), GLUT_BITMAP_HELVETICA_18);
}

void DrawMenu()
{
	DrawButton(startBtn);
	DrawButton(levelBtn);
	DrawButton(exitBtn);
}

void iDraw()
{
	iClear();
	iShowImage(0, 0, 800, 600, backgroundImg1);

	switch (currentState)
	{
	case GameState::MENU:
		DrawMenu();
		break;
	case GameState::LEVEL_SELECT:
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
			currentState = GameState::PLAYING;
		}
		else if (IsInsideButton(levelBtn, mx, my))
		{
			currentState = GameState::LEVEL_SELECT;
		}
		else if (IsInsideButton(exitBtn, mx, my))
		{
			exit(0);
		}
	}

	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{

		
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
	iStart();
	return 0;
}