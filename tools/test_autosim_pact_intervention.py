#!/usr/bin/env python3
"""AUTO-002 pact/intervention reports and Council result source contract.

Structural checks only; native pact-termination replacement and CouncWindow
still require separate Windows x86 gameplay/state comparison.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
faction = (ROOT / "src/faction.cpp").read_text(encoding="utf-8")
events = (ROOT / "src/faction_event.cpp").read_text(encoding="utf-8")
turn = (ROOT / "src/gameturn.cpp").read_text(encoding="utf-8")
policy = (ROOT / "src/autosim.h").read_text(encoding="utf-8")
ui = (ROOT / "src/gui_dialog.cpp").read_text(encoding="utf-8")
engine = (ROOT / "src/engine.cpp").read_text(encoding="utf-8")
patch = (ROOT / "src/patch.cpp").read_text(encoding="utf-8")

# Intervention status must not be invented: current source declares war
# for the third party only when at least one original belligerent is human.
start = faction.index("void __cdecl intervention(")
end = faction.index("void __cdecl double_cross(", start)
intervention = faction[start:end]
assert "if (is_human(faction_id_atk) || is_human(faction_id_def)) {" in intervention
assert intervention.index("if (is_human(faction_id_atk) || is_human(faction_id_def)) {") < intervention.index("treaty_on(i, faction_id_atk, DIPLO_UNK_80000000|DIPLO_VENDETTA|DIPLO_COMMLINK);")
for label in ("AIDSTHEM", "AIDSUS"):
    assert f'X_pop("{label}", 0);' in intervention
    assert f'std::strcmp(label, "{label}") == 0' in policy
assert "int __cdecl X_pop(const char* label, fp_none fn)" in ui
assert "const AutoUiAction action = autosim_xpop_policy(label, fn != 0);" in ui
assert "if (autosim_ui_active()) {" in ui
assert "return X_pop_9(ScriptFile, label, -1, 0, 0, fn);" in ui

# Ordinary third-party pact news is already wrapped. Real pact state is
# updated after notification, not replaced by a fake popup answer.
assert 'autosim_notice_diplomacy(NetMsg, "PACTUNPACT", 5000, 0, 0);' in events
assert 'autosim_notice_diplomacy(NetMsg, "ENEMYPACTENDS", 5000, 0, 0);' in events
rupture = events[events.index("void __cdecl enemies_unpact("):events.index("void __cdecl enemies_team_up(")]
assert rupture.index("autosim_notice_diplomacy(NetMsg, \"ENEMYPACTENDS\"") < rupture.index("plr1.diplo_status[faction_id_2] &= ")
assert "plr2.diplo_status[faction_id_1] &= " in rupture
# The source-owned ENEMYPACTENDS filter was not effective until the native
# third-party breakup entrypoint used this audited equivalent implementation.
# No native pact_ends/net_pact_ends entrypoint or withdrawal action is changed.
assert "Fenemies_unpact enemies_unpact = (Fenemies_unpact)0x55CE20;" in engine
assert "write_jump(0x55CE20, (int)enemies_unpact);" in patch
assert "write_jump(0x53C650, (int)pact_ends);" in patch
assert "write_jump(0x539660, (int)net_pact_ends);" in patch
assert "Fpact_ends pact_ends = (Fpact_ends)0x53C650;" in engine
assert "Fnet_pact_ends net_pact_ends = (Fnet_pact_ends)0x539660;" in engine

# The source-owned Governor winner report is already a one-way notice
# and notification clearing remains. Native resolution is reused by the
# separate all-AI Governor experiment; ordinary human UI remains native.
assert 'autosim_notice_popp(ScriptFile, "COUNCILHOTGOVWIN", "council_sm.pcx");' in turn
assert 'autosim_notice_popp(ScriptFile, "COUNCILHOTGOVLOSE", "council_sm.pcx");' in turn
assert 'autosim_notice_popp(ScriptFile, "COUNCILHOTGOVNONE", "council_sm.pcx");' in turn
assert "clear_council_notify(MapWin->cOwner);" in turn
assert "CouncWin_init(win, faction_id, faction_id_2);" in turn
assert 'telemetry_record_council_event("CouncWin_init", faction_id, action);' in turn

print("AUTO-002 pact/intervention source contract passed (native Windows behavioural parity unverified)")
