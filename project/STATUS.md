# Current status

**Updated:** 2026-11-04. **Repository:** `Necaladun/thinkerNeca`. **Active work:** [draft PR #4](https://github.com/Necaladun/thinkerNeca/pull/4), `feature/minor-terrain-generation-migration` → `master`.

## Where we are

The fork has an independent per-tile minor-terrain framework for **Jungle (+1 nutrient), Uranium (+1 energy), and Ironstone (+1 mineral)**. Storage/yields were merged with [PR #3](https://github.com/Necaladun/thinkerNeca/pull/3); generation, native graphics, native Inspector integration, Canyon retirement and test INI changes are on draft PR #4.

**Working in initial Develop gameplay tests:** Jungle and Uranium rendering, Ironstone alignment at normal and far zoom, minor-terrain names in the tile Inspector, and the latest test configuration starting/running without reported problems. This is an **initial user-reported smoke test**, not full game/release certification.

**Automated:** Windows 32-bit Develop compilation, synthetic minor-save decoding, atlas tests, source/INI/Canyon contract tests. CI does not launch SMAC/X.

**Not yet release-ready:** full real save/reload, fog/focus/redraw and extended resource interactions; native Jungle patch robustness; Ironstone/Inspector Release parity; PRACX/reduced mode. PR #4 remains **draft**.

## What is active now

- **Testing:** exercise the fresh-game map, terrain graphics, yields and save/reload matrix — [TESTING](TESTING.md), [TERR-001](WORK-QUEUE.md).
- **Bugs/engineering:** isolate the probe-related `base_capture_fix=0` workaround; strengthen Jungle native-hook installation — [BUG-001](WORK-QUEUE.md), [REND-001](WORK-QUEUE.md).
- **Balance experiment:** current `tile_output_limit=3,3,3` (up from 2/2/2). Forest 1/1/1 and mine-only extraction are **not implemented**.
- **New landmark:** slot 10 is reserved after retiring Nessus Canyon; replacement design is exploratory.

## What to do next

1. Continue the same-build runtime checks, especially save/reload and special tile/improvement combinations ([TESTING](TESTING.md)).
2. Record concrete findings as pass/fail observations and update [WORK-QUEUE](WORK-QUEUE.md) rather than expanding this status page.
3. Once runtime behavior is stable, tackle Jungle hook preflight and packaging/Release parity before merging the draft PR.
4. Defer Former AI and balance rewrites until the observed resource economy is clearer.

**Current configuration:** [`docs/thinker.ini`](../docs/thinker.ini) contains the active test profile, including `base_capture_fix=0`, extra starting units, no polar caps, excluded events 13/17 and 3/3/3 caps. Local `thinker_user.ini` may override it.

See [WORK-QUEUE](WORK-QUEUE.md) for actual tasks, [Fork changelog](../Fork-Changelog.md) for recent changes, and [Decisions](DECISIONS.md) for established positions.
