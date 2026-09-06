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

For both penetrating and stopped hard-armor hits, the health damage amount previously applied to global HP is now applied unchanged to the struck zone. Global health loss is that amount multiplied by the zone's configured health transfer coefficient (1 if absent; negative values clamped to 0). There is no inverse scaling of local damage. For example, a 24-point result with a transfer coefficient of 0.3 removes 24 local HP and 7.2 global HP. Blood and shock handling is unchanged by this remapping. Configured fatal-zone thresholds, bleeding eligibility for penetrations, leg injury checks and immediate shock checks run after the queued application. Stopped hits do not create bullet wounds.

Unarmored hits, armor already ruined before impact, non-firearm damage and unsupported/soft armor use the native damage event. In particular, normal leg/foot shots retain vanilla fracture and bleeding handling. Native fallback hits are labeled separately because the harness has no ArPen penetration result for them.

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

Compile the dev build first. Engine integration checks below are pending; source checks cannot establish engine callback ordering or fatal-zone behavior.

| Scenario | Expected result |
| --- | --- |
| Unarmored leg/foot hit | Native local HP loss, global damage, wounds and fracture behavior; no queued ArPen duplicate |
| Fresh helmet, stopped hit | Previous blunt health result applied directly to local HP; coefficient-scaled global loss; no new bullet wound |
| Fresh helmet, penetration | Separate local and global HP losses; fatal configured zone kills even if global HP would remain |
| Vest penetration | Struck zone loses the previous health result; global loss equals local damage times its transfer coefficient |
| Hit that ruins armor | One armor calculation and one wearer packet; normalization uses the pre-hit calculation |
| Follow-up hit on ruined armor | Native event only |
| Rapid hits | Each packet subtracts from the current pools; no stale snapshots or health restoration |
| Penetration with blood damage | Original ammo's bleeding probability/threshold and component are used, once |
| Soft armor or melee/explosion | Native behavior retained |

Use single-shot HUD readings to compare local-zone HP loss against global loss. They are not expected to be identical on every body part. Also verify shock knockout/recovery, fatal head hits and repeated fractures in DayZ before merging this branch.
