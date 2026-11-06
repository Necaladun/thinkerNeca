# Testing and verification

> **Branch scope:** Most terrain evidence below relates to draft PR #4, **not** a merged feature on `master`. PR #5's Empire Report is also unmerged. Documentation-only integration has no Windows runtime test requirement; code changes still do.

**Updated 2026-11-07.** Runtime evidence is from draft [PR #4](https://github.com/Necaladun/thinkerNeca/pull/4), Windows 32-bit **Develop**. The user installed a matching DLL and a local 1024×1024 `neca_sprites.pcx`, with `minor_native_titanium=1` and `minor_titanium_sprite_mode=3`. The user's entire installed `thinker.ini`/`thinker_user.ini` has **not** been inspected; [docs/thinker.ini](../docs/thinker.ini) is the distributed test profile.

*CI/static* means source/synthetic-file checks. *User-verified* means observed in-game for that specific case. *Pending* does not mean broken. Compilation does not establish runtime rendering or save correctness.

## Test matrix

| Area | Evidence | Remaining checks |
| --- | --- | --- |
| UI-001 Empire Report (PR #5, parked) | **Windows user reports:** native Report and 12 views functional; Overview and genuine SMAC/X inset frames visible; six colour-separated summary groups approved visually; current Stockpile Energy links open Change Production; native populated/empty Queue Insert/Replace/Delete and Cancel/OK work. [PR #5](https://github.com/Necaladun/thinkerNeca/pull/5) source head `cbddd36` passed Windows Develop+Release [CI](https://github.com/Necaladun/thinkerNeca/actions/runs/42289432728), which does not execute the game. **This code is not merged into master.** | On rebased/retargeted final PR #5: test 1024×768 and high-res clipping; 40–60+ colonies, scrolling and correct base clicks; zoom-independent text; all tabs and action routing including Hurry/Gov/modal focus, fullscreen/windowed where supported; new-game same-build save/reload/continue. Game-load-while-open quirk accepted. UI-002 future event report is out of scope. |
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

## Combined PR #4 + PR #5 Windows Develop smoke report — 7 November 2026

**User-reported, not instrumented validation.** The user installed [Develop CI artifact for PR #5 head `3c08b52`](https://github.com/Necaladun/thinkerNeca/actions/runs/42293858129), which contains PR #4 terrain code plus PR #5 Empire Report UI. Both x86 Develop and Release cross-build jobs passed; only **Develop** was tested in the game. This user confirmation covers the six test areas previously supplied with the combined build:

1. **Same-build fresh-game save / close / reload / advance:** reported passing.
2. **Native Empire Report:** reported passing for tabs, base/production actions, scrolling and navigation; earlier explicit Queue Insert/Replace/Delete and Cancel/OK evidence remains valid.
3. **Independent terrain graphics:** reported appearing correctly for Jungle, Uranium, Titanium, standard resource art and map redraw/fog situations in the tested session.
4. **Tile Inspector:** reported displaying correctly in the tested session; previous intermittent BUG-002 has not been isolated or proven impossible.
5. **New-map generation:** reported satisfactory for the seeds/settings tried; exact seeds, sizes, climates and deposit counts were not supplied.
6. **General compatibility/regressions:** reported passing, including the requested UI layout/interaction checks; the test report did not itemise exact colony counts, display modes, or probe capture scenarios.

**Evidence boundaries:** No separate log, coordinate list, screenshots, exact map-seed matrix, independently inspected effective INI or isolated failure reproductions accompany this confirmation. This is strong positive broad *Windows Develop* acceptance, not source-level proof of each original edge case or any Windows Release/PRACX validation. Specifically:
- **BUG-003:** no visible problem was reported in this pass, but procedural resource icons on top of deposits require an explicitly identified overlap to close confidently.
- **BUG-002:** no visible Inspector problem was reported; intermittent labels remain monitor/repro until isolated or sufficiently targeted to close.
- **REND-001:** normal supported-game path passed at smoke-test level; forced mismatched-preimage, all-or-nothing patch negative test remains for an engineering harness.
- **REND-004:** no variant-1 sprite geometry log or alternative artwork has been supplied. Variant 1 deliberately retains native Uranium art.
- **REND-002:** Release binary still omits native Titanium artwork and Inspector hooks; build success is not feature parity.
- **BUG-001:** a general regression pass does not prove `base_capture_fix=1` safe; keep `base_capture_fix=0` workaround until specific probe/mind-control tests.

**Integration gate:** this is evidence for the *existing* PR #5 binary `3c08b52`, not for a future merged/rebased DLL. Rerun a short Windows regression on the final integrated commit before closing either PR.

## Follow-up graphics tests and design reference — 7 November 2026

**Tested build:** PR #5's combined PR #4 + PR #5 Windows x86 Develop artifact `3c08b52`; user confirmed relevant INI options. This is **Windows user evidence**, not a CI/runtime instrumented result.

- **Renderer installation and atlas:** `TITANIUM DRAW HOOK bonus=0x00465DFC draw=0x00465E37 matches=1 target=0x005E4B9A active=1`. `NECA SPRITE` log shows 1024×1024 indexed atlas, 7→9/6→9 mask conversion, Uranium reference hash `24A1B67A` verified and Titanium hash `D4B3AC2C` verified. Both extracted 100×62, transparency key 9.
- **Variant coverage:** four `TITANIUM SPRITE TEST mode=3 variant=0 swapped=1` entries at (35,41), (31,43), (33,43), (35,43). Native source pointer `0078a700`, geometry 100×62 / offset 9,2 / pitch 82 / key 9. Selected Titanium 100×62 / offset 11,3 / pitch 80 / key 9. **No variant-1 draw observed.** REND-004's variant-1 geometry remains unknown, not defective by inference.
- **Inspector:** native strlen-hook `active=1`; user cannot reproduce earlier label omission. This specific log contains **no** `INSPECTOR HOVER` or `INSPECTOR APPEND` entries. Preserve intermittent BUG-002 as monitor/repro.
- **Ordinary resource on independent geology:** user explicitly found a naturally generated resource overlapping Uranium/Titanium; normal bonus sprite **replaces** geology glyph, unlike Jungle (native texture). Both mechanical yields and the tile viewer still register both. **Original BUG-003 precedence error is corrected and Windows-confirmed.** User now requests **two visible glyphs**, resource **above** geology: independent REND-005, not yet implemented.
- **Visual precedent:** user supplied a screenshot of the **native Uranium Flats major landmark on the default Planet map**, whose landmark terrain remains apparent with a normal resource icon over it. Treat as a visual reference for REND-005. Native major-landmark rendering differs from the present single resource-slot Uranium/Titanium minor art; screenshot does not verify code reuse.
- **Atlas fallback:** with mode 3 and `neca_sprites.pcx` absent, Titanium appears as native yellow Uranium with **+1 mineral** still functional. Develop fallback passes; not Release parity.
- **Jungle:** positive-path appearance passes; three-site unexpected-byte **negative preflight** remains engineering-only. **Probe/base capture** comparison and **Release** runtime parity were explicitly deferred by user; keep `base_capture_fix=0` workaround.

**For REND-005 acceptance:** test naturally generated *and* editor-placed resources over both geology types; distinguish both icons visually rather than only rendering the upper glyph; confirm overlay order, native zoom/clipping/fog, movement/bases, reload, fallback and actual yields. New code requires a new DLL and in-game verification. Existing UI-001 Empire Report acceptance is unaffected.

## Additional user observations — 7 November 2026

- In a separate game, independent terrain and associated yields remain present after saving and reloading. This is **user-reported runtime evidence**; no before/after save pair was supplied for independent structural comparison.
- Several additional rainy/dry maps looked satisfactory. Jungle abundance changed substantially with rainfall, as intended; geology distribution appeared normal. No exact seed, map-size or climate matrix was supplied.
- Geological placement does not read the erosion setting directly, but its current source favours lower terrain elevations. Erosion can therefore have an *indirect* effect by changing eligible terrain; this is not currently a separate implemented geological erosion rule.
- **Evidence correction:** the subsequently attached SAV was identified by the user as the **wrong save**. Exclude it entirely from PR #4 testing, resource-overlap regression, map-size/patch statistics, coordinates and persistence claims. The screenshot of native Uranium Flats on the *default Planet map* remains the separate REND-005 visual reference. No save-specific statistics or coordinates from that withdrawn attachment are accepted.

## Verified sprite trace — 5 November 2026

- **External Uranium mode 1:** `NECA SPRITE processed name=uranium_test corner=9 transparent=5635 visible=565 hash=24A1B67A expected=24A1B67A verified=1`. Extracted `100x62 offset=9,2 pitch=82 key=9`; native reported the same geometry, offsets, pitch and key, with `variant=0 swapped=1`. User confirmed visual equivalence.
- **Blue-grey Titanium mode 3:** `NECA SPRITE processed name=titanium_test corner=9 transparent=5641 visible=559 hash=D4B3AC2C expected=00000000 verified=1` and extracted `100x62 offset=11,3 pitch=80 key=9`. User then switched from mode 1 to 3 and confirmed distinct Titanium art appeared correctly, and zooming in/out was fine; mineral yield still +1M. The Titanium pixel hash is **logged**, not hard-coded for future art edits.
- This verifies the **variant-0 native-stage PCX pipeline** for these prepared sprites. It does not prove other art dimensions or native variant 1. Earlier pink/translucent attempts were caused by incorrect indexed-processing assumptions; the current loader reproduces native +10 palette offset, decoded 6/7→9 masks and key-9 extraction.

## Reported resource-icon overlap — 5 November 2026

The user reports editor-placed resource bonuses visually replace Titanium/Uranium icons while retaining independent yields, but naturally seeded resources can be visually obscured by the geological art even though the Inspector and both yield effects remain. Forest/fungus naturally render beneath mineral art; roads and Forest descriptions coexist in the Inspector; independent Jungle textures continue to show beneath resource icons. Code inspection confirmed `mod_bonus_at` previously checked only `BIT_BONUS_RES` before returning renderer sentinel `4`, while the native resource routine also recognises *procedural* map-seed/coordinate bonuses without that flag. **BUG-003 source fix** now resolves the complete ordinary bonus (including procedural) before the geological sentinel; the 7 November Windows natural-overlap test confirms that the normal glyph takes precedence while both yields remain. User's subsequent two-glyph visual request is **new REND-005** work, not regression of BUG-003.

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


## Combat reactor normalization — source finding, Windows validation pending

**BUG-004:** The existing `ignore_reactor_power=1` setting removes effective reactor endurance advantages in conventional direct combat and weapon-versus-weapon artillery duels. Ordinary bombardment still subtracts unscaled damage from `10 × reactor` stored HP; this is a source-confirmed discrepancy, **not yet** a Windows reproduction or fixed feature. A user separately reported poor Spore Launcher performance versus Fusion units; this is consistent with BUG-004 but its cause has not been isolated.

| Target/test | Expected evidence |
| --- | --- |
| Otherwise identical Fission/Fusion/Quantum/Singularity defenders | Compare effective percentage HP loss and damage distribution under repeated identical artillery conditions |
| Conventional land bombardment | Check armour, morale, terrain, repair and the correct target reactor conversion |
| Spore Launcher vs conventional defenders | Ensure native psi-artillery attack does not inadvertently reward target reactor level |
| Conventional artillery vs native units | Preserve existing special native/psi handling; no false reactor advantages |
| Sea bombardment vs transport/non-artillery vessel | Check damage, 100% sea cap and destruction |
| Armed ship vs armed ship at range | Confirm weapon-versus-weapon duel path and existing reactor-normalised round damage |
| Bunker/base and open-ground targets | Check 50% / 99% damage limits and integer rounding near HP floors |
| `ignore_reactor_power=0` | Verify original reactor behaviour remains consistent if retaining this configuration path |
| Save/reload, visual damage, healing | Correct stored damage and recovery using same DLL and effective INI |

Record the exact unit prototypes, reactors, morale, tile, year, attack method, effective `thinker.ini`/`thinker_user.ini`, stored/effective HP before/after, DLL commit and screenshot/log. Source inspection, automated calculations, compilation and user observation are **distinct** evidence classes.

**BAL-004 (future, not yet tested):** If chassis HP, armour-priced Tanks or armour-contributing artillery duels are approved, compare individual battle odds, sustained engagements, repair/replacement mineral costs, land terrain bonuses, sea defence, unit-design AI and new-game save/reload. Do not mark hypothetical formula results as Windows-verified.

## CI and deployment

[PR #4 Windows workflow](https://github.com/Necaladun/thinkerNeca/blob/feature/minor-terrain-generation-migration/.github/workflows/build-windows.yml): Develop and Release compilation (separate jobs); synthetic tests for the local-input [sprite atlas builder](technical/sprite-atlas.md), old legacy placeholder and source masks; source/INI/slot-10/Mesa contracts and synthetic save reader. **The artifact distributes DLL and EXE only**, not user/original-game art or the old placeholder PCX.

Record the commit, effective INI including overrides, map/save coordinates and year, screenshot, PCX source and `minor_terrain_render.log` when investigating remaining problems.
