#ifndef MENU_H
#define MENU_H

// MENU / CHARACTER_SELECT / LOADING / LEVEL_SELECT / SETTINGS came from the menu build.
// GAMEPLAY / CUTSCENE came from the "Save the Witch" map build (part_raya) - they replace
// the old placeholder PLAYING state now that the map actually exists.
// HOMEBASE came from part_maria's build: after the witch is rescued and the player walks
// through the portal, they arrive at Home Base - pick a fighter, spar, and gather resources.
// BATTLE is reached from Home Base by clicking the Attack button - a 1v1 fight against an
// enemy from Enemy.hpp (see Battle.hpp).
enum class GameState { MENU, CHARACTER_SELECT, LOADING, LEVEL_SELECT, SETTINGS, GAMEPLAY, CUTSCENE, HOMEBASE, BATTLE };

// characterNumber uses these values - matches the order of CH[] in Player.hpp
enum CharacterId { CHARACTER_ALCHEMIST = 0, CHARACTER_RANGER = 1, CHARACTER_GUARDIAN = 2 };

struct Button {
	int x1, y1, x2, y2;
	const char* label;
	int imgId;   // 0 = no button image loaded - DrawButton() falls back to the old manual rectangle+text look
};

struct CharacterBox {
	int x1, y1, x2, y2;
	int imgId;
};

// Globals are declared with "extern"
extern GameState currentState;
extern Button startBtn;
extern Button levelBtn;
extern Button settingsBtn;
extern Button exitBtn;
extern Button backBtn;
extern Button resetLevelBtn;
extern Button easyBtn;
extern Button mediumBtn;
extern Button hardBtn;
extern Button nextCharBtn;
extern Button confirmCharBtn;   // spawns below the card once it's clicked; confirms the character
extern int backgroundImg1;
extern int backgroundImg2;
extern int selectCharacterImg;

extern CharacterBox charBoxes[3];
extern int characterNumber;    // -1 = none picked yet, otherwise CharacterId of the confirmed hero
extern int currentCardIndex;   // which of the 3 character cards is currently shown

extern int currentLevel;
extern int difficulty;   // 0 = Easy, 1 = Medium, 2 = Hard

// Function declarations only - bodies live in iMain.cpp
bool IsInsideButton(const Button& b, int mx, int my);
bool IsInsideBox(const CharacterBox& b, int mx, int my);
void DrawButton(const Button& b, bool highlighted = false);
void DrawMenu();
void DrawCharacterSelect();
void DrawLoading();
void DrawLevelSelect();
void DrawSettings();

// Defined in iMain.cpp, but HomeBase.hpp/Battle.hpp need to call it (to transition into
// BATTLE and back) before its actual definition appears later in that file - declaring it
// here up front, same as the Draw* functions above, makes that legal.
void EnterLoading(GameState target);

#endif
