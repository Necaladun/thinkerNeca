# Independent minor-terrain rendering — current implementation

**Reference document.** Actionable follow-ups: [work queue](../WORK-QUEUE.md); test matrix: [testing](../TESTING.md).

**Status:** Initial Windows Develop in-game verification succeeded for Jungle, Uranium, Titanium and their native tile-Inspector names. This remains draft work: regression coverage, Release parity and PRACX/reduced-mode support are incomplete. This document records the *current* implementation, not each historical rendering experiment.

## Rendering paths

| Feature | Current preferred rendering | Notes |
| --- | --- | --- |
| Jungle | Native `MapWin_gen_terrain_poly` terrain/texture stage | Independent Jungle flag is read through three x86 hooks |
| Uranium | Native resource sprite stage | Renderer-only `bonus_at() == 4` displays the original Uranium resource art |
| Titanium | Native resource stage, with a **guarded Develop-only Sprite* substitution** | Native position, scale, clipping and layering are retained. Uses first atlas slot; if hook/PCX unavailable, shows original Uranium sprite |
| Tile Inspector | Native feature-line append | Develop-only `strlen` callsite hook appends minor terrain names |

Uranium and Titanium both use the native renderer-only `bonus_at()==4` path. The user confirmed the shared Uranium placeholder aligned correctly on existing saves and newly generated maps. For distinct art, a **new narrow native sprite-substitution hook** changes only the Sprite pointer for Titanium immediately before a verified resource draw call; it does not compute screen coordinates. This new hook is **not yet verified in-game**. No custom replacement of `bonus_at` is installed.

## Native hooks and controls

- **Jungle:** `minor_native_jungle=1` installs three original-terrain-stage patches at `0x463CC5`, `0x463D93` and `0x464E78`. Disabled for reduced/PRACX mode.
- **Uranium and Titanium:** `minor_native_uranium=1` and `minor_native_titanium=1` independently permit the same renderer-only `bonus_at()==4` shortcut, only for supported native terrain calls and without an ordinary resource bonus. Normal gameplay/AI still receives 0–3. For Titanium, a **Develop-only guarded hook** now targets exactly `0x465E37`, the `Sprite_draw_3` call inside the resource type-4 branch following `bonus_at` at `0x465DFC`. An uploaded supported `terranx.exe` was disassembled to verify `cmp eax,4` at `0x465E0E`, the native Sprite pointer calculation at `0x465E2E`, and the direct `0x5E4B9A` call. All relevant instruction bytes are preflight-checked before patching; unsupported/modified bytes fail closed. The native wrapper preserves the draw arguments and replaces saved ECX at `+28` after saving registers (the previous code incorrectly wrote saved EDX at `+24`). The erroneous `EBP+0x0C == MapWin` check has been removed; `EBP+0x20/+0x24` provide tile x/y. No `MapWin_tile_to_pixel` calculation is used. The callsite and register contract have now been verified by offline disassembly of the user's supported terranx.exe, but runtime rendering/PCX behaviour still requires a new Windows test.
- **Inspector:** `minor_native_inspector=1` installs the Develop-only verified `strlen` hook at `0x4B7A0C` if its native five-byte preimage matches `E8 CF D9 18 00`. The Inspector is independent of geology rendering; intermittent labels require separate verification.

The Jungle byte-site preflight is not yet all-or-nothing and remains REND-001. Hook assumptions remain specific to the supported x86 `terranx.exe`; do not assume PRACX/reduced-mode parity.

## Atlas and deployment

`minor_terrain.pcx` is a 384×64 indexed atlas (Titanium, Jungle, Uranium slots). The custom Titanium artwork uses the first 128×64 slot; the renderer extracts its 104×56 region at (12,4). The original `ter1.pcx` source includes control indices **252** (purple V mask) and **254** (cyan atlas separator), which the native game does not show as ordinary painted pixels. The custom `Sprite_extract_2(...,0x1FD)` path uses transparent index **253**; prepare a user's unmodified pasted sprite with `python tools/prepare_titanium_sprite.py input.pcx output.pcx` to normalize only those two indices in Titanium's slot, preserving every ore pixel and the palette. This requires no user-side manual pixel editing.

**Deployment:** The compiled `thinker.dll` and user's prepared `minor_terrain.pcx` belong next to `terranx.exe`. CI's default atlas is a separately generated placeholder and **does not contain the user's recoloured original-game sprite**. Do not replace the prepared custom PCX with the CI placeholder when installing the DLL. Original SMAC art is not redistributed in the public repository.

`tools/make_minor_terrain.py` now copies the same placeholder artwork to Titanium and Uranium atlas slots for the optional fallback/diagnostic path. The late post-map overlay is not equivalent to the native rendering layer, and can obscure units or terrain if explicitly enabled. CI checks the atlas format but cannot validate the native game.

The native Titanium substitution hook remains Develop-only and intentionally fails closed if the known draw call is absent/ambiguous or the PCX cannot be loaded. Release retains the shared native Uranium placeholder pending REND-002. PRACX/reduced modes remain unsupported. Inspector remains Develop-only.

## Verification

**Previously observed:** The first hand-positioned custom Titanium sprite sometimes drew offset; subsequently, the user confirmed Uranium and Titanium both appear correctly using the shared native Uranium placeholder, including a newly generated map. **New and untested:** guard/preflight, sprite-pointer substitution, actual custom blue art, fog/zoom/partial redraw, normal bonus precedence, failure fallback, Release and PRACX.

## Diagnostics

The Develop build writes `minor_terrain_render.log` beside the game. For the verified supported binary, expect `TITANIUM DRAW HOOK bonus=0x00465DFC draw=0x00465E37 matches=1 target=0x005E4B9A active=1`, then `native Titanium sprite substitute` for affected tile coordinates when the blue PCX loads successfully. If any expected instructions differ, `active=0` and Uranium's original artwork remains. `active=1` alone proves hook installation, **not** successful asset loading or in-game alignment.

**Previous failing build (before executable disassembly):** log reported `draw=0x0046658B matches=15 active=0`; the broad scan matched 15 unrelated sprite calls. The corrected hook targets the disassembled type-4 resource branch directly. The old run never exercised sprite substitution. The separate Inspector installation line is `INSPECTOR HOOK native feature strlen ... active=1`.

Turn off the relevant `minor_native_*` setting to isolate a rendering issue; note that disabling a hook may expose the experimental late-overlay fallback rather than perfectly suppressing that feature. Record which DLL/PCX and map/save were used when reporting bugs. Once runtime testing is settled, remove superseded preview and reconnaissance code in a separate cleanup.
