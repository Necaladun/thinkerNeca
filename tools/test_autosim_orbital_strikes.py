#!/usr/bin/env python3
"""AUTO-002 satellite lifecycle and active C++ orbital-strike UI contract.

Structural test, not a Windows x86 gameplay test. The native EXE's
action_sat_attack is replaced by basewin.cpp via a write_jump.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
policy = (ROOT / "src/autosim.h").read_text(encoding="utf-8")
ui = (ROOT / "src/gui_dialog.cpp").read_text(encoding="utf-8")
host = (ROOT / "tools/test_autosim_policy.cpp").read_text(encoding="utf-8")
enemy = (ROOT / "src/faction_event.cpp").read_text(encoding="utf-8")
base = (ROOT / "src/base.cpp").read_text(encoding="utf-8")
turn = (ROOT / "src/gameturn.cpp").read_text(encoding="utf-8")
actions = (ROOT / "src/veh_action.cpp").read_text(encoding="utf-8")
patch = (ROOT / "src/patch.cpp").read_text(encoding="utf-8")
engine = (ROOT / "src/engine.cpp").read_text(encoding="utf-8")
basewin = (ROOT / "src/basewin.cpp").read_text(encoding="utf-8")
ui_header = (ROOT / "src/gui_dialog.h").read_text(encoding="utf-8")

# Script.txt: offensive attack results and passive Planet Buster interception.
for label in ("WEKILLEDIT", "THEYKILLEDIT", "WEMUFFEDIT", "THEYMUFFEDIT",
              "WEMUSTSAC", "THEYMUSTSAC", "WESHOTIT", "THEYSHOTIT",
              "WESHOTIT2", "THEYSHOTIT2"):
    assert f'std::strcmp(label, "{label}") == 0' in policy
    assert f'"{label}"' in host
assert 'if (!label || has_callback)' in policy  # AI-only no-callback classification
assert "return AUTO_UI_NATIVE;" in policy
assert "return conf.autosim_suppress_notices && autosim_control_active();" in ui
assert "const AutoUiAction action = autosim_xpop_policy(label, fn != 0);" in ui
assert "return X_pop_9(ScriptFile, label, -1, 0, 0, fn);" in ui
assert "return X_pop_9(filename, label, -1, 0, 0, fn);" in ui
# Alternate native entrypoints and portrait/wait callbacks are filtered only
# for the four orbital hit/miss results, never for Council, diplomacy or
# other user-input choices. The core X_pops_18 remains untouched.
for addr, wrapper in (("0x5BF330", "X_pop_2"),
                      ("0x5BF5D0", "X_pops"),
                      ("0x5BF7D0", "X_pops_11")):
    assert f"write_jump({addr}, (int){wrapper});" in patch
for label in ("WEKILLEDIT", "THEYKILLEDIT", "WEMUFFEDIT", "THEYMUFFEDIT"):
    assert f'!strcmp(label, "{label}")' in ui
assert "static bool autosim_orbital_result_popup(const char* label, fp_none fn)" in ui
assert "if (!autosim_ui_active() || !label" in ui
assert "(fn && fn != pop_wait && fn != pop_expire)" in ui
for entry in ('"x_pop"', '"x_pop_2"', '"x_pops"', '"x_pops_11"'):
    assert f'telemetry_record_ui_notice({entry}, label, "suppressed_orbital_result");' in ui
assert 'return X_pops_18(ScriptFile, label, -1, 0, 0, a2, 1, 1, fn);' in ui
assert 'return X_pops_18(ScriptFile, label, -1, 0, a2, a3, 1, 1, fn);' in ui
assert "write_jump(0x5BF930," not in patch

# The active implementation is C++ basewin.cpp::action_sat_attack, NOT the
# original terranx.exe body (which was incorrectly hooked in an earlier pass).
assert "write_jump(0x4CF480, (int)action_sat_attack);" in patch
assert "write_jump(0x4A0260, (int)sat_attack);" in patch
assert "write_call(0x4CF70F," not in patch
assert "autosim_orbital_result_popp" not in patch
assert "autosim_orbital_result_popp" not in ui
assert "autosim_orbital_result_popp" not in ui_header
active = basewin.split("void __cdecl action_sat_attack(", 1)[1].split(
    "void __cdecl sat_attack(", 1)[0]
assert "game_rand() % 100 >= OrbitalAttackChance" in active
assert "double_cross(faction_id, faction_id_tgt, -1);" in active
assert "++Factions[faction_id].ODP_deployed;" in active
assert "--Factions[faction_id].satellites_ODP;" in active
assert "set_fac(FAC_GEOSYNC_SURVEY_POD, base_id, 0);" in active
for label in ("WEMUFFEDIT", "THEYMUFFEDIT", "WEKILLEDIT", "THEYKILLEDIT"):
    assert active.count(
        f'autosim_notice_popp(ScriptFile, "{label}", "satbat_sm.pcx");') == 1
    assert f'popp(ScriptFile, "{label}", 0, "satbat_sm.pcx", 0);' not in active
assert active.count("autosim_notice_popp(") == 4
# This general wrapper has an explicit human/opt-out native fallback.
assert 'if (autosim_ui_active()) {' in ui
assert 'telemetry_record_ui_notice("popp", label, "suppressed_notice");' in ui
assert "return popp(filename, label, 0, image, 0);" in ui
# Do not suppress manual Orbital Attack View's command/selection dialogs.
manual = basewin.split("void __cdecl sat_attack(", 1)[1].split(
    "void BaseWin_support_zoom(", 1)[0]
for label in ("NOKILLERS", "KILLERSDEPLOYED"):
    assert f'popp(ScriptFile, "{label}", 0, "space_sm.pcx", 0);' in manual
assert 'Popup_start(&cur_popup, PopupScriptFile, "WHICHGSP",' in manual
assert "BasePop_exec_3(&cur_popup, 0, 0);" in manual

# Source-owned satellite construction already preserves counts and visibility.
for counter in ("satellites_nutrient++", "satellites_mineral++",
                "satellites_energy++", "satellites_ODP++"):
    assert counter in base
assert 'autosim_notice_popp(ScriptFile, "SEEMAP", "space_sm.pcx");' in base
assert base.count('telemetry_record_ui_notice("satellite_production", StrBuffer,') == 2
assert "mon_first_in_space(faction_id);" in base

# Native AI offensive orbital attack is still called exactly once. Telemetry
# snapshots are read-only and exist only under active all-AI + telemetry flags.
start = enemy.index("while (plr.satellites_ODP - plr.ODP_deployed > max_busters)")
end = enemy.index("void __cdecl enemy_capabilities(", start)
attack = enemy[start:end]
assert "const bool trace_satellite = autosim_ui_active() && conf.telemetry_enabled;" in attack
assert attack.count("action_sat_attack(faction_id, i, sat_type, base_id);") == 1
assert attack.count('telemetry_record_ui_notice("satellite_native", "action_sat_attack", action);') == 2
assert attack.index('"enter actor=%d') < attack.index("action_sat_attack(faction_id, i, sat_type, base_id);") < attack.index('"exit actor=%d')
for counter in ("plr.satellites_ODP", "plr.ODP_deployed",
                "Factions[i].satellites_ODP", "Factions[i].satellites_nutrient",
                "Factions[i].satellites_mineral", "Factions[i].satellites_energy"):
    assert counter in attack

# Defensive Orbital Defense Pods and Flechette defence against planet
# busters/tectonic/fungal missiles are separate and already use wrappers.
for label in ("FWESHOTIT", "TWESHOTIT", "FTHEYSHOTIT", "TTHEYSHOTIT",
              "FWESHOTIT2", "TWESHOTIT2", "FTHEYSHOTIT2", "TTHEYSHOTIT2",
              "FWEMUSTSAC", "TWEMUSTSAC", "THEYMUSTSAC",
              "WEFAILEDFLECHETTE", "THEYFAILEDFLECHETTE"):
    assert f'"{label}"' in actions
assert 'autosim_notice_popp(ScriptFile, label_we_shot, "space_sm.pcx");' in actions
assert 'autosim_notice_popp(ScriptFile, label_they_shot, "space_sm.pcx");' in actions
for label in ("FUNGALSAC", "TECTONICSAC"):
    assert f'"{label}"' in actions
assert 'int value = popp(ScriptFile, flag ? "FUNGALSAC" : "TECTONICSAC"' in actions
assert "if (*MultiplayerActive || !is_human(faction_id_def))" in actions
assert "plr_def->satellites_ODP--;" in actions

# Environmental satellite losses update inventories before their notices.
for label in ("SOLARSTORM", "SOLARFLARE", "ASTEROID"):
    assert f'autosim_notice_pop2("{label}"' in turn
assert turn.index("Factions[i].satellites_ODP = 0;") < turn.index(
    'autosim_notice_pop2("SOLARSTORM"')
assert turn.index("Factions[i].satellites_mineral = 0;") < turn.index(
    'autosim_notice_pop2("ASTEROID"')

# Active combat and Orbital Attack View remain hooked at the established entrypoints.
assert "write_jump(0x4CF480, (int)action_sat_attack);" in patch
assert "write_jump(0x4A0260, (int)sat_attack);" in patch
assert "Faction_sat_attack action_sat_attack = (Faction_sat_attack)0x4CF480;" in engine

print("AUTO-002 active C++ orbital result suppression contract passed (Windows test pending)")
