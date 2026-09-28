// =====================================================================
//  Fighters.hpp  (Home Base combat characters)
// =====================================================================
//  3 playable characters, all built from ONE struct (Fighter) with
//  different numbers and different images:
//     Guardian  - tanky, slow, short range, has a Shield ability
//     Ranger    - fragile, fast, long range, has a Dash ability
//     Alchemist - middle stats, long range, no special ability yet
//
//  STATS[] below holds each character's numbers + sprite file prefix,
//  in the same order as the FighterType enum, so initFighter() can
//  just look up STATS[type] instead of repeating the same setup code
//  three times.
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
enum FighterType { CHAR_GUARDIAN = 0, CHAR_RANGER = 1, CHAR_ALCHEMIST = 2 };
enum FacingDir   { FACE_LEFT = 0, FACE_RIGHT = 1 };
enum AnimState   { ANIM_IDLE = 0, ANIM_WALK = 1, ANIM_ATTACK = 2, ANIM_DASH = 3 };

// ---------------------------------------------------------------
//  THE FIGHTER STRUCT - one on-screen character + everything it needs to draw itself
// ---------------------------------------------------------------
struct Fighter
{
	FighterType type;

	int   maxHealth, currentHealth, damage;
	float moveSpeed, meleeRange;

	float x, y;
	FacingDir facing;

	AnimState     animState;
	int           frameIndex;
	unsigned long lastFrameSwapTime;

	unsigned int idleTex[2];
	unsigned int walkTex[2][WALK_MAX_FRAMES];
	int          walkFrameCount;
	unsigned int attackTex[2][ATTACK_MAX_FRAMES];
	int          attackFrameCount;

	bool          isAttacking;
	unsigned long attackAnimStartTime, lastAttackTriggerTime;

	// Guardian-only
	bool          shieldActive, shieldOnCooldown;
	unsigned long shieldActiveStartTime, shieldCooldownStartTime, shieldLastFrameSwapTime;
	int           shieldFrameIndex;
	unsigned int  shieldRingTex[SHIELD_RING_FRAMES];

	// Ranger-only
	bool          dashInProgress;
	unsigned long dashAnimStartTime, lastDashTriggerTime;
	unsigned int  dashTex[2][DASH_FRAMES];
	unsigned int  dashTrailTex;
};

// ---------------------------------------------------------------
//  PER-CHARACTER STATS TABLE (index = FighterType)
// ---------------------------------------------------------------
struct FighterStats { const char* prefix; int hp, dmg; float speed, range; int walkFrames, atkFrames; };

static const FighterStats STATS[3] = {
	{ "guardian",  GUARDIAN_MAX_HEALTH,  GUARDIAN_DAMAGE,  GUARDIAN_MOVE_SPEED,  GUARDIAN_MELEE_RANGE,  3, 5 },
	{ "ranger",    RANGER_MAX_HEALTH,    RANGER_DAMAGE,    RANGER_MOVE_SPEED,    RANGER_MELEE_RANGE,    4, 4 },
	{ "alchemist", ALCHEMIST_MAX_HEALTH, ALCHEMIST_DAMAGE, ALCHEMIST_MOVE_SPEED, ALCHEMIST_MELEE_RANGE, 4, 7 },
};

// =====================================================================
//  SMALL HELPERS
// =====================================================================
inline float distanceBetween(float x1, float y1, float x2, float y2)
{
	float dx = x2 - x1, dy = y2 - y1;
	return sqrtf(dx * dx + dy * dy);
}

// Moves to the next animation frame once "intervalMs" have passed.
inline int nextAnimationFrame(int frame, int frameCount, unsigned long &lastSwapTime, int intervalMs)
{
	unsigned long now = GetTickCount();
	if (now - lastSwapTime >= (unsigned long)intervalMs)
	{
		lastSwapTime = now;
		frame = (frame + 1) % frameCount;
	}
	return frame;
}

