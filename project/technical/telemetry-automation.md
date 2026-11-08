# Campaign telemetry and automated AI games — design and implementation status


## TEL-LOG-01 Council political events and Secret Project lifecycle (9 November 2026; draft PR #11)

**Scope / evidence:** implemented in the still-unmerged PR #11 source, with host/synthetic tests and CI; no native Windows verification of the new streams yet. This section supersedes the *older Council UI-trace design* below. It does **not** change the existing Council native hook preflight, AI ballot selection, make-resolution invocation, modal fallback, vote-bribery policy, popup suppression or project production choices.

- **Council gameplay:** \`council.jsonl\` now holds typed \`proposal\`, \`ballots\` and \`resolution\` events sharing a per-session \`meeting_id\`. The hook observes the native \`CouncWin_init\` result after the engine computes votes. The raw \`faction_vote[1..7]\`, \`faction_vote_count[1..7]\`, mask, candidate and outcome are preserved. A positive or negative native ballot code (including \`-3\`) is **not translated** without verified meaning. Each ballot has \`reason:null\`; \`reason_status\` distinguishes \`native_ai_reason_not_instrumented\` for AI factions from \`human_choice_not_instrumented\` for human-controlled factions, with separate alive/human flags. Neither AI reasoning nor vote input scoring is implemented yet. The \`convening_faction\` comes only from the currently wrapped \`call_council(faction_id)\` path; an unknown source is \`null\`. \`proposer_faction\` is **always null pending verification**; the observed camera is not assumed to be sponsor. The native result is logged after the headless \`CouncWin_make_resolution\` or native modal returns, with raw \`outcome_raw\`, governor/pass-count/game-state before-and-after where captured; unresolved votes do not claim a result. Native pre-resolved outcomes are handled after \`CouncWin_init\` without another call to the vote or resolution engine.
- **Council diagnostics:** routine \`call_council\` entry/exit and \`CouncWin_init\` entry/exit with all 8 raw arrays/energy snapshots are no longer written to \`council.jsonl\`. Only meaningful UI guards (e.g. \`modal_fallback\`, omitted human-only offers, suppressed Council announcements, native vote-offer result) flow via \`ui_notices.jsonl\` as \`source="council"\`. Other existing automation UI logs remain in \`ui_notices.jsonl\`; this stream is **not a gameplay ledger**.
- **Secret Projects:** \`projects.jsonl\` tracks active queue transitions (\`started\`, \`changed\`, \`abandoned\`, \`interrupted\`) and actual \`completed\` at \`src/base.cpp::mod_base_production\` immediately after the authoritative \`SecretProjects[project_id]\` assignment. Events include a per-session event ID, turn/year, faction, facility/project name/ID, base name and coordinates, old/new project choices and a source/cause label. \`src/base.cpp::mod_base_change\` observes immediate queue changes; full-cycle reconciliation catches direct queue writes and base ownership/deletion that bypass that call. Coordinates+owner avoid mistaking recycled native base indices for stable IDs. Existing in-progress projects at session start are a **baseline**, not falsely dated as new starts. A same-faction project switch is one \`changed\` event, not an additional new start. Interruption on capture/removal is distinct from a voluntary queue abandonment. Full-cycle reconciliation can date direct changes only to the completed cycle, not the exact intra-turn action. The recorded \`change_cause\` is a **source-observed transition**, never an inferred AI strategic motive. Ownership transfers of already-built projects, AI design scoring and arbitrary mid-turn direct queue oscillations remain outside this pass.
- **Reports:** the existing one-window Automation Launcher still calls the same Python \`campaign_report.py\` engine. Generated reports now include Council meetings, individual raw ballots and Secret Project timelines, plus \`council_meetings.csv\`, \`council_ballots.csv\` and \`projects.csv\`. \`ui_notices.csv\` remains available under a collapsed automation-diagnostics section rather than being presented as game statistics. The reader accepts older campaigns lacking either new stream, and does **not** reinterpret old \`phase/detail\` Council debug events as real motions.

**Runtime acceptance:** compare a real Council's proposer/convener/native ballot raw codes and outcome against game-state changes; verify all 10 motions eventually without auto-voting or prompting incorrectly, plus suppression-off/human parity. Test one Secret Project start, switch, abandonment, completion, capture/interruption, direct-queue AI change, and multiple simultaneous builders. Confirm no duplicate starts/completions, no user-notice regression, native saved-game reload in same build, and complete report/ZIP retention. CI alone does not verify native event accuracy.

## AUTO-LAUNCH named campaign folders and pause-excluded AI runtime (9 November 2026; PR #11, Windows runtime pending)

A **new** campaign now receives a filesystem ID `<sanitized-launcher-campaign-name>-YYYYMMDD-HHMMSS`, using the machine's local creation time, with `-2` etc. on same-second collisions. The existing display name is retained in `campaign.json`, and is truncated/sanitized only for the immutable folder ID. Named saves linked to an existing campaign still use their original ID and create a new `session-NNN`; old folders/readers/sidecars remain valid. Editing a campaign's display name **after creation** intentionally does not rename its folder.

Each run accumulates monotonic **active all-AI wall-clock milliseconds**: begin after the startup confirmation when autonomous processing actually becomes eligible; stop before the operator pause/turn-cap/save-control dialog; stop at the *first* native `end_of_game` terminal result **before** the victory dialog and any post-victory saving; resume only on an explicit continuation, then freeze again at any later terminal result. Human takeover stops the clock. This is elapsed real time spent in running AI phases (including game I/O), **not CPU time**; it is per-process/per-session and does not attempt to total multiple restored sessions or include paused wall time. `status.json` and `result.json` gain nullable `simulation_elapsed_ms` and `simulation_time_basis`; subsequent post-victory result rewrites preserve the frozen value. Both in-game control dialogs, the unified Campaign Analyzer session picker and its HTML report show the formatted duration, including for turn-limit exits. Sessions recorded by older builds report `Not recorded`, never a fabricated zero.

Structural check `tools/test_autosim_campaign_naming_timing.py` plus synthetic `tools/test_campaign_report.py`; native acceptance: create two identically named campaigns, check distinct timestamped folders; continue/resume a linked save without renaming its original folder; compare active duration to observed unattended turn processing; leave a victory dialog open several minutes, save, and verify `result.json`/`victories.jsonl` duration stays fixed; explicitly continue after victory and check cumulative time excludes the paused period. Also check a turn-limit dialog and runtime with telemetry disabled (dialog only).

## AUTO-LAUNCH native new-game world and rules controls (9 November 2026; PR #11, Windows runtime pending)

The existing PowerShell WinForms launcher now offers a separate **Game rules & victory** tab and expands **World generation**; no alternate launcher or game executable. Both tabs are disabled for loaded SAVs, and no normal `thinker.ini`, `thinker_user.ini`, `alphax.txt` or `Alpha Centauri.ini` setting is written. All selections are captured in `automation.ini` (GUI defaults) and the immutable per-run `[automation]` profile, which `src/main.cpp::autosim_apply_launch_profile` validates before enabling automatic New Game.

- **Generated Planet:** the existing five sizes Tiny, Small, Standard, Large and Huge plus **Very Huge**, the sixth real `#WORLDSIZE` entry in distributed `docs/alphax.txt` (native X/Y 80×160), and **Custom dimensions** (native X 16–512, Y 16–512). In the game's existing `size_of_planet` custom path, `MapAreaX = 2 * X` for staggered map coordinates; the launcher supplies the custom values directly without opening `CUSTOMMAP`, calls `map_init` and `Path_init`, and retains native world generation. Very large dimensions are source-permitted, not verified for allocation/performance/save reliability. Ocean/erosion/cloud/native-life and difficulty remain selectable.
- **Fixed Planet maps:** the stock `map_menu` branches for **Map of Planet** (`maps\\xplanet.mp`) and **Huge Map of Planet** (`maps\\xplanetx.mp`). These use `mod_load_map_daemon` and `world_climate`, **not** Thinker's random generator; generated size/climate selectors are disabled. Both launcher and DLL fail clearly before entering `map_menu` if the requested stock map is absent, avoiding the native retry loop. Whether a particular installed SMAC/X copy has both files is installation-dependent.
- **Regular custom-game rules:** the 16 stock toggles in `GameRulesOptions` and its inverted Unity Survey option are exposed using their actual `RULES_` bit constants: Conquest, Diplomatic, Economic, Transcendence and Cooperative victories; Do or Die, Look First, Tech Stagnation, Spoils of War, Blind Research, Intense Rivalry, Unity Survey initially present, No Unity Pod Scattering, Bell Curve, Time Warp and Ironman. Two `GameState` options additionally randomise personalities and social agendas. The stock preset is `DefaultRules=0x1A0E`. This is the regular Custom Game Rules menu, **not** all Scenario Editor-specific restrictions such as no Planetary Council.
- **Opt-in compatibility:** `rules_mode=0` keeps previously installed native game-rule preferences; `rules_mode=1` applies precisely the profile's low-16-bit rules and the two native random-leader flags. Old launcher profiles implicitly use `world_source=0`, sizes 0–4 and `rules_mode=0` with no override. Loaded-SAV profiles do not affect these game-creation settings. Normal launches without a profile remain unchanged.

