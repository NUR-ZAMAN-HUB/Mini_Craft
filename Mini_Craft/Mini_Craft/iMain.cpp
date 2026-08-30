#include "iGraphics.h"

// Game States
enum GameState {
	STATE_GAMEPLAY,
	STATE_CUTSCENE,
	STATE_LOADING
};

GameState currentState = STATE_GAMEPLAY;

// Screen Dimensions (800 x 600)
int screenWidth = 800;
int screenHeight = 600;

// Hero Position & Size
int heroX = 160;
int heroY = 350;
int heroWidth = 65;
int heroHeight = 85;

// Hero Direction & Animation States
bool isMoving = false;
bool facingRight = true; // Tracks direction (true = R ; false = L)
int walkFrameIndex = 0;
int frameDelayCounter = 0;

// PNG Sprite Image Handles
int heroStandRightID = -1;
int heroStandLeftID = -1;
int heroWalkRightIDs[4] = { -1, -1, -1, -1 };
int heroWalkLeftIDs[4] = { -1, -1, -1, -1 };

int witchUnconsciousID = -1;
int witchStandingID = -1;

int heroPortraitID = -1;
int witchPortraitID = -1;
int spellBookID = -1;
int goblinHelperID = -1;

// Witch Screen Position & Dimensions
int witchX = 280;
int witchY = 295;
int witchWidth = 110;
int witchHeight = 110;

// Proximity Test State
bool isNearWitch = false;
bool isWitchRescued = false;
float currentDistance = 0.0f;
float proximityThreshold = 100.0f;

// Portal Position & Proximity
int portalX = 450;
int portalY = 320;
int portalWidth = 100;
int portalHeight = 130;
bool isNearPortal = false;
float portalProximityThreshold = 140.0f;

// Input Debounce
bool spacePressedLastFrame = false;

// Cutscene Dialogue Sequence
int currentDialogueIndex = 0;
const int TOTAL_DIALOGUES = 8;

const char* dialogueSpeakers[] = {
	"Witch:",
	"Hero:",
	"Witch:",
	"Hero:",
	"Witch:",
	"Hero:",
	"Witch:",
	"Hero:"
};

const char* dialogues[] = {
	"Thank you, dear, for saving me from that curse!",
	"Are you alright? What kind of dark power held you?",
	"Dark energies linger ahead... Take this Spell Book, it will guide you.",
	"A Spell Book? I'm so grateful for your guidance! I will use it to power up.",
	"I am also granting you Goblins to assist you!",
	"Goblins to help me? Thank you so much! With them, we can stop the dark forces!",
	"Step through the nearby portal to reach my home safely.",
	"Understood! Let's go there right away!."
};

// Distance calculations
bool checkProximityToWitch()
{
	float heroCenterX = heroX + (heroWidth / 2.0f);
	float heroCenterY = heroY + (heroHeight / 2.0f);
	float witchCenterX = witchX + (witchWidth / 2.0f);
	float witchCenterY = witchY + (witchHeight / 2.0f);

	float dx = heroCenterX - witchCenterX;
	float dy = heroCenterY - witchCenterY;

	currentDistance = (float)sqrt(dx * dx + dy * dy);
	return currentDistance <= proximityThreshold;
}

bool checkProximityToPortal()
{
	float heroCenterX = heroX + (heroWidth / 2.0f);
	float heroCenterY = heroY + (heroHeight / 2.0f);
	float portalCenterX = portalX + (portalWidth / 2.0f);
	float portalCenterY = portalY + (portalHeight / 2.0f);

	float dx = heroCenterX - portalCenterX;
	float dy = heroCenterY - portalCenterY;

	return (float)sqrt(dx * dx + dy * dy) <= portalProximityThreshold;
}

// Render dynamic objective inside the Task UI Box
void renderTaskBox()
{


	if (!isWitchRescued)
	{
		iSetColor(50, 30, 20);
		iText(225, 70, "SAVE THE WITCH", GLUT_BITMAP_HELVETICA_12);
	}
	else
	{
		iSetColor(50, 30, 20);
		iText(225, 70, "ENTER THE PORTAL TO PROCEED TO NEXT MAP", GLUT_BITMAP_HELVETICA_12);
	}
}

