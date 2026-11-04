# thinkerNeca — fork changelog

This is the **fork's change history**, separate from Induktio's upstream [Thinker release changelog](Changelog.md). Record meaningful work when it lands or is substantively tested; Git commits and PRs remain the exact history. **Unreleased** means present on a draft/unmerged branch, not a shipped release.

## Unreleased — draft PR #4 (2026-11-04)

[PR #4: independent minor-terrain generation/rendering](https://github.com/Necaladun/thinkerNeca/pull/4) — **draft, not merged**.

- Migrated Jungle, Uranium and Ironstone world generation to independent minor type/code records rather than native major-landmark flags. Jungle uses climate-sensitive early patches; Uranium and Ironstone use dispersed late geological deposits.
- Integrated native-stage Jungle textures and Uranium sprite rendering; added a Develop-only Ironstone native-stage sprite from a dedicated indexed-PCX atlas. Adjusted Ironstone positioning, zoom scaling and opacity during in-game testing.
- Added a guarded Develop-only tile-Inspector feature-line hook. User confirmed all three names display in the Inspector; additional UI regression tests remain.
- Removed the old Nessus Canyon random-map generator, mineral effect and `canyon` INI setting. Reserved native landmark slot 10 (`0x200`) for later design.
- Restricted Sunny Mesa's +1 energy to the central 3×3 tiles (codes 0–8) in both actual and AI-projected resource yields; preserved its larger geographical footprint. User-confirmed centre-only bonus in-game on 2026-11-04.
- **Mesa elevation experiment reverted after visual test (MAP-006):** trialled a 2,000–2,999 m central 3×3, 1,000–1,999 m middle ring and sub-1,000 m rim. The native graphic rendered a rounded/bulging summit rather than a flat mesa (user screenshot, 2026-11-04). Restored the previous 5×5 plateau at 1,000–1,999 m and sub-1,000 m outer rim. The confirmed 3×3 +1E bonus and all sprites remain unchanged. Updated the source contract to guard against reintroducing the raised core.
- Added Windows Develop CI and source/atlas/save/INI/slot-10 checks. CI compilation is **not** a substitute for in-game testing.
- Reconciled independent-terrain documentation and standardised the distributed **test** `thinker.ini` with user's choices: `base_capture_fix=0` workaround, `spawn_free_units=0,2,2,0,2,2,1,2,2`, `tile_output_limit=3,3,3` experiment, no polar caps, and events 13/17 disabled.
- Organised fork documentation into project status, work queue, decisions, ideas, testing, technical references and this changelog. No gameplay rules changed by that documentation reorganisation.

**Current test report:** initial Windows Develop gameplay and test INI appear functional. Release graphics parity, extensive regression and the probe-team capture issue remain open. See [STATUS](project/STATUS.md).

## Merged foundation — 2026-11-03

- [PR #3](https://github.com/Necaladun/thinkerNeca/pull/3): established independent per-tile `MinorTerrainTile { type, code }` and versioned `THMIN001` v2 save format; centralised Jungle +1 nutrient, Uranium +1 energy and Ironstone +1 mineral resource definitions; began AI yield integration. Native generation/rendering migration followed in PR #4.
- [PR #2](https://github.com/Necaladun/thinkerNeca/pull/2): removed the unsafe experimental tile-Inspector wrapper and temporary native landmark/name spoofing after an Ironstone click crash; removed false native named-landmark handling. These removed approaches are **not** part of the present design.
- [PR #1](https://github.com/Necaladun/thinkerNeca/pull/1): early attempt to show Ironstone in the original tile Inspector. It was superseded by PR #2 and later by the narrow verified hook in PR #4. Do not treat PR #1's implementation as current.

## Upstream

The inherited [Changelog.md](Changelog.md) remains the upstream Thinker version history (through v5.5 in the current source). Fork-only changes belong here; do not append fork work to upstream release entries.
