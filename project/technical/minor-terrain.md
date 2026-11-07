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
| Uranium | +1 energy | Late | Dispersed 3–5 tile deposits, lowland preference; existing fungus retained |
| Titanium | +1 mineral | Late | Dispersed 3–5 tile deposits, identical lowland preference to Uranium; existing fungus retained |

**Source implementation on PR #4 (TERR-004; Windows runtime not yet tested):** each tile may now hold several independent minor types with per-type codes. Jungle uses a climate/rainfall pattern; Uranium and Titanium use patch-local indices (0–8). The `minor_landmark_has/any_at/entries_at` queries, type-specific `minor_landmark_set/clear` and `minor_landmark_clear_all` replace the former single-valued lookup. Do not conflate *representability* with procedural overlap placement, and do not infer that every original native `LM_*` combination is mechanically/renderably valid.

## Meaning of deliberately stacked minor features

A **stacked minor tile** has more than one independent `THMIN001` feature ID on the same map square. Current feature contributions are Jungle +1 nutrient, Uranium +1 energy, and Titanium +1 mineral. A manually constructed Jungle+Uranium+Titanium square therefore has nominal feature bonuses **+1N/+1M/+1E**, subject to the ordinary terrain, improvement and cap logic. These are **distinct from an ordinary N/M/E resource icon overlapping a single geological feature**; that latter combination is normal and its dual-layer artwork was user-verified under REND-005.

TERR-004's per-tile ordered list can represent each pair or all three simultaneously, retaining separate codes; `minor_landmark_set(x,y,type,code)` adds/updates only the named type, and `minor_landmark_clear(x,y,type)` removes only that type. Synthetic v3 parser fixtures exercise two-type and three-type records. **The ordinary generator intentionally does not produce any Jungle/Uranium/Titanium pairings or triples** for balance and landmark separation. There is no established in-game editor control for constructing these new independent-type stacks.

**User decision (7 November): defer the following constructed-stack Windows runtime tests.** They do not block acceptance for ordinary generated-world use. Reopen them before deliberately generating or editing multiple independent minor types onto one tile. The deferred technical tests are: adding/removing a type must retain other types and codes; individual N/M/E contributions and Inspector names should appear once; save→exit→reload→advance must preserve each entry; ordinary resource graphics and foreground must remain intact. Forced Uranium+Titanium need only deterministic, crash-free geological drawing (both geological sprites need not be legible), with any ordinary N/M/E icon still visible above. These are technical consistency checks, **not a proposal to generate stacked deposits naturally**. REND-005's ordinary resource-over-geology rendering is accepted for the supported Develop test build; user separately deferred optional missing-art fallback and native Scenario Editor Uranium compatibility tests.

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
3. The late phase uses **one shared geological generator**, alternating Uranium then Titanium for each round (when enabled). In the approved **new distribution trial on draft PR #4**, patch sizes are **3–5 tiles** weighted 1:3:1 (mean 4, down from 6); targets scale from the old 2–6 deposits to **3–9 patches per type**, depending on map area. Both retain the same lowland preference, two-tile separation from existing patches/landmarks, independent RNG streams, shared centre-spacing score and land eligibility. **Existing fungus is preserved**, not cleared. The generator still makes at most eight attempts to complete each patch and discards incomplete candidate patches; actual successful counts can fall below targets on crowded maps. This modifies new-map RNG outcomes but not saves, yields, or current terrain overlap policy.
4. Mirrored map modes skip independent minor generation (symmetry is not implemented).

**Configuration:** all three minor types currently require `modified_landmarks=1` and the `new_world_builder=1` path. Geological types can be switched independently with `minor_uranium` and `minor_titanium`; the same shared routine handles either one alone. Mirrored worlds remain unsupported.

The legacy `LM_JUNGLE` / `LM_URANIUM` generator paths remain available in some unmodified/editor modes. These are not the independent registry types. Original major-landmark slot 10 (former Nessus Canyon) has been retired and reserved as `LM_RESERVED_10 = 0x200`; it is not used for the new minor features.

