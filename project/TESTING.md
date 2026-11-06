# Testing and verification

**Updated 2026-11-07.** Runtime evidence for independent terrain primarily concerns draft PR #4 and the user's Windows x86 Develop installation. The native Empire Report (UI-001) is on draft PR #5. User-observed game behaviour, source inspection, CI builds and release validation remain distinct.

*CI/static* means source/synthetic-file checks. *User-verified* means observed in-game for that specific case. *Pending* does not mean broken. Compilation does not establish runtime rendering or save correctness.

## Test matrix

| Area | Evidence | Remaining checks |
| --- | --- | --- |
| UI-001 native Empire Report | **User-reported Windows:** original native report open/reopen, fixed font in prior build, 12 views/navigation and base scrolling; native Change, Queue including empty/populated entries and Insert/Replace/Delete plus Cancel/OK; Stockpile Energy Overview list and Change buttons; native SMAC/X inset art visible in screenshot; latest six-colour-group Overview appearance looks good. **Source:** PR #5 `cbddd36`; both x86 Develop/Release CI passed run `42289432728`. | **Still unverified on the *latest* build:** 1024×768/narrow layout, large-base-set row/scroll-target identity, map-zoom font regression, Hurry/Gov/modal focus/other dialogs, display modes/ALT+TAB where supported and new-game same-build save→reload→turn-advance. Accepted game-load-while-open quirk is not blocking. Verify no wrong-base actions after filtering/scrolling. Not required for UI-001: automatic post-upkeep report, historical completion records and optional Secret Project movies (UI-002). |
| Windows Develop | CI success on `4809cb7` (5 November 2026): x86 DLL/launcher cross-compilation plus synthetic sprite, save, config and source contracts | Clean installation and actual Windows runtime tests; compiler/CI does not execute the game |
| Minor storage/yields | Synthetic THMIN001 v2 saves; user confirms Jungle +1N, Uranium +1E, Titanium +1M. **5 Nov user test:** Titanium yield persists through save→load→turns→load, works with Mine and raised resource caps; Forest/road Inspector and built improvements/bases work. | Repeat explicit persistence for Jungle/Uranium; fog and crawler interactions; actual normal-resource icon precedence after BUG-003 fix |
| Jungle/Uranium graphics | User reports native sprites and resource effects working; `b76feed` preflights all three Jungle patch sites before modifying any | Check Jungle hook on the supported executable and forced unsupported preimage; fog/units, composite tiles and Release |
| Titanium custom sprite | **User-verified:** mode-3 blue-grey art, transparency, positioning, zoom-in/out and +1M work on variant 0. Save/reload and turn advance preserve art/yield; units, bases, improvements and Forest/fungus layering behave normally in tested cases. | Native variant 1, fog, clean-install fallback; BUG-003 naturally seeded resource icon precedence requires retest on new DLL |
| Inspector | Jungle/Uranium labels and some Titanium labels observed; Forest/road composite text works on tested Titanium. `INSPECTOR APPEND` logs actual native insertions. | Compare actual append vs hover for intermittent labels and focus/fog (BUG-002) |
| World generation | Independent Jungle/Uranium/Titanium appeared in new games | Several map seeds/sizes/climates and real save/reload; mirrored modes unsupported |
| Major landmarks | Great Dunes accepted; Mesa central +1E confirmed; raised Mesa visually rejected and reverted | Other majors/improvement interactions and restored low Mesa confirmation |
| Canyon retirement | Static slot-10/source/INI checks | In-game new-map smoke test under alternative builders |
| Configuration | Parser/INI structural checks, mode 3 selected by user | Local overrides and experimental economic balance |
| Release/PRACX | Release x86 DLL/launcher cross-compilation **passed** on `4809cb7`; custom Titanium/Inspector hooks remain Develop-only, PRACX unverified | Native art/Inspector parity, actual in-game checks, clean install and supported binaries |

## UI-001 resumed Windows smoke test (not yet executed on final merged code)

Run on the rebased PR #5 Develop build after PR #4/master documentation reconciliation; retain one save created under the same build. Use normal high-resolution mode and 1024×768 when available.

1. Open/close Empire Report repeatedly; verify six summary categories, all five main sections, twelve detailed views, native frames, no critical clipping and unchanged 17px font after zooming the map. The Intelligence view is an explicit placeholder.
2. Use a 40–60+ colony empire or equivalent test save: wheel/PageUp/PageDown/Home/End, first/middle/last rows, section switching and correct colony identity after scrolling. Check the Attention severity ordering and that current Stockpile Energy entries are not labelled completed-this-turn.
3. Open the **Building** Change Production picker, **Next/None** native Queue editor (including Insert/Replace/Delete and Cancel/OK), Hurry and Governor from correct colonies; check dialog focus, return behaviour and absence of delayed popups. Verify a change in a base other than the first listed updates the intended base.
4. Test keyboard/window focus with F4 and other native dialogs; where supported check fullscreen/windowed and ALT+TAB. Save a new game, close/reload using the same DLL and advance at least a turn. Confirm no production, treasury, research or report-state corruption.

**Non-blockers:** further aesthetic polish, explicit game-load-while-open auto-hide (user accepted existing quirk), Intelligence expansion and the event-driven Turn Report UI-002.

## Verified sprite trace — 5 November 2026

