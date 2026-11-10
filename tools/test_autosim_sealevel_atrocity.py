#!/usr/bin/env python3
"""AUTO-002 late-game sea-level and major-atrocity notification contract.

Source-order checks only: compilation and native Windows runtime remain separate.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sea = (ROOT / "src/map.cpp").read_text(encoding="utf-8")
turn = (ROOT / "src/gameturn.cpp").read_text(encoding="utf-8")
faction = (ROOT / "src/faction.cpp").read_text(encoding="utf-8")
air = (ROOT / "src/veh_action.cpp").read_text(encoding="utf-8")
dialog = (ROOT / "src/gui_dialog.cpp").read_text(encoding="utf-8")

def must_order(src, *tokens):
    offsets = [src.index(token) for token in tokens]
    assert offsets == sorted(offsets), f"unexpected call order: {tokens}"

# Rising seas update sea level and recalculate climate BEFORE announcing it.
upkeep = turn[turn.index("void __cdecl turn_upkeep("):
              turn.index("void __cdecl faction_upkeep(")]
must_order(upkeep, "*MapSeaLevel += alt_val;", "world_climate();",
           'autosim_notice_popp(ScriptFile, "SEARISING"')
assert 'autosim_notice_popp(ScriptFile, "SEAFALLING", "seafal_sm.pcx");' in upkeep

# Shoreline crossings preserve all side effects, including casualties, emergency
# dome installation, last-colony survival, deletion and non-air vehicle loss.
alt = sea[sea.index("void __cdecl alt_set("):
          sea.index("void __cdecl alt_set_both(")]
for effect in (
    "sq->items &= ~(BIT_SENSOR", "base->pop_size = max(0,",
    "mapgen_rand(3) + 2", "set_fac(FAC_PRESSURE_DOME, base_id, 1);",
    "Factions[base->faction_id].player_flags |= PFLAG_UNK_80000;",
    "MapBaseSubmergedCount[base->faction_id]++;",
    "if (is_objective(base_id) && base->pop_size < 1)",
    "if (Factions[base->faction_id].base_count <= 1",
    "base->pop_size = 1;", "mod_base_kill(base_id);",
    "veh_kill(veh_id);", "set_alt();",
):
    assert effect in alt, f"submersion effect missing: {effect}"
must_order(alt, "base->pop_size = max(0,",
           "set_fac(FAC_PRESSURE_DOME, base_id, 1);",
           "MapBaseSubmergedCount[base->faction_id]++;",
           "if (base->pop_size > 0)",
           "if (Factions[base->faction_id].base_count <= 1")
for label in ("BASESUBMERGED", "BASESUBMERGED2"):
    assert f'X_pop_2("{label}", "subbase_sm.pcx", 0);' in alt
    assert f'"{label}", "suppressed_notice");' in alt
assert alt.count("if (autosim_ui_active()) {") == 2
for label in ("BASESUBMERGED1", "BASESUBMERGED3"):
    assert f'autosim_notice_popp(ScriptFile, "{label}", "subbase_sm.pcx");' in alt
must_order(alt, 'autosim_notice_popp(ScriptFile, "BASESUBMERGED1"',
           "mod_base_kill(base_id);")
must_order(alt, 'autosim_notice_popp(ScriptFile, "BASESUBMERGED3"',
           "mod_base_kill(base_id);")
assert "if (!*VehBattleState && (is_player || is_visible))" in alt

# Major atrocity: real sanctions, relations, diplomatic retaliation and
# declarations must occur BEFORE the notification-only message.
major = faction[faction.index("void __cdecl major_atrocity("):
                faction.index("int __cdecl break_treaty(")]
for effect in (
    "DIPLO_MAJOR_ATROCITY_VICTIM|DIPLO_ATROCITY_VICTIM",
    "plr->integrity_blemishes",
    "plr->major_atrocities++;",
    "pact_ends(faction_id, i);",
    "treaty_on(i, faction_id, DIPLO_VENDETTA|DIPLO_COMMLINK);",
    "Factions[i].diplo_status[faction_id] |= DIPLO_UNK_40;",
    "Factions[i].diplo_merc[faction_id] = 50;",
    "plr->diplo_spoke[i] = *CurrentTurn;",
    'snprintf(StrBuffer, StrBufLen, "MAJORATROCITY%d"',
):
    assert effect in major, f"major-atrocity effect missing: {effect}"
must_order(major, "plr->major_atrocities++;",
           "treaty_on(i, faction_id, DIPLO_VENDETTA|DIPLO_COMMLINK);",
           'autosim_notice_popp(ScriptFile, StrBuffer, "council_sm.pcx");')
assert 'popp(ScriptFile, StrBuffer, 0, "council_sm.pcx", 0);' not in major

minor = faction[faction.index("void __cdecl atrocity("):
                faction.index("void __cdecl major_atrocity(")]
for label in ("ATROCITYSPOTS", "ATROCITY1", "ATROCITY2", "ATROCITY"):
    assert f'autosim_notice_netmsg(NetMsg, "{label}", 5000, 0, 0);' in minor
assert "plr->sanction_turns += turns;" in minor
assert "major_atrocity(faction_id, faction_id_tgt);" in minor
# Human-choice treaty-breaking dialogs remain native.
break_treaty = faction[faction.index("int __cdecl break_treaty("):]
for label in ("BREAKINGTREATY", "BREAKINGTRUCE", "BEGINVENDETTA"):
    assert f'popp(ScriptFile, "{label}"' in break_treaty

# Orbital insertion was already fixed on the branch. Return-unused popup
# is wrapped, but airdrop legality and actual relocation are preserved.
drop = air[air.index("int __cdecl action_airdrop("):
           air.index("void __cdecl action_arty(", air.index("int __cdecl action_airdrop("))]
assert 'autosim_notice_popp(ScriptFile, "MADEORBITAL", image_drop);' in drop
assert 'autosim_notice_popp(ScriptFile, "MADEAIRDROP", image_drop);' in drop
must_order(drop, "game_rand() % 10 + 50;",
           'autosim_notice_popp(ScriptFile, "MADEORBITAL", image_drop);',
           "veh->state |= VSTATE_MADE_AIRDROP;",
           "stack_put(veh_id, tx, ty);")

# No global auto-clicker: only source-classified notices and all-AI opt-in.
assert "return conf.autosim_suppress_notices && autosim_control_active();" in dialog
assert "return popp(filename, label, 0, image, 0);" in dialog

print("AUTO-002 sea-level, submersion, atrocity and orbital popup source contract passed (not Windows runtime)")
