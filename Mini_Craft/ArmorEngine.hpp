// =====================================================================
//  ArmorEngine.hpp
// =====================================================================
//  Armor crafting panel for Mini Craft - built the same way as
//  CraftingEngine.hpp's potion book (same blur panel reused from
//  inventory.hpp's g_invPanelTexUI, same panel rect, same Recipe /
//  canCraft() / craftItem() plumbing from CraftingEngine.hpp), just a
//  separate recipe list/output items for Iron / Rusty / Steel Armor.
//
//  Flow (mirrors CraftingEngine.hpp's toggleCraftingMenu() flow):
//    1. Player clicks the Armor icon (Home Base corner button, next to
//       the crafting-book icon) -> toggleArmorMenu() opens the same
//       Images/inventory_craftingbg.png panel used by the crafting book
//       and the inventory panel (same size/position on screen). With
//       nothing selected, all 3 armor pieces show in a row.
//    2. Clicking one selects it - only that armor shows (big, on the
//       left), the other two drop into a small column further left, and
//       the right side lists what it costs, one row per ingredient
//       (only ingredients the player currently holds > 0 of show a row -
//       same rule CraftingEngine.hpp's potion book uses).
//    3. The rectangular CRAFT button crafts it - if there's enough of
//       every ingredient, the armor is added to the inventory and the
//       ingredients are spent (canCraft()/craftItem(), unchanged from
//       CraftingEngine.hpp).
//    4. Clicking outside the panel closes it.
//
//  Ingredient amounts below are a first pass at balance - tune freely,
//  nothing else needs to change to retune them.
//
//  Wiring into HomeBase.hpp: include this AFTER CraftingEngine.hpp (it
//  reuses CraftingEngine.hpp's Recipe struct/makeRecipe2/canCraft()/
//  craftItem()/CRAFT_PANEL_* geometry + inventory.hpp's g_invPanelTexUI),
//  then call drawArmorMenu() in HomeBase_Draw() and toggleArmorMenu()/
//  armorMenuOnClick() from HomeBase_OnMouseDown().
// =====================================================================
#pragma once

#include "iGraphics.h"
#include "inventory.hpp"
#include "CraftingEngine.hpp"   // Recipe / makeRecipe2 / canCraft() / craftItem() / CRAFT_PANEL_* / g_invPanelTexUI / g_taskBoxTex / g_selectIconTex
#include <stdio.h>              // sprintf()

// ---------------------------------------------------------------
//  ARMOR RECIPE BOOK  - same Recipe struct/helpers as the potion book.
//  Every armor piece now takes ONLY Iron, no other resource and no coins -
//  the cost scales with how strong the armor is: Rusty (weakest/starter)
//  costs the least, Steel (mid) costs more, Iron (best) costs the most.
//  Crafting spends Iron straight out of inventory (see craftItem()).
// ---------------------------------------------------------------
static Recipe ARMOR_RECIPE_BOOK[] =
{
	makeRecipe1(ITEM_IRON, 200, ITEM_IRON_ARMOR,  1),
	makeRecipe1(ITEM_IRON, 60,  ITEM_RUSTY_ARMOR, 1),
	makeRecipe1(ITEM_IRON, 130, ITEM_STEEL_ARMOR, 1),
};
static const int ARMOR_RECIPE_COUNT = sizeof(ARMOR_RECIPE_BOOK) / sizeof(ARMOR_RECIPE_BOOK[0]);

// ---------------------------------------------------------------
//  MENU STATE
// ---------------------------------------------------------------
bool isArmorMenuOpen = false;
int  g_selectedArmorRecipeIndex = -1;   // which ARMOR_RECIPE_BOOK[] slot's task-box is showing, -1 = none

inline void openArmorMenu() { isArmorMenuOpen = true; }
inline void closeArmorMenu() { isArmorMenuOpen = false; g_selectedArmorRecipeIndex = -1; }
inline void toggleArmorMenu() { if (isArmorMenuOpen) closeArmorMenu(); else openArmorMenu(); }

// ---------------------------------------------------------------
//  LAYOUT  (mirrors CraftingEngine.hpp's book layout, minus the book art
//  itself - the selected armor's own icon, enlarged, stands in for it)
// ---------------------------------------------------------------
#define ABIG_ICON_SIZE   180
#define ABIG_ICON_X      (CRAFT_PANEL_X + (CRAFT_PANEL_W - ABIG_ICON_SIZE) / 2)
#define ABIG_ICON_Y      (CRAFT_PANEL_Y + (CRAFT_PANEL_H - ABIG_ICON_SIZE) / 2)

