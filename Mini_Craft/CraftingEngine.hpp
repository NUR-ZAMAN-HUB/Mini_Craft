// =====================================================================
//  CraftingEngine.hpp
// =====================================================================
//  Witchcraft Crafting Engine for Mini Craft.
//
//  Flow (matches the Home Base "inventory icon -> crafting book" UI):
//    1. Player clicks the inventory icon (Home Base corner button)
//       -> toggleCraftingMenu() opens Images/inventory_craftingbg.png
//          as a panel over Home Base, with Images/craftingbook.png
//          drawn on it and one slot per recipe (the potions you can craft).
//    2. Clicking a potion slot on the book selects it and opens the
//       right-side box (Images/task_box.png, one row per ingredient)
//       listing what that recipe needs.
//    3. An ingredient row is ONLY shown if the player currently holds
//       more than 0 of that item - a required item you have none of
//       does not get a row (see the `have <= 0` skip below).
//    4. A CRAFT button inside that box calls craftItem() when clicked
//       (canCraft() gates whether it's actually allowed to do anything).
//
//  Builds on inventory.hpp's addItem()/removeItem()/hasResource() -
//  a Recipe just describes which items go in and which item comes out;
//  canCraft()/craftItem() do the actual checking/spending.
//
//  Self-contained: does not touch iMain.cpp. Wiring into HomeBase.hpp
//  is 3 lines - see the comment block at the bottom of this file.
//
//  Written for Visual Studio 2013 (no C++11 features used) + iGraphics.
// =====================================================================
#pragma once

#include "iGraphics.h"
#include "inventory.hpp"
#include <stdio.h>    // sprintf()
#include <string.h>   // strlen()

// A recipe can use at most this many different ingredient types.
#define MAX_RECIPE_INPUTS 4

// ---------------------------------------------------------------
//  Recipe
// ---------------------------------------------------------------
struct Recipe
{
	int inputItemID[MAX_RECIPE_INPUTS];   // which items are consumed
	int inputCount[MAX_RECIPE_INPUTS];    // how much of each
	int inputTotal;                       // how many of the slots above are actually used

	int outputItemID;                     // item produced
	int outputCount;                      // how many of it
};

// Small helper so recipe tables read like the proposal's
// "5 Wood + 2 Water = 1 Health Potion" instead of raw struct literals.
inline Recipe makeRecipe1(int in1, int c1, int outItem, int outCount)
{
	Recipe r;
	r.inputItemID[0] = in1; r.inputCount[0] = c1;
	r.inputTotal = 1;
	r.outputItemID = outItem; r.outputCount = outCount;
	return r;
}

inline Recipe makeRecipe2(int in1, int c1, int in2, int c2, int outItem, int outCount)
{
	Recipe r;
	r.inputItemID[0] = in1; r.inputCount[0] = c1;
	r.inputItemID[1] = in2; r.inputCount[1] = c2;
	r.inputTotal = 2;
	r.outputItemID = outItem; r.outputCount = outCount;
	return r;
}

inline Recipe makeRecipe3(int in1, int c1, int in2, int c2, int in3, int c3, int outItem, int outCount)
{
	Recipe r;
	r.inputItemID[0] = in1; r.inputCount[0] = c1;
	r.inputItemID[1] = in2; r.inputCount[1] = c2;
	r.inputItemID[2] = in3; r.inputCount[2] = c3;
	r.inputTotal = 3;
	r.outputItemID = outItem; r.outputCount = outCount;
	return r;
}

// ---------------------------------------------------------------
//  RECIPE BOOK  - the crafting menu just loops over this array.
//  Potions no longer cost Wood/Water/Iron/Stone - each one is bought
//  straight with Coins only. Coins are checked/spent via ITEM_COIN,
//  which hasResource()/removeItem()/getItemCount() in inventory.hpp
//  redirect to the real g_playerCoins balance.
// ---------------------------------------------------------------
static Recipe RECIPE_BOOK[] =
{
	makeRecipe1(ITEM_COIN, 15, ITEM_HEALTH_POTION, 1),
	makeRecipe1(ITEM_COIN, 15, ITEM_POWER_POTION, 1),
	makeRecipe1(ITEM_COIN, 15, ITEM_MOVEMENT_POTION, 1),
	makeRecipe1(ITEM_COIN, 15, ITEM_REINFORCEMENT_POTION, 1),
};
static const int RECIPE_COUNT = sizeof(RECIPE_BOOK) / sizeof(RECIPE_BOOK[0]);

