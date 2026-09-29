// =====================================================================
//  Battle.hpp
// =====================================================================
//  A wave fight: the same Fighter you're playing in Home Base
//  (roster[activeFighterIndex], from HomeBase.hpp) against a row of
//  enemies built from Enemy.hpp. Reached by clicking the Attack button
//  in the bottom-left corner of the Home Base screen (see HomeBase.hpp).
//
//  ENEMY COUNT: this is now a wave system within ONE battle, not a ramp
//  across separate battles. Each battle starts with a wave of 1 enemy;
//  once every enemy in the current wave is dead, the next wave spawns
//  with one more enemy than the last (2, then 3, ... up to 6). Once a
//  wave of 6 is cleared, the battle is won and iMain.cpp sends the
//  player back to Home Base. Boss battles (Monster) skip all of this -
//  they're always a single solo enemy, no waves. Each wave's enemies
//  spawn at random positions within x:[80,680] y:[0,425] (see
//  randomSpawnX()/randomSpawnY()).
//
//  SKELETON ART: skeleton_walk_left_1..4.png, skeleton_walk_right_1..4.png,
//  skeleton_attack_left_1..4.png, skeleton_attack_right_1..4.png are loaded
//  once (loadSkeletonSpritesOnce()) and shared by every Skeleton instance -
//  see drawBattleEnemy() below. There's no dedicated idle art, so the first
//  walk frame is reused for idle. Zombie/Monster still have no art at all,
//  so they keep drawing as the colored placeholder box.
//
//  Arena background is "Images/Arena_1.png", loaded lazily on first use.
//
//  iMain.cpp calls into this the same way it does HomeBase.hpp:
//
//      Battle_Init()          -> call once, right when BATTLE starts
//                                 (see the LOADING branch of fixedUpdate())
//      Battle_FixedUpdate()   -> call every tick while in BATTLE; returns
//                                 true once iMain.cpp should leave BATTLE
//      Battle_Draw()          -> call every frame while in BATTLE
//      Battle_OnMouseDown()   -> call from iMouse() while in BATTLE
// =====================================================================
#pragma once

#include "iGraphics.h"
#include "Menu.h"
#include "HomeBaseConfig.hpp"
#include "Fighters.hpp"
#include "Enemy.hpp"
#include "HomeBase.hpp"   // roster[] / activeFighterIndex - the player's fighter, g_homeRightClickPending
#include <stdlib.h>       // rand() - see randomSpawnX()/randomSpawnY() below
#include <time.h>         // time() - seeds rand() once, see Battle_Init()
#include <string.h>       // strlen() - used by drawBattlePotionSelectMenu() below

// ---------------------------------------------------------------
//  STATE
// ---------------------------------------------------------------
#define BATTLE_MAX_ENEMIES 6

Enemy battleEnemies[BATTLE_MAX_ENEMIES];
int   battleEnemyCount = 1;    // how many enemies are in the CURRENT wave
int   battleWave = 1;          // current wave number (1..BATTLE_MAX_ENEMIES)
bool  battleIsBossFight = false;
EnemyType battleEnemyType = ENEMY_ZOMBIE;

// Counts every battle started from Home Base's Attack button (not counting
// forced Boss-button fights - see Battle_Init()). Used to cycle the grunt
// type instead of currentLevel: currentLevel only advances by finishing an
// overworld map and walking through its portal (iMain.cpp), which is a
// separate progression loop from Home Base battles. Rolling off currentLevel
// meant a player who just kept clicking Attack in Home Base without ever
// touching the overworld could get stuck fighting the same grunt type
// forever - starting at currentLevel==1 (odd) always rolls Zombie, and it
// can never become Skeleton without an overworld portal trip - so
// g_hasWonSkeletonRound could never flip true and the Boss button could
// never appear. Rolling off this counter instead guarantees both grunt
// types (and eventually the boss) are reachable from Home Base alone.
int g_battleAttemptCount = 0;

// g_forceBossFight / g_hasWonSkeletonRound / g_hasWonZombieRound are declared
// in HomeBase.hpp (included below) since its Boss button needs them too, and
// HomeBase.hpp is the one included first from iMain.cpp.

// Arena backgrounds. Loaded lazily the first time Battle_Init() runs (rather
// than every battle) since iLoadImage() only needs to happen once per file.
// Boss fights (battleIsBossFight - whether reached via the level roll or the
// Boss button) use arenaBackgroundBossID (Images/Arena_2.png) instead of the
// regular arenaBackgroundID (Images/Arena_1.png) - see Battle_Draw().
unsigned int arenaBackgroundID = 0;
unsigned int arenaBackgroundBossID = 0;

// Skeleton walk/attack textures - loaded once and shared by every Skeleton
// instance this session (see loadSkeletonSpritesOnce() / applySkeletonSprites()).
unsigned int skeletonWalkTex[2][ENEMY_WALK_MAX_FRAMES];
unsigned int skeletonAttackTex[2][ENEMY_ATTACK_MAX_FRAMES];
bool skeletonSpritesLoaded = false;

enum BattleOutcome { BATTLE_FIGHTING, BATTLE_WON, BATTLE_LOST };
BattleOutcome battleOutcome = BATTLE_FIGHTING;
int battleOutcomeTimer = 0;
const int BATTLE_OUTCOME_DURATION = 90;   // ~1.5s (fixedUpdate ticks roughly every 16ms) - how
                                           // long "VICTORY!"/"DEFEATED..." shows before returning

