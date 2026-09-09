Armor Lab: https://downfall318.github.io/ArPen/

## Profiles

Initialization creates `Materials.json`, `ArmorProfiles.json` and `AmmoProfiles.json` only when each file is absent. Existing files are loaded without supplementation, migration or rewriting. An explicitly disabled JSON entry also disables its config-based enrollment. Damage multipliers and the existing torso/head transfer rates remain server tuning choices.

## Ceramic tiles

`ArPen_TiledPlateCarrierVest` is an optional body-armor item with the plate carrier's existing appearance and 16 independently damaged ceramic tiles. It is available for admin spawning or server economy enrollment; it is not automatically added to loot. The original `PlateCarrierVest` keeps its original one-plate behavior.

An armor profile's optional `Tiles` array holds each equal-area tile's maximum HP. The number of entries determines the number of tiles. For example, add this field to a ceramic armor profile for sixteen tiles, each using the original 800-HP ceramic calculation:

```json
"Tiles": [800, 800, 800, 800, 800, 800, 800, 800, 800, 800, 800, 800, 800, 800, 800, 800]
```

- Each hit uniformly selects one tile, including defeated tiles. These are separate hit locations, not layers traversed by the same bullet.
- `TileDamageMultiplier` defaults to **3.0** (3× total damage) for configured tiles; direct config uses `tileDamageMultiplier`. It multiplies tile health damage after penetration is decided, without changing maximum HP or penetration resistance for that hit. A full-severity sequence becomes 100% → 25% → defeated. Monolithic armor is unaffected.
- When a stopped hit destroys its tile, the tile is consumed but the wearer takes no health, blood, or shock damage from that hit. Penetrating hits are not exempt, and subsequent hits through that hole penetrate normally.
- Damage, effective Krupp, and stopped-hit severity use only the selected tile's remaining health and configured maximum HP. Healthy neighboring tiles keep their original resistance.
- A hit on a defeated tile penetrates without further armor resistance.
- Whole-item HP follows average normalized tile health, but becomes zero as soon as at least 25% of tiles are defeated. Sixteen tiles fail at four defeated tiles; five fail at two. The shot that defeats a tile completes using its pre-hit resistance.
- Empty or omitted `Tiles` uses the original monolithic plate. `"Tiles": [800]` with `"TileDamageMultiplier": 1.0` reproduces the original 800-HP ceramic durability calculation. Entries must be positive; unsupported material types or invalid arrays use the monolithic path.
- Repairs and external item damage are reflected in tile health. Repairs distribute restored condition proportionally across missing tile health. External damage scales tile health proportionally.

The new item supplies its default `tiles[]` config as well, so it works with an existing profile file without rewriting that file. To customize or disable it there, add an explicit `ArPen_TiledPlateCarrierVest` profile. JSON enrollment takes precedence over config enrollment. Modded armor can also supply an `ArPen` config with `tiles[]` and the usual ceramic material/armor fields.

The lab includes the tiled carrier and an editable tile-HP list. It uses a repeatable random sequence shared across velocity bins so appending a graph preserves earlier tile choices. The game samples randomly per hit. Lab stopped-hit H/S represents global damage after the existing transfers: torso transfers calculated health and shock independently at 1×/1×; head transfers local health at 2× and calculated shock at 3×. Existing local zone factors remain in the calculations; global transfer is a separate step. Blunt health and shock profile multipliers each control their own channel. Penetrating wearer damage depends on the incoming DayZ damage result and is not numerically predicted by the lab.

## Persistence and verification

All schemas and the item persistence layout remain unversioned. **This build adds tile health and the last synchronized item condition to the fixed item save layout. Existing world item persistence from earlier builds is incompatible and must be reset, or used with its matching earlier mod build.** Deleting profile JSON files does not convert saved items. No migration or backward-compatible reader is included.

Developer regressions:

```sh
node tests/lab-regression.cjs
python tests/state-regression.py
```

The first executes the actual HTML lab physics. The second adapts the production item-state methods to a C++ host with mock health, persistence, and call-queue APIs (requires `g++`). These checks do not replace DayZ compilation and in-game testing. Test shotgun pellets in one frame, repairs, a save/reload, disabled config-enrolled armor, and the tiled carrier before running a live server.
