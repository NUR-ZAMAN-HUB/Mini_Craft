#define _CRT_SECURE_NO_WARNINGS   // silence sprintf/strcpy "unsafe" warnings (Fighters.hpp/HomeBase.hpp use plain sprintf)

#include <iostream>
#include "iGraphics.h"
#include "Menu.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>
#include "Player.hpp"
#include "HomeBase.hpp"   // Home Base scene (fighters + resource gathering) - see Menu.h's HOMEBASE state
#include "Battle.hpp"     // Battle scene (Home Base Attack button) - see Menu.h's BATTLE state

// mciSendString() (background music) lives in <mmsystem.h> / winmm.lib. iGraphics.h already
// includes <windows.h> but not <mmsystem.h>, and nothing in the project ever explicitly
// linked winmm.lib - both are pulled in here explicitly so mciSendString always compiles
// and links, regardless of project settings.
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

GameState currentState = GameState::MENU;


Button startBtn = { 342, 239, 457, 271};
Button levelBtn = { 307, 198, 422, 230};
Button settingsBtn = { 301, 149, 416, 191}; //  315 163
Button exitBtn = { 361, 110, 476, 142};// 375 110

// Shared "back to menu" button, reused on the LEVEL and SETTINGS screens
Button backBtn = { 30, 15, 150, 55};

// LEVEL_SELECT screen
Button resetLevelBtn = { 275, 220, 464, 263};

// SETTINGS screen (difficulty showcase)
Button easyBtn = { 100, 300, 260, 345};
Button mediumBtn = { 320, 300, 480, 345};
Button hardBtn = { 540, 300, 700, 345};

// CHARACTER_SELECT screen
// "Next >" cycles cards. Clicking the card picture selects it, which spawns
// "Confirm" underneath it - clicking Confirm is what actually moves on.
Button nextCharBtn = { 650, 15, 770, 55};
Button confirmCharBtn = { 345, 15, 456, 58};

int currentLevel = 1;
int difficulty = 0;   // 0 = Easy, 1 = Medium, 2 = Hard

int backgroundImg1;
int backgroundImg2;
int selectCharacterImg;   // "select character.jpg" banner shown at the top of CHARACTER_SELECT

// New button-art images (LEVEL_SELECT / SETTINGS screens)
unsigned int currentLevelImg;   // replaces the "Current Level:" text label (the number itself stays as text)
unsigned int resetWarningImg;   // replaces the red "resetting will take you back to Level 1" text
unsigned int difficultyImg;     // replaces the "Difficulty" text header

// Music on/off toggle (SETTINGS screen, bottom-right corner)
bool audioOn = true;   // starts ON, per spec
unsigned int musicOnImg;
unsigned int musicOffImg;
Button musicToggleBtn = { 700, 30, 750, 80, "", 0 };   // imgId is set every frame in DrawSettings() based on audioOn

// Sound on/off toggle - sits directly beside the music toggle. There's no
// separate sound-effects system in the project yet (the only other audio
// asset, Audios/gameover.mp3, is opened in main() but nothing ever plays
// it), so soundOn doesn't gate anything real yet - it's wired exactly like
// audioOn so it's ready the moment there's something to mute.
bool soundOn = true;   // starts ON, same as music
unsigned int soundOnImg;
unsigned int soundOffImg;
Button soundToggleBtn = { 650, 30, 696, 80, "", 0 };   // sits left of musicToggleBtn with a small gap

// Card images are 842x576. On the 800x600 screen we scale that down to
// 614x420 (same aspect ratio) so the banner still fits above and the
// Confirm/Next buttons still fit below.
CharacterBox charBoxes[3] = {
	{ 93, 70, 707, 490, -1 },
	{ 93, 70, 707, 490, -1 },
	{ 93, 70, 707, 490, -1 }
};

int characterNumber = -1;    // 0 = Alchemist, 1 = Ranger, 2 = Guardian (see CharacterId in Menu.h). Set when the player clicks a card picture, confirmed as the hero used in gameplay.
int currentCardIndex = 0;    // which of the 3 cards is currently displayed

