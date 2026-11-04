# Current status

**Updated:** 2026-11-04. **Repository:** `Necaladun/thinkerNeca`. **Active work:** [draft PR #4](https://github.com/Necaladun/thinkerNeca/pull/4), `feature/minor-terrain-generation-migration` → `master`.

## Where we are

The fork has an independent per-tile minor-terrain framework for **Jungle (+1 nutrient), Uranium (+1 energy), and Titanium (+1 mineral)**. Storage/yields were merged with [PR #3](https://github.com/Necaladun/thinkerNeca/pull/3); generation, native graphics, native Inspector integration, Canyon retirement and test INI changes are on draft PR #4.

**Latest user-reported Develop tests:** Jungle and Uranium native graphics work; Titanium +1 mineral works; the shared native Uranium placeholder now renders for Titanium **without the old positioning problem**, and Uranium/Titanium generation looks correct in a fresh map. **Latest Titanium observation:** external Uranium mode 1 rendered translucent/fragmented, but missing PCX fell back to good native Uranium. The copied PCX source pixels are correct; inspection of the supported executable shows the external path omitted native +10 palette-index decoding and mask conversions 6/7→9. Mode 1 now reproduces those steps with a processed-pixel checksum gate and substitutes only the matching first native variant. **Windows runtime visual result pending**; modes 0/2, generation and yields unchanged. Previous broad-scan hook returned active=0 (15 candidates). The Inspector's earlier intermittent Titanium label remains unverified. Great Dunes look good. Sunny Mesa's centre-only +1 energy is confirmed in-game. **Mesa elevation experiment rejected after user visual test:** the 2,000 m centre bulged into a dome beneath the native artwork; restored the original 5×5 plateau at 1,000–1,999 m with the low outer rim (MAP-006 closed). The test INI starts and runs.

**Automated:** Windows 32-bit Develop compilation, synthetic minor-save decoding, atlas tests, source/INI/Canyon contract tests. CI does not launch SMAC/X.

**Not yet release-ready:** Titanium sprite/data/Inspector mismatch (BUG-002), full real save/reload, fog/focus/redraw and extended resource interactions; native Jungle patch robustness; Titanium/Inspector Release parity; PRACX/reduced mode. PR #4 remains **draft**.

## What is active now

- **Testing:** use matching Develop DLL with local `neca_sprites.pcx` and `minor_titanium_sprite_mode=1` to compare corrected external Uranium against native mode 0. Confirm `NECA SPRITE processed ... verified=1`, native variant 0 substitution, variant 1 untouched, zoom/fog/layering, Inspector and same-build save/reload (REND-003, BUG-002). Shared native placeholder rendering and new-map geology have positive user reports.
- **Bugs/engineering:** `base_capture_fix=0` seems to resolve the observed probe problem, but disables unrelated capture fixes (BUG-001). Native Jungle hook preflight remains outstanding (REND-001).
- **Balance experiment:** current `tile_output_limit=3,3,3` (up from 2/2/2). Forest 1/1/1 and mine-only extraction are **not implemented**.
- **Major landmarks:** Great Dunes appearance passed; Sunny Mesa's centre-only +1E verified (MAP-005). The stepped Mesa profile was visually rejected and reverted (MAP-006). No special energy resource has been added. Slot 10 remains reserved.

## What to do next

1. Retest corrected Titanium mode 1 against native mode 0 on the same save/zoom and capture `NECA SPRITE processed ... verified=1` and variant logs; second native sprite variant is intentionally not replaced. Then verify missing-asset fallback, fog/zoom/units and Inspector (REND-003, BUG-002).
2. Test same-build save/reload and special tile/resource/improvement combinations ([TESTING](TESTING.md)).
3. Confirm the restored low, flat-topped Mesa appearance on a new generated map if convenient; continue other major-landmark checks (MAP-001).
4. Once runtime behavior is stable, harden Jungle hooks and Release packaging. Defer broader AI/Forest changes.

**Current configuration:** [`docs/thinker.ini`](../docs/thinker.ini) contains the active test profile, including `base_capture_fix=0`, extra starting units, no polar caps, excluded events 13/17 and 3/3/3 caps. Local `thinker_user.ini` may override it.

See [WORK-QUEUE](WORK-QUEUE.md) for actual tasks, [Fork changelog](../Fork-Changelog.md) for recent changes, and [Decisions](DECISIONS.md) for established positions.
