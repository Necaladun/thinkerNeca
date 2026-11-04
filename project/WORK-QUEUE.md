# Work queue — current and future

**Owner of actionable work.** Keep concise acceptance criteria here. This is not a release changelog. Updated 2026-11-04.

**Statuses:** `WIP` = being worked on/tested; `TODO` = accepted next work; `TBD` = needs a design decision; `Blocked` = explicit dependency; `Deferred` = intentionally later; `Done` = verified/closed. An **Idea** belongs in [IDEAS](IDEAS.md) until promoted. Priorities: **P1** = correctness/merging blocker, **P2** = useful next, **P3** = cleanup or longer-term.

## Active and near-term

| ID | Priority | State | Task and completion criterion |
| --- | --- | --- | --- |
| TERR-001 | P1 | WIP | **Runtime regression for independent terrain.** New-game Jungle/Uranium/Titanium survive actual save→reload, Inspector and fog/zoom/focus tests; document version and results in [TESTING](TESTING.md). |
| TERR-002 | P2 | TODO | **Generation coverage.** Evaluate several seeds, map sizes, climate values, patch distributions and landmark overlaps; record reproducible anomalies or mark checked. Mirrored modes are currently deliberately unsupported. |
| TERR-003 | P2 | TODO | **Projected vs actual yield audit.** Catalogue divergence in `item_yield()` vs live calculations for future improvements (especially boreholes); correct after resource-extraction policy is decided. |
| REND-001 | P1 | TODO | **Native Jungle hook safety.** Preflight all three executable byte sites before changing any; on unsupported preimage do not half-install the hook or crash solely for a mismatched optional hook. Verify supported executable still works. |
| REND-002 | P1 | TODO | **Release parity and packaging.** Test Release DLL/PCX/Inspector; include atlas if needed and document any unsupported renderer modes. Pass a clean installation smoke test. |
| REND-003 | P2 | WIP | **External Titanium sprite processing parity.** User tested mode 1 and reported translucent/fragmented raw Uranium while missing PCX correctly fell back to native. Exact source pixel copy (822,253→16,16; 100×62) verified. Native path from supplied terranx.exe uses loader offset 10/count 236, remaps decoded indices 7→9 and 6→9, then extracts with key 9. Develop mode 1 now mirrors this and gates extraction on pixel checksum `24A1B67A`, 5,635 transparent and 565 visible pixels, and variant-0 native sprite pointer `0x78A700` only; variant 1 remains native. **Pending:** CI and in-game comparison of mode 1 against native at identical zoom; inspect preprocessing diagnostics, mask, offsets, fog/units and missing-asset fallback. Modes 0 and 2 remain unchanged. |
| BUG-001 | P1 | TODO | **Probe/base capture regression.** `base_capture_fix=0` appears to resolve observed probe issue. Investigate post-mind-control stack cleanup using pacts with **former defender** (possible attacker probe kill); test other capturing/allied/supported units, assimilation and unrelated fix regressions. Restore `1` only after targeted verification. |
| MAP-001 | P2 | TODO | **Major-landmark regression.** Inspect Dunes, Borehole Cluster, Fossil Field and other majors on fresh maps; check new terrain overlaps and rendering. |
| MAP-002 | P2 | TODO | **Retired Canyon slot validation.** Confirm new maps never set slot 10 (`0x200`) or generate old names/effects under both available world-builder paths. Source guard already exists. |
| MAP-005 | P2 | Done | **Sunny Mesa centre-only +1E.** Both actual `mod_energy_yield()` and projected AI `item_yield()` require `code_at() < 9`; unchanged *energy* footprint. Static CI passed and user confirmed +1E in central 3×3, none outside (2026-11-04). Broader improvement interactions remain MAP-001/TERR-001. |
| MAP-006 | P2 | Done (reverted) | **Stepped Sunny Mesa experiment rejected.** Raising the central 3×3 to 2,000–2,999 m created a visibly rounded/bulging summit under the existing native Mesa artwork (user screenshot, 2026-11-04). Reverted to the prior 5×5 1,000–1,999 m elevated area and outer rim below 1,000 m. Kept +1E on central 3×3 (MAP-005). Reconsider only with alternative terrain/sprite rendering. |
| BUG-002 | P1 | WIP | **Titanium sprite/Inspector discrepancy.** Old manually positioned overlay was offset; user reports shared native Uranium/Titanium icons appear correctly, including on new maps. Verify distinct Titanium sprite substitution retains that alignment. Inspector name consistency remains a separate outstanding check. |
| CFG-001 | P2 | TODO | **Generator switch consistency.** Decide whether `modified_landmarks` should govern independent Jungle/Uranium, and document/migrate settings without silently changing tested maps. |
| TEST-001 | P2 | TODO | **Runtime observability.** Make useful seed/patch-count diagnostics available to Develop tests and consider automated generation checks. Do not accidentally change RNG order. |

## Design decisions and later work

| ID | Priority | State | Decision or work |
| --- | --- | --- | --- |
| BAL-001 | P2 | WIP | **3/3/3 cap test.** Compare economy/AI outcomes to 2/2/2 over actual games. Leave as configurable test value pending evidence; see [DECISIONS](DECISIONS.md). |
| BAL-002 | P3 | TBD | **Forest yield 1/1/1?** Evaluate against existing 1/2/1, Tree Farms and alternative terrain; do not edit until tested. |
| GEO-001 | P3 | TBD | **Future geological differentiation.** Consider Titanium preferring rolling/rocky ground and/or retaining fungus while Uranium favours lowlands/clears fungus. Current generators deliberately identical; revisit after runtime regression and map distribution tests. See IDEAS. |
| BAL-003 | P3 | TBD | **Mine-dependent geological extraction?** Decide whether Titanium/Uranium should grant bonuses only with Mines, and how this interacts with Forests, Boreholes and base squares. |
| AI-001 | P3 | Deferred | **Former AI overhaul.** Assess projected Farm/Mine/Solar/Forest/Borehole yields, turn costs and context; avoid Forest default bias after balance rules settle. |
| MAP-003 | P3 | TBD | **Replacement major for slot 10.** Choose identity, yield and graphics design before new code or native rendering hook. |
| MAP-004 | P3 | Deferred | **Repurpose original major slots 3/4.** Defer removal of legacy Jungle/Uranium native semantics until independent render path and future major designs are stable. |
| MAINT-001 | P2 | Deferred | **Prune prototype rendering/diagnostic code** only after REND-001/002 and runtime tests pass; keep fallback paths meanwhile. |
| MAINT-002 | P3 | Done (pending runtime) | **Shared geology generator:** Uranium/Titanium now alternate using a common placement routine and distinct RNG seeds. New-map distribution may differ from prior builds; test several seeds under TERR-002. |
| MAINT-003 | P2 | TODO | **Review `skip_facility` high-ID shift safety** before exposing 33–64 as supported values in documentation. |
| MAINT-004 | P3 | Deferred | **Remove superseded merged PR branches** after confirming no needed independent work is stranded there. |

## Updating this file

When an item is verified, mark `Done` with a brief link to its PR/commit or [TESTING](TESTING.md). Periodically move closed items to a short completed section or remove them once the [fork changelog](../Fork-Changelog.md) records the durable result. New ideas go first to [IDEAS](IDEAS.md); don't put hypothetical changes into WIP.
