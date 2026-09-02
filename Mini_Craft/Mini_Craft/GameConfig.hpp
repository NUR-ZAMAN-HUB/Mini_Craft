// =====================================================================
//  GameConfig.hpp
// =====================================================================
//  Every "magic number" used by the player / resource systems lives
//  here, in ONE place, so anyone on the team can tune the game just
//  by editing this file (no need to dig through Player.hpp or
//  ResourceSystem.hpp).
//
//  This file has NO logic in it - only #defines. It is safe to
//  #include from anywhere.
// =====================================================================
#pragma once

// ---------------------------------------------------------------
//  SCREEN
// ---------------------------------------------------------------
#define SCREEN_WIDTH  960
#define SCREEN_HEIGHT 600

// ---------------------------------------------------------------
//  CONTROLS  (documented here so nobody has to read the .cpp to
//  find out what key does what)
// ---------------------------------------------------------------
//  Move          : W A S D  or  Arrow Keys
//  Attack        : Right-Click  OR  Spacebar
//  Guardian Shield : E
//  Ranger Dash     : Shift + D   (capital 'D')
//                    -> plain lowercase 'd' is already used for
//                       "move right", so Dash uses the SHIFTED key.
//                       GLUT reports 'd' and 'D' as different key
//                       codes, so this does not clash with movement.
//  Gather resource : Left-Click, while standing near a node
//  Switch character (TESTING ONLY, remove once a real character-select
//                    menu exists) : 1 = Guardian, 2 = Ranger, 3 = Alchemist
// ---------------------------------------------------------------

// ---------------------------------------------------------------
//  CHARACTER DRAW SIZE  (on-screen size, not the raw PNG size)
//  Standardized across all 3 characters: heroWidth x heroHeight
// ---------------------------------------------------------------
#define CHAR_DRAW_W 75   // heroWidth
#define CHAR_DRAW_H 95   // heroHeight

// ---------------------------------------------------------------
//  HARD-CODED START POSITIONS
//  (Change these to move a character's spawn point. A tilemap /
//   spawn system from a teammate can later overwrite these at
//   runtime - these are just the checkpoint-1 defaults.)
// ---------------------------------------------------------------
#define GUARDIAN_START_X  220.0f
#define GUARDIAN_START_Y  150.0f

#define RANGER_START_X    420.0f
#define RANGER_START_Y    150.0f

#define ALCHEMIST_START_X 620.0f
#define ALCHEMIST_START_Y 150.0f

// ---------------------------------------------------------------
//  BASE STATS PER CHARACTER
// ---------------------------------------------------------------
// Guardian: tanky, slow, short range
#define GUARDIAN_MAX_HEALTH   200
#define GUARDIAN_MOVE_SPEED   2.0f     // pixels per fixedUpdate() tick
#define GUARDIAN_DAMAGE       25
#define GUARDIAN_MELEE_RANGE  55.0f    // LOW range

// Ranger: fragile, fast, long range
#define RANGER_MAX_HEALTH     90
#define RANGER_MOVE_SPEED     4.5f
#define RANGER_DAMAGE         15
#define RANGER_MELEE_RANGE    150.0f   // HIGH range

// Alchemist: middle HP/speed, long range, no ability yet
#define ALCHEMIST_MAX_HEALTH  120
#define ALCHEMIST_MOVE_SPEED  3.0f
#define ALCHEMIST_DAMAGE      18
#define ALCHEMIST_MELEE_RANGE 150.0f   // HIGH range

// ---------------------------------------------------------------
//  ANIMATION FRAME TIMING  (ms between frames - "advance every
//  N ms, not every game tick")
// ---------------------------------------------------------------
#define WALK_FRAME_INTERVAL_MS         120
#define ATTACK_FRAME_INTERVAL_MS       80
#define DASH_FRAME_INTERVAL_MS         80
#define SHIELD_RING_FRAME_INTERVAL_MS  60

// Max frames that exist for each animation type (see Images folder)
#define WALK_MAX_FRAMES    4   // Alchemist/Ranger walk = 4 frames
#define ATTACK_MAX_FRAMES  8   // Alchemist attack = 8 frames
#define DASH_FRAMES        3   // Ranger dash = 3 frames per direction
#define SHIELD_RING_FRAMES 13  // shield_around_guardian_1..13

// ---------------------------------------------------------------
//  ATTACK
// ---------------------------------------------------------------
#define ATTACK_COOLDOWN_MS 500   // stops holding Space/Right-Click from spamming

// ---------------------------------------------------------------
//  GUARDIAN SHIELD ABILITY
// ---------------------------------------------------------------
#define SHIELD_ACTIVE_MS   4000   // 4 seconds of (near) full damage block
#define SHIELD_COOLDOWN_MS 15000  // 15 seconds before it can be used again
#define SHIELD_RING_SIZE   140    // on-screen size of the shield overlay

// ---------------------------------------------------------------
//  RANGER DASH ABILITY
// ---------------------------------------------------------------
#define DASH_COOLDOWN_MS  600   // near-zero cooldown -> can be spammed
#define DASH_DISTANCE     110.0f
#define DASH_ANIM_MS      250   // how long the 3-frame dash animation plays
#define DASH_TRAIL_W      130
#define DASH_TRAIL_H       60
#define DASH_TRAIL_OFFSET  55   // how far behind the character the trail sits

// ---------------------------------------------------------------
//  RESOURCE SYSTEM
// ---------------------------------------------------------------
#define RESOURCE_DRAW_W    80
#define RESOURCE_DRAW_H    70
#define GATHER_RING_SIZE   50
#define INTERACT_RADIUS    70.0f   // how close the player must stand to gather

// Gather + respawn time per resource type (Stone/Iron slower than Wood/Water)
#define STONE_GATHER_MS   3000
#define STONE_RESPAWN_MS  6000

#define WOOD_GATHER_MS    1200
#define WOOD_RESPAWN_MS   4000

#define IRON_GATHER_MS    3500
#define IRON_RESPAWN_MS   7000

#define WATER_GATHER_MS   1500
#define WATER_RESPAWN_MS  4000

// Hard-coded node positions (placeholder until the real tilemap arrives)
#define STONE_NODE_X  150.0f
#define STONE_NODE_Y  480.0f

#define WOOD_NODE_X   380.0f
#define WOOD_NODE_Y   500.0f

#define IRON_NODE_X   600.0f
#define IRON_NODE_Y   480.0f

#define WATER_NODE_X  820.0f
#define WATER_NODE_Y  500.0f
