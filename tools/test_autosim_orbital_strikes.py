#!/usr/bin/env python3
"""AUTO-002 satellite lifecycle and native orbital-strike trace source contract.

A structural test, not proof that native action_sat_attack routes its popup
through X_pop/X_pop_2. Windows x86 validation remains required.
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

# Native attack implementation / Orbital Attack View must remain untouched.
assert "write_jump(0x4CF480, (int)action_sat_attack);" in patch
assert "write_jump(0x4A0260, (int)sat_attack);" in patch
assert "Faction_sat_attack action_sat_attack = (Faction_sat_attack)0x4CF480;" in engine

print("AUTO-002 orbital lifecycle and AI attack trace source contract passed (native popup path unverified)")
