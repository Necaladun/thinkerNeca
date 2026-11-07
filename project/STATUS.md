# Current status

**Updated:** 2026-11-07. **Authority:** `master` is the published branch; terrain/rendering changes remain on [draft PR #4](https://github.com/Necaladun/thinkerNeca/pull/4), `feature/minor-terrain-generation-migration`. [Draft PR #5](https://github.com/Necaladun/thinkerNeca/pull/5), the native Empire Report, is parked on an older PR #4 ancestry and is not part of this PR.

## PR #4 — current Develop scope

- Independent Jungle **+1 nutrient**, Uranium **+1 energy**, Titanium **+1 mineral**, with no natural independent-type overlap; geological Uranium/Titanium artwork underneath ordinary resource icons. Current independent-terrain storage uses ordered `THMIN001` **v3**, retaining native `LM_*` identities. Distinct blue-grey Titanium art requires the user's local `neca_sprites.pcx`.
- **Windows Develop user evidence:** repeated same-build save → quit → reload → turn advance; minor yields/Inspector; native Jungle/Uranium; custom Titanium and resource underlay; fog/zoom/units/bases; Forest geological suppression and restoration; fungus removal; Titanium Mine + Road output (Rolling 4M, Flat 3M); nutrient/energy ordinary resource overlaps; Titanium Unity pod reroll producing nutrient and energy outcomes.
- **Fresh-map survey:** user checked multiple new seeds for mineral-resource exclusions and geological patch sizes, as well as rainy/dry Jungle abundance. This is positive *qualitative* evidence for the 3–5-tile experiment, not a logged quantitative successful-patch/seed/size matrix.
- **Nessus Canyon:** generator, old mineral yield, parser option and native call removed; slot 10 reserved, source/CI tests pass. User observed no Canyon; the old `canyon=0` was already the default. This does not need another gameplay test.
- **Configuration:** distributed trial `docs/thinker.ini` uses **`tile_output_limit=3,4,4` N/M/E**; user confirms those caps applied throughout previous Windows tests. Their complete installed `thinker.ini`/`thinker_user.ini` was not independently inspected. Core native graphics/Inspector support is **Develop-only**; Release cross-builds pass, but Release/PRACX parity is not claimed.
- **AI-002:** scoped automatic Titanium Mine selection implemented and source/CI checked, but the user deferred behavioural testing until multiple longer logged AI-only games are possible.

## PR #4 merge disposition

The user has **already completed** the requested basic smoke, cap, qualitative multi-seed patch-size/resource and Canyon checks and does not need to repeat them. Develop and Release CI passed at the last audited PR head. A final merge review must check that the latest head is still green, the intended experimental/Develop-specific scope and differences against `master`. CI cannot establish Windows gameplay correctness. [Evidence](TESTING.md), [work queue](WORK-QUEUE.md) and [decisions](DECISIONS.md) retain the detailed qualifications.

**Explicitly deferred, non-blocking:** Release/PRACX native features (REND-002), native sprite variant 1 (REND-004), constructed multi-minor tiles (TERR-004), optional editor/invalid-art/fog edge cases (REND-005), intermittent Inspector recurrence (BUG-002), probe capture under the current workaround (BUG-001), quantitative geology and economy studies (TERR-002/BAL-001/BAL-003), and multi-game AI-002 behavioural acceptance (TEL/AUTO dependency). Other major-landmark regression (MAP-001) and alternate generation-switch policy (CFG-001) are outside the accepted normal-world test scope.

**Separate backlog:** projected-vs-live yield discrepancies (TERR-003), general Former AI (AI-001), Forest balance (BAL-002), combat/arty BUG-004 and BAL-004, telemetry/automation, and prototype renderer retirement (MAINT-001). Do not fold these into PR #4 merge acceptance.

**After PR #4:** reconcile PR #5 against updated `master`, rerun its CI, then perform its final integrated Empire Report regression. Neither its UI code nor UI-002's future Turn Report should be merged through PR #4.