// ---------------------------------------------------------------------------
// LOADING screen transition helper
// Both the menu flow and the gameplay flow route through GameState::LOADING
// (e.g. after confirming a character, or after walking through the portal).
// pendingState says where LOADING should hand off to once its timer runs out.
// ---------------------------------------------------------------------------
GameState pendingState = GameState::MENU;
int loadingTimer = 0;
const int LOADING_DURATION = 60;   // ~1 second (fixedUpdate ticks roughly every 16ms)

// Which state we were leaving when EnterLoading() was called - lets the LOADING->HOMEBASE
// handoff below tell "coming back from the Battle arena" (resume the paused night timer)
// apart from "fresh arrival at Home Base" (start the night timer from scratch).
GameState g_loadingFromState = GameState::MENU;

// Autosave while in Home Base/Battle (see SaveGame.hpp) - safety net for closing the window
// with the X button / Alt+F4, which never goes through the Exit button's SaveGame() call.
const unsigned long AUTOSAVE_INTERVAL_MS = 3000;
unsigned long g_lastAutoSaveTick = 0;

void EnterLoading(GameState target)
{
	g_loadingFromState = currentState;   // remember what we're leaving BEFORE it flips to LOADING
	currentState = GameState::LOADING;
	pendingState = target;
	loadingTimer = LOADING_DURATION;
}

// ---------------------------------------------------------------------------
// Gameplay state ("Save the Witch" map)
// ---------------------------------------------------------------------------

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

// Dynamic Hero file prefixes for loading character specific sprites (matches CharacterId order)
const char* heroFilePrefix[3] = { "alchemist", "ranger", "guardian" };

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

// Clickable "SKIP" in the dialogue box's top-right corner - jumps straight
// to GAMEPLAY, same as if the last line of dialogue had just been dismissed.
Button skipDialogueBtn = { 495, 270, 580, 295, "SKIP", 0 };

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
	"Old magic. Older than this village, older than me. It doesn't matter now - what matters is I'm out.",
	"This Spell Book has outlived three owners before me. It chooses who it works for - and right now, it's chosen you. Don't waste that.",
	"I won't. I'll learn everything it can teach me.",
	"You'll need more than pages against what's coming. The forest's already stirring - I can feel it. So I'm giving you what's left of my strength.",
	"I'm giving you Goblins. Rough around the edges, but they bleed for the people who freed me. They'll bleed for you too, now.",
	"Then whatever's out there... it's going to have a fight on its hands.",
	"Good. Hold onto that. There's a portal - it'll take you somewhere safe to catch your breath and prepare. Go now, while the curse's grip is still broken.",
	"Then let's not waste time and get in!"
};

// Loads the sprites/portrait for whichever hero was confirmed on CHARACTER_SELECT (characterNumber).
// Called once, right when the player hits Confirm, so the load happens while LOADING is shown.
void loadCharacterAssets()
{
	if (characterNumber < 0 || characterNumber >= 3) return;

	const char* prefix = heroFilePrefix[characterNumber];
	char pathBuffer[128];

	// Hero PNGs - Stand
	sprintf_s(pathBuffer, "Images/%s_standR.png", prefix);
	heroStandRightID = iLoadImage(pathBuffer);

	sprintf_s(pathBuffer, "Images/%s_standL.png", prefix);
	heroStandLeftID = iLoadImage(pathBuffer);

	// Hero PNGs - Right Walk Animation
	for (int i = 0; i < 4; i++) {
		sprintf_s(pathBuffer, "Images/%s_walkR%d.png", prefix, i + 1);
		heroWalkRightIDs[i] = iLoadImage(pathBuffer);
	}

	// Hero PNGs - Left Walk Animation
	for (int i = 0; i < 4; i++) {
		sprintf_s(pathBuffer, "Images/%s_walkL%d.png", prefix, i + 1);
		heroWalkLeftIDs[i] = iLoadImage(pathBuffer);
	}

	// Hero Cutscene Portrait
	sprintf_s(pathBuffer, "Images/%s_portrait.png", prefix);
	heroPortraitID = iLoadImage(pathBuffer);
}

