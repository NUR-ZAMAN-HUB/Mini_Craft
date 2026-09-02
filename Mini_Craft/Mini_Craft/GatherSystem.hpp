// =====================================================================
//  GatherSystem.hpp  (Home Base resource nodes)
// =====================================================================
//  Was "ResourceSystem.hpp" in the standalone Home Base build. Renamed
//  for clarity and updated to use Fighter (formerly Character - see
//  Fighters.hpp for why it was renamed).
//
//  Scope:
//   - 4 resource types: Stone, Wood, Iron, Water
//   - Each is a world node placed at a fixed (hard-coded) position
//   - Player walks near it + left-clicks -> gather timer starts
//   - On completion: inventory count +1, node depletes, respawns later
//   - Counts are exposed via getters for a HUD
// =====================================================================
#pragma once

#include "iGraphics.h"
#include "HomeBaseConfig.hpp"
#include "Fighters.hpp"     // Fighter, distanceBetween()
#include <windows.h>
#include <stdio.h>

// ---------------------------------------------------------------
//  ENUM + STRUCTS
// ---------------------------------------------------------------
enum ResourceType
{
	RES_STONE = 0,
	RES_WOOD = 1,
	RES_IRON = 2,
	RES_WATER = 3
};

struct ResourceNode
{
	ResourceType type;
	float x, y;

	bool available;          // false while depleted, waiting to respawn
	bool isBeingGathered;
	unsigned long gatherStartTime;
	unsigned long gatherTimeMs;   // tunable per resource (Stone/Iron slower)

	unsigned long depletedAtTime;
	unsigned long respawnTimeMs;

	unsigned int nodeTex;
};

struct Inventory
{
	int stone;
	int wood;
	int iron;
	int water;
};

// One shared inventory (the whole roster gathers into the same pool).
// NOTE: plain globals (not "inline") on purpose - VS2013 / C++11 doesn't
// support inline variables (that's a C++17 feature), and this header is
// only ever included once (from iMain.cpp), so a plain global is safe.
Inventory g_inventory = { 0, 0, 0, 0 };
unsigned int g_gatherRingTex = 0;

// ---------------------------------------------------------------
//  SETUP
// ---------------------------------------------------------------
inline void initResourceNode(ResourceNode &node, ResourceType type, float x, float y,
	const char* textureFile, unsigned long gatherTimeMs, unsigned long respawnTimeMs)
{
	node.type = type;
	node.x = x;
	node.y = y;
	node.available = true;
	node.isBeingGathered = false;
	node.gatherStartTime = 0;
	node.gatherTimeMs = gatherTimeMs;
	node.depletedAtTime = 0;
	node.respawnTimeMs = respawnTimeMs;
	node.nodeTex = (textureFile[0] != '\0') ? iLoadImage((char*)textureFile) : 0;
}

// Creates all 4 nodes at their hard-coded HomeBaseConfig.hpp positions.
inline void initAllResourceNodes(ResourceNode nodes[4])
{
	initResourceNode(nodes[RES_STONE], RES_STONE, STONE_NODE_X, STONE_NODE_Y,
		"", STONE_GATHER_MS, STONE_RESPAWN_MS);

	initResourceNode(nodes[RES_WOOD], RES_WOOD, WOOD_NODE_X, WOOD_NODE_Y,
		"", WOOD_GATHER_MS, WOOD_RESPAWN_MS);

	initResourceNode(nodes[RES_IRON], RES_IRON, IRON_NODE_X, IRON_NODE_Y,
		"", IRON_GATHER_MS, IRON_RESPAWN_MS);

	initResourceNode(nodes[RES_WATER], RES_WATER, WATER_NODE_X, WATER_NODE_Y,
		"", WATER_GATHER_MS, WATER_RESPAWN_MS);
}

// ---------------------------------------------------------------
//  INTERACTION (left mouse click = "Gather", per the on-screen control hint)
// ---------------------------------------------------------------
// Finds the nearest available, not-already-being-gathered node within
// INTERACT_RADIUS of the player and starts its gather timer. Only one
// node gathers at a time, which keeps this simple to reason about.
inline void tryGatherNearestNode(Fighter &player, ResourceNode nodes[], int count)
{
	for (int i = 0; i < count; i++)
	{
		if (!nodes[i].available || nodes[i].isBeingGathered) continue;

		if (distanceBetween(player.x, player.y, nodes[i].x, nodes[i].y) <= INTERACT_RADIUS)
		{
			nodes[i].isBeingGathered = true;
			nodes[i].gatherStartTime = GetTickCount();
			return;
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
			node.available = false;      // depleted
			node.depletedAtTime = now;

			switch (node.type)
			{
			case RES_STONE: g_inventory.stone++; break;
			case RES_WOOD:  g_inventory.wood++;  break;
			case RES_IRON:  g_inventory.iron++;  break;
			case RES_WATER: g_inventory.water++; break;
			}
		}
	}
	else if (!node.available)
	{
		if (now - node.depletedAtTime >= node.respawnTimeMs)
			node.available = true;             // respawned, ready again
	}
}

// ---------------------------------------------------------------
//  DRAWING
// ---------------------------------------------------------------
inline void drawResourceNode(ResourceNode &node)
{
	if (!node.available) return; // depleted - nothing to show until respawn

	if (node.isBeingGathered)
	{
		unsigned long now = GetTickCount();
		float progress = (float)(now - node.gatherStartTime) / (float)node.gatherTimeMs;
		if (progress > 1.0f) progress = 1.0f;

		// simple primitive progress bar - no extra art required
		float barX = node.x - 25;
		float barY = node.y + RESOURCE_DRAW_H / 2 + 12;
		iSetColor(0.15, 0.15, 0.15);
		iFilledRectangle(barX, barY, 50, 6);
		iSetColor(0.25, 0.85, 0.35);
		iFilledRectangle(barX, barY, 50 * progress, 6);

		// the generic circular "in progress" asset
		iShowImage((int)(node.x - GATHER_RING_SIZE / 2), (int)(node.y - GATHER_RING_SIZE / 2),
			GATHER_RING_SIZE, GATHER_RING_SIZE, g_gatherRingTex);
	}
}

// ---------------------------------------------------------------
//  GETTERS (for HUD text)
// ---------------------------------------------------------------
inline int getStoneCount() { return g_inventory.stone; }
inline int getWoodCount()  { return g_inventory.wood; }
inline int getIronCount()  { return g_inventory.iron; }
inline int getWaterCount() { return g_inventory.water; }
