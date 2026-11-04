# Decisions and working assumptions

Record choices that materially constrain implementation. **Established** means intentional and current; **Trial** means subject to gameplay testing. A proposal remains in [IDEAS](IDEAS.md), not here, unless adopted. Updated 2026-11-04.

| ID | State | Decision / rationale | Revisit when |
| --- | --- | --- | --- |
| DEC-001 | Established | **Fresh games are the target.** Old saves and legacy scenarios need not remain compatible across mod revisions. New-game same-build save/reload still must work. | A specific compatibility need appears |
| DEC-002 | Established | **Independent minor terrain** has its own type/code per tile; no repurposing native major landmark flags/names to represent Jungle, Uranium or Titanium. | Changing core terrain architecture |
| DEC-003 | Established | **Retire Nessus Canyon**; preserve unused native slot 10 (`LM_RESERVED_10=0x200`) for future major design. The former Canyon generator, bonus and `canyon` setting are gone. | MAP-003 design approved |
| DEC-004 | Established, implementation under test | **Prefer native-stage terrain rendering** to overlays drawn over units/fungus/improvements. Retain experimental fallbacks until verified. | Release/PRACX renderer strategy decided |
| DEC-005 | Established | **No broad AI/terrain rebalance while verifying terrain foundations.** Preserve strong altitude-dependent Solar yields; focus on getting AI to exploit specialised improvements rather than automatically nerfing Solar. | BAL-002/003 or AI-001 advanced |
| DEC-006 | Trial | **3/3/3 resource limits** in the distributed test INI; original cap 2/2/2 remains available. This is a test configuration, not a hardcoded mechanics change. | Gameplay evidence from BAL-001 |
| DEC-007 | Trial / workaround | **`base_capture_fix=0`** because of reported probe-team issues; other capture fixes are lost with it. `probe_action_fix=1` remains enabled. | BUG-001 isolated/fixed |
| DEC-008 | Current play preference | **No polar caps; exclude Sunspots and Base Asteroid Strike; extra starting units.** Test-profile values are in [configuration](technical/configuration.md). Not a universal balance standard. | Player preference changes |
| DEC-009 | Established, user-verified | **Sunny Mesa adds +1 energy only in its central 3×3**, not the wider 7×7. Both actual and projected energy use tile codes 0–8; user confirmed this effect in-game. | Future landmark balance decision |
| DEC-010 | Rejected / reverted | **Do not raise Mesa's 3×3 centre to ~2,000 m with the existing renderer.** The stepped variant produced a rounded/bulging summit instead of a flat top in the 2026-11-04 user screenshot. Restore the 5×5 1,000–1,999 m plateau and surrounding sub-1,000 m rim; keep core +1E. | A different rendering/terrain solution is explicitly proposed |

**Not decided:** reducing Forest to 1/1/1; making geological bonuses Mine-only; a replacement for major slot 10; reworking Former scoring. Those are tracked as TBD in [WORK-QUEUE](WORK-QUEUE.md) or exploratory [IDEAS](IDEAS.md).

**Versioning:** PR #3 merged the type/code foundation; draft PR #4 carries the present generation, native rendering, slot-10 and test-INI changes. See [Fork changelog](../Fork-Changelog.md). Git is the precise implementation history, not these summaries.
