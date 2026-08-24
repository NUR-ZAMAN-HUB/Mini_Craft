#include "iGraphics.h"

//FOR TEST !!


// Screen Dimensions
int screenWidth = 1400;
int screenHeight = 800;

// Hero Position
int x = 100;
int y = 100;

//Witch unconscious position
int witchX = 540;
int witchY = 310;


void iDraw()
{
	iClear();

	//Background Map
	iShowBMP(0, 0, "images/map.bmp");


	//Witch laying unconscious
	iShowBMP2(witchX, witchY, "images/unconscious_witch.bmp", 0xFFFFFF);


}


void iMouseMove(int mx, int my)
{
}

void iPassiveMouseMove(int mx, int my)
{
}

void iMouse(int button, int state, int mx, int my)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
	}

	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
	{
	}
}

void iKeyboard(unsigned char key)
{
}

void iSpecialKeyboard(unsigned char key)
{
}

void fixedUpdate()
{
	
	if (isKeyPressed('w') || isSpecialKeyPressed(GLUT_KEY_UP))
	{
		y += 5;
	}
	if (isKeyPressed('a') || isSpecialKeyPressed(GLUT_KEY_LEFT))
	{
		x -= 5;
	}
	if (isKeyPressed('s') || isSpecialKeyPressed(GLUT_KEY_DOWN))
	{
		y -= 5;
	}
	if (isKeyPressed('d') || isSpecialKeyPressed(GLUT_KEY_RIGHT))
	{
		x += 5;
	}
}


int main()
{
	iInitialize(screenWidth, screenHeight, "Save the Witch");
	iStart();
	return 0;
}