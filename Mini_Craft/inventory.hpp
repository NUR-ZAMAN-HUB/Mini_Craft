// =====================================================================
//  inventory.hpp
// =====================================================================
//  Inventory / Slot-Management System for Mini Craft (2D top-down
//  survival game, iGraphics + Visual Studio 2013).
//
//  Holds the 4 gathered resources (Stone/Wood/Iron/Water) plus the
//  potions/scroll/coin the rest of the project already refers to
//  (CraftingEngine.hpp's RECIPE_BOOK uses these same ItemID names),
//  as one InventorySlot per ItemID - "array of structures to keep
//  track of item quantities", same idea as GatherSystem.hpp's own
//  g_inventory[4], just generalized to every item type.
//
//  Self-contained: does NOT touch iMain.cpp or any other existing
//  file, and does not change any ItemID/ITEM_DB/function name or
//  signature that CraftingEngine.hpp already relies on.
//
//  Usage:
//      initInventory();                    // call once, e.g. top of main()
//      loadInventoryTextures();            // call once, AFTER iInitialize()
//                                           //   (also loads the corner icon +
//                                           //   panel art used by the UI below)
//      addItem(ITEM_WOOD, 5);
//      if (hasResource(ITEM_WOOD, 5)) removeItem(ITEM_WOOD, 5);
//
//  Home Base UI (the corner icon on Home Base -> click it -> panel opens):
//      drawInventoryUI()                   // call every frame while on Home Base
//      handleInventoryClick(button, x, y)  // call from your mouse handler
//
//  Written for Visual Studio 2013 (no C++11 features used) + iGraphics.
// =====================================================================
#pragma once

#include "iGraphics.h"
#include "Menu.h"     // Button struct, IsInsideButton() - used by the corner icon below
#include <string.h>   // strncpy()
#include <stdio.h>    // sprintf() - handy for HUD text built from this file

// Defined later in HomeBase.hpp - the actual coin balance shown in the HUD
// (awarded on enemy kills, saved/loaded by SaveGame.hpp). ITEM_COIN in
// ITEM_DB below exists so a Recipe can list "coins" as an ingredient, but
// the real balance lives here, NOT in INVENTORY[] - see hasResource() /
// removeItem() / getItemCount() below, which redirect ITEM_COIN to this.
extern int g_playerCoins;

// Defined in GatherSystem.hpp (already included before this file in
// HomeBase.hpp) - the real Stone/Wood/Iron/Water counts, filled in by
// gathering resource nodes. ITEM_STONE/ITEM_WOOD/ITEM_IRON/ITEM_WATER in
// ITEM_DB below exist so a Recipe can list them as ingredients, but the
// real counts live here, NOT in INVENTORY[] - see hasResource()/removeItem()/
// getItemCount()/addItem() below, which redirect those 4 items to this
// (ResourceType's RES_STONE..RES_WATER share the same 0..3 values as
// ItemID's ITEM_STONE..ITEM_WATER, so g_inventory[itemID] just works).
extern int g_inventory[4];

// Defined in Fighters.hpp/HomeBase.hpp (both already included before this
// file in HomeBase.hpp) - the on-screen roster and which slot is currently
// controlled. Needed so equipping an armor piece from the inventory panel
// (see toggleEquipArmor() below) can actually change the active fighter's
// HP, not just sit as inventory bookkeeping.
extern Fighter roster[3];
extern int activeFighterIndex;

// ---------------------------------------------------------------
//  ITEM IDS
// ---------------------------------------------------------------
//  Keep this in sync with ITEM_DB[] below - the array index IS the ID.
//  (Unchanged from before - CraftingEngine.hpp's RECIPE_BOOK already
//  references these exact names.)
enum ItemID
{
	ITEM_STONE = 0,
	ITEM_WOOD,
	ITEM_IRON,
	ITEM_WATER,

	ITEM_HEALTH_POTION,
	ITEM_POWER_POTION,
	ITEM_MOVEMENT_POTION,
	ITEM_REINFORCEMENT_POTION,

	ITEM_IRON_ARMOR,
	ITEM_RUSTY_ARMOR,
	ITEM_STEEL_ARMOR,

	ITEM_DAMAGE_SCROLL,
	ITEM_COIN,

