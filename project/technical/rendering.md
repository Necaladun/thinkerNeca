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
- **Uranium and Titanium:** `minor_native_uranium=1` and `minor_native_titanium=1` independently permit the same renderer-only `bonus_at()==4` shortcut, only for supported native terrain calls and without an ordinary resource bonus. Normal gameplay/AI still receives 0–3. For Titanium, a **Develop-only guarded hook** first scans for the unique direct native `bonus_at` call and then precisely one downstream direct `Sprite::draw` call (known entrypoints). On one match it forwards the original call with the same arguments and **only ECX (Sprite pointer) changed**. It never calls `MapWin_tile_to_pixel`, and an absent or ambiguous native callsite leaves the working Uranium placeholder intact. The exact x86 draw-call layout and result require Windows runtime validation.
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

The Develop build writes `minor_terrain_render.log` beside the game. For custom Titanium expect `TITANIUM DRAW HOOK bonus=... draw=... matches=1 target=... active=1` followed by up to twelve `native Titanium sprite substitute` messages at matching tile coordinates. If `matches=0` or `matches>1`, the guard refuses installation: the game should still render the original Uranium placeholder and the log shows why. The existing Inspector installation line is `INSPECTOR HOOK native feature strlen ... active=1`.

Turn off the relevant `minor_native_*` setting to isolate a rendering issue; note that disabling a hook may expose the experimental late-overlay fallback rather than perfectly suppressing that feature. Record which DLL/PCX and map/save were used when reporting bugs. Once runtime testing is settled, remove superseded preview and reconnaissance code in a separate cleanup.
