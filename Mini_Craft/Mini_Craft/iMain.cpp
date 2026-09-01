#include "iGraphics.h"

//Game States
enum GameState {
	STATE_GAMEPLAY, //player movement
	STATE_CUTSCENE, //dialogue between hero and witch
	STATE_LOADING //next map load
};

GameState currentState = STATE_GAMEPLAY;

//screen dimension
int screenWidth = 800;
int screenHeight = 600;

//hero coordinates
int heroX = 160;
int heroY = 350;
int heroWidth = 75;
int heroHeight = 95;

// Direction & Animation state
bool isMoving = false;
bool facingRight = true; // true = facing Right, false = facing Left
int walkFrameIndex = 0; //walking frame 
int frameDelayCounter = 0; //speed

//IDs
int heroStandRightID = -1;
int heroStandLeftID = -1;
int heroWalkRightIDs[4] = { -1, -1, -1, -1 };
int heroWalkLeftIDs[4] = { -1, -1, -1, -1 };

int heroPortraitID = -1;
int witchPortraitID = -1;
int spellBookID = -1;
int goblinHelperID = -1;


int witchUnconsciousID = -1;
int witchStandingID = -1;

//witch coordinates
int witchX = 280;
int witchY = 295;
int witchWidth = 110;
int witchHeight = 110;

//Witch proximity
bool isNearWitch = false;
bool isWitchRescued = false;
float currentDistance = 0.0f;
float proximityThreshold = 100.0f;

//Portal proximity
int portalX = 450;
int portalY = 320;
int portalWidth = 100;
int portalHeight = 130;
bool isNearPortal = false;
float portalProximityThreshold = 140.0f;

//input
bool spacePressedLastFrame = false;

//Cutscene Dialogue
int currentDialogueIndex = 0;
const int TOTAL_DIALOGUES = 10;

const char* dialogueSpeakers[] = {
	"Witch:",
	"Hero:",
	"Witch:",
	"Witch:",
	"Hero:",
	"Witch:",
	"Witch:",
	"Hero:",
	"Witch:",
	"Hero:"
};

const char* dialogues[] = {
	"...It's over. The chains, the whispers in my skull - gone. You have no idea what you just pulled me out of.",
	"What was that? I've never seen a curse hold someone like that.",
	"Old magic. Older than this village, older than me. It doesn't matter now - what matters is I'm out. She presses a worn, leather-bound book into your hands. It's warmer than it should be.",
	"This Spell Book has outlived three owners before me. It chooses who it works for - and right now, it's chosen you. Don't waste that.",
	"I won't. I'll learn everything it can teach me.",
	"You'll need more than pages against what's coming. The forest's already stirring - I can feel it. So I'm giving you what's left of my strength.",
	"I'm giving you Goblins. Rough around the edges, but they bleed for the people who freed me. They'll bleed for you too, now.",
	"Then whatever's out there... it's going to have a fight on its hands.",
	"Good. Hold onto that. There's a portal - it'll take you somewhere safe to catch your breath. Go now, while the curse's grip is still broken.",
	"Then let's not waste time and get in!"
};

//Proximity calculation
// Distance between the centers of two axis-aligned boxes.
// Both proximity checks used to repeat this exact math -- pulled out
// once so there's a single place to fix if the formula ever changes.
float getCenterDistance(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2){
	float cx1 = x1 + (w1 / 2.0f);
	float cy1 = y1 + (h1 / 2.0f);
	float cx2 = x2 + (w2 / 2.0f);
	float cy2 = y2 + (h2 / 2.0f);

	float dx = cx1 - cx2;
	float dy = cy1 - cy2;

	return (float)sqrt(dx * dx + dy * dy);
}

bool checkProximityToWitch(){
	currentDistance = getCenterDistance(heroX, heroY, heroWidth, heroHeight, witchX, witchY, witchWidth, witchHeight);

	return currentDistance <= proximityThreshold;
}

bool checkProximityToPortal(){
	float distance = getCenterDistance(heroX, heroY, heroWidth, heroHeight, portalX, portalY, portalWidth, portalHeight);

	return distance <= portalProximityThreshold;
}

// Render dynamic objective inside the Task UI Box
void renderTaskBox(){
	iSetColor(50, 30, 20);

	if (!isWitchRescued){

		iText(225, 70, "SAVE THE WITCH", GLUT_BITMAP_HELVETICA_12);
	}

	else{

		iText(225, 70, "ENTER THE PORTAL TO PROCEED TO NEXT MAP", GLUT_BITMAP_HELVETICA_12);
	}
}