	ITEM_COUNT          // always last - total number of known item types
};

// ---------------------------------------------------------------
//  Item  - static "what is this item" data (name, icon, stack limit)
// ---------------------------------------------------------------
struct Item
{
	int  id;
	char name[32];
	char texturePath[96];   // sprite/icon path, loaded into iconTex by loadInventoryTextures()
	unsigned int iconTex;   // filled in at runtime by loadInventoryTextures(), 0 until then
	int  maxStack;          // 0 = unlimited
};

// ---------------------------------------------------------------
//  InventorySlot - "how many of this item does the player have"
// ---------------------------------------------------------------
struct InventorySlot
{
	int itemID;
	int quantity;
};

// ---------------------------------------------------------------
//  ITEM DATABASE  (index == ItemID, so ITEM_DB[ITEM_WOOD] just works)
//
//  Paths corrected to match the actual files under Images/ in this
//  project (previous version had "Images//Iron.png" - no such file,
//  the real one is "Images/Iron.jpg" - and "Images//coin.png" instead
//  of the real "Images/coin_icon.png"; the double "//" is harmless on
//  Windows but is cleaned up to a single "/" here too).
//  ITEM_DAMAGE_SCROLL has no dedicated art yet anywhere in Images/, so
//  it borrows spark_effect_yellow.png as a placeholder - swap in a real
//  "Damage_scroll.png" here the moment one is added, nothing else needs
//  to change.
// ---------------------------------------------------------------
static Item ITEM_DB[ITEM_COUNT] =
{
	{ ITEM_STONE,                "Stone",                "Images/Stone.jpg",                0, 0 },
	{ ITEM_WOOD,                 "Wood",                 "Images/Wood.jpg",                 0, 0 },
	{ ITEM_IRON,                 "Iron",                 "Images/Iron.jpg",                 0, 0 },
	{ ITEM_WATER,                "Water",                "Images/water.png",                0, 0 },

	{ ITEM_HEALTH_POTION,        "Health",        "Images/Health_potion.png",        0, 10 },
	{ ITEM_POWER_POTION,         "Power",         "Images/Power_potion.png",         0, 10 },
	{ ITEM_MOVEMENT_POTION,      "Movement",      "Images/Movement_potion.png",      0, 10 },
	{ ITEM_REINFORCEMENT_POTION, "Reinforcement", "Images/Reinforcement_potion.png", 0, 10 },

	// maxStack = 0 (unlimited) - previously capped at 1, which meant a
	// second craft of the same armor piece just clamped back to 1 instead
	// of incrementing (and still spent the Iron for nothing, since
	// craftItem() in CraftingEngine.hpp spends ingredients before checking
	// whether addItem() actually added anything). Now each successful
	// craft (as long as there's enough Iron) keeps adding +1, same
	// stacking behavior as potions.
	{ ITEM_IRON_ARMOR,           "Iron Armor",           "Images/Iron_Armor.png",           0, 0  },
	{ ITEM_RUSTY_ARMOR,          "Rusty Armor",          "Images/Rusty_Armor.png",          0, 0  },
	{ ITEM_STEEL_ARMOR,          "Steel Armor",          "Images/Steel_Armor.png",          0, 0  },

	{ ITEM_DAMAGE_SCROLL,        "Damage Scroll",        "Images/spark_effect_yellow.png",  0, 5  },
	{ ITEM_COIN,                 "Coin",                 "Images/coin_icon.png",            0, 0  },
};

// ---------------------------------------------------------------
//  INVENTORY  - one slot per known item type, indexed by ItemID.
//  (Simple "one stack per item" model - matches the way GatherSystem.hpp
//  already tracks resources, just generalized to cover potions/scrolls too.)
// ---------------------------------------------------------------
static InventorySlot INVENTORY[ITEM_COUNT];

// =====================================================================
//  HOME BASE UI STATE  -  corner icon (Images/inventory_icon.png) that
//  opens a slot panel on top of Images/inventory_craftingbg.png when
//  clicked. Declared up here so loadInventoryTextures() below can fill
//  the two textures in along with everything else, in one call.
// =====================================================================
static unsigned int g_invIconTexUI = 0;    // corner icon texture
static unsigned int g_invPanelTexUI = 0;   // panel background texture
static bool         g_inventoryPanelOpen = false;

