# Current status

**Updated:** 2026-11-05. **Repository:** `Necaladun/thinkerNeca`. **Active:** [draft PR #4](https://github.com/Necaladun/thinkerNeca/pull/4), `feature/minor-terrain-generation-migration` → `master`.

## Verified foundation

- Independent **Jungle +1 nutrient**, **Uranium +1 energy**, **Titanium +1 mineral** per-tile storage and yields are in the fork. Storage/yields merged in PR #3; generation, rendering, Inspector, Canyon retirement and test INI changes remain on PR #4.
- **Windows Develop user tests (5 Nov):** local `neca_sprites.pcx` with distinct blue-grey Titanium art displays on Titanium tiles, with correct transparency, positioning and zoom-in/out; +1 mineral works. The user subsequently reports the visible feature persisted across repeated loads and turn advances. Yield after reloading and Jungle/Uranium persistence were not separately reported. This is successful **native sprite variant-0** evidence, not a complete graphics regression.
- External Uranium reference mode 1 visually matched native Uranium. Runtime log confirmed processed hash `24A1B67A`, 5635 transparent / 565 visible pixels and identical sprite size, offsets, pitch and transparency key. Mode-3 Titanium log: hash `D4B3AC2C` and 559 visible pixels.
- New-map geology, native Jungle and Uranium art have positive user reports. Great Dunes look good; Sunny Mesa centre-only +1E confirmed. Raised Mesa experiment was rejected and reverted (MAP-006).
- **CI passed on `4809cb7` (5 November):** separate Windows 32-bit Develop and Release cross-builds; Develop synthetic save/atlas/configuration/source contracts all passed. **Neither build executes SMAC/X.** Source changes add Jungle patch preflight, avoid redundant overlay scans, trace successful native Inspector appends and independently log variant-1 sprite geometry; their Windows runtime effects remain unverified.

## Experimental UI (separate draft PR #5)

- [UI-001](WORK-QUEUE.md): Native Empire Report [draft PR #5](https://github.com/Necaladun/thinkerNeca/pull/5). **User-tested 6 Nov:** twelve tabs, native Messages, Unit Register, Growth/Economy/Defenses/Attention, original detached Change Production picker and Next in Queue preview, plus Hurry/Gov. **Production changes pending Windows retest:** `bd334ba` full-colony sequential handler was defective (picker appeared only after closing colony); `96e0eba`/`5a671ca` move the Change picker into the colony modal loop via one-shot registered message, unverified in-game. **New source `6baa749`:** Production's Building remains the native Change action; Next in Queue becomes a second clickable action opening the colony's native Queue controls via its queue-title click region, even when no next item exists (label `None` instead of `--`). Both retain colony context; native queue handling is source-level provisional until tested. No direct changes to queue_items or saved data. Formatting deferred; PR draft/unmerged; save/reload and UI regression tests outstanding.

## Active test configuration

Distributed [test INI](../docs/thinker.ini): `minor_native_titanium=1` and `minor_titanium_sprite_mode=3`, matching the user's successful local art test. Compiled absent-option default is mode 0 (native Uranium). Mode 3 needs a separately prepared 1024×1024 indexed `neca_sprites.pcx` next to the executable; neither Git nor public CI provides that art. Missing art falls back to native Uranium. `thinker_user.ini` may override the distributed profile, and the complete user's installed INI is not independently inspected.

Other current test-profile values include `base_capture_fix=0` (probe workaround), `tile_output_limit=3,3,3` (balance trial), extra starting units, polar caps disabled and excluded events 13/17. See [configuration](technical/configuration.md).

## Remaining before PR #4 can merge

- [TERR-001](WORK-QUEUE.md): new-game **same-build save→reload→continue**, unusual improvements, fog, unit/base drawing and Inspector checks.
- [REND-001/002/004](WORK-QUEUE.md): Jungle all-site preflight is implemented but needs Windows verification; Release/packaging parity and the second Uranium/Titanium sprite variant remain open (variant 1 still uses native Uranium).
- [BUG-002](WORK-QUEUE.md): intermittent Inspector label. [BUG-003](WORK-QUEUE.md): naturally seeded resource icons were hidden by geology though yields stacked; resource-priority source fix now needs Windows retest. [BUG-001](WORK-QUEUE.md): probe/base capture workaround still needs root-cause investigation.
- More map seeds/climates and landmark regressions remain open. PR #4 stays **draft**.

**Next:** same-build save/reload, fog/unit/Inspector checks with the working mode-3 atlas; verify the new all-site Jungle preflight, then variant 1 and Release parity. See [TESTING](TESTING.md), [WORK-QUEUE](WORK-QUEUE.md), and [sprite-atlas](technical/sprite-atlas.md).