// Puts the "Save the Witch" map back into its starting condition. Called whenever a fresh
// playthrough begins (Confirm on CHARACTER_SELECT), so replaying after a reset/level change works.
void ResetGameplayState()
{
	heroX = 160;
	heroY = 350;
	isMoving = false;
	facingRight = true;
	walkFrameIndex = 0;
	frameDelayCounter = 0;

	isNearWitch = false;
	isWitchRescued = false;
	isNearPortal = false;

	spacePressedLastFrame = false;
	currentDialogueIndex = 0;
}

//Proximity calculation
// Distance between the centers of two axis-aligned boxes.
float getCenterDistance(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2)
{
	float cx1 = x1 + (w1 / 2.0f);
	float cy1 = y1 + (h1 / 2.0f);
	float cx2 = x2 + (w2 / 2.0f);
	float cy2 = y2 + (h2 / 2.0f);

	float dx = cx1 - cx2;
	float dy = cy1 - cy2;

	return (float)sqrt(dx * dx + dy * dy);
}

bool checkProximityToWitch()
{
	currentDistance = getCenterDistance(heroX, heroY, heroWidth, heroHeight, witchX, witchY, witchWidth, witchHeight);
	return currentDistance <= proximityThreshold;
}

bool checkProximityToPortal()
{
	float distance = getCenterDistance(heroX, heroY, heroWidth, heroHeight, portalX, portalY, portalWidth, portalHeight);
	return distance <= portalProximityThreshold;
}

// Render dynamic objective inside the Task UI Box (the box art itself lives in the saving_place.bmp background)
void renderTaskBox()
{
	iSetColor(50, 30, 20);

	if (!isWitchRescued)
		iText(225, 70, const_cast<char*>("SAVE THE WITCH"), GLUT_BITMAP_HELVETICA_12);
	else
		iText(225, 70, const_cast<char*>("ENTER THE PORTAL TO PROCEED TO NEXT MAP"), GLUT_BITMAP_HELVETICA_12);
}


void iTextWrapped(int x, int y, const char* text, int maxWidth, int lineSpacing)
{
	
	char currentLine[256] = "";
	char word[64];

	int textLen = (int)strlen(text);
	int bufIdx = 0;
	int currentY = y;

	for (int i = 0; i <= textLen; i++) {

		if (text[i] == ' ' || text[i] == '\0') {

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
			int estimatedWidth = (int)(strlen(testLine) * 7.5);

			if (estimatedWidth > maxWidth && strlen(currentLine) > 0) {

				iText(x, currentY, currentLine, GLUT_BITMAP_HELVETICA_12);
				currentY -= lineSpacing;
				strcpy_s(currentLine, word);
			}
			else {

				strcpy_s(currentLine, testLine);
			}
		}
		else {
			if (bufIdx < 63) {

				word[bufIdx++] = text[i];
			}
		}
	}

	if (strlen(currentLine) > 0) {
		iText(x, currentY, currentLine, GLUT_BITMAP_HELVETICA_12);
	}
}