- **External Uranium mode 1:** `NECA SPRITE processed name=uranium_test corner=9 transparent=5635 visible=565 hash=24A1B67A expected=24A1B67A verified=1`. Extracted `100x62 offset=9,2 pitch=82 key=9`; native reported the same geometry, offsets, pitch and key, with `variant=0 swapped=1`. User confirmed visual equivalence.
- **Blue-grey Titanium mode 3:** `NECA SPRITE processed name=titanium_test corner=9 transparent=5641 visible=559 hash=D4B3AC2C expected=00000000 verified=1` and extracted `100x62 offset=11,3 pitch=80 key=9`. User then switched from mode 1 to 3 and confirmed distinct Titanium art appeared correctly, and zooming in/out was fine; mineral yield still +1M. The Titanium pixel hash is **logged**, not hard-coded for future art edits.
- This verifies the **variant-0 native-stage PCX pipeline** for these prepared sprites. It does not prove other art dimensions or native variant 1. Earlier pink/translucent attempts were caused by incorrect indexed-processing assumptions; the current loader reproduces native +10 palette offset, decoded 6/7→9 masks and key-9 extraction.

## Reported resource-icon overlap — 5 November 2026

The user reports editor-placed resource bonuses visually replace Titanium/Uranium icons while retaining independent yields, but naturally seeded resources can be visually obscured by the geological art even though the Inspector and both yield effects remain. Forest/fungus naturally render beneath mineral art; roads and Forest descriptions coexist in the Inspector; independent Jungle textures continue to show beneath resource icons. Code inspection confirmed `mod_bonus_at` previously checked only `BIT_BONUS_RES` before returning renderer sentinel `4`, while the native resource routine also recognises *procedural* map-seed/coordinate bonuses without that flag. **BUG-003 source fix** now resolves the complete ordinary bonus (including procedural) before returning sprite sentinel 4. This is source/CI-level reasoning only; actual Windows overlap behaviour must be retested with the new DLL.

## Remaining runtime regression procedure

Prioritise one normal Windows Develop test game before changing INI options or art.

1. **Same-build persistence (TERR-001):** on a newly generated map, record an exact Jungle, Uranium and Titanium tile coordinate; inspect each name, art and +1N/+1E/+1M contribution subject to resource limits. Save, close the game, reload with the identical DLL and INI, compare each tile, advance turns, save and reload again. The user has already reported visible persistence through repeated loads/turns; explicit post-load yield and cross-type checks remain.
2. **Native Jungle drawing (REND-001 positive path):** inspect irregular Jungle edges, seams and adjacent ordinary/fungus/forest tiles while zooming, panning and reopening map/base windows; retry after reload. The incompatible-instruction *negative* patch-preflight case is an **engineering harness/isolated executable test**, not something the user should attempt by editing `terranx.exe`.
3. **Titanium layering/redraw (TERR-001):** move a unit onto/off Titanium; settle a base if practical; compare revealed, fogged and re-revealed tiles, partial redraw, multiple zoom levels, and adjacent normal resource icons. Neither the sprite nor fog should paint incorrectly over units, bases or normal resource art.
4. **Inspector (BUG-002):** select/hover an otherwise plain minor tile, then one with Forest, road, fungus or other native feature text as available. Compare the actual displayed label with `INSPECTOR HOVER` and `INSPECTOR APPEND` records at the same coordinate; include zoom/focus changes. A log showing `INSPECTOR APPEND` is evidence of text insertion, not necessarily evidence that the UI displayed it.
5. **Sprite variant 1 (REND-004):** inspect several deposits while moving/zooming, then search `minor_terrain_render.log` for `TITANIUM SPRITE TEST mode=3 variant=1`. The logger captures four samples per variant, with native sprite dimensions/offset/pitch/key. If absent, report "not encountered"; do not invent dimensions or expect blue-grey art on variant 1 yet.
6. **Atlas fallback (REND-002):** in a disposable installation (preserving the working PCX), temporarily remove or rename `neca_sprites.pcx`, restart and confirm Titanium reverts to native Uranium without crashing; restore the PCX and restart for mode 3. Mode 0 is the PCX-free reference. The actual Release DLL currently compiles but intentionally excludes custom Titanium and native Inspector hooks, so Release graphics parity is **not** an ordinary user-pass condition until implemented.
7. **Generation/landmarks (TERR-002, MAP-001/002):** inspect at least three distinct fresh seeds (ideally varying map size/climate). Geological patches should be separate 3–9-tile land deposits, no independent feature should overlap a major landmark, and no new-map Nessus Canyon/slot-10 effects should appear. Check Dunes, Fossil Field, Borehole Cluster and Sunny Mesa where generated. Mirrored worlds currently skip independent features by design.
8. **Probe capture (BUG-001, separate):** only on a disposable save, check mind-control captures with the current `base_capture_fix=0`; do not turn the workaround back on in an ongoing campaign merely to test stack cleanup.

**Evidence handling:** `minor_terrain_render.log` is written beside the game executable and **truncated on DLL startup**; copy it before restarting when a repro occurs. Hover and append diagnostics are capped per process. For failures, supply DLL commit, effective INI (including `thinker_user.ini`), map size/seed/year, tile coordinates, what happened versus expected, and a screenshot or a copied log. Do not alter your original `terranx.exe` for negative hook tests.

## CI and deployment

[Windows workflow](../.github/workflows/build-windows.yml): Develop and Release compilation (separate jobs); synthetic tests for the local-input [sprite atlas builder](technical/sprite-atlas.md), old legacy placeholder and source masks; source/INI/slot-10/Mesa contracts and synthetic save reader. **The artifact distributes DLL and EXE only**, not user/original-game art or the old placeholder PCX.

Record the commit, effective INI including overrides, map/save coordinates and year, screenshot, PCX source and `minor_terrain_render.log` when investigating remaining problems.