// Bottom-right corner icon, 50x50, on an 800x600 Home Base screen - same
// corner/size the Attack button already uses (mirrored) in HomeBase.hpp.
static Button g_inventoryIconBtn = { 800 - 15 - 70, 15 + 10, 800 - 15, 15 + 10 + 70, "", 0 };

// Same panel size/position as the crafting-book panel (CraftingEngine.hpp's
// CRAFT_PANEL_* - 690x460, centered on the 800x600 Home Base screen). Defined
// here with the same literal numbers rather than reusing those macros, since
// inventory.hpp is included BEFORE CraftingEngine.hpp in HomeBase.hpp - keep
// both in sync if that panel size ever changes.
#define INV_PANEL_W        690
#define INV_PANEL_H        460
#define INV_PANEL_X        ((800 - INV_PANEL_W) / 2)
#define INV_PANEL_Y        ((600 - INV_PANEL_H) / 2)

// Two fixed rows: potions on top, armor below - each item always gets a
// slot (icon + white quantity box), even at 0, so the layout stays put.
#define INV_ROW_ICON_SIZE   80
#define INV_ROW_SLOT_GAP    40
#define INV_ROW_BOX_H       28
#define INV_ROW_BOX_GAP     6
// Widened from 150 so there's more breathing room between the potion row
// and the armor row below it.
#define INV_ROWS_VGAP       190

// ---------------------------------------------------------------
//  SETUP
// ---------------------------------------------------------------
// Call once (e.g. at the top of main(), before the game loop starts).
inline void initInventory()
{
	for (int i = 0; i < ITEM_COUNT; i++)
	{
		INVENTORY[i].itemID = i;
		INVENTORY[i].quantity = 0;
	}
}

// Call once, AFTER iInitialize(), so an OpenGL context already exists.
// Loads every item's icon texture from ITEM_DB[i].texturePath, plus the
// two extra textures the Home Base corner icon / panel UI needs below.
inline void loadInventoryTextures()
{
	for (int i = 0; i < ITEM_COUNT; i++)
	{
		ITEM_DB[i].iconTex = iLoadImage(ITEM_DB[i].texturePath);
	}

	g_invIconTexUI = iLoadImage((char*)"Images/inventory_icon.png");
	g_invPanelTexUI = iLoadImage((char*)"Images/inventory_craftingbg.png");
}

// ---------------------------------------------------------------
//  CORE INVENTORY OPERATIONS
// ---------------------------------------------------------------
// Adds `count` of itemID to the inventory. Respects maxStack (0 = no cap).
// Returns true if the full amount was added, false if it was clamped/rejected.
// ITEM_STONE/ITEM_WOOD/ITEM_IRON/ITEM_WATER are special-cased to the real
// gathered-resource pool (see the g_inventory extern above).
inline bool addItem(int itemID, int count)
{
	if (itemID < 0 || itemID >= ITEM_COUNT || count <= 0) return false;

	if (itemID == ITEM_STONE || itemID == ITEM_WOOD || itemID == ITEM_IRON || itemID == ITEM_WATER)
	{
		// ITEM_STONE/WOOD/IRON/WATER share the same 0..3 values as
		// ResourceType's RES_STONE..RES_WATER (see the extern g_inventory
		// comment above), so clampBasicResource() (GatherSystem.hpp) just
		// works here too - Stone/Wood/Iron cap at 500, Water stays uncapped.
		bool wasAtCap = (itemID != ITEM_WATER) && (g_inventory[itemID] >= MAX_BASIC_RESOURCE);
		g_inventory[itemID] += count;
		clampBasicResource((ResourceType)itemID);
		return !wasAtCap;
	}

	InventorySlot &slot = INVENTORY[itemID];
	int cap = ITEM_DB[itemID].maxStack;

	if (cap > 0 && slot.quantity + count > cap)
	{
		slot.quantity = cap;   // clamp instead of overflowing the stack
		return false;
	}

	slot.quantity += count;
	return true;
}

