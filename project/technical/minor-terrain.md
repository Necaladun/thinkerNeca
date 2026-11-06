# Independent minor terrain — current implementation

> **Branch scope (2026-11-07):** This reference describes experimental terrain/rendering code on [draft PR #4](https://github.com/Necaladun/thinkerNeca/pull/4), **not yet merged into `master`**. The default branch still has earlier Ironstone and map behaviour; verify implementation against the working branch.


**Reference document.** Work items: [queue](../WORK-QUEUE.md); runtime evidence: [testing](../TESTING.md).

**Status:** Implemented on draft PR #4; initial Windows Develop gameplay checks have passed, but full regression coverage and Release parity are outstanding. This document describes the implemented rules, **not** proposed forest/cap/deposit balance changes.

## Data model

Independent minor terrain is now represented per map tile as `MinorTerrainTile { std::vector<MinorTerrainEntry> features; }`, where each sorted `MinorTerrainEntry` has its own `uint8_t type` and `uint8_t code` (`src/savegame.cpp` / `src/savegame.h`). It does **not** consume an original `LM_*` landmark bit or create a named major landmark. The definitions registry supplies name, nutrient/mineral/energy bonus, fallback sprite slot and generation phase.

`THMIN001` **version 3** writes the same magic/version/map tile count, then for each tile a one-byte entry count followed by ordered `(type,code)` pairs. It validates count, strict ascending unique types and type-specific code limits before accepting a save. The old experimental v2 format is **not** readable by the new DLL; use a **fresh game**, and always verify same-build v3 save/reload. The current reader accepts native saves with no extension; version 1 extensions are incompatible. **Forward development assumes new games**; compatibility with older mod saves or custom scenarios is not a design requirement, although some legacy support code remains.

| Type | Current bonus | Phase | Distribution |
| --- | --- | --- | --- |
| Jungle | +1 nutrient | Early | Climate-/moisture-sensitive patches, generally 8–16 tiles, occasionally larger |
| Uranium | +1 energy | Late | Dispersed 3–9 tile deposits, lowland preference |
| Titanium | +1 mineral | Late | Dispersed 3–9 tile deposits; currently identical lowland preference to Uranium |

**Source implementation on PR #4 (TERR-004; Windows runtime not yet tested):** each tile may now hold several independent minor types with per-type codes. Jungle uses a climate/rainfall pattern; Uranium and Titanium use patch-local indices (0–8). The `minor_landmark_has/any_at/entries_at` queries, type-specific `minor_landmark_set/clear` and `minor_landmark_clear_all` replace the former single-valued lookup. Do not conflate *representability* with procedural overlap placement, and do not infer that every original native `LM_*` combination is mechanically/renderably valid.

## Multi-feature implementation and pending verification (TERR-004)

The source-layer storage and consumers have been refactored to a **compositional** model. Windows x86 Develop and Release CI compilation and the synthetic v3 tests passed on `8952ebe` (run 42314671873); the following remain acceptance boundaries requiring actual in-game verification:

- `minor_landmark_has(x,y,type)`, `minor_landmark_code_at(x,y,type)`, and add/remove/iterate operations are type-specific. Setting Titanium on Jungle must retain Jungle and its code. A tile may hold Jungle + Uranium + Titanium; the model must also retain native `LM_*` flags and ordinary resource bonuses unchanged.
- Each present type contributes its registered N/M/E effects **once** through the existing live yield branches and resource caps, with future special-case restrictions made explicit. A tile with Jungle + Uranium + Titanium nominally has +1N/+1M/+1E before technology/improvement/cap rules; no double-counting or treating the extra geology as normal `bonus_at()` resources.
- **Storage permits combinations; procedural generation does not currently use them.** Retain mutual exclusion for **all three current independent types** (Jungle/Uranium/Titanium) on generated worlds: no Jungle+geology, no Uranium+Titanium, and no triples. Retain existing separation from native major landmarks. Do not loosen generator eligibility or deliberately overlap deposits merely because the save model now permits it; combined +N/+M/+E tiles are too powerful for present world balance. **Future types may opt into overlapping placements explicitly**, using per-type rules and balance review, without changing the storage model.
- `THMIN001` v3 now stores a variable-length ordered list per tile and has an updated synthetic parser/format test. More future types require adding registry entries and retaining the one-byte-per-tile feature-count bound; 255 nonzero type IDs are representable but must be implemented individually. Do not prematurely choose a fixed mask width solely from the current three features; preserve future extensibility. Old experimental saves are not a release compatibility requirement, but a **new game must save/reload/continue with the same DLL**.
- The native tile Inspector must list all applicable independent feature names and keep native descriptions. AI evaluation, forecast/worker yields and special improvements require an explicit multi-type audit. If a manually/synthetically constructed tile has both geological types, REND-005 requires predictable and crash-free rendering, **not** a polished or fully legible dual-geology composition; ordinary resources must still draw above geology.

This decision retains native `LM_JUNGLE`, `LM_URANIUM` and all other original flags unmodified; it does not introduce `LM_TITANIUM` or consume retired Canyon position `0x200`.

## Generation

For the intended independent-feature maps, use `new_world_builder=1`, `modified_landmarks=1`, `minor_jungle=1`, `minor_uranium=1` and `minor_titanium=1`. The older `jungle=0` and `uranium=0` controls disable their **native major** placement, not these minor types.

1. The early phase generates Jungle before the native-major landmark placement loop and final climate/rainfall pass. Major landmarks can overwrite parts of those patches; the post-placement cleanup removes overlapping independent minor tiles.
2. The modified world generator then finalises climate and rockiness.
3. The late phase uses **one shared geological generator**, alternating Uranium then Titanium for each round (when enabled). Both use the same land eligibility, lowland preference, 3–9-tile weighted patch sizes, two-tile separation from existing patches/landmarks and fungus clearing. Independent RNG streams and a shared centre-spacing score keep the types separate and distributed. Each type targets 2–6 patches depending on map area; attempts can fail.
4. Mirrored map modes skip independent minor generation (symmetry is not implemented).

**Configuration:** all three minor types currently require `modified_landmarks=1` and the `new_world_builder=1` path. Geological types can be switched independently with `minor_uranium` and `minor_titanium`; the same shared routine handles either one alone. Mirrored worlds remain unsupported.

The legacy `LM_JUNGLE` / `LM_URANIUM` generator paths remain available in some unmodified/editor modes. These are not the independent registry types. Original major-landmark slot 10 (former Nessus Canyon) has been retired and reserved as `LM_RESERVED_10 = 0x200`; it is not used for the new minor features.

## Resource yields and AI

The registry currently defines **unconditional +1 basic-resource bonuses** (subject to the actual resource-calculation branch and restrictions). Normal land, base squares and Forests generally preserve their minor bonuses. Some special improvement branches overwrite calculated values: notably, a Thermal Borehole suppresses Jungle's nutrient and Titanium's mineral bonus, whereas Uranium's energy bonus can survive. Fungus and other special cases require individual checks. Ordinary resource restrictions can cap the realised bonus.

Thinker's worker selection, crawler logic and former scoring consume modified actual tile yields in several paths. Settlement scoring also explicitly recognises independent terrain, but uses coarse fixed bonuses. The projected `item_yield()` calculation is not fully identical to actual `mod_crop_yield()`, `mod_mine_yield()` and `mod_energy_yield()` for certain future improvements (especially Titanium/Jungle plus a Thermal Borehole). These discrepancies should be fixed in the later AI/terraform pass, once extraction rules are finalised.

**Balance status:** The distributed test `thinker.ini` now sets `tile_output_limit=3,3,3` using Thinker's existing configurable pre-tech resource-cap mechanic (up from 2,2,2); this is an **experimental configuration**, not a new source-level yield rule. The proposed 1/1/1 Forests, improvement-dependent geological extraction, and general terraform-AI rewrite are **not** implemented.

## Testing and limitations

**User-verified Windows Develop:** all three minor types generated and +1 nutrient/energy/mineral effects appeared; native Jungle/Uranium art worked. A user-local `neca_sprites.pcx` in mode 3 now renders distinct blue-grey Titanium art with correct transparency, alignment and zoom-in/out. The custom sprite covers native variant 0 only; variant 1 retains Uranium. The Inspector name was previously intermittent on Titanium and still needs isolated testing.

**Automated:** Windows Develop/Release CI compiled the DLL/launcher, and Develop checked static source invariants, generated/tested the indexed PCX fallback and parsed synthetic `THMIN001` v3 stacked saves (workflow 42314671873). These are **not** executable world-generation or full save/reload tests.

**Still to verify on the v3 DLL:** new-game same-build save/reload/continue and actual multi-feature behaviour (synthetic/compile evidence is not an in-game test); multiple seeds and map sizes; fragment/spacing statistics; complete fog, focus and partial redraw behaviour; resource and improvement combinations; native Inspector on standalone/composite features; real in-game save/reload; mirrored maps (currently excluded); Release and PRACX/reduced-mode behaviour.

See [rendering.md](rendering.md) for graphics architecture and deployment, and [sprite-atlas.md](sprite-atlas.md) for the reproducible local indexed-artwork pipeline.