// Word wrapper helper for GLUT_BITMAP_HELVETICA_12
void iTextWrapped(int x, int y, const char* text, int maxWidth, int lineSpacing)
{
	char buffer[256];
	char currentLine[256] = "";
	char word[64];

	int textLen = strlen(text);
	int bufIdx = 0;
	int currentY = y;

	for (int i = 0; i <= textLen; i++)
	{
		if (text[i] == ' ' || text[i] == '\0')
		{
			word[bufIdx] = '\0';
			bufIdx = 0;

			char testLine[256];
			if (strlen(currentLine) == 0) {
				strcpy_s(testLine, word);
			}
			else {
				sprintf_s(testLine, "%s %s", currentLine, word);
			}

			// Estimate width (~7.5 pixels per character in Helvetica 12)
			int estimatedWidth = strlen(testLine) * 7.5;

			if (estimatedWidth > maxWidth && strlen(currentLine) > 0)
			{
				iText(x, currentY, currentLine, GLUT_BITMAP_HELVETICA_12);
				currentY -= lineSpacing;
				strcpy_s(currentLine, word);
			}
			else
			{
				strcpy_s(currentLine, testLine);
			}
		}
		else
		{
			if (bufIdx < 63)
			{
				word[bufIdx++] = text[i];
			}
		}
	}

	if (strlen(currentLine) > 0)
	{
		iText(x, currentY, currentLine, GLUT_BITMAP_HELVETICA_12);
	}
}

void renderCutscene()
{
	iShowBMP(0, 0, "images/saving_placeblur.bmp");

	if (heroPortraitID > 0)
	{
		iShowImage(610, 100, 160, 260, heroPortraitID);
	}

	if (witchPortraitID > 0)
	{
		iShowImage(30, 100, 160, 260, witchPortraitID);
	}

	int boxX = 200;
	int boxY = 120;
	int boxWidth = 400;
	int boxHeight = 180;

	iSetColor(15, 15, 25);
	iFilledRectangle(boxX, boxY, boxWidth, boxHeight);

	if (currentDialogueIndex % 2 == 0) {
		iSetColor(255, 215, 0);
	}
	else {
		iSetColor(70, 130, 180);
	}
	iRectangle(boxX, boxY, boxWidth, boxHeight);

	int itemY = boxY + boxHeight + 10;

	if (currentDialogueIndex == 2 || currentDialogueIndex == 3)
	{
		iSetColor(15, 15, 25);
		iFilledRectangle(boxX, itemY, 320, 45);
		iSetColor(255, 215, 0);
		iRectangle(boxX, itemY, 320, 45);

		if (spellBookID > 0)
		{
			iShowImage(boxX + 6, itemY + 4, 36, 36, spellBookID);
		}
		iSetColor(255, 255, 255);
		iText(boxX + 50, itemY + 15, "[ Item: Ancient Spellbook ]", GLUT_BITMAP_HELVETICA_12);
	}
	else if (currentDialogueIndex == 4 || currentDialogueIndex == 5)
	{
		iSetColor(15, 15, 25);
		iFilledRectangle(boxX, itemY, 330, 45);
		iSetColor(255, 215, 0);
		iRectangle(boxX, itemY, 330, 45);

		if (goblinHelperID > 0)
		{
			iShowImage(boxX + 6, itemY + 4, 36, 36, goblinHelperID);
		}
		iSetColor(255, 255, 255);
		iText(boxX + 50, itemY + 15, "[ Unlocked: Crafting Goblins ]", GLUT_BITMAP_HELVETICA_12);
	}

	// Render Speaker Name
	iSetColor(255, 255, 255);
	iText(boxX + 20, boxY + 145, (char*)dialogueSpeakers[currentDialogueIndex], GLUT_BITMAP_HELVETICA_18);

	// Wrapped Dialogue Text (360px max width inside 400px box)
	iTextWrapped(boxX + 20, boxY + 110, dialogues[currentDialogueIndex], 360, 20);

	// Prompt to advance
	iSetColor(180, 180, 180);
	iText(boxX + 180, boxY + 15, "Press [SPACE] to continue...", GLUT_BITMAP_HELVETICA_12);
}

