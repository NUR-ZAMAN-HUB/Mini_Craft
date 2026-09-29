// =====================================================================
//  Enemy.hpp  (Zombie / Skeleton / Monster boss)
// =====================================================================
//  Same idea as Fighters.hpp: every enemy is built from ONE struct
//  (Enemy) with different numbers and different sprite sets:
//     Zombie   - slow, tanky-ish melee grunt
//     Skeleton - faster, fragile melee grunt (ranged attack can be
//                added later - see the note above tryEnemyAttack())
//     Monster  - boss version, much higher HP/damage, and gets an
//                "enrage" speed/damage boost once it drops low on HP
//
//  STATS[] below holds each enemy's numbers + sprite file prefix, in
//  the same order as the EnemyType enum, so initEnemy() can just look
//  up STATS[type] instead of repeating the same setup code per enemy.
//
//  This header is intentionally self-contained (it does NOT include
//  Fighters.hpp), so it can be dropped into the project and built
//  against on its own before enemies are actually wired into combat.
//  Because of that, tryEnemyAttack() below takes a plain target x/y
//  instead of a Fighter* - when you wire this into HomeBase.hpp, call
//  it with the active Fighter's x/y and apply the damage to that
//  Fighter yourself (see the comment on that function).
//
//  SPRITES: this header does NOT load any images itself. initEnemy()
//  only sets up stats/position/state - you load textures and assign
//  them to e.idleTex / e.walkTex / e.attackTex yourself in main code
//  (see the comment above initEnemy() for the exact fields to fill in).
// =====================================================================
#pragma once

#include "iGraphics.h"
#include <windows.h>   // GetTickCount()
#include <math.h>      // sqrtf()
#include <stdio.h>     // sprintf()
#include <string.h>    // memset()

// ---------------------------------------------------------------
//  TUNABLE CONSTANTS
//  (kept in this file for now since Enemy.hpp is meant to be
//  self-contained - move these into a shared config header later
//  if that becomes the project convention for enemies too)
// ---------------------------------------------------------------
#define ENEMY_DRAW_W 56
#define ENEMY_DRAW_H 61

// Zombie: slow, tanky-ish melee grunt
#define ZOMBIE_MAX_HEALTH   40
#define ZOMBIE_MOVE_SPEED   1.5f     // pixels per fixedUpdate() tick
#define ZOMBIE_DAMAGE       8
#define ZOMBIE_MELEE_RANGE  50.0f

// Skeleton: faster, fragile melee grunt
#define SKELETON_MAX_HEALTH   60
#define SKELETON_MOVE_SPEED   2.0f
#define SKELETON_DAMAGE       10
#define SKELETON_MELEE_RANGE  50.0f

// Monster (boss): much higher HP/damage than either grunt.
// MAX_HEALTH bumped up from 500 - at 500 HP a Guardian could kill it in as
// little as ~10s of continuous melee (ATTACK_COOLDOWN_MS=500 -> 2 hits/sec
// * 25 dmg = 50 dmg/sec), well under the 20s fire-breath interval below, so
// the super move often never got a chance to trigger. 900 gives it enough
// room to reliably survive to its first fire breath under normal play.
#define MONSTER_MAX_HEALTH   900
#define MONSTER_MOVE_SPEED   2.4f   // was 3.0f (before that, 3.5f) - nerfed again, still faster than Skeleton (2.0f)
#define MONSTER_DAMAGE       45
#define MONSTER_MELEE_RANGE  50.0f

// Monster enrage: once HP drops to/below this % of max, it speeds up
// and hits harder - gives the boss fight a second phase
#define MONSTER_ENRAGE_HEALTH_PCT   0.30f
#define MONSTER_ENRAGE_SPEED_MULT   1.5f
#define MONSTER_ENRAGE_DAMAGE_MULT  1.5f

// ---------------------------------------------------------------
//  ANIMATION FRAME TIMING (ms between frames)
// ---------------------------------------------------------------
#define ENEMY_WALK_FRAME_INTERVAL_MS    120
#define ENEMY_ATTACK_FRAME_INTERVAL_MS  80

// Max frames that exist for each animation type (adjust once art
// is in place - see the Images/ path list in the header comment above)
#define ENEMY_WALK_MAX_FRAMES    4
#define ENEMY_ATTACK_MAX_FRAMES  4

