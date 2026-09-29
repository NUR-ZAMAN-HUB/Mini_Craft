# Mini Craft (C2)

## Game Description

**Mini Craft (C2)** is a 2D C++ game project built using the **iGraphics/GLUT** framework in C++. The project features a multi-state game flow, including a story-driven rescue mission, a persistent Home Base hub for resource gathering and crafting, and wave-based combat encounters.

## Features
- Three playable character classes (Guardian, Ranger, Alchemist) with unique abilities like shields and dashes.
- Interactive Home Base hub featuring combat, resource gathering, and item crafting.
- Resource gathering system (Stone, Wood, Iron, Water) with individual gather times and respawn timers.
- Crafting system for potions (Movement, Power, Reinforcement, Health), Damage Scrolls, and Armor (Iron, Rusty, Steel).
- Day/Night cycle with automatic zombie defense events triggered at night.
- Wave-based enemy battles and Boss fights (e.g., Dragon with scratch attacks and fire-breath moves).
- Automatic save/load system storing progress, resources, coins, and crafted inventory in `savegame.txt`.
- Full mouse and keyboard controls for movement, interaction, combat, and menu navigation.

## Project Details
IDE: Visual Studio 2013

Language: C, C++

Platform : Windows PC

Genre : 2D action adventure / survival sandbox

## How to Run the Project

Make sure you have the following installed:
- **Visual Studio 2013**
- **iGraphics / GLUT Library** (vendored inside the `Mini_Craft/` folder)

Open the project in Visual Studio 2013
- Open Visual Studio 2013.
- Go to File → Open → Project/Solution.
- Locate and select `Mini_Craft/Mini_Craft.vcxproj` (or `Mini_Craft.sln`) from the cloned repository.
- Click Build → Build Solution
- Run the program by clicking Debug → Start Without Debugging

## How to Play

### **Controls**
| Player / Action | Move Left / Up | Move Right / Down | Jump / Ability | Attack / Interacts | Block / Dash | Extra Action |
|-------------|----------|-----------|-----------|-------|------|-------|
| **Player (Keyboard)** | `A` / `W` | `D` / `S` | `E` (Guardian Shield) | `Space` | `Shift + D` (Ranger Dash) | `M` (Menu) / `Esc` (Controls) |
| **Player (Mouse/Arrow)** | `←` (Left Arrow) | `→` (Right Arrow) | `↑` / `↓` Arrows | Right-Click (Attack) | Left-Click (Gather Resource) | — |

### **Game Rules**

- **Progression**: Flow progresses from MENU → CHARACTER_SELECT → LOADING → GAMEPLAY ("Save the Witch") → HOMEBASE <-> BATTLE.
- **Resource Gathering**: Stand near resource nodes and left-click to harvest Stone, Wood, Iron, or Water.
- **Crafting & Buffs**: Use gathered items in the crafting book to make Potions and Armor for timed buffs, healing, or permanent stat boosts.
- **Day/Night & Bosses**: Nightfall triggers automatic base defense. Boss fights trigger automatically every 5th level or on-demand via the Boss button after clearing waves.
- **Defeat & Game Over**: If hero HP hits 0 in battle, the save file resets and the player is returned to the Main Menu.

## Project Contributors

1. MARIA AKTER (00725105101146)
2. MD NUR ZAMAN LAM (00725105101155)
3. ADRITA TASNEEM RAYA (00725105101161)

## Screenshots

### **Menu**
*(Not provided)*

### **Character**
*(Not provided)*

## Youtube Link
[CSE Project: Mini Craft (C2) Demo](https://www.youtube.com/)

## Project Report
[Project Report: Mini Craft (C2)](https://drive.google.com/)
