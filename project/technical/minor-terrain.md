# Independent minor terrain — current implementation

> **Branch scope (2026-11-07):** This reference describes experimental terrain/rendering code on [draft PR #4](https://github.com/Necaladun/thinkerNeca/pull/4), **not yet merged into `master`**. The default branch still has earlier Ironstone and map behaviour; verify implementation against the working branch.


**Reference document.** Work items: [queue](../WORK-QUEUE.md); runtime evidence: [testing](../TESTING.md).

**Status:** Implemented on draft PR #4; initial Windows Develop gameplay checks have passed, but full regression coverage and Release parity are outstanding. This document describes the implemented rules, **not** proposed forest/cap/deposit balance changes.

## Data model

Independent minor terrain is stored per map tile as `MinorTerrainTile { uint8_t type, uint8_t code }` in `src/savegame.cpp` / `src/savegame.h`. It does **not** consume an original `LM_*` landmark bit or create a named major landmark. The definitions registry supplies name, nutrient/mineral/energy bonus, fallback sprite slot and generation phase.

`THMIN001` version 2 writes two bytes per tile after the native map data. The current reader accepts native saves with no extension; version 1 extensions are incompatible. **Forward development assumes new games**; compatibility with older mod saves or custom scenarios is not a design requirement, although some legacy support code remains.

| Type | Current bonus | Phase | Distribution |
| --- | --- | --- | --- |
| Jungle | +1 nutrient | Early | Climate-/moisture-sensitive patches, generally 8–16 tiles, occasionally larger |
| Uranium | +1 energy | Late | Dispersed 3–9 tile deposits, lowland preference |
| Titanium | +1 mineral | Late | Dispersed 3–9 tile deposits; currently identical lowland preference to Uranium |

**Present limitation:** only one independent minor type is stored per tile. The `code` field is type-specific: Jungle uses a climate/rainfall pattern; Uranium and Titanium currently use patch-local indices (0–8). **Approved replacement (DEC-016/TERR-004; not coded):** each tile may carry several independently identifiable minor types, each with its own code. Do not conflate *representability* with procedural overlap placement, and do not infer that every original native `LM_*` combination is mechanically/renderably valid.

## Approved multi-feature design (TERR-004; not implemented)

The independent layer must become **compositional**, rather than a one-of enum:

- `has_minor(x,y,type)`, `minor_code_at(x,y,type)`, and add/remove/iterate operations must be type-specific. Setting Titanium on Jungle must retain Jungle and its code. A tile may hold Jungle + Uranium + Titanium; the model must also retain native `LM_*` flags and ordinary resource bonuses unchanged.
- Each present type contributes its registered N/M/E effects **once** through the existing live yield branches and resource caps, with future special-case restrictions made explicit. A tile with Jungle + Uranium + Titanium nominally has +1N/+1M/+1E before technology/improvement/cap rules; no double-counting or treating the extra geology as normal `bonus_at()` resources.
- Distinguish **storage permission** from **generation choice**. The current generation has one-type-per-tile eligibility and removes early minor features when a major landmark wins. Allowing multiple minor types does *not by itself* decide whether random geology should overlap Jungle, another deposit or a native major. Placement and removal must be consciously updated rather than accidentally overwriting features; optional overlap restrictions need their own documented rationale.
- `THMIN001` v2's 2-byte record cannot store a general set of independent type/code pairs. A new versioned save representation and associated synthetic tests are required. Do not prematurely choose a fixed mask width solely from the current three features; preserve future extensibility. Old experimental saves are not a release compatibility requirement, but a **new game must save/reload/continue with the same DLL**.
- The native tile Inspector must list all applicable independent feature names and keep native descriptions. AI evaluation, forecast/worker yields and special improvements require an explicit multi-type audit. Render composition, especially two geological sprites, is a separate REND-005 task.

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

**Automated:** the Windows Develop CI builds the DLL/launcher, checks static source invariants, generates/tests the indexed PCX fallback and parses synthetic `THMIN001` v2 saves. These are **not** executable world-generation or full save/reload tests.

**Still to verify:** multiple seeds and map sizes; fragment/spacing statistics; complete fog, focus and partial redraw behaviour; resource and improvement combinations; native Inspector on standalone/composite features; real in-game save/reload; mirrored maps (currently excluded); Release and PRACX/reduced-mode behaviour.

See [rendering.md](rendering.md) for graphics architecture and deployment, and [sprite-atlas.md](sprite-atlas.md) for the reproducible local indexed-artwork pipeline.
