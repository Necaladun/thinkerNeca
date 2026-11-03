# Independent minor-terrain graphics — Develop-only prototype

## Source artwork and atlas

`tools/make_minor_terrain.py` builds a three-sprite, 384x64 indexed PCX
atlas from **your locally installed `ter1.pcx`**. Pillow is required:

```bash
python tools/make_minor_terrain.py "/path/to/SMAC/ter1.pcx" "/path/to/SMAC/minor_terrain.pcx"
```

The script preserves the game's palette (index 253 is transparency),
uses 128x64 slots and extracts 104x56 sprites. Slots:

| Slot | X offset | Feature | Source |
| --- | ---: | --- | --- |
| 0 | 12 | Ironstone | Rust recolour of Uranium, preserving previous Ironstone art |
| 1 | 140 | Jungle | **Temporary stylised canopy marker**, generated in Pillow |
| 2 | 268 | Uranium | Native Uranium sprite, copied from `ter1.pcx` at (815,256,104,56) |

The repo **does not redistribute native game artwork**. The existing
128x64 `assets/minor_terrain.pcx` remains a legacy Ironstone-only
fallback (slot 0). It must be replaced with the generated three-type
atlas to draw Jungle and Uranium. The current Jungle marker is NOT
pixel-for-pixel native Jungle graphics; matching that texture still
requires further engine/artwork investigation.

### Deployment

The game loads `minor_terrain.pcx` from the **same directory as
`terranx.exe`**, not the working directory. Put the newly generated
384x64 file there. The Code::Blocks post-build copy now
installs the 128x64 legacy asset **only if no PCX exists in the patch
directory**; it will not overwrite a locally generated three-type atlas.

## Current rendering integration

Develop builds use Thinker's existing `mod_gen_map` wrapper and
`MapWin_tile_to_pixel` to draw registered minor sprites on explored
tiles. `src/gui.cpp` now uses a type-indexed atlas loader and shared
draw routine, instead of hard-coding Ironstone. Missing sprite slots
are diagnosed in `minor_terrain_render.log`; the old 128x64 asset
still displays Ironstone.

**This is not a final rendering implementation.** The sprites still
draw after the normal map pass, including partial redraws. That may
overpaint bases, improvements or units. The correct native terrain
stage has not been established. Release builds and PRACX/reduced mode
do not draw these independent sprites yet.

The tile inspector is also **not integrated**. Do not restore the
previous crashing `StatusWin::draw_status` shim or reassign native
landmark names.

## Verification needed on Windows

1. Regenerate a 384x64 atlas from the installed game's `ter1.pcx`.
2. Start a fresh modified-landmarks map with all three `minor_*` flags enabled.
3. Check visibility at normal/fogged zoom levels, base founding,
   terraforming, unit occupancy, and map panning.
4. Confirm the debug log finds three extractable sprite slots.
5. Complete terrain-stage rendering and inspector text before treating
   this as game-ready or merging the draft migration into `master`.

## Existing per-tile rendering callback — experimental A/B mode

The Thinker executable already replaces `MapWin_gen_overlays` at
`0x4688E0`. The new `minor_render_mode` option chooses between:

- `minor_render_mode=0` (default): existing post-`MapWin_gen_map`
  whole-map drawing. Known risk: incorrect layering / units-only redraws.
- `minor_render_mode=1`: draws each minor sprite inside the existing
  per-tile `MapWin_gen_overlays` callback. This avoids the repeated
  whole-map sprite scan and may integrate more naturally with native
  tile painting, but its order relative to units and improvements is
  **not yet established** without an in-game comparison.

No new binary hook, status-window wrapper or `LM_*` flag spoofing is used.
Both modes suppress overlay sprites on tiles containing bases/vehicles to
avoid covering unit artwork while rendering order remains uncertain.

For visual debugging, `minor_debug_tile_names=1` paints the definition's
name on a hovered map tile through the same supported per-tile drawing
callback (Develop only). This is **not** the native tile-inspector label,
and deliberately makes no claim to solve that outstanding integration.
