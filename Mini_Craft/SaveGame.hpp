// =====================================================================
//  SaveGame.hpp
// =====================================================================
//  Plain file-based save/load for Mini Craft.
//
//  WHAT GETS SAVED (one line per value, fixed order, plain text -
//  open savegame.txt in Notepad if you ever need to debug it):
//      characterNumber, activeFighterIndex, currentLevel, difficulty,
//      g_playerCoins, g_inventory[4] (Stone/Wood/Iron/Water - the
//      counters GatherSystem.hpp actually draws in the HUD), and every
//      INVENTORY[] quantity from inventory.hpp (potions/scroll/coin
//      slot - whatever CraftingEngine.hpp has produced so far).
//
//  WHAT DOESN'T GET SAVED (on purpose): hero health/position inside
//  Home Base. Those already reset to full every "new day" in
//  HomeBase.hpp's own win/lose handling, so there's nothing meaningful
//  to persist there - re-loading always drops the player back in at
//  the same spawn point HomeBase_Init() already uses.
//
//  HOW IT'S WIRED (see iMain.cpp):
//      - LoadGame() is called once in main(), right AFTER
//        HomeBase_Init() (so initInventory()/g_inventory are already
//        zeroed before this overwrites them with saved values).
//      - SaveGame() is called: the moment the player first reaches
//        Home Base (portal step-through in GAMEPLAY), every time they
//        leave Home Base back to the Menu ('M'), on a ~3s autosave
//        timer while inside Home Base (safety net for Alt+F4 / the
//        window's X button), and right before the Exit button quits.
//      - g_hasSaveData tells the Menu's Play button whether to resume
//        straight into Home Base or start a fresh CHARACTER_SELECT run.
//      - ResetSaveGame() is wired to the existing "Reset Level" button
//        (LEVEL_SELECT screen) - wipes the file AND every global back
//        to a brand new game, per Raya's spec: nothing resets until
//        that button is clicked by hand.
//
//  Included from HomeBase.hpp, right after g_playerCoins/
//  activeFighterIndex are declared and after inventory.hpp - needs all
//  three (plus characterNumber/currentLevel/difficulty from Menu.h,
//  already extern'd there) to exist first. Same single-TU assumption
//  GatherSystem.hpp's own g_inventory already relies on.
// =====================================================================
#pragma once

#include <stdio.h>

#define SAVE_FILE_PATH "savegame.txt"

// True once a save file has been written/loaded this run. The Menu's
// Play button checks this to decide "resume into Home Base" vs
// "start a fresh CHARACTER_SELECT run".
bool g_hasSaveData = false;

// True if a save file exists on disk. Cheap existence check only -
// doesn't read or change anything.
inline bool HasSaveGame()
{
	FILE* f = fopen(SAVE_FILE_PATH, "r");
	if (f == NULL) return false;
	fclose(f);
	return true;
}

// Writes every piece of persistent progress to disk. Safe to call
// often - it's a handful of ints, so overwriting it on a timer costs
// nothing. Does nothing if no character has been confirmed yet (i.e.
// there's no real run to save - stops a bare visit to the Menu from
// creating a bogus "empty" save).
inline void SaveGame()
{
	// characterNumber is set as soon as a card is clicked on CHARACTER_SELECT (before
	// Confirm), and currentLevel only goes above 1 once the portal has been walked
	// through (iMain.cpp) - so requiring both (for a run with no save yet) means a
	// card click + Exit can't create a bogus save. But once a real save DOES exist
	// (g_hasSaveData), dying/resetting legitimately drops currentLevel back to 1 -
	// that wiped state still needs to overwrite the file, or the old pre-death
	// numbers would come back the next time LoadGame() runs.
	if (characterNumber < 0 || (currentLevel < 2 && !g_hasSaveData)) return;

	FILE* f = fopen(SAVE_FILE_PATH, "w");
	if (f == NULL) return;   // disk/permission problem - fail silently, keep playing

	fprintf(f, "%d\n", characterNumber);
	fprintf(f, "%d\n", activeFighterIndex);
	fprintf(f, "%d\n", currentLevel);
	fprintf(f, "%d\n", difficulty);
	fprintf(f, "%d\n", g_playerCoins);

	// Stone, Wood, Iron, Water - GatherSystem.hpp's g_inventory[4]
	fprintf(f, "%d %d %d %d\n", g_inventory[0], g_inventory[1], g_inventory[2], g_inventory[3]);

	// Every general-inventory quantity (potions/scroll/coin slot),
	// same fixed ITEM_COUNT order every time.
	for (int i = 0; i < ITEM_COUNT; i++)
		fprintf(f, "%d ", INVENTORY[i].quantity);
	fprintf(f, "\n");

	fclose(f);
	g_hasSaveData = true;
}

