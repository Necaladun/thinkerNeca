# Independent minor-terrain rendering — implementation

> **Branch scope (2026-11-07):** This reference describes experimental terrain/rendering code on [draft PR #4](https://github.com/Necaladun/thinkerNeca/pull/4), **not yet merged into `master`**. The default branch still has earlier Ironstone and map behaviour; verify implementation against the working branch.


**Code:** [draft PR #4](https://github.com/Necaladun/thinkerNeca/pull/4). **Evidence:** [testing](../TESTING.md). **Open work:** [queue](../WORK-QUEUE.md). **Artwork:** [sprite atlas](sprite-atlas.md).

## Native renderer integration

| Feature | Current path | Status |
| --- | --- | --- |
| Jungle | Native map terrain/texture stage using three guarded hooks | All-site preflight implemented on `b76feed`; Windows failure-path validation pending (REND-001) |
| Uranium | Original native resource glyph through renderer-only `bonus_at()==4` | User-tested, +1 energy separate |
| Titanium | Same native resource draw, with guarded `Sprite*` substitution | **Blue-grey custom art verified in-game for native variant 0**, with +1 mineral separate |
| Tile Inspector | Narrow native feature-line append (`strlen` callsite hook) | Develop tested, intermittent Titanium label still to investigate (BUG-002) |

The normal resource-bonus icon is intended to take precedence over a minor mineral feature; it occupies the same native resource sprite slot, so the geological icon is not simultaneously visible. A 5 November user regression found that the previous renderer sentinel obscured *procedural* resource bonuses (no `BIT_BONUS_RES`), while explicitly editor-placed resources already took precedence. `src/map.cpp` now evaluates the complete ordinary bonus function before considering geological renderer-only type 4 (BUG-003); this correction requires Windows in-game verification. Both underlying resource and minor-terrain bonuses remain available to yield calculations. No native major-landmark flag is spoofed for minor terrain.

**Native hook contract (supported x86 `terranx.exe` only):** Uranium bonus call `0x465DFC`, type-4 branch `0x465E0E`, verified original sprite draw at `0x465E37` targeting `0x5E4B9A`. `src/patch.cpp` checks expected instruction bytes before installing. The wrapper swaps the Sprite pointer in saved ECX at +24 and forwards its transparency colour index at the original draw argument, retaining original zoom, clipping, position and layering.

Native Uranium selects `0x78A700` (**variant 0**) or `0x78A72C` (**variant 1**). The external atlas currently substitutes **only variant 0**; variant 1/unknown sprite remains native Uranium, including on Titanium tiles. Develop diagnostics now log up to four samples **per native variant** (not merely the first 12 draws overall), ensuring a later-encountered variant 1 can report native dimensions, offsets, pitch and key for REND-004.

Jungle's current patch sites are `0x463CC5`, `0x463D93` and `0x464E78`. The installer now compares all three original six-byte instruction sequences before writing any; an unsupported/mismatched site returns without installing a partial hook. This has source-level coverage, not yet Windows failure-path verification. The Develop Inspector uses a guarded native `strlen` site at `0x4B7A0C`, without hijacking the status window or rewriting terrain flags. PRACX/reduced mode and Release custom-art parity are not established.

## PCX loading and extraction

The **local** `neca_sprites.pcx` is a 1024×1024 8-bit indexed image with the exact original `TER1.PCX` palette. The verified native processing sequence is:

1. `Buffer_load_pcx(&image, path, nullptr, 10, 236)` applies a +10 modulo-256 index offset.
2. `Buffer_change_color(&image, 0x107, 0x109)` and `(0x106, 0x109)` convert the raw source masks 253 and 252 (now decoded 7 and 6) to transparent index **9**.
3. `Sprite_extract_2` extracts a registered rectangle with key **9**. The native draw call receives the selected sprite's matching colour key.

The Uranium reference at `(16,16,100,62)` retains a strict processed FNV-1a hash `24A1B67A` (5635 transparent / 565 visible pixels). Titanium at `(144,16,100,62)` validates a real nonempty sprite and transparency but only **logs** its checksum; the tested artwork yielded `D4B3AC2C` and 559 visible pixels. This means later colour edits do not require recompilation.

**Failure behaviour:** absent/invalid PCX, failed validation/extraction or unsupported native sprite variant returns the original native Uranium sprite. This works for the tested compatible sprite dimensions; differently-sized sprites or other render contexts have not been verified.

## Config and deployment

The distributed experimental [`docs/thinker.ini`](../../docs/thinker.ini) selects `minor_native_titanium=1` and **`minor_titanium_sprite_mode=3`**, matching the user's successful Windows Develop art test. The compiled default when the mode entry is missing is `0`.

| Mode | Behaviour |
| --- | --- |
| `0` | Original Uranium on Titanium, no PCX required |
| `1` | External Uranium reference from `neca_sprites.pcx`; user-verified variant 0 |
| `2` | **Legacy** experimental `minor_terrain.pcx` Titanium; not recommended |
| `3` | **User-verified** blue-grey Titanium from `neca_sprites.pcx`; variant 0 only |

Modes 1 and 3 require a user-produced PCX next to `terranx.exe` and a matching Develop DLL. The public Git repository and CI artifact contain **no copyrighted game art or user-created PCX**. See [sprite-atlas.md](sprite-atlas.md) for a reproducible local build tool, mask rules and coordinates. `thinker_user.ini` can override mode 3.

## Retained legacy and diagnostics

When all three native rendering switches are set, the Develop redraw path now bypasses the old full-map overlay scan. The early hand-positioned Titanium overlay and broad draw-call scan were superseded by the current native-stage hook. The old `minor_terrain.pcx` mode-2 loader and optional late overlay still have runtime and test dependencies, so they are retained temporarily under MAINT-001; do not substitute them for the verified custom-art path.

Develop logs `TITANIUM DRAW HOOK ... active=1`, `NECA SPRITE processed ... verified=1`, `NECA SPRITE extract ... ready=1` and `TITANIUM SPRITE TEST mode=3 variant=0 ... swapped=1` to `minor_terrain_render.log`. Obsolete read-only Inspector/native-mask hex reconnaissance at startup was removed; exact patch sites are known. A checksum match is preprocessing evidence, not proof of visual correctness.

The Develop Inspector helper now emits up to 24 `INSPECTOR APPEND` records for successful native label insertions, separate from hover-only samples, to isolate BUG-002.

**Observed in Windows Develop on 5 Nov 2026:** external Uranium matches native, custom blue-grey Titanium draws correctly with transparent mask and zoom-in/out, and +1M works. **Still open:** variant 1, fog/unit/base combinations, save/reload, Release/PRACX and intermittent Inspector names.
