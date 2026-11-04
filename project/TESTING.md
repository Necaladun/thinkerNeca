# Testing and verification

**Updated 2026-11-05.** Runtime evidence is from draft [PR #4](https://github.com/Necaladun/thinkerNeca/pull/4), Windows 32-bit **Develop**. The user installed a matching DLL and a local 1024×1024 `neca_sprites.pcx`, with `minor_native_titanium=1` and `minor_titanium_sprite_mode=3`. The user's entire installed `thinker.ini`/`thinker_user.ini` has **not** been inspected; [docs/thinker.ini](../docs/thinker.ini) is the distributed test profile.

*CI/static* means source/synthetic-file checks. *User-verified* means observed in-game for that specific case. *Pending* does not mean broken. Compilation does not establish runtime rendering or save correctness.

## Test matrix

| Area | Evidence | Remaining checks |
| --- | --- | --- |
| Windows Develop | Compilation and source tests passed through the mode-3 commit `197426a` | Re-run for cleanup commit and clean installation |
| Minor storage/yields | Synthetic THMIN001 v2 saves; user confirms Jungle +1N, Uranium +1E, Titanium +1M | Actual same-build save→reload→continue; resource/improvement and crawler interactions |
| Jungle/Uranium graphics | User reports native sprites and resource effects working | Fog/units, composite tiles and Release |
| Titanium custom sprite | **User-verified:** mode-3 blue-grey art, transparency, positioning, zoom-in/out and +1M work on tested variant 0 | Native variant 1, fog, unit/base layering, partial redraw, clean-install fallback |
| Inspector | Jungle/Uranium labels and some Titanium labels observed | Earlier intermittent Titanium name, feature-line combinations, focus/fog (BUG-002) |
| World generation | Independent Jungle/Uranium/Titanium appeared in new games | Several map seeds/sizes/climates and real save/reload; mirrored modes unsupported |
| Major landmarks | Great Dunes accepted; Mesa central +1E confirmed; raised Mesa visually rejected and reverted | Other majors/improvement interactions and restored low Mesa confirmation |
| Canyon retirement | Static slot-10/source/INI checks | In-game new-map smoke test under alternative builders |
| Configuration | Parser/INI structural checks, mode 3 selected by user | Local overrides and experimental economic balance |
| Release/PRACX | Not verified | Release graphics/Inspector strategy and supported binaries |

## Verified sprite trace — 5 November 2026

- **External Uranium mode 1:** `NECA SPRITE processed name=uranium_test corner=9 transparent=5635 visible=565 hash=24A1B67A expected=24A1B67A verified=1`. Extracted `100x62 offset=9,2 pitch=82 key=9`; native reported the same geometry, offsets, pitch and key, with `variant=0 swapped=1`. User confirmed visual equivalence.
- **Blue-grey Titanium mode 3:** `NECA SPRITE processed name=titanium_test corner=9 transparent=5641 visible=559 hash=D4B3AC2C expected=00000000 verified=1` and extracted `100x62 offset=11,3 pitch=80 key=9`. User then switched from mode 1 to 3 and confirmed distinct Titanium art appeared correctly, and zooming in/out was fine; mineral yield still +1M. The Titanium pixel hash is **logged**, not hard-coded for future art edits.
- This verifies the **variant-0 native-stage PCX pipeline** for these prepared sprites. It does not prove other art dimensions or native variant 1. Earlier pink/translucent attempts were caused by incorrect indexed-processing assumptions; the current loader reproduces native +10 palette offset, decoded 6/7→9 masks and key-9 extraction.

## Remaining runtime regression procedure

1. Install matching Develop `thinker.dll` and user-prepared `neca_sprites.pcx`; set mode 3 in the effective INI, restart the game.
2. Confirm Titanium graphics, Inspector name and +1M at several zooms; distinguish the native second sprite variant, which still appears as Uranium.
3. Check ordinary resources, fungus, forests, bases, units, fog and partial redraw; record coordinates/screenshot and log on discrepancies.
4. Start a **new game**, save, quit, reload using the **same DLL**, compare tiles and yields, advance turns and save again.
5. Remove the PCX in a disposable install to verify native Uranium fallback; mode 0 is native reference, mode 1 is external Uranium reference, mode 2 is legacy-only.
6. Vary seeds and map sizes and inspect landmark overlaps separately.

## CI and deployment

[Windows workflow](../.github/workflows/build-windows.yml): Develop compilation; synthetic tests for the local-input [sprite atlas builder](technical/sprite-atlas.md), old legacy placeholder and source masks; source/INI/slot-10/Mesa contracts and synthetic save reader. **The artifact distributes DLL and EXE only**, not user/original-game art or the old placeholder PCX.

Record the commit, effective INI including overrides, map/save coordinates and year, screenshot, PCX source and `minor_terrain_render.log` when investigating remaining problems.
