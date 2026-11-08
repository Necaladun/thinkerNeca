#!/usr/bin/env python3
"""TEL-ORBIT: structural contracts for the initial orbital JSONL stream.

These checks do not execute the native Win32 hooks or prove in-game RNG parity.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
src = {name: (root / name).read_text(encoding="utf-8") for name in (
    "src/telemetry.h", "src/telemetry.cpp", "src/base.cpp",
    "src/faction_event.cpp", "src/gameturn.cpp", "src/gui_dialog.cpp",
    "src/patch.cpp",
)}
telemetry = src["src/telemetry.cpp"]
header = src["src/telemetry.h"]
base = src["src/base.cpp"]
enemy = src["src/faction_event.cpp"]
turn = src["src/gameturn.cpp"]
ui = src["src/gui_dialog.cpp"]
patch = src["src/patch.cpp"]

# Every session creates and closes the stream, and recorder failures close it.
for token in (
    'FILE* orbit_file = NULL;',
    'orbit_file = fopen((session_dir + "/orbit.jsonl").c_str(), "wb");',
    'if (!turns_file || !bases_file || !ui_notices_file || !orbit_file',
    'if (orbit_file) {',
    'fclose(orbit_file);',
    'if (!telemetry_orbit_snapshot("session_start")) return;',
    'if (!telemetry_orbit_snapshot("completed_turn")) return;',
    'last_committed_turn = turn;',
    'if (!append_line(orbit_file, line)) {',
    'recorder_failure();',
    '"inventory_snapshot"', '"launch"', '"attack_decision"',
    '"attack_result"', '"environmental_loss"',
):
    assert token in telemetry, token
assert telemetry.index('if (!telemetry_orbit_snapshot("completed_turn")) return;') < (
    telemetry.index('last_committed_turn = turn;'))
for token in (
    '\"seq\":', '\"turn\":', '\"mission_year\":', '\"processing_faction\":',
    '\"attacker_odp\":', '\"reserve_required\":',
):
    # Backslash escaped C++ JSON keys are verified below instead.
    assert token.replace('"', r'\"') in telemetry, token
assert 'std::ostringstream out = orbit_record(' in telemetry

# The recorder does not call random/AI APIs for decisions or derived outcomes.
orbit_section = telemetry.split('// Dedicated orbital event ledger.', 1)[1].split(
    'void telemetry_record_ui_notice(', 1)[0]
for forbidden in ('game_rand(', 'map_rand(', 'random(', 'satellite_goal(',
                  'faction_might(', 'action_sat_attack('):
    assert forbidden not in orbit_section, forbidden

# Launch only after production actually increments an orbital inventory.
start = base.index('if (is_satellite) {', base.index('base->state_flags |= BSTATE_PRODUCTION_DONE;'))
end = base.index('if (!(plr->player_flags & PFLAG_MAP_REVEALED))', start)
launch = base[start:end]
assert launch.index('plr->satellites_ODP++;') < launch.index('telemetry_orbit_launch(')
assert 'orbit_after_count - 1' in launch

# AI attack remains native and called exactly once; trace independent of UI flag.
start = enemy.index('while (plr.satellites_ODP - plr.ODP_deployed > max_busters)')
end = enemy.index('void __cdecl enemy_capabilities(', start)
attack = enemy[start:end]
assert attack.count('action_sat_attack(faction_id, i, sat_type, base_id);') == 1
assert 'telemetry_orbit_begin_attack(faction_id, i, sat_type, base_id, max_busters);' in attack
assert 'telemetry_orbit_end_attack(orbit_trace, faction_id, i, sat_type, base_id);' in attack
assert attack.index('telemetry_orbit_begin_attack(') < attack.index(
    'action_sat_attack(faction_id, i, sat_type, base_id);') < attack.index(
    'telemetry_orbit_end_attack(')
assert 'const bool trace_satellite = autosim_ui_active() && conf.telemetry_enabled;' in attack

# Environmental effects occur before loss event records; preserve notifications.
for cause in ('solar_storm', 'asteroid'):
    assert f'telemetry_orbit_environment_loss(i, "{cause}"' in turn
assert turn.index('Factions[i].satellites_ODP = 0;') < turn.index(
    'telemetry_orbit_environment_loss(i, "solar_storm"')
assert turn.index('Factions[i].satellites_mineral = 0;') < turn.index(
    'telemetry_orbit_environment_loss(i, "asteroid"')
assert 'autosim_notice_pop2("SOLARSTORM"' in turn
assert 'autosim_notice_pop2("ASTEROID"' in turn

# Exact guarded native result callsite, callback-preserving normal-mode path.
assert 'write_call(0x4CF70F, (int)autosim_orbital_result_popp);' in patch
assert 'if (!memcmp((const void*)0x4CF70F, orbital_result_expected,' in patch
assert 'if (autosim_ui_active() && !fn && label' in ui
assert 'return popp(filename, label, a3, image, fn);' in ui

print("TEL-ORBIT event stream and native-result source contracts passed")
