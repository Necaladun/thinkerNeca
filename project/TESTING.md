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
| Titanium rendering | **User-verified:** mode 1 external Uranium visually matches native Uranium. Log confirms native +10 decode, both remaps success, hash `24A1B67A` with 5635/565 transparent/visible pixels, extraction `ready=1`, and native-vs-external Sprite matching 100×62, offsets (9,2), pitch 82, key 9. **New mode 3 Titanium remains untested in game.** | Test user's unchanged blue-grey Titanium in new local `neca_sprites.pcx` via mode 3; inspect `NECA SPRITE processed name=titanium_test`, `NECA SPRITE extract name=titanium_test`, `TITANIUM SPRITE TEST mode=3 ... swapped=1` on variant 0. Compare original Uranium, zoom, fog/units, fallback and save/reload. |
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

**User-reported custom-sprite hook failure (4 November 2026):** with updated Develop DLL and prepared Titanium PCX, both mineral types still displayed Uranium art. The `minor_terrain_render.log` line was `TITANIUM DRAW HOOK bonus=0x00465DFC draw=0x0046658B matches=15 target=0x005E4B9A active=0`. This shows the optional hook did not install due to 15 candidate native draw calls; it **does not** indicate a PCX failure. The diagnostic revision logs every candidate's address/target/nearby bytes to identify an exact callsite without hooking any of them.

**4 November — executable-level diagnosis:** User's supported PE32 `terranx.exe` was inspected (without executing or distributing the binary). Native `bonus_at` call is at `0x465DFC`; the `cmp eax,4` resource branch at `0x465E0E` leads to the exact `Sprite_draw_3` call at `0x465E37` (target `0x5E4B9A`). The hook's original saved-ECX write offset was correct (+24 after the helper arguments are removed). An intermediate correction mistakenly used +28; stack auditing caught and reverted this before another runtime test. The incorrect `EBP+0x0C == MapWin` check was removed. **This is source/disassembly verification only.** The latest DLL must still be run in SMAC/X to establish that a Titanium sprite actually loads, draws on correct squares and preserves fallback behaviour.

**4 November — Titanium sprite visible but pink background (user report):** The unique native resource-sprite hook now runs and shows the user's blue-grey Titanium art, but the overlay includes bright pink. The prepared indexed PCX retains ore colours and has 5,265 transparent-index-253 pixels in the active 104×56 extraction region. A later disassembly audit established `Sprite_extract_2` masks its colour-key argument to 8 bits, so `0x1FD` is actually equivalent to 253 at extraction. The initial explanation was incorrect; passing 253 alone did not remove the pink background. Subsequent work forwarded the selected Sprite's colour key at draw time. The external Uranium-control test isolates the unresolved sprite-processing mismatch.

**4 November — pink persists after first transparency correction:** The supplied prepared PCX's active 104×56 sprite region has 5,265 pixels at source index 253 and 559 ore pixels; the five source sprite corners sampled include transparent index 253. `Sprite_extract_2(...,253)` still showed pink in the user's game. An earlier audit incorrectly read `push 0xFD` at `0x45328E` as transparency; it is the Y source coordinate. The extractor's transparency argument is `push 0x109` (=9 at 8 bits). This was corrected after tracing the full native argument order. The latest Develop source uses `Buffer_get_pixel` to sample the *loaded* transparent pixel, passes it to the extractor, and forwards its post-load key in the guarded x86 shim. The same user's PCX should be retained. The runtime logs will distinguish actual index remapping from other failure modes. **No game verification yet.**

**4 November — pink removed, colours still wrong (user screenshot):** The in-game Titanium sprite now has transparent background but its ore pixels show cyan, bright blue, white and fragmented highlights rather than the supplied subdued blue-grey PCX. Palette inspection confirmed every palette entry of the prepared custom PCX is identical to the user's `ter1.pcx` (256/256); the original source artwork has not changed. The custom loader was still passing `canvas->palette` to `Buffer_load_pcx`, which can remap indexed pixel values. The new test build passes `nullptr` instead to skip remapping of this already-native indexed PCX, retaining sampled transparency and the working sprite hook. **Awaiting Windows runtime visual confirmation.**


**5 November — external Uranium control added (unverified):** user supplied a local 1024×1024 `neca_sprites.pcx` atlas containing a direct indexed copy of original Uranium at (16,16,100,62). New Develop-only setting `minor_titanium_sprite_mode` defaults to native Uranium (0), tests the external source (1), or restores legacy PCX (2). No copyrighted original-game art is committed. Source contract checks exercise the mode registry, parser/default and fail-closed loader. The external copy may still contain special source indices 252 and 253 and thus is not guaranteed to match the native preprocessed sprite visually. No actual Windows runtime verification yet.

**5 November — native Uranium preprocessing audit, implementation awaiting runtime:** The user's external Uranium atlas was confirmed pixel-identical to TER1 source rectangle (822,253,100,62); earlier claims that Y should be 265 were incorrect. Disassembly shows native `Buffer_load_pcx(...,nullptr,10,236)` adds 10 to source indices modulo 256; global `Buffer_change_color(0x107,0x109)` and `(0x106,0x109)` convert source mask colours 253/252 (decoded to 7/6) into transparent index 9. Native extractor at `0x4532A7` receives key `0x109`. Standalone local computation from the existing supplied atlas yields FNV-1a `24A1B67A`, 5635 transparent and 565 visible pixels. The new mode-1 runtime loader verifies those values and fails closed, only substituting native variant `0x78A700`; variant `0x78A72C` remains untouched. **No in-game pass claimed until user retests.**

**5 November — external Uranium control successful (user visual test + log):** mode 1 with corrected loader looks identical to Uranium in-game. `NECA SPRITE processed name=uranium_test corner=9 transparent=5635 visible=565 hash=24A1B67A ... verified=1`; `NECA SPRITE extract ... result=0 ready=1 sprite=100x62 offset=9,2 pitch=82 key=9`. Draw hook active and multiple `variant=0 swapped=1` substitutions have exactly matching native sprite dimensions, offsets, pitch and key. This verifies this sprite and path, **not arbitrary artwork**. New mode 3 uses the same pipeline with the user's earlier 559-pixel blue-grey Titanium sample; it is pending user visual validation. Atlas PCX is distributed privately as a ZIP and not committed to GitHub.
