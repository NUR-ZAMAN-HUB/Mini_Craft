// =====================================================================
//  GatherSystem.hpp  (Home Base resource nodes)
// =====================================================================
//  4 resource types: Stone, Wood, Iron, Water. Each is a fixed spot
//  on the map. Flow: walk close + left-click -> gather timer starts
//  (with a progress bar/ring) -> on finish, inventory +1 and the node
//  depletes -> after a respawn delay it becomes available again.
//
//  NODE_CFG[] holds each node's position + timing, in the same order
//  as ResourceType, so setup/lookup is just one array index instead
//  of repeating code per resource.
// =====================================================================
#pragma once

#include "iGraphics.h"
#include "Menu.h"           // currentLevel - see RESOURCE_CAP_PER_LEVEL / getResourceCap() below
#include "HomeBaseConfig.hpp"
#include "Fighters.hpp"     // Fighter, distanceBetween()
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>   // rand()/srand() - see updateRandomResourceBonus() below
#include <time.h>     // time() - seeds rand() once

enum ResourceType { RES_STONE = 0, RES_WOOD = 1, RES_IRON = 2, RES_WATER = 3 };

struct ResourceNode
{
	ResourceType type;
	float x, y;
	bool available, isBeingGathered;
	unsigned long gatherStartTime, gatherTimeMs;
	unsigned long depletedAtTime, respawnTimeMs;
};

// Position + timing per resource type (index = ResourceType)
struct NodeConfig { float x, y; unsigned long gatherMs, respawnMs; };

static const NodeConfig NODE_CFG[4] = {
	{ STONE_NODE_X, STONE_NODE_Y, STONE_GATHER_MS, STONE_RESPAWN_MS },
	{ WOOD_NODE_X,  WOOD_NODE_Y,  WOOD_GATHER_MS,  WOOD_RESPAWN_MS },
	{ IRON_NODE_X,  IRON_NODE_Y,  IRON_GATHER_MS,  IRON_RESPAWN_MS },
	{ WATER_NODE_X, WATER_NODE_Y, WATER_GATHER_MS, WATER_RESPAWN_MS },
};

// How many of each resource the player has - one shared pool, indexed
// by ResourceType (0=stone, 1=wood, 2=iron, 3=water). Plain global
// (not "inline"): this header is only ever included once, from
// iMain.cpp, and this project's C++ standard predates "inline" vars.
int g_inventory[4] = { 0, 0, 0, 0 };
unsigned int g_gatherRingTex = 0;   // spinning "in progress" icon, loaded in main()

// Stone/Wood/Iron are capped at 100 per level (Water is unlimited - there's no
// equivalent cap on it, since nothing asked for one). currentLevel starts at 1
// (see iMain.cpp), so the starting cap is 100 and it rises by 100 each time the
// player clears a "Save the Witch" map and steps through the portal.
#define RESOURCE_CAP_PER_LEVEL 100
inline int getResourceCap() { return RESOURCE_CAP_PER_LEVEL * currentLevel; }

// Adds "amount" to one resource, clamping Stone/Wood/Iron at getResourceCap()
// (Water passes straight through uncapped). Both gather sources - the resource
// nodes below and the random bonus - go through this instead of touching
// g_inventory[] directly, so the cap can't be missed by either path.
inline void addResource(ResourceType type, int amount)
{
	g_inventory[type] += amount;

	if (type != RES_WATER)
	{
		int cap = getResourceCap();
		if (g_inventory[type] > cap) g_inventory[type] = cap;
	}
}

// ---------------------------------------------------------------
//  SETUP
// ---------------------------------------------------------------
inline void initAllResourceNodes(ResourceNode nodes[4])
{
	for (int i = 0; i < 4; i++)
	{
		nodes[i].type = (ResourceType)i;
		nodes[i].x = NODE_CFG[i].x;
		nodes[i].y = NODE_CFG[i].y;
		nodes[i].available = true;
		nodes[i].isBeingGathered = false;
		nodes[i].gatherStartTime = 0;
		nodes[i].gatherTimeMs = NODE_CFG[i].gatherMs;
		nodes[i].depletedAtTime = 0;
		nodes[i].respawnTimeMs = NODE_CFG[i].respawnMs;
	}
}

