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

// ---------------------------------------------------------------
//  STATE
// ---------------------------------------------------------------
#define BATTLE_MAX_ENEMIES 6

Enemy battleEnemies[BATTLE_MAX_ENEMIES];
int   battleEnemyCount = 1;    // how many enemies are in the CURRENT wave
int   battleWave = 1;          // current wave number (1..BATTLE_MAX_ENEMIES)
bool  battleIsBossFight = false;
EnemyType battleEnemyType = ENEMY_ZOMBIE;

// Arena background. Loaded lazily the first time Battle_Init() runs (rather
// than every battle) since iLoadImage() only needs to happen once per file.
unsigned int arenaBackgroundID = 0;

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
	loadSkeletonSpritesOnce();

	// Cycles grunt types by level so fights don't feel identical - every
	// 5th level's fight is the Monster boss instead of a Zombie/Skeleton grunt.
	battleEnemyType = (currentLevel % 5 == 0) ? ENEMY_MONSTER
		: ((currentLevel % 2 == 0) ? ENEMY_SKELETON : ENEMY_ZOMBIE);
	battleIsBossFight = (battleEnemyType == ENEMY_MONSTER);

	battleWave = 1;
	spawnBattleWave(battleIsBossFight ? 1 : battleWave);   // boss fights are always solo, no waves

	Fighter &player = roster[activeFighterIndex];
	player.x = BATTLE_PLAYER_START_X;
	player.y = BATTLE_PLAYER_START_Y;
	player.facing = FACE_RIGHT;
	player.animState = ANIM_IDLE;
	player.isAttacking = false;

	battleOutcome = BATTLE_FIGHTING;
	battleOutcomeTimer = 0;
	battleFaceLockUntil = 0;
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
		battleOutcomeTimer--;
		return battleOutcomeTimer <= 0;
	}

	bool up = isKeyPressed('w') || isSpecialKeyPressed(GLUT_KEY_UP);
	bool down = isKeyPressed('s') || isSpecialKeyPressed(GLUT_KEY_DOWN);
	bool left = isKeyPressed('a') || isSpecialKeyPressed(GLUT_KEY_LEFT);
	bool right = isKeyPressed('d') || isSpecialKeyPressed(GLUT_KEY_RIGHT);
	handleMovement(player, up, down, left, right);

	// Re-assert the attack-triggered facing over whatever handleMovement() just
	// set, for as long as the lock window is still active (see BATTLE_FACE_LOCK_MS).
	if (GetTickCount() < battleFaceLockUntil)
		player.facing = battleLockedFacing;

	bool attackTrigger = isKeyPressed(' ') || g_homeRightClickPending;
	tryPlayerAttackEnemies(player, battleEnemies, battleEnemyCount, attackTrigger);
	g_homeRightClickPending = false;

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
			moveEnemyToward(e, player.x, player.y);
			if (tryEnemyAttack(e, player.x, player.y))
				applyDamageToFighter(player, e.damage);   // handles the Guardian-shield block too
		}
		updateEnemy(e);
	}

	if (!anyAlive)
	{
		if (battleIsBossFight || battleWave >= BATTLE_MAX_ENEMIES)
		{
			// Boss down, or wave 6 just got cleared - the whole battle is won.
			battleOutcome = BATTLE_WON;
			battleOutcomeTimer = BATTLE_OUTCOME_DURATION;
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
		player.currentHealth = player.maxHealth;   // respawn back at Home Base at full health
	}

	return false;
}

inline void Battle_OnMouseDown(int button)
{
	if (button == GLUT_RIGHT_BUTTON) g_homeRightClickPending = true;
}

// ---------------------------------------------------------------
//  POST-BATTLE "REINFORCEMENTS" SCENE
//  Plays once a non-boss BATTLE ends in BATTLE_WON (i.e. the wave of 6
//  was just cleared - see the battleOutcome/battleIsBossFight check in
//  iMain.cpp's fixedUpdate()). Hero stands on the left, a Skeleton
//  stands on the right, and it delivers two lines of taunt - one per
//  SPACE press/click - before iMain.cpp sends the player on to Home Base.
//
//  iMain.cpp calls into this the same way as the rest of Battle.hpp:
//      PostBattle_Init()          -> call once, right when the state
//                                    switches to POST_BATTLE_CUTSCENE
//      PostBattle_Draw()          -> call every frame while in that state
//      PostBattle_Advance()       -> call on SPACE/click while in that state;
//                                    advances to the next line, or - once the
//                                    last line's been shown - sends the player
//                                    on to Home Base itself (calls EnterLoading()).
// ---------------------------------------------------------------
const float POST_BATTLE_HERO_X = 220.0f, POST_BATTLE_HERO_Y = 300.0f;
const float POST_BATTLE_SKELETON_X = 560.0f, POST_BATTLE_SKELETON_Y = 300.0f;

// Each entry is up to 2 lines (second line left "" if the taunt only needs one).
#define POST_BATTLE_TOTAL_LINES 2
const char* postBattleLines[POST_BATTLE_TOTAL_LINES][2] = {
	{ "This is not over.",          "We are calling for reinforcements!" },
	{ "I am also calling the boss!", "" }
};
int postBattleLineIndex = 0;

