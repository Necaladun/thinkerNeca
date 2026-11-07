#!/usr/bin/env python3
"""AUTO-002 fast profile integration contract (not a runtime test)."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
header = (ROOT / "src/gui_dialog.h").read_text(encoding="utf-8")
turn = (ROOT / "src/gameturn.cpp").read_text(encoding="utf-8")
combat = (ROOT / "src/veh_combat.cpp").read_text(encoding="utf-8")
enums = (ROOT / "src/engine_enums.h").read_text(encoding="utf-8")
ini = (ROOT / "docs/thinker.ini").read_text(encoding="utf-8")

required = {
    "PREF_ADV_FAST_BATTLE_RESOLUTION": "0x8",
    "PREF_BSC_DONT_QUICK_MOVE_ENEMY_VEH": "0x4",
    "PREF_BSC_DONT_QUICK_MOVE_ALLY_VEH": "0x8000",
    "PREF_AV_MAP_ANIMATIONS": "0x80",
    "MPREF_ADV_QUICK_MOVE_ALL_VEH": "0x80",
    "MPREF_ADV_QUICK_MOVE_VEH_ORDERS": "0x40",
    "MPREF_ADV_PAUSE_AFTER_BATTLES": "0x10",
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
assert "autosave_interval=1" in ini
assert "autosim_fast_combat=1" in ini

# Native operator/save calls remain outside all temporary profile scopes.
end_turn = turn[:turn.index("int __cdecl net_upkeep_phase(")]
assert end_turn.index("save_game(0);", end_turn.index("if (action == AUTO_CYCLE_STOP")) > end_turn.index("mod_enemy_turn(current_id);")
print("AUTO-002 full-turn fast profile structural contract passed (not native gameplay)")
