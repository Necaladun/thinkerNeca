#!/usr/bin/env python3
"""AUTO-002 fast profile integration contract (not a runtime test)."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
header = (ROOT / "src/gui_dialog.h").read_text(encoding="utf-8")
turn = (ROOT / "src/gameturn.cpp").read_text(encoding="utf-8")
combat = (ROOT / "src/veh_combat.cpp").read_text(encoding="utf-8")
enums = (ROOT / "src/engine_enums.h").read_text(encoding="utf-8")
ini = (ROOT / "docs/thinker.ini").read_text(encoding="utf-8")
movement = (ROOT / "src/veh_action.cpp").read_text(encoding="utf-8")
ecology = (ROOT / "src/base.cpp").read_text(encoding="utf-8")

required = {
    "PREF_ADV_FAST_BATTLE_RESOLUTION": "0x8",
    "PREF_BSC_DONT_QUICK_MOVE_ENEMY_VEH": "0x4",
    "PREF_BSC_DONT_QUICK_MOVE_ALLY_VEH": "0x8000",
    "PREF_AV_MAP_ANIMATIONS": "0x80",
    "MPREF_ADV_QUICK_MOVE_ALL_VEH": "0x80",
    "MPREF_ADV_QUICK_MOVE_VEH_ORDERS": "0x40",
    "MPREF_ADV_PAUSE_AFTER_BATTLES": "0x10",
    "PREF_ADV_NO_CENTER_VEH_ORDERS": "0x80000",
    "PREF_BSC_MOUSE_EDGE_SCROLL_VIEW": "0x1000",
    "MPREF_MAP_HIDE_ACTIVE_VEH_GOTO_PATH": "0x2000000",
    "MPREF_AV_WHOLE_VEH_BLINKS": "0x200",
    "MPREF_MAP_SHOW_BASE_NAMES": "0x2000",
    "MPREF_MAP_SHOW_PROD_WITH_BASE_NAMES": "0x1000",
}
for key, val in required.items():
    assert f"{key} = {val}" in enums, f"Unexpected engine flag {key}"
    assert key in header, f"Fast profile missing {key}"

assert "AutoScopedCombatPrefs(autosim_fast_combat_active()" in header
assert "AutoScopedAutosimFastPrefs fast_presentation;" in combat
assert turn.count("AutoScopedAutosimFastPrefs fast_presentation;") == 3
assert turn.index("AutoScopedAutosimFastPrefs fast_presentation;\n                    turn_upkeep();") > 0
assert "AutoScopedAutosimFastPrefs fast_presentation;\n                mod_enemy_turn(current_id);" in turn

upkeep_start = turn.index("void __cdecl faction_upkeep(")
upkeep_end = turn.index("void __cdecl repair_phase(", upkeep_start)
upkeep = turn[upkeep_start:upkeep_end]
assert "AutoScopedAutosimFastPrefs fast_presentation;" in upkeep
assert upkeep.index("init_save_game(faction_id);") < upkeep.index("AutoScopedAutosimFastPrefs fast_presentation;")
assert upkeep.index("} // Restore all preference bits before the ordinary autosave call.") < upkeep.index("auto_save();")
assert "PREF_BSC_AUTOSAVE_EACH_TURN" in upkeep
assert "((*GamePreferences & PREF_BSC_AUTOSAVE_EACH_TURN) || autosim_control_active())" in upkeep
assert "auto_save();" in upkeep
assert "autosave_interval=1" in ini
assert "autosim_fast_combat=1" in ini

# Render suppression must not shortcut damage iteration or visibility state.
assert "const bool suppress_combat_presentation = fast_autosim_combat && render_battle;" in combat
assert "if (suppress_combat_presentation) {\n        render_battle = 0;\n    }" in combat
bombard = combat[combat.index("if (combat_type == CT_CAN_ARTY) {", combat.index("const bool suppress_combat_presentation")):
                  combat.index("return 0; // Always return here")]
assert "while (veh_id_def >= 0)" in bombard
assert "roll.damage(combat_rand(roll.range))" in bombard
assert "artillery_damage_floor" in bombard
assert "veh_def->damage_taken += damage_value;" in bombard
assert "veh_atk->visibility |= (1 << faction_id_def);" in bombard
assert "draw_tile_fixup(tx, ty, 1, 2);" in bombard  # one final target redraw
assert "if (suppress_combat_presentation)" in combat[combat.index("return 0; // Always return here"):]
assert "veh_atk->visibility |= (1 << faction_id_def);" in combat[combat.index("return 0; // Always return here"):]
assert "while (veh_atk->cur_hitpoints() > 0 && veh_def->cur_hitpoints() > 0)" in combat

# Use native offscreen presentation branch, not a movement/gameplay shortcut.
start = movement.index("// AUTO-002: visibility was already computed")
stop = movement.index("stack_veh(veh_id, 1);", start)
quick = movement[start:stop]
assert "if (autosim_fast_combat_active())" in quick
assert "move_delay = 0;" in quick and "visual_tgl = false;" in quick
assert "veh_scoot" not in quick
assert "Vehs[veh_id].rotate_angle = offset;" in movement[stop:stop+900]
assert "stack_put(veh_id, tgt_x, tgt_y);" in movement

# Ecological mutation occurs before noninteractive notification suppression.
fungus = ecology[ecology.index("void __cdecl mod_base_ecology()"):ecology.index("void __cdecl mod_base_ecology()")+10500]
assert fungus.index("bit_set(nx, ny, BIT_FUNGUS, 1);") < fungus.index('autosim_notice_pop2("FUNGUSGROWS"')
assert "bit_set(nx, ny, fungus_remove, 0);" in fungus
assert "synch_bit(nx, ny, player_id);" in fungus
assert 'autosim_notice_pop2("FUNGUSGROWS2"' in fungus
assert 'autosim_notice_pop2("FUNGUSGROWS"' in fungus
assert 'autosim_notice_pop2("FORESTGROWS"' in turn
assert 'autosim_notice_pop2("KELPGROWS"' in turn
assert 'autosim_notice_netmsg(NetMsg, "FORESTGROWS"' in turn
assert 'autosim_notice_netmsg(NetMsg, "KELPGROWS"' in turn

# Native operator/save calls remain outside all temporary profile scopes.
end_turn = turn[:turn.index("int __cdecl net_upkeep_phase(")]
assert end_turn.index("autosim_save_named(", end_turn.index("if (++current_id >= 8)")) > end_turn.index("mod_enemy_turn(current_id);")
# Startup confirmation has its own AUTOSIM_DIALOG_FAILED branch before the
# first AI phase; anchor the emergency Save Game fallback to the running
# control menu at a fully completed AI faction cycle, not the first mention
# of that enum in the file.
cycle = end_turn[end_turn.index("if (++current_id >= 8)"):]
failed = cycle.index("if (selection == AUTOSIM_DIALOG_FAILED) {")
fallback = cycle.index("save_game(0);", failed)
assert cycle.index("telemetry_record_completed_turn();") < failed < fallback
assert cycle.index("autosim_show_control_dialog(&dialog);") < failed
assert cycle.index("*ControlTurnA = 1;", fallback) > fallback
assert cycle.index("*ControlTurnB = 1;", fallback) > fallback
assert end_turn.index("autosim_show_control_dialog(&dialog);") > end_turn.index("telemetry_record_completed_turn();")
print("AUTO-002 full-turn fast profile structural contract passed (not native gameplay)")