// ---------------------------------------------------------------
//  INTERACTION (left-click = "Gather")
// ---------------------------------------------------------------
// Starts gathering the first available node within range of the player.
inline void tryGatherNearestNode(Fighter &player, ResourceNode nodes[], int count)
{
	for (int i = 0; i < count; i++)
	{
		if (!nodes[i].available || nodes[i].isBeingGathered) continue;

		if (distanceBetween(player.x, player.y, nodes[i].x, nodes[i].y) <= INTERACT_RADIUS)
		{
			nodes[i].isBeingGathered = true;
			nodes[i].gatherStartTime = GetTickCount();
			return;   // only one node gathers at a time
		}
	}
}

// ---------------------------------------------------------------
//  PER-TICK UPDATE
// ---------------------------------------------------------------
inline void updateResourceNode(ResourceNode &node)
{
	unsigned long now = GetTickCount();

	if (node.isBeingGathered)
	{
		if (now - node.gatherStartTime >= node.gatherTimeMs)
		{
			node.isBeingGathered = false;
			node.available = false;
			node.depletedAtTime = now;
			addResource(node.type, 1);
		}
	}
	else if (!node.available && now - node.depletedAtTime >= node.respawnTimeMs)
	{
		node.available = true;
	}
}

// ---------------------------------------------------------------
//  DRAWING
// ---------------------------------------------------------------
// The node's idle picture is already part of the Home Base background,
// so this only draws the progress bar + spinning ring while gathering.
inline void drawResourceNode(ResourceNode &node)
{
	if (!node.available || !node.isBeingGathered) return;

	unsigned long now = GetTickCount();
	float progress = (float)(now - node.gatherStartTime) / (float)node.gatherTimeMs;
	if (progress > 1.0f) progress = 1.0f;

	float barX = node.x - 25, barY = node.y + RESOURCE_DRAW_H / 2 + 12;
	iSetColor(0.15, 0.15, 0.15);
	iFilledRectangle(barX, barY, 50, 6);
	iSetColor(0.25, 0.85, 0.35);
	iFilledRectangle(barX, barY, 50 * progress, 6);

	iShowImage((int)(node.x - GATHER_RING_SIZE / 2), (int)(node.y - GATHER_RING_SIZE / 2),
		GATHER_RING_SIZE, GATHER_RING_SIZE, g_gatherRingTex);
}

// ---------------------------------------------------------------
//  RANDOM RESOURCE BONUS (every 5 seconds, while in Home Base)
//  Rolls a number 1-100 on a timer; which third it lands in decides
//  which single resource gets +5 for free. Called once per tick from
//  HomeBase_FixedUpdate() - see updateRandomResourceBonus() below.
// ---------------------------------------------------------------
#define RANDOM_BONUS_INTERVAL_MS 5000   // 5 seconds
#define RANDOM_BONUS_AMOUNT      5

inline void updateRandomResourceBonus()
{
	// Seeds rand() once. Battle.hpp seeds it too (its own fights need randomness
	// before Home Base necessarily runs a tick) - reseeding twice with time(NULL)
	// is harmless, each guarded by its own "only once" flag.
	static bool seeded = false;
	if (!seeded) { seeded = true; srand((unsigned int)time(NULL)); }

	static unsigned long lastBonusTime = 0;
	unsigned long now = GetTickCount();
	if (lastBonusTime == 0) lastBonusTime = now;   // first call just starts the 5s timer
	if (now - lastBonusTime < RANDOM_BONUS_INTERVAL_MS) return;
	lastBonusTime = now;

	int roll = rand() % 100 + 1;   // 1..100
	ResourceType bonusType;
	if (roll <= 33)       bonusType = RES_STONE;   // 1-33   -> Stone
	else if (roll <= 66)  bonusType = RES_WOOD;    // 34-66  -> Wood
	else                  bonusType = RES_IRON;    // 67-100 -> Iron

	addResource(bonusType, RANDOM_BONUS_AMOUNT);
}

// ---------------------------------------------------------------
//  GETTERS (for HUD text)
// ---------------------------------------------------------------
inline int getStoneCount() { return g_inventory[RES_STONE]; }
inline int getWoodCount()  { return g_inventory[RES_WOOD]; }
inline int getIronCount()  { return g_inventory[RES_IRON]; }
inline int getWaterCount() { return g_inventory[RES_WATER]; }