// Word wrapper helper for GLUT_BITMAP_HELVETICA_12
void iTextWrapped(int x, int y, const char* text, int maxWidth, int lineSpacing){

	char buffer[256];
	char currentLine[256] = "";
	char word[64];

	int textLen = strlen(text);
	int bufIdx = 0;
	int currentY = y;

	for (int i = 0; i <= textLen; i++){

		if (text[i] == ' ' || text[i] == '\0'){

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

			if (estimatedWidth > maxWidth && strlen(currentLine) > 0){

				iText(x, currentY, currentLine, GLUT_BITMAP_HELVETICA_12);
				currentY -= lineSpacing;
				strcpy_s(currentLine, word);
			}
			else{

				strcpy_s(currentLine, testLine);
			}
		}
		else{
			if (bufIdx < 63){


				word[bufIdx++] = text[i];
			}
		}
	}

	if (strlen(currentLine) > 0){
		iText(x, currentY, currentLine, GLUT_BITMAP_HELVETICA_12);
	}
}

void renderCutscene(){
	iShowBMP(0, 0, "images/saving_placeblur.bmp");

	if (heroPortraitID > 0){
		iShowImage(610, 80, 160, 260, heroPortraitID);
	}

	if (witchPortraitID > 0){
		iShowImage(30, 100, 160, 260, witchPortraitID);
	}

	int boxX = 200;
	int boxY = 120;
	int boxWidth = 390;
	int boxHeight = 180;

	//Dialogue Box Color
	iSetColor(15, 15, 25);
	iFilledRectangle(boxX, boxY, boxWidth, boxHeight);

	if (strcmp(dialogueSpeakers[currentDialogueIndex], "Witch:") == 0) {
		iSetColor(255, 215, 0);   //Witch: gold outline
	}
	else {
		iSetColor(70, 130, 180);  //Hero: Blue outline
	}
	iRectangle(boxX, boxY, boxWidth, boxHeight);

	// Render Speaker Name
	iSetColor(255, 255, 255);
	iText(boxX + 20, boxY + 145, (char*)dialogueSpeakers[currentDialogueIndex], GLUT_BITMAP_HELVETICA_18);

	// Wrapped Dialogue Text
	iTextWrapped(boxX + 20, boxY + 110, dialogues[currentDialogueIndex], 350, 20);


	iSetColor(180, 180, 180);
	iText(boxX + 170, boxY + 15, "Press [SPACE] to continue...", GLUT_BITMAP_HELVETICA_12);

	// Item unlocked msg
	int imgSize = 100;

	// Position image directly above the main box
	int itemX = boxX + 30;
	int itemY = boxY + boxHeight + 15;

	// Position text badge right next to the image
	int tagX = itemX + imgSize + 15;
	int tagY = itemY + (imgSize / 2) - 20;
	int tagW = 180;
	int tagH = 40;

	if (currentDialogueIndex == 2 || currentDialogueIndex == 3){
		// Render Spell Book Image above box
		if (spellBookID > 0){
			iShowImage(itemX, itemY, 75, 80, spellBookID);
		}

		// Text Tag Beside Book
		iSetColor(15, 15, 25);
		iFilledRectangle(tagX, tagY, tagW, tagH);
		iSetColor(255, 255, 255);
		iRectangle(tagX, tagY, tagW, tagH); //white outline

		iSetColor(255, 255, 255);
		iText(tagX + 15, tagY + 14, "Spell Book Unlocked", GLUT_BITMAP_HELVETICA_12);
	}
	else if (currentDialogueIndex == 6 || currentDialogueIndex == 7){
		// Render Goblin Image above box
		if (goblinHelperID > 0)
		{
			iShowImage(10, 30, 200 ,200, goblinHelperID);
		}

		// Text Tag Beside Goblin
		iSetColor(15, 15, 25);
		iFilledRectangle(tagX, tagY, tagW, tagH);
		iSetColor(255, 255, 255);
		iRectangle(tagX, tagY, tagW, tagH); //white outline


		iSetColor(255, 255, 255);
		iText(tagX + 15, tagY + 14, "Goblin Unlocked", GLUT_BITMAP_HELVETICA_12);
	}
}

void renderLoadingScreen()
{
	iSetColor(0, 0, 0);
	iFilledRectangle(0, 0, screenWidth, screenHeight);

	iSetColor(255, 255, 255);
	iText(screenWidth / 2 - 130, screenHeight / 2, "NEXT SCENE LOADING...", GLUT_BITMAP_TIMES_ROMAN_24);
}

