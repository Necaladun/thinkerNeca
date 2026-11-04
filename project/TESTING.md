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
| Independent minor storage / N/M/E logic | Synthetic v2 save parser; source contract; Jungle +1N, Uranium +1E, Titanium +1M confirmed by user | Real fresh-game save→reload→save; resource/improvement combinations |
| Jungle rendering | User reports correct appearance and +1 food; no graphics issues reported | Fog, redraw/focus, border cases, unsupported preimages |
| Uranium rendering | User reports correct sprite, +1 energy and Inspector label | Native resource coexistence, redraw/zoom/fog |
| Titanium rendering | +1 mineral and shared native Uranium placeholder appearance confirmed by user, including new-map generation. Previous custom renderer was offset. **Distinct blue-grey sprite substitution is now implemented but not in-game verified.** | Verify `TITANIUM DRAW HOOK ... active=1`, blue Titanium/normal Uranium, exact saved tile alignment, all zooms, fade/fog/units and no errors with missing PCX. |
| Native tile Inspector | Jungle/Uranium labels confirmed; **Titanium label intermittent** in latest user test (BUG-002) | Inspect same tile sprite vs registry data, click/focus/fog and composite feature cases |
| New world generation | User reports the shared Uranium/Titanium alternating generation appears correct in a new game. CI covers source invariants but not live map generation. | Broader map sizes/seeds, 3–9 tile patch statistics, two-tile separation, save/reload and other climate values. |
| Major landmarks | Great Dunes look good; Mesa's +1E centre-only verified. **Stepped 2,000 m Mesa visually failed:** screenshot shows bulging, non-flat summit; generator restored to original 5×5 plateau at 1,000–1,999 m and outer low rim. | Optionally verify restored flat-topped visual in a new game; other landmarks/improvement interactions remain. |
| Retired Canyon | Static source/INI/landmark-table guard passes | Observe random maps built with both generators |
| Configuration | Parser/INI structural CI passed; user reports current settings seem to work | Confirm local overrides, starting allocation/event exclusions and cap effects in play |
| Release build and PRACX | Not verified | Native Uranium/Titanium value-4 rendering is now common in source; Release runtime still unverified, Inspector remains Develop-only; PRACX/reduced mode not established |

**4 November 2026 in-game findings:** Jungle visually correct and +1 nutrient; Uranium visually correct, +1 energy, Inspector label present; Titanium +1 mineral but intermittent Inspector label and sprites sometimes drawn on non-Titanium tiles; Great Dunes look good; Sunny Mesa previously +1E on all 7×7 tiles (now corrected; user confirmed +1E central 3×3 only); probe-team issue **appears** resolved with `base_capture_fix=0`. These are user-reported observations, not completed regression coverage. See BUG-001, BUG-002 and MAP-005.

## Recommended practical regression pass

1. Create a new game with `new_world_builder=1`, `modified_landmarks=1`, all three minor types, `minor_native_*=1` and the chosen test INI.
2. Inspect Jungle, Uranium and Titanium with and without improvements, normal resource bonuses, fungus, rivers and overlapping unit/base artwork. Compare displayed N/M/E to base worker and crawler outputs.
3. Zoom from maximum out to close in; switch selection and map focus repeatedly; verify tile Inspector text and fog-of-war rules.
4. Save, close the game, relaunch the **same build**, load and inspect the same minor tiles; save again. Old-version save interoperability is **not** a requirement.
5. Check several seeds/map sizes and modified landmarks; Mesa's raised 2,000 m inner summit was **rejected** after a user screenshot showed a rounded bulge. On a fresh map, optionally confirm the reverted low 5×5 Mesa looks as before, with +1E only on the centre 3×3. Record seed and anomalies.
6. For the **distinct Titanium-art test**, deploy the compiled Develop DLL and the separately prepared user's `minor_terrain.pcx`. The CI-supplied generated PCX is only a placeholder. Run `python tools/prepare_titanium_sprite.py source.pcx clean.pcx` if starting from the original unmasked purple-V/cyan-guide atlas. Check `minor_terrain_render.log` for the unique draw-call hook; on failure it should retain the native Uranium sprite. Test Release separately.

## CI coverage

Workflow: [`.github/workflows/build-windows.yml`](../.github/workflows/build-windows.yml).

- Compile Develop DLL/launcher for 32-bit Windows.
- Generate/test indexed minor-terrain PCX atlas.
- Check minor-terrain source invariants, reserved major slot 10 and Mesa's centre-only energy plus restored original elevation source contract.
- Check distributed INI against supported parser settings.
- Decode a synthetic independent-minor SAV extension.
- Upload the DLL/EXE/atlas artifact.

CI does not run SMAC/X, validate native hook behavior on arbitrary executables, or render screenshots. When reporting a bug, include PR/commit, `thinker.ini` overrides, map seed/year, exact coordinates or screenshot, matching PCX and `minor_terrain_render.log` if relevant.

**After shared-geology refactor (pending Windows runtime):** Retest the 2101 save where stored Titanium tiles include (84,96), (83,97), (82,98), (81,99), (83,99), (84,100), (86,100). Old custom-sprite markers appeared one map coordinate northwest of most of these tiles; this must not be considered fixed until native rendering is observed at the recorded positions. A new game is needed to validate alternating patch generation.

**4 November, latest user observation:** Uranium and Titanium both appeared correctly using the *same* native Uranium glyph on the existing save; new-map generation also looked correct. This validates the shared placeholder only. The distinct Titanium sprite-pointer hook is a new unverified Develop experiment and must be checked with the provided prepared PCX.
