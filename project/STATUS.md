# Current status

> **Master documentation status (7 November 2026):** PR #4 terrain/rendering and PR #5 Empire Report are still unmerged. Their working branches contain code that `master` does not; this document records their experimental state, not master runtime functionality.

**Updated:** 2026-11-07. **Repository:** `Necaladun/thinkerNeca`. **Active:** [draft PR #4](https://github.com/Necaladun/thinkerNeca/pull/4), `feature/minor-terrain-generation-migration` → `master`.

## Verified foundation

- Independent **Jungle +1 nutrient**, **Uranium +1 energy**, **Titanium +1 mineral** per-tile storage and yields are in the fork. Storage/yields merged in PR #3; generation, rendering, Inspector, Canyon retirement and test INI changes remain on PR #4.
- **Windows Develop user tests (5 Nov):** local `neca_sprites.pcx` with distinct blue-grey Titanium art displays on Titanium tiles, with correct transparency, positioning and zoom-in/out; +1 mineral works. The user subsequently reports the visible feature persisted across repeated loads and turn advances. Yield after reloading and Jungle/Uranium persistence were not separately reported. This is successful **native sprite variant-0** evidence, not a complete graphics regression.
- External Uranium reference mode 1 visually matched native Uranium. Runtime log confirmed processed hash `24A1B67A`, 5635 transparent / 565 visible pixels and identical sprite size, offsets, pitch and transparency key. Mode-3 Titanium log: hash `D4B3AC2C` and 559 visible pixels.
- New-map geology, native Jungle and Uranium art have positive user reports. Great Dunes look good; Sunny Mesa centre-only +1E confirmed. Raised Mesa experiment was rejected and reverted (MAP-006).
- **CI passed on `4809cb7` (5 November):** separate Windows 32-bit Develop and Release cross-builds; Develop synthetic save/atlas/configuration/source contracts all passed. **Neither build executes SMAC/X.** Source changes add Jungle patch preflight, avoid redundant overlay scans, trace successful native Inspector appends and independently log variant-1 sprite geometry; their Windows runtime effects remain unverified.

## Parked Empire Report (unmerged draft PR #5)

- [UI-001](WORK-QUEUE.md): Draft [PR #5](https://github.com/Necaladun/thinkerNeca/pull/5) remains **parked** and stacked on draft PR #4, whose master documentation conflicts were resolved in `392fd66`. The **combined Windows Develop test build `3c08b52`** has now passed the user's six-part broad smoke check including the native Empire Report, colony actions, same-build save/reload and layout/interaction. UI-001 is functionally accepted **for this tested build**, pending PR #4 completion and PR #5 ancestry reconciliation, rerun CI and short final integrated-binary regression. See [testing](TESTING.md).
- [UI-002](WORK-QUEUE.md): Deferred **start-of-turn consolidated Turn Report** with 30+ actual production-completion events, post-upkeep finances/research, actionable colony orders and optional Secret Project movie choice. This is **not implemented** in PR #5 and is not a blocker for merging its manual report.

## Active test configuration

Distributed [test INI](../docs/thinker.ini): `minor_native_titanium=1` and `minor_titanium_sprite_mode=3`, matching the user's successful local art test. Compiled absent-option default is mode 0 (native Uranium). Mode 3 needs a separately prepared 1024×1024 indexed `neca_sprites.pcx` next to the executable; neither Git nor public CI provides that art. Missing art falls back to native Uranium. `thinker_user.ini` may override the distributed profile, and the complete user's installed INI is not independently inspected.

Other current test-profile values include `base_capture_fix=0` (probe workaround), `tile_output_limit=3,3,3` (balance trial), extra starting units, polar caps disabled and excluded events 13/17. See [configuration](technical/configuration.md).

## Combined Windows Develop milestone — 7 November 2026

- User reports all six requested combined PR #4 + PR #5 Develop checks passing with binary `3c08b52`: save/reload/advance, Empire Report, terrain graphics, Inspector, map generation and compatibility. The same build's Develop/Release cross-builds passed CI; **only Develop has in-game verification**. No exact seed list, detailed Inspector/resource-overlap trace or Release test was supplied. Details: [TESTING](TESTING.md).

## Remaining before PR #4 can merge

- [TERR-001](WORK-QUEUE.md): broad same-build save→reload→continue, fog/redraw and UI smoke now reported passing on combined Develop; finish any specifically identified edge-case evidence on the final integrated binary.
- [REND-001/002/004](WORK-QUEUE.md): Jungle positive-path rendering and missing-PCX Uranium fallback pass Windows Develop. Negative Jungle-hook preflight requires a harness; Release runtime parity is deferred; the supplied log only encountered Titanium sprite variant 0 (variant 1 not characterised).
- [BUG-002](WORK-QUEUE.md): Inspector hook installed and labels look correct; prior intermittent omission remains unreproduced. [BUG-003](WORK-QUEUE.md): **done** for the original resource-precedence bug. [TERR-004](WORK-QUEUE.md): approved architecture change for **multiple independent features per tile**, native `LM_*` preserved. [REND-005](WORK-QUEUE.md): approved native Uranium-style geological underlay beneath ordinary resources, including multiple geology composition; neither feature implemented yet. [BUG-001](WORK-QUEUE.md): capture investigation deferred, workaround remains.
- User reports additional save/reload persistence and greater/lesser Jungle abundance in rainy/dry world-generation tests; geological placement looked normal. The subsequently withdrawn save attachment is **not accepted as test evidence**. Representative seed/climate metrics remain unrecorded. PR #4 stays **draft**.


## Other development and design documentation

- **PR #5:** Empire Report native UI remains a draft feature requiring further game-specific validation; its source is not part of `master`.
- **Telemetry/automation:** accepted future work with actionable `TEL-*`, `AUTO-*`, `DIAG-*` and `DEV-*` entries; no unattended simulation code is implemented in `master`. See [technical plan](technical/telemetry-automation.md).
- **Deferred studies:** [special abilities](technical/special-abilities.md), [aircraft combat](technical/aircraft-combat.md) and [AI operations/formations](technical/ai-strategy-operations.md) record source inspection and ideas without committing to gameplay changes.

## Separate combat review

Source inspection identified an existing ordinary-artillery-bombardment reactor-normalization mismatch under `ignore_reactor_power=1` ([BUG-004](WORK-QUEUE.md)); a reported late-game Spore Launcher weakness is consistent but lacks isolated Windows reproduction. Broader chassis, armour cost and artillery-duel changes are **unapproved exploration** ([BAL-004](WORK-QUEUE.md)). Neither changes the active terrain PR's merge criteria; [technical reference](technical/unit-combat-and-costs.md).

**Next:** design/implement REND-005 safe native dual-glyph rendering with default Planet Uranium Flats screenshot as visual precedent. Investigate reachability of native sprite variant 1; complete REND-001 negative preflight harness. Probe capture and Release feature parity are explicitly deferred, not resolved. Test all resulting code on the final integrated Windows binary. See [TESTING](TESTING.md), [WORK-QUEUE](WORK-QUEUE.md), and [sprite-atlas](technical/sprite-atlas.md).
