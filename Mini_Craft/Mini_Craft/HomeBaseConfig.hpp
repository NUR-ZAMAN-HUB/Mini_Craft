// =====================================================================
//  HomeBaseConfig.hpp
// =====================================================================
//  Every tunable number used by the Home Base scene (fighters +
//  resource gathering) lives here, in ONE place, so anyone on the
//  team can rebalance the game without digging through the .hpp
//  files that contain actual logic.
//
//  This file has NO logic in it - only #defines. Safe to include
//  from anywhere.
//
//  NOTE: this used to be called "GameConfig.hpp" / SCREEN_WIDTH /
//  SCREEN_HEIGHT back when Home Base was its own standalone project.
//  Now that it lives inside the bigger Mini Craft project (which
//  already opens an 800x600 window in iMain.cpp's main()), the
//  screen-size constants are renamed to HOME_AREA_W / HOME_AREA_H
//  and matched to that same 800x600 so nothing draws off-screen.
// =====================================================================
#pragma once

// ---------------------------------------------------------------
//  HOME BASE PLAY AREA (matches the 800x600 window opened in main())
// ---------------------------------------------------------------
#define HOME_AREA_W 800
#define HOME_AREA_H 600

// ---------------------------------------------------------------
//  CONTROLS (Home Base scene only)
// ---------------------------------------------------------------
//  Move            : W A S D  or  Arrow Keys
//  Attack          : Right-Click  OR  Spacebar
//  Guardian Shield : E
//  Ranger Dash     : Shift + D   (capital 'D' - lowercase 'd' already
//                     means "move right", GLUT reports them as
//                     different key codes so there's no clash)
//  Gather resource : Left-Click, while standing near a node
//  (Fighter is fixed - whichever hero was picked on CHARACTER_SELECT)
//  Back to Menu    : M
// ---------------------------------------------------------------

// ---------------------------------------------------------------
//  FIGHTER DRAW SIZE (on-screen size, not the raw PNG size)
// ---------------------------------------------------------------
#define FIGHTER_DRAW_W 56
#define FIGHTER_DRAW_H 61

// ---------------------------------------------------------------
//  BASE STATS PER FIGHTER
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
//  ANIMATION FRAME TIMING (ms between frames)
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
#define SHIELD_ACTIVE_MS   4000   // ~4 seconds of damage block
#define SHIELD_COOLDOWN_MS 15000  // 15 seconds before it can be used again
#define SHIELD_RING_SIZE   140    // on-screen size of the shield overlay

// ---------------------------------------------------------------
//  RANGER DASH ABILITY
// ---------------------------------------------------------------
#define DASH_COOLDOWN_MS  600   // near-zero cooldown -> can be spammed
#define DASH_DISTANCE     110.0f
#define DASH_ANIM_MS      250   // how long the 3-frame dash animation plays
#define DASH_TRAIL_OFFSET  55   // how far behind the fighter the trail sits

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

// Hard-coded node positions (placeholder until a real tilemap arrives)
// Re-scaled to fit the project's actual 800-wide window (Home Base used
// to run in its own 960-wide window before it was merged into this project).
#define STONE_NODE_X  130.0f
#define STONE_NODE_Y  480.0f

#define WOOD_NODE_X   320.0f
#define WOOD_NODE_Y   500.0f

#define IRON_NODE_X   500.0f
#define IRON_NODE_Y   480.0f

#define WATER_NODE_X  690.0f
#define WATER_NODE_Y  500.0f

// ---------------------------------------------------------------
//  NIGHT TIMER (Home Base only)
//  Counts down from the moment Home Base is entered (see
//  HomeBase_StartNightTimer() in HomeBase.hpp) - once it hits
//  0:00, IsNightTime() flips true for whatever "night" gameplay
//  gets hooked onto it later.
// ---------------------------------------------------------------
#define NIGHT_TIMER_DURATION_MS (1 * 60 * 1000)   // 1 minutes
