// =====================================================================
//  Player.hpp
// =====================================================================
#pragma once

#include "iGraphics.h"
#include "GameConfig.hpp"
#include <windows.h>   // GetTickCount()
#include <math.h>      // sqrt()
#include <stdio.h>     // sprintf()
#include <string.h>    // memset()

// ---------------------------------------------------------------
//  ENUMS
// ---------------------------------------------------------------
enum CharacterType
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
//  THE CHARACTER STRUCT
// ---------------------------------------------------------------
struct Character
{
	CharacterType type;

	int   maxHealth;
	int   currentHealth;
	float moveSpeed;
	int   damage;
	float meleeRange;
	FacingDir facing;

	float x, y;

	AnimState    animState;
	int          frameIndex;
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

float distanceBetween(float x1, float y1, float x2, float y2)
{
	float dx = x2 - x1;
	float dy = y2 - y1;
	return sqrtf(dx * dx + dy * dy);
}

int advanceFrame(int currentFrame, int frameCount, unsigned long &lastSwapTime, int intervalMs)
{
	unsigned long now = GetTickCount();
	if (now - lastSwapTime >= (unsigned long)intervalMs)
	{
		lastSwapTime = now;
		currentFrame = (currentFrame + 1) % frameCount;
	}
	return currentFrame;
}

void loadFrames(unsigned int destRow[], char* prefix, char* actionAndDir, int frameCount)
{
	char buf[160];
	for (int i = 0; i < frameCount; i++)
	{
		sprintf(buf, "Images/%s_%s_0%d.png", prefix, actionAndDir, i + 1);
		destRow[i] = iLoadImage(buf);
	}
}

char* characterName(CharacterType t)
{
	if (t == CHAR_GUARDIAN)  return "Guardian";
	if (t == CHAR_RANGER)    return "Ranger";
	return "Alchemist";
}

// =====================================================================
//  CHARACTER CREATION
// =====================================================================
void initGuardian(Character &c, float startX, float startY)
{
	memset(&c, 0, sizeof(Character));

	c.type = CHAR_GUARDIAN;
	c.maxHealth = GUARDIAN_MAX_HEALTH;
	c.currentHealth = GUARDIAN_MAX_HEALTH;
	c.moveSpeed = GUARDIAN_MOVE_SPEED;
	c.damage = GUARDIAN_DAMAGE;
	c.meleeRange = GUARDIAN_MELEE_RANGE;
	c.facing = FACE_RIGHT;
	c.x = startX;
	c.y = startY;
	c.animState = ANIM_IDLE;

	c.idleTex[FACE_LEFT] = iLoadImage("Images/guardian_idle_left.png");
	c.idleTex[FACE_RIGHT] = iLoadImage("Images/guardian_idle_right.png");

	loadFrames(c.walkTex[FACE_LEFT], "guardian", "walk_left", 3);
	loadFrames(c.walkTex[FACE_RIGHT], "guardian", "walk_right", 3);
	c.walkFrameCount = 3;

	loadFrames(c.attackTex[FACE_LEFT], "guardian", "attack_left", 5);
	loadFrames(c.attackTex[FACE_RIGHT], "guardian", "attack_right", 5);
	c.attackFrameCount = 5;

	char buf[160];
	for (int i = 0; i < SHIELD_RING_FRAMES; i++)
	{
		sprintf(buf, "Images/shield_around_guardian_%d.png", i + 1);
		c.shieldRingTex[i] = iLoadImage(buf);
	}
}

void initRanger(Character &c, float startX, float startY)
{
	memset(&c, 0, sizeof(Character));

	c.type = CHAR_RANGER;
	c.maxHealth = RANGER_MAX_HEALTH;
	c.currentHealth = RANGER_MAX_HEALTH;
	c.moveSpeed = RANGER_MOVE_SPEED;
	c.damage = RANGER_DAMAGE;
	c.meleeRange = RANGER_MELEE_RANGE;
	c.facing = FACE_RIGHT;
	c.x = startX;
	c.y = startY;
	c.animState = ANIM_IDLE;

	c.idleTex[FACE_LEFT] = iLoadImage("Images/ranger_idle_left.png");
	c.idleTex[FACE_RIGHT] = iLoadImage("Images/ranger_idle_right.png");

	loadFrames(c.walkTex[FACE_LEFT], "ranger", "walk_left", 4);
	loadFrames(c.walkTex[FACE_RIGHT], "ranger", "walk_right", 4);
	c.walkFrameCount = 4;

	loadFrames(c.attackTex[FACE_LEFT], "ranger", "attack_left", 4);
	loadFrames(c.attackTex[FACE_RIGHT], "ranger", "attack_right", 4);
	c.attackFrameCount = 4;

	loadFrames(c.dashTex[FACE_LEFT], "ranger", "dash_left", DASH_FRAMES);
	loadFrames(c.dashTex[FACE_RIGHT], "ranger", "dash_right", DASH_FRAMES);
	c.dashTrailTex = iLoadImage("Images/speed_streak_white.png");
}

void initAlchemist(Character &c, float startX, float startY)
{
	memset(&c, 0, sizeof(Character));

	c.type = CHAR_ALCHEMIST;
	c.maxHealth = ALCHEMIST_MAX_HEALTH;
	c.currentHealth = ALCHEMIST_MAX_HEALTH;
	c.moveSpeed = ALCHEMIST_MOVE_SPEED;
	c.damage = ALCHEMIST_DAMAGE;
	c.meleeRange = ALCHEMIST_MELEE_RANGE;
	c.facing = FACE_RIGHT;
	c.x = startX;
	c.y = startY;
	c.animState = ANIM_IDLE;

	c.idleTex[FACE_LEFT] = iLoadImage("Images/alchemist_idle_left.png");
	c.idleTex[FACE_RIGHT] = iLoadImage("Images/alchemist_idle_right.png");

	loadFrames(c.walkTex[FACE_LEFT], "alchemist", "walk_left", 4);
	loadFrames(c.walkTex[FACE_RIGHT], "alchemist", "walk_right", 4);
	c.walkFrameCount = 4;

	loadFrames(c.attackTex[FACE_LEFT], "alchemist", "attack_left", 7);
	loadFrames(c.attackTex[FACE_RIGHT], "alchemist", "attack_right", 7);
	c.attackFrameCount = 7;
}

// =====================================================================
//  MOVEMENT
// =====================================================================
void handleMovement(Character &c, bool up, bool down, bool left, bool right)
{
	bool moved = false;

	if (up)    { c.y += c.moveSpeed; moved = true; }
	if (down)  { c.y -= c.moveSpeed; moved = true; }
	if (left)  { c.x -= c.moveSpeed; c.facing = FACE_LEFT;  moved = true; }
	if (right) { c.x += c.moveSpeed; c.facing = FACE_RIGHT; moved = true; }

	float halfW = CHAR_DRAW_W / 2.0f;
	float halfH = CHAR_DRAW_H / 2.0f;
	if (c.x < halfW)                    c.x = halfW;
	if (c.x > SCREEN_WIDTH - halfW)    c.x = SCREEN_WIDTH - halfW;
	if (c.y < halfH)                    c.y = halfH;
	if (c.y > SCREEN_HEIGHT - halfH)    c.y = SCREEN_HEIGHT - halfH;

	if (!c.isAttacking && !c.dashInProgress)
	{
		c.animState = moved ? ANIM_WALK : ANIM_IDLE;
	}
}

// =====================================================================
//  ATTACK
// =====================================================================
void applyDamageToCharacter(Character &target, int damage)
{
	if (target.type == CHAR_GUARDIAN && target.shieldActive)
	{
		return;
	}

	target.currentHealth -= damage;
	if (target.currentHealth < 0) target.currentHealth = 0;
}

void tryAttack(Character &attacker, bool triggerRequested, Character* targets[], int targetCount)
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
		Character* target = targets[i];
		if (target == &attacker) continue;