// Hero Sprite Select
// Picks the sprite ID that should be drawn this frame based on facing
// direction and whether the hero is walking. Replaces the nested
// if/else block that used to sit directly inside iDraw().

int getCurrentHeroSpriteID(){
	if (facingRight){

		if (isMoving && heroWalkRightIDs[walkFrameIndex] > 0)
			return heroWalkRightIDs[walkFrameIndex];

		return heroStandRightID;
	}
	else{

		if (isMoving && heroWalkLeftIDs[walkFrameIndex] > 0)
			return heroWalkLeftIDs[walkFrameIndex];

		if (heroStandLeftID > 0)
			return heroStandLeftID;

		return heroStandRightID; // fallback if a left-facing stand sprite is missing
	}
}

void iDraw(){

	iClear();

	if (currentState == STATE_GAMEPLAY){

		// Gameplay Background
		iShowBMP(0, 0, "images/saving_place.bmp");

		// Task Box Text
		renderTaskBox();

		// Witch Rendering
		if (isWitchRescued){

			if (witchStandingID > 0)
				iShowImage(witchX, witchY, witchWidth, witchHeight, witchStandingID);
		}
		else{

			if (witchUnconsciousID > 0)
				iShowImage(witchX, witchY, witchWidth, witchHeight, witchUnconsciousID);
		}

		// Hero Rendering
		int heroSpriteID = getCurrentHeroSpriteID();
		if (heroSpriteID > 0){

			iShowImage(heroX, heroY, heroWidth, heroHeight, heroSpriteID);
		}

		// Proximity Prompts
		if (isNearWitch && !isWitchRescued){

			iSetColor(15, 15, 25);
			iFilledRectangle(witchX - 30, witchY + 120, 175, 35);

			iSetColor(255, 255, 255);
			iRectangle(witchX - 30, witchY + 120, 175, 35);

			iText(witchX - 25, witchY + 132, "PRESS 'X' TO SAVE WITCH", GLUT_BITMAP_HELVETICA_12);
		}

		if (isNearPortal && isWitchRescued){

			iSetColor(15, 15, 25);
			iFilledRectangle(portalX - 40, portalY + 140, 180, 35);

			iSetColor(255, 255, 255);
			iRectangle(portalX - 40, portalY + 140, 180, 35);

			iText(portalX - 30, portalY + 152, "PRESS 'ENTER' TO GET IN", GLUT_BITMAP_HELVETICA_12);
		}
	}
	else if (currentState == STATE_CUTSCENE){

		renderCutscene();
	}
	else if (currentState == STATE_LOADING){

		renderLoadingScreen();
	}
}

void iMouseMove(int mx, int my) {}
void iPassiveMouseMove(int mx, int my) {}
void iMouse(int button, int state, int mx, int my) {}

void iKeyboard(unsigned char key){

	if (currentState == STATE_GAMEPLAY && (key == '\r' || key == '\n') && isNearPortal && isWitchRescued)
	{
		currentState = STATE_LOADING;
	}
}

void fixedUpdate(){
	if (currentState == STATE_CUTSCENE){

		if (isKeyPressed(' ')){

			if (!spacePressedLastFrame){

				currentDialogueIndex++;

				if (currentDialogueIndex >= TOTAL_DIALOGUES){
					currentState = STATE_GAMEPLAY;
					currentDialogueIndex = 0;
				}
				spacePressedLastFrame = true;
			}
		}
		else{

			spacePressedLastFrame = false;
		}
	}
	else if (currentState == STATE_GAMEPLAY){

		isMoving = false;
		int moveSpeed = 2;

		isNearWitch = checkProximityToWitch();
		isNearPortal = checkProximityToPortal();

		if ((isKeyPressed('\r') || isKeyPressed('\n')) && isNearPortal && isWitchRescued){
			currentState = STATE_LOADING;
			return;
		}

		if ((isKeyPressed('x') || isKeyPressed('X')) && isNearWitch && !isWitchRescued){

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
		if (heroX > screenWidth - heroWidth)
			heroX = screenWidth - heroWidth;
		if (heroY > screenHeight - heroHeight)
			heroY = screenHeight - heroHeight;

		// Frame animation counter
		if (isMoving){

			frameDelayCounter++;
			if (frameDelayCounter >= 8){

				walkFrameIndex = (walkFrameIndex + 1) % 4;
				frameDelayCounter = 0;
			}
		}
		else{

			walkFrameIndex = 0;
			frameDelayCounter = 0;
		}
	}
}

int main(){

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
	goblinHelperID = iLoadImage("images/goblin_trio.png");

	// Timer loop
	iSetTimer(16, fixedUpdate);

	iStart();
	return 0;
}