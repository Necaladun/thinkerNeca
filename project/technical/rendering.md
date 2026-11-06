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

**Present renderer (Windows-confirmed 7 November):** a normal N/M/E resource glyph and independent Uranium/Titanium glyph compete for **one original resource sprite slot**. `src/map.cpp` resolves both procedural and editor-placed ordinary resources before the geology-only type-4 sentinel. User specifically found a natural overlap and confirmed the normal sprite appears while both geological and resource yields/Inspector attributes remain active; **BUG-003 original precedence problem is fixed**. Native Jungle remains visible because it is rendered as terrain texture rather than occupying the resource slot.

**New approved REND-005 (not implemented):** user wants both resource and independent Uranium/Titanium art visible simultaneously, **ordinary resource above geology**, with each visually legible. The user supplied a default Planet-map screenshot of the *native major-landmark Uranium Flats* where the landmark background and normal resource icon coexist; this is the desired **visual precedent**, not proof of a directly reusable minor-terrain hook. Reversing `mod_bonus_at()` priority would simply hide the ordinary resource. Existing Titanium Sprite-pointer substitution only executes on type-4 native resource draws, so can't by itself show another sprite when a normal resource occupies the slot. Source-level design must identify the safest additional native-stage draw/composition location, valid sprite/canvas parameters, z-order, transparency, coordinates, fog/clipping and variant cases. Avoid speculative native call arguments, post-unit overlays and spoofing landmark/resource flags. Keep mechanical yields unchanged and require Windows rendering validation before calling the feature implemented.

**Native hook contract (supported x86 `terranx.exe` only):** Uranium bonus call `0x465DFC`, type-4 branch `0x465E0E`, verified original sprite draw at `0x465E37` targeting `0x5E4B9A`. `src/patch.cpp` checks expected instruction bytes before installing. The wrapper swaps the Sprite pointer in saved ECX at +24 and forwards its transparency colour index at the original draw argument, retaining original zoom, clipping, position and layering.

Native Uranium selects `0x78A700` (**variant 0**) or `0x78A72C` (**variant 1**). The external atlas currently substitutes **only variant 0**; variant 1/unknown sprite remains native Uranium, including on Titanium tiles. Develop diagnostics now log up to four samples **per native variant** (not merely the first 12 draws overall), ensuring a later-encountered variant 1 can report native dimensions, offsets, pitch and key for REND-004.

Jungle's current patch sites are `0x463CC5`, `0x463D93` and `0x464E78`. The installer now compares all three original six-byte instruction sequences before writing any; an unsupported/mismatched site returns without installing a partial hook. This has source-level coverage, not yet Windows failure-path verification. The Develop Inspector uses a guarded native `strlen` site at `0x4B7A0C`, without hijacking the status window or rewriting terrain flags. PRACX/reduced mode and Release custom-art parity are not established.

## Native-editor Uranium baseline for REND-005 (source/save inspection)

The user supplied `uranium H'minee of the Caretakers, 2105.SAV`, with an area manually stamped using the Scenario Editor's old Uranium tile tool. The tool does **not** create a named Uranium Flats entry, but the saved MAP tiles **do** carry the original `LM_URANIUM` bit (`0x8`). At **(48,64)**, native landmarks are `0x00400008`, while `THMIN001` independent minor type/code is `0/0`. The contiguous painted native Uranium region contains 225 tiles. At **(47,67)**, `LM_URANIUM=0x8` and normal `BIT_BONUS_RES=0x400` coexist; the normal resource is nutrient according to `src/map.cpp::underlying_resource_bonus_at`. This is a concrete native editor reference for the observed ability to draw a resource above Uranium.