**Evidence boundary:** source wiring, host regression and Windows GUI-construction smoke are not in-game validation. Windows acceptance includes the sixth preset's reported map size, at least two custom X/Y examples, both fixed Planet map files and missing-file failure, a custom victory/rules combination checked against resulting `GameRules` and `GameState`, native faction-agenda randomisation, new-game save/reload in the same build, loaded-SAV isolation, and normal unlaunched human play. Use fresh test campaigns; changing map source or random-leader options deliberately changes RNG progression.

## TEL-ORBIT basic orbital events and reachable C++ result suppression (9 November 2026; PR #11)

**Corrected popup route:** `patch.cpp` already redirects the original `terranx.exe` `action_sat_attack` at `0x4CF480` to `src/basewin.cpp::action_sat_attack()`. The uploaded EXE does contain a direct native `popp` call at `0x4CF70F`, but that original body is unreachable in a patched game. The initial PR #11 callsite hook at `0x4CF70F` was therefore ineffective (confirmed by user-observed `WEKILLEDIT` popup on build `194a155`). **Fixed:** replace the four direct, return-unused `popp` statements in the active C++ function with `autosim_notice_popp` for `WEKILLEDIT`, `THEYKILLEDIT`, `WEMUFFEDIT`, `THEYMUFFEDIT`; delete the mistaken native hook/wrapper/declaration. This preserves `game_rand() % 100 >= OrbitalAttackChance`, attacker ODP loss/deployment, target satellite or GSP mutation, and the native presentation fallback for ordinary humans/suppression off. The older `X_pop/X_pops` allowlists remain separate but do not handle these C++ calls.

**Event stream:** `campaigns/<campaign>/sessions/session-NNN/orbit.jsonl` is created whenever `telemetry_enabled=1` under the existing single-player recorder rules, independent of `autosim_suppress_notices`. Initial JSONL version 1: common `seq,event,utc,turn,mission_year,processing_faction`; `launch` with actual base/type/quantity change, `attack_decision` with reserve, target, inventories and diplomatic flags, `attack_result` linked by session-local `attack_id` with before/after counters and `result_basis=inventory_delta`; `environmental_loss` for solar storms and asteroids; and `inventory_snapshot` per session start and completed turn (all factions, satellite types, deployed ODPs, GSP/Flechette counts). The recorder is read-only and flushed immediately; an interrupted attack may have a decision without a result. Existing UI notices remain in `ui_notices.jsonl`.

**Deferred:** `production_selected`, rejected-target logs, complete all-entrypoint orbital combat decisions, real attack roll capture, defensive ODP/Flechette missile event records, inventory reconciliation and Analyzer orbital charts. AI attack event records currently cover `enemy_strategy`, not manual Orbital Attack View calls. Source/CI checks do not establish Windows game-state parity; see `TEL-ORBIT` in WORK-QUEUE and TESTING.

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

### AUTO-002 orbital satellite attack/defence UI (8–9 November; corrected active source)

The source-owned satellite lifecycle is separate from offensive combat: `base.cpp` increments nutrient/mineral/energy/ODP counters, handles first-orbit reveal and milestone, and uses guarded completion notices. `gameturn.cpp` environmental solar storms remove energy/ODP and asteroids remove mining satellites, with notices only after the actual inventory changes. Orbital troop insertion `MADEORBITAL` is unrelated to satellite production.

