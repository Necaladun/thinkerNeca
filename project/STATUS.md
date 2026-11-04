# Current status

**Updated:** 2026-11-04. **Repository:** `Necaladun/thinkerNeca`. **Active work:** [draft PR #4](https://github.com/Necaladun/thinkerNeca/pull/4), `feature/minor-terrain-generation-migration` → `master`.

## Where we are

The fork has an independent per-tile minor-terrain framework for **Jungle (+1 nutrient), Uranium (+1 energy), and Titanium (+1 mineral)**. Storage/yields were merged with [PR #3](https://github.com/Necaladun/thinkerNeca/pull/3); generation, native graphics, native Inspector integration, Canyon retirement and test INI changes are on draft PR #4.

**Previous user-reported Develop tests:** Jungle graphics and +1 nutrient work; Uranium graphics, +1 energy and Inspector name work; Titanium grants +1 mineral, but its old custom sprite appeared offset from saved tiles and its Inspector name appeared inconsistently. **New untested change:** Titanium now shares Uranium's native rendering and the two geological types alternate generation; the same-save and fresh-map regressions are pending. Great Dunes look good. Sunny Mesa's centre-only +1 energy is confirmed in-game. **Mesa elevation experiment rejected after user visual test:** the 2,000 m centre bulged into a dome beneath the native artwork; restored the original 5×5 plateau at 1,000–1,999 m with the low outer rim (MAP-006 closed). The test INI starts and runs.

**Automated:** Windows 32-bit Develop compilation, synthetic minor-save decoding, atlas tests, source/INI/Canyon contract tests. CI does not launch SMAC/X.

**Not yet release-ready:** Titanium sprite/data/Inspector mismatch (BUG-002), full real save/reload, fog/focus/redraw and extended resource interactions; native Jungle patch robustness; Titanium/Inspector Release parity; PRACX/reduced mode. PR #4 remains **draft**.

## What is active now

- **Testing:** reload the saved 2101 coordinates with shared native Uranium/Titanium sprites; verify no offset, Inspector names and 1E/1M; then start a fresh map to verify alternating geological patches, two-tile separation, fog/zoom and save/reload (BUG-002, TERR-001). Jungle, Uranium and Dunes have positive observations.
- **Bugs/engineering:** `base_capture_fix=0` seems to resolve the observed probe problem, but disables unrelated capture fixes (BUG-001). Native Jungle hook preflight remains outstanding (REND-001).
- **Balance experiment:** current `tile_output_limit=3,3,3` (up from 2/2/2). Forest 1/1/1 and mine-only extraction are **not implemented**.
- **Major landmarks:** Great Dunes appearance passed; Sunny Mesa's centre-only +1E verified (MAP-005). The stepped Mesa profile was visually rejected and reverted (MAP-006). No special energy resource has been added. Slot 10 remains reserved.

## What to do next

1. Test the shared native Uranium/Titanium rendering on the known saved 2101 tiles and recheck Inspector labels; confirm fresh-map alternating patch separation (BUG-002, TERR-002).
2. Test same-build save/reload and special tile/resource/improvement combinations ([TESTING](TESTING.md)).
3. Confirm the restored low, flat-topped Mesa appearance on a new generated map if convenient; continue other major-landmark checks (MAP-001).
4. Once runtime behavior is stable, harden Jungle hooks and Release packaging. Defer broader AI/Forest changes.

**Current configuration:** [`docs/thinker.ini`](../docs/thinker.ini) contains the active test profile, including `base_capture_fix=0`, extra starting units, no polar caps, excluded events 13/17 and 3/3/3 caps. Local `thinker_user.ini` may override it.

See [WORK-QUEUE](WORK-QUEUE.md) for actual tasks, [Fork changelog](../Fork-Changelog.md) for recent changes, and [Decisions](DECISIONS.md) for established positions.