// Loads "Images/<prefix>_<actionAndDir>_0<1..count>.png" into dest[].
inline void loadFrames(unsigned int dest[], const char* prefix, const char* actionAndDir, int count)
{
	char path[160];
	for (int i = 0; i < count; i++)
	{
		sprintf(path, "Images/%s_%s_0%d.png", prefix, actionAndDir, i + 1);
		dest[i] = iLoadImage(path);
	}
}

inline const char* fighterTypeName(FighterType t)
{
	if (t == CHAR_GUARDIAN) return "Guardian";
	if (t == CHAR_RANGER)   return "Ranger";
	return "Alchemist";
}

// =====================================================================
//  FIGHTER CREATION - one shared function, driven by STATS[type]
// =====================================================================
inline void initFighter(Fighter &c, FighterType type, float startX, float startY)
{
	memset(&c, 0, sizeof(Fighter));
	const FighterStats &s = STATS[type];

	c.type = type;
	c.maxHealth = c.currentHealth = s.hp;
	c.moveSpeed = s.speed;
	c.damage = s.dmg;
	c.meleeRange = s.range;
	c.x = startX; c.y = startY;
	c.facing = FACE_RIGHT;
	c.animState = ANIM_IDLE;

	char path[160];
	sprintf(path, "Images/%s_idle_left.png", s.prefix);  c.idleTex[FACE_LEFT] = iLoadImage(path);
	sprintf(path, "Images/%s_idle_right.png", s.prefix); c.idleTex[FACE_RIGHT] = iLoadImage(path);

	c.walkFrameCount = s.walkFrames;
	loadFrames(c.walkTex[FACE_LEFT], s.prefix, "walk_left", s.walkFrames);
	loadFrames(c.walkTex[FACE_RIGHT], s.prefix, "walk_right", s.walkFrames);

	c.attackFrameCount = s.atkFrames;
	loadFrames(c.attackTex[FACE_LEFT], s.prefix, "attack_left", s.atkFrames);
	loadFrames(c.attackTex[FACE_RIGHT], s.prefix, "attack_right", s.atkFrames);

	// Ability-specific extra images
	if (type == CHAR_GUARDIAN)
	{
		for (int i = 0; i < SHIELD_RING_FRAMES; i++)
		{
			sprintf(path, "Images/shield_around_guardian_%d.png", i + 1);
			c.shieldRingTex[i] = iLoadImage(path);
		}
	}
	else if (type == CHAR_RANGER)
	{
		loadFrames(c.dashTex[FACE_LEFT], "ranger", "dash_left", DASH_FRAMES);
		loadFrames(c.dashTex[FACE_RIGHT], "ranger", "dash_right", DASH_FRAMES);
		c.dashTrailTex = iLoadImage("Images/speed_streak_white.png");
	}
}

// =====================================================================
//  MOVEMENT
// =====================================================================
inline void handleMovement(Fighter &c, bool up, bool down, bool left, bool right)
{
	bool moved = up || down || left || right;

	if (up)    c.y += c.moveSpeed;
	if (down)  c.y -= c.moveSpeed;
	if (left)  { c.x -= c.moveSpeed; c.facing = FACE_LEFT;  }
	if (right) { c.x += c.moveSpeed; c.facing = FACE_RIGHT; }

	// keep the fighter inside the play area
	float halfW = FIGHTER_DRAW_W / 2.0f, halfH = FIGHTER_DRAW_H / 2.0f;
	if (c.x < halfW)               c.x = halfW;
	if (c.x > HOME_AREA_W - halfW) c.x = HOME_AREA_W - halfW;
	if (c.y < halfH)               c.y = halfH;
	if (c.y > HOME_AREA_H - halfH) c.y = HOME_AREA_H - halfH;

	if (!c.isAttacking && !c.dashInProgress)
		c.animState = moved ? ANIM_WALK : ANIM_IDLE;
}