// Removes `count` of itemID from the inventory.
// Returns false (and changes nothing) if there isn't enough to remove.
// ITEM_COIN is special-cased to spend from g_playerCoins (see the extern above).
// ITEM_STONE/ITEM_WOOD/ITEM_IRON/ITEM_WATER are special-cased to spend from
// the real gathered-resource pool (see the g_inventory extern above).
inline bool removeItem(int itemID, int count)
{
	if (count <= 0) return false;

	if (itemID == ITEM_COIN)
	{
		if (g_playerCoins < count) return false;
		g_playerCoins -= count;
		return true;
	}

	if (itemID == ITEM_STONE || itemID == ITEM_WOOD || itemID == ITEM_IRON || itemID == ITEM_WATER)
	{
		if (g_inventory[itemID] < count) return false;
		g_inventory[itemID] -= count;
		return true;
	}

	if (itemID < 0 || itemID >= ITEM_COUNT) return false;

	InventorySlot &slot = INVENTORY[itemID];
	if (slot.quantity < count) return false;

	slot.quantity -= count;
	return true;
}

// True if the player currently holds at least requiredCount of itemID.
// ITEM_COIN is special-cased to check g_playerCoins (see the extern above).
// ITEM_STONE/ITEM_WOOD/ITEM_IRON/ITEM_WATER are special-cased to check the
// real gathered-resource pool (see the g_inventory extern above).
inline bool hasResource(int itemID, int requiredCount)
{
	if (itemID == ITEM_COIN) return g_playerCoins >= requiredCount;
	if (itemID == ITEM_STONE || itemID == ITEM_WOOD || itemID == ITEM_IRON || itemID == ITEM_WATER)
		return g_inventory[itemID] >= requiredCount;
	if (itemID < 0 || itemID >= ITEM_COUNT) return false;
	return INVENTORY[itemID].quantity >= requiredCount;
}

// Convenience getter, handy for HUD text (Stone: X, Wood: Y, ...).
// ITEM_COIN is special-cased to read g_playerCoins (see the extern above).
// ITEM_STONE/ITEM_WOOD/ITEM_IRON/ITEM_WATER are special-cased to read the
// real gathered-resource pool (see the g_inventory extern above).
inline int getItemCount(int itemID)
{
	if (itemID == ITEM_COIN) return g_playerCoins;
	if (itemID == ITEM_STONE || itemID == ITEM_WOOD || itemID == ITEM_IRON || itemID == ITEM_WATER)
		return g_inventory[itemID];
	if (itemID < 0 || itemID >= ITEM_COUNT) return 0;
	return INVENTORY[itemID].quantity;
}

// ---------------------------------------------------------------
//  ARMOR EQUIP  -  clicking an owned armor piece in the inventory panel
//  (see armorRowSlotAt()/handleInventoryClick() below) selects it as
//  "worn": it gets a highlighted border (drawInventorySlot() below) and
//  its HP bonus is applied to the active fighter (roster[activeFighterIndex]).
//  Clicking the already-equipped piece again un-equips it. Only one piece
//  can be worn at a time - equipping a new one automatically un-equips
//  whatever was worn before.
// ---------------------------------------------------------------
#define ARMOR_HP_BONUS_IRON_ARMOR    100
#define ARMOR_HP_BONUS_STEEL_ARMOR   60
#define ARMOR_HP_BONUS_RUSTY_ARMOR   20

// -1 = nothing currently worn, otherwise one of ITEM_IRON_ARMOR/
// ITEM_RUSTY_ARMOR/ITEM_STEEL_ARMOR.
static int g_equippedArmorItemID = -1;

inline int getArmorHealthBonus(int itemID)
{
	if (itemID == ITEM_IRON_ARMOR)  return ARMOR_HP_BONUS_IRON_ARMOR;
	if (itemID == ITEM_STEEL_ARMOR) return ARMOR_HP_BONUS_STEEL_ARMOR;
	if (itemID == ITEM_RUSTY_ARMOR) return ARMOR_HP_BONUS_RUSTY_ARMOR;
	return 0;
}

// Adds (or, with a negative delta, removes) an HP amount from the active
// fighter's max health, carrying current health along with it so gearing
// up always feels like a straight gain and gearing down clamps current
// health back down instead of leaving it floating above the new max.
inline void adjustActiveFighterHealth(int delta)
{
	Fighter &f = roster[activeFighterIndex];

	f.maxHealth += delta;
	if (f.maxHealth < 1) f.maxHealth = 1;

	f.currentHealth += delta;
	if (f.currentHealth > f.maxHealth) f.currentHealth = f.maxHealth;
	if (f.currentHealth < 0) f.currentHealth = 0;
}