// ---------------------------------------------------------------
//  CRAFTING MENU STATE
// ---------------------------------------------------------------
bool isCraftingMenuOpen = false;
int  g_selectedRecipeIndex = -1;   // which RECIPE_BOOK[] slot's task-box is showing, -1 = none

// ---------------------------------------------------------------
//  TEXTURES
// ---------------------------------------------------------------
//  The panel background itself reuses inventory.hpp's g_invPanelTexUI
//  (already loaded from Images/inventory_craftingbg.png by
//  loadInventoryTextures() - no point loading that file twice).
//  Only the book + task-box art are new; load them once, AFTER
//  iInitialize(), same as loadInventoryTextures().
static unsigned int g_craftingBookTex = 0;   // Images/craftingbook.png
static unsigned int g_taskBoxTex = 0;        // Images/task_box.png
static unsigned int g_selectIconTex = 0;

inline void loadCraftingTextures()
{
	g_craftingBookTex = iLoadImage((char*)"Images/craftingbook.png");
	g_taskBoxTex = iLoadImage((char*)"Images/task_box.png");

}

// ---------------------------------------------------------------
//  OPEN / CLOSE
// ---------------------------------------------------------------
inline void openCraftingMenu() { isCraftingMenuOpen = true; }
inline void closeCraftingMenu() { isCraftingMenuOpen = false; g_selectedRecipeIndex = -1; }
inline void toggleCraftingMenu() { if (isCraftingMenuOpen) closeCraftingMenu(); else openCraftingMenu(); }

// ---------------------------------------------------------------
//  "+1" CRAFT POPUP
//  Whenever craftItem() below successfully crafts something (a potion
//  OR an armor piece - both funnel through this one function), a "+1"
//  pops up in the middle of the Home Base screen, drifts upward for a
//  moment, then disappears - right as the crafted item is added to the
//  inventory (addItem() call happens the same instant, in craftItem()).
// ---------------------------------------------------------------
static bool g_craftPopupActive = false;
static unsigned long g_craftPopupStartTime = 0;

#define CRAFT_POPUP_DURATION_MS  900   // how long the "+1" stays on screen
#define CRAFT_POPUP_RISE_PX      55    // how far up it drifts over that time
#define CRAFT_POPUP_CENTER_X     400   // Home Base window is 800 wide (see HomeBaseConfig.hpp)
#define CRAFT_POPUP_CENTER_Y     300   // ...and 600 tall, so this is screen-middle

inline void triggerCraftPopup()
{
	g_craftPopupActive = true;
	g_craftPopupStartTime = GetTickCount();
}

// Called once per frame from HomeBase_Draw() - draws nothing unless a
// craft just happened and the popup's short lifetime hasn't elapsed yet.
inline void drawCraftPopup()
{
	if (!g_craftPopupActive) return;

	unsigned long elapsed = GetTickCount() - g_craftPopupStartTime;
	if (elapsed >= CRAFT_POPUP_DURATION_MS)
	{
		g_craftPopupActive = false;
		return;
	}

	float t = (float)elapsed / (float)CRAFT_POPUP_DURATION_MS;   // 0 -> 1 over its lifetime
	int riseY = (int)(t * CRAFT_POPUP_RISE_PX);

	int x = CRAFT_POPUP_CENTER_X - 14;
	int y = CRAFT_POPUP_CENTER_Y + riseY;

	iSetColor(80, 220, 90);
	// GLUT bitmap fonts have no bold weight - fake it the same way the
	// armor name plates do, by stamping the text twice, 1px apart.
	iText(x,     y, (char*)"+1", GLUT_BITMAP_TIMES_ROMAN_24);
	iText(x + 1, y, (char*)"+1", GLUT_BITMAP_TIMES_ROMAN_24);
}

// ---------------------------------------------------------------
//  CORE CRAFTING OPERATIONS
// ---------------------------------------------------------------
// True if the inventory currently holds every ingredient a recipe needs.
inline bool canCraft(const Recipe &recipe)
{
	for (int i = 0; i < recipe.inputTotal; i++)
	{
		if (!hasResource(recipe.inputItemID[i], recipe.inputCount[i]))
			return false;
	}
	return true;
}

// Spends the ingredients and adds the crafted item to the inventory.
// Returns false (and changes nothing) if canCraft(recipe) would be false.
inline bool craftItem(const Recipe &recipe)
{
	if (!canCraft(recipe)) return false;

	for (int i = 0; i < recipe.inputTotal; i++)
		removeItem(recipe.inputItemID[i], recipe.inputCount[i]);

	addItem(recipe.outputItemID, recipe.outputCount);
	triggerCraftPopup();
	return true;
}