// =====================================================================
//  ATTACK
// =====================================================================
inline void applyDamageToFighter(Fighter &target, int damage)
{
	if (target.type == CHAR_GUARDIAN && target.shieldActive) return;   // shield blocks all damage
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
		Fighter* t = targets[i];
		if (t == &attacker) continue;
		if (distanceBetween(attacker.x, attacker.y, t->x, t->y) <= attacker.meleeRange)
			applyDamageToFighter(*t, attacker.damage);
	}
}

// =====================================================================
//  GUARDIAN SHIELD  /  RANGER DASH
// =====================================================================
inline void tryActivateShield(Fighter &guardian, bool triggerRequested)
{
	if (!triggerRequested || guardian.shieldActive || guardian.shieldOnCooldown) return;

	unsigned long now = GetTickCount();
	guardian.shieldActive = true;
	guardian.shieldActiveStartTime = now;
	guardian.shieldFrameIndex = 0;
	guardian.shieldLastFrameSwapTime = now;
}

inline bool isShieldActive(Fighter &guardian)     { return guardian.shieldActive; }
inline bool isShieldOnCooldown(Fighter &guardian) { return guardian.shieldOnCooldown; }

inline void tryDash(Fighter &ranger, bool triggerRequested)
{
	if (!triggerRequested) return;

	unsigned long now = GetTickCount();
	if (now - ranger.lastDashTriggerTime < DASH_COOLDOWN_MS) return;
	ranger.lastDashTriggerTime = now;

	ranger.x += (ranger.facing == FACE_RIGHT) ? DASH_DISTANCE : -DASH_DISTANCE;
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

	if (c.animState == ANIM_WALK)
		c.frameIndex = nextAnimationFrame(c.frameIndex, c.walkFrameCount, c.lastFrameSwapTime, WALK_FRAME_INTERVAL_MS);
	else if (c.animState == ANIM_ATTACK)
		c.frameIndex = nextAnimationFrame(c.frameIndex, c.attackFrameCount, c.lastFrameSwapTime, ATTACK_FRAME_INTERVAL_MS);
	else if (c.animState == ANIM_DASH)
		c.frameIndex = nextAnimationFrame(c.frameIndex, DASH_FRAMES, c.lastFrameSwapTime, DASH_FRAME_INTERVAL_MS);
	else
		c.frameIndex = 0;   // idle

	if (c.isAttacking && now - c.attackAnimStartTime >= (unsigned long)(c.attackFrameCount * ATTACK_FRAME_INTERVAL_MS))
	{
		c.isAttacking = false;
		c.animState = ANIM_IDLE;
		c.frameIndex = 0;
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
			c.shieldFrameIndex = nextAnimationFrame(c.shieldFrameIndex, SHIELD_RING_FRAMES, c.shieldLastFrameSwapTime, SHIELD_RING_FRAME_INTERVAL_MS);
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
	if (c.animState == ANIM_WALK)        tex = c.walkTex[c.facing][c.frameIndex];
	else if (c.animState == ANIM_ATTACK) tex = c.attackTex[c.facing][c.frameIndex];
	else if (c.animState == ANIM_DASH)   tex = c.dashTex[c.facing][c.frameIndex];
	else                                 tex = c.idleTex[c.facing];

	if (c.type == CHAR_RANGER && c.dashInProgress)
	{
		float trailX = (c.facing == FACE_RIGHT) ? c.x - DASH_TRAIL_OFFSET : c.x + DASH_TRAIL_OFFSET;
		iShowImage((int)(trailX - FIGHTER_DRAW_W / 2), (int)(c.y - FIGHTER_DRAW_H / 2), FIGHTER_DRAW_W, FIGHTER_DRAW_H, c.dashTrailTex);
	}

	iShowImage((int)(c.x - FIGHTER_DRAW_W / 2), (int)(c.y - FIGHTER_DRAW_H / 2), FIGHTER_DRAW_W, FIGHTER_DRAW_H, tex);

	if (c.type == CHAR_GUARDIAN && c.shieldActive)
	{
		int size = 60;
		iShowImage((int)(c.x - size / 2), (int)(c.y - size / 2), size, size, c.shieldRingTex[c.shieldFrameIndex]);
	}
}
