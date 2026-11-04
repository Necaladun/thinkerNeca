# Configuration — current fork test profile

**Authoritative shipped example:** [`docs/thinker.ini`](../../docs/thinker.ini). **Parser/defaults:** `src/main.cpp` / `src/main.h`. **Status:** user-selected test configuration, not upstream Thinker defaults or a settled balance release.

At startup, the fork reads `thinker.ini`, then optionally `thinker_user.ini` from the same game folder. The latter overrides ordinary single-value settings when present. Repeated options such as `skip_event`, `skip_faction` and `music_label` have special additive semantics, so they cannot always be undone by simply placing a different value in an override. To reproduce the tested profile, check both files.

## Currently intentional differences

| Setting | Test value | Why / status |
| --- | --- | --- |
| `base_capture_fix` | `0` | Temporary workaround for reported probe/capture issue; investigation BUG-001 |
| `probe_action_fix` | `1` | Separate probe-team mechanic fixes remain on |
| `spawn_free_units` | `0,2,2,0,2,2,1,2,2` | Player-preferred additional starting Formers and scouts, including aquatic equivalents |
| `tile_output_limit` | `3,3,3` | **Experimental** pre-tech resource cap; BAL-001, previously 2/2/2 |
| `world_polar_caps` | `0` | Player preference |
| `skip_event` | `13`, `17` on separate lines | Exclude Sunspots and Base Asteroid Strike |
| `new_world_builder` / `modified_landmarks` | `1` / `1` | Intended modified-map path |
| `minor_jungle` / `minor_uranium` / `minor_ironstone` | All `1` | Independent minor-terrain generation |
| `minor_native_jungle` / `minor_native_uranium` / `minor_native_ironstone` | All `1` | Current native rendering integrations; Ironstone Develop-only |
| `minor_native_inspector` | `1` | Develop-only native feature-line labels |
| `minor_render_mode` / `minor_debug_tile_names` | `0` / `0` | Prototype fallback mode selection; no debug tile-name overlays |

The older major-landmark options `jungle=0` and `uranium=0` **are distinct** from the independent minor settings. **`canyon=` is no longer recognised**; the retired native major slot 10 is reserved. Delete old `canyon=1` or `canyon=0` lines from *both* INI files to avoid parser warnings.

`modified_landmarks=1` currently gates independent Jungle/Uranium generation, while Ironstone's generator is not tied to that switch in the same way; the three types still require `new_world_builder=1`. See [minor terrain](minor-terrain.md) and [CFG-001](../WORK-QUEUE.md).

## Config maintenance

- Comments in `docs/thinker.ini` explain each supported setting; **avoid maintaining a second 170-option catalogue here**.
- Leave untouched values as they are while testing. Distinguish source-level mechanics from settings that merely activate existing code.
- `tools/test_thinker_ini_contract.py` checks names against the parser, expected array lengths, duplicates and the retired Canyon key. It does **not** prove every value is sensible in play.
- The compiled `Config` defaults may differ from the distributed INI. An option missing from a local file normally inherits a compiled default, not necessarily `0`.
- A clean game installation and a local `thinker_user.ini` can produce different behaviour despite identical repository contents; always record overrides when reporting bugs.