// ---------------------------------------------------------------
//  SKELETON TAUNT SCENE
//  Plays once "VICTORY!" finishes, but only after winning a *regular*
//  (non-boss) Skeleton round - see Battle_FixedUpdate(). Three Skeletons
//  stand facing the hero and taunt him, line by line, click to advance
//  (see Battle_OnMouseDown()). The click that dismisses the last line
//  is what actually ends the battle and sends the player back to Home
//  Base. Boss fights and Zombie rounds skip this entirely.
// ---------------------------------------------------------------
bool  g_skeletonTauntActive = false;
bool  g_skeletonTauntTrioReady = false;
bool  g_skeletonTauntDone = false;   // just finished - tells Battle_FixedUpdate() to end the battle, once
int   g_skeletonTauntLine = 0;
Enemy g_skeletonTauntTrio[3];

const char* SKELETON_TAUNT_LINES[] = {
	"This is not over, hero.",
	"You've only broken our bones - not our will!",
	"We will call our BOSS...",
	"...and you will wish you had fled tonight."
};
const int SKELETON_TAUNT_LINE_COUNT = 4;

// setupSkeletonTauntTrio() (builds these three) is defined just below
// applySkeletonSprites(), since it needs that function.

// How long the hero keeps facing the enemy it just attacked. handleMovement()
// (Fighters.hpp) unconditionally overwrites facing based on movement keys, so
// without this the "look at the enemy" turn from an attack would get flipped
// back on the very next tick if movement keys were still held. This holds the
// attack-triggered facing for BATTLE_FACE_LOCK_MS regardless of movement input.
unsigned long battleFaceLockUntil = 0;
FacingDir battleLockedFacing = FACE_RIGHT;
const unsigned long BATTLE_FACE_LOCK_MS = 900;

// Player spawn point, and the random spawn range for enemies each wave.
const float BATTLE_PLAYER_START_X = 160.0f, BATTLE_PLAYER_START_Y = 300.0f;
#define BATTLE_ENEMY_SPAWN_X_MIN 80
#define BATTLE_ENEMY_SPAWN_X_MAX 680
#define BATTLE_ENEMY_SPAWN_Y_MIN 0
#define BATTLE_ENEMY_SPAWN_Y_MAX 425

inline float randomSpawnX() { return (float)(BATTLE_ENEMY_SPAWN_X_MIN + rand() % (BATTLE_ENEMY_SPAWN_X_MAX - BATTLE_ENEMY_SPAWN_X_MIN + 1)); }
inline float randomSpawnY() { return (float)(BATTLE_ENEMY_SPAWN_Y_MIN + rand() % (BATTLE_ENEMY_SPAWN_Y_MAX - BATTLE_ENEMY_SPAWN_Y_MIN + 1)); }

// ---------------------------------------------------------------
//  POTION HOTKEYS  -  1 = Health, 2 = Movement, 3 = Power, 4 = Reinforcement
//  The hotkey logic, timers, icons and the "<Potion> Potion activated" banner
//  are shared with Home Base and live in PotionSystem.hpp (included through
//  HomeBase.hpp): Potion_HotkeysUpdate() / Potion_Use() / Potion_DrawHUD().
//  Battle_Init() clears them, so a buff never carries into the next fight.
// ---------------------------------------------------------------

// ---------------------------------------------------------------
//  POTION SELECT MENU  -  right-click opens a panel (pausing combat -
//  see Battle_FixedUpdate()) listing every potion currently in stock;
//  left-clicking one drinks it, same effect/duration as the '1'-'4'
//  hotkeys above (both paths funnel through Potion_Use()
//  below). Reuses CraftingEngine.hpp's CRAFT_PANEL_* rect/background
//  (already available here via the HomeBase.hpp include chain) so it
//  looks consistent with the crafting book / inventory panel.
//  Attacking used to also fire on right-click (see the old
//  Battle_OnMouseDown()) - that's now space-bar only inside Battle so
//  right-click is free for this menu; Home Base's own right-click-to-
//  attack is untouched (separate flag, separate handler).
// ---------------------------------------------------------------
bool g_battlePotionMenuOpen = false;

// ---------------------------------------------------------------
//  POTION SELECT MENU  -  drawing + click handling (opened/closed from
//  Battle_OnMouseDown() below via right-click).
// ---------------------------------------------------------------
#define BPSEL_SLOT_SIZE   100
#define BPSEL_SLOT_GAP    50

// Slots in a single row, horizontally + vertically centered in the shared
// CRAFT_PANEL_* rect (CraftingEngine.hpp) - same layout idea as the
// crafting book's own slot row.
inline void getBattlePotionSelectSlotRect(int index, int count, int &x, int &y, int &w, int &h)
{
	w = h = BPSEL_SLOT_SIZE;
	int rowW = count * BPSEL_SLOT_SIZE + (count - 1) * BPSEL_SLOT_GAP;
	int startX = CRAFT_PANEL_X + (CRAFT_PANEL_W - rowW) / 2;
	x = startX + index * (BPSEL_SLOT_SIZE + BPSEL_SLOT_GAP);
	y = CRAFT_PANEL_Y + (CRAFT_PANEL_H - BPSEL_SLOT_SIZE) / 2;
}

// Fills outSlots with every owned potion SLOT index (0..3) that's currently
// in stock (getItemCount() > 0), Health/Power/Movement/Reinforcement order.
// Returns how many were filled (0..4 - outSlots must hold 4).
inline int getOwnedBattlePotionSelection(int outSlots[POTION_SLOT_COUNT])
{
	int n = 0;
	for (int i = 0; i < POTION_SLOT_COUNT; i++)
		if (getItemCount(ITEM_HEALTH_POTION + i) > 0) outSlots[n++] = i;
	return n;
}

