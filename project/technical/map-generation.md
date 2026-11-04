# Map generation and major landmarks

**Current code:** `src/mapgen.cpp`, `src/engine_enums.h`, `src/main.cpp`. **Settings:** [configuration](configuration.md) and [`docs/thinker.ini`](../../docs/thinker.ini). Outstanding regression work: [MAP-001/MAP-002](../WORK-QUEUE.md). This is the **implemented** arrangement, not a proposal for replacement landmarks.

## Generation paths

- `new_world_builder=1` invokes Thinker's generator. The current test profile has `modified_landmarks=1`.
- The modified generator places independent climate-sensitive Jungle **early**, then native major landmarks; major placement clears any overlapping earlier minor terrain. It finalises climate/rockiness before depositing independent Uranium/Ironstone **late**.
- Independent Uranium and Ironstone do not use `LM_URANIUM` or any other major slot. The old major Jungle/Uranium generators are disabled under the modified-map route.
- Mirrored world generation currently skips independent minor terrain. Neither mirrored modes nor the native original builder are a supported target for the current fresh-game terrain experience.

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

## Testing and balance

A new generation seed need not reproduce older builds if algorithms change. During the current testing pass, however, avoid RNG-affecting refactors without deliberate review, so seed comparisons remain meaningful.

See [TESTING](../TESTING.md) for map distributions, overlaps, graphics, fog and save/reload coverage. Suggestions for future major slots belong in [IDEAS](../IDEAS.md), not this implemented-behavior reference.

## Landmark minerals and Mine effects

Current `src/map.cpp` mechanics: Garland Crater and Mount Planet grant **+1 mineral in their centre 3×3** (`code_at() < 9`); Mount Planet also grants **+1 energy** there. Fossil Field Ridge has +1 mineral in its special-coded tiles (`code_at() < 6`). These mineral bonuses are conditional on the landmark not being disabled.

For ordinary land, a qualifying **landmark mineral bonus**, normal mineral resource bonus, or independent **Ironstone** mineral bonus also increments the Mine modifier by **+1**. Thus the central Crater/Mount Planet tiles and Ironstone can gain +1 inherent mineral plus an *additional* +1 mineral from Mine compared to equivalent tiles without that feature. The actual difference is subject to Mine-without-Road restrictions and mineral output caps (currently `tile_output_limit=3,3,3` in the test INI). Mines are **not required** for the inherent +1 mineral. Projected `item_yield()` should not be treated as a comprehensive Mine evaluation; see TERR-003.
