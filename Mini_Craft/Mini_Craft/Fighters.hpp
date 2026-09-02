// =====================================================================
//  Fighters.hpp  (Home Base combat characters)
// =====================================================================
//  This used to be called "Player.hpp" when Home Base was its own
//  standalone project. It's renamed to Fighters.hpp and the struct is
//  renamed Character -> Fighter, because the merged project already
//  has a Player.hpp with its own (unrelated) "Character" struct used
//  by the main menu / character-select screens. Renaming avoids a
//  duplicate-definition compile error and keeps the two systems
//  clearly separate.
//
//  Nothing here was removed - every function that existed before
//  still exists (just renamed/consolidated where it made the file
//  shorter and easier to follow, e.g. the three initGuardian /
//  initRanger / initAlchemist functions now share one core routine
//  instead of repeating the same 15 lines three times).
// =====================================================================
#pragma once

#include "iGraphics.h"
#include "HomeBaseConfig.hpp"
#include <windows.h>   // GetTickCount()
#include <math.h>      // sqrtf()
#include <stdio.h>     // sprintf()
#include <string.h>    // memset()

// ---------------------------------------------------------------
//  ENUMS
// ---------------------------------------------------------------
enum FighterType
{
	CHAR_GUARDIAN = 0,
	CHAR_RANGER = 1,
	CHAR_ALCHEMIST = 2
};

enum FacingDir
{
	FACE_LEFT = 0,
	FACE_RIGHT = 1
};

enum AnimState
{
	ANIM_IDLE = 0,
	ANIM_WALK = 1,
	ANIM_ATTACK = 2,
	ANIM_DASH = 3
};

// ---------------------------------------------------------------
//  THE FIGHTER STRUCT  (was "Character" in the original Home Base build)
// ---------------------------------------------------------------
struct Fighter
{
	FighterType type;

	int   maxHealth;
	int   currentHealth;
	float moveSpeed;
	int   damage;
	float meleeRange;
	FacingDir facing;

	float x, y;

	AnimState     animState;
	int           frameIndex;
	unsigned long lastFrameSwapTime;

	unsigned int idleTex[2];
	unsigned int walkTex[2][WALK_MAX_FRAMES];
	int          walkFrameCount;
	unsigned int attackTex[2][ATTACK_MAX_FRAMES];
	int          attackFrameCount;

	bool          isAttacking;
	unsigned long attackAnimStartTime;
	unsigned long lastAttackTriggerTime;

	bool          shieldActive;
	unsigned long shieldActiveStartTime;
	bool          shieldOnCooldown;
	unsigned long shieldCooldownStartTime;
	int           shieldFrameIndex;
	unsigned long shieldLastFrameSwapTime;
	unsigned int  shieldRingTex[SHIELD_RING_FRAMES];

	bool          dashInProgress;
	unsigned long dashAnimStartTime;
	unsigned long lastDashTriggerTime;
	unsigned int  dashTex[2][DASH_FRAMES];
	unsigned int  dashTrailTex;
};

// =====================================================================
//  SMALL REUSABLE HELPERS
// =====================================================================

inline float distanceBetween(float x1, float y1, float x2, float y2)
{
	float dx = x2 - x1;
	float dy = y2 - y1;
	return sqrtf(dx * dx + dy * dy);
}

// Advances an animation frame index once every "intervalMs" milliseconds.
inline int advanceFrame(int currentFrame, int frameCount, unsigned long &lastSwapTime, int intervalMs)
{
	unsigned long now = GetTickCount();
	if (now - lastSwapTime >= (unsigned long)intervalMs)
	{
		lastSwapTime = now;
		currentFrame = (currentFrame + 1) % frameCount;
	}
	return currentFrame;
}

// Loads "Images/<prefix>_<actionAndDir>_0<1..frameCount>.png" into destRow[].
inline void loadFrames(unsigned int destRow[], const char* prefix, const char* actionAndDir, int frameCount)
{
	char buf[160];
	for (int i = 0; i < frameCount; i++)
	{
		sprintf(buf, "Images/%s_%s_0%d.png", prefix, actionAndDir, i + 1);
		destRow[i] = iLoadImage(buf);
	}
}

