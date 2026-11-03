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
`terranx.exe`). The runtime loader uses the relative filename
`minor_terrain.pcx`.

This hook is disabled in Release builds and when Thinker's `reduced_mode`
indicates PRACX, to avoid overwriting PRACX's hook at SMACX 0x466C70.
PRACX compatibility is not implemented in this proof of concept.

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
The entrypoint uses SMACX's per-tile hook documented by PRACX. If the build
fails, or a crash occurs during rendering, remove the experimental
`write_call(0x466C70, ...)` hook in `src/patch.cpp` and report the details.

Known limitations: only Ironstone is drawn; no yield modifiers or other minor
sprites; no PRACX integration; the presentation/scale may require adjustment
after actual in-game testing.
