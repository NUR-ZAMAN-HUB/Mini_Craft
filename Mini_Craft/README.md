# Mini Craft

Open `Mini_Craft/Mini_Craft.vcxproj` (or the `.sln` if you keep one alongside it)
in Visual Studio 2013 and build.

## What's in this build

- **Menu system**: MENU -> CHARACTER_SELECT -> LOADING -> GAMEPLAY, plus
  LEVEL_SELECT and SETTINGS showcase screens. (`Menu.h`, `Player.hpp`)
- **"Save the Witch" map**: the rescue objective, cutscene dialogue, and the
  portal that ends the level. (in `iMain.cpp`)
- **Home Base**: reached by walking through the portal after rescuing the
  witch. Pick a fighter (Guardian / Ranger / Alchemist - keys `1`/`2`/`3`),
  move with `WASD`/arrows, attack with `Space`/right-click, and gather
  Stone/Wood/Iron/Water by left-clicking near a resource node while standing
  close to it. Guardian has a shield (`E`), Ranger has a dash (`Shift+D`).
  Press `M` to return to the main menu.
  (`HomeBase.hpp`, `Fighters.hpp`, `GatherSystem.hpp`, `HomeBaseConfig.hpp`)

## How this was put together

This project merges two previously-separate builds:

1. The menu + "Save the Witch" map (`iMain.cpp`, `Menu.h`, `Player.hpp`).
2. The Home Base combat/resource-gathering sandbox, which used to be its own
   standalone project with its own `main()`. Since a project can only have
   one `main()` / `iDraw()` / `iMouse()` / `fixedUpdate()`, that code was
   rewritten as four plain functions (`HomeBase_Init/Draw/FixedUpdate/OnMouseDown`)
   that the shared `iMain.cpp` calls into whenever `currentState == GameState::HOMEBASE`.

Along the way, Home Base's old `Character` struct was renamed to `Fighter`
(and its file renamed `Player.hpp` -> `Fighters.hpp`) because this project's
menu system already has its own, unrelated `Character` struct in its own
`Player.hpp` - having two different `struct Character` definitions in the
same program is a compile error, and was very likely the main source of the
"too many errors" from earlier merge attempts.

Also fixed along the way: `iLoadImage()` takes a non-`const char*`, but the
character-select screen was passing it `std::string::c_str()` (which is
`const char*`) directly - a genuine compile error, now wrapped in a
`const_cast`.
