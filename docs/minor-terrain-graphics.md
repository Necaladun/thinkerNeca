# Independent minor terrain graphics: Ironstone proof of concept

This is an experimental **Develop/Debug build only** feature. It uses the
minor-terrain byte array and versioned SAV/MP extension from earlier batches.
It does **not** modify original LM_* landmarks, tile resource yields, map
generator random sequences, or vanilla PCX sheets.

## Asset

* `assets/minor_terrain.pcx`: 128x64, 8-bit indexed PCX.
* Derived from an original `ter1.pcx` Uranium sprite region
  (source rectangle x=815, y=256, width=104, height=56).
* The recoloured sprite sits at x=12, y=4, width=104, height=56.
* Palette index 253 is the transparency key (extraction flag `0x1FD`).
* Source `ter1.pcx` is not distributed in this repository.
* Regenerate: `python tools/make_minor_terrain.py /path/to/ter1.pcx assets/minor_terrain.pcx` (Pillow required).

## Local deployment

Build the Code::Blocks **Develop** target. The post-build steps copy the DLL
and `assets/minor_terrain.pcx` into the project's `patch/` folder.

If running the game from another installation directory, copy **both**
`thinker.dll` and `minor_terrain.pcx` into the game directory (next to
`terranx.exe`). The runtime loader resolves `minor_terrain.pcx` relative to the game
executable's directory, independently of the working directory.

The Develop renderer routes the normal MapWin_gen_map calls through Thinker's
existing smooth-scrolling wrapper, including when `smooth_scrolling=0`.
Ironstone draws after the normal map pass. The optional debug-goals overlay
(`MapWin_gen_overlays`) is not used for custom terrain painting. No patch at
0x466C70 is needed. Drawing remains disabled in Release and PRACX/reduced mode.

## Verification

1. Generate a new game with distributed minor Ironstone or load a SAV produced
   after minor terrain Batch 2.
2. Reveal / inspect Ironstone tiles and confirm rust-red deposits display.
3. Pan, zoom, inspect fog of war and ensure units, improvements and bases
   remain visible.
4. Save/reload and confirm the same Ironstone tiles still display.
5. Move `minor_terrain.pcx` out of the game directory temporarily and
   ensure a new game still renders without the custom sprite.

Rendering is experimental and **has not yet been compiled or run in SMACX**.
This version reuses Thinker's known map redraw callsites. For a graphics
crash, revert the Develop-only MapWin_gen_map calls in `src/patch.cpp`
and report the details.

Known limitations: only Ironstone is drawn; independent Jungle and Uranium
now have type/code support on a migration branch but require atlas sprites
and native-style layered drawing before they can be considered game-ready.
No PRACX integration; presentation/scale require in-game testing.

The diagnostics are written to `minor_terrain_render.log` in the same folder
as the game executable (not necessarily the current working directory).
The custom PCX is also loaded from that executable folder. The normal map
render entrypoint, terrain scan, asset loading, and first draw are logged.

Zoom/redraw diagnostic: minor terrain is reapplied after **both** full and
`units_only` map passes in Develop builds. The log records up to 40 distinct
`map redraw zoom / tile-pixels / units_only` transitions. This is an interim
measure for the original map renderer; drawing over unit sprites or other
layering glitches are possible until the feature has a dedicated terrain
render stage.

If the log reports `PCX file not found/open failed`, copy
`assets/minor_terrain.pcx` from the repository into the **game installation**
folder containing the actual running executable, not merely the project's
`patch/` build output directory. Rebuilding the DLL is not required for this.

For a found PCX, the Develop log separately records file size/header,
`Buffer_load_pcx` return, image clipping dimensions, and `Sprite_extract_2`
return. A decoded image may use a DirectDraw surface with a null raw `data`
pointer; image dimensions now control whether extraction is attempted.
