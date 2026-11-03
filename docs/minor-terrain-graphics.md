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
name on revealed minor tiles at readable zoom levels through the same
supported per-tile drawing callback (Develop only). This is **not** the native tile-inspector label,
and deliberately makes no claim to solve that outstanding integration.

## CI test atlas (no local art required)

GitHub Actions now generates a **384x64 fallback atlas** from the existing
`assets/minor_terrain.pcx` and bundles it with the Develop DLL artifact.
That fallback preserves the existing Ironstone sprite pixel-for-pixel,
draws the provisional Jungle canopy, and tints the Ironstone shape
yellow-green as temporary Uranium art.

This is appropriate for engine smoke tests without requiring Python or
`ter1.pcx` on the player's computer, but **does not yet reproduce native
Uranium or Jungle graphics**. To override it, run the generator using
the locally installed original `ter1.pcx` and copy the output alongside
the executable instead.

## Native Uranium sprite (new, no extra PCX)

`minor_native_uranium=1` (default) makes `mod_bonus_at` return the
native renderer's **display-only bonus 4** for an independent Uranium tile
when the direct caller belongs to `MapWin_gen_terrain_poly` (supported
32-bit executable at `0x4632D0..0x466FFF`). The original renderer then
draws the loaded `kSprSpecial` sprite at its usual depth, scale, and zoom.
Other callers — including resource yields and AI — still receive the
normal 0..3 result. **Neither native LM bits nor save data change.**

Where the tile already has a real native bonus, that bonus takes priority
and the legacy Uranium preview sprite is retained in Develop builds.
The default avoids drawing duplicate atlas Uranium on ordinary deposits.
Turn the option off to restore the previous atlas behaviour.

This technique uses a return-address boundary because the original
renderer calls `bonus_at` directly. It needs in-game verification on
the supported GOG `terranx.exe` that the return address falls within
that reconstructed function; the Windows compile check alone cannot
establish the runtime callsite. It is an incremental integration, not a
general mechanism for Jungle or arbitrary native landmarks.

## Native Jungle graphics preview (Develop only)

`minor_native_jungle_preview=1` now uses the original game's loaded
Jungle texture array at `0x789C28` (15 textures), the 256-entry
`RainTransition` table at `0x685484`, and the native textured polygon
blitter at `0x61A760`. Neighbour joins recognise both independent and
legacy Jungle, without altering the game map's native LM bits.

The coordinates are reconstructed using the native terrain polygon arrays
and `MapWin_get_corner_y_offset`, following the OpenSMAC C++ reconstruction.
The artwork is loaded from the game's `texture.pcx` by SMAC/X itself;
nothing needs to be extracted or redistributed.

**Not a production-ready terrain-stage hook.** This Develop-only
implementation runs through the existing late overlay callback and may
overpaint roads or improvements. Base and vehicle-occupied tiles are skipped.
The definitive integration must extend native Jungle recognition inside
`MapWin_gen_terrain_poly` at instruction-verified patch points.
No unverified binary patch has been added. Set the option to 0 to compare
with the previous PCX fallback.

## Native instruction reconnaissance (no game binary redistributed)

Develop startup now logs the location and a small surrounding byte window
for occurrences of the mask constants `0x80000004` (native Jungle) and
`0x80000008` (native Uranium) inside
`MapWin_gen_terrain_poly` (`0x4632D0..0x466600`).
Results go to `minor_terrain_render.log`.

This is **read-only instrumentation**. Matching a mask constant does
*not* prove that an instruction is safe to replace. Before adding an
insertion hook we still must inspect instruction boundaries, branches,
register/stack liveness and existing Thinker/PRACX patch overlaps. The
binary bytes are not copied into the repository; a developer can inspect
the short diagnostic log from the supported local executable.

## Independent Jungle at original native terrain stage (next integration)

