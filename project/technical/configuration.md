# Configuration — current fork test profile

> **Merge status (7 November 2026):** The terrain/rendering implementation documented here was **merged into `master` via [PR #4](https://github.com/Necaladun/thinkerNeca/pull/4)**. Windows Develop gameplay smoke was user-verified; Release/PRACX art parity, long-run AI decisions and selected edge-case tests remain deferred. Historical branch-stage notes below are retained as development context.


**Authoritative shipped example:** [`docs/thinker.ini`](../../docs/thinker.ini). **Parser/defaults:** `src/main.cpp` / `src/main.h`. **Status:** user-selected test configuration, not upstream Thinker defaults or a settled balance release.

At startup, the fork reads `thinker.ini`, then optionally `thinker_user.ini` from the same game folder. The latter overrides ordinary single-value settings when present. Repeated options such as `skip_event`, `skip_faction` and `music_label` have special additive semantics, so they cannot always be undone by simply placing a different value in an override. To reproduce the tested profile, check both files.

## Currently intentional differences

| Setting | Test value | Why / status |
| --- | --- | --- |
| `base_capture_fix` | `0` | Probe-team problem appears mitigated in-game; temporary workaround, not a confirmed underlying fix (BUG-001) |
| `probe_action_fix` | `1` | Separate probe-team mechanic fixes remain on |
| `spawn_free_units` | `0,2,2,0,2,2,1,2,2` | Player-preferred additional starting Formers and scouts, including aquatic equivalents |
| `tile_output_limit` | `3,4,4` | **Experimental** N/M/E pre-tech caps; previous trial 3/3/3, original 2/2/2 (BAL-001) |
| `world_polar_caps` | `0` | Player preference |
| `skip_event` | `13`, `17` on separate lines | Exclude Sunspots and Base Asteroid Strike |
| `new_world_builder` / `modified_landmarks` | `1` / `1` | Intended modified-map path |
| `minor_jungle` / `minor_uranium` / `minor_titanium` | All `1` | Independent minor-terrain generation |
| `minor_native_jungle` / `minor_native_uranium` / `minor_native_titanium` | All `1` | Native Jungle and Uranium; Titanium has native-stage custom art in supported Develop mode |
| `minor_native_inspector` | `1` | Develop-only native feature-line labels |
| `minor_geology_underlay` | `1` (compiled default `0`) | Experimental REND-005 Develop-only Uranium-style geological underlay beneath normal resources; original-byte preflight; set `0` in `thinker_user.ini` for fallback. Core Windows Develop visuals accepted; optional rare compatibility checks deferred |
| `minor_titanium_sprite_mode` | `3` | **User-verified** local blue-grey Titanium atlas in Develop; compiled absent-option default `0` |
| `minor_render_mode` / `minor_debug_tile_names` | `0` / `0` | Prototype fallback mode selection; no debug tile-name overlays |

The older major-landmark options `jungle=0` and `uranium=0` **are distinct** from the independent minor settings. **`canyon=` is no longer recognised**; the retired native major slot 10 is reserved. Delete old `canyon=1` or `canyon=0` lines from *both* INI files to avoid parser warnings.

`modified_landmarks=1` and `new_world_builder=1` are required for all three independent minor types. Uranium and Titanium each retain an on/off switch but share alternating late generation. See [minor terrain](minor-terrain.md) and [CFG-001](../WORK-QUEUE.md).


## Combat-policy settings and known mismatch

- **`ignore_reactor_power=1`** is the established intended policy (DEC-012): reactor research retains cost advantages but must **not** increase effective unit endurance. The engine still stores `10 × reactor` HP; direct combat and artillery duels correctly scale damage. Ordinary bombardment fails to do so (BUG-004), pending a separate fix and Windows validation. This is a source behaviour defect, **not** a reason to change the setting to `0`.
- **`long_range_artillery=2`** grants additional artillery range to qualifying ships equipped with Heavy Artillery. Conventional positive-weapon ships can already perform artillery under `can_arty()`. It does not grant artillery ability to them in the first place. Naval direct attacks, ranged weapon-only duels and ordinary bombardment differ.
- **`sensor_defense_ocean=1`** enables qualifying sensor defence at sea under the distributed profile, but ships still lack land forest/rocky and anti-mobile-land ECM defence. AAA against aircraft and port facilities remain conditional exceptions.

See [unit combat and costs](unit-combat-and-costs.md) for current code paths and [BUG-004/BAL-004](../WORK-QUEUE.md) for correctness versus balance design. This section explains *existing* settings; no parser, compiled-default or distribution-profile changes are proposed.

**Telemetry trial (unmerged TEL-001 branch):** `telemetry_enabled=0` is the shipped and compiled default. Set `1` in the local `thinker_user.ini` to write optional per-session JSONL under `campaigns/`. The separate offline analyser is `tools/campaign_report.py`, with a Tkinter GUI at `tools/Campaign Analyzer.pyw` (copy both beside one another to the game's `campaigns/` directory). See [telemetry design](telemetry-automation.md); do not confuse this with `debug.txt` or developer Debug mode.

## Config maintenance

- Comments in `docs/thinker.ini` explain each supported setting; **avoid maintaining a second 170-option catalogue here**.
- Leave untouched values as they are while testing. Distinguish source-level mechanics from settings that merely activate existing code.
- `tools/test_thinker_ini_contract.py` checks names against the parser, expected array lengths, duplicates, unsigned shift widths and the retired Canyon key. It does **not** prove every value is sensible in play.
- `skip_facility` uses a 64-bit unsigned mask for parser IDs 1–64 and for its `src/build.cpp` facility-filter consumer (value 0 clears the mask). `skip_event`/`skip_faction` use 32-bit unsigned parser masks for IDs 1–32. This removes shift-width undefined behaviour; actual high-ID exclusions have not been tested in-game.
- The compiled `Config` defaults may differ from the distributed INI. An option missing from a local file normally inherits a compiled default, not necessarily `0`.
- A clean game installation and a local `thinker_user.ini` can produce different behaviour despite identical repository contents; always record overrides when reporting bugs.

## What `base_capture_fix=0` also disables

The setting gates multiple corrections in `src/base.cpp`, not just probe-team behaviour:

- **Captured-base drones/assimilation:** with the fix enabled, most newly captured foreign bases get a size/facility-dependent 20–50 turn assimilation penalty (modified by the REVOLT rule); without it, they generally revert to the original fixed 50 turns.
- **Probe capture mixed-stack handling:** with the fix enabled, `mod_capture_base()` removes units at the captured base if they are not the **old defender's** units and are not pacted with that old defender. This can include the attacking probe team or allied units of the *attacker*. Suspected source of the probe issue; needs a reproduction.
- **Refugee colony-pod relocation:** with the fix enabled, placement avoids nearby foreign non-pacted units in certain alien/human captures. Disabled by the workaround.
- **Cloudbase Academy:** disables the fix for aircraft movement points after capturing the project.
- **Post-capture diplomacy:** disables a check that prevents repeated AI requests to talk after losing a base.

**Not disabled:** the captured base's old owner's units **supported by that base** are still reassigned to another eligible surviving base, or destroyed if none is suitable, in an unconditional section of `mod_capture_base()`. Also `facility_capture_fix=1` and `probe_action_fix=1` are separate switches that remain enabled. See [BUG-001](../WORK-QUEUE.md).


## Titanium sprite modes and overrides

The **distributed experimental test INI** selects `minor_native_titanium=1` and **`minor_titanium_sprite_mode=3`** because the user confirmed that setup rendered blue-grey Titanium correctly (transparency, positioning, zoom and +1M) in Windows Develop on 5 November 2026. The **compiled Config default** when this option is absent remains `0` (native Uranium art).

| Mode | Behaviour |
| --- | --- |
| 0 | Native Uranium on Titanium tiles; no PCX |
| 1 | External Uranium reference from local `neca_sprites.pcx`; user-verified |
| 2 | Old experimental `minor_terrain.pcx` loader (legacy, not recommended) |
| 3 | Distinct blue-grey Titanium from local `neca_sprites.pcx`; **user-verified**, native variant 0 only |

Modes 1/3 require the separately user-produced indexed PCX next to `terranx.exe`; missing/invalid art or an unmatched native sprite variant falls back to original Uranium. The **public CI artifact does not include user art**. A local `thinker_user.ini` entry can override the distributed `thinker.ini`; the user's successful mode-3 setting is known, but the complete installed INI contents have not been inspected. Check both files when reproducing a report.

See [sprite-atlas.md](sprite-atlas.md) for layout, palette/mask rules and local builder. Parser/default authority is `src/main.cpp`/`src/main.h`; drawing selection is `src/gui.cpp`.
