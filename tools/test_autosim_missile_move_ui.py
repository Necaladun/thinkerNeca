#!/usr/bin/env python3
"""AUTO-002 missile, random-event and observed-AI movement UI source contract.

These are static checks. They do not prove Windows runtime behaviour or
combat/state/RNG equivalence.
"""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
action = (ROOT / "src/veh_action.cpp").read_text(encoding="utf-8")
combat = (ROOT / "src/veh_combat.cpp").read_text(encoding="utf-8")
turn = (ROOT / "src/gameturn.cpp").read_text(encoding="utf-8")
base = (ROOT / "src/base.cpp").read_text(encoding="utf-8")
dialog = (ROOT / "src/gui_dialog.cpp").read_text(encoding="utf-8")
autosim = (ROOT / "src/autosim.h").read_text(encoding="utf-8")

# The 32-iteration cancel-orders confirmation belongs only to human observer
# movement. When all-AI, observed faction uses non-observed AI movement path.
guard = "if (veh_fc_id == MapWin->cOwner && !autosim_control_active()) {"
start = action.index("// KEEPMOVING is a *human observer*")
end = action.index("MOV_END:", start)
move = action[start:end]
assert guard in move
assert move.index(guard) < move.index("++Vehs[veh_id].iter_count;")
assert move.index(guard) < move.index('X_pop("KEEPMOVING", 0)')
assert "Vehs[veh_id].order = ORDER_NONE;" in move
assert "Vehs[veh_id].iter_count = 0;" in move
assert "Vehs[veh_id].rotate_angle = offset;" in move[move.index(guard):]
assert 'std::strcmp(label, "KEEPMOVING")' not in autosim
assert 'autosim_control_active()' in dialog

# Planet Buster strike warning is one-way; terrain mutation, orbital defense,
# unit/base removals, atrocities and explosion stay in native source order.
strike = combat[combat.index("void __cdecl planet_busting("):
                combat.index("void __cdecl promote(", combat.index("void __cdecl planet_busting("))]
assert 'autosim_notice_popp(ScriptFile, "PLANETBUSTER", img);' in strike
assert strike.index("Console_focus(") < strike.index('autosim_notice_popp(ScriptFile, "PLANETBUSTER"')
assert strike.index('autosim_notice_popp(ScriptFile, "PLANETBUSTER"') < strike.index("shoot_value = shoot(")
for symbol in ("shoot_value = shoot(", "kill(veh_id);", "world_lower_alt(tx, ty);",
               "mod_base_kill(b);", "stack_kill(v);",
               "major_atrocity(faction_id, tgt_base_faction_id);",
               "boom(tx, ty, 32);", "boom(tx, ty, 64);"):
    assert symbol in strike, f"Planet Buster gameplay/animation missing: {symbol}"

# Tectonic and fungal missiles use notifications, not auto-acknowledged
# defender choices. Confirm the underlying attack operations remain.
tect = action[action.index("void  __cdecl action_tectonic("):
              action.index("void  __cdecl action_fungal(")]
fung = action[action.index("void  __cdecl action_fungal("):
              action.index("void __cdecl action_give(")]
for src, labels in ((tect, ("TECTONICMISSILE", "TECMOFIED")),
                    (fung, ("FUNGALMISSILE", "FUNGMOTIZED"))):
    for label in labels:
        assert f'autosim_notice_popp(ScriptFile, "{label}",' in src
    assert "shoot_it(" in src and "kill(veh_id);" in src
assert "world_raise_alt(tx, ty);" in tect
assert "TectonicDetonationCount[faction_id_atk]++;" in tect
for value in ("game_rand()", "bit_set(x, y, BIT_FUNGUS, 1);",
              "veh_init(BSC_FUNGAL_TOWER", "world_alt_set("):
    assert value in fung

