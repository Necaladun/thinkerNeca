# Current status

**Updated:** 2026-11-04. **Repository:** `Necaladun/thinkerNeca`. **Active work:** [draft PR #4](https://github.com/Necaladun/thinkerNeca/pull/4), `feature/minor-terrain-generation-migration` → `master`.

## Where we are

The fork has an independent per-tile minor-terrain framework for **Jungle (+1 nutrient), Uranium (+1 energy), and Ironstone (+1 mineral)**. Storage/yields were merged with [PR #3](https://github.com/Necaladun/thinkerNeca/pull/3); generation, native graphics, native Inspector integration, Canyon retirement and test INI changes are on draft PR #4.

**Latest user-reported Develop tests:** Jungle graphics and +1 nutrient work; Uranium graphics, +1 energy and Inspector name work; Ironstone grants +1 mineral, but **its sprite sometimes appears on tiles without Ironstone and its Inspector name appears inconsistently**. Great Dunes look good. Sunny Mesa's centre-only +1 energy is confirmed in-game. **New Mesa elevation experiment:** 3×3 plateau at 2,000–2,999 m; 5×5 middle ring at 1,000–1,999 m; outer 7×7 ring below 1,000 m. Rendering and real generated heights await a gameplay test (MAP-006). The test INI starts and runs.

**Automated:** Windows 32-bit Develop compilation, synthetic minor-save decoding, atlas tests, source/INI/Canyon contract tests. CI does not launch SMAC/X.

**Not yet release-ready:** Ironstone sprite/data/Inspector mismatch (BUG-002), full real save/reload, fog/focus/redraw and extended resource interactions; native Jungle patch robustness; Ironstone/Inspector Release parity; PRACX/reduced mode. PR #4 remains **draft**.

## What is active now

- **Testing:** reproduce Ironstone false-position sprites and intermittent Inspector names (BUG-002); check fresh-game save/reload, fog/zoom, yields and overlap (TERR-001). Jungle, Uranium and Dunes have positive observations.
- **Bugs/engineering:** `base_capture_fix=0` seems to resolve the observed probe problem, but disables unrelated capture fixes (BUG-001). Native Jungle hook preflight remains outstanding (REND-001).
- **Balance experiment:** current `tile_output_limit=3,3,3` (up from 2/2/2). Forest 1/1/1 and mine-only extraction are **not implemented**.
- **Major landmarks:** Great Dunes appearance passed; Sunny Mesa's centre-only +1E verified (MAP-005). The new stepped Mesa elevation profile is implemented for **visual/runtime testing** (MAP-006). No special energy resource has been added. Slot 10 remains reserved.

## What to do next

1. Reproduce Ironstone sprite/data and Inspector mismatch with exact coordinates and zoom; coordinate with the ongoing sprite redesign (BUG-002, REND-003).
2. Test same-build save/reload and special tile/resource/improvement combinations ([TESTING](TESTING.md)).
3. Start a fresh generated world and inspect Mesa's new summit/middle/outer altitude tiers, cliff sprite, neighbouring terrain, coastlines and Solar yields (MAP-006). Continue other major-landmark checks (MAP-001).
4. Once runtime behavior is stable, harden Jungle hooks and Release packaging. Defer broader AI/Forest changes.

**Current configuration:** [`docs/thinker.ini`](../docs/thinker.ini) contains the active test profile, including `base_capture_fix=0`, extra starting units, no polar caps, excluded events 13/17 and 3/3/3 caps. Local `thinker_user.ini` may override it.

See [WORK-QUEUE](WORK-QUEUE.md) for actual tasks, [Fork changelog](../Fork-Changelog.md) for recent changes, and [Decisions](DECISIONS.md) for established positions.