void renderCutscene()
{
	iShowBMP(0, 0, const_cast<char*>("Images/saving_placeblur.bmp"));

	if (heroPortraitID > 0) {
		iShowImage(610, 80, 160, 260, heroPortraitID);
	}

	if (witchPortraitID > 0) {
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
	iText(boxX + 170, boxY + 15, const_cast<char*>("Press [SPACE] to continue..."), GLUT_BITMAP_HELVETICA_12);

	// SKIP - top-right corner of the dialogue box, cuts straight to GAMEPLAY
	iSetColor(70, 70, 80);
	iFilledRectangle(skipDialogueBtn.x1, skipDialogueBtn.y1,
		skipDialogueBtn.x2 - skipDialogueBtn.x1, skipDialogueBtn.y2 - skipDialogueBtn.y1);
	iSetColor(230, 230, 230);
	iRectangle(skipDialogueBtn.x1, skipDialogueBtn.y1,
		skipDialogueBtn.x2 - skipDialogueBtn.x1, skipDialogueBtn.y2 - skipDialogueBtn.y1);
	iText(skipDialogueBtn.x1 + 12, (skipDialogueBtn.y1 + skipDialogueBtn.y2) / 2 - 5,
		const_cast<char*>("SKIP"), GLUT_BITMAP_HELVETICA_18);

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

	if (currentDialogueIndex == 2 || currentDialogueIndex == 3) {
		// Render Spell Book Image above box
		if (spellBookID > 0) {
			iShowImage(itemX, itemY, 75, 80, spellBookID);
		}

		// Text Tag Beside Book
		iSetColor(15, 15, 25);
		iFilledRectangle(tagX, tagY, tagW, tagH);
		iSetColor(255, 255, 255);
		iRectangle(tagX, tagY, tagW, tagH); //white outline

		iSetColor(255, 255, 255);
		iText(tagX + 15, tagY + 14, const_cast<char*>("Spell Book Unlocked"), GLUT_BITMAP_HELVETICA_12);
	}
	else if (currentDialogueIndex == 6 || currentDialogueIndex == 7) {
		// Render Goblin Image above box
		if (goblinHelperID > 0) {
			iShowImage(10, 30, 200, 200, goblinHelperID);
		}

		// Text Tag Beside Goblin
		iSetColor(15, 15, 25);
		iFilledRectangle(tagX, tagY, tagW, tagH);
		iSetColor(255, 255, 255);
		iRectangle(tagX, tagY, tagW, tagH); //white outline

		iSetColor(255, 255, 255);
		iText(tagX + 15, tagY + 14, const_cast<char*>("Goblin Unlocked"), GLUT_BITMAP_HELVETICA_12);
	}
}

// Picks the sprite ID that should be drawn this frame based on facing direction and whether the hero is walking.
int getCurrentHeroSpriteID()
{
	if (facingRight) {

		if (isMoving && heroWalkRightIDs[walkFrameIndex] > 0)
			return heroWalkRightIDs[walkFrameIndex];

		return heroStandRightID;
	}
	else {

		if (isMoving && heroWalkLeftIDs[walkFrameIndex] > 0)
			return heroWalkLeftIDs[walkFrameIndex];

		// FIX: Return left standing sprite if valid; otherwise fallback to right standing sprite
		if (heroStandLeftID > 0)
			return heroStandLeftID;

		return heroStandRightID; 
	}
}

void DrawGameplay()
{
	// Gameplay Background
	iShowBMP(0, 0, const_cast<char*>("Images/saving_place.bmp"));

	// Task Box Text
	renderTaskBox();

	// Witch Rendering
	if (isWitchRescued) {
		if (witchStandingID > 0)
			iShowImage(witchX, witchY, witchWidth, witchHeight, witchStandingID);
	}
	else {
		if (witchUnconsciousID > 0)
			iShowImage(witchX, witchY, witchWidth, witchHeight, witchUnconsciousID);
	}

	// Hero Rendering
	int heroSpriteID = getCurrentHeroSpriteID();
	if (heroSpriteID > 0) {
		iShowImage(heroX, heroY, heroWidth, heroHeight, heroSpriteID);
	}

	// Proximity Prompts
	if (isNearWitch && !isWitchRescued) {
		iSetColor(15, 15, 25);
		iFilledRectangle(witchX - 30, witchY + 120, 175, 35);

		iSetColor(255, 255, 255);
		iRectangle(witchX - 30, witchY + 120, 175, 35);

		iText(witchX - 25, witchY + 132, const_cast<char*>("PRESS 'X' TO SAVE WITCH"), GLUT_BITMAP_HELVETICA_12);
	}

	if (isNearPortal && isWitchRescued) {
		iSetColor(15, 15, 25);
		iFilledRectangle(portalX - 40, portalY + 140, 180, 35);

		iSetColor(255, 255, 255);
		iRectangle(portalX - 40, portalY + 140, 180, 35);

		iText(portalX - 30, portalY + 152, const_cast<char*>("PRESS 'ENTER' TO GET IN"), GLUT_BITMAP_HELVETICA_12);
	}

	// Movement controls hint - top-right corner, GAMEPLAY only.
	// Same dark-box + white-border style as the "PRESS 'X' TO SAVE WITCH" prompts.
	const int hintX = 585, hintY = 530, hintW = 200, hintH = 55;

	iSetColor(15, 15, 25);
	iFilledRectangle(hintX, hintY, hintW, hintH);

	iSetColor(255, 255, 255);
	iRectangle(hintX, hintY, hintW, hintH);

	iText(hintX + 12, hintY + 35, const_cast<char*>("MOVEMENT"), GLUT_BITMAP_HELVETICA_12);
	iText(hintX + 12, hintY + 15, const_cast<char*>("W A S D  or  Arrow Keys"), GLUT_BITMAP_HELVETICA_12);
}

// ---------------------------------------------------------------------------
// Menu screens (unchanged from the menu build)
// ---------------------------------------------------------------------------

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
	// Every Button that goes through here now has a real image loaded in
	// main() (start/level/settings/exit/back/easy/medium/hard/next/confirm/
	// reset/music/sound all do) - the old manual rectangle+text fallback for
	// image-less buttons has been removed since it never runs anymore.
	iShowImage(b.x1, b.y1, b.x2 - b.x1, b.y2 - b.y1, b.imgId);

	if (highlighted)
	{
		// "highlighted" (the currently-selected option, e.g. on SETTINGS) still
		// needs to be visible on top of the image - a yellow outline does that
		// without covering the button art itself.
		iSetColor(255, 255, 0);
		iRectangle(b.x1 - 1, b.y1 - 1, b.x2 - b.x1 + 2, b.y2 - b.y1 + 2);
		iRectangle(b.x1, b.y1, b.x2 - b.x1, b.y2 - b.y1);
	}
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
	iShowImage(150 +40, 500 + 20, 500 -40 , 81-20, selectCharacterImg);
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
	// Level indicator - "Current Level:" label is now the image; the number
	// itself stays as text (still driven by currentLevel, untouched logic-wise),
	// recolored black and positioned in the blank space inside the box, right
	// after the colon (the box image reserves that space empty on purpose).
	iShowImage(230, 405, 280, 44, currentLevelImg);

	char levelNumText[16];
	sprintf_s(levelNumText, "%d", currentLevel);
	iSetColor(0, 0, 0);
	iText(440, 417, levelNumText, GLUT_BITMAP_TIMES_ROMAN_24);

	DrawButton(resetLevelBtn);

	// Warning shown under the reset button
	iShowImage(210, 75, 380, 136, resetWarningImg);

	DrawButton(backBtn);
}