inline const char* fighterTypeName(FighterType t)
{
	if (t == CHAR_GUARDIAN)  return "Guardian";
	if (t == CHAR_RANGER)    return "Ranger";
	return "Alchemist";
}

// ---------------------------------------------------------------
//  Per-type tuning + sprite-file-prefix table, used by initFighterCore()
//  so the three init functions below don't have to repeat themselves.
// ---------------------------------------------------------------
struct FighterTypeInfo
{
	const char* spritePrefix;
	int   maxHealth;
	float moveSpeed;
	int   damage;
	float meleeRange;
	int   walkFrames;
	int   attackFrames;
};

inline FighterTypeInfo getFighterTypeInfo(FighterType t)
{
	FighterTypeInfo info = { "", 0, 0.0f, 0, 0.0f, 0, 0 };
	switch (t)
	{
	case CHAR_GUARDIAN:
		info.spritePrefix = "guardian";
		info.maxHealth = GUARDIAN_MAX_HEALTH;
		info.moveSpeed = GUARDIAN_MOVE_SPEED;
		info.damage = GUARDIAN_DAMAGE;
		info.meleeRange = GUARDIAN_MELEE_RANGE;
		info.walkFrames = 3;
		info.attackFrames = 5;
		break;
	case CHAR_RANGER:
		info.spritePrefix = "ranger";
		info.maxHealth = RANGER_MAX_HEALTH;
		info.moveSpeed = RANGER_MOVE_SPEED;
		info.damage = RANGER_DAMAGE;
		info.meleeRange = RANGER_MELEE_RANGE;
		info.walkFrames = 4;
		info.attackFrames = 4;
		break;
	case CHAR_ALCHEMIST:
	default:
		info.spritePrefix = "alchemist";
		info.maxHealth = ALCHEMIST_MAX_HEALTH;
		info.moveSpeed = ALCHEMIST_MOVE_SPEED;
		info.damage = ALCHEMIST_DAMAGE;
		info.meleeRange = ALCHEMIST_MELEE_RANGE;
		info.walkFrames = 4;
		info.attackFrames = 7;
		break;
	}
	return info;
}

// =====================================================================
//  FIGHTER CREATION
// =====================================================================

// Shared setup used by all three initXxx() functions below: stats,
// starting position, idle/walk/attack sprites.
inline void initFighterCore(Fighter &c, FighterType type, float startX, float startY)
{
	memset(&c, 0, sizeof(Fighter));

	FighterTypeInfo info = getFighterTypeInfo(type);

	c.type = type;
	c.maxHealth = info.maxHealth;
	c.currentHealth = info.maxHealth;
	c.moveSpeed = info.moveSpeed;
	c.damage = info.damage;
	c.meleeRange = info.meleeRange;
	c.facing = FACE_RIGHT;
	c.x = startX;
	c.y = startY;
	c.animState = ANIM_IDLE;

	char buf[160];
	sprintf(buf, "Images/%s_idle_left.png", info.spritePrefix);
	c.idleTex[FACE_LEFT] = iLoadImage(buf);
	sprintf(buf, "Images/%s_idle_right.png", info.spritePrefix);
	c.idleTex[FACE_RIGHT] = iLoadImage(buf);

	loadFrames(c.walkTex[FACE_LEFT], info.spritePrefix, "walk_left", info.walkFrames);
	loadFrames(c.walkTex[FACE_RIGHT], info.spritePrefix, "walk_right", info.walkFrames);
	c.walkFrameCount = info.walkFrames;

	loadFrames(c.attackTex[FACE_LEFT], info.spritePrefix, "attack_left", info.attackFrames);
	loadFrames(c.attackTex[FACE_RIGHT], info.spritePrefix, "attack_right", info.attackFrames);
	c.attackFrameCount = info.attackFrames;
}

// Guardian-only: the rotating shield-ring overlay used by tryActivateShield().
inline void initGuardian(Fighter &c, float startX, float startY)
{
	initFighterCore(c, CHAR_GUARDIAN, startX, startY);

	char buf[160];
	for (int i = 0; i < SHIELD_RING_FRAMES; i++)
	{
		sprintf(buf, "Images/shield_around_guardian_%d.png", i + 1);
		c.shieldRingTex[i] = iLoadImage(buf);
	}
}

