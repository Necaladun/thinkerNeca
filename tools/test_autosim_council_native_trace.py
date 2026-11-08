#!/usr/bin/env python3
"""AUTO-002 native Council trace integration (source-only regression).

The supported binary fingerprint and Win32 ABI must additionally be
validated against the uploaded patched terranx.exe and in-game runtime.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
turn = (root / "src/gameturn.cpp").read_text(encoding="utf-8")
hdr = (root / "src/gameturn.h").read_text(encoding="utf-8")
patch = (root / "src/patch.cpp").read_text(encoding="utf-8")

assert "void __thiscall autosim_trace_CouncWin_init(CouncWindow* win," in turn
assert "void __thiscall autosim_trace_CouncWin_init(CouncWindow* win," in hdr
trace = turn[turn.index("void __thiscall autosim_trace_CouncWin_init("):
             turn.index("int __cdecl autosim_council_announcement(")]
assert 'const bool trace = conf.telemetry_enabled;' in trace
assert trace.count('telemetry_record_council_event("CouncWin_init", faction_id, action);') == 2
assert trace.index("CouncWin_init(win, faction_id, faction_id_2);") > trace.index('"enter arg1=')
assert trace.index("CouncWin_init(win, faction_id, faction_id_2);") < trace.index('"exit proposal=')
for field in ("win->proposal_type", "win->candidate_faction_id",
              "win->proposal_outcome", "win->vote_faction_mask",
              "win->faction_vote[0]", "win->faction_vote[7]",
              "win->faction_vote_count[0]", "win->faction_vote_count[7]"):
    assert field in trace, f"missing native Council state field: {field}"
assert "game_random" not in trace and "game_rand(" not in trace
assert "CouncWin_set_vote" not in trace and "CouncWin_make_resolution" not in trace

hook = patch[patch.index("// Native Council-only hooks:"):
             patch.index("write_call(0x43F2F4, (int)diplomacy_check);")]
for token in ("council_native_call(0x4284CA, 0x427230)",
              "council_native_call(0x4284A9, 0x48C0A0)",
              "council_native_call(0x428542, 0x48C0A0)",
              "council_modal[0] == 0xFF",
              "council_modal[5] == 0x00",
              "if (council_known)",
              "write_call(0x4284CA, (int)autosim_trace_CouncWin_init);"):
    assert token in hook, f"native Council preflight or hook missing: {token}"
assert "write_jump(0x428350" not in patch
assert "write_jump(0x427230" not in patch
assert "write_jump(0x424720" not in patch
print("AUTO-002 native Council tracing integration passed (not Windows runtime)")