// ---------------------------------------------------------------
//  ATTACK
// ---------------------------------------------------------------
#define ENEMY_ATTACK_COOLDOWN_MS 800   // stops the attack from re-triggering every tick

// Boss-only override: the Dragon's basic (scratch) attack fires once every
// 5 seconds instead of the grunt cooldown above - see tryEnemyAttack().
#define BOSS_ATTACK_COOLDOWN_MS 5000

// ---------------------------------------------------------------
//  ENUMS
// ---------------------------------------------------------------
enum EnemyType     { ENEMY_ZOMBIE = 0, ENEMY_SKELETON = 1, ENEMY_MONSTER = 2 };
enum EnemyFacing   { ENEMY_FACE_LEFT = 0, ENEMY_FACE_RIGHT = 1 };
enum EnemyAnimState{ ENEMY_ANIM_IDLE = 0, ENEMY_ANIM_WALK = 1, ENEMY_ANIM_ATTACK = 2, ENEMY_ANIM_DEAD = 3 };

// ---------------------------------------------------------------
//  THE ENEMY STRUCT - one on-screen enemy + everything it needs to draw itself
// ---------------------------------------------------------------
struct Enemy
{
	EnemyType type;
	bool      isBoss;

	int   maxHealth, currentHealth, damage;
	float moveSpeed, meleeRange;

	float x, y;
	EnemyFacing facing;

	EnemyAnimState animState;
	int            frameIndex;
	unsigned long  lastFrameSwapTime;

	unsigned int idleTex[2];
	unsigned int walkTex[2][ENEMY_WALK_MAX_FRAMES];
	int          walkFrameCount;
	unsigned int attackTex[2][ENEMY_ATTACK_MAX_FRAMES];
	int          attackFrameCount;

	bool          isAttacking;
	unsigned long attackAnimStartTime, lastAttackTriggerTime;

	bool isDead;

	// Monster (boss) only
	bool enraged;
};

// ---------------------------------------------------------------
//  PER-ENEMY STATS TABLE (index = EnemyType)
// ---------------------------------------------------------------
struct EnemyStats { const char* prefix; int hp, dmg; float speed, range; int walkFrames, atkFrames; bool isBoss; };

static const EnemyStats ENEMY_STATS[3] = {
	{ "zombie",   ZOMBIE_MAX_HEALTH,   ZOMBIE_DAMAGE,   ZOMBIE_MOVE_SPEED,   ZOMBIE_MELEE_RANGE,   ENEMY_WALK_MAX_FRAMES, ENEMY_ATTACK_MAX_FRAMES, false },
	{ "skeleton", SKELETON_MAX_HEALTH, SKELETON_DAMAGE, SKELETON_MOVE_SPEED, SKELETON_MELEE_RANGE, ENEMY_WALK_MAX_FRAMES, ENEMY_ATTACK_MAX_FRAMES, false },
	// Boss art is the Dragon: 2 walk frames/side, 3 scratch(attack) frames/side -
	// see loadDragonSpritesOnce()/applyDragonSprites() in Battle.hpp.
	{ "dragon",   MONSTER_MAX_HEALTH,  MONSTER_DAMAGE,  MONSTER_MOVE_SPEED,  MONSTER_MELEE_RANGE,  2,                      3,                        true  },
};

// =====================================================================
//  SMALL HELPERS
// =====================================================================
inline float enemyDistanceTo(float x1, float y1, float x2, float y2)
{
	float dx = x2 - x1, dy = y2 - y1;
	return sqrtf(dx * dx + dy * dy);
}

// Moves to the next animation frame once "intervalMs" have passed.
inline int nextEnemyAnimationFrame(int frame, int frameCount, unsigned long &lastSwapTime, int intervalMs)
{
	unsigned long now = GetTickCount();
	if (now - lastSwapTime >= (unsigned long)intervalMs)
	{
		lastSwapTime = now;
		frame = (frame + 1) % frameCount;
	}
	return frame;
}

// OPTIONAL helper for your main-code sprite loading: loads
// "Images/<prefix>_<actionAndDir>_0<1..count>.png" into dest[].
// Nothing in this header calls this automatically - use it (or not)
// wherever you're loading enemy sprites.
inline void loadEnemyFrames(unsigned int dest[], const char* prefix, const char* actionAndDir, int count)
{
	char path[160];
	for (int i = 0; i < count; i++)
	{
		sprintf(path, "Images/%s_%s_0%d.png", prefix, actionAndDir, i + 1);
		dest[i] = iLoadImage(path);
	}
}

