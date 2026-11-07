# Testing and verification

> **Branch scope:** Most terrain evidence below relates to draft PR #4, **not** a merged feature on `master`. PR #5's Empire Report is also unmerged. Documentation-only integration has no Windows runtime test requirement; code changes still do.

**Updated 2026-11-07.** Runtime evidence is from draft [PR #4](https://github.com/Necaladun/thinkerNeca/pull/4), Windows 32-bit **Develop**. The user installed a matching DLL and a local 1024×1024 `neca_sprites.pcx`, with `minor_native_titanium=1` and `minor_titanium_sprite_mode=3`. The user's entire installed `thinker.ini`/`thinker_user.ini` has **not** been inspected; [docs/thinker.ini](../docs/thinker.ini) is the distributed test profile.

*CI/static* means source/synthetic-file checks. *User-verified* means observed in-game for that specific case. *Pending* does not mean broken. Compilation does not establish runtime rendering or save correctness.

## Test matrix

| Area | Evidence | Remaining checks |
| --- | --- | --- |
| UI-001 Empire Report (PR #5, parked) | **Windows user reports:** native Report and 12 views functional; Overview and genuine SMAC/X inset frames visible; six colour-separated summary groups approved visually; current Stockpile Energy links open Change Production; native populated/empty Queue Insert/Replace/Delete and Cancel/OK work. [PR #5](https://github.com/Necaladun/thinkerNeca/pull/5) source head `cbddd36` passed Windows Develop+Release [CI](https://github.com/Necaladun/thinkerNeca/actions/runs/42289432728), which does not execute the game. **This code is not merged into master.** | On rebased/retargeted final PR #5: test 1024×768 and high-res clipping; 40–60+ colonies, scrolling and correct base clicks; zoom-independent text; all tabs and action routing including Hurry/Gov/modal focus, fullscreen/windowed where supported; new-game same-build save/reload/continue. Game-load-while-open quirk accepted. UI-002 future event report is out of scope. |
| Windows Develop | CI success on `4809cb7` (5 November 2026): x86 DLL/launcher cross-compilation plus synthetic sprite, save, config and source contracts | Clean installation and actual Windows runtime tests; compiler/CI does not execute the game |
| Minor storage/yields | **Current PR #4 source:** THMIN001 v3 multi-type data compiled and synthetic-parser checked on `8952ebe` ([CI](https://github.com/Necaladun/thinkerNeca/actions/runs/42314671873)); previous Windows checks below used **v2**. User confirms Jungle +1N, Uranium +1E, Titanium +1M on prior Develop. **5 Nov user test:** Titanium yield persists through save→load→turns→load, works with Mine and raised resource caps; Forest/road Inspector and built improvements/bases work. | Repeat explicit persistence for Jungle/Uranium; fog and crawler interactions; actual normal-resource icon precedence after BUG-003 fix |
| Jungle/Uranium graphics | User reports native sprites and resource effects working; `b76feed` preflights all three Jungle patch sites before modifying any | Check Jungle hook on the supported executable and forced unsupported preimage; fog/units, composite tiles and Release |
| Titanium custom sprite | **User-verified:** mode-3 blue-grey art, transparency, positioning, zoom-in/out and +1M work on variant 0. Save/reload and turn advance preserve art/yield; units, bases, improvements and Forest/fungus layering behave normally in tested cases. | Native variant 1, fog, clean-install fallback; BUG-003 naturally seeded resource icon precedence requires retest on new DLL |
| Inspector | Jungle/Uranium labels and some Titanium labels observed; Forest/road composite text works on tested Titanium. `INSPECTOR APPEND` logs actual native insertions. | Compare actual append vs hover for intermittent labels and focus/fog (BUG-002) |
| World generation | Independent Jungle/Uranium/Titanium appeared in new games | Several map seeds/sizes/climates and real save/reload; mirrored modes unsupported |
| Major landmarks | Great Dunes accepted; Mesa central +1E confirmed; raised Mesa visually rejected and reverted | Other majors/improvement interactions and restored low Mesa confirmation |
| Canyon retirement | Static slot-10/source/INI checks | In-game new-map smoke test under alternative builders |
| Configuration | Parser/INI structural checks, mode 3 selected by user | Local overrides and experimental economic balance |
| Release/PRACX | Release x86 DLL/launcher cross-compilation **passed** on `4809cb7`; custom Titanium/Inspector hooks remain Develop-only, PRACX unverified | Native art/Inspector parity, actual in-game checks, clean install and supported binaries |

## Multi-minor source regression and native-underlay plan (DEC-016; new source not yet Windows-verified)

**TERR-004 storage and mechanics:** exercise single-feature tiles and all pairwise combinations of Jungle/Uranium/Titanium, plus the three-way combination **through constructed/editor/test tiles only**. Fresh generated worlds must contain **zero overlap among the current three types** and retain existing native-major separation. Verify independent per-type code persistence after adding and removing another type; exact per-feature Inspector labels; stacked N/M/E effects once per feature, resource limits and ordinary bonus coexistence; Forest, Mine, Borehole, river, fungus, base square, crawler and future AI-yield paths. Include native-landmark-plus-minor data coexistence without altering native `MAP::landmarks`. Validate fresh-game generation, save→exit→reload→turn advance→save→reload using the *same* DLL and new versioned `THMIN001` format. Structural synthetic tests do not establish Windows runtime correctness.

**REND-005 layering:** compare independent Uranium alone to editor-painted native `LM_URANIUM`; compare explicit natural and editor resource overlaps, with resource glyph always above geology. Check Jungle + Uranium, Jungle + Titanium, Uranium + Titanium and all three on **constructed** tiles for no crashes, corrupted draw state or resource-overdraw; **dual-geology artwork need not be pretty or both visible**, provided behaviour is deterministic and ordinary resource icons are preserved. Exercise fog, reveal, movement/units/bases, scrolling/zoom/clipping, optional/missing `neca_sprites.pcx` and same-build save/reload. Validate only after the native Uranium rendering stage/call contract has been established and a new Windows build is available.

**Status:** TERR-004 source compiled successfully for both Windows x86 Develop and Release on `8952ebe`; the Develop static source contracts and synthetic v3 stacked-save parser test passed ([CI run 42314671873](https://github.com/Necaladun/thinkerNeca/actions/runs/42314671873)); the final PR #4 head `96977aba` also passed both jobs ([CI run 42315046405](https://github.com/Necaladun/thinkerNeca/actions/runs/42315046405)). **User Windows Develop v3 gameplay and same-build save→exit→reload→advance→save/reload checks now positive.** REND-005 still lacks its native rendering change. Existing Develop variant-0 and broader BUG-003 evidence also belongs to the previous single-type/v2 implementation.

## Initial PR #4 v3 Windows Develop gameplay check — 7 November 2026

**User-reported, fresh v3 Develop build from CI head `96977aba` (artifact recommended from workflow 42315046405).** The user started on a Uranium tile; the tile displayed normally and the +1 energy yield worked. The starting position also had **ordinary nutrient resource + independent Uranium**: only the nutrient resource icon was visible, while both yield effects and the Inspector labels appeared correct. This is **expected current renderer precedence** (REND-005 has not introduced geological underlay), not evidence that geology and resource art both render. User reports **no generated independent terrain overlaps** in the inspected world. This is a visual observation, not a seed-wide automated overlap count. **Follow-up user report:** save, exit, load, advance, save again and subsequent load all succeeded on this v3 build; inspected yields and Inspector labels survived. This is positive actual Windows same-build persistence evidence. **Further user confirmation:** Jungle and Titanium also persisted across this same v3 save/reload/advance cycle, including their inspected yields and labels. No exact tile coordinates or before/after save pair were supplied for structural comparison. Deliberately stacked minor tiles, independent per-type removal and some improvement interactions remain untested.

## REND-005 blue-grey Titanium underlay — user Windows positive test, 7 November 2026

**User-reported Windows Develop test on PR #4 build `34f736e`**: "Fixed. Blue titanium underneath the resource icon." This confirms the previously missing custom Titanium artwork is now visible **under the ordinary resource bonus icon**, in the same native Uranium-style underlay pipeline. In an earlier session the user separately confirmed Uranium and its overlapping normal resource art were simultaneously visible. The combination establishes the central two-layer visual behaviour and distinct geological colouring on the tested tiles.

**Evidence boundary:** the post-fix result is a user visual observation; no `minor_terrain_render.log` from `34f736e` was attached, so exact `REND005 UNDERLAY SELECT` fields (including `swapped=1`) are **not** independently verified for that build. The prior `6074192` trace proves the native pointer mismatch `0x78A700` vs `0x78A72C`, which the source fix accepts, and the `34f736e` workflow passed Develop/Release x86 compilation and synthetic/source tests ([CI run 42325377771](https://github.com/Necaladun/thinkerNeca/actions/runs/42325377771)). **Still unverified on this final binary:** fog/reveal, zoom/scroll/clipping, unit and base draw foreground order, native major Uranium, optional/missing `neca_sprites.pcx` fallback, same-build v3 save→exit→reload→advance, and forcibly stacked independent minor combinations. Keep REND-005 **WIP** pending applicable tests.

## REND-005 Titanium underlay native-pointer mismatch — Windows evidence, 7 November 2026

**Second user Windows Develop test on PR #4 `6074192`:** Titanium still appears yellow/native Uranium despite `minor_titanium_sprite_mode=3` and `minor_geology_underlay=1`. Uploaded `minor_terrain_render(2).log` establishes the exact cause:

- Native underlay installed (`REND005 UNDERLAY HOOK ... active=1`), the cached feature type is **3 (Titanium)**, and cached x/y agree with frame x/y on the sampled tiles.
- Every sampled Titanium `REND005 UNDERLAY SELECT` has `original=0078a700 expected=0`; the selector incorrectly required only `0x78A72C` and therefore rejected the actual native pointer.
- `canvas_palette=1 loader_palette=1 enabled=1 mode=3`; therefore configuration, available graphics canvas and tile classification are not the blocking conditions. `atlas_attempted=0 atlas_ready=0 swapped=0` confirms the custom atlas loader was never called, rather than an attempted load failing.
- All sampled native sprites have key 9, dimensions 100x62, offset 9,2, pitch 82. This matches the old verified working variant-0 sprite, not an unknown artwork format. The instruction bytes at the static callsite reference `0x78A72C`, but the selector receives `0x78A700` in this Windows callback; do not conflate the two observations or claim the reason for this discrepancy is established.

**Source correction prepared on PR #4:** permit exactly `0x78A700` and `0x78A72C` in the existing early-underlay native pointer guard and log the observed `native_variant` (0 or 1), while retaining the earlier cached tile type and palette readiness checks. No native hook address, renderer ordering, v3 save data, INI defaults or yields are changed. After successful CI, the new Develop DLL should print `REND005 UNDERLAY SELECT cached=3 ... variant=0 expected=1 ... atlas_attempted=1 atlas_ready=1 swapped=1` for eligible Titanium, then show blue-grey Titanium beneath normal resource icons. **Windows visual confirmation remains required.**

## REND-005 Windows test: dual-layer success, Titanium artwork regression — 7 November 2026

**User Windows observation, experimental Develop DLL at PR #4 commit `69e2b9a`:** independent geology **and ordinary resource artwork are now simultaneously visible**. This confirms the critical native Uranium-style underlay effect on the user's observed tiles, but the user reports **Titanium looks like native Uranium** after the change. No post-change reload/fog/zoom/missing-PCX matrix has been reported, so REND-005 remains WIP.

**Attached runtime log** `minor_terrain_render(1).log` confirms `REND005 UNDERLAY HOOK mask=0x00464D9C draw=0x00464DE3 active=1`, while both old resource Titanium hook and Inspector hook also report active. The Inspector identifies Titanium at (68,70), (67,71) and (66,72), yet the log contains **no `REND005 UNDERLAY SPRITE` lines and no successful custom Titanium extraction**. This establishes that the earlier draw was enabled but **does not establish why the selector returned its native art**. The old type-4 resource branch no longer draws independent geology when the underlay is active, so its verified blue-grey replacement path cannot compensate.

**Follow-up source fix requiring a new Develop runtime test:** use the earlier native landmark-mask hook's own tile classification to select Titanium at its immediately following native sprite draw, instead of silently testing coordinates drawn from the later type-4 resource-hook frame contract. Add bounded `REND005 UNDERLAY SELECT` logs *before* any selector guard and `REND005 UNDERLAY GEOMETRY` for attempted Titanium, capturing valid native pointer, canvas/palette readiness, atlas load and swap decisions. Prefer the already available map canvas as an art-loader fallback if the earlier call's first buffer lacks a palette. This is a source-level hypothesis until the new Windows log and visible Titanium art confirm it. Validate resource-over-geology on both types, native Uranium unaffected, variant-1 positioning, transparency, fog/zoom and save→reload/advance.

## REND-005 native underlay evidence and next Windows test — 7 November 2026

**Received user-uploaded live log** from a new Windows Develop game, captured by the bounded read-only `ddcca4e` native probe. It reports the original Uranium landmark predicate at `0x464D9C` (`AND ESI,0x80000008`), early Uranium sprite draw at `0x464DE3` (native pointer `0x78A72C`) and the separate type-4 resource icon draw at `0x465E37`. The existing mode-3 Titanium art was extracted and selected correctly on four normal tiles (`22,68`, `23,69`, `24,70`, `23,71`), variant 0, key 9. The native Inspector appended independent Uranium at `(70,86)`. This was **diagnostic evidence**, not evidence of both glyphs visible on one tile. A CI install step initially stalled for over 13 minutes; the user reran it in 46 seconds and obtained the successful Develop diagnostic DLL.

**New experimental REND-005 code on PR #4 (Develop only; source not yet Windows-tested):** `minor_geology_underlay=1` enables an exact-byte preflight of original Uranium predicate/early draw, preserves native `LM_DISABLE` in a local-register mask hook, substitutes external Titanium via the *earlier* native sprite draw, and disables the duplicate fake type-4 geology sentinel while leaving the normal resource algorithm unchanged. With missing artwork it falls back to native Uranium. The old renderer is retained if the preimage fails; `minor_geology_underlay=0` in `thinker_user.ini` disables the experiment on restart.

**Next Windows test:** install the new Develop build only after CI succeeds (the previous `ddcca4e` diagnostic build cannot test this fix). Confirm `REND005 UNDERLAY HOOK ... active=1` in `minor_terrain_render.log`. Inspect Uranium+nutrient and Titanium+normal bonus: both geological underlay and resource icon must be visible, with unchanged yields/Inspector. Check ordinary Uranium/Titanium tiles, original editor-painted Uranium, zoom/fog, bases/units, missing `neca_sprites.pcx`, then fresh-game save→quit→reload→advance. Variant-1 underlay geometry and artefacts are especially important. If drawing is displaced or the game crashes, set `minor_geology_underlay=0` and restart and preserve the log. **Do not count build/CI as runtime acceptance.**

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

## Editor-painted native Uranium precedent — 7 November 2026

**Correct reference save:** `uranium H'minee of the Caretakers, 2105.SAV`, user-supplied for REND-005 investigation. The *previously withdrawn 2101 SAV* remains excluded and must not be used as supporting evidence.

Decoded the actual native MAP records and `THMIN001` extension for the **160×160** 2105 map:

- At **(48,64)**, native `MAP::landmarks = 0x00400008` (including `LM_URANIUM=0x8`), while independent minor type/code is `0/0`; no explicitly placed `BIT_BONUS_RES` on that tile. User reports stamping Uranium using the Scenario Editor tile tool, not creating a **named Uranium Flats**.
- The editor-painted `LM_URANIUM` area connected to (48,64) contains **225** tiles (x 34–62, y 50–78); the editor sets the original `LM_URANIUM` flag irrespective of whether a named landmark is created. Therefore, the distinction is **named major landmark versus tile-level native landmark flag**, *not* an alternative independent feature encoding.
- At **(47,67)**, the MAP record simultaneously holds `LM_URANIUM=0x8` and an explicitly placed normal resource `BIT_BONUS_RES=0x400` (`items=0x00101403`); the native resource bonus lookup resolves **nutrient** for this tile. Independent minor type is `MINOR_NONE`. This is a precise **native Uranium + resource** regression location for checking the user's observed two-layer appearance.
- **Implementation hypothesis, not yet confirmed:** existing native `LM_URANIUM` rendering may furnish an underlay stage separate from the ordinary resource icon. Investigate the supported executable's native landmark predicates/drawing and preserve original draw ordering before introducing a new draw call. The saved bytes alone prove coexistence of tile flags and resource data, *not* which functions drew each glyph.
- Neither the SAV nor original game artwork is committed to the repository; evidence refers to the uploaded user-local save. REND-005 still needs an actual code implementation and a new Windows build for independent Uranium/Titanium.

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