#define ATOP_SLOT_SIZE    90
#define ATOP_SLOT_GAP     40
#define ATOP_COL_H        (ARMOR_RECIPE_COUNT * ATOP_SLOT_SIZE + (ARMOR_RECIPE_COUNT - 1) * ATOP_SLOT_GAP)

// Name tag - white rounded plate that sits beside each icon.
#define ATOP_LABEL_GAP     22   // space between icon and its name plate
#define ATOP_LABEL_W       190
#define ATOP_LABEL_H       36

// The icon + name-plate pair is centered as one block in the panel.
#define ATOP_ROW_TOTAL_W  (ATOP_SLOT_SIZE + ATOP_LABEL_GAP + ATOP_LABEL_W)
#define ATOP_COL_X        (CRAFT_PANEL_X + (CRAFT_PANEL_W - ATOP_ROW_TOTAL_W) / 2)
#define ATOP_COL_Y        (CRAFT_PANEL_Y + (CRAFT_PANEL_H - ATOP_COL_H) / 2)

#define ALEFT_SLOT_SIZE   70
#define ALEFT_SLOT_GAP    26
// Shifted right and down a bit from the panel's top-left corner so the
// unselected pieces (and their name labels) sit clear of the "ARMOR"
// title and the panel edge, instead of crowding into the corner.
#define ALEFT_COL_X       (CRAFT_PANEL_X + 55)
#define ALEFT_COL_TOP_Y   (CRAFT_PANEL_Y + CRAFT_PANEL_H - 170)

// Right-side ingredient box - one Images/task_box.png row per ingredient.
// Pulled in a bit from the edge (20px margin instead of 55) so it clears
// the bigger, now-centered big icon, and dropped down from just under the
// title to roughly mid-panel, leaving clear space above the output line
// and Craft button at the bottom.
#define ATASK_ROW_W    230
#define ATASK_ROW_H    56
#define ATASK_ROW_GAP  10
#define ATASK_ROW_X    (CRAFT_PANEL_X + CRAFT_PANEL_W - ATASK_ROW_W - 20)
#define ATASK_ROW_TOP_Y (CRAFT_PANEL_Y + CRAFT_PANEL_H - 210)

// Craft button, bottom of the right column - same size as the potion book's.
#define ACRAFT_BTN_W  140
#define ACRAFT_BTN_H  34

inline void getArmorCraftButtonRect(int &x, int &y, int &w, int &h)
{
	w = ACRAFT_BTN_W;
	h = ACRAFT_BTN_H;
	x = ATASK_ROW_X + (ATASK_ROW_W - ACRAFT_BTN_W) / 2;
	y = CRAFT_PANEL_Y + 12;
}

// ---------------------------------------------------------------
//  SLOT LAYOUT  (shared by drawing AND click detection)
// ---------------------------------------------------------------
inline void getArmorSlotRect(int index, int &x, int &y, int &w, int &h)
{
	if (index == g_selectedArmorRecipeIndex)
	{
		w = ABIG_ICON_SIZE;
		h = ABIG_ICON_SIZE;
		x = ABIG_ICON_X;
		y = ABIG_ICON_Y;
		return;
	}

	// Nothing selected -> all 3 armor pieces stack in a column, centered.
	if (g_selectedArmorRecipeIndex < 0)
	{
		w = ATOP_SLOT_SIZE;
		h = ATOP_SLOT_SIZE;
		x = ATOP_COL_X;
		y = ATOP_COL_Y + (ARMOR_RECIPE_COUNT - 1 - index) * (ATOP_SLOT_SIZE + ATOP_SLOT_GAP);
		return;
	}

	// One selected (shown big) -> the rest stack in a small left column.
	int orderPos = 0;
	for (int i = 0; i < index; i++)
	if (i != g_selectedArmorRecipeIndex) orderPos++;

	w = ALEFT_SLOT_SIZE;
	h = ALEFT_SLOT_SIZE;
	x = ALEFT_COL_X;
	y = ALEFT_COL_TOP_Y - orderPos * (ALEFT_SLOT_SIZE + ALEFT_SLOT_GAP);
}

