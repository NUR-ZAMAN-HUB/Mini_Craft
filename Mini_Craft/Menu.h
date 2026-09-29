#ifndef MENU_H
#define MENU_H

enum class GameState { MENU, CHARACTER_SELECT, LOADING, LEVEL_SELECT, SETTINGS, PLAYING };

// characterNumber uses these values - matches the order of CH[] in Player.hpp
enum CharacterId { CHARACTER_ALCHEMIST = 0, CHARACTER_RANGER = 1, CHARACTER_GUARDIAN = 2 };

struct Button {
	int x1, y1, x2, y2;
	const char* label;
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
extern int characterNumber;
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

#endif