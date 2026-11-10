#!/usr/bin/env python3
"""TEL-ORBIT: structural contracts for the initial orbital JSONL stream.

These checks do not execute the native Win32 hooks or prove in-game RNG parity.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
src = {name: (root / name).read_text(encoding="utf-8") for name in (
    "src/telemetry.h", "src/telemetry.cpp", "src/base.cpp",
    "src/faction_event.cpp", "src/gameturn.cpp", "src/gui_dialog.cpp",
    "src/basewin.cpp", "src/patch.cpp",
)}
telemetry = src["src/telemetry.cpp"]
header = src["src/telemetry.h"]
base = src["src/base.cpp"]
enemy = src["src/faction_event.cpp"]
turn = src["src/gameturn.cpp"]
ui = src["src/gui_dialog.cpp"]
basewin = src["src/basewin.cpp"]
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

# Inventory history must not spam all-zero per-turn records before ANY
# faction constructs a satellite. A loaded SAV with existing satellites gets
# a session-start snapshot; the first launch emits a baseline immediately.
# Once activated it continues even if satellites are subsequently destroyed.
snapshot = telemetry[telemetry.index("static bool telemetry_orbit_snapshot(const char* phase) {"):
                     telemetry.index("void telemetry_orbit_launch(")]
launch_writer = telemetry[telemetry.index("void telemetry_orbit_launch("):
                          telemetry.index("OrbitAttackTrace telemetry_orbit_begin_attack(")]
start_session = telemetry[telemetry.index("void telemetry_begin_session("):
                          telemetry.index("// Dedicated orbital event ledger.")]
assert 'orbit_snapshots_started = false;' in start_session
assert 'if (!orbit_snapshots_started)' in snapshot
for counter in ("satellites_nutrient", "satellites_mineral",
                "satellites_energy", "satellites_ODP"):
    assert f"f.{counter} > 0" in snapshot, counter
assert 'if (!orbit_snapshots_started) return true;' in snapshot
assert snapshot.index('if (!orbit_snapshots_started) return true;') < snapshot.index(
    'orbit_record("inventory_snapshot")')
assert 'orbit_snapshots_started = true;' in snapshot
assert 'telemetry_orbit_snapshot("first_launch");' in launch_writer
assert launch_writer.index("if (!orbit_append(out.str())) return;") < (
    launch_writer.index('telemetry_orbit_snapshot("first_launch");'))
assert 'if (!orbit_snapshots_started)' not in telemetry[
    telemetry.index("OrbitAttackTrace telemetry_orbit_begin_attack("):
    telemetry.index("void telemetry_record_ui_notice(")]

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

# Actual C++ orbital combat is the reachable callback. No native EXE
# callsite instrumentation: patch.cpp redirects the entire entrypoint.
assert 'write_jump(0x4CF480, (int)action_sat_attack);' in patch
assert 'write_call(0x4CF70F,' not in patch
assert 'autosim_orbital_result_popp' not in ui
active = basewin.split('void __cdecl action_sat_attack(', 1)[1].split(
    'void __cdecl sat_attack(', 1)[0]
for label in ("WEMUFFEDIT", "THEYMUFFEDIT", "WEKILLEDIT", "THEYKILLEDIT"):
    assert f'autosim_notice_popp(ScriptFile, "{label}", "satbat_sm.pcx");' in active
assert active.count('autosim_notice_popp(') == 4
assert 'return popp(filename, label, 0, image, 0);' in ui

print("TEL-ORBIT event stream and reachable C++ result suppression source contracts passed")