void renderLoadingScreen()
{
	iSetColor(0, 0, 0);
	iFilledRectangle(0, 0, screenWidth, screenHeight);

	iSetColor(255, 255, 255);
	iText(screenWidth / 2 - 130, screenHeight / 2, "NEXT SCENE LOADING...", GLUT_BITMAP_TIMES_ROMAN_24);
}

void iDraw()
{
	iClear();

	if (currentState == STATE_GAMEPLAY)
	{
		// Gameplay Background
		iShowBMP(0, 0, "images/saving_place.bmp");

		// Task Box Text
		renderTaskBox();

		// Witch Rendering
		if (isWitchRescued)
		{
			if (witchStandingID > 0)
				iShowImage(witchX, witchY, witchWidth, witchHeight, witchStandingID);
		}
		else
		{
			if (witchUnconsciousID > 0)
				iShowImage(witchX, witchY, witchWidth, witchHeight, witchUnconsciousID);
		}

		// Safe Hero Rendering (Guards against invalid handles)
		if (facingRight)
		{
			if (isMoving && heroWalkRightIDs[walkFrameIndex] > 0)
			{
				iShowImage(heroX, heroY, heroWidth, heroHeight, heroWalkRightIDs[walkFrameIndex]);
			}
			else if (heroStandRightID > 0)
			{
				iShowImage(heroX, heroY, heroWidth, heroHeight, heroStandRightID);
			}
		}
		else // Facing Left
		{
			if (isMoving && heroWalkLeftIDs[walkFrameIndex] > 0)
			{
				iShowImage(heroX, heroY, heroWidth, heroHeight, heroWalkLeftIDs[walkFrameIndex]);
			}
			else if (heroStandLeftID > 0)
			{
				iShowImage(heroX, heroY, heroWidth, heroHeight, heroStandLeftID);
			}
			else if (heroStandRightID > 0) // Fallback to right stand if left stand missing
			{
				iShowImage(heroX, heroY, heroWidth, heroHeight, heroStandRightID);
			}
		}

		// Proximity Prompts
		if (isNearWitch && !isWitchRescued)
		{
			iSetColor(255, 105, 180);
			iFilledRectangle(witchX - 30, witchY + 120, 175, 35);

			iSetColor(255, 255, 255);
			iRectangle(witchX - 30, witchY + 120, 175, 35);

			iText(witchX - 25, witchY + 132, "PRESS 'X' TO SAVE WITCH", GLUT_BITMAP_HELVETICA_12);
		}

		if (isNearPortal && isWitchRescued)
		{
			iSetColor(128, 0, 128);
			iFilledRectangle(portalX - 40, portalY + 140, 180, 35);

			iSetColor(255, 255, 255);
			iRectangle(portalX - 40, portalY + 140, 180, 35);

			iText(portalX - 30, portalY + 152, "PRESS 'ENTER' TO GET IN", GLUT_BITMAP_HELVETICA_12);
		}
	}
	else if (currentState == STATE_CUTSCENE)
	{
		renderCutscene();
	}
	else if (currentState == STATE_LOADING)
	{
		renderLoadingScreen();
	}
}

void iMouseMove(int mx, int my) {}
void iPassiveMouseMove(int mx, int my) {}
void iMouse(int button, int state, int mx, int my) {}

void iKeyboard(unsigned char key)
{
	if (currentState == STATE_GAMEPLAY && (key == '\r' || key == '\n') && isNearPortal && isWitchRescued)
	{
		currentState = STATE_LOADING;
	}
}

