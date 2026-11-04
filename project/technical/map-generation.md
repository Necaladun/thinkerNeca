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

Additional modified features include changed Great Dunes geometry, resource distribution and a freshwater oasis. **These major-landmark edits are not yet covered by a full in-game regression pass.**

**Slot 10:** the old Canyon-shaped random terrain, name and economic bonus no longer generate, and the native WorldBuilder Canyon call is disabled. The original executable may still contain native Canyon graphics/editor interpretation for flag `0x200`; do not assign it to new terrain before implementing appropriate new rendering. No new landmark has been selected.

## Testing and balance

A new generation seed need not reproduce older builds if algorithms change. During the current testing pass, however, avoid RNG-affecting refactors without deliberate review, so seed comparisons remain meaningful.

See [TESTING](../TESTING.md) for map distributions, overlaps, graphics, fog and save/reload coverage. Suggestions for future major slots belong in [IDEAS](../IDEAS.md), not this implemented-behavior reference.
