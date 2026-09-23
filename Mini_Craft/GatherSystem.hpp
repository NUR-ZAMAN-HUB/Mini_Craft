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
#include "HomeBaseConfig.hpp"
#include "Fighters.hpp"     // Fighter, distanceBetween()
#include <windows.h>
#include <stdio.h>

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

// Stone/Wood/Iron are capped at 500 each - every place that adds to
// g_inventory[RES_STONE/RES_WOOD/RES_IRON] (node gathering below, passive
// trickle, and inventory.hpp's addItem() for those 3 items) clamps through
// this. Water is intentionally left uncapped - only Stone/Wood/Iron were
// asked for.
#define MAX_BASIC_RESOURCE 500

inline void clampBasicResource(ResourceType type)
{
	if (type == RES_WATER) return;   // Water has no cap
	if (g_inventory[type] > MAX_BASIC_RESOURCE) g_inventory[type] = MAX_BASIC_RESOURCE;
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
			g_inventory[node.type]++;
			clampBasicResource(node.type);
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
//  PASSIVE RESOURCE GAIN
//  Stone/Wood/Iron trickle in on their own every 10 seconds, on top
//  of whatever's gathered manually from nodes. Runs from iMain.cpp's
//  fixedUpdate() while the player is in HOMEBASE *or* BATTLE (see the
//  call site there), so it keeps ticking mid-fight - only reset via
//  g_passiveResourceLastTick = 0, which nothing currently does, so it
//  never restarts. Water is intentionally left out; only Stone/Wood/Iron
//  were asked for.
// ---------------------------------------------------------------
#define PASSIVE_RESOURCE_INTERVAL_MS 10000   // 10 seconds
#define PASSIVE_RESOURCE_MIN 1                // random amount is 1-5 inclusive
#define PASSIVE_RESOURCE_MAX 5

unsigned long g_passiveResourceLastTick = 0;

inline void updatePassiveResourceGain()
{
	unsigned long now = GetTickCount();

	// First call ever: just start the clock, don't award anything yet.
	if (g_passiveResourceLastTick == 0)
	{
		g_passiveResourceLastTick = now;
		return;
	}

	if (now - g_passiveResourceLastTick >= PASSIVE_RESOURCE_INTERVAL_MS)
	{
		g_passiveResourceLastTick = now;

		int gain = PASSIVE_RESOURCE_MIN + rand() % (PASSIVE_RESOURCE_MAX - PASSIVE_RESOURCE_MIN + 1);
		g_inventory[RES_STONE] += gain;
		clampBasicResource(RES_STONE);

		gain = PASSIVE_RESOURCE_MIN + rand() % (PASSIVE_RESOURCE_MAX - PASSIVE_RESOURCE_MIN + 1);
		g_inventory[RES_WOOD] += gain;
		clampBasicResource(RES_WOOD);

		gain = PASSIVE_RESOURCE_MIN + rand() % (PASSIVE_RESOURCE_MAX - PASSIVE_RESOURCE_MIN + 1);
		g_inventory[RES_IRON] += gain;
		clampBasicResource(RES_IRON);
	}
}

// ---------------------------------------------------------------
//  GETTERS (for HUD text)
// ---------------------------------------------------------------
inline int getStoneCount() { return g_inventory[RES_STONE]; }
inline int getWoodCount()  { return g_inventory[RES_WOOD]; }
inline int getIronCount()  { return g_inventory[RES_IRON]; }
inline int getWaterCount() { return g_inventory[RES_WATER]; }
