# ArPen test/dev range

This branch adds a local **DayZDiag offline** penetration test range. Multiplayer clients and dedicated servers cannot activate it. `main` remains the release branch.

## Start

1. Build this branch as the ArPen mod, including both `Scripts` and `GUI`. Keep the existing `ArPen` PBO prefix so layout paths resolve. Load this build instead of the Workshop/release copy.
2. Copy `missions/ArPenTest.ChernarusPlus` into your DayZ `missions` directory. Its `ArPenTest.enable` marker enables the controls.
3. Launch DayZDiag with your built mod and `-mission=missions/ArPenTest.ChernarusPlus` (plus your usual local mod/profile arguments).
4. The supplied mission creates the tester with a backpack, M4A1 and ammunition. Load a magazine, press **F5**, then spawn a selected helmet/vest, all helmets/vests, or two unarmored controls.
5. Close F5 to shoot. **F6** hides/shows the persistent damage display. Move to open ground before spawning: targets are placed eight metres forward at terrain height, not checked against buildings or obstacles.

For an existing offline mission, copy only `ArPenTest.enable` into its mission directory. Its existing player initialization is retained.

## Controls and readings

- Hard armor and helmet dropdown includes compatible installed armor classes discovered by the existing ArPen classifier.
- Six kits: M4A1, AKM, FAL, Glock19, Deagle and MKII. Each supplies a chambered weapon, three full magazines and loose ammunition. Inventory overflow appears at the tester's feet; magazines still need to be inserted.
- Up to 40 spawned targets per session. Remove targets/start fresh deletes only range-created NPCs, including corpses. Spawn again for pristine armor and injuries.
- Latest hit: ammo, hit zone, component index, penetration/stop, impact/exit velocity, depth ratio, effective threat, actual health/blood/shock and hit-zone changes, armor health, before/after bleed count.
- Live target: alive/unconscious state, active bleeds, leg fracture enum, and color-coded body-zone health bars. Fracture states: 0 = no fracture, 1 = broken, 2 = splinted.
- “Bone” means DayZ limb/foot damage-zone health and fracture state; animation bones do not have independent HP. The component index is reported without pretending it is an animation-bone ID.

## Interpretation

For enrolled ammunition, the original health-damage formula applies unchanged only to the struck zone. There is no explicit global-health write, transfer coefficient scaling, or scripted fatal-zone kill. DayZ remains responsible for native consequences of damaged zones. Blood and shock use the original custom amounts.

Unarmored and already-ruined armor hits also retain the original custom calculation and local-only health application; they do not switch to native global bullet damage after armor breaks. Unsupported/soft armor, unenrolled ammunition and non-firearm events still use native handling. Penetrating custom hits retain bleeding eligibility checks; stopped hits do not create bullet wounds.

Each queued hard-armor hit contains damage amounts and samples current pools when applied; packets queued after the target dies are discarded. Armor still takes damage once in the original calculation. The custom path does not replay EEHitBy: third-party hooks that rely on that native event are not restored by this fix.

Custom-hit global deltas are captured immediately around each queued application. Armor deltas cover the original calculation and queued report. Native fallback deltas are sampled on the next script queue update and can include overlapping impacts; use single shots for attribution. The HUD retains the latest hit, not a historical log. No ArPen RPT prints or broadcast notifications are added.

## Validation checklist (requires DayZ)

The source was checked against Bohemia's published DayZ script APIs; no DayZ compiler/runtime is available in the editing environment. Compile in Workbench and run these checks before relying on the build:

- Start the bundled offline mission; F5 opens/closes without leaving cursor/game focus stuck; F6 toggles the HUD.
- Request each kit with free and full inventory; load/fire each weapon.
- Spawn selected vest and helmet, both full rows and unarmored controls; remove targets and repeat.
- Fire single head/torso/leg shots: confirm zone, health loss, fracture state and bleeding readings. Compare custom versus native fallback ammunition/armor.
- Repeat armor hits through ruined state; verify penetration and actual global loss without doubled damage. Fire a short burst to check queued application.
- Start without the marker and on multiplayer: controls and spawning remain disabled.

API reference: https://github.com/BohemiaInteractive/DayZ-Script-Diff

## Zone-damage regression checks

DayZ compilation and runtime checks are pending. Source checks cannot establish engine-side consequences of local health setters.

- Fire repeated 9mm shots at a plate carrier until it is ruined and continue firing. The original formula's health amount must remain a local-zone subtraction before and after ruin; no custom global health subtraction is made.
- Repeat unarmored, helmet, torso and limb tests. Confirm local HP, bleeds and shock in the HUD. Native death/injury consequences of zone damage remain possible.
- Fire a short burst: each packet uses current local HP, with no stale health restoration.
- Verify soft/unsupported armor, unenrolled ammunition and melee/explosion retain native handling.