inline const char* enemyTypeName(EnemyType t)
{
	if (t == ENEMY_ZOMBIE)   return "Zombie";
	if (t == ENEMY_SKELETON) return "Skeleton";
	return "Dragon";
}

// =====================================================================
//  ENEMY CREATION - one shared function, driven by ENEMY_STATS[type]
// =====================================================================
//  NOTE: this only sets up stats/position/state - it does NOT call
//  iLoadImage() or touch idleTex/walkTex/attackTex. Sprite loading is
//  done in your own main code, since that's where you're wiring up
//  the actual image files. After calling initEnemy(), assign the
//  loaded texture IDs yourself, e.g.:
//
//      Enemy z;
//      initEnemy(z, ENEMY_ZOMBIE, 300.0f, 200.0f);
//      z.idleTex[ENEMY_FACE_LEFT]  = iLoadImage("Images/zombie_idle_left.png");
//      z.idleTex[ENEMY_FACE_RIGHT] = iLoadImage("Images/zombie_idle_right.png");
//      z.walkFrameCount = 4;
//      z.walkTex[ENEMY_FACE_LEFT][0] = iLoadImage("Images/zombie_walk_left_01.png");
//      ... etc
//
//  loadEnemyFrames() further down is still here as an optional shortcut
//  if you want it for the walk/attack arrays, but nothing calls it
//  automatically anymore.
// =====================================================================
inline void initEnemy(Enemy &e, EnemyType type, float startX, float startY)
{
	memset(&e, 0, sizeof(Enemy));
	const EnemyStats &s = ENEMY_STATS[type];

	e.type = type;
	e.isBoss = s.isBoss;
	e.maxHealth = e.currentHealth = s.hp;
	e.moveSpeed = s.speed;
	e.damage = s.dmg;
	e.meleeRange = s.range;
	e.x = startX; e.y = startY;
	e.facing = ENEMY_FACE_LEFT;   // enemies default to facing the player area on the left
	e.animState = ENEMY_ANIM_IDLE;
	e.walkFrameCount = s.walkFrames;
	e.attackFrameCount = s.atkFrames;
}

// =====================================================================
//  MOVEMENT - simple "walk toward a target point" for now
//  (swap this out for real pathing/AI later; this is just enough to
//  make an enemy chase the player in a straight line)
// =====================================================================
inline void moveEnemyToward(Enemy &e, float targetX, float targetY)
{
	if (e.isDead || e.isAttacking) return;

	float dx = targetX - e.x, dy = targetY - e.y;
	float dist = sqrtf(dx * dx + dy * dy);

	bool moved = dist > 1.0f;
	if (moved)
	{
		e.x += (dx / dist) * e.moveSpeed;
		e.y += (dy / dist) * e.moveSpeed;
		e.facing = (dx >= 0) ? ENEMY_FACE_RIGHT : ENEMY_FACE_LEFT;
	}

	e.animState = moved ? ENEMY_ANIM_WALK : ENEMY_ANIM_IDLE;
}

// =====================================================================
//  ATTACK / DAMAGE
// =====================================================================
inline void applyDamageToEnemy(Enemy &e, int damage)
{
	if (e.isDead) return;

	e.currentHealth -= damage;
	if (e.currentHealth <= 0)
	{
		e.currentHealth = 0;
		e.isDead = true;
		e.animState = ENEMY_ANIM_DEAD;
	}
}

// Checks range + cooldown and, if the attack lands, returns true so the
// caller can apply e.damage to whatever target struct they're using
// (e.g. the player's Fighter from Fighters.hpp). Kept generic on
// purpose - see the header comment at the top of this file for why.
//
// NOTE: only a melee range check for now. If Skeleton (or a later
// enemy) should shoot a projectile instead of melee-hitting, that's
// a separate system to add on top of this - this just flags "attack
// happened," it doesn't know about projectiles.
inline bool tryEnemyAttack(Enemy &e, float targetX, float targetY)
{
	if (e.isDead) return false;

	unsigned long now = GetTickCount();
	unsigned long cooldown = e.isBoss ? BOSS_ATTACK_COOLDOWN_MS : ENEMY_ATTACK_COOLDOWN_MS;
	if (now - e.lastAttackTriggerTime < cooldown) return false;
	if (enemyDistanceTo(e.x, e.y, targetX, targetY) > e.meleeRange) return false;

	e.lastAttackTriggerTime = now;
	e.isAttacking = true;
	e.animState = ENEMY_ANIM_ATTACK;
	e.frameIndex = 0;
	e.attackAnimStartTime = now;

	return true;
}

