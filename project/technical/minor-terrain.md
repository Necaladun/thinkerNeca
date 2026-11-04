# Independent minor terrain — current implementation

**Reference document.** Work items: [queue](../WORK-QUEUE.md); runtime evidence: [testing](../TESTING.md).

**Status:** Implemented on draft PR #4; initial Windows Develop gameplay checks have passed, but full regression coverage and Release parity are outstanding. This document describes the implemented rules, **not** proposed forest/cap/deposit balance changes.

## Data model

Independent minor terrain is stored per map tile as `MinorTerrainTile { uint8_t type, uint8_t code }` in `src/savegame.cpp` / `src/savegame.h`. It does **not** consume an original `LM_*` landmark bit or create a named major landmark. The definitions registry supplies name, nutrient/mineral/energy bonus, fallback sprite slot and generation phase.

`THMIN001` version 2 writes two bytes per tile after the native map data. The current reader accepts native saves with no extension; version 1 extensions are incompatible. **Forward development assumes new games**; compatibility with older mod saves or custom scenarios is not a design requirement, although some legacy support code remains.

| Type | Current bonus | Phase | Distribution |
| --- | --- | --- | --- |
| Jungle | +1 nutrient | Early | Climate-/moisture-sensitive patches, generally 8–16 tiles, occasionally larger |
| Uranium | +1 energy | Late | Dispersed 3–9 tile deposits, lowland preference |
| Ironstone | +1 mineral | Late | Dispersed 3–9 tile deposits, rolling/rocky preference |

Only one independent minor type is stored per tile. The `code` field is type-specific: Jungle uses a climate/rainfall pattern; Uranium and Ironstone currently use patch-local indices (0–8).

## Generation

For the intended independent-feature maps, use `new_world_builder=1`, `modified_landmarks=1`, `minor_jungle=1`, `minor_uranium=1` and `minor_ironstone=1`. The older `jungle=0` and `uranium=0` controls disable their **native major** placement, not these minor types.

1. The early phase generates Jungle before the native-major landmark placement loop and final climate/rainfall pass. Major landmarks can overwrite parts of those patches; the post-placement cleanup removes overlapping independent minor tiles.
2. The modified world generator then finalises climate and rockiness.
3. The late phase generates Uranium and Ironstone, avoiding existing major-landmark flags and other independent minor patches. Each geological generator targets 2–6 geographically dispersed patches, depending on map area, but placement can reject or fail targets.
4. Mirrored map modes skip independent minor generation (symmetry is not implemented).

**Configuration caveat:** `minor_generator_enabled()` currently requires `modified_landmarks=1` for Jungle and Uranium but not Ironstone. The independent features are only generated through Thinker's `new_world_builder=1` path. This coupling is pending a separate configuration review; do not silently treat all three toggles as fully independent.

The legacy `LM_JUNGLE` / `LM_URANIUM` generator paths remain available in some unmodified/editor modes. These are not the independent registry types. Original major-landmark slot 10 (former Nessus Canyon) has been retired and reserved as `LM_RESERVED_10 = 0x200`; it is not used for the new minor features.

## Resource yields and AI

The registry currently defines **unconditional +1 basic-resource bonuses** (subject to the actual resource-calculation branch and restrictions). Normal land, base squares and Forests generally preserve their minor bonuses. Some special improvement branches overwrite calculated values: notably, a Thermal Borehole suppresses Jungle's nutrient and Ironstone's mineral bonus, whereas Uranium's energy bonus can survive. Fungus and other special cases require individual checks. Ordinary resource restrictions can cap the realised bonus.

Thinker's worker selection, crawler logic and former scoring consume modified actual tile yields in several paths. Settlement scoring also explicitly recognises independent terrain, but uses coarse fixed bonuses. The projected `item_yield()` calculation is not fully identical to actual `mod_crop_yield()`, `mod_mine_yield()` and `mod_energy_yield()` for certain future improvements (especially Ironstone/Jungle plus a Thermal Borehole). These discrepancies should be fixed in the later AI/terraform pass, once extraction rules are finalised.

**Balance status:** The distributed test `thinker.ini` now sets `tile_output_limit=3,3,3` using Thinker's existing configurable pre-tech resource-cap mechanic (up from 2,2,2); this is an **experimental configuration**, not a new source-level yield rule. The proposed 1/1/1 Forests, improvement-dependent geological extraction, and general terraform-AI rewrite are **not** implemented.

## Testing and limitations

**Observed in game by the user:** all three minor types generate and are visible; Jungle +1 nutrient and Ironstone +1 mineral were observed; native Jungle and Uranium graphics render; corrected Ironstone graphics work at ordinary and far zoom; all three feature names appear in the native tile Inspector in the tested Develop build.

**Automated:** the Windows Develop CI builds the DLL/launcher, checks static source invariants, generates/tests the indexed PCX fallback and parses synthetic `THMIN001` v2 saves. These are **not** executable world-generation or full save/reload tests.

**Still to verify:** multiple seeds and map sizes; fragment/spacing statistics; complete fog, focus and partial redraw behaviour; resource and improvement combinations; native Inspector on standalone/composite features; real in-game save/reload; mirrored maps (currently excluded); Release and PRACX/reduced-mode behaviour.

See [rendering.md](rendering.md) for the rendering architecture, deployment and diagnostics.