inline void drawBattlePotionSelectMenu()
{
	if (!g_battlePotionMenuOpen) return;

	if (g_invPanelTexUI != 0)
		iShowImage(CRAFT_PANEL_X, CRAFT_PANEL_Y, CRAFT_PANEL_W, CRAFT_PANEL_H, g_invPanelTexUI);
	else
	{
		iSetColor(13, 13, 20);
		iFilledRectangle(CRAFT_PANEL_X, CRAFT_PANEL_Y, CRAFT_PANEL_W, CRAFT_PANEL_H);
	}

	iSetColor(0, 0, 0);
	{
		char* title = (char*)"CHOOSE POTION";
		int titleX = CRAFT_PANEL_X + CRAFT_PANEL_W / 2 - 100;
		int titleY = CRAFT_PANEL_Y + CRAFT_PANEL_H - 40;
		iText(titleX,     titleY, title, GLUT_BITMAP_TIMES_ROMAN_24);
		iText(titleX + 1, titleY, title, GLUT_BITMAP_TIMES_ROMAN_24);
	}

	int slots[POTION_SLOT_COUNT];
	int count = getOwnedBattlePotionSelection(slots);

	if (count == 0)
	{
		iSetColor(0, 0, 0);
		iText(CRAFT_PANEL_X + CRAFT_PANEL_W / 2 - 130, CRAFT_PANEL_Y + CRAFT_PANEL_H / 2,
			(char*)"(no potions in stock - craft some first)");
		return;
	}

	static const int slotItemID[POTION_SLOT_COUNT] = { ITEM_HEALTH_POTION, ITEM_POWER_POTION, ITEM_MOVEMENT_POTION, ITEM_REINFORCEMENT_POTION };

	for (int i = 0; i < count; i++)
	{
		int x, y, w, h;
		getBattlePotionSelectSlotRect(i, count, x, y, w, h);
		int itemID = slotItemID[slots[i]];

		unsigned int tex = ITEM_DB[itemID].iconTex;
		if (tex != 0)
			iShowImage(x, y, w, h, tex);
		else
		{
			iSetColor(38, 38, 38);
			iFilledRectangle(x, y, w, h);
		}

		char qty[8];
		sprintf(qty, "x%d", getItemCount(itemID));
		iSetColor(0, 0, 0);
		iText(x + w / 2 - (int)strlen(ITEM_DB[itemID].name) * 4, y - 20, ITEM_DB[itemID].name);
		iText(x + w - 26, y + 4, qty);
	}
}

// Call from Battle_OnMouseDown() (left-click) while g_battlePotionMenuOpen
// is true. Clicking a slot drinks that potion (Potion_Use()) and
// closes the menu; clicking outside the panel just closes it, nothing spent.
inline void battlePotionMenuOnClick(int mx, int my, Fighter &player)
{
	if (!g_battlePotionMenuOpen) return;

	int slots[POTION_SLOT_COUNT];
	int count = getOwnedBattlePotionSelection(slots);

	for (int i = 0; i < count; i++)
	{
		int x, y, w, h;
		getBattlePotionSelectSlotRect(i, count, x, y, w, h);
		if (mx >= x && mx <= x + w && my >= y && my <= y + h)
		{
			Potion_Use(slots[i], player);
			g_battlePotionMenuOpen = false;   // combat resumes - see Battle_FixedUpdate()'s pause check
			return;
		}
	}

	bool insidePanel = (mx >= CRAFT_PANEL_X && mx <= CRAFT_PANEL_X + CRAFT_PANEL_W &&
		my >= CRAFT_PANEL_Y && my <= CRAFT_PANEL_Y + CRAFT_PANEL_H);
	if (!insidePanel)
		g_battlePotionMenuOpen = false;
}

// ---------------------------------------------------------------
//  SKELETON SPRITE LOADING
//  Filenames as given: skeleton_walk_left_1.png .. _4.png,
//  skeleton_walk_right_1.png .. _4.png, skeleton_attack_left_1.png .. _4.png,
//  skeleton_attack_right_1.png .. _4.png (no leading zero on the frame number -
//  note this is a different convention from Fighters.hpp/loadEnemyFrames()'s
//  "_0N" padding, so it's a dedicated loader here rather than reusing those).
// ---------------------------------------------------------------
inline void loadSkeletonSpritesOnce()
{
	if (skeletonSpritesLoaded) return;
	skeletonSpritesLoaded = true;

	char path[160];
	for (int frame = 1; frame <= ENEMY_WALK_MAX_FRAMES; frame++)
	{
		sprintf(path, "Images/skeleton_walk_left_%d.png", frame);
		skeletonWalkTex[ENEMY_FACE_LEFT][frame - 1] = iLoadImage(path);

		sprintf(path, "Images/skeleton_walk_right_%d.png", frame);
		skeletonWalkTex[ENEMY_FACE_RIGHT][frame - 1] = iLoadImage(path);
	}
	for (int frame = 1; frame <= ENEMY_ATTACK_MAX_FRAMES; frame++)
	{
		sprintf(path, "Images/skeleton_attack_left_%d.png", frame);
		skeletonAttackTex[ENEMY_FACE_LEFT][frame - 1] = iLoadImage(path);

		sprintf(path, "Images/skeleton_attack_right_%d.png", frame);
		skeletonAttackTex[ENEMY_FACE_RIGHT][frame - 1] = iLoadImage(path);
	}
}

// Points one Skeleton's walkTex/attackTex/idleTex at the shared textures
// above (no per-instance loading - the same 16 frames are reused by every
// Skeleton on screen).
inline void applySkeletonSprites(Enemy &e)
{
	for (int dir = 0; dir < 2; dir++)
	{
		for (int f = 0; f < ENEMY_WALK_MAX_FRAMES; f++)   e.walkTex[dir][f] = skeletonWalkTex[dir][f];
		for (int f = 0; f < ENEMY_ATTACK_MAX_FRAMES; f++) e.attackTex[dir][f] = skeletonAttackTex[dir][f];
		e.idleTex[dir] = skeletonWalkTex[dir][0];   // no dedicated idle art yet - reuse first walk frame
	}
}