		if (distanceBetween(attacker.x, attacker.y, target->x, target->y) <= attacker.meleeRange)
		{
			applyDamageToCharacter(*target, attacker.damage);
		}
	}
}

// =====================================================================
//  GUARDIAN - SHIELD ABILITY
// =====================================================================
void tryActivateShield(Character &guardian, bool triggerRequested)
{
	if (!triggerRequested) return;
	if (guardian.shieldActive || guardian.shieldOnCooldown) return;

	unsigned long now = GetTickCount();
	guardian.shieldActive = true;
	guardian.shieldActiveStartTime = now;
	guardian.shieldFrameIndex = 0;
	guardian.shieldLastFrameSwapTime = now;
}

bool isShieldActive(Character &guardian)     { return guardian.shieldActive; }
bool isShieldOnCooldown(Character &guardian) { return guardian.shieldOnCooldown; }

// =====================================================================
//  RANGER - DASH ABILITY
// =====================================================================
void tryDash(Character &ranger, bool triggerRequested)
{
	if (!triggerRequested) return;

	unsigned long now = GetTickCount();
	if (now - ranger.lastDashTriggerTime < DASH_COOLDOWN_MS) return;

	ranger.lastDashTriggerTime = now;

	float dx = (ranger.facing == FACE_RIGHT) ? DASH_DISTANCE : -DASH_DISTANCE;
	ranger.x += dx;

	float halfW = CHAR_DRAW_W / 2.0f;
	if (ranger.x < halfW)                 ranger.x = halfW;
	if (ranger.x > SCREEN_WIDTH - halfW)  ranger.x = SCREEN_WIDTH - halfW;

	ranger.dashInProgress = true;
	ranger.dashAnimStartTime = now;
	ranger.animState = ANIM_DASH;
	ranger.frameIndex = 0;
}