// Ranger-only: dash sprites + speed-streak trail used by tryDash().
inline void initRanger(Fighter &c, float startX, float startY)
{
	initFighterCore(c, CHAR_RANGER, startX, startY);

	loadFrames(c.dashTex[FACE_LEFT], "ranger", "dash_left", DASH_FRAMES);
	loadFrames(c.dashTex[FACE_RIGHT], "ranger", "dash_right", DASH_FRAMES);
	c.dashTrailTex = iLoadImage("Images/speed_streak_white.png");
}

// Alchemist has no special ability yet - core setup is all it needs.
inline void initAlchemist(Fighter &c, float startX, float startY)
{
	initFighterCore(c, CHAR_ALCHEMIST, startX, startY);
}

// =====================================================================
//  MOVEMENT
// =====================================================================
inline void handleMovement(Fighter &c, bool up, bool down, bool left, bool right)
{
	bool moved = false;

	if (up)    { c.y += c.moveSpeed; moved = true; }
	if (down)  { c.y -= c.moveSpeed; moved = true; }
	if (left)  { c.x -= c.moveSpeed; c.facing = FACE_LEFT;  moved = true; }
	if (right) { c.x += c.moveSpeed; c.facing = FACE_RIGHT; moved = true; }

	float halfW = FIGHTER_DRAW_W / 2.0f;
	float halfH = FIGHTER_DRAW_H / 2.0f;
	if (c.x < halfW)                 c.x = halfW;
	if (c.x > HOME_AREA_W - halfW)   c.x = HOME_AREA_W - halfW;
	if (c.y < halfH)                 c.y = halfH;
	if (c.y > HOME_AREA_H - halfH)   c.y = HOME_AREA_H - halfH;

	if (!c.isAttacking && !c.dashInProgress)
		c.animState = moved ? ANIM_WALK : ANIM_IDLE;
}

// =====================================================================
//  ATTACK
// =====================================================================
inline void applyDamageToFighter(Fighter &target, int damage)
{
	if (target.type == CHAR_GUARDIAN && target.shieldActive) return;

	target.currentHealth -= damage;
	if (target.currentHealth < 0) target.currentHealth = 0;
}

inline void tryAttack(Fighter &attacker, bool triggerRequested, Fighter* targets[], int targetCount)
{
	if (!triggerRequested) return;

	unsigned long now = GetTickCount();
	if (now - attacker.lastAttackTriggerTime < ATTACK_COOLDOWN_MS) return;

	attacker.lastAttackTriggerTime = now;
	attacker.isAttacking = true;
	attacker.animState = ANIM_ATTACK;
	attacker.frameIndex = 0;
	attacker.attackAnimStartTime = now;

	for (int i = 0; i < targetCount; i++)
	{
		Fighter* target = targets[i];
		if (target == &attacker) continue;

		if (distanceBetween(attacker.x, attacker.y, target->x, target->y) <= attacker.meleeRange)
			applyDamageToFighter(*target, attacker.damage);
	}
}

// =====================================================================
//  GUARDIAN - SHIELD ABILITY
// =====================================================================
inline void tryActivateShield(Fighter &guardian, bool triggerRequested)
{
	if (!triggerRequested) return;
	if (guardian.shieldActive || guardian.shieldOnCooldown) return;

	unsigned long now = GetTickCount();
	guardian.shieldActive = true;
	guardian.shieldActiveStartTime = now;
	guardian.shieldFrameIndex = 0;
	guardian.shieldLastFrameSwapTime = now;
}

inline bool isShieldActive(Fighter &guardian)     { return guardian.shieldActive; }
inline bool isShieldOnCooldown(Fighter &guardian) { return guardian.shieldOnCooldown; }

// =====================================================================
//  RANGER - DASH ABILITY
// =====================================================================
inline void tryDash(Fighter &ranger, bool triggerRequested)
{
	if (!triggerRequested) return;

	unsigned long now = GetTickCount();
	if (now - ranger.lastDashTriggerTime < DASH_COOLDOWN_MS) return;

	ranger.lastDashTriggerTime = now;

	float dx = (ranger.facing == FACE_RIGHT) ? DASH_DISTANCE : -DASH_DISTANCE;
	ranger.x += dx;

	float halfW = FIGHTER_DRAW_W / 2.0f;
	if (ranger.x < halfW)               ranger.x = halfW;
	if (ranger.x > HOME_AREA_W - halfW) ranger.x = HOME_AREA_W - halfW;

	ranger.dashInProgress = true;
	ranger.dashAnimStartTime = now;
	ranger.animState = ANIM_DASH;
	ranger.frameIndex = 0;
}