// ---------------------------------------------------------------
//  LAYOUT  (800x600 Home Base window - see HomeBaseConfig.hpp)
// ---------------------------------------------------------------
// Whole overlay panel (Images/inventory_craftingbg.png), centered.
#define CRAFT_PANEL_W   690
#define CRAFT_PANEL_H   460
#define CRAFT_PANEL_X   ((800 - CRAFT_PANEL_W) / 2)
#define CRAFT_PANEL_Y   ((600 - CRAFT_PANEL_H) / 2)

// The open book (Images/craftingbook.png) - shifted right by 60px.
#define CRAFT_BOOK_W   250
#define CRAFT_BOOK_H   250

#define CRAFT_SIDE_GAP   20
#define CRAFT_FLANK_W    ((CRAFT_PANEL_W - CRAFT_BOOK_W - 2 * CRAFT_SIDE_GAP) / 2 - 25)

#define CRAFT_BOOK_X   (CRAFT_PANEL_X + CRAFT_FLANK_W + CRAFT_SIDE_GAP + 40)
#define CRAFT_BOOK_Y   (CRAFT_PANEL_Y + (CRAFT_PANEL_H - CRAFT_BOOK_H) / 25)

// The currently-selected recipe gets one big icon floating centered right on top of the book.
#define BOOK_ICON_SIZE  100
#define BOOK_ICON_X     (CRAFT_BOOK_X + (CRAFT_BOOK_W - BOOK_ICON_SIZE) / 2)
#define BOOK_ICON_Y     (CRAFT_BOOK_Y + 240)

#define TOP_SLOT_SIZE     70
// Widened from 35 so the name plates below (which can be wider than the
// icons themselves - "Reinforcement" needs ~154px) always keep a minimum
// gap between each other instead of running into their neighbor.
#define TOP_SLOT_GAP      60
#define TOP_ROW_W         (RECIPE_COUNT * TOP_SLOT_SIZE + (RECIPE_COUNT - 1) * TOP_SLOT_GAP)
#define TOP_ROW_X         (CRAFT_PANEL_X + (CRAFT_PANEL_W - TOP_ROW_W) / 2)
// Dropped further down from the panel top (was just -30) so the "POTION"
// title above has clear space and doesn't crowd the icons.
#define TOP_ROW_Y         (CRAFT_PANEL_Y + CRAFT_PANEL_H - TOP_SLOT_SIZE - 75)

// Name plate under each potion icon (unselected top row) - same white
// rounded-plate look as ArmorEngine.hpp's ATOP_LABEL, just placed below
// the icon instead of beside it since the potion book's icons sit in a
// horizontal row rather than a vertical column. Width is sized to the
// name itself (see drawCraftingMenu()) since "Reinforcement" needs more
// room than "Power" does.
#define TOP_LABEL_GAP     14
#define TOP_LABEL_H       30
#define TOP_LABEL_MIN_W   90

#define LEFT_SLOT_SIZE    70
#define LEFT_SLOT_GAP     26
#define LEFT_COL_X        (CRAFT_PANEL_X + 45)
#define LEFT_COL_TOP_Y    (CRAFT_PANEL_Y + CRAFT_PANEL_H - 120)

// Right-side ingredient box - one Images/task_box.png row per ingredient.
// Height/gap trimmed down from the original 70/10 so a 3rd row (the new
// 15-Coin requirement added to every recipe above) still fits above the
// output line + Craft button without overlapping them.
#define TASK_ROW_W    165
#define TASK_ROW_H    50
#define TASK_ROW_GAP  8
#define TASK_ROW_X    (CRAFT_BOOK_X + CRAFT_BOOK_W + CRAFT_SIDE_GAP + (CRAFT_FLANK_W - TASK_ROW_W) / 2)
#define TASK_ROW_TOP_Y (CRAFT_BOOK_Y + CRAFT_BOOK_H - 74)

// Craft button, bottom of the right column.
#define CRAFT_BTN_W  140
#define CRAFT_BTN_H  34

inline void getCraftButtonRect(int &x, int &y, int &w, int &h)
{
	w = CRAFT_BTN_W;
	h = CRAFT_BTN_H;
	x = TASK_ROW_X + (TASK_ROW_W - CRAFT_BTN_W) / 2;
	y = CRAFT_PANEL_Y + 12;
}