// Called when an armor slot is clicked in the inventory panel. Ignored if
// the player doesn't actually own that piece yet (nothing to equip).
inline void toggleEquipArmor(int itemID)
{
	if (getItemCount(itemID) <= 0) return;

	if (g_equippedArmorItemID == itemID)
	{
		// Clicking the already-equipped piece again takes it off.
		adjustActiveFighterHealth(-getArmorHealthBonus(itemID));
		g_equippedArmorItemID = -1;
		return;
	}

	// Switching pieces (or equipping from bare) - drop the old bonus
	// first so bonuses never stack.
	if (g_equippedArmorItemID != -1)
		adjustActiveFighterHealth(-getArmorHealthBonus(g_equippedArmorItemID));

	g_equippedArmorItemID = itemID;
	adjustActiveFighterHealth(getArmorHealthBonus(itemID));
}

// ---------------------------------------------------------------
//  OPTIONAL DRAWING HELPER
// ---------------------------------------------------------------
// Draws one inventory slot: the item icon at (x, y), with a white
// quantity box directly beneath it (how many of that item you have)
// and the item's name above it. Used by the potion/armor rows in
// drawInventoryUI() below.
inline void drawInventorySlot(int itemID, int x, int y, int size)
{
	if (itemID < 0 || itemID >= ITEM_COUNT) return;

	// Icon
	if (ITEM_DB[itemID].iconTex != 0)
		iShowImage(x, y, size, size, ITEM_DB[itemID].iconTex);
	else
	{
		iSetColor(38, 38, 38);
		iFilledRectangle(x, y, size, size);
	}

	// Highlight border - drawn around whichever armor piece is currently
	// equipped (see toggleEquipArmor() above), so choosing one in the
	// panel gives clear visual feedback.
	if (itemID == g_equippedArmorItemID)
	{
		iSetColor(255, 215, 0);
		iRectangle(x - 3, y - 3, size + 6, size + 6);
		iRectangle(x - 4, y - 4, size + 8, size + 8);
	}

	// Name, just above the icon
	iSetColor(255, 255, 255);
	iText(x, y + size + 6, ITEM_DB[itemID].name);

	// White quantity box, directly below the icon
	int boxY = y - INV_ROW_BOX_H - INV_ROW_BOX_GAP;
	iSetColor(255, 255, 255);
	iFilledRectangle(x, boxY, size, INV_ROW_BOX_H);
	iSetColor(60, 60, 60);
	iRectangle(x, boxY, size, INV_ROW_BOX_H);

	char qty[16];
	sprintf(qty, "%d", INVENTORY[itemID].quantity);
	iSetColor(0, 0, 0);
	iText(x + size / 2 - 4, boxY + INV_ROW_BOX_H / 2 - 5, qty);
}

// Slot rect for item `index` of a row of `count` items drawn at
// icon-baseline `y` - shared by drawing AND click detection so a click
// lands on exactly the icon that's on screen (see armorRowSlotAt() below).
inline void getInventoryRowSlotRect(int count, int y, int index, int &x, int &slotY, int &w, int &h)
{
	int rowW = count * INV_ROW_ICON_SIZE + (count - 1) * INV_ROW_SLOT_GAP;
	int startX = INV_PANEL_X + (INV_PANEL_W - rowW) / 2;

	x = startX + index * (INV_ROW_ICON_SIZE + INV_ROW_SLOT_GAP);
	slotY = y;
	w = h = INV_ROW_ICON_SIZE;
}

// Draws a full row of item slots, horizontally centered in the inventory
// panel, at the given icon-baseline y. Shared by the potion row and the
// armor row in drawInventoryUI() below.
inline void drawInventoryItemRow(const int *itemIDs, int count, int y)
{
	for (int i = 0; i < count; i++)
	{
		int x, slotY, w, h;
		getInventoryRowSlotRect(count, y, i, x, slotY, w, h);
		drawInventorySlot(itemIDs[i], x, slotY, INV_ROW_ICON_SIZE);
	}
}

