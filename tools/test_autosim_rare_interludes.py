#!/usr/bin/env python3
"""AUTO-002 first Mind Worm loss and alien story interludes source contract.

Structural checks only. Real Windows native interlude state persistence,
player-story state and save/reload still require manual parity verification.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
base = (root / "src/base.cpp").read_text(encoding="utf-8")
vehicle = (root / "src/veh.cpp").read_text(encoding="utf-8")
faction = (root / "src/faction.cpp").read_text(encoding="utf-8")
dialogs = (root / "src/gui_dialog.cpp").read_text(encoding="utf-8")
game = (root / "src/game.cpp").read_text(encoding="utf-8")

# On production, the initial story Mind Worm remains marked after its
# interlude 4. This is a unique flagged unit, not every Mind Worm death.
create = base[base.index("if (unit_id == BSC_MIND_WORMS) {"):
              base.index("// remove redundant Spore Launcher check",
                         base.index("if (unit_id == BSC_MIND_WORMS) {"))]
assert "if (faction_id == player_id && is_human(faction_id)" in create
assert "&& !(*GameInterludeState & 0x10)) {" in create
assert create.index("autosim_interlude(4, base->name, 1, 0);") < create.index(
    "veh->state |= VSTATE_ASSISTANT_WORM;")

kill = vehicle[vehicle.index("int __cdecl battle_kill("):
               vehicle.index("void __cdecl battle_kill_stack(")]
assert "veh->state & VSTATE_ASSISTANT_WORM" in kill
assert "veh_id_atk >= 0" in kill and "Vehs[veh_id_atk].faction_id" in kill
assert "(!(*GamePreferences & PREF_AV_INTERLUDES_DISABLED) || autosim_ui_active())" in kill
assert kill.index("Bases[base_id].state_flags |= BSTATE_ASSISTANT_KILLER_HOME;") < kill.index(
    "Factions[atk_faction_id].player_flags |= PFLAG_UNK_1000;") < kill.index(
    "autosim_interlude(6, Bases[base_id].name, 1, 0);")
assert "interlude(6, Bases[base_id].name, 1, 0);" not in kill.replace(
    "autosim_interlude(6, Bases[base_id].name, 1, 0);", "")
assert "veh_kill(veh_id);" in kill

# The subsequent special base-capture story still sees this state and
# preserves its base naming/military effects.
capture = base[base.index("mon_conquer_base(faction_id_atk, old_name);"):
               base.index("mod_eliminate_player(faction_id, faction_id_atk);",
                          base.index("mon_conquer_base(faction_id_atk, old_name);"))]
assert "base->state_flags & BSTATE_ASSISTANT_KILLER_HOME" in capture
assert "autosim_interlude(7, 0, 1, 1);" in capture
assert "strcpy_n(base->name, 25, MFactions[player_id].assistant_city);" in capture

# Rare non-victory alien interludes use the same presentation-only gate.
assert "autosim_interlude(41, 0, 1, 0);" in vehicle
assert "autosim_interlude(42, 0, 1, 0);" in faction
assert "interlude(41, 0, 1, 0);" not in vehicle.replace(
    "autosim_interlude(41, 0, 1, 0);", "")
assert "interlude(42, 0, 1, 0);" not in faction.replace(
    "autosim_interlude(42, 0, 1, 0);", "")

# The UI wrapper calls the native handler in both modes, temporarily
# disabling only presentation for eligible all-AI sessions.
assert "if (!autosim_ui_active()) {" in dialogs
assert "AutoTemporaryPrefFlag disable_ui(GamePreferences, PREF_AV_INTERLUDES_DISABLED);" in dialogs
assert dialogs.count("interlude(event_id, text, a3, a4);") == 2
assert "prior_interlude_state, (unsigned)*GameInterludeState" in dialogs
assert 'telemetry_record_ui_notice("interlude", label, action);' in dialogs

# Actual game-over sequence, credits and player continue/exit choice are
# deliberately NOT included in the interlude-only suppression pass.
assert 'popp(ScriptFile, "GAMEOVERMAN", 0, "stars_sm.pcx", 0)' in game

print("AUTO-002 rare story interludes source contract passed (native runtime unverified)")
