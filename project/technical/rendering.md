# Independent minor-terrain rendering — current implementation

**Reference document.** Actionable follow-ups: [work queue](../WORK-QUEUE.md); test matrix: [testing](../TESTING.md).

**Status:** Initial Windows Develop in-game verification succeeded for Jungle, Uranium, Titanium and their native tile-Inspector names. This remains draft work: regression coverage, Release parity and PRACX/reduced-mode support are incomplete. This document records the *current* implementation, not each historical rendering experiment.

## Rendering paths

| Feature | Current preferred rendering | Notes |
| --- | --- | --- |
| Jungle | Native `MapWin_gen_terrain_poly` terrain/texture stage | Independent Jungle flag is read through three x86 hooks |
| Uranium | Native resource sprite stage | Renderer-only `bonus_at() == 4` displays the original Uranium resource art |
| Titanium | **Exactly the same native resource sprite stage and placeholder art as Uranium** | Distinct independent terrain type and +1 mineral gameplay effect |
| Tile Inspector | Native feature-line append | Develop-only `strlen` callsite hook appends minor terrain names |

Uranium and Titanium both use the already loaded native resource glyph. The custom Titanium `bonus_at` wrapper and its hand-computed pixel offsets have been removed. This is intended to eliminate the prior systematic apparent (-1,-1) tile placement fault; **it is not yet verified in the game**.

## Native hooks and controls

- **Jungle:** `minor_native_jungle=1` installs three original-terrain-stage patches at `0x463CC5`, `0x463D93` and `0x464E78`. Disabled for reduced/PRACX mode.
- **Uranium and Titanium:** `minor_native_uranium=1` and `minor_native_titanium=1` independently permit the **same** renderer-only value-4 shortcut in `mod_bonus_at()`, only for the supported terrain renderer and when no ordinary resource bonus is present. All gameplay/AI calls still receive 0–3. No separate Titanium sprite hook is installed.
- **Inspector:** `minor_native_inspector=1` installs the Develop-only verified `strlen` hook at `0x4B7A0C` if its native five-byte preimage matches `E8 CF D9 18 00`. The Inspector is independent of geology rendering; intermittent labels require separate verification.

The Jungle byte-site preflight is not yet all-or-nothing and remains REND-001. Hook assumptions remain specific to the supported x86 `terranx.exe`; do not assume PRACX/reduced-mode parity.

## Atlas and deployment

`minor_terrain.pcx` remains a 384×64 indexed fallback atlas (Titanium, Jungle, Uranium slots). **With both native geological options enabled, the game draws its own Uranium sprite for both deposits and does not need the Titanium atlas artwork.** This avoids the external palette-conversion issue for the normal rendering path.

`tools/make_minor_terrain.py` now copies the same placeholder artwork to Titanium and Uranium atlas slots for the optional fallback/diagnostic path. The late post-map overlay is not equivalent to the native rendering layer, and can obscure units or terrain if explicitly enabled. CI checks the atlas format but cannot validate the native game.

The native Uranium/Titanium shortcut is no longer tied to the custom Develop-only Titanium wrapper. Release and PRACX/reduced modes are still unverified. The Inspector remains Develop-only.

## Verification

**Previously observed:** Uranium placed and drew correctly; Titanium +1M worked but the custom icon appeared on some non-Titanium tiles and the Inspector name was intermittent. **Pending:** same-save 2101 native placeholder alignment; game effects, fog/focus/redraw, existing resource bonus co-location; fresh-map alternate generation and 2-tile separation; Release tests.

## Diagnostics

The Develop build writes `minor_terrain_render.log` beside the game. Expected Inspector installation log includes `INSPECTOR HOOK native feature strlen ... active=1`. There are read-only native-instruction reconnaissance logs and limited cursor-hover examples as well. These are development aids, not prerequisites for each game.

Turn off the relevant `minor_native_*` setting to isolate a rendering issue; note that disabling a hook may expose the experimental late-overlay fallback rather than perfectly suppressing that feature. Record which DLL/PCX and map/save were used when reporting bugs. Once runtime testing is settled, remove superseded preview and reconnaissance code in a separate cleanup.