## Resource yields and AI

The registry defines +1 basic-resource contributions, subject to improvement-specific branches and caps. **Independent Titanium's +1 mineral and independent Uranium's +1 energy do not apply to Forests**, both in actual worker yields and Thinker's future-Forest `item_yield()` forecast. Forests retain their ordinary 1/2/1 baseline (plus unrelated normal resource/special project effects); the geological record is not erased by forestry and becomes productive again if the Forest is removed. Jungle's +1 nutrient in Forests is not changed. Some special improvement branches overwrite calculated values: notably, a Thermal Borehole suppresses Jungle's nutrient and Titanium's mineral bonus, whereas Uranium's energy bonus can survive. Fungus takes its own resource-yield branch in the current live calculation: on a fungus-covered Uranium/Titanium tile, the +1E/+1M geological bonus does **not** increase the displayed fungus yield; clearing fungus exposes the underlying bonus through the normal land calculation. This is now intentional for the distribution trial. Other special cases require individual checks. Ordinary resource restrictions can cap the realised bonus.

Thinker's worker selection, crawler logic and former scoring consume modified actual tile yields in several paths. Settlement scoring also explicitly recognises independent terrain, but uses coarse fixed bonuses. The projected `item_yield()` calculation is not fully identical to actual `mod_crop_yield()`, `mod_mine_yield()` and `mod_energy_yield()` for certain future improvements (especially Titanium/Jungle plus a Thermal Borehole). These discrepancies should be fixed in the later AI/terraform pass, once extraction rules are finalised.

**Balance status:** The distributed test `thinker.ini` now sets `tile_output_limit=3,3,3` using Thinker's existing configurable pre-tech resource-cap mechanic (up from 2,2,2); this is an **experimental configuration**, not a new source-level yield rule. The proposed 1/1/1 Forests, improvement-dependent geological extraction, and general terraform-AI rewrite are **not** implemented. The **existing Mine enhancement remains**: Titanium adds +1 passive mineral to suitable non-Forest land and supplies the same single +1 Mine modifier used for ordinary mineral resource/major mineral landmark tiles, not an extra Mine increment for each stacked condition. Keep this for the smaller 3–5-tile test before deciding whether it is still too strong. **Newly generated Titanium cannot occupy normal mineral-resource squares** (flagged or seed/procedural); nutrient/energy resources may overlap. This avoids treating one mineral-rich deposit as both normal mineral bonus and dispersed Titanium.

## Testing and limitations

**User-verified Windows Develop:** all three minor types generated and +1 nutrient/energy/mineral effects appeared; native Jungle/Uranium art worked. A user-local `neca_sprites.pcx` in mode 3 now renders distinct blue-grey Titanium art with correct transparency, alignment and zoom-in/out. The custom sprite covers native variant 0 only; variant 1 retains Uranium. The Inspector name was previously intermittent on Titanium and still needs isolated testing.

**Automated:** Windows Develop/Release CI compiled the DLL/launcher, and Develop checked static source invariants, generated/tested the indexed PCX fallback and parsed synthetic `THMIN001` v3 stacked saves (workflow 42314671873). These are **not** executable world-generation or full save/reload tests.

**Still to verify on the v3 DLL:** new-game same-build save/reload/continue and actual multi-feature behaviour (synthetic/compile evidence is not an in-game test); multiple seeds and map sizes; fragment/spacing statistics; complete fog, focus and partial redraw behaviour; resource and improvement combinations; native Inspector on standalone/composite features; real in-game save/reload; mirrored maps (currently excluded); Release and PRACX/reduced-mode behaviour.

See [rendering.md](rendering.md) for graphics architecture and deployment, and [sprite-atlas.md](sprite-atlas.md) for the reproducible local indexed-artwork pipeline.