`minor_native_jungle=1` (default) now installs three instruction-verified
trampolines in the native `MapWin_gen_terrain_poly` function at:
`0x463CC5` (current tile), `0x463D93` (neighbour transition), and
`0x464E78` (later neighbour pass). Each patches only the complete
6-byte `and edx,0x80000004` instruction. The expected original bytes
are validated *before* writing the 5-byte JMP plus NOP. Wrong versions
fail closed rather than patch unrelated instructions.

The helper returns an independent-Jungle match while the trampoline
retains the original register values and executes the original masked
landmark check. No native landmark flags or code bits are ever modified.
The game's original `RainTransition` texture/rotation selection then
runs unchanged, before forest, fungus, resources and improvements.

**Important:** This is the first executable-level native Jungle patch
and has not yet been run in-game. The compilation validates the ABI and
naked stubs, but runtime behaviour still needs testing. If the hook
fails at startup on an unsupported executable version, do not force
it on; use the previous build. For an explicit late-overlay A/B test
with the same compiled DLL, set `minor_native_jungle=0` and use
`minor_native_jungle_preview=1`. The late overlay remains Develop-only.

The trampoline is enabled only for MinGW i386 builds and does not hook
PRACX reduced-mode rendering.

## Native-stage Ironstone test (Develop only)

`minor_native_ironstone=1` (default) now seeks the unique native
direct `E8` call from `MapWin_gen_terrain_poly` to `bonus_at`
(`0x592030`), in the original function's range. Only if *exactly
one* target is found does Thinker redirect that call to a cdecl
thunk. A missing or ambiguous site installs **no hook** and retains
the old drawing fallback. The result is appended to
`minor_terrain_render.log` as `native Ironstone bonus_at site`.

The thunk preserves the native renderer EBP and argument stack, calls
the existing gameplay `mod_bonus_at`, and draws the current independent
Ironstone sprite into the **native renderer's destination Buffer** with
its existing terrain sprite position and native zoom factors. The
caller receives the original resource result. The current Uranium
special sentinel (4) is explicitly preserved even though the wrapper
changes the direct calling address.

The original terrain stage is after fungus and before subsequent
resource/improvement sprites and units; this avoids both late-overdraw
and units-only focus wipes. Native LM flags, resource values and
minor save extension are untouched.

**This has not been validated in the running game yet**. The source
has a guarded callsite hook, plus an explicit escape hatch:
`minor_native_ironstone=0` restores the old sprite renderer.
The original custom 104x56 art remains experimental; palette,
distinctness and final pixel alignment still require in-game review.

## Ironstone position and artwork correction (next test)

The first native-stage wrapper mistakenly drew at the incoming
`MapWin_gen_terrain_poly` arguments `px,py`. These are **tile polygon
origins**, not sprite origins. The original renderer adjusts by
`px -= 4*q`, `py -= average_corner_lift + 2*q`, where
`q=(zoom_unit(zoom)+1)/4`, before drawing its resource sprites.
The new Ironstone wrapper reproduces that adjustment using the
original zoom argument and corner elevations. This should fix the
apparent drift when zooming or focusing on a tile.

The old Ironstone artwork was a very sparse, translucent-looking
recolour of the native Uranium graphic. The atlas generator now draws
original, substantially opaque iron-rich mineral outcrops with angular
edges, distinct rust faces and a darker base/shadow; it works with the
local game's indexed palette or the legacy fallback palette.

**Deploy BOTH files:** `thinker.dll` AND the newly generated
`minor_terrain.pcx`, from the *same* CI build. Merely updating the DLL
will leave the previous ghostly asset in place.

`Sprite_extract_2` trims transparent margins; its readiness check now
accepts positive bounded width/height rather than requiring the
full source rectangle width. It retains the original sprite's trim
offsets and transparently blits at palette index 253.

Neither Jungle/Uranium's native graphics, terrain generation,
resource yields, nor the inspector are changed by this iteration.

### Coordinate-source hardening

