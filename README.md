# Mini Craft (C2)

A 2D C++ game built with the **iGraphics/GLUT** framework in **Visual Studio 2013**.

## Building

Open `Mini_Craft/Mini_Craft.vcxproj` (or `Mini_Craft.sln`) in Visual Studio 2013 and build/run.
No external dependencies beyond what's already vendored in `Mini_Craft/` (`iGraphics.h`,
`glut.h`/`glut32.lib`, `OPENGL32.LIB`, `Glaux.lib`, `stb_image.h`, etc.).

## Game flow

```
MENU -> CHARACTER_SELECT -> LOADING -> GAMEPLAY -> (portal) -> HOMEBASE <-> BATTLE
                                            |
                                        CUTSCENE
```

- **MENU**: Play, Level Select, Settings. Play resumes straight into Home Base if a save
  exists (`g_hasSaveData`), otherwise starts a fresh Character Select run.
- **CHARACTER_SELECT / LOADING**: pick a fighter card (Guardian / Ranger / Alchemist),
  Confirm transitions through a loading screen.
- **GAMEPLAY**: the "Save the Witch" map — a rescue objective with cutscene dialogue,
  ending at a portal that sends the player to Home Base.
- **HOMEBASE**: the persistent hub — combat, gathering, crafting, and the day/night cycle.
- **BATTLE**: entered from Home Base's Attack button (or forced via the Boss button).

## Home Base

Reached the first time by walking through the GAMEPLAY portal; every later Play resumes
here directly.

**Controls**

| Action | Key |
|---|---|
| Move | `WASD` / arrow keys |
| Attack | `Space` or right-click |
| Guardian shield | `E` (4s active, 15s cooldown) |
| Ranger dash | `Shift + D` |
| Gather (Stone/Wood/Iron/Water) | left-click a resource node while standing close to it |
| Return to main menu | `M` |
| Toggle controls panel | `Esc` |

**Systems**
- **Resource gathering** (`GatherSystem.hpp`): 4 node types, each with its own gather time
  and respawn delay (e.g. Wood ~1.2s gather / 4s respawn, Iron ~3.5s gather / 7s respawn).
  A HUD shows current Stone/Iron/Wood counts.
- **Crafting** (`CraftingEngine.hpp`): the inventory icon opens a crafting book of potions
  (Movement, Power, Reinforcement, Health) and a Damage Scroll; only ingredients you
  actually hold are listed, and a Craft button spends them via `canCraft()`/`craftItem()`.
- **Armor** (`ArmorEngine.hpp`): a second panel, same UI pattern, for Iron / Rusty / Steel
  Armor.
- **Buffs** (`PlayerBuffs.hpp`): what each crafted item does when consumed — the movement,
  power, and reinforcement potions are timed buffs, the health potion heals over time, and
  the damage scroll is a permanent stat boost.
- **Day/night cycle**: a timer counts down to night; when it hits zero a "zombie defense"
  mini-game/battle triggers at the base.
- **Boss fights**: every 5th level auto-triggers a boss fight (`Arena_1.png`). After winning
  one full Skeleton wave-round, a Boss button also appears next to Attack, letting the
  player force a boss fight on demand (`Arena_2.png`). The current boss is a Dragon (walk
  + scratch-attack sprites, with a fire-breath super move every ~10s).
- **Save/load** (`SaveGame.hpp`): plain-text `savegame.txt` stores character, active
  fighter, level, difficulty, coins, the 4 gathered-resource counts, and every crafted-item
  quantity. Autosaves on reaching Home Base, on leaving to the menu, and every ~3s while
  in Home Base. Hero health/position are *not* saved — Home Base always resets those to a
  fresh spawn. The Reset Level button wipes the save and every related global back to a
  brand-new game.

## Battle

A wave fight between the player's active Fighter and a growing row of enemies
(`Enemy.hpp`): each battle starts at 1 enemy and adds one more per cleared wave, up to 6.
Boss battles are always a single solo enemy with no wave scaling. On defeat (HP reaches 0)
the whole save resets and the player is sent back to `MENU`.

## Project history

This build merges two originally-separate projects: the menu + "Save the Witch" map, and
a Home Base combat/resource-gathering sandbox that used to have its own `main()`. Since a
program can only have one `main()`/`iDraw()`/`iMouse()`/`fixedUpdate()`, the Home Base code
was rewritten as plain functions (`HomeBase_Init/Draw/FixedUpdate/OnMouseDown`) that the
shared `iMain.cpp` calls into whenever `currentState == GameState::HOMEBASE` — and the
same pattern (`Battle_Init/Draw/FixedUpdate/OnMouseDown`) was later used to add the
`BATTLE` state on top.

## Notes for future work

- iGraphics has no `iKeyboard` callback — key state is polled with `isKeyPressed()` /
  `isSpecialKeyPressed()` inside `fixedUpdate()`.
- `iLoadImage()` expects a non-`const char*`; wrap `std::string::c_str()` calls in a
  `const_cast` where needed.
- New art assets (button icons, arena backgrounds, enemy sprites) go in `Images/` and are
  added by hand after code wiring is in place.
