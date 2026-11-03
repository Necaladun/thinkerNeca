# Independent minor terrain — generator and yield framework

## Data model

Minor terrain uses a separate per-tile `MinorTerrainTile { type, code }`
record and **never requires** one of the original sixteen `LM_*` flags.
The optional `code` field allows a minor feature to carry per-tile shape,
strength, climate or other type-specific state.

`THMIN001` saves now use **version 2**, storing two bytes per tile. Version 1
extension saves are deliberately incompatible. Fresh maps and saves are the
supported development target.

The type registry in `src/savegame.cpp` defines the common resource effects.
Additional mechanics are handled in their relevant engine components, rather
than assigning arbitrary meanings to the minor `code` byte globally.

| Minor type | Common effect | Special generation |
| --- | --- | --- |
| Jungle | +1 nutrient | Early, climate-aware, moisture/rainfall-sensitive patches |
| Uranium | +1 energy | Late, small dispersed patches favouring lower elevations |
| Ironstone | +1 mineral | Late, dispersed patches favouring rocky/rolling terrain |

Jungle's previous Thinker patch algorithm is retained, including its size
distribution, independent RNG seed, latitude/moisture preferences, and
`minor_code % 121` pattern used by the rainfall algorithm. It also retains
its special random-resource-placement behaviour in `world_rocky()`.

Uranium retains its existing weighted 3–9 tile deposit sizes and independent
seed. Uranium and Ironstone currently use patch-local codes (0–8). Future
specialised geology generators can replace their placement strategies without
altering the shared terrain representation.

## Generation ordering and options

Set `modified_landmarks=1`, `jungle=0`, `uranium=0`. These old options
are no longer used to generate Jungle/Uranium in the modified world builder.

`minor_jungle=1` (default) invokes Jungle before the original major-landmark
loop, allowing the later `world_climate()` / `world_rainfall()` pass to read
the independent type/code. Major landmarks may replace occupied Jungle tiles;
the post-landmark cleanup explicitly removes the replaced minor type.
`minor_uranium=1` and `minor_ironstone=1` (defaults) invoke late generators
after final climate and rockiness computation.

One minor type per tile. The generators avoid major-landmark footprints and
other minor terrain; patches keep separation buffers. No minor generator
calls `new_landmark()`.

Without `modified_landmarks=1`, the original native Jungle/Uranium
generators remain available for vanilla-style world creation.

## Resource effects

Basic nutrient/mineral/energy bonuses feed `mod_crop_yield`,
`mod_mine_yield`, `mod_energy_yield`, and the independent AI `item_yield`
estimator. Native legacy `LM_*` effects remain available for existing
scenarios and unmodified world generation. Minor features are independent
from bonus resource icons. Standard output caps and improvement effects apply.

## Incomplete integration — do not consider this a game-ready release

- The separate Ironstone sprite is currently **Develop-only** and drawn after
  the native map pass; layering/zoom require in-game verification.
- Native Jungle and Uranium sprites **will not automatically render** for
  tiles using independent minor IDs. The shared minor-terrain sprite atlas
  and proper draw-layer integration must be completed.
- The native tile inspector does **not** yet display independent minor names.
  A prior speculative status-window hook caused crashes and was removed.
- The map editor still has native Jungle/Uranium tools and `#NATURAL` entries.
  Do not reuse original landmark flags or graphics yet.
- Windows DLL compilation, in-game generation, yield, fog, and save/reload
  tests have **not** been performed in this environment.

The source migration is suitable for continued implementation on a feature
branch, not yet for replacing a working Jungle/Uranium game build.

## Verification checklist

1. Build Develop and confirm type/code save and reload with fresh maps.
2. Count Jungle, Uranium and Ironstone minor tiles; check that no generated
   minor feature has a native Jungle/Uranium landmark flag or name.
3. Compare Jungle rainfall and bonus-resource distribution against the
   previous Thinker generator; verify per-tile code influences moisture.
4. Verify +1 N/E/M from engine status yields and `item_yield`, respecting caps.
5. Check patch counts, lengths, distance buffers and major-landmark overlap.
6. Complete graphics, tile inspector and map editor before merging the entire
   migration into a playable default.