// =====================================================================
//  MONSTER (BOSS) ENRAGE
// =====================================================================
// Call this once per tick for the Monster; it flips "enraged" on the
// first time HP drops to/below the threshold and stays on afterward.
inline void updateMonsterEnrage(Enemy &e)
{
	if (e.type != ENEMY_MONSTER || e.enraged || e.isDead) return;

	if (e.currentHealth <= (int)(e.maxHealth * MONSTER_ENRAGE_HEALTH_PCT))
	{
		e.enraged = true;
		e.moveSpeed *= MONSTER_ENRAGE_SPEED_MULT;
		e.damage = (int)(e.damage * MONSTER_ENRAGE_DAMAGE_MULT);
	}
}

// =====================================================================
//  PER-TICK UPDATE
// =====================================================================
inline void updateEnemy(Enemy &e)
{
	if (e.isDead) return;

	unsigned long now = GetTickCount();

	if (e.animState == ENEMY_ANIM_WALK)
		e.frameIndex = nextEnemyAnimationFrame(e.frameIndex, e.walkFrameCount, e.lastFrameSwapTime, ENEMY_WALK_FRAME_INTERVAL_MS);
	else if (e.animState == ENEMY_ANIM_ATTACK)
		e.frameIndex = nextEnemyAnimationFrame(e.frameIndex, e.attackFrameCount, e.lastFrameSwapTime, ENEMY_ATTACK_FRAME_INTERVAL_MS);
	else
		e.frameIndex = 0;   // idle

	if (e.isAttacking && now - e.attackAnimStartTime >= (unsigned long)(e.attackFrameCount * ENEMY_ATTACK_FRAME_INTERVAL_MS))
	{
		e.isAttacking = false;
		e.animState = ENEMY_ANIM_IDLE;
		e.frameIndex = 0;
	}

	if (e.isBoss) updateMonsterEnrage(e);
}

// =====================================================================
//  DRAWING
// =====================================================================
inline void drawEnemy(Enemy &e)
{
	if (e.isDead) return;   // no death sprite/animation yet - just stop drawing

	unsigned int tex;
	if (e.animState == ENEMY_ANIM_WALK)        tex = e.walkTex[e.facing][e.frameIndex];
	else if (e.animState == ENEMY_ANIM_ATTACK) tex = e.attackTex[e.facing][e.frameIndex];
	else                                        tex = e.idleTex[e.facing];

	// Bosses (e.g. the Dragon) draw at double size, same scale-up the
	// placeholder box already used for isBoss enemies.
	int w = e.isBoss ? ENEMY_DRAW_W * 2 : ENEMY_DRAW_W;
	int h = e.isBoss ? ENEMY_DRAW_H * 2 : ENEMY_DRAW_H;
	iShowImage((int)(e.x - w / 2), (int)(e.y - h / 2), w, h, tex);
}

// Simple HP bar overlay - mainly meant for the Monster boss fight, but
// works for any enemy. barWidth/barHeight are on-screen pixel sizes;
// (x, y) is the bottom-left corner, same convention as iFilledRectangle().
inline void drawEnemyHealthBar(Enemy &e, int x, int y, int barWidth, int barHeight)
{
	if (e.isDead || e.maxHealth <= 0) return;

	float pct = (float)e.currentHealth / (float)e.maxHealth;
	if (pct < 0) pct = 0;
	if (pct > 1) pct = 1;

	iSetColor(60, 0, 0);
	iFilledRectangle(x, y, barWidth, barHeight);

	iSetColor(200, 20, 20);
	iFilledRectangle(x, y, (double)(barWidth * pct), barHeight);

	iSetColor(255, 255, 255);
	iRectangle(x, y, barWidth, barHeight);
}