// Name-plate rect for a slot - only meaningful while nothing is selected
// (that's the only state that draws a plate at all; see drawArmorMenu()).
// Shared by drawing AND click detection so a click on the plate itself
// (not just the icon) also selects that armor piece.
inline void getArmorLabelRect(int index, int &lx, int &ly, int &lw, int &lh)
{
	int x, y, w, h;
	getArmorSlotRect(index, x, y, w, h);

	lw = ATOP_LABEL_W;
	lh = ATOP_LABEL_H;
	lx = x + w + ATOP_LABEL_GAP;
	ly = y + h / 2 - ATOP_LABEL_H / 2;
}

inline int armorSlotAt(int mx, int my)
{
	for (int i = 0; i < ARMOR_RECIPE_COUNT; i++)
	{
		int x, y, w, h;
		getArmorSlotRect(i, x, y, w, h);

		if (mx >= x && mx <= x + w && my >= y && my <= y + h)
			return i;

		// Nothing selected -> each icon also has a name plate beside it;
		// clicking the plate should select the same item as the icon.
		if (g_selectedArmorRecipeIndex < 0)
		{
			int lx, ly, lw, lh;
			getArmorLabelRect(i, lx, ly, lw, lh);

			if (mx >= lx && mx <= lx + lw && my >= ly && my <= ly + lh)
				return i;
		}
	}
	return -1;
}