void fixedUpdate()
{
	if (currentState == STATE_CUTSCENE)
	{
		if (isKeyPressed(' '))
		{
			if (!spacePressedLastFrame)
			{
				currentDialogueIndex++;
				if (currentDialogueIndex >= TOTAL_DIALOGUES)
				{
					currentState = STATE_GAMEPLAY;
					currentDialogueIndex = 0;
				}
				spacePressedLastFrame = true;
			}
		}
		else
		{
			spacePressedLastFrame = false;
		}
	}
	else if (currentState == STATE_GAMEPLAY)
	{
		isMoving = false;
		int moveSpeed = 2;

		isNearWitch = checkProximityToWitch();
		isNearPortal = checkProximityToPortal();

		if ((isKeyPressed('\r') || isKeyPressed('\n')) && isNearPortal && isWitchRescued)
		{
			currentState = STATE_LOADING;
			return;
		}

		if ((isKeyPressed('x') || isKeyPressed('X')) && isNearWitch && !isWitchRescued)
		{
			isWitchRescued = true;
			currentState = STATE_CUTSCENE;
		}

		// Movement Controls & Facing Direction update
		if (isKeyPressed('w') || isSpecialKeyPressed(GLUT_KEY_UP)) {
			heroY += moveSpeed;
			isMoving = true;
		}
		if (isKeyPressed('s') || isSpecialKeyPressed(GLUT_KEY_DOWN)) {
			heroY -= moveSpeed;
			isMoving = true;
		}
		if (isKeyPressed('a') || isSpecialKeyPressed(GLUT_KEY_LEFT)) {
			heroX -= moveSpeed;
			isMoving = true;
			if (facingRight) {
				walkFrameIndex = 0;
				frameDelayCounter = 0;
			}
			facingRight = false; // Turn Left
		}
		if (isKeyPressed('d') || isSpecialKeyPressed(GLUT_KEY_RIGHT)) {
			heroX += moveSpeed;
			isMoving = true;
			if (!facingRight) {
				walkFrameIndex = 0;
				frameDelayCounter = 0;
			}
			facingRight = true; // Turn Right
		}

		// Screen Boundaries
		if (heroX < 0) heroX = 0;
		if (heroY < 0) heroY = 0;
		if (heroX > screenWidth - heroWidth) heroX = screenWidth - heroWidth;
		if (heroY > screenHeight - heroHeight) heroY = screenHeight - heroHeight;

		// Frame animation counter
		if (isMoving)
		{
			frameDelayCounter++;
			if (frameDelayCounter >= 8)
			{
				walkFrameIndex = (walkFrameIndex + 1) % 4;
				frameDelayCounter = 0;
			}
		}
		else
		{
			walkFrameIndex = 0;
			frameDelayCounter = 0;
		}
	}
}

int main()
{
	iInitialize(screenWidth, screenHeight, "Save the Witch");

	// Hero PNGs - Stand
	heroStandRightID = iLoadImage("images/alchemist_standR.png");
	heroStandLeftID = iLoadImage("images/alchemist_standL.png");

	// Hero PNGs - Right Walk Animation
	heroWalkRightIDs[0] = iLoadImage("images/alchemist_walkR1.png");
	heroWalkRightIDs[1] = iLoadImage("images/alchemist_walkR2.png");
	heroWalkRightIDs[2] = iLoadImage("images/alchemist_walkR3.png");
	heroWalkRightIDs[3] = iLoadImage("images/alchemist_walkR4.png");

	// Hero PNGs - Left Walk Animation
	heroWalkLeftIDs[0] = iLoadImage("images/alchemist_walkL1.png");
	heroWalkLeftIDs[1] = iLoadImage("images/alchemist_walkL2.png");
	heroWalkLeftIDs[2] = iLoadImage("images/alchemist_walkL3.png");
	heroWalkLeftIDs[3] = iLoadImage("images/alchemist_walkL4.png");

	// Witch PNGs
	witchUnconsciousID = iLoadImage("images/unconscious_witch.png");
	witchStandingID = iLoadImage("images/right_looking_witch.png");

	// Cutscene PNGs
	heroPortraitID = iLoadImage("images/alchemist_portrait.png");
	witchPortraitID = iLoadImage("images/right_looking_witch.png");
	spellBookID = iLoadImage("images/witch_book.png");
	goblinHelperID = iLoadImage("images/ranger_walk1.png");

	// Timer loop
	iSetTimer(16, fixedUpdate);

	iStart();
	return 0;
}