inline void PostBattle_Init()
{
	// Face the two of them at each other, standing still - it's a taunt, not a fight.
	Fighter &player = roster[activeFighterIndex];
	player.x = POST_BATTLE_HERO_X;
	player.y = POST_BATTLE_HERO_Y;
	player.facing = FACE_RIGHT;
	player.animState = ANIM_IDLE;
	player.isAttacking = false;

	postBattleLineIndex = 0;
}

// Called on SPACE/click while in POST_BATTLE_CUTSCENE. Moves to the next taunt
// line, or - once the last one's already been shown - hands off to Home Base.
inline void PostBattle_Advance()
{
	postBattleLineIndex++;
	if (postBattleLineIndex >= POST_BATTLE_TOTAL_LINES)
		EnterLoading(GameState::HOMEBASE);
}

inline void PostBattle_Draw()
{
	// Same arena backdrop the fight itself just used.
	iShowImage(0, 0, HOME_AREA_W, HOME_AREA_H, arenaBackgroundID);

	drawFighter(roster[activeFighterIndex]);   // hero, left side

	// Skeleton, right side, facing left toward the hero - reuses the walk texture
	// set already loaded for battle (no dedicated "idle facing" art, same as
	// drawBattleEnemy()'s skeleton handling above). Drawn at the same size as
	// the hero (FIGHTER_DRAW_W/H, HomeBaseConfig.hpp) rather than the battle's
	// enlarged size, so the two look evenly matched in this face-off.
	unsigned int skeletonTex = skeletonWalkTex[ENEMY_FACE_LEFT][0];
	if (skeletonTex > 0)
		iShowImage((int)(POST_BATTLE_SKELETON_X - FIGHTER_DRAW_W / 2), (int)(POST_BATTLE_SKELETON_Y - FIGHTER_DRAW_H / 2),
			FIGHTER_DRAW_W, FIGHTER_DRAW_H, skeletonTex);

	// Dialogue box, same look as renderCutscene() in iMain.cpp but with a red
	// outline to mark an enemy speaker instead of an ally one.
	int boxX = 200, boxY = 90, boxWidth = 390, boxHeight = 110;

	iSetColor(15, 15, 25);
	iFilledRectangle(boxX, boxY, boxWidth, boxHeight);

	iSetColor(180, 30, 30);   // Skeleton: red outline
	iRectangle(boxX, boxY, boxWidth, boxHeight);

	iSetColor(255, 255, 255);
	iText(boxX + 20, boxY + 80, const_cast<char*>("Skeleton:"), GLUT_BITMAP_HELVETICA_18);

	// Two short lines rather than iTextWrapped() - that helper lives in iMain.cpp,
	// defined after Battle.hpp is included, so it isn't visible here. Whichever
	// entry postBattleLineIndex points at is the line currently being shown -
	// see PostBattle_Advance() above for how that index moves forward.
	int clampedIndex = (postBattleLineIndex < POST_BATTLE_TOTAL_LINES) ? postBattleLineIndex : POST_BATTLE_TOTAL_LINES - 1;
	iText(boxX + 20, boxY + 50, const_cast<char*>(postBattleLines[clampedIndex][0]), GLUT_BITMAP_HELVETICA_18);
	if (postBattleLines[clampedIndex][1][0] != '\0')
		iText(boxX + 20, boxY + 25, const_cast<char*>(postBattleLines[clampedIndex][1]), GLUT_BITMAP_HELVETICA_18);

	iSetColor(180, 180, 180);
	iText(boxX + 90, boxY - 20, const_cast<char*>("Press [SPACE] or click to continue..."), GLUT_BITMAP_HELVETICA_12);
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

// Skeleton now has real walk/attack sprites (drawEnemy(), from Enemy.hpp,
// already knows how to pick the right frame) - everything else still falls
// back to the placeholder box.
inline void drawBattleEnemy(Enemy &e)
{
	if (e.isDead) return;
	if (e.type == ENEMY_SKELETON) drawEnemy(e);
	else                            drawBattleEnemyPlaceholder(e);
}

inline void Battle_Draw()
{
	// "Arena_1.png" - full-screen battle arena background.
	iShowImage(0, 0, HOME_AREA_W, HOME_AREA_H, arenaBackgroundID);

	drawFighter(roster[activeFighterIndex]);   // real sprite - reuses the Home Base Fighter as-is

	for (int i = 0; i < battleEnemyCount; i++)
	{
		Enemy &e = battleEnemies[i];
		drawBattleEnemy(e);
		if (!e.isDead)
			drawEnemyHealthBar(e, (int)(e.x - 40),
				(int)(e.y + (e.isBoss ? ENEMY_DRAW_H : ENEMY_DRAW_H / 2) + 25), 80, 10);
	}

	drawHomeHealthBar();   // same left-side "just the number" HUD used in Home Base

	if (!battleIsBossFight && battleOutcome == BATTLE_FIGHTING)
	{
		char waveBuf[32];
		sprintf(waveBuf, "Wave %d / %d", battleWave, BATTLE_MAX_ENEMIES);
		iSetColor(255, 255, 255);
		iText(HOME_AREA_W - 110, 20, waveBuf);   // bottom-right corner
	}

	iSetColor(255, 255, 255);
	if (battleOutcome == BATTLE_WON)
		iText(340, 300, const_cast<char*>("VICTORY!"), GLUT_BITMAP_TIMES_ROMAN_24);
	else if (battleOutcome == BATTLE_LOST)
		iText(310, 300, const_cast<char*>("DEFEATED..."), GLUT_BITMAP_TIMES_ROMAN_24);
}