// Builds the three taunting Skeletons once per scene (initEnemy() gives
// them real stats/position/idle state, applySkeletonSprites() above points
// them at the same shared walk/attack textures every other Skeleton uses -
// see loadSkeletonSpritesOnce()). They default to facing left (initEnemy(),
// Enemy.hpp), which is already toward the hero here.
inline void setupSkeletonTauntTrio()
{
	if (g_skeletonTauntTrioReady) return;
	g_skeletonTauntTrioReady = true;

	float xs[3] = { 520.0f, 600.0f, 680.0f };
	for (int i = 0; i < 3; i++)
	{
		initEnemy(g_skeletonTauntTrio[i], ENEMY_SKELETON, xs[i], BATTLE_PLAYER_START_Y);
		applySkeletonSprites(g_skeletonTauntTrio[i]);
	}
}

// ---------------------------------------------------------------
//  DRAGON (BOSS) SPRITE LOADING
//  Filenames as given: Dragon_walk_L1.png/_L2.png, Dragon_walk_R1.png/_R2.png
//  (2 walk frames/side - note the different naming convention from the
//  skeleton's "_left_1" style, so this gets its own loader), and the
//  Dragon's basic attack is a scratch: Dragon_scratch_L1..L3.png,
//  Dragon_scratch_R1..R3.png (3 frames/side). See ENEMY_STATS' "dragon"
//  row in Enemy.hpp for the matching walkFrames=2/atkFrames=3 counts.
// ---------------------------------------------------------------
unsigned int dragonWalkTex[2][ENEMY_WALK_MAX_FRAMES];
unsigned int dragonScratchTex[2][ENEMY_ATTACK_MAX_FRAMES];
bool dragonSpritesLoaded = false;

inline void loadDragonSpritesOnce()
{
	if (dragonSpritesLoaded) return;
	dragonSpritesLoaded = true;

	char path[160];
	for (int frame = 1; frame <= 2; frame++)
	{
		sprintf(path, "Images/Dragon_walk_L%d.png", frame);
		dragonWalkTex[ENEMY_FACE_LEFT][frame - 1] = iLoadImage(path);

		sprintf(path, "Images/Dragon_walk_R%d.png", frame);
		dragonWalkTex[ENEMY_FACE_RIGHT][frame - 1] = iLoadImage(path);
	}
	for (int frame = 1; frame <= 3; frame++)
	{
		sprintf(path, "Images/Dragon_scratch_L%d.png", frame);
		dragonScratchTex[ENEMY_FACE_LEFT][frame - 1] = iLoadImage(path);

		sprintf(path, "Images/Dragon_scratch_R%d.png", frame);
		dragonScratchTex[ENEMY_FACE_RIGHT][frame - 1] = iLoadImage(path);
	}
}

// Points one Dragon (ENEMY_MONSTER)'s walkTex/attackTex/idleTex at the
// shared textures above - same idea as applySkeletonSprites().
inline void applyDragonSprites(Enemy &e)
{
	for (int dir = 0; dir < 2; dir++)
	{
		for (int f = 0; f < 2; f++) e.walkTex[dir][f] = dragonWalkTex[dir][f];
		for (int f = 0; f < 3; f++) e.attackTex[dir][f] = dragonScratchTex[dir][f];
		e.idleTex[dir] = dragonWalkTex[dir][0];   // no dedicated idle art yet - reuse first walk frame
	}
}

// ---------------------------------------------------------------
//  DRAGON SUPER MOVE - FIRE BREATH
//  Every DRAGON_FIRE_INTERVAL_MS the Dragon stops moving/scratching, opens
//  its mouth (Fire_stand_l.png / Fire_stand_r.png, picked by facing) for
//  DRAGON_FIRE_WINDUP_MS, then launches one Fire.png bolt straight at
//  wherever the player was standing the instant the windup finished.
//  Self-contained here (not folded into Enemy.hpp) since it's a one-off
//  boss mechanic, not something every Enemy needs.
// ---------------------------------------------------------------
#define DRAGON_FIRE_INTERVAL_MS   10000   // time between super moves
#define DRAGON_FIRE_WINDUP_MS       600   // how long the mouth-open pose holds before the bolt launches
#define DRAGON_FIRE_SPEED           6.0f  // pixels per fixedUpdate() tick
// Hits harder than the Dragon's scratch (MONSTER_DAMAGE, currently 45 - see
// Enemy.hpp) since this is the boss's big telegraphed super move, not its
// basic attack - it should feel like the one to respect/dodge.
#define DRAGON_FIRE_DAMAGE           60
#define DRAGON_FIRE_SIZE             50   // on-screen size of the Fire.png bolt

unsigned int dragonFireStandTex[2] = { 0, 0 };   // [ENEMY_FACE_LEFT/RIGHT] mouth-open windup pose
unsigned int dragonFireBoltTex = 0;              // Images/Fire.png - the bolt itself
bool         dragonFireTexturesLoaded = false;

inline void loadDragonFireTexturesOnce()
{
	if (dragonFireTexturesLoaded) return;
	dragonFireTexturesLoaded = true;

	dragonFireStandTex[ENEMY_FACE_LEFT] = iLoadImage("Images/Fire_stand_l.png");
	dragonFireStandTex[ENEMY_FACE_RIGHT] = iLoadImage("Images/Fire_stand_r.png");
	dragonFireBoltTex = iLoadImage("Images/Fire.png");
}

unsigned long g_dragonNextFireTime = 0;      // GetTickCount() timestamp of the next windup
bool          g_dragonFireWindingUp = false; // true while the mouth-open pose is showing
unsigned long g_dragonFireWindupStart = 0;

struct DragonFireBolt { bool active; float x, y, vx, vy; };
DragonFireBolt g_dragonFire = { false, 0, 0, 0, 0 };

// Call once whenever a boss fight starts (see Battle_Init()) so the first
// super move is DRAGON_FIRE_INTERVAL_MS after the fight begins, not left
// over from a previous battle.
inline void resetDragonFireState()
{
	g_dragonNextFireTime = GetTickCount() + DRAGON_FIRE_INTERVAL_MS;
	g_dragonFireWindingUp = false;
	g_dragonFire.active = false;
}