// ---------------------------------------------------------------
//  DRAWING
// ---------------------------------------------------------------
inline void drawArmorMenu()
{
	if (!isArmorMenuOpen) return;

	// Same blur panel background used by the inventory panel / crafting book.
	if (g_invPanelTexUI != 0)
		iShowImage(CRAFT_PANEL_X, CRAFT_PANEL_Y, CRAFT_PANEL_W, CRAFT_PANEL_H, g_invPanelTexUI);
	else
	{
		iSetColor(13, 13, 20);
		iFilledRectangle(CRAFT_PANEL_X, CRAFT_PANEL_Y, CRAFT_PANEL_W, CRAFT_PANEL_H);
	}

	iSetColor(0, 0, 0);
	{
		// Centered, bigger title - bold faked the same way as the name
		// plates below (GLUT bitmap fonts have no real bold weight).
		char* title = (char*)"ARMOR";
		int titleX = CRAFT_PANEL_X + CRAFT_PANEL_W / 2 - 45;
		int titleY = CRAFT_PANEL_Y + CRAFT_PANEL_H - 40;
		iText(titleX,     titleY, title, GLUT_BITMAP_TIMES_ROMAN_24);
		iText(titleX + 1, titleY, title, GLUT_BITMAP_TIMES_ROMAN_24);
	}

	for (int i = 0; i < ARMOR_RECIPE_COUNT; i++)
	{
		int x, y, w, h;
		getArmorSlotRect(i, x, y, w, h);

		bool craftable = canCraft(ARMOR_RECIPE_BOOK[i]);

		unsigned int tex = ITEM_DB[ARMOR_RECIPE_BOOK[i].outputItemID].iconTex;
		if (tex != 0)
			iShowImage(x, y, w, h, tex);
		else
		{
			iSetColor(38, 38, 38);
			iFilledRectangle(x, y, w, h);
		}

		if (i != g_selectedArmorRecipeIndex)
		{
			// Nothing selected -> icons sit in a centered vertical column;
			// each name gets its own white plate beside the icon, with the
			// name in bold black for a cleaner, more readable look.
			if (g_selectedArmorRecipeIndex < 0)
			{
				int lx, ly, lw, lh;
				getArmorLabelRect(i, lx, ly, lw, lh);

				iSetColor(255, 255, 255);
				iFilledRectangle(lx, ly, lw, lh);

				char* nameStr = ITEM_DB[ARMOR_RECIPE_BOOK[i].outputItemID].name;
				int tx = lx + 16;
				int ty = ly + lh / 2 - 6;

				iSetColor(0, 0, 0);
				// GLUT bitmap fonts have no bold weight, so fake it by
				// stamping the text twice, offset by a pixel.
				iText(tx,     ty, nameStr, GLUT_BITMAP_HELVETICA_18);
				iText(tx + 1, ty, nameStr, GLUT_BITMAP_HELVETICA_18);
			}
			else
			{
				if (craftable) iSetColor(140, 255, 140);
				else           iSetColor(217, 140, 140);
				iText(x, y - 14, ITEM_DB[ARMOR_RECIPE_BOOK[i].outputItemID].name);
			}
		}
	}

	int hoveredSlot = armorSlotAt(iMouseX, iMouseY);
	if (hoveredSlot >= 0 && hoveredSlot != g_selectedArmorRecipeIndex && g_selectIconTex != 0)
	{
		int hx, hy, hw, hh;
		getArmorSlotRect(hoveredSlot, hx, hy, hw, hh);

		int frameW = hw + 34;
		int frameH = hh + 54;
		int frameX = hx - (frameW - hw) / 2;
		int frameY = hy - 34;
		iShowImage(frameX, frameY, frameW, frameH, g_selectIconTex);
	}

	// Right-side task box + Craft button - only while an armor is selected.
	if (g_selectedArmorRecipeIndex < 0 || g_selectedArmorRecipeIndex >= ARMOR_RECIPE_COUNT)
		return;

	const Recipe &sel = ARMOR_RECIPE_BOOK[g_selectedArmorRecipeIndex];

	int rowY = ATASK_ROW_TOP_Y;
	int shownRows = 0;
	for (int i = 0; i < sel.inputTotal; i++)
	{
		int itemID = sel.inputItemID[i];
		int have = getItemCount(itemID);

		if (have <= 0) continue;

		if (g_taskBoxTex != 0)
			iShowImage(ATASK_ROW_X, rowY, ATASK_ROW_W, ATASK_ROW_H, g_taskBoxTex);
		else
		{
			iSetColor(26, 51, 153);
			iFilledRectangle(ATASK_ROW_X, rowY, ATASK_ROW_W, ATASK_ROW_H);
		}

		unsigned int itex = ITEM_DB[itemID].iconTex;
		if (itex != 0)
			iShowImage(ATASK_ROW_X - 10, rowY + (ATASK_ROW_H - 50) / 2, 50, 50, itex);

		char line[64];
		sprintf(line, "%s  %d/%d", ITEM_DB[itemID].name, have, sel.inputCount[i]);

		iSetColor(0, 0, 0);
		iText(ATASK_ROW_X + 68, rowY + ATASK_ROW_H / 2 - 4, line);

		rowY -= (ATASK_ROW_H + ATASK_ROW_GAP);
		shownRows++;
	}

	if (shownRows == 0)
	{
		iSetColor(255, 255, 255);
		iText(ATASK_ROW_X - 10, ATASK_ROW_TOP_Y + 30, (char*)"(none in stock)");
	}

	// Output reminder + Craft button
	iSetColor(0, 0, 0);
	char outLine[64];
	sprintf(outLine, "-> %s x%d", ITEM_DB[sel.outputItemID].name, sel.outputCount);
	iText(ATASK_ROW_X, CRAFT_PANEL_Y + 56, outLine);

	int bx, by, bw, bh;
	getArmorCraftButtonRect(bx, by, bw, bh);

	// Craft button background (pure white)
	iSetColor(255, 255, 255);
	iFilledRectangle(bx, by, bw, bh);

	// Button border (dark gray)
	iSetColor(50, 50, 50);
	iRectangle(bx, by, bw, bh);

	// CRAFT text (blue)
	iSetColor(0, 50, 200);
	iText(bx + bw / 2 - 22, by + bh / 2 - 5, (char*)"CRAFT");
}

// ---------------------------------------------------------------
//  CLICK HANDLING
// ---------------------------------------------------------------
inline void armorMenuOnClick(int mx, int my)
{
	if (!isArmorMenuOpen) return;

	if (g_selectedArmorRecipeIndex >= 0)
	{
		int bx, by, bw, bh;
		getArmorCraftButtonRect(bx, by, bw, bh);
		if (mx >= bx && mx <= bx + bw && my >= by && my <= by + bh)
		{
			craftItem(ARMOR_RECIPE_BOOK[g_selectedArmorRecipeIndex]);
			return;
		}
	}

	int slot = armorSlotAt(mx, my);
	if (slot >= 0)
	{
		g_selectedArmorRecipeIndex = (g_selectedArmorRecipeIndex == slot) ? -1 : slot;
		return;
	}

	bool insidePanel = (mx >= CRAFT_PANEL_X && mx <= CRAFT_PANEL_X + CRAFT_PANEL_W &&
		my >= CRAFT_PANEL_Y && my <= CRAFT_PANEL_Y + CRAFT_PANEL_H);
	if (!insidePanel)
		closeArmorMenu();
}
