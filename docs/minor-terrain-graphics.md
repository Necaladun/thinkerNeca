# Independent minor-terrain rendering — current implementation

**Status:** Initial Windows Develop in-game verification succeeded for Jungle, Uranium, Ironstone and their native tile-Inspector names. This remains draft work: regression coverage, Release parity and PRACX/reduced-mode support are incomplete. This document records the *current* implementation, not each historical rendering experiment.

## Rendering paths

| Feature | Current preferred rendering | Notes |
| --- | --- | --- |
| Jungle | Native `MapWin_gen_terrain_poly` terrain/texture stage | Reads the independently stored Jungle type via three verified x86 hook locations and uses the game's loaded Jungle transitions; no `LM_JUNGLE` spoofing |
| Uranium | Native resource sprite stage | `mod_bonus_at()` supplies renderer-only bonus value 4 for independent Uranium with no normal resource bonus; normal AI/yield calls still see 0–3 |
| Ironstone | Custom indexed-PCX sprite at native resource stage | Develop-only wrapper at the uniquely located `bonus_at` callsite; uses native tile position, elevation/zoom adjustment and a smaller glyph at far zoom |
| Tile Inspector | Native feature-line append | Develop-only verified `strlen` callsite hook adds Jungle/Uranium/Ironstone to the River/Forest/Fungus line, including standalone minor terrain |

The features remain independent map-tile type/code records. Rendering does not allocate or overwrite native landmark flags, `#NATURAL` entries or major-landmark names. Resource calculations are documented in [minor-terrain-yields.md](minor-terrain-yields.md).

## Native hooks and controls

- **Jungle:** `minor_native_jungle=1` installs patches at `0x463CC5`, `0x463D93` and `0x464E78`, extending the native `0x80000004` checks to independently stored Jungle. It uses the game's existing Jungle artwork and transition logic. Disabled for `reduced_mode` / PRACX.
- **Uranium:** `minor_native_uranium=1` returns renderer-only `bonus_at == 4` for Uranium when the direct caller is within the supported native terrain renderer and the tile has no normal resource bonus. Normal bonuses take precedence. A Develop fallback can draw the atlas sprite in cases the native shortcut does not cover.
- **Ironstone:** `minor_native_ironstone=1` installs a Develop-only wrapper after finding exactly one call from the native terrain renderer to `bonus_at`. The wrapper preserves gameplay bonus results and draws the loaded Ironstone atlas sprite into the native renderer canvas. `minor_native_ironstone=0` disables this route.
- **Inspector:** `minor_native_inspector=1` installs a Develop-only hook at `0x4B7A0C`, only if the original five-byte call matches `E8 CF D9 18 00`. It appends a minor name to the native 256-byte `StrBuffer`, respecting its capacity and the tile's visibility. `minor_native_inspector=0` disables this route.

**Native-hook failure semantics differ:** the Ironstone unique-callsite and Inspector mismatch paths skip their hook; the current Jungle path instead uses `write_bytes()`, whose mismatch calls `exit_fail()`. It is therefore *not* an all-or-nothing soft fallback if a Jungle instruction site differs. Reworking Jungle validation before broader distribution is outstanding.

The hooking addresses and stack assumptions are specific to the supported 32-bit `terranx.exe`; successful compilation does not validate a different executable. Do not apply those hooks to arbitrary versions.

## Atlas and deployment

`minor_terrain.pcx` is a 384x64 indexed atlas with three 128x64 slots (Ironstone, Jungle, Uranium). The current native Jungle and ordinary native Uranium rendering do not require their atlas entries; **Ironstone does**.

- CI constructs a fallback indexed atlas from the repository's original 128x64 PCX. Its Ironstone art is regenerated as opaque rust-coloured outcrops. The other slots primarily serve legacy fallback/debug rendering.
- `tools/make_minor_terrain.py` can build an atlas using the player's locally installed `ter1.pcx`; the repository does not redistribute original game art.
- For the Develop build, install the `thinker.dll` **and** the atlas from the same build alongside `terranx.exe`. Replacing only the DLL can retain obsolete Ironstone art.
- `minor_native_jungle_preview=1`, `minor_render_mode=0|1`, and `minor_debug_tile_names=1` provide experimental Develop fallback/diagnostic rendering. Late overlays may cover improvements, bases or units, so they are not an equivalent production route.

The current Release build does **not** include the Develop-only Ironstone native wrapper, Inspector hook or fallback sprite painter. The Release packaging script also omits the atlas. Do not claim feature parity with the tested Develop build. The Jungle, Ironstone and Inspector hooks are skipped in PRACX/reduced mode; the Uranium renderer-specific shortcut is likewise not established as PRACX-compatible.

## Verification

**User-confirmed in-game:** native Jungle joins and normal layering, native Uranium sprite, revised Ironstone alignment/visibility at normal and maximum zoom-out, and Jungle/Uranium/Ironstone names in the native Inspector. Ironstone artwork remains provisional.

**Automated CI:** Windows 32-bit Develop compilation, static checks of hook locations/guarding and sprites, synthetic save parsing, and indexed atlas generation.

**Not fully tested:** normal-resource bonus co-location, standalone/composite Inspector lines under all UI states, fog-of-war, repeated click/focus/redraw, base and vehicle tiles, full save/reload, unsupported executables, Release/PRACX parity.

## Diagnostics

The Develop build writes `minor_terrain_render.log` beside the game. Expected Inspector installation log includes `INSPECTOR HOOK native feature strlen ... active=1`. There are read-only native-instruction reconnaissance logs and limited cursor-hover examples as well. These are development aids, not prerequisites for each game.

Turn off the relevant `minor_native_*` setting to isolate a rendering issue; note that disabling a hook may expose the experimental late-overlay fallback rather than perfectly suppressing that feature. Record which DLL/PCX and map/save were used when reporting bugs. Once runtime testing is settled, remove superseded preview and reconnaissance code in a separate cleanup.
