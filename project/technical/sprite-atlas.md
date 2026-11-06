# thinkerNeca indexed sprite atlas

> **Branch scope (2026-11-07):** This reference describes experimental terrain/rendering code on [draft PR #4](https://github.com/Necaladun/thinkerNeca/pull/4), **not yet merged into `master`**. The default branch still has earlier Ironstone and map behaviour; verify implementation against the working branch.


**Scope:** tested native resource-art pipeline on [draft PR #4](https://github.com/Necaladun/thinkerNeca/pull/4). See [rendering](rendering.md), [configuration](configuration.md) and [runtime tests](../TESTING.md).

## Current source format

- Local file `neca_sprites.pcx`: **1024×1024**, **8-bit indexed PCX**, 256-colour palette copied exactly from the installed `TER1.PCX` (not `palette.pcx`).
- Normal **visible** source indices for this graphics path: **0–235**. Source index **253** is the transparent background, index **252** another native mask. Both become runtime transparency **9** through native palette processing. Other high indices are reserved.
- The remaining space is reserved for later art. Adding images to this sheet does not automatically add native drawing hooks.

| Sprite ID | Atlas (x,y,w,h) | Local input |
| --- | --- | --- |
| `uranium_test` | `(16,16,100,62)` | Original `TER1.PCX` pixels at `(822,253,100,62)` |
| `titanium_test` | `(144,16,100,62)` | Prepared Titanium sheet `(12,4,104,56)` crop, pasted at atlas `(155,19)` to preserve tested alignment |

The current prepared Titanium art has 559 visible pixels, 41 used visible colour indices, background 253, and no 252/254 control pixels within its source crop. Its previous 104×56 crop omitted six edge pixels of the original Uranium shape; do not fabricate missing pixels.

## Rebuild the atlas locally

Use the user's own original game files and edited Titanium artwork:

```bash
python tools/build_neca_sprites.py "C:/SMAC/TER1.PCX" "C:/art/Titanium_minor_terrain_READY.pcx" "C:/SMAC/neca_sprites.pcx"
```

The builder checks source dimensions/indexed mode, exact palette equality, nonempty visible Titanium art, index-253 border and reserved colours. It reopens its output to verify pixel-for-pixel accuracy. Using the same original art and edited Titanium input reproduces the **exact PCX used in the successful test**. Changing visible Titanium indices in the allowed palette range and rebuilding changes artwork **without** editing the DLL.

The CI test `tools/test_neca_sprites.py` uses **synthetic source images**, not Firaxis art. The repository does **not** commit generated `neca_sprites.pcx` because it includes original-game Uranium pixels and user artwork; public DLL artifacts likewise do not bundle it. Transfer your local PCX privately (e.g. in a ZIP) when installing the matching Develop DLL.

## Native runtime interpretation

The loader applies +10 (mod 256) to source indices, converts decoded **6 and 7 → 9**, and extracts with key 9. Do **not** pre-offset visible PCX indices when editing raw artwork. The exact Uranium processed reference hash is `24A1B67A`; the tested Titanium processed hash `D4B3AC2C` is logged only and may change when art is edited.

Current hook coverage is **native Uranium sprite variant 0** (`0x78A700`). Variant 1 (`0x78A72C`) keeps the original Uranium appearance on Titanium. The 1024×1024 atlas and two 100×62 source slots are today's tested implementation, **not** engine-supported maximums or arbitrary-dimension guarantees. Future sprites will need registry entries, anchor/palette tests and a suitable native draw context before gameplay use.
