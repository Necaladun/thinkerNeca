#!/usr/bin/env python3
"""AUTO-OBS Stage 3 structural regressions.

These checks prove source/guard placement only. They are not native Windows
gameplay, victory, saved-game or deterministic A/B validation.
"""
from pathlib import Path

r = Path(__file__).resolve().parents[1]
get = lambda name: (r / name).read_text(encoding="utf-8")

faction = get("src/faction.cpp")
base = get("src/base.cpp")
game = get("src/game.cpp")
tech = get("src/tech.cpp")
vehicle = get("src/veh.cpp")
movement = get("src/veh_action.cpp")
talk = get("src/faction_talk.cpp")
gui = get("src/gui_dialog.cpp")
gui_h = get("src/gui_dialog.h")
telemetry = get("src/telemetry.cpp")
telemetry_h = get("src/telemetry.h")
monuments = get("src/monument.cpp")
main = get("src/main.cpp")
cfg = get("src/main.h")
launcher = get("Run Automation.ps1")
thinker_ini = get("docs/thinker.ini")

def function_chunk(source, start, stop):
    return source[source.index(start):source.index(stop, source.index(start)+len(start))]

def verify(label, condition):
    assert condition, label
    print("PASS:", label)

expansion = function_chunk(faction, "bool allow_expand(", "bool has_plr_rule(")
verify("launcher-only expansion bypass follows scenario/engine maximum checks",
       "RULES_SCN_NO_COLONY_PODS" in expansion
       and "MaxBaseNum" in expansion
       and "conf.autosim_unlimited_expansion && autosim_control_active()" in expansion
       and expansion.index("MaxBaseNum") <
           expansion.index("conf.autosim_unlimited_expansion && autosim_control_active()")
       and expansion.index("conf.autosim_unlimited_expansion && autosim_control_active()") <
           expansion.index("conf.expansion_limit > 0"))
verify("normal thinker.ini does not gain automatic expansion override",
       "autosim_unlimited_expansion" not in thinker_ini
       and "int autosim_unlimited_expansion = 0;" in cfg
       and "cf->autosim_unlimited_expansion = 0;" in main
       and "cf->autosim_unlimited_expansion = p.unlimited_expansion;" in main
       and '"unlimited_expansion"' in main
       and "unlimited_expansion = '1'" in launcher
       and "unlimited_expansion=if ($unlimitedExpansion.Checked)" in launcher)

pact = function_chunk(talk, "void __cdecl pact_ends(", "void __cdecl pact_of_brotherhood(")
verify("pact and both forced withdrawals execute before mandatory unattended UI return",
       "treaty_off(faction_id, faction_id_2, DIPLO_PACT);" in pact
       and "pact_withdraw(faction_id, faction_id_2);" in pact
       and "pact_withdraw(faction_id_2, faction_id);" in pact
       and pact.index("pact_withdraw(faction_id_2, faction_id)") <
           pact.index("if (autosim_session_active())")
       and pact.index("if (autosim_session_active())") < pact.index("draw_map(1);")
       and "ENDPACT1/2/3" in pact and "BasePop_exec_3(&cur_popup" in pact)

setup = function_chunk(faction, "int __cdecl mod_setup_player(", "int __cdecl mod_eliminate_player(")
elim = faction[faction.index("int __cdecl mod_eliminate_player("):]
verify("observed AI defeat reassigns to a living nonzero player slot",
       "is_player = faction_id == *CurrentPlayerFaction && !autosim_session_active();" in setup
       and "const bool unattended = autosim_session_active();" in setup
       and "is_alive(id)" in setup
       and "MapWin->cOwner = replacement;" in setup
       and setup.index("set_alive(faction_id, false);",setup.index("if (!is_probe && (spawn_flag || spawn_skip))")) <
           setup.index("MapWin->cOwner = replacement;")
       and "terminal_world_extinction" in setup
       and "conquest_leader = unattended ? other_id : *CurrentPlayerFaction" in setup
       and "num_active - num_allied < 2" in setup)
verify("normal human defeat still exists and AI observer does not trigger escape RNG",
       "X_pop(\"YOULOSE\"" in setup
       and "VIC_LOST_CAPTURE" in setup
       and "is_player = faction_id == *CurrentPlayerFaction && !autosim_session_active();" in elim
       and "&& is_human(setup_id)" in elim)

verify("alien observer cannot change all-AI world rules",
       "&& !autosim_auto_new_game && MFactions[plr_id].is_alien()" in game)
verify("observed AI research follows ordinary AI timing and skips human choices",
       "(faction_id == MapWin->cOwner && is_human(faction_id))" in tech
       and "if (faction_id == MapWin->cOwner && is_human(faction_id)" in tech)
verify("observed AI transport/monolith/story path uses actual human bits",
       "is_human(veh_fc_id) && (veh_fc_id == MapWin->cOwner || *MultiplayerActive)" in movement
       and "is_human(veh_fc_id) && veh_fc_id == MapWin->cOwner" in movement
       and "&& is_human(veh->faction_id);" in vehicle
       and "if (faction_id == player_id && is_human(faction_id)" in base
       and "if (faction_id == player_id && is_human(faction_id))" in base
       and "&& (faction_id == player_id && is_human(faction_id))" not in base)
verify("base capture and project intelligence no longer depend on observed AI",
       "if (faction_id_atk == player_id && is_human(faction_id_atk)) {" in base
       and "if ((faction_id == player_id && is_human(faction_id)) || *SkipTechScreenB)" in base
       and base.count("if (autosim_session_active()) {") >= 2)

ending = function_chunk(game, "int __cdecl end_of_game(", "int __cdecl replay_base(")
verify("actual terminal endgame bypasses native player-only win screens",
       "if (autosim_session_active()) {" in ending
       and "telemetry_record_terminal_result(NULL, \"not_attempted_partial_cycle\");" in ending
       and "autosim_monitor_progress(\"terminal\", 0);" in ending
       and ending.index("if (autosim_session_active())") < ending.index("popp(ScriptFile, \"GAMEOVERMAN\"")
       and "STATE_FINAL_SCORE_DONE" in ending
       and "*ControlTurnA = 1;" in ending
       and "*ControlTurnB = 0;" in ending)
verify("terminal results record partial cycle without synthetic completed turn",
       "telemetry_record_terminal_result(" in telemetry_h
       and "void telemetry_record_terminal_result(" in telemetry
       and '"result.json"' in telemetry
       and "primary_winner_factions" in telemetry
       and "last_committed_turn" in telemetry
       and "terminal_result_recorded ? \"terminal\" : \"closed\"" in telemetry
       and "if (winner) mon_winning_unify(winner, -1);" in ending
       and "!autosim_session_active()" in monuments)
verify("independent all-AI session guard persists past owner death/game done",
       "bool autosim_session_active();" in gui_h
       and "bool autosim_session_active() {" in gui
       and "FactionStatus[0] & 0xFEu" in gui
       and "STATE_GAME_DONE" not in function_chunk(gui, "bool autosim_session_active()", "bool autosim_control_active()"))

print("AUTO-OBS Stage 3 structural checks passed; Windows in-game and save/reload acceptance pending")