The old native `LM_URANIUM` flag is not our new `MinorLandmark::MINOR_URANIUM` storage. In particular, `src/mapgen.cpp::mod_world_diamond` uses `bit2_set(..., LM_URANIUM, true)` for legacy Uranium, whereas independent geology stores `THMIN001` per-tile type/code and deliberately leaves native LM flags untouched. **Source hypothesis:** hook the native Uranium landmark's *rendering-only* predicate or stage to additionally accept independent Uranium/Titanium, then use validated native resource-sprite substitution for blue-grey Titanium while leaving ordinary bonus sprites above it. The save confirms data fields, not the exact draw-stage call graph; disassemble/trace supported `terranx.exe` before selecting a patch. No global LM mutation, named-landmark spoofing, altered yields, or unvalidated Sprite calls.

## REND-005 architecture investigation: native flags versus minor terrain

**Source inspection, not an implementation decision (7 November 2026).** The user proposed reusing original unnamed per-tile `LM_URANIUM` behaviour and possibly defining `LM_TITANIUM`, while retaining the ability to introduce additional *named* major landmarks later. The different native limits matter:

- `src/engine_enums.h` defines `MaxNaturalNum = 16` (native Natural *types*) and `MaxLandmarkNum = 64` (separately stored *named landmark instances*). A generated or editor-painted native landmark flag **does not require** a `Landmarks[]` name record.
- `src/engine_types.h::MAP` has `uint32_t landmarks`; `lm_items() = landmarks & 0xFFFF` exposes the **16 low-order native type flags**; `code_at() = landmarks >> 24` supplies a tile-local pattern/position code (and high-bit `LM_DISABLE` shares that byte). Several middle bits already have engine uses, including `LM_UNK_200000`, `LM_UNK_400000`, `LM_UNK_800000`. Merely defining `LM_TITANIUM = 0x10000` does **not** create a seventeenth native renderer-recognised type: native code/masks would need auditing and patching.
- Mount Planet is the concrete multi-part example: `mod_world_volcano()` sets **one** `LM_VOLCANO=0x2` across its generated tiles, uses `code_set(...,i)` to distinguish positions, and optionally invokes `name_landmark()` (its `skip_label` argument is explicit). `MAP::volcano_center()` tests `code_at()==0`; `mod_mine_yield` and `mod_energy_yield` grant extra yields for `code_at()<9`. Multiple subregions/rules require **one** flag plus tile codes, not separate landmark identities.
- Original Uranium similarly uses `LM_URANIUM=0x8`; `mod_world_diamond()` sets it with `bit2_set` and only separately invokes `name_landmark()`. Scenario Editor painting can omit the name while retaining the native tile flag. This does **not** free any low-order type bit.

**Candidate A, repurpose native slot 10:** `LM_TITANIUM=0x200` could replace the fork's reserved former Canyon flag, and generation could write `LM_URANIUM` and `LM_TITANIUM` without creating names. This still uses a native 16-slot type position, **does not increase the count**, and would overturn DEC-003's explicit decision to reserve slot 10 for a future major. The unmodified `terranx.exe` may retain original Canyon graphics, editor interpretation and other calls even though modified generation/bonuses were retired. Merely changing a C++ enum would not provide Titanium art, minerals, Inspector semantics or guaranteed native underlay/resource layering. It would require an executable hook audit plus rewriting independent storage/yields/generation and same-build new-game save/reload tests; risks include old save flag collisions and unwanted `code_at()` interactions.

**Candidate B, genuinely more types:** retaining `THMIN001` (or introducing a separate explicit tile-type flag field) supports terrain categories **beyond** the 16 original low-order landmark flags, without consuming named major identities. This is already the implemented terrain storage. To render like editor-painted Uranium, patch the **native landmark underlay/render predicate** for these independent types, not the one normal resource-sprite slot; preserve real normal resource drawing afterwards and preserve existing Uranium/Titanium bonuses and Inspector. The actual native Uranium underlay predicate must still be verified against the supported `terranx.exe`; the save and current C++ source alone do not establish a safe patch site.

**Current engineering judgement:** verify native Uranium draw stages first, then compare the patch burden against the much larger 0x200/native-storage migration. Do not adopt Candidate A, consume slot 10, modify `DEC-002/003`, or change game rules until the native stage is identified and the user approves the architectural change. Scenario Editor UI controls are optional and not a merge dependency.

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
