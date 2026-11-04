# Testing and verification

**Owner of test evidence and coverage.** Updated 2026-11-04. Test build: draft [PR #4](https://github.com/Necaladun/thinkerNeca/pull/4) on Windows 32-bit **Develop**, using the matching `thinker.dll` and `minor_terrain.pcx`. The current test INI is [`docs/thinker.ini`](../docs/thinker.ini); local `thinker_user.ini` may override it.

## Evidence categories

- **CI/static:** compiler and scripted source/data checks. Does not mean the executable worked in-game.
- **User-reported pass:** tested in game, but only for the specific observed case.
- **Not checked:** unverified, not presumed broken or working.
- **Deliberately unsupported:** not an acceptance requirement, e.g. older saves/scenarios.

## Current matrix

| Area | Evidence | Remaining checks |
| --- | --- | --- |
| Windows Develop compilation | CI passes for current PR head before this documentation change | Re-run CI on modified head; clean local install |
| Independent minor storage / N/M/E logic | Synthetic `THMIN001` v2 parser; source contract; Jungle nutrient and Ironstone mineral user observations | Save→reload→save in a real new game; combinations with special improvements/resource bonuses |
| Jungle rendering | User confirmed native texture, joins and layering beneath improvements | Fog, redraw/focus, border cases, unsupported preimages |
| Uranium rendering | User confirmed native resource graphic | Native bonus-resource coexistence, redraw/zoom/fog |
| Ironstone rendering | User confirmed native-stage positioning and maximum zoom-out | Visual polish, overlapped units/bases/terrain and matching PCX on deployment |
| Native tile Inspector | User confirmed Jungle/Uranium/Ironstone names appear | Standalone/composite lines, repeated selection/clicking, fog and empty-feature cases |
| New world generation | User observed generated independent terrain, current INI launches/works | Multiple deterministic seeds, map sizes, rainfall, count and major/minor overlap |
| Retired Canyon | Static source/INI/landmark-table guard passes | Observe random maps built with both generators |
| Configuration | Parser/INI structural CI passed; user reports current settings seem to work | Confirm local overrides, starting allocation/event exclusions and cap effects in play |
| Release build and PRACX | Not verified | Ironstone/Inspector not wired in Release; PRACX/reduced mode not established |

The user's latest report, **"tested and it seems to work fine"**, is a general gameplay smoke test of the current configuration. It is **not** a pass for every pending case above.

## Recommended practical regression pass

1. Create a new game with `new_world_builder=1`, `modified_landmarks=1`, all three minor types, `minor_native_*=1` and the chosen test INI.
2. Inspect Jungle, Uranium and Ironstone with and without improvements, normal resource bonuses, fungus, rivers and overlapping unit/base artwork. Compare displayed N/M/E to base worker and crawler outputs.
3. Zoom from maximum out to close in; switch selection and map focus repeatedly; verify tile Inspector text and fog-of-war rules.
4. Save, close the game, relaunch the **same build**, load and inspect the same minor tiles; save again. Old-version save interoperability is **not** a requirement.
5. Check at least a few seeds/map sizes and the major landmarks that were modified; record RNG seed, map settings and any reproducible anomaly.
6. Test the actual deployed ZIP pair of DLL/PCX, not just source compilation. Separately assess Release when engineering catches up.

## CI coverage

Workflow: [`.github/workflows/build-windows.yml`](../.github/workflows/build-windows.yml).

- Compile Develop DLL/launcher for 32-bit Windows.
- Generate/test indexed minor-terrain PCX atlas.
- Check minor-terrain source invariants and reserved major slot 10.
- Check distributed INI against supported parser settings.
- Decode a synthetic independent-minor SAV extension.
- Upload the DLL/EXE/atlas artifact.

CI does not run SMAC/X, validate native hook behavior on arbitrary executables, or render screenshots. When reporting a bug, include PR/commit, `thinker.ini` overrides, map seed/year, exact coordinates or screenshot, matching PCX and `minor_terrain_render.log` if relevant.