# ODP and Flechette notifications are return-unused. The real sacrifice
# prompt and its actual yes/no outcome remain native when defender is human.
odps = action[action.index("int  __cdecl shoot_it("):
              action.index("void  __cdecl action_tectonic(")]
for src in ("label_we_shot", "label_they_shot",
            'flag ? "FWESHOTIT2" : "TWESHOTIT2"',
            'flag ? "FTHEYSHOTIT2" : "TTHEYSHOTIT2"',
            'flag ? "FWEMUSTSAC" : "TWEMUSTSAC"',
            '"THEYMUSTSAC"', '"WEFAILEDFLECHETTE"',
            '"THEYFAILEDFLECHETTE"'):
    assert f'autosim_notice_popp(ScriptFile, {src}, "space_sm.pcx");' in odps
assert 'int value = popp(ScriptFile, flag ? "FUNGALSAC" : "TECTONICSAC"' in odps
assert "if (*MultiplayerActive || !is_human(faction_id_def)) {" in odps
assert "plr_def->satellites_ODP--;" in odps
assert "plr_def->ODP_deployed++;" in odps
assert "game_rand() % 100 < MissileDefendChance" in odps

# Random-event UI was already migrated; the new eruption report isn't new.
events = turn[turn.index("void __cdecl random_events("):
              turn.index("void __cdecl alien_fauna(")]
assert not re.search(r"(?<![a-zA-Z_])(?:popp|POP2|X_pop|NetMsg_pop)\s*\(", events)
for label in ("PERIHELION", "SOLARSTORM", "ASTEROID", "FAMINE",
              "HAIL", "NEWRESOURCE", "NETCRASH", "INFLATION"):
    assert f'"{label}"' in events
for label in ("BIGFATVOLCANO", "EATTHIS"):
    assert f'autosim_notice_popp(ScriptFile, "{label}"' in events
assert 'autosim_notice_popp(ScriptFile, "VOLCANO", "volc_sm.pcx");' in base
assert "mod_world_volcano(vx, vy, 1);" in base
assert "mod_world_crater(bx, by);" in events

# All presentation wrappers preserve the ordinary/human native path.
assert "return conf.autosim_suppress_notices && autosim_control_active();" in dialog
assert "return popp(filename, label, 0, image, 0);" in dialog
# Related late-game source-owned reports ignore their return values.
for label in ("MADEORBITAL", "MADEAIRDROP", "OBLITTED", "OBLITTED2",
              "TOOKARTIFACT", "TAKENARTIFACT"):
    assert f'autosim_notice_popp(ScriptFile, "{label}",' in action
assert action.index("mod_base_kill(base_id);", action.index("void __cdecl action_oblit(")) < action.index(
    'autosim_notice_popp(ScriptFile, "OBLITTED"')
assert action.index("owner_set(tgt_x, tgt_y, Vehs[veh_id].faction_id);") < action.index(
    'autosim_notice_popp(ScriptFile, "TOOKARTIFACT"')
for label in ("CORNERFOILED", "CORNERTHEMFOILED", "HOMELESS", "HOMELESSONE"):
    assert f'autosim_notice_popp(ScriptFile, "{label}",' in base
for label in ("ESCAPED", "ESCAPED2"):
    assert f'autosim_notice_netmsg(NetMsg, "{label}"' in base
for label in ("GOTUNIT", "GAVEUNIT", "GAVEWILD"):
    assert f'autosim_notice_netmsg(NetMsg, "{label}"' in action
for label in ("GOTUNITDEFEND", "GOTUNITOFFEND"):
    assert f'std::strcmp(label, "{label}") == 0' in autosim
for choice in ('X_pop("KEEPMOVING", 0)',
               'popp(ScriptFile, "GOTOGOODY", 0',
               'popp(ScriptFile, flag ? "FUNGALSAC" : "TECTONICSAC"'):
    assert choice in action, f"Meaningful decision or human message missing: {choice}"

print("AUTO-002 missiles, random events and observed-AI movement source checks passed (not Windows runtime)")