void DrawSettings()
{
	iShowImage(285, 415, 230, 34, difficultyImg);

	// Showcase only - highlights whichever difficulty is currently selected
	DrawButton(easyBtn, difficulty == 0);
	DrawButton(mediumBtn, difficulty == 1);
	DrawButton(hardBtn, difficulty == 2);

	DrawButton(backBtn);

	// Music on/off toggle - bottom-right corner. Starts ON (audioOn = true,
	// set in main()); clicking it in iMouse() flips audioOn and swaps which
	// texture this points at, and starts/stops "bgsong" to match.
	musicToggleBtn.imgId = audioOn ? musicOnImg : musicOffImg;
	DrawButton(musicToggleBtn);

	// Sound on/off toggle - sits beside the music toggle, same pattern.
	// See the comment on soundOn above for why it doesn't control anything yet.
	soundToggleBtn.imgId = soundOn ? soundOnImg : soundOffImg;
	DrawButton(soundToggleBtn);
}

void iDraw()
{
	iClear();

	// The MENU screen keeps the original background; CHARACTER_SELECT/LEVEL_SELECT/SETTINGS
	// use Game_background2.jpg. LOADING, GAMEPLAY, CUTSCENE and HOMEBASE all draw their own
	// full-screen art (DrawLoading fills black; the rest draw their own bitmaps/textures),
	// so they skip both menu background images.
	if (currentState == GameState::MENU)
		iShowImage(0, 0, 800, 600, backgroundImg1);
	else if (currentState != GameState::LOADING && currentState != GameState::GAMEPLAY &&
		currentState != GameState::CUTSCENE && currentState != GameState::HOMEBASE &&
		currentState != GameState::BATTLE)
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
	case GameState::GAMEPLAY:
		DrawGameplay();
		break;
	case GameState::CUTSCENE:
		renderCutscene();
		break;
	case GameState::HOMEBASE:
		HomeBase_Draw();
		break;
	case GameState::BATTLE:
		Battle_Draw();
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
	std::cout << mx << " " << my << " ";

	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && currentState == GameState::MENU)
	{
		if (IsInsideButton(startBtn, mx, my))
		{
			// A save exists -> resume straight into Home Base; otherwise start a fresh run.
			if (g_hasSaveData)
				EnterLoading(GameState::HOMEBASE);
			else
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
			SaveGame();
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
			// Confirm only exists once a card is selected. Clicking it loads that
			// character's sprites, resets the map, and heads into LOADING -> GAMEPLAY.
			loadCharacterAssets();
			ResetGameplayState();

			// Make the confirmed hero the one Home Base starts on too. Without this,
			// activeFighterIndex stays at its hardcoded default (CHAR_GUARDIAN in
			// HomeBase.hpp), so Home Base always opened on the Guardian no matter which
			// character was picked here. CharacterId (Menu.h) and CHAR_* (Fighters.hpp)
			// don't share the same numbering, so map explicitly instead of casting.
			if (characterNumber == CHARACTER_ALCHEMIST) activeFighterIndex = CHAR_ALCHEMIST;
			else if (characterNumber == CHARACTER_RANGER) activeFighterIndex = CHAR_RANGER;
			else if (characterNumber == CHARACTER_GUARDIAN) activeFighterIndex = CHAR_GUARDIAN;

			EnterLoading(GameState::GAMEPLAY);
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
			ResetSaveGame();    // also wipes savegame.txt + coins/resources/inventory
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
		else if (IsInsideButton(musicToggleBtn, mx, my))
		{
			// Toggle background music. audioOn drives which of musicOnImg/musicOffImg
			// DrawSettings() shows next frame, and mciSendString actually starts/stops
			// "bgsong" (opened once in main() - see the mciSendString calls there).
			audioOn = !audioOn;
			if (audioOn)
				mciSendString("play bgsong repeat", NULL, 0, NULL);
			else
				mciSendString("stop bgsong", NULL, 0, NULL);
		}
		else if (IsInsideButton(soundToggleBtn, mx, my))
		{
			// See the comment on soundOn's declaration - toggles and redraws,
			// nothing to actually mute yet.
			soundOn = !soundOn;
		}
	}

	if (currentState == GameState::HOMEBASE && state == GLUT_DOWN)
	{
		HomeBase_OnMouseDown(button, mx, my);
	}

	if (currentState == GameState::BATTLE && state == GLUT_DOWN)
	{
		Battle_OnMouseDown(button, mx, my);
	}

	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && currentState == GameState::CUTSCENE)
	{
		if (IsInsideButton(skipDialogueBtn, mx, my))
		{
			// Same end state as dismissing the final line normally.
			currentState = GameState::GAMEPLAY;
			currentDialogueIndex = 0;
		}
	}
}

