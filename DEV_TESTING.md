# ArPen test/dev range

This branch adds a local **DayZDiag offline** penetration test range. Multiplayer clients and dedicated servers cannot activate it. `main` remains the release branch.

## Start

1. Build this branch as the ArPen mod, including both `Scripts` and `GUI`. Keep the existing `ArPen` PBO prefix so layout paths resolve. Load this build instead of the Workshop/release copy.
2. Copy `missions/ArPenTest.ChernarusPlus` into your DayZ `missions` directory. Confirm the copied folder contains `init.c` and `ArPenTest.enable`. The mission entry script is `init.c`; the marker enables the controls.
3. Launch DayZDiag with your built mod and `-mission=missions/ArPenTest.ChernarusPlus` (plus your usual local mod/profile arguments).
4. The supplied mission creates the tester with a backpack, M4A1 and ammunition. Load a magazine, press **F5**, then spawn a selected helmet/vest, all helmets/vests, or two unarmored controls.
5. Close F5 to shoot. **F6** creates the damage display on first press and hides/shows it afterward. Move to open ground before spawning: targets are placed eight metres forward at terrain height, not checked against buildings or obstacles.

For an existing **working** offline mission, copy only `ArPenTest.enable` into its mission directory. Its existing player initialization is retained. A marker alone cannot supply a missing mission script or player setup.

### Loading stays on the splash screen

If the RPT reports `Mission script has no main function, player connect will stay disabled!`, confirm `init.c` is directly inside the folder selected by `-mission`. It must contain `void main()` and `CreateCustomMission`. The engine's `Creating Mission: ...mission.c` message does not mean the entry script should be renamed to mission.c. The previous instructions to rename init.c were incorrect.

For this range, install `init.c` and `ArPenTest.enable` in `DayZ/missions/ArPenTest.ChernarusPlus`. If you followed the earlier rename instruction, rename that supplied mission.c back to init.c (ensure it is not init.c.txt). Rebuilding the mod does not copy this separate mission folder. Keep the same mission launch argument and your generated profile JSON files.

Reference: [Bohemia's Modding Basics — offline mission setup](https://community.bistudio.com/wiki/DayZ:Modding_Basics) specifies init.c as the entry point.

The `PluginConfigDebugProfile is not Registred` diagnostic comes from DayZ's plugin manager, before ArPen mission `OnInit`. Keep it separate from the missing-main failure when reading logs; changing armor profiles does not register that engine plugin.

## Controls and readings

- Hard armor and helmet dropdown includes compatible installed armor classes discovered by the existing ArPen classifier.
- Six kits: M4A1, AKM, FAL, Glock19, Deagle and MKII. Each supplies a chambered weapon, three full magazines and loose ammunition. Inventory overflow appears at the tester's feet; magazines still need to be inserted.
- Up to 40 spawned targets per session. Remove targets/start fresh deletes only range-created NPCs, including corpses. Spawn again for pristine armor and injuries.
- Latest hit: ammo, hit zone, component index, penetration/stop, impact/exit velocity, depth ratio, effective threat, actual health/blood/shock and hit-zone changes, armor health, before/after bleed count.
- Live target: alive/unconscious state, active bleeds, leg fracture enum, and color-coded body-zone health bars. Fracture states: 0 = no fracture, 1 = broken, 2 = splinted.
- “Bone” means DayZ limb/foot damage-zone health and fracture state; animation bones do not have independent HP. The component index is reported without pretending it is an animation-bone ID.

## Interpretation

For custom-handled enrolled ammunition, the original health formula still defines the local HP damage. Torso hits additionally transfer that final amount 1:1 to global health and shock. Head/Brain firearm hits transfer it at 2x to global health and 3x to shock. These shock transfers replace the previous shock result for those zones, rather than adding to it. Other zones retain their current local-health and shock behavior; blood handling is unchanged. No scripted fatal-zone kill is added.

| Local health damage | Zone | Global health loss | Global shock loss |
| --- | --- | --- | --- |
| 20 | Torso | 20 | 20 |
| 20 | Head / Brain (firearm) | 40 | 60 |

Transfers use the calculated local damage, before capping it to remaining zone HP. They apply to custom stopped and penetrating hits. Global values are written once per packet from its starting pools, and health is never restored after native zone death. Already-ruined armor remains on the native path.

Unarmored hits retain the original custom calculation and local-only health application. Armor already ruined before impact uses the native damage event, including native global damage. A hit that starts against intact armor and ruins it still completes the custom calculation once; subsequent hits use native handling. Unsupported/soft armor, unenrolled ammunition and non-firearm events still use native handling. Custom penetrations on surviving targets request a bleeding source at the original hit component, independently of the vanilla armor-filtered blood-damage amount. This bypasses the native blood-damage chance roll for custom penetrations; the native manager still rejects invalid locations and duplicate/disallowed sources. Stopped hits do not request bullet wounds. Native fallback hits retain native bleeding behavior.

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

- Fire repeated 9mm shots at a plate carrier until it is ruined and continue firing. While the plate stops the bullet, the original formula's health amount must be applied locally, with the requested torso/head health and shock transfers. Once the plate is already ruined before impact, expect native damage handling.
- Repeat unarmored, helmet, torso and limb tests. Confirm local HP, bleeds and shock in the HUD. Native death/injury consequences of zone damage remain possible.
- Fire a short burst: each packet uses current local HP, with no stale health restoration.
- Verify soft/unsupported armor, unenrolled ammunition and melee/explosion retain native handling.

- Verify a 20-point torso result removes 20 global health and 20 shock; a 20-point Head/Brain firearm result removes 40 global health and 60 shock (subject to remaining pools). Confirm shock is replaced, not added to the older shock calculation.

## Penetration bleed regression

- Penetrate intact armor whose vanilla Projectile Blood multiplier is zero: a surviving target should gain a wound at a valid, previously unbled hit selection.
- A stopped hit must not add a wound. Already-ruined armor retains the native event and its bleeding rules.
- Repeated hits to an already bleeding selection need not increase the count; the manager refuses duplicates. Invalid component mappings and lethal hits also need not add a source.
- Global health/shock transfer and immediate blood damage are unchanged. Bleeding subsequently drains blood through the native manager.

Compile and verify these cases in DayZ; no runtime verification was available here.