// Call once per tick while the boss fight is going (see Battle_FixedUpdate()).
// Handles the windup timer, launching the bolt, and moving/landing it.
inline void updateDragonFire(Enemy &dragon, Fighter &player)
{
	unsigned long now = GetTickCount();

	if (g_dragonFireWindingUp)
	{
		if (now - g_dragonFireWindupStart >= DRAGON_FIRE_WINDUP_MS)
		{
			g_dragonFireWindingUp = false;

			float dx = player.x - dragon.x, dy = player.y - dragon.y;
			float dist = sqrtf(dx * dx + dy * dy);
			if (dist < 1.0f) dist = 1.0f;

			g_dragonFire.active = true;
			g_dragonFire.x = dragon.x;
			g_dragonFire.y = dragon.y;
			g_dragonFire.vx = (dx / dist) * DRAGON_FIRE_SPEED;
			g_dragonFire.vy = (dy / dist) * DRAGON_FIRE_SPEED;

			g_dragonNextFireTime = now + DRAGON_FIRE_INTERVAL_MS;
		}
	}
	else if (!dragon.isDead && now >= g_dragonNextFireTime)
	{
		g_dragonFireWindingUp = true;
		g_dragonFireWindupStart = now;
	}

	if (g_dragonFire.active)
	{
		g_dragonFire.x += g_dragonFire.vx;
		g_dragonFire.y += g_dragonFire.vy;

		if (enemyDistanceTo(g_dragonFire.x, g_dragonFire.y, player.x, player.y) <= DRAGON_FIRE_SIZE / 2 + 20)
		{
			applyDamageToFighter(player, Potion_AdjustedDamage(DRAGON_FIRE_DAMAGE));
			g_dragonFire.active = false;
		}
		else if (g_dragonFire.x < -50 || g_dragonFire.x > HOME_AREA_W + 50 ||
			g_dragonFire.y < -50 || g_dragonFire.y > HOME_AREA_H + 50)
		{
			g_dragonFire.active = false;   // flew off the arena - just despawn it
		}
	}
}

// ---------------------------------------------------------------
//  SCENE LIFECYCLE
// ---------------------------------------------------------------
// Spawns a fresh wave of "count" enemies of battleEnemyType, overwriting
// whatever was in battleEnemies[] before (previous wave is already dead
// by the time this is called - see Battle_FixedUpdate()).
inline void spawnBattleWave(int count)
{
	battleEnemyCount = count;
	for (int i = 0; i < count; i++)
	{
		initEnemy(battleEnemies[i], battleEnemyType, randomSpawnX(), randomSpawnY());
		if (battleEnemyType == ENEMY_SKELETON)
			applySkeletonSprites(battleEnemies[i]);
		else if (battleEnemyType == ENEMY_MONSTER)
			applyDragonSprites(battleEnemies[i]);
	}
}

inline void Battle_Init()
{
	// Nothing in the project seeds rand() yet - do it once here so enemy
	// spawn positions (see randomSpawnX()/randomSpawnY()) actually vary
	// between runs instead of repeating the same sequence every time.
	static bool rngSeeded = false;
	if (!rngSeeded) { rngSeeded = true; srand((unsigned int)time(NULL)); }

	// Only actually loads files the first time - safe to call every battle.
	if (arenaBackgroundID == 0)
		arenaBackgroundID = iLoadImage("Images/Arena_1.png");
	if (arenaBackgroundBossID == 0)
		arenaBackgroundBossID = iLoadImage("Images/Arena_2.png");
	loadSkeletonSpritesOnce();
	loadDragonSpritesOnce();
	loadDragonFireTexturesOnce();

	if (g_forceBossFight)
	{
		// Boss button on Home Base was clicked - skip the level roll entirely.
		g_forceBossFight = false;
		battleEnemyType = ENEMY_MONSTER;
		battleIsBossFight = true;
	}
	else
	{
		// Cycles grunt types by Home Base attack attempt (see g_battleAttemptCount's
		// comment above for why this isn't currentLevel anymore) - every 5th attempt
		// is the Monster boss instead of a grunt.
		// Zombie is intentionally EXCLUDED from this rotation - Arena_1 (the
		// regular, non-boss arena reached from the Attack button) should never
		// spawn a Zombie, so every non-boss attempt rolls Skeleton instead.
		g_battleAttemptCount++;
		battleEnemyType = (g_battleAttemptCount % 5 == 0) ? ENEMY_MONSTER : ENEMY_SKELETON;
		battleIsBossFight = (battleEnemyType == ENEMY_MONSTER);
	}

	if (battleIsBossFight) resetDragonFireState();

	battleWave = 1;
	spawnBattleWave(battleIsBossFight ? 1 : battleWave);   // boss fights are always solo, no waves

	Fighter &player = roster[activeFighterIndex];
	if (battleIsBossFight)
	{
		// Boss fight: spawnBattleWave() above already randomized the boss's spot
		// (randomSpawnX()/randomSpawnY()) - place the hero on the opposite side
		// of the arena from it (same spawn range, mirrored) so the two aren't
		// randomly dropped right on top of each other, but the fight still
		// opens from a different spot each time.
		player.x = (BATTLE_ENEMY_SPAWN_X_MIN + BATTLE_ENEMY_SPAWN_X_MAX) - battleEnemies[0].x;
		player.y = (BATTLE_ENEMY_SPAWN_Y_MIN + BATTLE_ENEMY_SPAWN_Y_MAX) - battleEnemies[0].y;
	}
	else
	{
		player.x = BATTLE_PLAYER_START_X;
		player.y = BATTLE_PLAYER_START_Y;
	}
	player.facing = FACE_RIGHT;
	player.animState = ANIM_IDLE;
	player.isAttacking = false;

	battleOutcome = BATTLE_FIGHTING;
	battleOutcomeTimer = 0;
	battleFaceLockUntil = 0;

	// Fresh potion state every battle (also restores any Home Base buff's damage/speed).
	Potion_Reset(player);
	g_battlePotionMenuOpen = false;   // don't carry a still-open panel in from a previous battle
}