// Special Keys:
// GLUT_KEY_F1, GLUT_KEY_F2, GLUT_KEY_F3, GLUT_KEY_F4, GLUT_KEY_F5, GLUT_KEY_F6, GLUT_KEY_F7, GLUT_KEY_F8, GLUT_KEY_F9, GLUT_KEY_F10, GLUT_KEY_F11, GLUT_KEY_F12, 
// GLUT_KEY_LEFT, GLUT_KEY_UP, GLUT_KEY_RIGHT, GLUT_KEY_DOWN, GLUT_KEY_PAGE UP, GLUT_KEY_PAGE DOWN, GLUT_KEY_HOME, GLUT_KEY_END, GLUT_KEY_INSERT

// NOTE: iGraphics never calls a user "iKeyboard" callback - key state is polled with
// isKeyPressed()/isSpecialKeyPressed() instead, which is what fixedUpdate() below does.
void fixedUpdate()
{
	if (currentState == GameState::LOADING)
	{
		loadingTimer--;
		if (loadingTimer <= 0)
		{
			currentState = pendingState;
			if (pendingState == GameState::BATTLE)
				Battle_Init();   // fresh enemy + reset positions each time BATTLE is entered
			if (pendingState == GameState::HOMEBASE)
			{
				// Coming back from the arena: the night timer was paused the moment we
				// left HOMEBASE (it only ticks inside HomeBase_FixedUpdate - see
				// updateNightTimer() in HomeBase.hpp), so just resume it where it left
				// off. Any other arrival at Home Base (fresh game, portal, loaded save)
				// starts the countdown over from the full duration, same as before.
				if (g_loadingFromState == GameState::BATTLE)
					HomeBase_ResumeNightTimer();
				else
					HomeBase_StartNightTimer();
			}
		}
		return;
	}

	if (currentState == GameState::CUTSCENE)
	{
		if (isKeyPressed(' '))
		{
			if (!spacePressedLastFrame)
			{
				currentDialogueIndex++;

				if (currentDialogueIndex >= TOTAL_DIALOGUES)
				{
					currentState = GameState::GAMEPLAY;
					currentDialogueIndex = 0;
				}
				spacePressedLastFrame = true;
			}
		}
		else
		{
			spacePressedLastFrame = false;
		}
		return;
	}

	if (currentState == GameState::GAMEPLAY)
	{
		isMoving = false;
		// Movement speed scaled directly from the confirmed character's stats in Player.hpp
		int moveSpeed = (int)(CH[characterNumber].moveSpeed * 0.016f);

		isNearWitch = checkProximityToWitch();
		isNearPortal = checkProximityToPortal();

		if ((isKeyPressed('\r') || isKeyPressed('\n')) && isNearPortal && isWitchRescued)
		{
			// This map is cleared - bump the level counter and step through the portal
			// into Home Base (the witch's "somewhere safe to catch your breath").
			currentLevel++;
			SaveGame();   // first arrival at Home Base - create/refresh the save
			EnterLoading(GameState::HOMEBASE);
			return;
		}

		if ((isKeyPressed('x') || isKeyPressed('X')) && isNearWitch && !isWitchRescued)
		{
			isWitchRescued = true;
			currentState = GameState::CUTSCENE;
			return;
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
		if (heroX > 800 - heroWidth)
			heroX = 800 - heroWidth;
		if (heroY > 600 - heroHeight)
			heroY = 600 - heroHeight;

		// Frame animation counter
		if (isMoving) {

			frameDelayCounter++;
			if (frameDelayCounter >= 8) {

				walkFrameIndex = (walkFrameIndex + 1) % 4;
				frameDelayCounter = 0;
			}
		}
		else {

			walkFrameIndex = 0;
			frameDelayCounter = 0;
		}
	}

	if (currentState == GameState::HOMEBASE)
	{
		bool wantsMenu = HomeBase_FixedUpdate();
		if (wantsMenu)
		{
			SaveGame();
			EnterLoading(GameState::MENU);
		}
	}

	if (currentState == GameState::BATTLE)
	{
		bool wantsExit = Battle_FixedUpdate();
		if (wantsExit)
			EnterLoading(GameState::HOMEBASE);
	}

	if (currentState == GameState::HOMEBASE || currentState == GameState::BATTLE)
	{
		unsigned long now = GetTickCount();
		if (now - g_lastAutoSaveTick >= AUTOSAVE_INTERVAL_MS)
		{
			SaveGame();
			g_lastAutoSaveTick = now;
		}

		// Stone/Wood/Iron trickle in on their own, whether we're back at
		// base or mid-battle in the arena (see GatherSystem.hpp).
		updatePassiveResourceGain();
	}
}


int main()
{
	// Background music (looping) + a game-over stinger, opened up front so both are
	// ready to play the instant they're needed.
	mciSendString("open \"Audios//game_bgm.mp3\" alias bgsong", NULL, 0, NULL);
	mciSendString("open \"Audios//gameover.mp3\" alias ggsong", NULL, 0, NULL);
	mciSendString("play bgsong repeat", NULL, 0, NULL);

	iInitialize(800, 600, "Mini Craft");
	// NOTE: fixedUpdate() is already driven automatically by iInitialize()'s internal
	// keypress-sampling timer, so it must NOT also be registered via iSetTimer() here -
	// doing both would call it twice as often and double all movement speeds.

	backgroundImg1 = iLoadImage("Images//Game_background1.jpg");
	backgroundImg2 = iLoadImage("Images//Game_background2.jpg");
	selectCharacterImg = iLoadImage("Buttons//select_character.png");

	// These read CH[i].fightStand from Player.hpp, which points at
	// "Alchemist card.jpg", "Ranger card.jpg", and "Guardian card.jpg" (all 842x576).
	// NOTE: iLoadImage() takes a non-const char*, but std::string::c_str() returns
	// const char* - const_cast is safe here since iLoadImage only reads the filename.
	charBoxes[0].imgId = iLoadImage(const_cast<char*>(CH[0].fightStand.c_str()));
	charBoxes[1].imgId = iLoadImage(const_cast<char*>(CH[1].fightStand.c_str()));
	charBoxes[2].imgId = iLoadImage(const_cast<char*>(CH[2].fightStand.c_str()));

	// Button art (PNG set - see the alpha-blending note added to iShowImage()
	// in iGraphics.h; these all have transparent rounded corners now).
	// RESET LEVEL and Confirm have no art provided, so their imgId stays 0
	// and DrawButton() keeps drawing them the old manual way.
	startBtn.imgId = iLoadImage("Buttons//start_button.png");
	levelBtn.imgId = iLoadImage("Buttons//level_button.png");
	settingsBtn.imgId = iLoadImage("Buttons//settings_button.png");
	exitBtn.imgId = iLoadImage("Buttons//exit_button.png");
	backBtn.imgId = iLoadImage("Buttons//Back_button.png");
	easyBtn.imgId = iLoadImage("Buttons//easy_button.png");
	mediumBtn.imgId = iLoadImage("Buttons//medium_button.png");
	hardBtn.imgId = iLoadImage("Buttons//hard_button.png");
	nextCharBtn.imgId = iLoadImage("Buttons//next_button.png");
	confirmCharBtn.imgId = iLoadImage("Buttons//select_button.png");

	// LEVEL_SELECT / SETTINGS label art
	currentLevelImg = iLoadImage("Buttons//current_level_button.png");
	resetWarningImg = iLoadImage("Buttons//Reset_level_warning.png");
	difficultyImg = iLoadImage("Buttons//difficulty_button.png");
	resetLevelBtn.imgId = iLoadImage("Buttons//reset_level_button.png");

	// Music on/off toggle
	musicOnImg = iLoadImage("Buttons//music_on_button.png");
	musicOffImg = iLoadImage("Buttons//music_off_button.png");

	// Sound on/off toggle
	soundOnImg = iLoadImage("Buttons//sound_on_button.png");
	soundOffImg = iLoadImage("Buttons//sound_off_button.png");

	// Gameplay assets that don't depend on which hero is chosen - safe to load up front.
	// (Hero sprites themselves are loaded lazily by loadCharacterAssets() once confirmed.)
	witchUnconsciousID = iLoadImage("Images//unconscious_witch.png");
	witchStandingID = iLoadImage("Images//right_looking_witch.png");
	witchPortraitID = iLoadImage("Images//right_looking_witch.png");
	spellBookID = iLoadImage("Images//witch_book.png");
	goblinHelperID = iLoadImage("Images//goblin_trio.png");

	// Home Base scene (reached through the portal once the witch is rescued)
	HomeBase_Init();

	// Must come AFTER HomeBase_Init() (which zeroes the inventory) - restores any saved progress.
	LoadGame();

	iStart();
	return 0;
}