// =====================================================================
//  PER-TICK UPDATE
// =====================================================================
inline void updateFighter(Fighter &c)
{
	unsigned long now = GetTickCount();

	int frameCount = 1;
	int interval = WALK_FRAME_INTERVAL_MS;

	if (c.animState == ANIM_WALK)
	{
		frameCount = c.walkFrameCount;
		interval = WALK_FRAME_INTERVAL_MS;
	}
	else if (c.animState == ANIM_ATTACK)
	{
		frameCount = c.attackFrameCount;
		interval = ATTACK_FRAME_INTERVAL_MS;
	}
	else if (c.animState == ANIM_DASH)
	{
		frameCount = DASH_FRAMES;
		interval = DASH_FRAME_INTERVAL_MS;
	}

	c.frameIndex = (frameCount > 1)
		? advanceFrame(c.frameIndex, frameCount, c.lastFrameSwapTime, interval)
		: 0;

	if (c.isAttacking)
	{
		unsigned long attackDuration = (unsigned long)(c.attackFrameCount * ATTACK_FRAME_INTERVAL_MS);
		if (now - c.attackAnimStartTime >= attackDuration)
		{
			c.isAttacking = false;
			c.animState = ANIM_IDLE;
			c.frameIndex = 0;
		}
	}

	if (c.type == CHAR_GUARDIAN)
	{
		if (c.shieldActive && now - c.shieldActiveStartTime >= SHIELD_ACTIVE_MS)
		{
			c.shieldActive = false;
			c.shieldOnCooldown = true;
			c.shieldCooldownStartTime = now;
		}
		if (c.shieldOnCooldown && now - c.shieldCooldownStartTime >= SHIELD_COOLDOWN_MS)
			c.shieldOnCooldown = false;

		if (c.shieldActive)
			c.shieldFrameIndex = advanceFrame(c.shieldFrameIndex, SHIELD_RING_FRAMES,
				c.shieldLastFrameSwapTime, SHIELD_RING_FRAME_INTERVAL_MS);
	}

	if (c.type == CHAR_RANGER && c.dashInProgress && now - c.dashAnimStartTime >= DASH_ANIM_MS)
	{
		c.dashInProgress = false;
		c.animState = ANIM_IDLE;
		c.frameIndex = 0;
	}
}

// =====================================================================
//  DRAWING
// =====================================================================
inline void drawFighter(Fighter &c)
{
	unsigned int tex;
	switch (c.animState)
	{
	case ANIM_WALK:   tex = c.walkTex[c.facing][c.frameIndex];   break;
	case ANIM_ATTACK: tex = c.attackTex[c.facing][c.frameIndex]; break;
	case ANIM_DASH:   tex = c.dashTex[c.facing][c.frameIndex];   break;
	default:          tex = c.idleTex[c.facing];                break;
	}

	if (c.type == CHAR_RANGER && c.dashInProgress)
	{
		float trailX = (c.facing == FACE_RIGHT) ? c.x - DASH_TRAIL_OFFSET : c.x + DASH_TRAIL_OFFSET;
		iShowImage((int)(trailX - FIGHTER_DRAW_W / 2), (int)(c.y - FIGHTER_DRAW_H / 2),
			FIGHTER_DRAW_W, FIGHTER_DRAW_H, c.dashTrailTex);
	}

	iShowImage((int)(c.x - FIGHTER_DRAW_W / 2), (int)(c.y - FIGHTER_DRAW_H / 2),
		FIGHTER_DRAW_W, FIGHTER_DRAW_H, tex);

	if (c.type == CHAR_GUARDIAN && c.shieldActive)
	{
		int shieldSize = 60;
		unsigned int ring = c.shieldRingTex[c.shieldFrameIndex];
		iShowImage((int)(c.x - shieldSize / 2), (int)(c.y - shieldSize / 2),
			shieldSize, shieldSize, ring);
	}
}