The **active orbital combat** handler is `basewin.cpp::action_sat_attack()`, installed by `write_jump(0x4CF480, (int)action_sat_attack)` in `patch.cpp`. It resolves hit/miss using `game_rand`, applies ODP and satellite/GSP state changes, then invokes one of four return-unused `popp` notices for the observed attacker/defender. The earlier no-callback `X_pop` classification never reached these direct C++ `popp` calls. The briefly added original-native `0x4CF70F` callsite hook was also unreachable; both are documented as the reason for the subsequent Windows popup regression. Four `autosim_notice_popp` calls now cover those results in eligible all-AI notice suppression. Manual Orbital Attack View's `NOKILLERS`, `KILLERSDEPLOYED`, and `WHICHGSP` selection remain native.

The AI orbital loop in `faction_event.cpp::enemy_strategy` continues to invoke `action_sat_attack` exactly once per attempt, with `satellite_native/action_sat_attack` entry/exit diagnostic records under all-AI+telemetry, plus the new dedicated `orbit.jsonl` decision/result pair under telemetry alone. These records do not change RNG or target selection.

Defensive ODP/Flechette `shoot_it` logic for fungal/tectonic missiles has source-owned guarded passive results, while human `FUNGALSAC`/`TECTONICSAC` choices remain active. Planet Buster `BUSTERSAC` sacrifice remains a real choice. Existing `X_pop/X_pop_2` classification for passive Planet Buster reports does not imply all native callers were validated. Source tests now establish the reachable C++ orbital route, but Windows proof of no interruptions and state parity is still pending.

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
4. **Windows GUI (unified PR #11):** run `Run Automation.bat`, then use the `Campaign tools & archive` tab inside the **same Automation Launcher** window. Browse/filter recorded sessions, inspect status/source-save/faction details, generate and open a report, open an existing report or campaign folder, export a whole campaign ZIP, or open launcher logs. Python 3 (standard library) is required for campaign scan/report generation; the launcher and ZIP functions remain PowerShell/WinForms and require no Python. The CLI reporter `tools/campaign_report.py` remains available to scripts. Legacy `Campaign Analyzer.pyw` remains in the repository for previous installations but is no longer launched or packaged.
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


**Null Rendering startup protection-order fix (9 Nov):** `patch_setup`
now calls `autosim_install_null_render_hooks` while the native executable
image is writable, *before* restoring `PAGE_EXECUTE_READ`. Previously
`write_jump` attempted to write the seven x86 detours after permissions
were read/execute; this is a likely cause of the game disappearing before
any recorder heartbeat on both New and Load routes. The static regression
checks order; runtime is still unverified.


### AUTO-002 9 Nov: one-run exit and scoped Null drawing

In the first Windows zero-input New Game test, Save & Exit returned to
`control_game()` with native `ControlTurnB=1`. The launcher-specific
`autosim_auto_new_game` flag was still set, so the next loop generated
another map. An automation launcher invocation now terminates cleanly
after `control_turn()` returns; future multi-game batching must be
orchestrated by the external launcher, not by reentering the native loop.

`X_pop`, `X_pop_2`, `X_pop_6`, `X_pops`, `X_pops_11`,
`X_dialog`, and `X_dialog_alt` previously deactivated null drawing
for a native modal and left it off after that modal returned. They now
use a scope guard to restore the prior suppression state after completion,
keeping the modal visible while blocked. Null's display still has
unhooked paths and may show partial draw artefacts. The live `.status`
adds configured and effective rendering state for diagnosis.
The console requests initial foreground focus after receiving the first
`ai_turn` event; this is best effort under Windows focus-stealing rules.


### AUTO-NULL screen-present gate and instrumentation (9 November)

The initial Windows Null test reaches 100 turns with `configured=2`,
and R/N change the engine's live `restored/null` state, but map graphics
remain visible; no evidence of input-command failure. Seven high-level
map/tile hooks are insufficient. A new, native-prologue-verified
`Win_update_screen` hook at 0x5F7320 skips the on-screen update pass
only during all-AI Null phases, while native `do_all_draws`, messages
and AI visibility data remain untouched. The supported on-disk EXE's
original entry begins with one complete 5-byte
`mov eax,[0x9B7B30]`; all eight sites are preflighted before detours.
The hook's return value and any GUI bookkeeping still require Windows
runtime validation. Run status now includes cumulative actual skipped
and forwarded high-level draws and screen passes. R/N must affect
*actual* counts/visible updates before Null is considered accepted.
A 45s no-progress warning remains active for the operator dialog too,
as deliberate pauses also satisfy the user's desired inactivity alert.

### AUTO-002 native foreign pact / orbital attack results (9 November 2026; Windows test pending)

**Foreign pact notice:** Source-owned `enemies_unpact` already suppresses the one-way `PACTUNPACT` / `ENEMYPACTENDS` notices when all-AI presentation suppression is enabled. However, native `enemies_unpact` (`0x55CE20`) was not previously redirected, allowing native third-party pact renunciation reports to bypass the guard. Draft PR #11 now redirects that native entry to the existing source-equivalent implementation, which clears the two factions' pact/status bits after the report. The separate `pact_ends` / `net_pact_ends` functions and any unit withdrawal remain unchanged. The observer need not be at war with either faction; the source notification condition relates to allied status, infiltration, Governor visibility or omniscient view. A distinct native `ENDPACT*` message is still possible and must not be assumed covered.

**Orbital hit/miss notices (corrected):** The four exact script labels were previously present in the `X_pop` allowlist, but the actual Thinker C++ `basewin.cpp::action_sat_attack` called `popp` directly. After an unsuccessful native-EXE callsite hook (`0x4CF70F`), all four active C++ callsites now use `autosim_notice_popp`. The `X_pop_2/X_pops/X_pops_11` wrappers remain for their own entrypoints; they are not the orbital hit/miss path. Orbital RNG, ODP deployment/destruction, satellite and GSP losses are unchanged.

**Evidence boundary:** source patches and structural checks only. Windows tests must confirm foreign pact notification no longer interrupts, both treaty states and partner-unit withdrawals are correct, and failed ODP strikes do not remove satellites or show a modal. Check `ui_notices.jsonl` for `diplomacy/ENEMYPACTENDS` or `popp` / exact orbital result label. If an orbital popup persists, check the current C++ suppression gate and installed DLL rather than hunting the unreachable EXE route. Compare all-AI suppression-off and ordinary human paths; save/reload and continue with the same build.

### AUTO-OBS Stage 1 — observer-neutrality source audit (9 November 2026; first pass)

**Authority/scope:** draft PR #11 `feature/auto-002-noninteractive-ui`, inspected at `d6eb27532ae746d4c7d0df61dc29dc1a185d2a1a`. Read-only inspection of branch C++ source (especially runtime files changed by #11) and distributed `docs/thinker.ini`. **No gameplay fix, compile or Windows replay is part of this audit.** It does not establish exhaustiveness over untouched code, native `terranx.exe` callsites, installed `thinker_user.ini`, or other runtime configurations.

**Invariants to establish:**
- All faction slots 1–7 use native AI-controlled execution when all-AI automation is active: `FactionStatus[0] & 0xFE == 0`. The current explicit New Game setup satisfies this in `game.cpp::config_game`; the loaded-save/operator fallback must be checked independently.
- `MapWin->cOwner` / `*CurrentPlayerFaction` designate a **camera/presentation observer**, not a game participant entitled to special mechanics. `*CurrentFaction` is the **currently executing** faction. Slot 0 represents native life/Planet, not a spectator slot.
- Changing observer must leave world rules, AI actions/decisions, RNG consumption, economies, combat and victory invariant. Observer-specific rendering, fog presentation and metadata can differ without changing any faction's mechanical knowledge or decisions.

**Confirmed source-dependent paths (the existence of the branch is confirmed, its impact on real Windows games is not yet measured):**

| ID | Class | Source path | Source-level finding and audit action |
| --- | --- | --- | --- |
| OBS-01 | **Rules, critical** | `game.cpp::config_game` around lines 1578–1599 | Before applying the zero-human bitmask, choosing an **alien observer** switches the entire game to no Blind Research, No Unity Survey and Intense Rivalry. This is a confirmed selection-dependent rules mutation; normal human/alien setup should be retained, while all-AI startup should use explicitly selected world rules independent of the observer. |
| OBS-02 | **Game termination, critical** | `faction.cpp::mod_setup_player` around lines 1873, 2109–2164; `mod_eliminate_player` around 2445; `gameturn.cpp::control_turn` around 251–270; `game.cpp::end_of_game` | `faction_id == CurrentPlayerFaction` can turn an ordinary AI elimination into `VIC_LOST_CAPTURE/REMOVE` plus `STATE_GAME_DONE`; the all-AI driver also requires a living view owner. Reassign observer without cancelling the faction's real elimination, distinguish that from world conquest/victory, and ensure no terminal presentation on a recoverable observer loss. |
| OBS-03 | **AI orders, high** | `veh_action.cpp` around 3035–3069 | When the moving transport belongs to `MapWin->cOwner`, code can set embarked units' `ORDER_NONE` and wake transported land units near shore under player UI preference, **without an `is_human` guard**. This is a concrete owner-dependent order write; inspect when both paths are reached by AI and disable human automation for an observed AI while preserving actual AI movement. |
| OBS-04 | **Research timing/RNG, high** | `tech.cpp::tech_advance` around 661–679; `gameturn.cpp::faction_upkeep` around 2651–2654 | `mod_tech_selection` immediately chooses the observed faction's next research inside `tech_advance`, whereas other AI factions can defer until faction upkeep. Selection is stochastic, so timing can change shared RNG order even if the resulting tech choices are broadly equivalent. Record phase-level RNG/selection evidence before fixing. |
| OBS-05 | **Base capture/intelligence, high** | `base.cpp::mod_capture_base` around 435–466 | Capturing an unfinished project base via the observed faction conditionally clears the defender's `secret_project_intel` entry; unobserved AI captors do not take this source branch. Determine whether this bit is presentation-only or used by strategy; keep any necessary project bookkeeping symmetrical. |
| OBS-06 | **World/story flags, high** | `base.cpp::mod_base_production` around 3410–3414; `veh.cpp::battle_kill` | First built Mind Worm gets `VSTATE_ASSISTANT_WORM` only when its faction equals the observer, leading to an additional later story/flag chain on death. This is actual unit-state inequality; classify the story effect and ensure an all-AI observer cannot select who gets it. |
| OBS-07 | **Unit orders/flags, provisional** | `veh.cpp::monolith` around 1029–1140; `base.cpp::mod_base_production` around 3440–3442 | Monolith execution uses `is_player` (viewer equality) for some order checks and `GameState` flags rather than strictly actual human control. Newly produced units of the observed faction also get `VFLAG_UNK_1000` (cleared at later upkeep). Trace native/AI reachability and downstream flag consumers before classifying as balance-changing. |
| OBS-08 | **Endgame attribution, critical** | `game.cpp::end_of_game` around 2115–2305; `faction_diplo.cpp` around 3063–3086 | Native victory types include `SOLO/COOP/LOSS` **relative to `CurrentPlayerFaction`**; one conquest-check path also counts surrendering/pacted factions relative to `MapWin->cOwner` (its all-AI reachability remains unverified). Terminal reporting must reconstruct actual winners from authoritative game state and audit native victory branches before displaying or recording results. |
| OBS-09 | **Observer-specific notifications/bookkeeping, provisional** | `gameturn.cpp::turn_upkeep` around 2565–2575; `base.cpp::mod_base_production` around 3705–3885; `tech.cpp::tech_achieved` around 833–1070 | Interludes, warnings and special report flags/tech windows are keyed to observer identity. Some write persistent flags (e.g. Voice of Planet announcement marker); ordinary popup suppression does not prove those paths side-effect-free or RNG-neutral. Audit state writes/return values separately from visual calls. |
| OBS-10 | **Visualization/performance** | `veh_action.cpp` around 2779–2886; `veh.cpp` around 3233–3799; `tech.cpp` around 856–924; `monument.cpp` | Many owner-dependent camera, visibility, movement-animation, prototype notification and monument/report branches explain plausible observer overhead. Not yet benchmarked; avoid rewriting visibility/AI intelligence simply to gain speed. Use engine phase durations, not console poll intervals. |

**Adjacent all-AI / no-human logic (not strictly observer-selection bugs):**

| ID | Source | Effect |
| --- | --- | --- |
| OBS-11 | `base.cpp::can_build` around 4709; `docs/thinker.ini:limit_project_start=3` | Below configured difficulty threshold, AI Secret Project building requires prerequisite tech ownership by **some human bit**; with no humans this can be permanently false. At Librarian level 3 the configured threshold is satisfied, so this does not explain current Librarian tests. |
| OBS-12 | `base.cpp::mod_capture_base` around 561–566 | The size-1 base-capture branch tests whether *any human* can see the base; with zero human bits it takes the same side of the test for every base. This changes AI-only campaign semantics relative to games with a human viewer, even though changing the current observer will not alter it. |
| OBS-13 | `faction.cpp::great_beelzebub/great_satan` around 811–856; `base.cpp::mod_cost_factor/mineral_factor`; `faction.cpp::evaluate_attack`; `faction_event.cpp::enemy_diplomacy` around 744 | The strongest explicit anti-human-threat tests correctly use `is_human` and become inactive with zero human bits. But AI production costs still depend on global `DiffLevel`, and `PFLAG_EXT_SHAMELESS_BETRAY_HUMANS` influences diplomatic aggression without checking that the current target is human. Audit **zero-human balance semantics** separately; these are not evidence that the observer is marked human. |
| OBS-14 | `game.cpp::config_game`, `faction.cpp::mod_setup_player`, `base.cpp::mod_base_support`, `tech.cpp::mod_tech_cost` | Explicit launcher New Game clears all human bits **before faction setup** and thus applies computer pods/satellites, unit-support and research-cost paths to the observed faction too. This is a positive source property, not yet an A/B gameplay equivalence test. An ordinary loaded SAV or operator handover must be treated distinctly. |

**Initial source triage:** prioritize OBS-01/02/03/04/05/06/08 as gameplay or terminal risks; inspect OBS-07/09 for flag or choice effects; treat OBS-10 as performance/presentation; classify OBS-11/12/13 under a separate zero-human rules audit. Many direct owner checks in `probe.cpp`, `faction_event.cpp`, `faction_talk.cpp`, `veh_action.cpp` are conditional presentation (often paired with `is_human`); they must not be globally replaced without checking actual downstream return values, unit orders and visibility.

**Next Stage-1 work before a mechanics patch:** complete cross-repository and untouched-native caller census; resolve the indicated AI reachability and affected flags (especially cargo orders, research selection, base capture, first-worm event, monolith); identify whether additional callbacks consume popup results. Prepare same-save A/B runs differing **only** in observer owner, with verified zero human mask, per-phase RNG state/decision trace, per-faction normalized world snapshot hashes and actual phase timing. Compare first divergence, not merely final win rates. Fixed-seed new-game A/B must additionally hold faction roster/map/rules identical. Normal-human and suppress-notices-off remain explicit regression paths. No assumptions about an available current runtime build or 100% native coverage.

### AUTO-OBS Stage 2 — zero-human rules audit (9 November 2026; source findings)

**Scope:** follow-up to Stage 1 on draft PR #11, inspecting current all-AI source rules and distributed `docs/thinker.ini`, with focus on **Librarian (difficulty index 3)** and **Transcend (index 5)**. This is an **audit**, not a proposed rules rebalance or gameplay patch. Source confirms the branches described below; their live frequency, installed `thinker_user.ini` override values, untouched native paths, and comparative gameplay consequences require Windows evidence. Human gameplay and an optional player-controlled campaign are separate modes. No reason to prioritize Citizen/Talent/Specialist compatibility for all-AI runs unless the scope changes.

**Baseline distinction:** `FactionStatus[0]` is the human-control mask, `FactionStatus[1]` is alive factions; `is_human(faction_id)` reads the former, not `MapWin->cOwner`. Explicit launcher New Game assigns `FactionStatus[0] = 0` before all-AI `setup_game` (`game.cpp::config_game`, 1593–1624; `setup_game`, 1999–2007). `faction.cpp::mod_setup_player` initializes each nonhuman's own `diff_level = 3` (around 1943), even if **global** `*DiffLevel = 5`. Many AI mechanics consult global difficulty and every AI receives those category modifiers; local `diff_level`, leader/personality, faction abilities and game state still vary. A loaded save or Take Control operation may have a different control mask and needs an explicit check.

| ID | Class / assessment | Verified source and consequence for zero-human games |
| --- | --- | --- |
| ZH-01 | **AI economic presets: expected** | `docs/thinker.ini` uses `cost_factor=13,12,11,10,8,7`, `tech_cost_factor=124,116,108,100,84,76`, `unit_support_bonus=0,0,0,0,1,1`, `content_pop_computer=3,3,3,3,3,3`. At Librarian vs Transcend: production factor **10 vs 7**; AI research factor **100% vs 76%** with `revised_tech_cost=1`; bonus support **0 vs +1 per base**; same content allowance **3 vs 3**. Confirmed in `base.cpp::mod_cost_factor/mod_base_support/base_psych_content_pop` and `tech.cpp::mod_tech_cost`. All seven AI slots use the same **class of modifier**, not necessarily identical totals. |
| ZH-02 | **AI maintenance: expected** | `gameturn.cpp::faction_upkeep` around 2881–2896 refunds **0** of maintenance on Librarian but **2/3** of accumulated facility maintenance on Transcend (1/3 on Thinker difficulty index 4). This is an additional Transcend AI economy bonus often omitted from simple cost comparisons. Applies per nonhuman faction. |
| ZH-03 | **Human-relative production bonus: expected inactive** | `base.cpp::mod_cost_factor` and `mineral_factor` each subtract up to two points from the AI cost factor when the highest-ranked faction is a human / a `great_satan` threat and `simple_cost_factor=0`. With no humans, both conditions evaluate false. Thus at Transcend a 7 stays 7 rather than becoming 6/5 through the *anti-leading-human* clauses. This differs from an otherwise similar game with a dominant human; it does **not** privilege the observer. |
| ZH-04 | **AI expansion cap: notable world rule** | `faction.cpp::allow_expand` around 198–221 uses the maximum **living human** base count to scale `expansion_limit` when `expansion_autoscale > 0`. With zero humans, this comparison is always 0; distributed `expansion_limit=50`, `expansion_autoscale=1` therefore imposes a **fixed 50 (bases + active colony pods)** AI expansion threshold. With a human above 50 bases, the AI cap could increase above 50. Symmetric across AIs but materially different long-run dynamics; determine whether an *AI-leader-based* autoscale or fixed cap best serves unattended tests before changing it. |
| ZH-05 | **Size-one base capture: notable world rule** | `base.cpp::mod_capture_base` around 561–566 has a special single-player condition `base->pop_size <= 1 && !(base->visibility & FactionStatus[0])`. With human mask zero, all size-one bases meet this condition regardless of which AI is involved or whether the observer can see them: the branch **preserves the base on capture**, where an observable size-one city can instead be decremented to zero and destroyed. Other branches (`is_probe`, recapture, objective etc.) also affect fate. This is observer-neutral among AI factions but not semantically identical to a human-in-game campaign. The source does not document a design rationale; do not infer intent. |
| ZH-06 | **Anti-human threat / climax: expected inactive, strategy gap** | `faction.cpp::great_beelzebub/great_satan` around 811–856 only identifies **human** runaway leaders; `climactic_battle` around 895–914 only becomes true for human-triggered economic/Planet conditions; `at_climax` around 921–950 requires it. With no humans, `at_climax` always returns false. This can disable AI emergency-threat diplomacy against an AI about to win, even though AI economic, diplomatic, conquest and transcendence **victory mechanics are still present**. Whether AIs should respond to *any* near-victor is a separate AI strategy design decision (not an observer-correction patch). |
| ZH-07 | **Difficulty and 'anti-human' aggression flag: deliberate/uncertain** | `faction.cpp::mod_setup_player` around 2211–2221 may give an AI `PFLAG_EXT_SHAMELESS_BETRAY_HUMANS` and atrocity flags when Intense Rivalry is set **or global difficulty is Thinker/Transcend**, subject to personality and RNG. `faction_event.cpp::enemy_diplomacy` around 740–749 and `faction_diplo.cpp::diplomacy_check` around 581–600 use this flag against AI targets without requiring `is_human(target)`. It is therefore *not* automatically inactive in an all-AI game; at Transcend it can make some AIs more aggressive toward other AIs. Source confirms the path, not whether its name reflects intended design. Avoid indiscriminate clearing. |
| ZH-08 | **Stale human-team flags: conditional risk** | `faction.cpp::evaluate_attack` around 1637–1639 returns false against a nonhuman target if attacking faction still has `PFLAG_TEAM_UP_VS_HUMAN`; `faction_diplo.cpp::diplomacy_check` around 75–81 and 599–600 also use `PFLAG_TEAM_UP_VS_HUMAN`/`PFLAG_COOP_WITH_HUMAN` without validating the counterpart is currently human. `mod_setup_player` clears `plr->player_flags` at fresh initial spawn; this audit has **not established a path that sets these flags in a fresh zero-human New Game**. However, after an actual human handover or old save they could persist and alter AI-vs-AI decisions. Check real loaded-save flags and setters/native callers before treating as a bug. |
| ZH-09 | **AI probe and military opponent class: expected** | `probe.cpp` has human-target-dependent discounts/mind-control thresholds (e.g. around 151–160 and 731–785); `veh_combat.cpp` around 1563–1574 has human attacker/defender difficulty-specific offense adjustments. With no human-controlled factions, those *human-targeted* branches are inactive; ordinary AI-vs-AI combat/probe logic still applies to every participant. The difference from an AI-vs-human game is expected, not proof of a hidden observer advantage. |
| ZH-10 | **Diplomacy human-only paths: expected, affects campaign behaviour** | `faction_event.cpp::enemy_diplomacy` around 628 and 673–800 uses an AI-to-AI encounter/trade path when `!is_human(partner)`; the separate human territorial request path is skipped entirely with zero humans. `gameturn.cpp::random_events` around 1391–1398 skips clearing human-targeted betrayal flags when no humans exist, but other random events proceed. This models AI-to-AI diplomatic behaviour, not the interaction pattern a human would see in their own negotiations. |
| ZH-11 | **AI production/psych/behaviour baseline: expected** | `faction.cpp::thinker_enabled` and `mod_setup_player`, `gameturn.cpp::enemy_diplomacy/call_council`, `base.cpp::base_psych_content_pop`, `tech.cpp::mod_tech_cost` all branch by `is_human`; with zero mask the observed slot is subject to AI starting pods/satellites, AI planning, computer content/drone allowance, AI research and Council path. **Stage 1 OBS-01** remains an exception: `game.cpp::config_game` applies alien-*observer*-dependent global world rules before the zero-human mask. |
| ZH-12 | **Scenarios: deferred** | `game.cpp::end_of_game` around 2172 checks whether the victorious faction is human for solo-mission scenario objectives. All-AI scenarios may therefore not preserve special human-only objective semantics; the fork prioritises fresh ordinary games and scenario support is explicitly out of scope unless separately requested. |
 
**Judgement for requested simulations:** Librarian and Transcend are both suitable **AI-category** configurations. Neither is a pure replication of fighting a human player: anti-human threat/cost adaptation and explicit human diplomacy/spy discounts disappear, while a difficulty-dependent *Shameless Betray Humans* flag may still affect AI-vs-AI diplomacy. The two largest **simulation-wide but observer-neutral** deviations warranting a design decision are the **fixed 50-base AI cap** (ZH-04) and **size-one base survival** (ZH-05). **No stage-2 finding warrants silently removing difficulty modifiers or overwriting standard AI bonuses.** Prioritize fixing Stage 1 observer-dependent simulation changes before comparative A/B evidence; stage 2 should be verified against actual configured `thinker.ini` + `thinker_user.ini` and runtime snapshots rather than assumed from the distributed profile.

**Stage-2 test matrix (source-derived, not executed):** (a) zero human mask + two different observers from same world; (b) Librarian index 3 vs Transcend index 5, checking individual per-faction AI modifiers and maintenance actuals; (c) base expansion at 49–51 bases with varying colony pods; (d) size-one base capture in unobserved and visible contexts with no humans, plus human-present control; (e) AI near Corner Market, Supreme Leader and Ascent to test `climactic_battle/at_climax` vs real victory; (f) fresh zero-human vs human-to-AI handover/save reload with PFLAG logs; (g) human game unchanged. Inspect native state and RNG parity before any patch. Lower-than-Librarian Secret Project gating (Stage 1 OBS-11) remains documented as **deprioritized**, not silently fixed.

### AUTO-OBS Stage 3 — targeted all-AI implementation (9 November 2026; draft PR #11)

**Source implementation, not native Windows acceptance.** The user approved an *explicit Automation Launcher override* of Thinker's AI colony expansion cap without changing distributed `thinker.ini` or normal-game defaults. The work extends existing draft PR #11; no separate PR and no merge.

- **Automation-only unlimited expansion:** `Run Automation.ps1` has a checkable `unlimited_expansion=1` option (default on for new launcher profiles; old profiles default off), propagated by `src/main.cpp` into runtime-only `conf.autosim_unlimited_expansion`. `faction.cpp::allow_expand` ignores `expansion_limit` solely if this toggle is enabled **and** `autosim_control_active()` confirms zero human bits, valid living observer, ongoing game and explicit launcher. Scenario `RULES_SCN_NO_COLONY_PODS`, global `MaxBaseNum`, AI economics, faction talents and normal games remain untouched. This makes high-base-count colony production *eligible*, not inevitable: AI strategy and valid colony placement may still limit growth.
- **Observer setup/research/actions:** all-AI New Game no longer acquires Progenitor observer's Intense Rivalry/No Unity Survey/Blind Research override; observed AI now follows unobserved AI tech selection timing, cargo wake-up and monolith behaviour. AI-only story Mind Worm/Voice-of-Planet flags, project-intel early returns and human SOCIETY notices are no longer assigned solely by camera identity. Base-capture HQ relocation and base-zoom defaults select the AI route independently of optional notice suppression. This is a targeted source pass, not a proven exhaustive native observer-dependency census.
- **Eliminated AI observer:** `mod_setup_player` treats an observed AI as AI, executes normal elimination, then moves camera to next living slot 1–7, logging `observer/REASSIGNED`. Conquest count is evaluated relative to the leading surviving AI rather than an arbitrarily selected camera faction; extinction is recorded without fabricating a winner. Native normal-human loss remains. This path needs in-game mid-upkeep testing, particularly the special ranking/surrender/cooperative conquest logic.
- **ENDPACT modals:** `faction_talk.cpp::pact_ends` completes `treaty_off` and both `pact_withdraw` calls first, then skips `ENDPACT1/ENDPACT2/ENDPACT3` popup construction in an eligible all-AI launcher session, regardless of optional passive notice preference. Other modes take the existing native UI path; no forced response or pact state fabrication.
- **True terminal result:** `game.cpp::end_of_game` retains native terminal detection and victory flags, but in all-AI mode bypasses observer-relative movies/credits/score replay/`GAMEOVERMAN`, writes `session-NNN/result.json` through `telemetry_record_terminal_result`, marks session terminal and exits the one-game launcher cleanly. JSON includes raw native win type, reason, current/last completed turn, partial-cycle flag, initial/final observer, alive mask and each faction's observed state. Winner IDs are populated only when independently grounded (e.g. sole survivor, native winner flags or transcendence monument) and otherwise marked **unresolved** rather than inferred from the observer. The terminal snapshot is NOT a fabricated completed-turn record.
- **Safety boundary for SAV:** no automatic terminal SAV is attempted when a victory can occur mid-upkeep, Council or another faction action. This avoids claiming native serializer safety inside an unverified partial cycle. Existing periodic native autosaves remain available. Result uses `terminal_save.status=not_attempted_partial_cycle` and null path. A future safe terminal checkpoint/reload requires explicit real-game validation.
- **Regression checks:** `tools/test_autosim_stage3.py` checks static guard placement; updated existing source tests cover pact, Mind Worm, launcher profile; Windows build workflow runs these and the existing suites. Passing structural CI does **not** validate native hooks, actual AI parity, source-driven victory cases or save/reload.

**Open:** Different-observer same-SAV A/B decision/RNG checks, alien faction setup roster/rule parity, survivor takeover before/after elimination, diplomatic Supreme Leader/conquest/cooperative/economic/transcendence/alien/time-limit terminals, partial-cycle status, full native GUI and normal human behaviour, pause/stop/reload, Windows x86 Develop/Release/PRACX, and game performance with uncapped AI colony production. Source branches elsewhere can still use `MapWin->cOwner`; stage 1 exhaustive census remains open. All-AI lower-than-Librarian `limit_project_start` is deferred, and no separate AI anti-human bias rebalance is included.

### AUTO-OBS victory pause follow-up (9 November 2026; draft PR #11)

**Trigger/evidence:** user-provided 9 November native Windows run progressed through MY2303, then exited at MY2304 turn 204. The MY2303 SAV held Hive `corner_market_turn=204`; the terminal path is consistent with a genuine economic victory. User independently observed university-to-Morgan and Morgan-to-Peacekeepers observer reassignment, both with zero humans. This is evidence for the **previous** Stage 3 executable, not verification of the newer victory-control code.

**Change:** instead of immediate `*ControlTurnA=1` / process shutdown on the first true world victory, `game.cpp::end_of_game` records the victory, disables Null Rendering while paused and presents `AUTOSIM_DIALOG_VICTORY`, a modal native operator window. A normal victory is never conflated with a paused turn-limit. The operator chooses: Save & Exit, Exit without Save, Save & Continue, Continue AI, or Take Control (continuation actions omitted if no factions remain). The process remains open while deciding. A failed modal attempts explicit retry/cancel rather than silent shutdown.

**Save limitations:** named saves still use transactional `mod_save_daemon` through `autosim_save_named`; the save now can be requested at a completed **native victory phase**, which may fall inside the overall eight-faction turn cycle. It is labelled `saved_post_victory_checkpoint_reload_unverified` and accompanied by a prominent modal warning. Save failure leaves the dialog open and the preexisting target SAV untouched. Actual same-build reload of such terminal/mid-cycle saves remains a **hard acceptance gap**; do not claim that saving is a verified continuation point merely because serialization succeeds.

**Continue semantics:** explicit post-victory *sandbox* continuation clears `STATE_GAME_DONE`, `STATE_FINAL_SCORE_DONE`, conquest/economic terminal control bits and the observer-relative `GameVictoryType` but preserves native historical victory flags, monuments and recorded winner information. The time-limit re-trigger is suppressed if the operator explicitly continues past retirement; alien-specific end-state marker is cleared on continuation. AI turn quota resets at the **next fully completed faction cycle**, avoiding reentrant driver changes inside Council/combat. Take Control sets the observed living faction's human bit after clearing terminal controls.

**Telemetry:** `result.json` reports the latest winning outcome, including actual winner evidence and any successful named victory-save path; `victories.jsonl` keeps an append-only one-event-per-victory history, even after later continuation. Resuming restores `status.json` to `recording`. Exiting leaves status terminal. The launcher progress monitor now reports `victory_paused`, avoids false idle warnings during the intentional modal, and explicitly distinguishes `N` (return to Null Rendering) from `Space` (request AI pause at next completed full turn). Existing console `R` restores rendering. **N never stopped or resumed AI turns in this launcher version**; keyboard input still relies on console focus and phase-level command polling.

**Verification:** source changes plus updated structural test `tools/test_autosim_stage3.py` and Windows CI. Pending: real economic win -> modal, Save & Exit, Save & Continue, direct Continue AI (including turn/quota tracking), Take Control, retry failed SAV writes, actual reload of both victory-save types, second victory after continuation, time-limit continuation, observer death, Council/alien/transcendence mid-upkeep wins, and normal non-automation human game. `N`/R rendering commands must be exercised while live simulation is processing, not while the victory modal is blocking the turn driver.

### AUTO-LOG — timed phase telemetry, checkpoint archives and unified launch/report tools (9 November 2026)

**Source build (draft PR #11; Windows runtime tests pending).** The existing Automation Launcher and TEL-001 session recorder were extended; no additional run executable, new configuration INI, non-launcher change, or general-purpose log database was created. Use `Run Automation.bat` as entrypoint.

**Per-run settings** in immutable `automation/profiles/<timestamp>.ini`:
- `timing_enabled=1` enables native elapsed wall timers; old profiles absent this option default off.
- `archive_interval=25` saves a native-serializer checkpoint in the active session every 25 **completed full faction cycles**; 0 disables. Old profiles absent this option default off. The separate ordinary `saves/auto/` native autosave interval and user choices remain unchanged. The launcher offers 0/5/10/25/50/100; no change to `thinker.ini` or `thinker_user.ini`.
- `faction_1` … `faction_7` explicitly select seven **distinct** stock faction file identifiers. Canonical SMAC IDs: `GAIANS,HIVE,UNIV,MORGAN,SPARTANS,BELIEVE,PEACE`. Alien Crossfire IDs: `CYBORG,PIRATES,DRONE,ANGELS,FUNGBOY,CARETAKE,USURPER`. Exactly seven must be present if explicit selection is used; older run profiles without entries retain native selection. Only new game setup applies overrides, through `read_faction`, `load_faction_art` and `load_flags` before `setup_game`. A SMAX-only identity in original SMAC mode fails visibly; never silently substitutes another faction. Loaded SAV rosters are not modified. Observer remains an independently selected camera anchor without human bits.
- Checkpoints require telemetry enabled. Archive saving is gated to eligible unattended all-AI runs and excludes Ironman because `mod_save_daemon` changes RNG via `game_srand` in Ironman. Default checkpoint writes will use disk and time on huge worlds; tune the interval or disable it for throughput benchmarking.

**New independent session streams**, all JSONL and flushed per record:
- `timings.jsonl`: `phase,faction,turn,mission_year,observer,started_utc,ended_utc,elapsed_ms`. Phase names `world_upkeep`, `faction_upkeep`, `ai_turn`, `full_faction_cycle`, `archive_save`, `named_save_archive_copy`. `GetTickCount` is monotonic for short phase durations; UTC millisecond stamps are for chronology. `elapsed_ms` measures elapsed wall time, **not CPU usage**; cycle and phase timings overlap, so do not sum them. Existing `turns.jsonl` and `bases.jsonl` remain completed-cycle snapshots; no fake terminal turn is emitted.
- `checkpoints.jsonl`: `utc,turn,mission_year,kind,status,path,fingerprint`; differentiates successful checkpoint writes, optional named SAV copies, failed commits, existing-name skips and Ironman exclusion. Fingerprint is byte length plus FNV-1a (not a cryptographic signature). The SAV path is within `sessions/session-NNN/saves/`; `operator_named` copies are retained with `.autosim-campaign` sidecars if available. Automated full-cycle checkpoint names are `checkpoint-Txxxxx-MYyyyy.sav`. Native `saves/auto` is untouched.
- `ui_notices.jsonl` and `council.jsonl` gain `utc` millisecond timestamps (and UI notices get `mission_year`), without changing game event effects. `manifest.json` includes the actual native faction names, faction filename/search keys, difficulty, config fingerprints and new run-profile settings. Existing `result.json`, `victories.jsonl` and session status remain.

**Launcher integration:**
- New `Faction roster` tab with seven validated selectors including SMAX factions; disabled when loading an existing SAV. Separate world-generation controls still determine difficulty, size, terrain/ocean/life and observer.
- Expanded `Presentation & logs` tab controls phase timing and checkpoint cadence independently from rendering/suppressed notices. The existing `Campaign tools & archive` WinForms tab now **contains** the previous Python analyzer's user-facing functions: configurable campaigns root, session table with player/observer, years, turns, bases, status and last update, case-insensitive filter, selected-session metadata, report generation/opening, existing report opening, campaign folder opening, launcher log access, and whole-campaign ZIP export using .NET `ZipFile`. It invokes the existing standard-library Python calculation engine headlessly; no second GUI, Tkinter or independent report window. The script warns that exports made while logging is active may be inconsistent and never auto-stops the game.
- Windows CI Develop artifact now includes `tools/campaign_report.py` only as a backend, alongside `Run Automation.bat` and `Run Automation.ps1`. The old `Campaign Analyzer.pyw` is retained in Git for existing users but no longer packaged. `campaign_report.py --scan-json <campaigns-root>` exposes the established `scan_campaigns()` metadata (with serializable paths) for the launcher; normal CLI generation with `<campaign> --session session-NNN` remains supported. The WinForms smoke tests include a synthetic session scan through the actual PowerShell → Python bridge, independent of game binaries. If Python is unavailable or the reporter cannot start, a read-only PowerShell directory fallback still shows campaign/session folders for opening and ZIP export, but labels detail as unscanned and disables report generation; no Python or metadata is fabricated.
- Offline report exports additional `timings.csv`, `checkpoints.csv` and HTML tables aggregating phase elapsed/maximum durations and listing SAV archives. It supports old sessions without these two optional streams.

**Acceptance NOT complete:** native x86 Develop/Release compile and Windows WinForms GUI smoke are CI evidence only. Needs real 25+ and 100+ turn huge-world run for timed phase throughput, disk overhead and size, correctly sampled on/off timing, native AI/observer equivalence, real saved checkpoint reload at a cycle boundary, mixed SMAC+SMAX seven-slot start under expansion, no-Expansion rejection, campaign ZIP re-extraction and report generation, and normal human-mode regression. The current huge-world run predates this build and is not evidence for these features.