// ---------------------------------------------------------------
//  SLOT LAYOUT  (shared by drawing AND click detection)
// ---------------------------------------------------------------
inline void getRecipeSlotRect(int recipeIndex, int &x, int &y, int &w, int &h)
{
	if (recipeIndex == g_selectedRecipeIndex)
	{
		w = BOOK_ICON_SIZE;
		h = BOOK_ICON_SIZE;
		x = BOOK_ICON_X;
		y = BOOK_ICON_Y;
		return;
	}

	// Kichu select na thakle -> shob potion top row e, order onujayi.
	if (g_selectedRecipeIndex < 0)
	{
		w = TOP_SLOT_SIZE;
		h = TOP_SLOT_SIZE;
		x = TOP_ROW_X + recipeIndex * (TOP_SLOT_SIZE + TOP_SLOT_GAP);
		y = TOP_ROW_Y;
		return;
	}

	// Ekta select ache (oita center e) -> baki gula left column e vertically.
	int orderPos = 0;
	for (int i = 0; i < recipeIndex; i++)
	if (i != g_selectedRecipeIndex) orderPos++;

	w = LEFT_SLOT_SIZE;
	h = LEFT_SLOT_SIZE;
	x = LEFT_COL_X;
	y = LEFT_COL_TOP_Y - orderPos * (LEFT_SLOT_SIZE + LEFT_SLOT_GAP);
}

// Forward-declared here so drawCraftingMenu() below can use it for hover
// detection; full definition is further down in the CLICK HANDLING section.
inline int craftingBookSlotAt(int mx, int my);

// ---------------------------------------------------------------
//  DRAWING
// ---------------------------------------------------------------
inline void drawCraftingMenu()
{
	if (!isCraftingMenuOpen) return;

	// Panel background
	if (g_invPanelTexUI != 0)
		iShowImage(CRAFT_PANEL_X, CRAFT_PANEL_Y, CRAFT_PANEL_W, CRAFT_PANEL_H, g_invPanelTexUI);
	else
	{
		iSetColor(0.05, 0.05, 0.08);
		iFilledRectangle(CRAFT_PANEL_X, CRAFT_PANEL_Y, CRAFT_PANEL_W, CRAFT_PANEL_H);
	}

	// The open book
	if (g_craftingBookTex != 0)
		iShowImage(CRAFT_BOOK_X, CRAFT_BOOK_Y, CRAFT_BOOK_W, CRAFT_BOOK_H, g_craftingBookTex);

	// Centered "POTION" title, drawn the same way ArmorEngine.hpp draws
	// its "ARMOR" title (bold faked by stamping the text twice, 1px apart).
	iSetColor(0, 0, 0);
	{
		char* title = (char*)"POTION";
		int titleX = CRAFT_PANEL_X + CRAFT_PANEL_W / 2 - 50;
		int titleY = CRAFT_PANEL_Y + CRAFT_PANEL_H - 40;
		iText(titleX,     titleY, title, GLUT_BITMAP_TIMES_ROMAN_24);
		iText(titleX + 1, titleY, title, GLUT_BITMAP_TIMES_ROMAN_24);
	}

	for (int i = 0; i < RECIPE_COUNT; i++)
	{
		int x, y, w, h;
		getRecipeSlotRect(i, x, y, w, h);

		bool craftable = canCraft(RECIPE_BOOK[i]);

		unsigned int tex = ITEM_DB[RECIPE_BOOK[i].outputItemID].iconTex;
		if (tex != 0)
			iShowImage(x, y, w, h, tex);

		if (i != g_selectedRecipeIndex)
		{
			// Nothing selected -> icons sit in the top row; each name is
			// plain black text centered under its icon - no white plate
			// behind it (removed on request).
			if (g_selectedRecipeIndex < 0)
			{
				char* nameStr = ITEM_DB[RECIPE_BOOK[i].outputItemID].name;

				// ~10px/char estimate for Helvetica 18 (same idea as the
				// 7.5px/char estimate iMain.cpp uses for Helvetica 12),
				// used to center the text under the icon.
				int textW = (int)strlen(nameStr) * 10;

				int tx = x + w / 2 - textW / 2;
				int ty = y - TOP_LABEL_GAP - TOP_LABEL_H / 2;

				iSetColor(0, 0, 0);
				iText(tx, ty, nameStr, GLUT_BITMAP_HELVETICA_18);
			}
			else
			{
				if (craftable) iSetColor(0.55, 1.0, 0.55);
				else           iSetColor(0.85, 0.55, 0.55);
				iText(x, y - 14, ITEM_DB[RECIPE_BOOK[i].outputItemID].name);
			}
		}
	}
	int hoveredSlot = craftingBookSlotAt(iMouseX, iMouseY);
	if (hoveredSlot >= 0 && hoveredSlot != g_selectedRecipeIndex && g_selectIconTex != 0)
	{
		int hx, hy, hw, hh;
		getRecipeSlotRect(hoveredSlot, hx, hy, hw, hh);

		// slotselected.png ekhon icon + niche-r name text - dutoi ghire
		// (border/frame hishebe) draw hobe, choto badge er bodole.
		int frameW = hw + 34;
		int frameH = hh + 54;
		int frameX = hx - (frameW - hw) / 2;
		int frameY = hy - 34;   // name text-er jonno extra jaiga niche
		iShowImage(frameX, frameY, frameW, frameH, g_selectIconTex);
	}

	// Right-side task box - only while a recipe slot is selected.
	if (g_selectedRecipeIndex < 0 || g_selectedRecipeIndex >= RECIPE_COUNT)
		return;

	const Recipe &sel = RECIPE_BOOK[g_selectedRecipeIndex];

	int rowY = TASK_ROW_TOP_Y;
	int shownRows = 0;
	for (int i = 0; i < sel.inputTotal; i++)
	{
		int itemID = sel.inputItemID[i];
		int have = getItemCount(itemID);

		if (have <= 0) continue;

		if (g_taskBoxTex != 0)
			iShowImage(TASK_ROW_X, rowY, TASK_ROW_W, TASK_ROW_H, g_taskBoxTex);
		else
		{
			iSetColor(0.10, 0.20, 0.60);
			iFilledRectangle(TASK_ROW_X, rowY, TASK_ROW_W, TASK_ROW_H);
		}

		unsigned int itex = ITEM_DB[itemID].iconTex;
		if (itex != 0)
			iShowImage(TASK_ROW_X - 10, rowY + (TASK_ROW_H - 50) / 2, 50, 50, itex);

		bool enough = (have >= sel.inputCount[i]);
		char line[64];
		sprintf(line, "%s  %d/%d", ITEM_DB[itemID].name, have, sel.inputCount[i]);

		iSetColor(enough ? 0.5 : 1.0, enough ? 1.0 : 0.35, 0.4);
		iText(TASK_ROW_X + 68, rowY + TASK_ROW_H / 2 - 4, line);

		rowY -= (TASK_ROW_H + TASK_ROW_GAP);
		shownRows++;
	}

	if (shownRows == 0)
	{
		iSetColor(1, 1, 1);
		iText(TASK_ROW_X - 10, TASK_ROW_TOP_Y + 30, (char*)"(none in stock)");
	}

	// Output reminder + Craft button
	iSetColor(0, 0, 0);
	char outLine[64];
	sprintf(outLine, "-> %s x%d", ITEM_DB[sel.outputItemID].name, sel.outputCount);
	iText(TASK_ROW_X, CRAFT_PANEL_Y + 56, outLine);

	int bx, by, bw, bh;
	getCraftButtonRect(bx, by, bw, bh);
	bool craftable = canCraft(sel);

	// Craft Button Background (Pure White)
	iSetColor(255, 255, 255);
	iFilledRectangle(bx, by, bw, bh);

	// Button Border (Dark Gray)
	iSetColor(50, 50, 50);
	iRectangle(bx, by, bw, bh);

	// CRAFT Text (Blue)
	iSetColor(0, 50, 200);
	iText(bx + bw / 2 - 22, by + bh / 2 - 5, (char*)"CRAFT");
}