// ---------------------------------------------------------------
//  PLAYER -> ENEMY ATTACK
//  Same shape as Fighters.hpp's tryAttack(), but the target is an Enemy
//  instead of another Fighter. Enemy.hpp is intentionally decoupled from
//  Fighters.hpp (see its header comment), so this glue code lives here.
//  One swing hits the closest living enemy that's in range - not a cleave.
//  The player also turns to face that closest enemy, whether or not the
//  swing actually reaches it, so an attack always visibly aims at a target.
// ---------------------------------------------------------------
inline void tryPlayerAttackEnemies(Fighter &attacker, Enemy enemies[], int count, bool triggerRequested)
{
	if (!triggerRequested) return;

	unsigned long now = GetTickCount();
	if (now - attacker.lastAttackTriggerTime < ATTACK_COOLDOWN_MS) return;

	attacker.lastAttackTriggerTime = now;
	attacker.isAttacking = true;
	attacker.animState = ANIM_ATTACK;
	attacker.frameIndex = 0;
	attacker.attackAnimStartTime = now;

	int nearest = -1;
	float nearestDist = 0;
	for (int i = 0; i < count; i++)
	{
		if (enemies[i].isDead) continue;
		float d = distanceBetween(attacker.x, attacker.y, enemies[i].x, enemies[i].y);
		if (nearest == -1 || d < nearestDist)
		{
			nearest = i;
			nearestDist = d;
		}
	}
	if (nearest == -1) return;   // no living enemies to face or hit

	battleLockedFacing = (enemies[nearest].x >= attacker.x) ? FACE_RIGHT : FACE_LEFT;
	battleFaceLockUntil = now + BATTLE_FACE_LOCK_MS;
	attacker.facing = battleLockedFacing;

	if (nearestDist <= attacker.meleeRange)
		applyDamageToEnemy(enemies[nearest], attacker.damage);
	if (enemies[nearest].isDead) awardCoinsForKill(enemies[nearest].type);
}

// ---------------------------------------------------------------
//  PER-TICK UPDATE
//  Returns true once the battle is fully over, so iMain.cpp knows to call
//  EnterLoading(GameState::HOMEBASE).
// ---------------------------------------------------------------
inline bool Battle_FixedUpdate()
{
	Fighter &player = roster[activeFighterIndex];

	// Win/lose message is already showing - just count down, no more input matters.
	if (battleOutcome != BATTLE_FIGHTING)
	{
		// Taunt scene is up - it's dismissed by clicks (Battle_OnMouseDown()),
		// not by time, so the player can read it at their own pace.
		if (g_skeletonTauntActive) return false;

		// The click that dismissed the taunt's last line already fired last
		// tick (Battle_OnMouseDown()) - end the battle now instead of falling
		// through to the BATTLE_WON check below, which would just start the
		// taunt scene over again since battleOutcome/battleEnemyType haven't
		// changed.
		if (g_skeletonTauntDone)
		{
			g_skeletonTauntDone = false;
			return true;
		}

		battleOutcomeTimer--;
		if (battleOutcomeTimer > 0) return false;

		// "VICTORY!" just finished after a regular (non-boss) Skeleton round -
		// hold on the taunt scene instead of returning to Home Base yet.
		if (battleOutcome == BATTLE_WON && !battleIsBossFight && battleEnemyType == ENEMY_SKELETON)
		{
			setupSkeletonTauntTrio();
			g_skeletonTauntActive = true;
			g_skeletonTauntLine = 0;
			return false;
		}

		// Win OR loss: iMain.cpp sends the player straight back to HOMEBASE, and the
		// night timer resumes from where it was paused (see the LOADING handoff there).
		return true;
	}

	// Potion select panel is open (right-click) - freeze the fight (player
	// input, enemy AI, the Dragon's fire-breath timer) until a potion is
	// chosen or the panel is dismissed (see battlePotionMenuOnClick()/
	// Battle_OnMouseDown()). Buff timers themselves are wall-clock based
	// (GetTickCount()), so an already-active potion keeps ticking down in
	// the background same as it would if you'd just tabbed away.
	if (g_battlePotionMenuOpen)
		return false;

	Potion_HotkeysUpdate(player);   // keys 1-4 - see PotionSystem.hpp

	bool up = isKeyPressed('w') || isSpecialKeyPressed(GLUT_KEY_UP);
	bool down = isKeyPressed('s') || isSpecialKeyPressed(GLUT_KEY_DOWN);
	bool left = isKeyPressed('a') || isSpecialKeyPressed(GLUT_KEY_LEFT);
	bool right = isKeyPressed('d') || isSpecialKeyPressed(GLUT_KEY_RIGHT);
	handleMovement(player, up, down, left, right);

	// Re-assert the attack-triggered facing over whatever handleMovement() just
	// set, for as long as the lock window is still active (see BATTLE_FACE_LOCK_MS).
	if (GetTickCount() < battleFaceLockUntil)
		player.facing = battleLockedFacing;

	// Right-click used to also fire an attack here (via g_homeRightClickPending) -
	// it now opens the potion menu instead (see Battle_OnMouseDown()), so
	// attacking inside Battle is space-bar only.
	bool attackTrigger = isKeyPressed(' ');
	tryPlayerAttackEnemies(player, battleEnemies, battleEnemyCount, attackTrigger);

	if (player.type == CHAR_GUARDIAN) tryActivateShield(player, isKeyPressed('e') != 0);
	if (player.type == CHAR_RANGER)   tryDash(player, isKeyPressed('D') != 0);

	updateFighter(player);

	bool anyAlive = false;
	for (int i = 0; i < battleEnemyCount; i++)
	{
		Enemy &e = battleEnemies[i];
		if (!e.isDead)
		{
			anyAlive = true;

			// While the Dragon is winding up its fire breath, it holds still
			// (no walking, no scratch) instead of also melee-attacking.
			bool dragonWindingUp = (e.type == ENEMY_MONSTER && g_dragonFireWindingUp);
			if (!dragonWindingUp)
			{
				moveEnemyToward(e, player.x, player.y);
				if (tryEnemyAttack(e, player.x, player.y))
					applyDamageToFighter(player, Potion_AdjustedDamage(e.damage));   // handles the Guardian-shield block too, Reinforcement Potion too
			}
		}
		updateEnemy(e);
	}

	// Boss fights are always solo (battleEnemies[0]) - drives the fire-breath
	// super move's windup timer, launch, flight and player-hit check.
	if (battleIsBossFight)
		updateDragonFire(battleEnemies[0], player);

	if (!anyAlive)
	{
		if (battleIsBossFight || battleWave >= BATTLE_MAX_ENEMIES)
		{
			// Boss down, or wave 6 just got cleared - the whole battle is won.
			battleOutcome = BATTLE_WON;
			battleOutcomeTimer = BATTLE_OUTCOME_DURATION;

			// Record which grunt round just got cleared (boss fights don't
			// count towards this - only a straight Skeleton or Zombie battle
			// does) so HomeBase.hpp knows when to show the Boss button.
			if (!battleIsBossFight)
			{
				if (battleEnemyType == ENEMY_SKELETON) g_hasWonSkeletonRound = true;
				else if (battleEnemyType == ENEMY_ZOMBIE) g_hasWonZombieRound = true;
			}
		}
		else
		{
			// Wave cleared but not at the cap yet - next wave has one more enemy.
			battleWave++;
			spawnBattleWave(battleWave);
		}
	}
	else if (player.currentHealth <= 0)
	{
		battleOutcome = BATTLE_LOST;
		battleOutcomeTimer = BATTLE_OUTCOME_DURATION;
		player.currentHealth = player.maxHealth;   // don't leave the fighter sitting at 0 HP

		// Losing no longer wipes the game - progress (coins, inventory, level,
		// character, boss unlock) is kept, and the outcome-timer block above
		// sends the player back to Home Base just like a win does.
	}

	return false;
}

