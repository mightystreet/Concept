# Concept — bodycam FPS prototype: build plan

Updated 2026-11-26 to fold in the "player stats, items and interactions" addition.
Rules from the brief still apply:
- C++ only, with no Blueprint graphs.
- MCP is used only for editor work.
- Header changes go into one batch per milestone, with a full build while the editor is closed. Ask before closing the editor.
- Zero errors, and no warnings in our own code.
- Never claim a feature works unless it was run.
- Stop and report at every milestone.

## Where things stand
- **Built:** all of the Part 1 C++ (movement, the first-person procedural motion, weapons, projectiles, AI, debug keys). It compiles.
- **Content:**
  - Input assets: 16 input actions and IMC_Default.
  - Five weapon data assets.
  - `L_ProtoArena` (80×40 m) with a nav mesh.
  - `L_Walkthrough`: one room per enemy type, described below.
- **Missing:**
  - `M_BodycamLens`, the lens post-process material.
  - The tuning panel, unit tests, selftest and a central config. The addition calls these "existing rules", but none of them exist yet. They are now part of milestone 2.

## Cross-cutting infrastructure (built first in milestone 2)
- **`UProtoTuning` config class.** A `UDeveloperSettings` subclass, with values stored in `Config/DefaultGame.ini`.
  - Every survival, stamina and item number lives here, so no gameplay number is hard-coded.
  - It also holds the survival time multiplier.
- **Item and loot data.**
  - `UProtoItemData` data assets, named `DA_Item_*`. Each holds the item's id, name, colour, stack size, use time, effects and its quick-use slot.
  - `UProtoLootTable` data asset: weighted entries with a min and max count per entry.
- **Tuning panel.** A Slate panel opened from the debug menu.
  - Sliders for the tuning config, including the survival time multiplier.
  - Buttons: give all weapons and ammo, set any stat, and spawn any item at the player's feet.
  - All edits apply live.
- **Tests.** Automation tests in `Proto/Tests`, run with `-ExecCmds="Automation RunTests Proto"`.
  - Unit tests cover the pure logic: stat drain and regen, the stamina lockout, low-stat effects, inventory stacking, loot rolls, ammo reserves and fall damage.
  - A selftest console command (`ProtoSelfTest`) spawns things in the world and checks them: item pickup, use, use cancel, door blocking and crate opening.
- **Data validation.** `IsDataValid` on the weapon, item and loot assets. The bar is zero errors from `DataValidation.ValidateAssets`.

## Milestones
2. **Arena, movement, collision, plus stamina and the HUD bars.**
   - `UProtoStatsComponent` holds Health, Stamina, Hunger and Hydration, each 0–100.
     - Health stays in `UProtoHealthComponent`, and the stats component reads it.
   - Stamina:
     - Sprinting costs 12 per second and a jump costs 10.
     - It regenerates at 18 per second after 1.2 s of rest.
     - Below 20, weapon sway is heavy.
     - At 0, sprint is locked until stamina is back to 25.
   - HUD: four thin bars plus the ammo count. H toggles the HUD. (The brief's "no HUD by default" is replaced by these bars.)
   - Tuning config, tuning panel shell, test harness and validation, as listed above.
3. **Lens post-process, procedural camera and weapon motion, free-aim.**
   - Create M_BodycamLens.
   - Tune the first-person spring motion.
4. **Shooting: five weapons, finite ammo, projectiles, rocket, targets.**
   - Ammo reserve pools are kept per ammo type, with five types. Reloading draws from the reserve.
   - The player starts with only the pistol and 30 spare rounds. The other four weapons become pickups.
5. **Health, fall damage, hunger, hydration, low-stat effects.**
   - Health is lost from bullets, blasts, falls over 4 m and starving.
   - It comes back only from a medkit or bandage.
   - Below 30 health: the screen desaturates, a vignette pulses, and movement is 10% slower.
   - Hunger empties in 40 minutes, 1.5× faster while sprinting.
     - Below 25, stamina regen drops by 40%.
     - At 0, the player loses 1 health every 6 s.
   - Hydration empties in 25 minutes, 2× faster while sprinting.
     - Below 25, max stamina is capped at 60.
     - At 0, the player loses 1 health every 3 s.
   - The survival time multiplier is on the tuning panel, so a full drain can be tested in about a minute.
6. **Items, interaction prompt, inventory, crates, doors, pickups.**
   - F interacts, while E stays as lean. The interact trace reaches 2.2 m, highlights the target and shows a prompt such as "F: Pick up Water bottle".
   - `AProtoPickup` items are colour-coded boxes:

     | Item | Effect | Use time |
     |---|---|---|
     | Canned food | +40 hunger | 4 s |
     | Bread | +20 hunger | 2 s |
     | Water bottle | +45 hydration | 3 s |
     | Energy drink | +15 hydration, and 2× stamina regen for 60 s | 2 s |
     | Medkit | +50 health over 4 s | 3 s |
     | Bandage | +15 health | 2 s |
     | Ammo boxes and weapons | Picked up instantly | — |

   - `UProtoInventoryComponent`:
     - 20 slots, with stacking.
     - Tab opens a Slate panel with Use and Drop buttons. The game keeps running while it is open.
     - Quick-use keys: 6 medkit, 7 bandage, 8 eat, 9 drink.
   - Using an item lowers the weapon and shows a progress bar. Firing or sprinting cancels the use without consuming the item.
   - `AProtoLootCrate` opens with F and spawns items rolled from its loot table.
   - `AProtoDoor` opens and closes with F.
     - A closed door blocks movement, bullets and line of sight.
     - Doors use a nav link or modifier so enemies open them on their path.
   - Arena additions: a hut with a door, two loot crates, one of each weapon, and scattered food, drink, medical and ammo pickups.
7. **Enemy AI with the rifle, plus loot drops.**
   - On death, an enemy drops its weapon, some ammo, and sometimes a random item from a loot table.
   - Enemies have no hunger, hydration or stamina.
8. **All five enemy weapon behaviours** (already coded, so this milestone tunes and verifies them; L_Walkthrough is the test bed).
9. **Tuning pass and README.**

## New input actions and keys
| Action | Key |
|---|---|
| IA_Interact | F |
| IA_Inventory | Tab |
| IA_ToggleHUD | H |
| IA_QuickMedkit, IA_QuickBandage, IA_QuickEat, IA_QuickDrink | 6, 7, 8, 9 |

## L_Walkthrough (`/Game/Prototype/Maps/L_Walkthrough`)
- A 320 m corridor, 24 m wide, made of six rooms.
- Doorways alternate sides, and each has a baffle wall behind it, so you meet one enemy at a time.
- Each room has a sign naming the enemy and its behaviour.

| Room | Length | Enemy |
|---|---|---|
| Start | 20 m | None. A practice spawn pad for F1–F5. |
| 1 | 30 m | Pistol |
| 2 | 50 m | Rifle |
| 3 | 40 m | Machine gun, behind a low wall |
| 4 | 60 m | RPG |
| 5 | 120 m | Sniper lane |
