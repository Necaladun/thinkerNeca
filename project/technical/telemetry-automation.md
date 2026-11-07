# Campaign telemetry and automated AI games — design and implementation status


> **Implementation status (7 November 2026):** `TEL-001` has an initial opt-in recorder and local HTML/CSV analyser on unmerged branch `feature/tel-001-recorder-foundation`. Windows x86 Develop/Release CI and synthetic analyser tests passed ([run 42350976871](https://github.com/Necaladun/thinkerNeca/actions/runs/42350976871)); **An initial Windows Develop 11-turn game session and offline HTML/CSV report are user-verified; uploaded exports passed consistency checks.** A second user-reported Windows session following save/quit/relaunch/load continued at MY 2112–2119; HTML/CSV comparisons confirmed consistent base identities and 19 consecutive game turns across both sessions. This supports initial same-build save/reload/continue. Automatic campaign lineage, RNG neutrality and error handling remain unverified. A first AUTO-001 manual-handover turn-driver prototype now exists on the **separate stacked** `feature/auto-001-manual-ai-turn-driver` branch; first Windows Develop user smoke confirms a 10-turn AI progression and clean return to menu after manual handover; frequent native information popups still blocked unattended operation in that build. Uploaded HTML/CSV telemetry confirms ten all-AI turns (MY 2121–2130), 77 faction rows and 433 base rows. The separate AUTO-002 branch now source-implements a limited, opt-in presentation-only suppression pass; Windows runtime verification is pending. AUTO-002 onward, detailed AI event/decision history and watchdog features remain unimplemented.

### AUTO-002 preliminary notification policy (unmerged stacked branch)

`feature/auto-002-noninteractive-ui` provides **opt-in, source-owned** UI suppression; it is not a general-purpose popup clicker. Configure `telemetry_enabled=1`, `autosim_manual_turns=20`, `autosim_suppress_notices=1` in local `thinker_user.ini` after installing the branch's matching Develop build. Use the AUTO-001 manual Scenario Editor all-AI handover. It does not modify native audiovisual/auto preferences.

Source-surveyed notification-only `NetMsg_pop` sites in `src/veh.cpp`, selected no-result `popp` sites, and techno-discovery presentation are allowed to suppress **presentation only**. Strict exact-label `X_pop` classification distinguishes one-way notices from optional manual technology-design and Social Engineering dialog requests, which are declined for the AI-controlled observer. Existing tech, faction and pod processing remain in place. No unclassified native dialogs or unknown choices are auto-answered. The telemetry session gains `ui_notices.jsonl`, with a processing-faction field (not necessarily the notice recipient), displayed/exported by the Analyzer. Following a user-reported 20-turn Windows run requiring Monument and new Secret Project popup dismissals, `monument(1)` new-achievement presentation is now suppressed only after its history record is stored. The return-unused `BEGINPROJECT` and `CHANGEPROJECT` UI notices are suppressed while the intel bit updates still execute. The turn-cap branch calls the native `save_game(0)` dialog **before** returning to menu; user Windows test verifies successful Save Game at turn cap, menu return, reload as Gaians, continued AI simulation to MY 2140, and another Save Game/menu return. Cancel is untested. Further source pass suppresses native-life capture feedback and spore-launcher terrain-destruction notices; observed AI hasty attacks now use the pre-existing AI half-movement cutoff, **not** an unconditional Yes confirmation. Those new fixes are not yet Windows-tested. A subsequent AUTO-002 source pass covers return-unused native life sighting, Secret Project completion, and both perihelion notices. Normal Book of Planet interlude calls are routed through the native handler with its `PREF_AV_INTERLUDES_DISABLED` bit **temporarily** enabled during all-AI suppression; the old value is restored before returning, and before/after `GameInterludeState` is logged in `ui_notices.jsonl`. This does not change the user's saved preferences, but whether this native route preserves interlude history is **not runtime-verified**. Endgame interludes in `game.cpp` remain untouched. No game preferences are changed. Unknown modal detection, movie/interlude policy and victory/endgame handling remain open for AUTO-002; 100+ turn unattended Windows verification is **not yet complete**.

### AUTO-002 keyboard delivery and combat pacing (source follow-up, unmerged)

A Windows run proved the previous F9/F10 handler works only before continuous AI simulation; while `control_turn()` loops through all factions, even Escape/Ctrl+Q are not dispatched by the ordinary UI. The source now drains keyboard messages **without dispatching normal gameplay commands** only in eligible all-AI sessions, periodically during AI vehicle iteration and again before a complete-turn control decision. F9/F10 requests are still acted upon **only at completed-turn boundaries**. Unknown native popup recovery is not provided by this mechanism. Actual input delivery must be verified in Windows.

`autosim_fast_combat=1` (compiled/distributed default, `0` to opt out) applies **only during a real eligible all-AI battle**. It temporarily enables fast battle resolution and quick movement, disables pause after battles and nonquick unit-render settings, and skips `veh_scoot` animation; the original preference bitfields are restored by RAII at each combat return. No preference file or game setting is written by this option. This changes presentation timing, not the combat formulas or RNG. There is no guarantee that all other native graphics waits are removed. Windows tests must check both speed and exact preference preservation.

### AUTO-002 fourth source pass: UI boundaries and operator controls

The experimental all-AI presentation layer now bypasses confirmed return-ignored foreign espionage/diplomacy/Council/random-event reports, automatic captured-city/base-upkeep windows, Secret Project cinematic and follow-on Datalinks help panel. Source-owned game state mutations still occur. Book of Planet presentation uses the existing temporary native interlude-disable route; user says repeated turns no longer display it, but interlude history is unverified. Unknown/choice-returning dialogs remain native by design.

**Turn-boundary controls** (requires a valid `autosim_manual_turns=N>0` all-AI session; independent of `autosim_suppress_notices`): **F9** queues pause at the next completed turn and presents a **Yes = Resume / No = native Save Game, then menu** dialog. **F10** queues native Save Game then main menu at the next completed turn. No partial-turn save/stop is attempted. The UI must receive the key event during simulation; check responsiveness in Windows. The timer, user preferences and normal human controls are not otherwise changed. The existing automatic turn cap continues to use the Save Game prompt.

### AUTO-002 Planetary Council execution (unresolved)

Windows all-AI simulation blocks on a Deirdre-convened Governor election: first a convening notice, then the Council window showing her self-vote. Source review identifies AI `call_council(faction_id)` in upkeep and native `council(MapWin->cOwner,0,0)` for pending sessions. Existing `COUNCILHOT*` suppression affects only later result notices. The native `council` function may process remaining ballots and resolution state, not just presentation. **Do not suppress or skip it before establishing behavior.** Required: all actual AI ballots and proposal effects, governor/veto/victory state, native human decisions unchanged, and a no-modal all-AI execution path. Source/runtime acceptance remains open.

### AUTO-002 probe notification follow-up (source audit, Windows unverified)

An observed-AI infiltration popup (`DETECTINFILTRATE`) exposed that `NetMsg_pop_2` post-action result coverage was not the complete probe UI surface. The current unmerged feature branch now bypasses the remaining return-unused `src/probe.cpp` NetMsg/spy/portrait/genetic/mind-control presentation calls, and the optional observer-only intelligence panel. `src/veh_action.cpp` similarly bypasses passive probe encounter reports and human confirmation prompts erroneously shown for an observed AI; the existing non-observed-AI operation path is retained. Actual AI operation selection, treaty flags, infiltration, side effects and human choice dialogs must not be bypassed. Suppressed or AI-path notices are recorded in `ui_notices.jsonl` when telemetry is active. This revision requires Windows gameplay testing; it does not establish exhaustive probe UI or BUG-001 capture correctness.

### Initial TEL-001 usage (unmerged branch)

1. Build the branch Develop DLL/launcher; install them into the same supported game directory and retain the normal game files/configuration.
2. Set `telemetry_enabled=1` in `thinker_user.ini` under `[thinker]` (the distributed `thinker.ini` ships `0`). This is independent of `debug.txt` and does **not** activate developer Debug mode.
3. Play normally. At each completed all-faction cycle, the recorder writes under `campaigns/campaign-*/sessions/session-001/`: `manifest.json`, `turns.jsonl`, `bases.jsonl` and `status.json`. `campaign.json` sits in the campaign root.
4. **Windows GUI (recommended):** copy BOTH `tools/Campaign Analyzer.pyw` and `tools/campaign_report.py` into the game's `campaigns/` directory (for example `C:\smac\smac-game\campaigns\`). Double-click `Campaign Analyzer.pyw` with normal Python/Tkinter installed. It lists recorded games and individual sessions with human faction, MY range, colony/unit counts, session status and update time, and can generate and open a report without typing any folder name. It defaults to its own folder and offers Refresh and Choose Folder.
5. **CLI remains available:** use `python tools/campaign_report.py "campaigns/<campaign-id>"` from a checkout, or point to the copied `campaign_report.py` in the game directory. Reports remain in each campaign's `reports/<session-id>/`. No network access or third-party packages are required.
6. Set `telemetry_enabled=0` to disable logging.

**First-slice limitations:** each new game session creates a *new* campaign directory, including when a save is loaded; menu-loaded save origin may be unknown, whereas command-line loaded saves have a source path. Automated save lineage, baseline and partial-terminal snapshots, AI decisions/events, live report regeneration, report packaging, and unattended turn control remain deferred. The new recorder makes no changes to the SAV format. The appended faction and base JSONL files are distinct from `debug.txt`. The GUI lists individual sessions, but each report still analyses one session. Automatic cross-session campaign lineage and comparative analytics remain `TEL-003`. The GUI list only previews metadata, whereas the report generator validates full records. User reports successful installed Windows GUI launch/use on 7 November 2026; exhaustive GUI edge cases remain untested.

**Scope:** optional historical logging of the *whole* SMAC/X game and unattended AI-vs-AI campaigns. This document defines the implementation contract; it is not evidence that features already exist. Relevant source: `src/main.cpp` / `src/main.h` (configuration and command line), `src/gameturn.cpp` (turn controller and faction upkeep), `src/game.cpp` (setup and victory), `src/savegame.cpp` (save/reload), `src/gui.cpp` (developer shortcuts and dialogs), `src/patch.cpp` (native hooks).

Accepted constraints:
- **Never modify, replace, redirect, or reinterpret legacy `debug.txt`.** It remains available for existing Thinker Debug diagnostics and crash output. New files live under campaign-specific folders.
- Capture all factions, world/ecology, diplomacy, combat, research, economic/production history, and AI decisions with actual inputs/rationales where instrumented. Human-played campaigns use the same recorder.
- Preserve gameplay, RNG calls, turn sequence, game rules, score and save compatibility for the *same build*. No global popup suppression; classify and handle each interaction.
- Prefer a GUI-based unattended run over true headless; it is acceptable to begin with a manual human-to-AI handover in Scenario Editor. Then automate it.
- Preserve build identity, configuration overrides and run/save lineage for interpretation and reproduction.
- Work in focused stages with Windows runtime acceptance rather than one large invasive turn-controller rewrite.

## 1. Separate campaign data and execution sessions

Proposed layout (names and file formats are subject to implementation validation):

```text
campaigns/
  <campaign-id>/
    campaign.json              # stable campaign identity, origin/save lineage
    sessions/
      <session-id>/
        manifest.json           # build, mode, settings, start/end, process identity
        turns.jsonl             # complete-turn snapshots, one object per turn
        events.jsonl            # game-wide significant changes, ordered
        decisions.jsonl         # instrumented AI evaluations and choices
        diagnostics/            # anomalous movement, crash/stall evidence
    saves/                      # checkpoint files with turn/session lineage
    reports/                    # generated graphs, timelines, comparisons
```

A **campaign** is the historical identity of a game. A **session** is one running process's segment of that game. A new game gets a fresh campaign ID; loading a recorded save resumes its campaign with a new session if identity is known; loading an earlier turn creates a branch, not duplicate 'official' events. Initially support an explicit campaign ID/profile or accompanying checkpoint metadata; automatic save identity resolution may come later. If lineage is unknown, create a new session/campaign and mark the source save rather than silently merging histories.

Parallel instances must never share writers or overwrite files. Use exclusive creation of unique session IDs and atomic finalisation where practical. Separate game directories are an acceptable first multi-instance strategy. Filename sanitisation, recoverable partial JSONL, file-error reporting and maximum diagnostic budgets are required.

The manifest should record mod version/commit/build type, executable and DLL hashes where possible, game rules and difficulty, faction identities, starting/map seeds where observable, source save, configuration files or their fingerprints (`thinker.ini`, `thinker_user.ini`, `alphax.txt`), active profile, time, and telemetry schema version. Distinguish observed effective values from config file text and inferred values.

## 2. Capture at consistent game phases

**First deliverable: snapshots for every completed game turn**, across all factions. Prefer a post-all-factions boundary in `control_turn()` rather than an arbitrary intermediate `faction_upkeep()` state. Include phase and sequential event index; avoid counting a turn twice after reload. Log a baseline after scenario setup if safe and meaningful.

Initial snapshot fields (names illustrative): `turn`, `mission_year`, `phase`, `faction_id`, `alive`, `human_control`, `bases`, `population`, `units`, `energy_credits`, `research_target`, `research_progress`, `technologies`, `SE_choices`, and appropriate faction totals. Add native/wildlife, climate, sea level, map counts and production/economy counters as fidelity is established. Preserve raw values; do not silently replace them with derived ratios.

Event stream: construction and capture/destruction of bases; troop production/loss; research and projects; diplomacy and pacts/wars; Social Engineering changes; terraforming, fungal and climate changes; council outcomes; victories/defeats; disasters and unusual events. Introduce stable instance identity for bases/vehicles; native array indices are reused and are not permanent identifiers.

Decision stream: **why an action was chosen** as structured source-backed inputs, candidate evaluations and selected option, followed by observable outcome. Never label post-hoc narrative as actual AI reasoning. When vanilla engine decisions lack inspectable rationale, state that explicitly. High-volume candidate logs must be optional.

Instrument without calls to `game_rand`, `next_rand` or any other game RNG; no extra expensive evaluation functions with possible state changes. Keep writes outside sensitive combat/AI loops wherever possible and buffer as appropriate.

## 3. Automated simulation profiles and launch

Proposed runtime profiles: `normal`, `diagnostic`, `observer`, `autosim`. Profile selection through launch arguments/Windows shortcuts and, where safe, a development control section in the in-game menu. Option names and hotkeys must be checked for collisions before implementation. Existing command-line options are `-smac`, `-native`, `-screen`, `-windowed`, and a save path; proposed profile arguments are **not implemented**.

Start with the already user-tested manual sequence: human-controlled faction -> Scenario Editor -> switch faction while retaining AI control. Source investigation must establish how this changes the human bitset and display ownership. **First automation proof** permits a manual handover, then advances successive turns without calling a human input loop. Later, offer an automatic handover at game start. Do not assume setting all factions AI without a display owner is safe. Do not couple read-only observer status, developer editing capability, human-control status, and scenario victory rules more than necessary.

The unattended mode must:
- Use the ordinary `turn_upkeep`, `faction_upkeep`, `mod_enemy_turn`, game RNG and victory mechanisms.
- Distinguish informational dialogs (can log/suppress), choices (must execute the appropriate AI/deterministic game path or stop with diagnostics), fatal/error dialogs (stop), and endgame handling (record result, checkpoint, exit or return cleanly).
- Avoid requiring mouse, keypress, focus or visible movie playback for progress; apply an explicit non-interactive preference profile such as disabling Secret Project movies/interludes where supported.
- Support pause/resume/stop, turn limit, optional speed/animation reduction, and a graceful exit after victory. Unknown modal dialogs must halt with evidence rather than blindly click a default.
- Allow autosaves/checkpoints in a campaign-specific location without overwriting or removing ordinary user saves. A temporary copy of an existing autosave is an acceptable early prototype only if made after the save finishes.
- Avoid gameplay-affecting Debug-mode flags unless explicitly requested; existing `Alt+D` toggles `STATE_DEBUG_MODE` and fast battle resolution, so treat a Debug-vs-Develop run as a distinct test environment.

## 4. Reliability and multi-instance operation

A heartbeat should include campaign/session ID, process ID, current turn, phase/faction, last progress timestamp, last successful save and cumulative writer error count. An external watchdog is preferable for detecting a truly hung game process; in-process telemetry cannot reliably report if the engine is stuck inside a blocking native call.

Checkpoints every configurable N turns; optional on suspicious stall or endgame. On relaunch, resume from the latest *complete and validated* checkpoint and a new session. Restarts must not silently fork or duplicate events. Begin with **one unattended instance** on Windows, then run multiple isolated game directories. Evaluate shared Windows preference keys, temporary files, window focus, renderer/display mode, file locking and actual throughput before calling multi-instance safe.

Do not require true headless operation. An unattended windowed process is sufficient for the first release.

## 5. Targeted movement-loop diagnostics

User-observed case: roughly 50 Spartan units repeatedly oscillated on unlimited magtubes, contributing to faction defeat. This is a reported symptom, **not a proven pathfinder root cause**.

Keep a bounded recent-movement ring buffer per unit or faction and flag repeated position/target/order signatures with no net progress. Distinguish oscillation *within one turn* from recurrence across turns, and intentional patrol or redeployment from an actual stalemate. Dump exact moves, chosen destinations, order/target changes, candidate movement priorities where available, movement points/roads/tubes, nearby occupancy, faction plan and relevant route costs. Include a faction-level detector for units exchanging places. Normal runs should not log every step indefinitely.

## 6. Analysis output and narrative layer

Generate offline reports from captured data, not from hidden in-engine UI history: per-turn faction comparison, major event timeline, wars, economic and research changes, unit losses, strategic objective outcomes, anomaly summaries and cross-build A/B experiments. Reports can be refreshed during long runs.

A later intelligence/flavour layer may render a logged event as e.g. 'Gaians adopted Planned economics', with faction-appropriate narration, but must respect in-game intelligence visibility for player-facing output. Narration never becomes evidence for the AI's motivation.

## 7. Ordered implementation and acceptance

The actionable backlog lives only in [WORK-QUEUE](../WORK-QUEUE.md).

1. **TEL-001:** independent recorder, campaign/session files, manifest, complete-turn snapshots. Build, schema and IO tests; Windows save/reload checks; confirm `debug.txt` unchanged.
2. **AUTO-001:** manual handover-compatible auto turn driver, single campaign, controlled stop.
3. **AUTO-002:** dialog classification and policy, movie preference handling, automated victory/stop; achieve 100+ unattended turns.
4. **AUTO-003:** campaign checkpoints, restart lineage and watchdog; repeat across save/reload and force a stall/crash test.
5. **TEL-002:** events and meaningful AI reasoning instrumentation; begin with a short set of source-backed decisions and broaden.
6. **DIAG-001:** pathfinding ring buffer and stuck-movement detectors, validated with synthetic signatures and real repeatable case when available.
7. **AUTO-004 / TEL-003:** concurrent isolated campaigns and offline comparative reports.
8. **DEV-001:** optional CI Debug artifact and diagnostic tooling, separate from enabling legacy extended Debug mode.

**Evidence hierarchy:** source inspection; Windows x86 compilation/CI; synthetic recorder tests; **actual Windows SMAC/X in-game trial**. CI passing does not prove unattended gameplay, popup coverage, rendering safety, RNG neutrality, or save/reload continuity.