The native sprite adapter now obtains its raw tile position through
`MapWin_tile_to_pixel`, which is the same coordinate conversion used by
the normal MapWin drawing path. It does not assume that the original
`MapWin_gen_terrain_poly` stack slots retain unmodified `px,py`
parameters by the resource stage. It then applies the native elevation/
zoom sprite adjustment. This avoids a fragile dependency on compiler
choices about parameter storage in the original executable.

## Minimum-zoom visibility correction

In-game testing showed Ironstone deposits vanished near maximum zoom-out.
The current native-stage wrapper explicitly rejected `zoom <= -8`, matching
the original engine's optional ordinary resource-icon cutoff. Independent
geological Ironstone is terrain and should **remain visible**. That cutoff
has been removed from the wrapper.

At zoom levels `-8` and `-6`, the adapter draws its existing custom
ore sprite at two-thirds of the native zoom-dependent sprite scale,
repositioning the sprite origin by the same ratio to preserve tile
alignment. This keeps clusters legible without larger overlapped ore
shapes obscuring the zoomed-out map. Other zoom levels are unaffected.

The user's Deirdre 2101 save (`THMIN001` v2, 160x160) was independently
decoded for diagnosis: a four-tile Ironstone deposit near (147,37)
occupies (148,38), (145,39), (147,39), (149,39); a seven-tile deposit
near (60,112) occupies (59,111), (61,111), (59,113), (61,113),
(60,114), (62,114), (63,115). The save and original game's art are
not committed to the repository. Runtime min-zoom visuals still need
confirmation by the user.

## Tile inspector: independent terrain in the Forest/Fungus feature line

The requested result is **not** an additional named landmark, nor a map
overlay. Independent Jungle, Uranium and Ironstone belong in the existing
terrain-feature text line where the original status panel shows
River/Forest/Fungus, preserving rockiness, rainfall, resource bonuses
and separately listed improvements.

OpenSMAC's reconstruction at `src/ui/maininfc.cpp` describes the native
`tile_words` logic; the original executable inlines it into
`StatusWin_draw_status` starting at `0x4B6570`. Importantly, the
native code can skip the entire feature-text draw when none of those
native features is present. Merely rewriting an existing nonempty line
would silently fail for *standalone* Ironstone/Uranium/Jungle tiles.

### Read-only instruction reconnaissance (first integration stage)

On Develop startup, `minor_terrain_render.log` now includes an
`INSPECTOR SCAN` section. The scan reads the original native
`0x4B6570..0x4B8890` code, records direct E8 calls to the game text
functions (`Buffer_write_l_3`, `Buffer_write_l_4`, `Buffer_wrap_2`,
`Buffer_write_l` and `Strings_get`), and records byte windows near
the native Forest, Fungus, River bit-mask constants. It writes **no
binary patches** and changes **no status panel drawing yet**.

We need this information from a supported local `terranx.exe` before
a safe minimal instruction detour can append the minor name to the
native feature buffer and force its display when that buffer would
otherwise be empty. The subsequent hook must preserve calling
conventions, source buffer capacities, fog-of-war/active-unit tile
selection, native line positioning, and the existing Thinker patch
sites at `0x4B6C44`, `0x4B6E4F` etc.

The earlier experimental implementation from commit `a3830d1fa2`
temporarily changed `LM_NEXUS` and `Natural[13]`; that crashed in
the running game and MUST NOT be resurrected.

This commit is intentionally a diagnostic prerequisite rather than
a claim of completed inspector integration. No new INI setting,
save format, gameplay yield or rendering graphics changes are needed.

The Develop cursor-hover path also exercises a *format-only* text
composer, logging `INSPECTOR HOVER ... sample_feature_line=Forest, Ironstone`
(or Jungle/Uranium) for discovered minor tiles. This confirms the
independent name lookup and intended comma-separated presentation; it
does not claim the native status panel has been altered.