// Right-click now opens/closes the potion select menu (pausing combat -
// see Battle_FixedUpdate()) instead of triggering an attack; attacking
// inside Battle is space-bar only now (see the attackTrigger line in
// Battle_FixedUpdate()). While the menu is open, left-click picks a potion
// from it (battlePotionMenuOnClick()).
inline void Battle_OnMouseDown(int button, int mx, int my)
{
	if (g_skeletonTauntActive)
	{
		// Any click advances a line; the click past the last line dismisses
		// the trio, and the next Battle_FixedUpdate() tick ends the battle.
		if (button == GLUT_LEFT_BUTTON || button == GLUT_RIGHT_BUTTON)
		{
			g_skeletonTauntLine++;
			if (g_skeletonTauntLine >= SKELETON_TAUNT_LINE_COUNT)
			{
				g_skeletonTauntActive = false;
				g_skeletonTauntTrioReady = false;   // fresh trio next time this scene plays
				g_skeletonTauntDone = true;         // tell Battle_FixedUpdate() to end the battle
			}
		}
		return;
	}

	if (button == GLUT_RIGHT_BUTTON)
	{
		g_battlePotionMenuOpen = !g_battlePotionMenuOpen;
		return;
	}

	if (button == GLUT_LEFT_BUTTON && g_battlePotionMenuOpen)
	{
		battlePotionMenuOnClick(mx, my, roster[activeFighterIndex]);
	}
}

// ---------------------------------------------------------------
//  DRAWING
// ---------------------------------------------------------------
// PLACEHOLDER for enemies with no art yet (Zombie / Monster) - see the
// header comment at the top of this file.
inline void drawBattleEnemyPlaceholder(Enemy &e)
{
	if (e.type == ENEMY_ZOMBIE) iSetColor(60, 140, 60);    // green grunt
	else                         iSetColor(140, 30, 30);   // dark red boss

	int w = e.isBoss ? ENEMY_DRAW_W * 2 : ENEMY_DRAW_W;
	int h = e.isBoss ? ENEMY_DRAW_H * 2 : ENEMY_DRAW_H;
	iFilledRectangle((int)(e.x - w / 2), (int)(e.y - h / 2), w, h);

	iSetColor(255, 255, 255);
	iRectangle((int)(e.x - w / 2), (int)(e.y - h / 2), w, h);
	iText((int)(e.x - w / 2), (int)(e.y + h / 2 + 10), (char*)enemyTypeName(e.type));
}

// Skeleton and Dragon (the boss) now have real walk/attack sprites
// (drawEnemy(), from Enemy.hpp, already knows how to pick the right frame) -
// Zombie still has no art yet, so it keeps falling back to the placeholder box.
// The Dragon has one extra override: while winding up its fire breath, it
// shows the mouth-open Fire_stand_l/r pose instead of its usual animation.
inline void drawBattleEnemy(Enemy &e)
{
	if (e.isDead) return;

	if (e.type == ENEMY_MONSTER && g_dragonFireWindingUp)
	{
		int w = ENEMY_DRAW_W * 2, h = ENEMY_DRAW_H * 2;   // same boss-size scale-up as drawEnemy()
		iShowImage((int)(e.x - w / 2), (int)(e.y - h / 2), w, h, dragonFireStandTex[e.facing]);
		return;
	}

	if (e.type == ENEMY_SKELETON || e.type == ENEMY_MONSTER) drawEnemy(e);
	else                                                       drawBattleEnemyPlaceholder(e);
}

