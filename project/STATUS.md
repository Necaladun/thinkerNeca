# Current status

**Updated:** 2026-11-07. **Authority:** [PR #4](https://github.com/Necaladun/thinkerNeca/pull/4) merged to `master` at [`0f05fdb`](https://github.com/Necaladun/thinkerNeca/commit/0f05fdbd5509616cb5fea598a68ab74d3fd13d8b), followed by [PR #5](https://github.com/Necaladun/thinkerNeca/pull/5) (native manual Empire Report) at [`0b2e77a`](https://github.com/Necaladun/thinkerNeca/commit/0b2e77a00655d332a9b50578691b56a0c595a861). The existing Report and terrain features are now in `master`; deferred tests are not closed.

## PR #4 — current Develop scope

- Independent Jungle **+1 nutrient**, Uranium **+1 energy**, Titanium **+1 mineral**, with no natural independent-type overlap; geological Uranium/Titanium artwork underneath ordinary resource icons. Current independent-terrain storage uses ordered `THMIN001` **v3**, retaining native `LM_*` identities. Distinct blue-grey Titanium art requires the user's local `neca_sprites.pcx`.
- **Windows Develop user evidence:** repeated same-build save → quit → reload → turn advance; minor yields/Inspector; native Jungle/Uranium; custom Titanium and resource underlay; fog/zoom/units/bases; Forest geological suppression and restoration; fungus removal; Titanium Mine + Road output (Rolling 4M, Flat 3M); nutrient/energy ordinary resource overlaps; Titanium Unity pod reroll producing nutrient and energy outcomes.
- **Fresh-map survey:** user checked multiple new seeds for mineral-resource exclusions and geological patch sizes, as well as rainy/dry Jungle abundance. This is positive *qualitative* evidence for the 3–5-tile experiment, not a logged quantitative successful-patch/seed/size matrix.
- **Nessus Canyon:** generator, old mineral yield, parser option and native call removed; slot 10 reserved, source/CI tests pass. User observed no Canyon; the old `canyon=0` was already the default. This does not need another gameplay test.
- **Configuration:** distributed trial `docs/thinker.ini` uses **`tile_output_limit=3,4,4` N/M/E**; user confirms those caps applied throughout previous Windows tests. Their complete installed `thinker.ini`/`thinker_user.ini` was not independently inspected. Core native graphics/Inspector support is **Develop-only**; Release cross-builds pass, but Release/PRACX parity is not claimed.
- **AI-002:** scoped automatic Titanium Mine selection implemented and source/CI checked, but the user deferred behavioural testing until multiple longer logged AI-only games are possible.

## PR #4 merged — outstanding limitations

The user previously completed the requested basic smoke, cap, qualitative multi-seed patch-size/resource and Canyon checks. Develop and Release **PR-head CI passed** before merging; `master` now contains the feature code and experimental INI. This is **Windows Develop user-tested functionality**, not full Release/PRACX parity or exhaustive in-game verification. The separate post-merge `master` CI checkpoint must be reported by its actual result, not inferred from PR CI. [Evidence](TESTING.md), [work queue](WORK-QUEUE.md) and [decisions](DECISIONS.md) retain the detailed qualifications.

**Explicitly deferred, non-blocking:** Release/PRACX native features (REND-002), native sprite variant 1 (REND-004), constructed multi-minor tiles (TERR-004), optional editor/invalid-art/fog edge cases (REND-005), intermittent Inspector recurrence (BUG-002), probe capture under the current workaround (BUG-001), quantitative geology and economy studies (TERR-002/BAL-001/BAL-003), and multi-game AI-002 behavioural acceptance (TEL/AUTO dependency). Other major-landmark regression (MAP-001) and alternate generation-switch policy (CFG-001) are outside the accepted normal-world test scope.

**Separate backlog:** projected-vs-live yield discrepancies (TERR-003), general Former AI (AI-001), Forest balance (BAL-002), combat/arty BUG-004, BAL-005 and BAL-004, telemetry/automation, and prototype renderer retirement (MAINT-001). Do not fold these into PR #4 merge acceptance.

**UI-001 merged:** PR #5 was reconciled against merged PR #4, with no terrain/save/config regression in its branch diff. PR #5 Develop and Release CI passed on final head `e5a4edd` ([run 42338436623](https://github.com/Necaladun/thinkerNeca/actions/runs/42338436623)). User reported the integrated Windows Develop Report **generally seems fine** and expressly **deferred** exhaustive 1024×768/high-resolution layout, 40–60+ colony/row targeting, map-zoom font, modal/F4/focus/display-mode and additional formal same-build save/reload regression cases. Earlier detailed queue/stockpile/navigation and save/reload reports remain evidence for prior tested build `3c08b52`, not proof of every final-binary combination. The report-stays-open-across-load quirk is user-accepted. [UI-002](WORK-QUEUE.md) automated start-of-turn report and popup batching remain unimplemented and deferred. Post-merge `master` CI must be checked independently.

## Artillery PR #10 — merged, deferred monitoring

[PR #10](https://github.com/Necaladun/thinkerNeca/pull/10) **merged into `master` at [`2b889dc`](https://github.com/Necaladun/thinkerNeca/commit/2b889dcb6c15477fdc930827245fad5292bebed7)** on 7 November 2026 with explicit user approval following successful [head CI run 42345065149](https://github.com/Necaladun/thinkerNeca/actions/runs/42345065149) (Develop/Release x86 builds and C++11 regression) and positive Windows Develop smoke: Impact 4 occasionally damages armour 8, Fusion-versus-Fission bombardment damage appears comparable, base/bunker 50% caps hold and Spore Launchers damage Fusion. This combines the [BUG-004](WORK-QUEUE.md) correction and a deliberately provisional [BAL-005](WORK-QUEUE.md) balance rule. **User explicitly accepts deferring wider checks** to subsequent games; untested reactor-mode/cap/stack/save cases are monitoring items, not established passes. No AI weights, artillery prices, range or direct/duel combat were modified.

## TEL-001 — campaign recorder implementation branch (not merged)

`feature/tel-001-recorder-foundation` implements an **opt-in** independent campaign JSONL recorder and initial local HTML/CSV analyser without modifying `debug.txt`, AI control, or game rules. [Windows x86 Develop and Release build plus synthetic report tests passed](https://github.com/Necaladun/thinkerNeca/actions/runs/42350976871); the user subsequently generated a normal, closed 11-turn Windows Develop session and local HTML/CSV reports; uploaded exports reconcile for all factions/bases/vehicles. A second Windows Develop session was produced after user-reported save/quit/relaunch/load, with MY 2112–2119 continuing MY 2101–2111; initial save/reload continuation is supported by matching base identities and consistent game statistics. Deterministic equivalence, auto-lineage and long-run stability remain unverified. A Tkinter campaign picker was added for selecting and generating reports without typed folder paths; Windows GUI launch/use is positively user-reported; comprehensive interaction/error-path testing remains unverified. No PR opened. See [TEL-001](WORK-QUEUE.md) and [telemetry design](technical/telemetry-automation.md).


**AUTO-002 actual Windows MY2182 Council + operator evidence (8 November):** new Develop DLL `71e435e` ran all-AI MY2101–2182 with telemetry. Deirdre (actor 1; zero human bits) completed Empath Guild, then native `call_council` blocked on Protocol Director OK before `CouncWin_init`; after user clicked OK, native CouncWindow required Vote/Deirdre selection and returned a recorded Governor outcome of **Lal (faction 7)**. Five faction votes encode `-3`, not yet decoded. Native ballot presentation and AI vote parity remain **unfixed/unverified**. F10 correctly queued until full-cycle boundary, Save & Exit recorded a named SAV, and telemetry status closed at completed turn 82. This verifies that specific operator path, **not** startup from a linked save nor unassisted Council. See `project/TESTING.md` and `WORK-QUEUE.md`.

**AUTO-002 unified operator menu/startup confirmation (8 November; source changes, Windows pending):** the previous end-of-run window now shares a modal Win32 setup/start/running UI: **Ctrl+Shift+F12** opens non-invasive human-world setup or requests the running menu at a safe complete-cycle boundary. After a fresh/reloaded game becomes fully AI-controlled, startup confirmation precedes upkeep even if local `autosim_manual_turns=0`; a declined/broken dialog restores human control. Shows immutable campaign and session IDs, rejoined status, editable display name, run-local turn statistics, turn budget and in-memory notice/fast options. Existing F9/F10 remain all-AI aliases only, while normal native keys remain in human play. Named SAV campaign sidecars now optionally persist runtime profile across explicit fingerprint-linked rejoin; plain autosaves get a new campaign/startup prompt. Linked-campaign rejoin Yes/No remains separate (not yet unified), telemetry enablement is fixed at game load, and Council remains unfixed. New code/static regression and CI wiring are committed but **no Windows x86 build/runtime test has been verified for this pass**.

**AUTO-002 operator control and readable campaign naming (8 November; new branch implementation, Windows runtime pending):** `feature/auto-002-noninteractive-ui` now source-implements a modal native **Simulation Control** window at the complete-turn cap and F9/F10/stop-file requests, with world/turn summary, user-entered **additional turn count (1–100000)**, campaign label and proposed SAV basename. Save & Continue uses patched native serializer through a temporary save and resets the segment count on successful commit; Save & Exit goes to menu, Take Control returns only observed faction to human on the next turn, and F9 can Resume. Default campaign directory names are now readable, editable display labels appear in the offline HTML/CSV GUI, and named saved files carry fingerprint-bound campaign sidecars for **explicit Yes/No session rejoin on reload**. [CI 42454429090](https://github.com/Necaladun/thinkerNeca/actions/runs/42454429090) at `09cd616e` passed Develop and Release x86, source contracts and synthetic analyzer tests. This remains unmerged. The user has since verified responsive F9/F10 and a five-turn Save & Continue continuation in Windows; native Save & Exit/rejoin/Take Control remain unverified, and Council remains blocking. [Technical details](technical/telemetry-automation.md), [Windows acceptance plan](TESTING.md).


**AUTO-002 satellite combat UI (8 November; unmerged, native route unverified):** user saw an Orbital Defense Pod destroy an enemy satellite and its result modal. The four success/failure attack labels and six passive Planet Buster ODP/Flechette report labels are now in the exact no-callback AI-only `X_pop`/`X_pop_2` allowlist, conditional on native presentation reaching these handlers. Added read-only `satellite_native` AI-attack inventory entry/exit telemetry; native action, attacks, RNG, satellite counters and optional sacrifice choices untouched. Construction/map-reveal, environmental satellite loss and fungal/tectonic defence were already guarded. Source contract plus host callback policy tests added/updated; **actual popup elimination and Windows state parity remain unverified**. See `project/TESTING.md`.

**AUTO-OBS Stage 1 source audit (9 Nov; draft PR #11, source only):** the new camera-neutrality audit identified confirmed observer-dependent branches affecting New Game rules, observer loss/endgame, cargo/transport orders, research-selection timing, project-intel/state and first-worm story flags; further classification of monolith, other native callers and no-human rules is pending. No gameplay patch or Windows A/B test. Work ID `AUTO-OBS`; [findings](technical/telemetry-automation.md#auto-obs-stage-1--observer-neutrality-source-audit-9-november-2026-first-pass), [acceptance](TESTING.md).  

**AUTO-002 9 November native notice follow-up (draft PR #11; Windows unverified):** native third-party pact breakup at `0x55CE20` is now routed to guarded `enemies_unpact`, preserving bilateral pact status processing; alternate native portrait popup wrappers also have exact-label suppression for satellite success/failure (`WEMUFFEDIT`/`THEYMUFFEDIT` included) with null/wait/expiry callbacks. Structural regressions updated. Original human/opt-out presentation, treaty/withdrawal state, actual ODP results, and same-build persistence need Windows acceptance; native `ENDPACT*` and unrecognised popup paths may remain. See [TESTING](TESTING.md).  

**AUTO-002 first Mind Worm loss interlude and rare-script audit (8 November; unmerged, Windows pending):** an observed `interlude(6)` path in `veh.cpp::battle_kill` has been routed through `autosim_interlude`; eligible all-AI suppression keeps the associated killer/base story flags, and rare alien interludes 41/42 are also now wrapped. A new structural regression is wired to Windows CI. User's uploaded `Script.txt` identifies further candidate unwrapped terrain/combat/marine/morale/rename notices, but labels alone cannot establish runtime reachability; native Council, `ENDPACT*` and meaningful decision prompts are not fixed. New patch has not been compiled or Windows-tested in this conversation. See `project/TESTING.md` and `WORK-QUEUE.md`.

**AUTO-002 newly reported diplomatic notices (8 November; source patch, Windows pending):** Added exact `AIDSUS`/`AIDSTHEM` intervention notification allowlist; no diplomatic effects changed. User also saw Spartan pact termination/territory withdrawal and Deirdre Governor-win; existing source-owned `PACTUNPACT`/`COUNCILHOTGOVWIN` wrappers do not establish coverage of their native popup paths. Council and native pact-ending presentation still need tracing; user-observed Morgan intervention may not reflect a real AI-vs-AI Vendetta in the inspected source path. 

**AUTO-002 fresh-game prioritisation (8 November; Council still unresolved):****AUTO-002 fresh-game prioritisation (8 November; Council still unresolved):** User reports a new all-AI game rapidly advanced about 68 turns before Deirdre's Council election blocked it. This independently confirms Council as an early-game unattended blocker, not solely a late-game problem. Previous MY2199 telemetry shows correct native AI ballots/ Governor assignment after initial acknowledgement; actual presentation callsite before `CouncWin_init` remains unisolated. No Council bypass committed; retain native vote/decision effects and target a narrow validated UI separation. 

**AUTO-002 ongoing 200+ turn audit (8 November; source changes, new Windows build/test pending):** User reports University orbital insertion, sea-level rise and tidal destruction of a colony, and Miriam declaring Vendetta following Zakharov's major atrocity. Orbital `MADEORBITAL` and `SEARISING` were already source-suppressed on the unmerged branch; new `map.cpp` site guards cover four `BASESUBMERGED*` notifications, and `faction.cpp` guards `MAJORATROCITY%d` plus ordinary `ATROCITY*` notifications. Flooding, casualties, emergency Pressure Dome, colony/vehicle loss, major-atrocity sanctions and AI Vendetta/pact changes remain intact. New source contract added to Develop CI; native correctness, actual branch compile and save/reload are not yet established.

**AUTO-002 new 200+ turn popup feedback (8 November; source patch, Windows test pending):** the user's installed game displayed volcano, Planet Buster and repeated "Keep moving [unit]?" messages. Current branch already guards random-event volcano `BIGFATVOLCANO` and ecological `VOLCANO`; source now also guards `PLANETBUSTER`, tectonic/fungal/ODP informational notices, orbital drop, base destruction, captured artifacts, refugee/escape, market and unit-gift announcements. Observed AI `KEEPMOVING` was traced to owner-only `order_veh` iteration logic and is bypassed to match non-observed AI during eligible full-AI automation; ordinary human order-cancellation prompts remain. Missile RNG, defence sacrifices, environmental damage and dramatic Planet Buster explosions are unchanged. Source test and CI workflow updated, native gameplay not yet verified. Council/true choice dialogs remain outstanding.

**AUTO-002 late-game notices (8 November; unmerged, new source patch pending Windows test):** first-satellite full-map `SEEMAP` after map/flag mutation, observed/foreign satellite production screens, faction `WIPEOUT*`/escape discovery, Subspace Generator, warming/volcano, alien arrival, Ascent and alien sanctions-end notices now use guarded presentation-only suppression. Tech `REACTORUPGRADE`/`TREETIME`/`SEEMAP2` and satellite-loss random-event notices were already covered. A structural source test has been added. The user confirms F9/F10 work and simulation is much faster on the **previous installed Windows build**; the uploaded trace shows five completed AI turns after Save & Continue. No in-game test yet for this new notice pass. Council voting and endgame choice dialogs remain unresolved.

## AUTO-001 — manual-handover AI-only turn driver (experimental branch)

**Branch:** `feature/auto-001-manual-ai-turn-driver` is stacked on unmerged `feature/tel-001-recorder-foundation`; neither feature is on `master`. Initial code is opt-in and source-implemented; [Develop/Release plus host tests passed CI](https://github.com/Necaladun/thinkerNeca/actions/runs/42355116603). **First Windows SMAC/X smoke positive by user report:** following manual Scenario Editor switch to another AI, the game advanced 10 turns and returned to the main menu. Unity pod and technology popups required manual input; genuinely unattended play remains AUTO-002. The uploaded analyzer HTML and two CSVs confirm one pre-handover human University snapshot (MY 2120) followed by **ten consecutive all-AI turns** (MY 2121–2130), complete seven-faction coverage, 433 matching base rows, and growth from 31 to 50 world bases. The session closed cleanly. Normal-human regression and automatic initial handover remain to check/implement.

`autosim_manual_turns=N` (default `0`): following a **manual** Scenario Editor handover of every faction to AI, the single-player `control_turn()` loop continues to use its normal `turn_upkeep()`, `faction_upkeep()` and `mod_enemy_turn()` paths. It skips the native between-turn `Console_human_turn()` only when **no playable faction's human-control bit is set**, there is a valid living map-view owner, no game-over state, and the game is neither multiplayer nor PBEM. Human play continues unchanged unless this condition actually becomes true. The driver counts complete post-handover faction cycles, excludes a partial resumed first cycle, and returns to the normal main menu after N cycles. Telemetry snapshots continue at each completed cycle. The configured limit is written into the telemetry manifest.

**Limitations:** the prototype **does not** automatically change faction control, handle blocking popups, guarantee that Scene Editor's handover clears all human bits, save a new checkpoint at stop, or perform an unattended endgame. Such work remains AUTO-001 follow-up and AUTO-002/003. Turn-limit return does not imply a save; enable/check ordinary autosaves if preservation matters. Native Console behaviour and control flags must be tested in SMAC/X, not inferred from source/host checks.

## AUTO-002 — opt-in informational popup suppression prototype (stacked branch)

`feature/auto-002-noninteractive-ui` is stacked on AUTO-001 and TEL-001, all unmerged. A first source-owned, **allowlisted** notification suppression pass is implemented, disabled by default (`autosim_suppress_notices=0`). It only activates alongside `autosim_manual_turns>0` after actual all-AI single-player handover, with a living observer owner and no completed game. It does not change saved game rules, audiovisual or auto-design preferences, faction control or RNG by design.

**CI status:** [CI 42356938677](https://github.com/Necaladun/thinkerNeca/actions/runs/42356938677) passed Windows x86 Develop and Release builds, host C++11 policy tests, Python report tests and repository static checks; no game runtime was executed. The pass removes the presentation step (not the reward/tech side effects) at source-controlled Unity pod and monolith result notices, selected tech discovery windows and pure announcement callsites. Optional `ASKSEEDESIGN` and `SOCIETY` prompts follow the noninteractive decline path; AI prototype evaluation and social upkeep remain separate. Unknown/interactive native dialogs are **not** globally clicked or suppressed. Each intercepted label is recorded in session `ui_notices.jsonl` and exported as `ui_notices.csv` and an HTML report section, provided telemetry is enabled.

**Keyboard/fast-combat finding:** Windows test shows F9/F10 work before the manual first End Turn but **all key input stops responding during continuous AI turns**; combat still uses normal visuals. The source follow-up now polls queued key messages during AI loops and only acts on F9/F10 at full-turn boundaries. New `autosim_fast_combat=1` (default within valid all-AI simulation; configurable `0`) applies a scoped fast-combat/no-battle-pause presentation profile and restores user preference bits after each battle. **Windows x86 CI [42362393860](https://github.com/Necaladun/thinkerNeca/actions/runs/42362393860) passed**, but the queued-input and visual-speed fixes remain **unverified in actual game runtime**; no AI/combat mechanical changes intended.

**Fourth AUTO-002 source pass (unmerged; Windows unverified):** [Develop and Release CI 42361420209](https://github.com/Necaladun/thinkerNeca/actions/runs/42361420209) passed, including policy and analyzer checks.  user reports repeat Book of Planet interludes are no longer blocking, but foreign probe reports, liberated-city management window, Secret Project movie/Datalinks and a Peacekeepers–Gaians friendship treaty still interrupt. Branch now suppresses source-classified passive probe, random event, diplomacy/Council results and base-capture UI, skips project movies/Datalinks only during all-AI presentation mode, and adds F9 pause / F10 early save-stop requests handled at completed-turn boundaries. Normal game preferences remain unchanged; Council voting and choice dialogs retain native AI paths. Native test of these changes, 100+ zero-click turns and termination/victory paths remain open.

**Full-turn fast profile, CI-checked (7 November):** all-AI `autosim_fast_combat=1` now applies the user-approved fast battle/movement and map-animations-off preferences for turn/upkeep/AI unit phases, restoring exact original flags before autosave and operator Save Game. Ordinary built-in year-named autosaves (`autosave_interval=1`) remain user-accepted and use the native save routine; AUTO-002 now explicitly calls that routine for eligible all-AI simulations regardless of the saved Autosave Each Turn preference (without modifying the preference). Dedicated AUTO-003 checkpoints deferred. Host tests/Win32 CI and Windows runtime verification must be reported separately. Council modal remains unresolved.

**Windows user runtime, 8 November:** latest AUTO-002 fast-presentation Develop build is reported to run much faster and to have progressed **100+ AI turns with no blocking popups**. After ~80 turns the observed faction's many displayed unit movements slow down markedly, while non-observed AI factions progress quickly. User clarifies that visible combat display, **especially ordinary artillery against large stacks**, is the main slowdown, not other factions' AI CPU time. Source confirms the observed `render_battle` path redraws focus, BattleWin/status and explosions once per target in the stack even with Fast Battle Resolution; combat-presentation suppression is now the leading scoped experiment. `PREF_ADV_NO_CENTER_VEH_ORDERS` remains a secondary movement-only idea. No code changed or performance benchmark run. **Follow-up source change (Develop/Release x86 CI passed; Windows runtime pending):** simulation-only profile now disables centering, visible unit scooting, per-target combat panels/explosions and additional map overlays, preserving the existing unseen-AI movement/damage paths and restoring native preferences after each phase. Fungus ecological `FUNGUSGROWS2`/`FUNGUSGROWS` notices and normal forest/kelp growth reports are now covered by opt-in suppression without skipping terrain mutations. Need a late-game large artillery stack plus ecology save/reload regression and independent combat RNG comparison; no runtime success claimed for this new pass. The previous 100+ smoke supports unattended progress for that session only, not complete Planetary Council/victory path coverage. [Tests](TESTING.md) and [queue](WORK-QUEUE.md) retain details.

**Council timing reconciled against actual project IDs (8 November):** uploaded `ui_notices(1).jsonl` records `PROJECT4` **Empath Guild** (faction 1) on turn 97, then `PROJECT36` **Planetary Energy Grid** (faction 3) and a Council/Governor election on turn 117. The Empath Guild preceded that vote by 20 turns; the identity of Deirdre's faction in that save and a causal unlock path remain unverified. [Test evidence](TESTING.md).

**AUTO-002 native executable/trace follow-up (8 November; Windows unverified):** uploaded `terranx(4).exe` SHA-256 `01901cbf7196b0c5d0df9540a029520f5df8fd9a6b343deef8b5663872805fcf` matches the previous installed game hash. Native disassembly confirmed `call_council -> council -> CouncWin_init`, native AI ballot generation and Council resolution/actual Governor updates **interleaved with window lifecycle code**. An opt-in, verified-callsite `__thiscall` diagnostic now records `CouncWin_init` entry/exit and final ballots to telemetry; it forwards to the original engine logic unchanged. [Windows x86 Develop/Release CI passed](https://github.com/Necaladun/thinkerNeca/actions/runs/42452468919); native gameplay still unverified. User reports Governor election often occurs around Deirdre completing Empath Guild, not yet confirmed as a deterministic trigger. No headless Council fix; a pre-Council Windows save and native run remain needed. [Technical details](technical/telemetry-automation.md), [testing](TESTING.md).

**AUTO-002 Council unattended blocker confirmed from Windows trace (8 November):** `ui_notices(1).jsonl` shows turn 117 `call_council(actor=5)` returning with Governor changed from `-1` to `7`; human mask `00000000`, observer `2`, no `diplomacy_guard` hits, native popup nonetheless displayed. This is a native Council decision/window problem. No verified headless Council implementation exists in the fork; source addresses and window structures alone do not justify suppressing `CouncWin` or skipping Council. **No Council fix made.** Precise native disassembly/runtime tracing with a pre-Council save remains required. [Testing](TESTING.md) and [technical audit](technical/telemetry-automation.md).

**AUTO-002 external stop/physical-key source pass CI: [run 42451218764](https://github.com/Necaladun/thinkerNeca/actions/runs/42451218764) passed Develop and Release x86 compilation plus `tools/test_autosim_operator_control.py` and prior host/static tests at `fff9fef`; actual Windows hotkey/file-stop/native Council tests remain pending.**

**AUTO-002 new Windows feedback/operator control (8 November; native test pending):** a Council popup still appeared on the Council/diplomacy-guard build. F9/F10 were reportedly inconsistent in continuous AI play, though F10 **did** register during the Council popup and the queued Save/Exit executed afterward. This narrows the issue to input delivery/UI state, not a permanently broken turn-boundary stop handler. Combat presentation is faster by user report, without deterministic parity evidence. A new feature-branch source change adds focus-gated physical F9/F10 state sampling and a separate one-shot `autosim-stop.txt` in the executable directory, consumed only at a completed all-faction boundary. Neither solves or bypasses a native Council modal. [Technical instructions](technical/telemetry-automation.md), [Windows tests](TESTING.md).

**AUTO-002 first-path source trial (8 November; Windows pending, x86 Develop/Release CI passed):** `enemy_diplomacy` already uses correct AI-to-AI negotiation when `is_human(i)` is false. No speculative rewrite was made. A verified all-AI entry guard now stops unexpected `communicate` human-dialog calls *before* any state/RNG processing and logs actor/partner/owner context. AI Council `call_council` entry/exit now writes read-only, telemetry-enabled event records to isolate whether the modal originates inside native Council. `tools/test_autosim_council_diplomacy.py` checks source integration. Lal's actual caller is still unknown: this is a defensive trial and diagnostic, **not a confirmed Council/bribery fix**. [Tests](TESTING.md) and [technical detail](technical/telemetry-automation.md) provide the Windows reproduction and event interpretation.

**Council source audit (8 November; docs only):** traced the two independent suspected modal routes: native `call_council` into `CouncWin` (which stores ballot and resolution fields), and native `enemy_diplomacy` calls redirected into `communicate`, whose human-oriented choice prompts do not guard `is_human` at entry. The observed owner is distinct from `FactionStatus[0]` after handover. Native invocation order and Lal's actual dialogue caller remain unverified. Recommended first step is narrowly scoped non-mutating call/state diagnostics, then reusing existing AI-vs-AI vote and bargaining decisions where available. [Technical audit](technical/telemetry-automation.md#auto-002-planetary-council-and-vote-bargaining--source-audit-8-november-2026-unresolved). No game code, build or Windows test for Council in this pass.

**New Council interactive blocker (8 November; Windows user report, unresolved):** a subsequent all-AI Council vote presented another modal and **Lal's offer to bribe the observed AI for its vote**, requiring a choice. This is decision-bearing Council diplomacy, not a passive result notification; the previous 100+ turn no-popup smoke did not cover this case. Source inspection identifies native `call_council` in every AI upkeep and a separate interactive `buy_council_vote` route, but Lal's precise native caller is not yet established. Current AUTO-002 source does **not** implement noninteractive Council AI voting/bribery. See [testing](TESTING.md) and [queue](WORK-QUEUE.md) for required native tracing and regression checks.

**Probe/Council follow-up (7 November):** Windows all-AI reports include Yang infiltration and a Deirdre-convened Governor-election voting window. New source audit covers return-unused probe, espionage, theft and observed-AI permission prompts; [Develop/Release CI + probe-source contract passed at `366a57e`](https://github.com/Necaladun/thinkerNeca/actions/runs/42363509724). No new Windows runtime validation. **Native Council election is not fixed**: it requires investigation of the vote-processing path before UI can safely be suppressed. The user now reports a 100+ turn zero-popup smoke on the newer fast-profile build; native Council votes, endgame and formal regressions remain open.

**Latest AUTO-002 popup pass (source only; Windows unverified):** user subsequently encountered native-life sighting, completed Secret Project, perihelion announcement and Book of Planet interlude. Source now wraps specific return-unused sightings, project completion and perihelion announcements with all-AI-only suppressors, leaving unit/project/planet-state mutations intact. Regular gameplay interludes call the original native function under a **temporary in-memory** disabled-interludes flag restored immediately; its effect on native `GameInterludeState` bookkeeping is unverified and logged for tests. Victory-related interludes are still native. [CI 42359983542](https://github.com/Necaladun/thinkerNeca/actions/runs/42359983542) passed Develop and Release builds plus host preference-restoration tests. This is **not** an unattended-run acceptance.

**First Windows user smoke:** 20 AI turns completed and returned to menu with default/native game preferences unchanged, but the user dismissed a Monument achievement and a Secret Project start notice. New source revision suppresses these specific presentation steps while keeping Monument history and project intelligence updates. The AUTO-001 configured stop now offers the native Save Game dialog before menu return; [CI 42358404736](https://github.com/Necaladun/thinkerNeca/actions/runs/42358404736) passed Develop/Release builds and host tests, but **user-verified** save and same-build reload/AI continuation (Gaians simulation resumed and reached MY 2140, then saved again); Cancel remains untested. Subsequent popup blockers were native-life capture notices, observed AI hasty attack confirmation and spore-launcher farm destruction. The branch now suppresses the passive notices and routes observed AI hasty attacks through the original AI movement cutoff rather than auto-confirming. The new suppression paths still require Windows testing. [Latest Develop/Release CI 42359230817](https://github.com/Necaladun/thinkerNeca/actions/runs/42359230817) passed including host hasty-policy tests. Further untested areas: completeness of popup classification, NewTechWin side effects, 100-turn unattended progression, movies/interludes, victory and unknown-modal detection. This is **not yet AUTO-002 completion**; 100+ turns without clicks remains a milestone. Ordinary player play must be regression-tested.


**AUTO-002 Council headless trial (8 Nov, PR #11 unmerged):** A new optional
all-AI Governor-election path now hooks only native Council callsites that
pass exact post-Scient x86 preflight, omits return-unused Council notices and
human-only vote-sale prompts, and calls the game's own vote-setting/resolution
when the native AI ballot array is complete. Otherwise it retains the native
Council UI. A line-flushed `council.jsonl` stream records context, ballots,
Governor and energy snapshots. Source/CI checks and Windows game acceptance
are **separate**; until Windows-tested, the path is experimental and the
Council issue remains open.


**AUTO-002 Council startup regression and corrective patch (8 Nov; user Windows report):**
The first headless trial at commit `d539c7d1` failed while patching
native instruction `0x004282F2` (generic “requires Alien Crossfire v2.0”
error). Static reproduction on the user's actual `terranx.exe` established
original bytes `FF 90 E4 00 00 00` (six-byte vtable call); existing
`write_call` accepts only direct E8 or FF15 and cannot patch FF90.
The corrective branch replaces **all six original bytes** with verified
`E8 rel32 / NOP` via `write_bytes`. The native `CouncWin_init`
has already sometimes called `CouncWin_set_vote` via the vote-purchase
branch, so the correction also refuses to replay an existing/resolved
vote. For an unvoted Governor election it calls native `council_get_vote`
for the observed AI faction exactly once and delegates to native
`CouncWin_set_vote`. Any inconsistent or unresolved state falls back to
the native modal, with separate `council.jsonl` diagnostics. Windows
in-game acceptance is still **pending**; the prior build is known bad.


**AUTO-002C MY2195 Council failure (user Windows reproduction, 8 Nov):**
Gaian faction 1 completed Empath Guild and initiated its first Governor
election in an all-AI game. `CALLSCOUNCIL` suppression worked. The
experimental modal hook then logged `observed_vote_recorded_but_unresolved`
and deliberately opened the voting UI. The native `CouncWin_init`
already populates **all AI ballots**, including the observed faction's,
at `0x4278E6..0x427974`. Nonzero observed vote is therefore normal.
The revised test path invokes native `CouncWin_make_resolution(win,1)`
directly (rather than recasting any vote); its native early return on a
missing ballot triggers a logged modal fallback. **Windows runtime validation
of the revised path remains pending.**


**AUTO-002 operator input regression (8 Nov, MY2195–2212 live test):**
The revised Governor Council headless election completed on Windows with
`native_make_resolution=1`, nonzero outcome, Governor faction 1 and
pending=0; the native window flashed briefly but needed no clicks.
Game advanced through at least MY2212. The user reports F9/F10 ineffective
and manually terminated the process. `ui_notices.jsonl` shows no
`OPERATOR_STOP`, `PAUSE` or other consumed operator command, despite
completed faction cycles. Source audit: keyboard polling/WinProc previously
only processed `WM_KEYDOWN`; F10 may arrive as `WM_SYSKEYDOWN`.
The follow-up patch accepts both types, logs keyboard latch sources and
turn-boundary consumption, and preserves the external `autosim-stop.txt`
fallback. Foreground focus and timing remain possible causes; awaiting
Windows testing. First Council Governor success does not verify other
proposals or vote purchases.


**AUTO-002C expanded Council trial (8 Nov; PR #11 draft):** After a
successful MY2195 all-AI Governor election on Windows, the same native
headless-resolution hook has been extended to stock Council proposals
0–9. Existing native ballots, thresholds, veto and motion actions are
preserved; proposal-specific before/after snapshots go to
`council.jsonl`. Non-Governor resolutions have **no Windows gameplay
acceptance yet**. Victory termination, final save/report and changing the
observed faction after elimination are **not implemented** in this pass.


**AUTO-LAUNCH Stage A (8 Nov, PR #11 unmerged; Windows test pending):**
`Run Automation.bat` opens a PowerShell pre-game GUI, saving defaults to
`automation.ini` and passing an immutable run profile to the existing
`thinker.exe`. A normal launch ignores legacy automation INI toggles;
only the explicit run profile enables telemetry/unattended play. Start from
an existing SAV or enter native new-game setup and manually hand all factions
to AI. Campaign save redirection/ZIP and automated game generation are later stages.


**AUTO-NULL / AUTO-NEW trial (8 Nov, draft PR #11; Windows runtime pending):**
larger four-tab PowerShell launcher selects normal/fast/null rendering and
world-generation settings. Native map/tile draw entrypoints are preflighted
and gated during all-AI turn phases, with a reversible renderer override.
The launcher console polls per-run faction/phase status, warns after 30–60
seconds without progress (default 45), offers Show Game, Restore/Null,
screenshot and safe-boundary Pause/Stop, and never auto-kills a stalled game.
Experimental native New Game quick-start clears human control before initial
AI setup, omits player planetfall, and skips its redundant in-game Start
prompt; no official runtime acceptance yet. Save ZIP, victory, observer
elimination and multi-game batches remain deferred.


**AUTO-LAUNCH diagnostic patch (8 Nov, PR #11 draft):** user observed
instant console closure and no campaigns on both Load and New Game.
The .bat now remains open after all exits and prints return code;
PowerShell writes `automation/logs/launcher-*.log` before starting
the game and logs PID handoff/errors. Windows-native launcher-GUI
construction CI added. Root cause of the game failing to start is
**not yet established**; subsequent Windows evidence required.


**AUTO-NULL startup crash candidate (9 Nov, draft PR #11):** Two user
logs show thinker.exe creating terranx.exe, which exits before the
first simulation heartbeat or campaign recorder. The renderer detours
were installed after executable pages had been restored to read/execute;
source patch now installs them while pages are writable. Awaiting
Windows user confirmation and actual crash-code evidence.


**AUTO-002 Windows New Game integration (9 Nov, PR #11 draft):** user
confirmed no-click fresh-map creation and AI progression; Save & Exit
unexpectedly began another game, graphics remained fully enabled and
monitor keys required Alt+Tab. Source follow-up closes a launcher game
after control_turn returns, restores Null Rendering after native
modal returns, adds per-phase renderer diagnostics and tries to foreground
the console on initial AI processing. Windows retest pending.


**AUTO-NULL Windows feedback (9 Nov, PR #11 draft):** A fresh all-AI New Game
ran through MY2200. Console output proves `configured=2` and actual
`null/restored/null` transitions when N/R are issued, but the user reports
substantial/complete graphics still appearing. This is incomplete native
presentation interception, **not a launcher/input bug**. Source now gates
the separate native `Win_update_screen` screen presentation entry at
`0x5F7320` and records skipped/forwarded map draw and screen update
counts; source/CI only, Windows runtime test pending. The 45-second idle
warning during the operator menu was expected—the user deliberately
left it paused—and remains enabled.
