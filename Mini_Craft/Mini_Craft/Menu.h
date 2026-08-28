#ifndef MENU_H
#define MENU_H

enum class GameState { MENU, LEVEL_SELECT, PLAYING };

struct Button {
	int x1, y1, x2, y2;
	const char* label;
};

// Globals are declared with "extern"
extern GameState currentState;
extern Button startBtn;
extern Button levelBtn;
extern Button exitBtn;
extern int backgroundImg;

// Function declarations only - bodies live in iMain.cpp
bool IsInsideButton(const Button& b, int mx, int my);
void DrawButton(const Button& b);
void DrawMenu();

#endif