// Fixed top-of-screen bar for the Dragon boss fight, instead of the small
// floating bar regular enemies get - wider, and anchored a bit below the
// very top of the screen (below the player HP bar / coin HUD row) so it
// isn't crammed against the edge or overlapping the other HUD elements.
inline void drawDragonBossHealthBar(Enemy &dragon)
{
	const double barW = 300, barH = 20;
	const double barX = (HOME_AREA_W - barW) / 2;   // centered
	const double barY = HOME_AREA_H - 70;           // leaves clear space above and below it

	double pct = (dragon.maxHealth > 0) ? (double)dragon.currentHealth / dragon.maxHealth : 0.0;
	if (pct < 0.0) pct = 0.0;
	if (pct > 1.0) pct = 1.0;

	iSetColor(40, 0, 0);
	iFilledRectangle((int)barX, (int)barY, (int)barW, (int)barH);

	if (pct > 0.0)
	{
		iSetColor(200, 20, 20);
		iFilledRectangle((int)barX, (int)barY, (int)(barW * pct), (int)barH);
	}

	iSetColor(255, 255, 255);
	iRectangle((int)barX, (int)barY, (int)barW, (int)barH);
	iText((int)(barX + barW / 2 - 26), (int)(barY + barH + 6), (char*)"DRAGON");
}

// Three Skeletons standing in front of the hero with a speech bubble over
// their heads, cycling through SKELETON_TAUNT_LINES[] - see
// g_skeletonTauntActive's comment above for how the scene starts/ends.
inline void drawSkeletonTauntScene()
{
	for (int i = 0; i < 3; i++)
		drawEnemy(g_skeletonTauntTrio[i]);

	// Bubble width is sized to the current line, not a fixed guess - a fixed
	// box was clipping the longer lines. iText()'s default font here is
	// GLUT_BITMAP_8_BY_13 (iGraphics.h), which is 8px wide per character.
	const char* line = SKELETON_TAUNT_LINES[g_skeletonTauntLine];
	const double padding = 20;
	double bubbleW = strlen(line) * 8.0 + padding * 2;
	const double bubbleH = 70;

	double bubbleX = g_skeletonTauntTrio[1].x - bubbleW / 2;
	if (bubbleX < 10) bubbleX = 10;                                   // don't run off the left edge
	if (bubbleX + bubbleW > HOME_AREA_W - 10) bubbleX = HOME_AREA_W - 10 - bubbleW;   // or the right

	const double bubbleY = g_skeletonTauntTrio[1].y + ENEMY_DRAW_H / 2 + 30;

	iSetColor(255, 255, 255);
	iFilledRectangle((int)bubbleX, (int)bubbleY, (int)bubbleW, (int)bubbleH);
	iSetColor(0, 0, 0);
	iRectangle((int)bubbleX, (int)bubbleY, (int)bubbleW, (int)bubbleH);
	iText((int)(bubbleX + padding), (int)(bubbleY + bubbleH - 28), const_cast<char*>(line));

	iSetColor(110, 110, 110);
	iText((int)(bubbleX + padding), (int)(bubbleY + 10), const_cast<char*>("(click to continue)"));
}

inline void Battle_Draw()
{
	// "Arena_1.png" for a regular wave fight, "Arena_2.png" for any boss
	// fight (level-roll boss or the Boss button) - full-screen background.
	iShowImage(0, 0, HOME_AREA_W, HOME_AREA_H, battleIsBossFight ? arenaBackgroundBossID : arenaBackgroundID);

	drawFighter(roster[activeFighterIndex]);   // real sprite - reuses the Home Base Fighter as-is

	for (int i = 0; i < battleEnemyCount; i++)
	{
		Enemy &e = battleEnemies[i];
		drawBattleEnemy(e);
		// Regular grunts keep their small floating bar; the Dragon boss gets
		// the fixed top-of-screen bar drawn separately below instead.
		if (!e.isDead && !e.isBoss)
			drawEnemyHealthBar(e, (int)(e.x - 40), (int)(e.y + ENEMY_DRAW_H / 2 + 25), 80, 10);
	}

	if (battleIsBossFight && !battleEnemies[0].isDead)
		drawDragonBossHealthBar(battleEnemies[0]);

	// Fire breath bolt (Images/Fire.png), only while one is actually in flight.
	if (g_dragonFire.active)
		iShowImage((int)(g_dragonFire.x - DRAGON_FIRE_SIZE / 2), (int)(g_dragonFire.y - DRAGON_FIRE_SIZE / 2),
			DRAGON_FIRE_SIZE, DRAGON_FIRE_SIZE, dragonFireBoltTex);

	drawHomeHealthBar();   // same left-side "just the number" HUD used in Home Base
	drawCoinHUD();
	drawBattlePotionSelectMenu();   // right-click panel - only while g_battlePotionMenuOpen
	Potion_DrawHUD();               // active-potion icons + "<Potion> Potion activated" banner

	if (!battleIsBossFight && battleOutcome == BATTLE_FIGHTING)
	{
		char waveBuf[32];
		sprintf(waveBuf, "Wave %d / %d", battleWave, BATTLE_MAX_ENEMIES);
		iSetColor(255, 255, 255);
		iText(HOME_AREA_W - 110, 20, waveBuf);   // bottom-right corner
	}

	if (battleOutcome == BATTLE_LOST)
	{
		iSetColor(255, 255, 255);
		iText(340, 300, const_cast<char*>("DEFEATED!"), GLUT_BITMAP_TIMES_ROMAN_24);
	}
	else if (battleOutcome == BATTLE_WON && !g_skeletonTauntActive)
	{
		iSetColor(255, 255, 255);
		iText(340, 300, const_cast<char*>("VICTORY!"), GLUT_BITMAP_TIMES_ROMAN_24);
	}

	if (g_skeletonTauntActive)
		drawSkeletonTauntScene();
}
