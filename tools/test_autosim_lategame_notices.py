#!/usr/bin/env python3
"""AUTO-002 late-game notice policy source contract, not a Windows runtime test.

The audited call sites ignore the popup return value. This test verifies that
the gameplay work surrounding each notice remains in its original order and
that decisions are not routed through a presentation-only suppression wrapper.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
base = (ROOT / "src/base.cpp").read_text(encoding="utf-8")
faction = (ROOT / "src/faction.cpp").read_text(encoding="utf-8")
turn = (ROOT / "src/gameturn.cpp").read_text(encoding="utf-8")
tech = (ROOT / "src/tech.cpp").read_text(encoding="utf-8")
policy = (ROOT / "src/autosim.h").read_text(encoding="utf-8")
dialogs = (ROOT / "src/gui_dialog.cpp").read_text(encoding="utf-8")
game = (ROOT / "src/game.cpp").read_text(encoding="utf-8")
monument = (ROOT / "src/monument.cpp").read_text(encoding="utf-8")

assert "return conf.autosim_suppress_notices && autosim_control_active();" in dialogs
assert 'telemetry_record_ui_notice("popp", label, "suppressed_notice");' in dialogs
assert "return popp(filename, label, 0, image, 0);" in dialogs

# First orbital launch: map reveal and satellite production precede UI notice.
launch = base[base.index("if (project_id < 0) {", base.index("int __cdecl mod_base_production()")):]
for satellite in ("satellites_nutrient++", "satellites_mineral++",
                  "satellites_energy++", "satellites_ODP++"):
    assert satellite in launch
assert launch.index("sq->visibility |= (1 << faction_id);") < launch.index("plr->player_flags |= PFLAG_MAP_REVEALED;")
assert launch.index("plr->player_flags |= PFLAG_MAP_REVEALED;") < launch.index(
    'autosim_notice_popp(ScriptFile, "SEEMAP", "space_sm.pcx");')
assert launch.index('autosim_notice_popp(ScriptFile, "SEEMAP", "space_sm.pcx");') < launch.index("mon_first_in_space(faction_id);")

# Both observed-faction production popb branches are narrowly satellite-only.
# The existing facility-announcement marker must still be written.
assert base.count("if (is_satellite && autosim_ui_active()) {") == 2
assert base.count('"satellite_production", StrBuffer,') == 2
assert launch.index("plr->facility_announced[ann_byte] |= ann_bit;") < launch.index(
    'telemetry_record_ui_notice("satellite_production", StrBuffer,')
assert "popb(StrBuffer, WARN_STOP_NEW_FAC_BUILT, -1, \"facblt_sm.pcx\", 0);" in launch
assert "popb(StrBuffer, warn_flag, -1, icon, 0);" in launch
assert "autosim_notice_popp(ScriptFile, label, image);" in launch
assert "autosim_notice_netmsg(NetMsg, label, 5000, 0, image);" in launch

# Existing satellite-loss events affect satellite counts before announcements.
for label in ("SOLARSTORM", "SOLARFLARE", "ASTEROID"):
    assert f'autosim_notice_pop2("{label}"' in turn
assert turn.index("Factions[i].satellites_energy = 0;") < turn.index(
    'autosim_notice_pop2("SOLARSTORM"')
assert turn.index("Factions[i].satellites_mineral = 0;") < turn.index(
    'autosim_notice_pop2("ASTEROID"')

# Technology UI was already classified: map revelation, reactor upgrades,
# initial forest-use tutorial. Human choice-returning UI must stay intact.
assert 'autosim_notice_popp(ScriptFile, "SEEMAP2", "native_sm.pcx");' in tech
for label in ("REACTORUPGRADE", "TREETIME"):
    assert f'std::strcmp(label, "{label}") == 0' in policy
for label in ("SOCIETY", "ASKSEEDESIGN"):
    assert f'std::strcmp(label, "{label}") == 0' in policy

# Eradication is resolved before the WIPEOUT popup, but monument history
# must continue afterward. Colony-pod escape discovery preserves its RNG.
elimination = faction[faction.index("int __cdecl mod_eliminate_player("):]
assert elimination.index("int setup_val = mod_setup_player(") < elimination.index(
    "autosim_notice_popp(ScriptFile, name, file);")
assert elimination.index("autosim_notice_popp(ScriptFile, name, file);") < elimination.index(
    "mon_killed_faction(setup_id, faction_id);")
assert elimination.count('autosim_notice_popp(ScriptFile, "TRACKED"') == 2
assert elimination.index("game_randv(infiltrate != 0 ? 2 : 4)") < elimination.index(
    'autosim_notice_popp(ScriptFile, "TRACKED"')
assert "void __cdecl mon_killed_faction(" in monument
assert "void __cdecl mon_first_in_space(" in monument

# Other late-game one-way notices: leave environmental effects, subspace
# victory setup, alien arrival, sanctions and advancement flags in place.
for label, icon in (
    ("GENERATORME", "subgen_sm.pcx"),
    ("GENERATOR", "subgen_sm.pcx"),
    ("TRIGGERWARMING", "searis_sm.pcx"),
    ("TRIGGERWARMING2", "searis_sm.pcx"),
    ("VOLCANO", "volc_sm.pcx"),
):
    assert f'autosim_notice_popp(ScriptFile, "{label}", "{icon}");' in base
assert base.index("*ClimateFutureChange += climate_change;") < base.index(
    'autosim_notice_popp(ScriptFile, "TRIGGERWARMING"')
assert base.index("mod_world_volcano(vx, vy, 1);") < base.index(
    'autosim_notice_popp(ScriptFile, "VOLCANO"')
for label, icon in (("ALIENSARRIVE", "al_land_sm.pcx"),
                    ("ASCENT", "asctran_sm.pcx"),
                    ("SANCTIONSENDALIEN", "Alopdir.pcx")):
    assert f'autosim_notice_popp(ScriptFile, "{label}", "{icon}");' in turn
assert turn.index("aliens_arrive = true;") < turn.index(
    'autosim_notice_popp(ScriptFile, "ALIENSARRIVE"')
assert turn.index("plr.player_flags |= PFLAG_UNK_2000;") < turn.index(
    'autosim_notice_popp(ScriptFile, "ASCENT"')

# Never suppress a choice about continuing the game or changing society.
assert 'popp(ScriptFile, "GAMEOVERMAN", 0, "stars_sm.pcx", 0)' in game
assert 'popp(ScriptFile, "INFRASTRUCTURE", 0, "genwarning_sm.pcx", 0)' in turn
assert 'X_pops_11("NOTRANSCEND", 0x100000' in turn
assert 'call_council(' in turn

print("AUTO-002 late-game notices source contract passed (not Windows runtime)")