// ---------------------------------------------------------------
//  CLICK HANDLING
// ---------------------------------------------------------------
inline int craftingBookSlotAt(int mx, int my)
{
	for (int i = 0; i < RECIPE_COUNT; i++)
	{
		int x, y, w, h;
		getRecipeSlotRect(i, x, y, w, h);

		if (mx >= x && mx <= x + w && my >= y && my <= y + h)
			return i;
	}
	return -1;
}

inline void craftingMenuOnClick(int mx, int my)
{
	if (!isCraftingMenuOpen) return;

	if (g_selectedRecipeIndex >= 0)
	{
		int bx, by, bw, bh;
		getCraftButtonRect(bx, by, bw, bh);
		if (mx >= bx && mx <= bx + bw && my >= by && my <= by + bh)
		{
			craftItem(RECIPE_BOOK[g_selectedRecipeIndex]);
			return;
		}
	}

	int slot = craftingBookSlotAt(mx, my);
	if (slot >= 0)
	{
		g_selectedRecipeIndex = (g_selectedRecipeIndex == slot) ? -1 : slot;
		return;
	}

	bool insidePanel = (mx >= CRAFT_PANEL_X && mx <= CRAFT_PANEL_X + CRAFT_PANEL_W &&
		my >= CRAFT_PANEL_Y && my <= CRAFT_PANEL_Y + CRAFT_PANEL_H);
	if (!insidePanel)
		closeCraftingMenu();
}