// =====================================================================
//  PER-TICK UPDATE
// =====================================================================
void updateCharacter(Character &c)
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

	if (frameCount > 1)
		c.frameIndex = advanceFrame(c.frameIndex, frameCount, c.lastFrameSwapTime, interval);
	else
		c.frameIndex = 0;

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
		{
			c.shieldOnCooldown = false;
		}
		if (c.shieldActive)
		{
			c.shieldFrameIndex = advanceFrame(c.shieldFrameIndex, SHIELD_RING_FRAMES,
				c.shieldLastFrameSwapTime, SHIELD_RING_FRAME_INTERVAL_MS);
		}
	}

	if (c.type == CHAR_RANGER && c.dashInProgress)
	{
		if (now - c.dashAnimStartTime >= DASH_ANIM_MS)
		{
			c.dashInProgress = false;
			c.animState = ANIM_IDLE;
			c.frameIndex = 0;
		}
	}
}

// =====================================================================
//  DRAWING (UPDATED WITH SMALLER SIZE)
// =====================================================================
void drawCharacter(Character &c)
{
	unsigned int tex;

	switch (c.animState)
	{
	case ANIM_WALK:   tex = c.walkTex[c.facing][c.frameIndex];   break;
	case ANIM_ATTACK: tex = c.attackTex[c.facing][c.frameIndex]; break;
	case ANIM_DASH:   tex = c.dashTex[c.facing][c.frameIndex];   break;
	default:          tex = c.idleTex[c.facing];                break;
	}

	// Custom Smaller Size Dimensions
	int customW = 56;
	int customH = 61;

	if (c.type == CHAR_RANGER && c.dashInProgress)
	{
		float trailX = (c.facing == FACE_RIGHT) ? c.x - DASH_TRAIL_OFFSET : c.x + DASH_TRAIL_OFFSET;
		iShowImage((int)(trailX - customW / 2), (int)(c.y - customH / 2),
			customW, customH, c.dashTrailTex);
	}

	// Render smaller sprite
	iShowImage((int)(c.x - customW / 2), (int)(c.y - customH / 2),
		customW, customH, tex);

	int shieldSize = 60; 

	if (c.type == CHAR_GUARDIAN && c.shieldActive)
	{
		unsigned int ring = c.shieldRingTex[c.shieldFrameIndex];
		iShowImage((int)(c.x - shieldSize / 2), (int)(c.y - shieldSize / 2),
			shieldSize, shieldSize, ring);
	}
	
}