// Fixed row contents + Y positions - pulled out of drawInventoryUI() to
// file scope so armorRowSlotAt() (click detection) can share the exact
// same layout drawInventoryUI() draws.
static const int INV_POTION_ROW[] = { ITEM_HEALTH_POTION, ITEM_POWER_POTION, ITEM_MOVEMENT_POTION, ITEM_REINFORCEMENT_POTION };
static const int INV_ARMOR_ROW[]  = { ITEM_IRON_ARMOR, ITEM_RUSTY_ARMOR, ITEM_STEEL_ARMOR };
static const int INV_POTION_ROW_COUNT = 4;
static const int INV_ARMOR_ROW_COUNT  = 3;

#define INV_ROW1_Y  (INV_PANEL_Y + INV_PANEL_H - 170)   // leaves room for the "INVENTORY" title + item name above it
#define INV_ROW2_Y  (INV_ROW1_Y - INV_ROWS_VGAP)

// Which armor slot (0..INV_ARMOR_ROW_COUNT-1), if any, is under (mx, my)
// while the inventory panel is open. -1 if none.
inline int armorRowSlotAt(int mx, int my)
{
	for (int i = 0; i < INV_ARMOR_ROW_COUNT; i++)
	{
		int x, y, w, h;
		getInventoryRowSlotRect(INV_ARMOR_ROW_COUNT, INV_ROW2_Y, i, x, y, w, h);

		if (mx >= x && mx <= x + w && my >= y && my <= y + h)
			return i;
	}
	return -1;
}

// ---------------------------------------------------------------
//  HOME BASE UI  -  drawing + click handling
// ---------------------------------------------------------------
// Call every frame while the player is on Home Base (after everything
// else, so the icon/panel draw on top of the scene). Draws the corner
// icon always; the panel + its slots only while g_inventoryPanelOpen.
inline void drawInventoryUI()
{
	iShowImage(g_inventoryIconBtn.x1, g_inventoryIconBtn.y1,
		g_inventoryIconBtn.x2 - g_inventoryIconBtn.x1, g_inventoryIconBtn.y2 - g_inventoryIconBtn.y1,
		g_invIconTexUI);

	if (!g_inventoryPanelOpen) return;
	iShowImage(INV_PANEL_X, INV_PANEL_Y, INV_PANEL_W, INV_PANEL_H, g_invPanelTexUI);

	iSetColor(255, 255, 255);
	{
		// Centered, bold "INVENTORY" title - same look/position as the
		// "ARMOR" / "POTION" titles in ArmorEngine.hpp / CraftingEngine.hpp
		// (bold faked by stamping the text twice, 1px apart).
		char* title = (char*)"INVENTORY";
		int titleX = INV_PANEL_X + INV_PANEL_W / 2 - 60;
		int titleY = INV_PANEL_Y + INV_PANEL_H - 40;
		iText(titleX,     titleY, title, GLUT_BITMAP_TIMES_ROMAN_24);
		iText(titleX + 1, titleY, title, GLUT_BITMAP_TIMES_ROMAN_24);
	}

	// Row 1: all 4 potions. Row 2: all 3 armor pieces. Fixed slots (shown
	// even at 0 quantity) so the layout never shifts around.
	drawInventoryItemRow(INV_POTION_ROW, INV_POTION_ROW_COUNT, INV_ROW1_Y);
	drawInventoryItemRow(INV_ARMOR_ROW, INV_ARMOR_ROW_COUNT, INV_ROW2_Y);
}

// Call from your mouse handler (e.g. HomeBase_OnMouseDown). Left-clicking
// the corner icon toggles the panel open/closed. While the panel is open,
// left-clicking an owned armor piece selects/equips it (gold highlight
// border, see drawInventorySlot()) and applies its HP bonus to the active
// fighter - clicking the equipped piece again un-equips it, see
// toggleEquipArmor() above.
inline void handleInventoryClick(int button, int mx, int my)
{
	if (button != GLUT_LEFT_BUTTON) return;

	if (IsInsideButton(g_inventoryIconBtn, mx, my))
	{
		g_inventoryPanelOpen = !g_inventoryPanelOpen;
		return;
	}

	if (!g_inventoryPanelOpen) return;

	int armorSlot = armorRowSlotAt(mx, my);
	if (armorSlot >= 0)
		toggleEquipArmor(INV_ARMOR_ROW[armorSlot]);
}
