# Map generation and major landmarks

> **Branch scope (2026-11-07):** This reference describes experimental terrain/rendering code on [draft PR #4](https://github.com/Necaladun/thinkerNeca/pull/4), **not yet merged into `master`**. The default branch still has earlier Ironstone and map behaviour; verify implementation against the working branch.


**Current code:** `src/mapgen.cpp`, `src/engine_enums.h`, `src/main.cpp`. **Settings:** [configuration](configuration.md) and [`docs/thinker.ini`](../../docs/thinker.ini). Outstanding regression work: [MAP-001/MAP-002](../WORK-QUEUE.md). This is the **implemented** arrangement, not a proposal for replacement landmarks.

## Generation paths

- `new_world_builder=1` invokes Thinker's generator. The current test profile has `modified_landmarks=1`.
- The modified generator places independent climate-sensitive Jungle **early**, then native major landmarks; major placement clears overlapping earlier minor terrain. It finalises climate/rockiness before a **shared late geology pass**, alternating one Uranium and one Titanium deposit each round (if both enabled).
- Independent Uranium and Titanium do not use `LM_URANIUM` or any other major slot. Each deposit uses the same lowland eligibility and **preserves any existing fungus** (clearing it is left to the player); completed patches enforce a two-tile buffer against both types and major landmarks. Different geology preferences are future ideas, not current rules. The old major Jungle/Uranium generators are disabled under the modified-map route.
- Mirrored world generation currently skips independent minor terrain. Neither mirrored modes nor the native original builder are a supported target for the current fresh-game terrain experience.

**Future overlap policy (DEC-016/TERR-004):** current eligibility and post-generation cleanup intentionally prevent minor-versus-minor and minor-versus-major overlaps. The approved **data model** must allow multiple minor features on a tile, including conjunctions with native flags. **Approved generation policy:** preserve existing exclusion and spacing between all current independent Jungle/Uranium/Titanium features, and separation from native major landmarks. The engine may *store* Jungle+geology or Uranium+Titanium on an intentionally edited/synthetic tile, but the generator must **not** place these combinations because of the unbalanced combined yields. Only future type-specific, explicitly approved overlap rules may relax exclusion. Shared native per-tile `code_at()` can conflict across overlapping native landmarks, so the ability to store multiple `LM_*` bits does not guarantee every native major combination works correctly. This section describes current generation until TERR-004 is implemented.

## Sixteen native major slots

Original slot ordering is retained; **new independent minor types consume none of them**.

| Slot | Major landmark ID/name | Current random-map test profile |
| --- | --- | --- |
| 1 | Garland Crater | `crater=1` |
| 2 | Mount Planet | `volcano=1` |
| 3 | Monsoon Jungle | Native major `jungle=0`; independent minor Jungle enabled |
| 4 | Uranium Flats | Native major `uranium=0`; independent minor Uranium enabled |
| 5 | New Sargasso | `sargasso=1` |
| 6 | The Ruins | `ruins=1` |
| 7 | Great Dunes | `dunes=1`; modified generator |
| 8 | Freshwater Sea | `fresh=1` |
| 9 | Sunny Mesa | `mesa=1` |
| 10 | Former Nessus Canyon | **Reserved** as `LM_RESERVED_10 = 0x200`; generator, old mineral bonus and INI option removed |
| 11 | Geothermal Shallows | `geothermal=1` |
| 12 | Pholus Ridge | `ridge=1` |
| 13 | Borehole Cluster | `borehole=1`; modified four-borehole generation |
| 14 | Manifold Nexus | `nexus=1` |
| 15 | U.N.S. Unity Wreckage | `unity=1` (expansion context) |
| 16 | Fossil Field Ridge | `fossil=1`; modified shelf placement |

Additional modified features include changed Great Dunes geometry, resource distribution and a freshwater oasis. **Great Dunes have a positive user visual test**, while full regression on other landmarks remains outstanding.

**Sunny Mesa (MAP-005, MAP-006):** the generator marks the full `TableRange[3]` (7×7, 49-tile) area as `LM_MESA`. The earlier generator raised the **central 5×5** (`TableRange[2]`, 25 tiles) to the 1,000–1,999 m band and left the outer 24 tiles below 1,000 m; the +1 energy effect was separately restricted to `code_at() < 9` (central 3×3), which the user verified in-game on 2026-11-04. **Current elevation (restored):** the central 5×5 (indices 0–24) is generated in the 1,000–1,999 m band and the outer 7×7 rim (indices 25–48) in the 0–999 m band. The engine's normal `world_alt_set(..., true)` slope adjustments remain. **Discarded MAP-006 experiment:** raising the 3×3 core to the 2,000–2,999 m band caused the native Mesa surface to look domed/bulging in a user screenshot (2026-11-04); reverted without changing the sprite or the confirmed +1E centre-only effect. No guaranteed energy resource was added.

**Slot 10:** the old Canyon-shaped random terrain, name and economic bonus no longer generate, and the native WorldBuilder Canyon call is disabled. The original executable may still contain native Canyon graphics/editor interpretation for flag `0x200`; do not assign it to new terrain before implementing appropriate new rendering. No new landmark has been selected.

## Geological distribution trial — 7 November 2026

Approved for draft PR #4: Uranium and Titanium deposits shrink from the old weighted 3–9 tiles (mean 6) to **3–5** (weights 1:3:1, mean 4), while per-type deposit attempts increase by approximately 50% from **2–6** to **3–9** based on map area. The mathematical expected number of geological tiles remains approximately constant where all attempts succeed; realised values can be lower because the existing 2-tile exclusion buffer can reject centres or shapes. Existing fungus is **not cleared** by geology generation. A fungus-covered deposit normally produces fungus-specific yields until clearing, even while its hidden geology is stored and Inspector-visible. Other generation rules, natural overlap exclusions, resource bonuses, saving and rendering remain unchanged. This is a **balance experiment** until representative fresh maps are compared; previous successful Develop graphics and save/reload tests do not verify this revised distribution.

## Testing and balance

A new generation seed need not reproduce older builds if algorithms change. During the current testing pass, however, avoid RNG-affecting refactors without deliberate review, so seed comparisons remain meaningful.

See [TESTING](../TESTING.md) for map distributions, overlaps, graphics, fog and save/reload coverage. Suggestions for future major slots belong in [IDEAS](../IDEAS.md), not this implemented-behavior reference.

## Landmark minerals and Mine effects

Current `src/map.cpp` mechanics: Garland Crater and Mount Planet grant **+1 mineral in their centre 3×3** (`code_at() < 9`); Mount Planet also grants **+1 energy** there. Fossil Field Ridge has +1 mineral in its special-coded tiles (`code_at() < 6`). These mineral bonuses are conditional on the landmark not being disabled.

For ordinary land, a qualifying **landmark mineral bonus**, normal mineral resource bonus, or independent **Titanium** mineral bonus also increments the Mine modifier by **+1**. Thus the central Crater/Mount Planet tiles and Titanium can gain +1 inherent mineral plus an *additional* +1 mineral from Mine compared to equivalent tiles without that feature. The actual difference is subject to Mine-without-Road restrictions and mineral output caps (currently `tile_output_limit=3,3,3` in the test INI). Mines are **not required** for the inherent +1 mineral. Projected `item_yield()` should not be treated as a comprehensive Mine evaluation; see TERR-003.
