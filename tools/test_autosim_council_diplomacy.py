#!/usr/bin/env python3
"""AUTO-002 human-diplomacy guard and Council trace source contracts.

Static integration assertions only. A native Windows test must verify whether
Lal's observed vote-bribery dialog comes from communicate() or CouncWin.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
diplo = (ROOT / "src/faction_diplo.cpp").read_text(encoding="utf-8")
events = (ROOT / "src/faction_event.cpp").read_text(encoding="utf-8")
turn = (ROOT / "src/gameturn.cpp").read_text(encoding="utf-8")
patch = (ROOT / "src/patch.cpp").read_text(encoding="utf-8")
autosim = (ROOT / "src/autosim.h").read_text(encoding="utf-8")

# The existing AI-to-AI route is not replaced by a blanket diplomacy skip.
ai = events[events.index("void __cdecl enemy_diplomacy("):
            events.index("void __cdecl enemy_capabilities(")]
assert "if (is_human(faction_id)) {\n        return;" in ai
assert "if (!is_human(i) || *SunspotDuration > 0)" in ai
assert "enemies_trade_tech(faction_id, i);" in ai
assert "encounter(faction_id, i, -1, -1, 0, 0);" in ai
assert "treaty_on(faction_id, i, DIPLO_UNK_80000000);" in ai
assert ai.index("if (!is_human(i) || *SunspotDuration > 0)") < ai.index(
    "if (i == MapWin->cOwner")
assert ai.count("communicate(i, faction_id, 0);") == 3

# No native enemy_diplomacy callsite is replaced with no-op handling.
assert "write_jump(0x55F930, (int)enemy_diplomacy);" in patch
for site in ("0x55FDBD", "0x5603E7"):
    assert f"write_call({site}, (int)communicate);" in patch

# Human-facing communicate() must fail closed ONLY for a fully eligible
# all-AI simulation, and BEFORE any state change or random call.
start = diplo.index("void __cdecl communicate(int faction_id, int faction_id_2, int flag) {")
end = diplo.index("    Faction& plr = Factions[faction_id];", start)
guard = diplo[start:end]
assert "if (autosim_control_active()) {" in guard
assert 'telemetry_record_ui_notice("diplomacy_guard", "communicate", context);' in guard
assert "return;" in guard
assert guard.index("if (autosim_control_active())") < guard.index("return;")
assert "game_rand" not in guard and "game_random" not in guard
assert "diplomacy_check" not in guard

# Eligibility requires ALL human faction bits clear, local single-player,
# living observer and unfinished game. No new config switch or saved mask.
assert "!(human_faction_mask & 0xFEu)" in autosim
assert "!multiplayer && !pbem && !game_done" in autosim

# Native Council caller remains unchanged; the new Council-specific hooks are
# separately verified and the pre-call record persists on modal stall.
assert 'autosim_trace_council_call("enter", faction_id);\n            call_council(faction_id);\n            autosim_trace_council_call("exit", faction_id);' in turn
assert 'telemetry_record_council_event("call_council", faction_id, action);' in turn
assert "CouncilProposal[owner], CouncilVoteState[owner]" in turn
assert "*CouncilSessionPending" in turn
assert "council(MapWin->cOwner, 0, 0);" in turn
assert "buy_council_vote(faction_id, faction_id_2" in diplo
print("AUTO-002 Council diplomacy structural contract passed (not native gameplay)")