// Reads the save file (if any) into every global listed above. Call
// once at startup, AFTER HomeBase_Init() has already zeroed things
// out. Returns false (and touches nothing) if there's no save yet -
// every global just keeps its normal fresh-game default.
inline bool LoadGame()
{
	FILE* f = fopen(SAVE_FILE_PATH, "r");
	if (f == NULL) { g_hasSaveData = false; return false; }

	// Everything is read into locals first and only copied into the real globals once the
	// whole header has parsed and passed a range check - a half-written or hand-edited
	// file then can't leave a mix of saved/default values or an out-of-range hero index
	// (characterNumber indexes CH[] in GAMEPLAY). The trailing INVENTORY[] quantities are
	// allowed to run short (a save from before a new item type was added) - missing ones stay 0.
	int ch, fighter, level, diff, coins;
	int res[4];
	int qty[ITEM_COUNT];
	for (int i = 0; i < ITEM_COUNT; i++) qty[i] = 0;

	bool ok = true;
	ok = ok && (fscanf(f, "%d", &ch) == 1);
	ok = ok && (fscanf(f, "%d", &fighter) == 1);
	ok = ok && (fscanf(f, "%d", &level) == 1);
	ok = ok && (fscanf(f, "%d", &diff) == 1);
	ok = ok && (fscanf(f, "%d", &coins) == 1);
	ok = ok && (fscanf(f, "%d %d %d %d", &res[0], &res[1], &res[2], &res[3]) == 4);

	if (ok)
		for (int i = 0; i < ITEM_COUNT; i++)
			if (fscanf(f, "%d", &qty[i]) != 1) break;

	fclose(f);

	if (ok && (ch < 0 || ch > 2 || fighter < 0 || fighter > 2 || level < 1 || diff < 0 || diff > 2))
		ok = false;

	if (!ok) { g_hasSaveData = false; return false; }

	characterNumber = ch;
	activeFighterIndex = fighter;
	currentLevel = level;
	difficulty = diff;
	g_playerCoins = (coins > 0) ? coins : 0;

	for (int i = 0; i < 4; i++) g_inventory[i] = (res[i] > 0) ? res[i] : 0;
	for (int i = 0; i < ITEM_COUNT; i++) INVENTORY[i].quantity = (qty[i] > 0) ? qty[i] : 0;

	g_hasSaveData = true;
	return true;
}

// "Reset Level" - deletes the save file and puts every global back to
// a brand-new-game state. Wired to resetLevelBtn in iMain.cpp; this is
// the ONLY thing that should ever zero out coins/resources/level.
inline void ResetSaveGame()
{
	remove(SAVE_FILE_PATH);

	characterNumber = -1;
	activeFighterIndex = CHAR_GUARDIAN;
	currentLevel = 1;
	difficulty = 0;
	g_playerCoins = 0;

	for (int i = 0; i < 4; i++) g_inventory[i] = 0;
	for (int i = 0; i < ITEM_COUNT; i++) INVENTORY[i].quantity = 0;

	g_hasSaveData = false;
}
