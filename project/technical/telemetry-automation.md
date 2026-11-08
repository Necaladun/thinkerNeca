# Campaign telemetry and automated AI games — design and implementation status


> **Implementation status (7 November 2026):** `TEL-001` has an initial opt-in recorder and local HTML/CSV analyser on unmerged branch `feature/tel-001-recorder-foundation`. Windows x86 Develop/Release CI and synthetic analyser tests passed ([run 42350976871](https://github.com/Necaladun/thinkerNeca/actions/runs/42350976871)); **An initial Windows Develop 11-turn game session and offline HTML/CSV report are user-verified; uploaded exports passed consistency checks.** A second user-reported Windows session following save/quit/relaunch/load continued at MY 2112–2119; HTML/CSV comparisons confirmed consistent base identities and 19 consecutive game turns across both sessions. This supports initial same-build save/reload/continue. Automatic campaign lineage, RNG neutrality and error handling remain unverified. A first AUTO-001 manual-handover turn-driver prototype now exists on the **separate stacked** `feature/auto-001-manual-ai-turn-driver` branch; first Windows Develop user smoke confirms a 10-turn AI progression and clean return to menu after manual handover; frequent native information popups still blocked unattended operation in that build. Uploaded HTML/CSV telemetry confirms ten all-AI turns (MY 2121–2130), 77 faction rows and 433 base rows. The separate AUTO-002 branch now source-implements a limited, opt-in presentation-only suppression pass; Windows runtime verification is pending. AUTO-002 onward, detailed AI event/decision history and watchdog features remain unimplemented.

### AUTO-002 simulation control window and named telemetry campaigns (8 November 2026; native Windows test pending)

**Source implementation on `feature/auto-002-noninteractive-ui` (not on master):** at the configured `autosim_manual_turns=N>0` cap, or a completed-turn F9 pause/F10 stop/external `autosim-stop.txt` request, the all-AI single-player turn driver now opens a native modal **thinkerNeca - Simulation Control** dialog instead of unconditionally saving and returning to menu. It reports completed segment/total turns, current/end mission years, world colony and vehicle totals and surviving factions. The operator edits an **arbitrary positive integer from 1 to 100000** for additional complete AI cycles, plus an editable campaign display name and native SAV filename. Buttons: **Save & Continue** (save first; on success reset the segment counter to the entered additional turns), **Save & Exit** (save then main menu), and **Take Control** (no save; make only the viewed living faction human and enter native human gameplay at the start of the NEXT turn; no double upkeep). F9 pause also supplies **Resume** without saving. Closing an automatic-cap modal cannot silently restart an expired run; save failure/overwrite cancellation reopens the control window. F10/stop file now *request the control dialog* rather than always forcing Save Game/menu; the old native save-and-exit survives solely as a fail-closed dialog-creation fallback. Council/native diplomacy modal blocking is **not fixed**.

**Save safety:** `src/autosim_control.cpp` validates a user-supplied basename (ASCII alphanumeric, spaces, dot, hyphen, underscore; 1–72 characters, no traversal/Win32 device names), builds `saves/<name>.sav` and asks before overwriting. It calls the **same version-12 `mod_save_daemon()` serializer** hooked into native Save Game, writing to `saves/.autosim-<pid>-<tick>.tmp.sav`. Only after success and a plausible nonempty file does it use `MoveFileExA(..., MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)`; a failed write leaves the previous save untouched. This is a source-level guarantee against an unsuccessful serialization overwriting a prior save, **not** evidence of in-game save/reload correctness. The temp save is not a complete atomic *directory* transaction with telemetry sidecars; sudden power loss remains a separate risk. Normal automatic year-named autosaves remain unchanged.

**Recorder identity/continuation:** TEL-001 now starts new folders named `campaigns/AI-Sim-<observed-faction>-MY<start-year>-<YYYYMMDD>[-N]/`, not opaque UTC/pid names. The stable directory ID is separate from an editable `campaign.json.display_name`; both the offline HTML report and Tkinter campaign picker display the chosen label. The current running game stays in one `session-NNN` across repeated Save & Continue segments, each with operator events. Named saves receive a `.sav.autosim-campaign` sidecar containing a validated campaign ID, original creation timestamp, chosen name and the saved file's length/FNV-1a fingerprint. Loading a *matching* named save through the native Load Game menu or startup path prompts **Yes = join that campaign in a NEW `session-NNN` / No = start fresh**. Missing, modified, corrupt or unlinked saves start a new campaign; map-seed matching is not used. This metadata linkage is explicit and requires user approval on reload. Old campaigns without a `display_name` remain readable in the analyzer using the folder name. No game SAV layout or RNG rules are changed.

**Windows acceptance pending:** 10-turn cap with editable 7-turn continuation (exact 7 full cycles, no early console), correct report and per-session counters, distinct year-name save, overwrite Yes/No, invalid/empty save count/name, cancelled/failed write, F9/F10/file-triggered dialog, Return to Manual without duplicate upkeep, normal human/MP unaffected, resumed `.SAV` joining old campaign and preserving previous session, explicit No-new-campaign alternative, corrupted/changed save link rejected, all preferences preserved, and same-build SAV reload/advance. Validate window sizing and native modal focus at 1024×768. Source tests and x86 builds are **not a substitute** for these runtime checks.

### AUTO-002 preliminary notification policy (unmerged stacked branch)

`feature/auto-002-noninteractive-ui` provides **opt-in, source-owned** UI suppression; it is not a general-purpose popup clicker. Configure `telemetry_enabled=1`, `autosim_manual_turns=20`, `autosim_suppress_notices=1` in local `thinker_user.ini` after installing the branch's matching Develop build. Use the AUTO-001 manual Scenario Editor all-AI handover. It does not modify native audiovisual/auto preferences.

Source-surveyed notification-only `NetMsg_pop` sites in `src/veh.cpp`, selected no-result `popp` sites, and techno-discovery presentation are allowed to suppress **presentation only**. Strict exact-label `X_pop` classification distinguishes one-way notices from optional manual technology-design and Social Engineering dialog requests, which are declined for the AI-controlled observer. Existing tech, faction and pod processing remain in place. No unclassified native dialogs or unknown choices are auto-answered. The telemetry session gains `ui_notices.jsonl`, with a processing-faction field (not necessarily the notice recipient), displayed/exported by the Analyzer. Following a user-reported 20-turn Windows run requiring Monument and new Secret Project popup dismissals, `monument(1)` new-achievement presentation is now suppressed only after its history record is stored. The return-unused `BEGINPROJECT` and `CHANGEPROJECT` UI notices are suppressed while the intel bit updates still execute. The turn-cap branch calls the native `save_game(0)` dialog **before** returning to menu; user Windows test verifies successful Save Game at turn cap, menu return, reload as Gaians, continued AI simulation to MY 2140, and another Save Game/menu return. Cancel is untested. Further source pass suppresses native-life capture feedback and spore-launcher terrain-destruction notices; observed AI hasty attacks now use the pre-existing AI half-movement cutoff, **not** an unconditional Yes confirmation. Those new fixes are not yet Windows-tested. A subsequent AUTO-002 source pass covers return-unused native life sighting, Secret Project completion, and both perihelion notices. Normal Book of Planet interlude calls are routed through the native handler with its `PREF_AV_INTERLUDES_DISABLED` bit **temporarily** enabled during all-AI suppression; the old value is restored before returning, and before/after `GameInterludeState` is logged in `ui_notices.jsonl`. This does not change the user's saved preferences, but whether this native route preserves interlude history is **not runtime-verified**. Endgame interludes in `game.cpp` remain untouched. No game preferences are changed. Unknown modal detection, movie/interlude policy and victory/endgame handling remain open for AUTO-002; 100+ turn unattended Windows verification is **not yet complete**.

### AUTO-002 satellite, eradication and late-game announcement pass (8 November; source-only)

The first satellite's `SEEMAP` notice is a return-unused `popp` inside `mod_base_production`; before it, the game increments the produced orbital count, reveals tiles/base intel and sets `PFLAG_MAP_REVEALED`. Only the notice is gated by `autosim_notice_popp`. Production completion `PRODUCE3`/NetMsg for other factions and both observed-owner `popb` branches are similarly gated; the latter still update `facility_announced` where applicable. The orbital milestone monument is retained. Solar storm/asteroid satellite-loss notifications were already guarded and actual damage to satellites remains.

`mod_eliminate_player` builds `WIPEOUT*` after its setup/elimination operations and before `mon_killed_faction`; its return value is unused and only the presentation is skipped. Return-unused `TRACKED`, `GENERATORME`/`GENERATOR`, `TRIGGERWARMING*`, `VOLCANO`, `ALIENSARRIVE`, `ASCENT`, and `SANCTIONSENDALIEN` have explicit guarded source sites. The technology notices `REACTORUPGRADE`, `TREETIME`, `SEEMAP2` were already supported. All wrappers require actual all-AI eligibility and `autosim_suppress_notices=1`; intercepted labels go to `ui_notices.jsonl`. Do not intercept `GAMEOVERMAN` (continue/quit), `INFRASTRUCTURE` (social choice), `NOTRANSCEND` (callback not yet audited), native Council votes or other unknown modals. CI/static contracts and new `tools/test_autosim_lategame_notices.py` cannot verify gameplay state; native satellite reveal, elimination and save/reload checks remain.

### AUTO-002 strategic missile, random-event and observed-AI movement audit (8 November; source-only)

The full `random_events()` switch already uses guarded `POP2` / return-unused announcement wrappers; `BIGFATVOLCANO` and `EATTHIS` are covered there, and the separate `mod_base_ecology()` `VOLCANO` message is also covered. Existing installed DLLs predating the latest branch remain capable of displaying them. Event RNG, terrain and unit/base changes, volcano dust, climate and resource effects are preserved. Source audit of this entrypoint does not establish that every native event handler is covered.

A non-notification exception was observed in `order_veh()`: the viewed owner `MapWin->cOwner` gets the human `KEEPMOVING` confirmation after 32 tracked movement iterations even when Scenario Editor has cleared all human faction bits. If accepted, the old code cancels orders and resets exploration/alert state. AUTO-002 now bypasses this **owner-only human guard and counter**, but only during `autosim_control_active()`, making the observed AI match non-observed AI without fabricating a Yes/No result. The ordinary human confirmation remains. This does not fix or diagnose possible magtube/path oscillations.

`planet_busting()` suppresses only the return-unused `PLANETBUSTER` announcement; `shoot`/ODP, climate, base/unit destruction, `major_atrocity` and all `boom`/audio remain native. `shoot_it()` and `action_tectonic/fungal()` suppress return-unused warning, sacrifice-result and impact messages but preserve the genuinely choice-returning `FUNGALSAC/TECTONICSAC` defender prompt, native AI sacrifice logic, missile interception RNG, `TectonicDetonationCount`, and fungus/life spawning. Observed AI orbital drop, obliteration, artifacts, unit gift, alien-refugee, escape and market-interruption reports also have guarded presentation. `GOTUNITDEFEND/OFFEND` are exact return-unused `X_pop` allowlist entries; the true human `GOTOGOODY` path remains. `CONQCOOP/CONQSING` informational conquest messages are guarded without bypassing the actual victory flag or `GAMEOVERMAN` continue/exit choice. `tools/test_autosim_missile_move_ui.py` is structural only; an installed Windows x86 build and meaningful state/RNG-equivalence checks remain necessary.

### AUTO-002 orbital satellite attack/defence UI (8 November; native entrypoint provisional)

Satellite lifecycle has multiple UI paths. `base.cpp` increments one of `satellites_nutrient/mineral/energy/ODP`, updates first-orbit map visibility and `PFLAG_MAP_REVEALED`, and then emits source-owned production and `SEEMAP` announcements already guarded; `mon_first_in_space` is not suppressed. `gameturn.cpp` solar storms remove energy/ODP satellites and asteroids remove mining satellites before their already-guarded `POP2` reports. Orbital insertion `MADEORBITAL` is a distinct troop event.

Offensive Orbital Defense Pod attacks against enemy satellites are native `action_sat_attack` (`0x4CF480`) called by `faction_event.cpp::enemy_strategy` or native Orbital Attack View. Script templates `WEKILLEDIT`, `THEYKILLEDIT`, `WEMUFFEDIT`, `THEYMUFFEDIT` announce success/failure without meaningful buttons. The exact labels are now classified as one-way **only for no-callback `X_pop` or `X_pop_2`** during eligible all-AI notice suppression. Whether native `action_sat_attack` actually enters either wrapper is **unverified**. The native combat action and own success/failure RNG, satellite counters and Geosynchronous Survey Pods are untouched.

To identify blocking native UI without changing combat, the C++ `enemy_strategy` call records `satellite_native/action_sat_attack` entry/exit with actor, target, satellite type, base ID, attacking ODP/deployed and defending satellite inventories only if both telemetry and all-AI notice suppression are active. A missing exit while the popup is shown isolates a native call lifetime; an exit after acknowledgement records actual state. This only covers AI strategy-initiated attacks; it is not an Orbital Attack View hook.

Defensive ODP and Flechette actions against **fungal/tectonic missiles** were previously wrapped in `veh_action.cpp::shoot_it`, with ODP deployment, interception, fleet sacrifice and RNG intact. Native **Planet Buster** `WEMUSTSAC`, `THEYMUSTSAC`, `WESHOTIT`, `THEYSHOTIT`, `WESHOTIT2`, `THEYSHOTIT2` passive results are also conditionally classified at `X_pop`/`X_pop_2`. Preserve `BUSTERSAC`, `FUNGALSAC`, `TECTONICSAC` human Yes/No sacrifice menus and any AI-only native decision. Command warnings `NOKILLERS`, `KILLERSDEPLOYED` remain native. The tooltip `SAT1` in Script.txt claims 50% chance but native combat percentage was not independently verified by this audit.

Structural test `tools/test_autosim_orbital_strikes.py` and host `tools/test_autosim_policy.cpp` verify these source boundaries. Actual native modal callsite and result parity (hit/miss, satellite yield, ODP loss, turn continuation, save/reload and disabled/human cases) require Windows testing; no success claim until then.
### AUTO-002 flooding and major-atrocity UI reports (8 November; source-only)

The existing AUTO-002 branch already routes `MADEORBITAL`/`MADEAIRDROP` from `action_airdrop` and `SEARISING`/`SEAFALLING` from `turn_upkeep` to return-unused presentation wrappers. A sea-level increment still calls `world_climate`, which can cause `map.cpp::alt_set` to flood coastal colonies. That function adjusts improvements, reduces population via `mapgen_rand`, grants emergency Pressure Domes, updates submerged-faction counts, preserves last/objective colonies when required, otherwise destroys emptied colonies and incompatible vehicles.

`alt_set` also contained four modal notices: `BASESUBMERGED`, `BASESUBMERGED2` (native `X_pop_2`), and `BASESUBMERGED1`, `BASESUBMERGED3` (`popp`). The first two now have narrow opt-in `autosim_ui_active()` guards with `telemetry_record_ui_notice`; the original calls are retained unchanged for human play, without altering the globally shared `X_pop_2` policy. The other two are routed through `autosim_notice_popp`. Base state changes and subsequent destruction occur regardless of UI suppression.

`faction.cpp::major_atrocity` marks the target, applies reputation and major-atrocity counts, ends pacts, and declares Vendetta for eligible AI factions before the one-way `MAJORATROCITY%d` message, which is now routed to `autosim_notice_popp`. Ordinary `ATROCITYSPOTS`, `ATROCITY`, `ATROCITY1`, `ATROCITY2` messages use `autosim_notice_netmsg` and do not bypass sanctions/escalation. The AI retaliatory rule does not consult whether the retaliating faction committed earlier atrocities; the user's report about Miriam and Zakharov is mechanically consistent without proving the save's exact strike chronology.

Static regression: `tools/test_autosim_sealevel_atrocity.py`. Native Windows acceptance requires tide/colony state parity, strategic diplomatic parity, normal human/UI-off checks, and same-build save/reload; a passing CI build is not gameplay validation.
### AUTO-002 pact and allied intervention messages (8 November; source-only)

`src/faction.cpp::intervention()` emits `X_pop("AIDSTHEM", 0)` and `X_pop("AIDSUS", 0)` based on whether attacker or defender equals `*CurrentPlayerFaction` (the local observer). Their return values are unused. These two exact labels now map to `AUTO_UI_NOTICE` in the existing no-callback `autosim_xpop_policy`, which is used only with active all-AI suppression; normal-human paths call native `X_pop_9` unchanged.

**Do not confuse presentation with diplomacy:** this source function only sets the third-party `DIPLO_VENDETTA` treaty when either original belligerent is human. In an all-AI simulation it can still display an allied intervention report about the observer although that specific state mutation is skipped. The scope of other native AI war-making paths is not established, so the report-to-state discrepancy is a distinct AI diplomacy audit, not a mandate to change outcomes in AUTO-002.

`faction_event.cpp::pact_unpact` and `enemies_unpact` already classify `PACTUNPACT` / `ENEMYPACTENDS` as source-owned no-result reports, while real pact ending and partner-unit withdrawal can occur in native `pact_ends` (`0x53C650`) / `net_pact_ends` (`0x539660`): their unknown UI callsite must be isolated before changing. `COUNCILHOTGOVWIN` is already wrapped in a human-turn `control_turn` notification branch, but an all-AI Governor-win popup may originate deeper in `CouncWin` and must be traced along with the initial election announcement and vote-buying. These native cases are intentionally not auto-clicked or hidden.

Structural source contract: `tools/test_autosim_pact_intervention.py`. Acceptance requires all-AI/no-human plus native-human/opt-out comparisons, unchanged treaty flags, real unit ejection, Governor votes and save/reload.
### AUTO-002 unified operator startup and hotkey menu (8 November; source-only)

The same Win32 `autosim_show_control_dialog` now has three modes: `SETUP` (normal interactive world, no AI/upkeep/save), `STARTUP` (first all-AI `control_turn` before native upkeep), and `RUNNING` (end of complete AI faction cycle). `Ctrl+Shift+F12` is reserved in thinkerNeca for the operator menu; it is modifier-gated in `ModWinProc` and physically/queued-key sampled during continuous all-AI loops. It posts `AUTOSIM_CTL_MENU`, never executing the dialog inside a combat, Council/diplomacy or unit loop. F9/F10 remain backward-compatible aliases in all-AI mode and retain native meaning during normal human play. A normal-world shortcut uses the setup dialog synchronously with no simulation state mutation; Arm changes runtime `conf.autosim_manual_turns`, `autosim_suppress_notices`, `autosim_fast_combat` after explicit button validation, then user hands all human bits to AI and uses native End Turn.

The driver checks `autosim_preflight_ready` (zero playable human bits, single player, game not ended, living observed faction) before native upkeep, independently of the installed INI turn count and whether the loaded SAV was operator-linked. This avoids an unusable native human console on an all-AI save with `autosim_manual_turns=0`. Start applies typed 1–100000 turn budget and options then enters existing pure AI loop; Take Control or dialog-construction failure restores the observed human flag. The partial first faction cycle of a loaded SAV still counts as zero completed turns. During running, the same menu supports atomic Save & Continue/Exit, Take Control on next human turn, no-save Continue with fresh segment budget, and Resume Unchanged if the ongoing segment has not reached its cap. Campaign name changes are metadata-only. No user edits to global INI/game preferences or native AI decisions.

UI shows `telemetry_get_campaign_id()`, `telemetry_get_session_id()`, whether a named save rejoined, immutable identity and separate editable display name, per-process total and current-segment completed cycles, world state, turn budget and runtime presentation options. Enabling/disabling **telemetry logging itself** cannot be performed mid-session and is displayed read-only; the user must set `telemetry_enabled` before game load. An operator-named SAV still has a five-line fingerprint-bound `thinkerNeca-campaign-v2` sidecar with an optional backwards-compatible sixth `autosim-v1 <turns> <notices> <fast>` profile line. Only after matching the exact SAV fingerprint and **Yes** to the existing rejoin prompt does `telemetry_begin_session` restore this profile, before creating the next immutable `session-NNN`. A plain autosave never inherits campaign lineage automatically, though it still gets a startup menu when all factions are AI. The existing rejoin Yes/No is still a separate preliminary prompt; folding it into the unified window requires a future telemetry-init refactor.

Regression: `tools/test_autosim_operator_menu.py` and revised `tools/test_autosim_session_control.py`, run in branch CI. No successful Win32 compile/modal focus/start-of-save/End Turn smoke or same-build reload has yet been verified for these source commits. See Windows acceptance in `project/TESTING.md`.

### AUTO-002 keyboard delivery and combat pacing (source follow-up, unmerged)

A Windows run proved the previous F9/F10 handler works only before continuous AI simulation; while `control_turn()` loops through all factions, even Escape/Ctrl+Q are not dispatched by the ordinary UI. The source now drains keyboard messages **without dispatching normal gameplay commands** only in eligible all-AI sessions, periodically during AI vehicle iteration and again before a complete-turn control decision. F9/F10 requests are still acted upon **only at completed-turn boundaries**. Unknown native popup recovery is not provided by this mechanism. Actual input delivery must be verified in Windows.

`autosim_fast_combat=1` (compiled/distributed default, `0` to opt out) applies **only during a real eligible all-AI battle**. It temporarily enables fast battle resolution and quick movement, disables pause after battles and nonquick unit-render settings, and skips `veh_scoot` animation; the original preference bitfields are restored by RAII at each combat return. No preference file or game setting is written by this option. This changes presentation timing, not the combat formulas or RNG. There is no guarantee that all other native graphics waits are removed. Windows tests must check both speed and exact preference preservation.

### AUTO-002 fourth source pass: UI boundaries and operator controls

The experimental all-AI presentation layer now bypasses confirmed return-ignored foreign espionage/diplomacy/Council/random-event reports, automatic captured-city/base-upkeep windows, Secret Project cinematic and follow-on Datalinks help panel. Source-owned game state mutations still occur. Book of Planet presentation uses the existing temporary native interlude-disable route; user says repeated turns no longer display it, but interlude history is unverified. Unknown/choice-returning dialogs remain native by design.

**Turn-boundary controls** (requires a valid `autosim_manual_turns=N>0` all-AI session; independent of `autosim_suppress_notices`): **F9** queues pause at the next completed turn and presents a **Yes = Resume / No = native Save Game, then menu** dialog. **F10** queues native Save Game then main menu at the next completed turn. No partial-turn save/stop is attempted. The UI must receive the key event during simulation; check responsiveness in Windows. The timer, user preferences and normal human controls are not otherwise changed. The existing automatic turn cap continues to use the Save Game prompt.

### AUTO-002 external stop file and physical F9/F10 fallback (8 November 2026; Windows unverified)

**Latest Windows feedback:** a Council popup still appeared. F9/F10 were initially unresponsive during continuous AI processing, but **F10 was accepted while the Council popup was displayed**, and the game's existing Save Game/Exit path executed after the popup completed. This proves the turn-boundary handler works in at least that UI route; the input-routing issue is intermittent/context-dependent, not universally broken. User also reports improved combat presentation speed, not yet deterministic mechanical parity.

**Source fallback:** `src/gui_dialog.cpp::autosim_collect_queued_keys()` additionally samples `GetAsyncKeyState(VK_F9/VK_F10)` while an eligible all-AI game process owns the foreground window. Only a rising edge queues F9 pause or F10 stop; F10 takes priority. Sampling occurs during the existing AI unit iterations and at complete-turn boundaries, supplementing (not replacing) the earlier Win32 queued-key capture. Hold the key while the game is focused until a polling point. No native AI action is interrupted to present a Save Game dialog. This cannot close a blocked Council modal by itself.

**Focus-independent stop file:** create an empty file `autosim-stop.txt` **in the directory containing terranx.exe / thinker.exe** while a matching-build eligible all-AI simulation is active. Example PowerShell for the known installation: `New-Item -ItemType File -Force 'C:\smac\smac-game\autosim-stop.txt'`. `autosim_collect_stop_file()` checks this at the next completed **full all-faction cycle**, queues the existing Save Game → main menu path, and attempts to delete it. It requires `autosim_manual_turns>0`, all human-control bits clear, single-player, and living/nonterminal observer; it does not require `autosim_suppress_notices=1` or game-window focus. If deletion fails, the stop still queues, but a stale file must be removed before the next run. The old DLL does not support this file. A blocking Council modal must first resolve so normal turn control resumes.

**Windows acceptance:** set an artificially high turn cap, verify F10 and F9 hold behaviour while focused during normal continuous AI turns (not just within Council), then run with game unfocused and create `autosim-stop.txt` after 3–5 full turns. Verify file consumption, `ui_notices.jsonl` `EXTERNAL_STOP` event if telemetry is on, existing native Save Game dialogue and menu return, same-build reload and continued AI play. Test ordinary human mode (file ignored), partial loaded turn, file already present before simulation, deletion failure, Cancel Save, no keyboard interference and a Council modal. This solution does **not** save during an unresolved Council vote or guarantee OS process interruption.

### AUTO-002 Planetary Council and vote bargaining — source audit (8 November 2026; unresolved)

**Windows observations:** a Deirdre-convened Governor election displayed a voting window, and another session showed Lal trying to buy the *observed AI faction's* Council vote, with an answer required. Neither is an informational notice. Current branch **does not fix these modal paths**; this is source analysis and an implementation plan, not compiled or in-game Council evidence.

**Critical control distinction.** In `src/faction.cpp`, `is_human(id)` reads the bit in `FactionStatus[0]`; `MapWin->cOwner` and `*CurrentPlayerFaction` still designate the local *observer*. AUTO-001 clears every human bit, not the owner identity. Native routines may branch on local owner instead of actual human control. This is the leading hypothesis, not yet a measured native stack trace. The `src/gameturn.cpp::control_turn` source invocation `council(MapWin->cOwner,0,0)` on `CouncilSessionPending` is **inside the switched-human `is_human(current_id)` block**, so that explicit call cannot explain an all-AI Council window by itself. AI `faction_upkeep` calls native `call_council(faction_id)` at each eligible full turn; it may trigger further native calls or leave a pending session. If it only leaves the pending flag, the native Council completion path may be missing after the handover. Establish which is true by live tracing.

**Two candidate modal entry routes requiring separate diagnosis:**

1. **Council session/window:** `call_council` at `0x52C880`, `council` at `0x428350`, `council_get_vote` at `0x52BE60`, `council_action` at `0x52BF60`. `CouncWin_init` `0x427230`, `CouncWin_show` `0x427130`, `CouncWin_set_vote` `0x424720` and `CouncWin_make_resolution` `0x424410` suggest separable operations, **not verified safe headless APIs**. The known `CouncWindow` layout in `src/engine_win.h` holds proposal/candidate/outcome, `vote_faction_mask`, `faction_vote[8]` and `faction_vote_count[8]`: some resolution data is tied to native window state. Do not just hide the window or skip `council`.
2. **Diplomacy/vote-bribery:** `src/faction_diplo.cpp::communicate(faction_id,faction_id_2,flag)` is a human-oriented negotiation function containing numerous choice-returning `X_dialog` calls; it does **not** check `is_human(faction_id)` at entry. The fork's `src/patch.cpp` redirects native `enemy_diplomacy` callsites `0x55FDBD` and `0x5603E7` to `communicate`, so an observed AI can plausibly enter this human dialog path if native routing depends on `MapWin->cOwner`. `make_a_proposal` contains player-initiated `COUNCILMATTER`, `COUNCILVOTE`, `COUNCILVOTEGOV` choices and native `buy_council_vote` at `0x53EB50`. **Lal's actual AI-to-observer offer has not been matched to that exact callsite**; the native Council window may also initiate vote bargaining.

**Independent reference:** `b-casey/OpenSMACX/src/council.cpp` separately decompiles `council_votes` and `eligible` (already implemented in this fork); it does **not** supply a complete Council-vote/bribery session controller. No external implementation has been established that can be copied safely to resolve the UI problem.

**First-path trial implemented on feature branch (source; Windows unverified):** `src/faction_event.cpp::enemy_diplomacy` was audited more deeply: it already branches to AI-vs-AI trading/encounter/treaty logic whenever `!is_human(i)`, and therefore does **not** call `communicate` for an AI pair through its normal source path. No change to its game logic or the two native callsite patches is warranted. `src/faction_diplo.cpp::communicate` now has a narrow entry guard `autosim_control_active()` before constructing its popup, calculating diplomacy or consuming RNG. An unexpected call in a verified all-AI session is stopped, with `diplomacy_guard / communicate` and actor, partner, observer, flag, human mask and Council-pending state logged to `ui_notices.jsonl` when `telemetry_enabled=1`. This is **fail-closed**: it does not invent a vote, accept a bribe, or apply a substitute diplomatic outcome. Normal AI-pair diplomatic decisions and all ordinary human-dialog paths are untouched. Since the native caller of Lal's vote offer is not established, the guard might **not** stop that popup.

**First-path diagnostics:** `src/gameturn.cpp` also records `council_trace / call_council` immediately before and after every native AI Council call during eligible telemetry-enabled simulation, including actor, owner, human mask, pending session, Governor and owner proposal/vote state. The entry record flushes before the native call; a missing `exit` points to a stall inside it. A `diplomacy_guard` entry shows the source handler was reached. **No** `diplomacy_guard` entry with a still-blocking Lal popup indicates that some *other* native/UI path is responsible; instrument `CouncWin`, `council_get_vote` and `buy_council_vote` next. Record all these observations against the exact source head, save and config before deciding a full solution. This diagnostic adds JSONL records only when campaign telemetry is enabled; it does not change AI Council state, game RNG or save formats.

**Windows Council trace actually reviewed (8 November 2026; no Council fix):** user supplied `ui_notices(1).jsonl` with 845 events covering turns 80–117. 602 records are `council_trace`, every sampled Council event shows `human_mask=00000000` and `owner=2`; **zero** `diplomacy_guard` events. At turn 117 `call_council(actor=5)` enters with `pending=0, governor=-1, owner_proposal=0, owner_vote=0` and returns with `pending=0, governor=7`. The log ends with a turn-117 `OPERATOR_STOP/save_dialog` event following the user's F10 request. Thus a native Council call completed and changed governing state even though a Council modal appeared. This rules out the observed *source-level* human `communicate` entry in that recorded session; it **does not** identify the native callsite that displayed the modal, establish a clean uninterrupted Council outcome, or prove whether vote-buy offers occur in this same path. No safe Council UI-only replacement has been demonstrated.

**Implementation decision (8 November; do not ship a speculative bypass):** existing engine bindings expose `call_council`, `council_action`, `council_get_vote`, `CouncWin_init/show/set_vote/make_resolution`, but these are addresses/types and window fields, **not** a verified headless controller. Available OpenSMACX code implements vote weights/candidate eligibility only. Without native disassembly/stepping of the exact supported `terranx.exe` executable (including the Scient patch variant), plus captured ballot/proposal/payment state from a reproducible pre-Council save, we cannot establish which functions choose votes, perform transfers, apply resolutions or merely display a window. Hiding `CouncWin_show`, bypassing `call_council`, flipping owner/human flags or fabricating vote results would risk changing gameplay. No Council fix committed; normal native behaviour retained. Future work should gather exact native call flow and state transitions first.

**Native executable received and disassembled (8 November 2026):** user-supplied `terranx(4).exe`, SHA-256 `01901cbf7196b0c5d0df9540a029520f5df8fd9a6b343deef8b5663872805fcf`, **matches the earlier installed `C:\smac\smac-game\terranx.exe` hash**. PE32 x86 disassembly validates the source's actual native addresses and the `0x4284CA` direct `call 0x427230` from `council()` to `CouncWin_init`. Within `call_council`, `0x52D9C2` calls `council` at `0x428350`. `council` calls `CouncWin_init` at `0x4284CA`; that initializer copies the native human-faction mask at `0x427833–0x42783F` into a per-voter mask, prepares nonhuman ballots using `council_get_vote` (`0x427934`), and invokes `CouncWin_set_vote` (`0x428151`/`0x4282DD`). `CouncWin_set_vote` (`0x424720`) can call `CouncWin_make_resolution` (`0x42481E`), which computes outcomes and updates `GovernorFaction` at `0x4246E5`. Crucially, the window initialization path also invokes native window lifecycle/UI calls (`0x427D63` virtual call and later callbacks): **the source of the blocking modal is not yet isolated**. These are static disassembly findings, not a claim that a headless UI bypass is safe.

**Targeted native diagnostic implemented, not a vote fix:** `src/patch.cpp` conditionally patches only the verified direct `0x4284CA` call, after checking its original `E8 rel32` target still equals `0x427230`. It enables this only when both `telemetry_enabled=1` and `autosim_manual_turns>0`. The matching `__thiscall` wrapper `autosim_trace_CouncWin_init` writes `council_native/CouncWin_init` `enter` immediately before invoking original native `CouncWin_init`; after return it logs proposal, candidate, outcome, vote mask, eight voter selections, eight weight counts, pending and Governor state. The wrapper makes no gameplay, vote, payment or RNG decision. If the native call target differs, it skips installation rather than overwriting an unknown callsite. A new entry without a corresponding exit means processing blocked inside native `CouncWin_init`; both entries plus another modal means inspect its surrounding Council/GUI caller next. **Native Windows runtime and same-build save/reload still required.**

**Project-ID reconciliation with the uploaded Windows log:** `src/base.cpp` calculates `project_id = facility_id - SP_ID_First` before writing `PROJECT%d`; `SP_ID_First = FAC_HUMAN_GENOME_PROJECT = 70`. Therefore `PROJECT4` at turn **97**, processing faction **1**, is the **Empath Guild (facility 74)**. The `PROJECT36` event at turn **117**, processing faction **3**, is the **Planetary Energy Grid (facility 106)**, **not** the Empath Guild. The Governor election occurs on turn 117, 20 turns after the logged Empath Guild completion; `call_council(actor=5)` changes Governor -1→7. This supports a possible eligibility/cooldown relationship, not a proven direct project→Council trigger. The log does not identify faction leaders, so avoid assigning Deirdre without the game's faction mapping.

**Timing clue (user observation, not established causal code path):** the election often occurs on or a few turns after Deirdre completes the Empath Guild, but exact timing varies. Council eligibility and AI selection in `can_call_council`/`call_council` must be checked separately from when the Guild is built. A saved game from a few turns *before* this event is the remaining requirement for a reproducible Windows trace and gameplay-state comparison.

**Next implementation stage — remaining native trace and parity tests:** add opt-in AUTO-002 diagnostics before/after `call_council`, on `communicate` entry/exit and the Council proposal branch, with immediate flush before potential modal entry. Record `*CurrentTurn`, active faction/`MapWin->cOwner`, the full human mask, `CouncilSessionPending`, Council proposal/notification state, Governor, proposed votes, energy balances and diplomacy participants. On an identical pre-Council Windows save, if these logs do not identify the blocking call, use verified native entry/callsite instrumentation for `CouncWin_init/show/set_vote/make_resolution`, `council_get_vote/action` and `buy_council_vote`, taking care with x86 `__cdecl` versus `__thiscall` and original-code/trampoline safety. **No random-number calls or state mutations from diagnostics.** Compare event flow when changing only which AI faction is observed, and against an unobserved AI vote if possible.

**Fix selection after trace:** prefer making the observed faction follow the *existing* unobserved-AI Council vote/diplomatic route with actual AI decisions. If AI-vs-AI never initiates human-style vote-bribery offers, the correct parity may be to **not create the offer**, rather than inventing acceptance/rejection. If native resolution is coupled to `CouncWindow`, separate headless vote-state and resolution from display, possibly reusing correctly initialised native Council state and operations; a full replacement is a last resort, not the first change. Restrict to `autosim_control_active()`, single-player, no human factions; ordinary human Councils and all-AI opt-out policy need explicit behaviour tests. Do **not** spoof the observer as another faction globally, clear `CouncilSessionPending`, bypass `call_council`, blindly accept/decline, or send clicks to a modal.

**Acceptance:** repeat Deirdre Governor vote and Lal's negotiation from a reproducible pre-Council save with zero input; check every actual ballot, vote weight, vote-buy decision and resource transfer, Council cooldown/next eligible date, Governor/other proposal effects, Veto and Supreme Leader/diplomatic victory, pending-state cleanup and next session eligibility. Verify unchanged turn/AI diplomacy and RNG sequence against an equivalent all-AI path, along with F9/F10 responsiveness, save/reload during or after the completed Council and normal-human Council behaviour. CI compilation/structural tests alone cannot certify these paths.

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


### AUTO-002 Council execution/telemetry trial (8 November 2026)

A separate `sessions/session-NNN/council.jsonl` file is opened with the
campaign recorder and flushed per event so pre-modal state remains available
if the process blocks. Each event contains turn/year, actor, processing faction,
observed faction, human and alive masks, Council pending/Governor state,
observer proposal/vote, the eight factions' current energy credits and a
phase/detail value. It is deliberately separate from `ui_notices.jsonl`.
The offline Campaign Analyzer has not yet been extended to visualise the
new stream; raw JSONL can be filtered by turn, faction and phase.

The native x86 experiment checks expected calls to `popp` at
`0x4284A9` and `0x428542`, `X_pops_11` at
`0x428115` and `0x42828A`, `CouncWin_init` at
`0x4284CA`, and the six original virtual-call bytes
`FF 90 E4 00 00 00` at `0x4282F2`. On a known build it patches
those sites after Scient; otherwise *all* Council-specific hooks are skipped.
Handlers check runtime all-AI eligibility and `autosim_suppress_notices=1`,
forwarding normal human/opt-out execution to the native methods.

The omitted notices have unused popup results. `VOTEFORME`/
`VOTEFORMETECH` are human-only purchase offers: all-AI mode records
`human_only_offer_omitted`, makes no transfer/technology award and does
not fabricate a response. This is a **trial AI-parity assumption**, not
proof that all native AI-vs-AI vote-buy interactions are represented.

The virtual modal replacement only attempts Governor elections. When
native Council state is **already resolved**, it does not replay voting.
When the observed faction already has an unresolved ballot, it leaves the
window native rather than guessing whether processing is safe. Otherwise it
calls native `council_get_vote` once for the observed AI and passes that
result through native `CouncWin_set_vote`, allowing normal AI ballots,
`CouncWin_make_resolution` and `council_action` to run. If the
returned outcome is still unresolved, it falls back to the native modal.
All other proposals/unsupported modes retain native UI. This is an **experimental native hook**,
not a claimed fully working headless Council. Existing UI-coupled effects
and Council-action popups require actual Windows tests before merge.


**Normal-game baseline coverage:** When `telemetry_enabled=1`, the same
Council trace now also records native human-election `CouncWin_init`
entry/exit and Council announcement/vote-offer response events; ordinary
native UI and its chosen response are forwarded unchanged. This provides
a comparison dataset for later all-AI investigations without relying on
`debug.txt`. The new file is still raw JSONL (not yet an Analyzer graph).


**Patch-format fix (8 November):** the first trial failed during install
because `write_call` cannot handle the original `FF 90 E4 00 00 00`
virtual call at `0x4282F2`. An exact-six-byte `write_bytes` patch to
`E8 rel32 90` is now used. The additional native vote-state guard
prevents the earlier possible duplicate `CouncWin_set_vote` call.
Both changes require a new Windows smoke before any runtime claims.


**MY2195 Council replay (8 Nov, AUTO-002C):** live `council.jsonl`
shows all-AI observer 1 entering `CouncWin_init`, followed by
`modal_fallback=observed_vote_recorded_but_unresolved` and a blocked
Governor voting screen. Native disassembly proves that `CouncWin_init`
already fills all nonhuman ballots, including the observed AI, before
the modal call. The former nonzero-ballot guard was invalid. The headless
Governor path now invokes **native `CouncWin_make_resolution(win,1)`**
once, with already-populated ballots and no `council_get_vote` reroll or
`CouncWin_set_vote` resubmission. A zero return or zero outcome logs
`native_resolution_incomplete` and uses the original window.
Non-Governor motions remain native. Actual game verification is pending.


**Operator input traces and F10 WM_SYSKEYDOWN (8 Nov):**
`autosim_input` records `F9`, `F10`, and `CTRL_SHIFT_F12`
with latch source `physical_latched`, `queued_latched` or
`winproc_latched`, plus one `BOUNDARY_REQUEST` with request ID,
eligibility and full-cycle state when the turn driver consumes any
queued request. Both keyboard paths now accept `WM_SYSKEYDOWN` for
F10 in addition to `WM_KEYDOWN`. Physical polling requires game
foreground; while another application has focus use
`autosim-stop.txt` in the executable directory to trigger the next
safe-boundary Save/Exit UI. This only adds input telemetry; native
Council vote logic is unchanged. Test Windows behaviour separately.


### AUTO-002C Council proposal generalisation (8 November 2026; experimental)

**Only Council voting is in scope.** On existing draft PR #11, the
Windows-tested Governor-only `CouncWin_make_resolution(win,1)` headless path
now accepts **all ten stock `CouncilProposal` IDs 0–9** (Governor, Supreme
Leader, Unity Core salvage, Global Trade Pact/repeal, solar shade launch/
increase, polar cap melting, UN Charter repeal/reinstatement). Binary review
of the supplied `terranx.exe`: `CouncWin_init` at `0x4278E6..0x427974`
precomputes nonhuman ballots irrespective of proposal; native
`CouncWin_make_resolution` at `0x424410` handles Governor candidate tally,
Supreme Leader threshold, ordinary motions, veto and the Council-action
virtual call. The hook does not recompute AI ballots, write Council state,
invent vote results, or fake popup responses. Unknown proposal IDs, invalid
mode or native unresolved ballots still take the original modal path.

The separate flushed `council.jsonl` now includes stock proposal name,
proposal ID, candidate, vote array and native weights **before** resolution,
and native return, outcome, per-proposal `ProposalPassCount` before/after,
Governor, pending state, game-state flags and victory-type **after**. These
are diagnostic snapshots, not verified AI vote parity. Supreme Leader
approval may set a native victory flag and encounter the **unchanged**
end-of-game presentation/save lifecycle; terminal saving, winner attribution,
and automatic camera reassignment if the observed faction is eliminated are
**deferred** and not implemented in this change. Test all categories of
vote and ordinary human Council separately before merge.


### AUTO-LAUNCH Stage A — isolated pre-game workflow (8 November 2026)

Install the CI Develop artifact's `thinker.dll`, `thinker.exe`,
`Run Automation.bat` and `Run Automation.ps1` together in the SMAC/X
folder. Double-click `Run Automation.bat`. Windows PowerShell opens a
native pre-game WinForms dialog with **New Game** (the regular native game
setup, manual Scenario Editor AI handover still required) or **Load SAV**
(start a chosen existing file directly). Choose campaign display name,
full AI turn budget, telemetry, notice suppression, fast presentation.
Use the existing F9/F10 control menu after handing all factions to AI.
Cancelling the launcher does not start the game. No extra executable or
Python installation is required.

The GUI persists editable defaults to `automation.ini`, *not* either
`thinker.ini` or `thinker_user.ini`. Each Start operation also writes
a unique, immutable `automation/profiles/<timestamp>-<id>.ini` snapshot,
passed via `THINKERNECA_AUTOSIM_PROFILE` in the environment of the same
`thinker.exe`; it is inherited by `terranx.exe` because the Thinker
launcher uses CreateProcess with inherited environment. The DLL reads only
this explicit profile, validates `[automation]` section and schema and
fails closed on invalid profiles or invalid selected SAV. Old automation
keys in normal `thinker.ini` settings are deliberately reset to 0 when
no such profile is provided; **all other normal game settings remain**.
The run-profile fingerprint is recorded in session manifest metadata.
On rejoining a named campaign, the selected launch budget/checkboxes
supersede older options stored in the save's lineage sidecar.

**No changes yet to saves or automation lifecycle:** native autosaves and
manual saves still write their former shared paths; sessions/campaign
folders retain existing naming; no ZIP/archive/checkpoints. New-game map
setup/handover is still manual. No batch controller, victory final SAV,
terminal reporting or observer replacement is part of Stage A.


### AUTO-NULL / AUTO-NEW trial: Null Rendering, monitor, zero-input New Game

`Run Automation.bat` opens an approximately 1.5x larger four-tab
WinForms launcher: **Game & campaign**, **World generation**,
**Presentation & logs**, **Monitor & recovery**. New-profile options:
`render_mode=0` normal, `1` fast, `2` Null; size 0–4,
ocean/erosion/cloud/native life 0–2, difficulty 0–5, observed slot 1–7,
monitor warning 30–60 s (default 45). The existing run-profile transport
remains process-local; normal thinker.exe has these settings disabled.

The first Null Rendering implementation only installs native x86
entrypoint gates when an explicit launcher profile selects Null. It
preflights and preserves whole original prologues for
`draw_tile`, `draw_tile_fixup`, `draw_tile_fixup_2`, `draw_tiles`,
`draw_map`, `MapWin_draw_map`, and `MapWin_draw_tile`; when not
actively suppressing, executable gateways call the original routines.
`do_all_draws` is deliberately **not** skipped because it also invokes
task/event dispatch. This is an experimental renderer-only bypass,
not a guaranteed zero-cost or visibility-neutral implementation; Windows
state parity is required. The main turn loop enables the gate only
inside all-AI cycles, turns it off for the operator menu and at exit.
Unclassified `X_pop` dialogs attempt to restore drawing before entering
the native modal. Other popup entrypoints can still block without it.

The DLL atomically writes `<run-profile>.status` with game PID,
phase, year/turn, faction, render status and human mask; the PowerShell
console uses its file update as a progress heartbeat. It warns at
the configured no-progress threshold without killing the game.
Console shortcuts: `G` Show Game, `R` Restore Rendering at next live
game phase, `N` Null at next live phase, `P` capture current game
window PNG under `automation/screenshots/`, `S` request stop after
completed cycle, Space request pause, `Q` close monitor without closing
game. If already stuck in a native modal, the game thread cannot
consume restore commands; G/P only inspect any current/stale GUI.

New Game is **experimental**. The launcher feeds quick-start native
`map_menu(1)` and `config_game(1)` from explicit profile choices,
so it does not use TOPMENU/DIFFICULTY/WORLDSIZE interactive dialogs.
Unlike ordinary quick-start, human mask is zero before native
`setup_game`, which allocates all starts by the AI route; the observed
faction is merely a valid viewer and player-only `planetfall` is skipped.
Run-local settings are not saved into the ordinary player INI.
Successful invariant validation skips the old in-game START prompt.
No custom roster/fixed seed yet, no victory lifecycle or multi-run
scheduler. **Native Windows setup and AI parity unverified.**


### Early-start launcher diagnostics (8 Nov 2026)

After an early-console-exit report, `Run Automation.bat` now shows
PowerShell's exit code and **waits for a keypress on all exit paths**,
including a crash/cancel. `Run Automation.ps1` starts a timestamped
independent `automation/logs/launcher-*.log` transcript before showing
WinForms. It logs the immutable `automation/profiles/<id>.ini` path,
`thinker.exe` PID, and the expected `<profile>.pid` and
`<profile>.status` paths. A dead Thinker launcher with no game PID
is reported explicitly; early PowerShell exceptions and stack traces
also print in the console. Run with `-GuiSelfTest` for an actual
Windows PowerShell 5.1 WinForms-construction check in Windows CI.
The resulting console log is independent of campaign telemetry because
`campaigns/` is only created after the game successfully initialises.
This is diagnosis and observability, not proof that the previous
game-start failure was fixed.
