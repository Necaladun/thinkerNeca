#!/usr/bin/env python3
"""AUTO-002 Council native hook contracts (source-level, not runtime proof)."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
patch = (root / "src/patch.cpp").read_text(encoding="utf-8")
turn = (root / "src/gameturn.cpp").read_text(encoding="utf-8")
tele = (root / "src/telemetry.cpp").read_text(encoding="utf-8")
pairs = [
    ("native Council init preflight", "council_native_call(0x4284CA, 0x427230)" in patch),
    ("announcement preflights", "council_native_call(0x4284A9, 0x48C0A0)" in patch and "council_native_call(0x428542, 0x48C0A0)" in patch),
    ("human offer preflights", "council_native_call(0x428115, 0x5BF7D0)" in patch and "council_native_call(0x42828A, 0x5BF7D0)" in patch),
    ("virtual six-byte preflight", "council_modal[5] == 0x00" in patch),
    ("all-or-nothing hook install", "if (council_known)" in patch),
    ("all-AI suppression guard", "autosim_ui_active()" in turn),
    ("native modal preserved for unsupported cases", "return original(win, a2, a3, a4);" in turn),
    ("Governor-only native ballot pass", "win->proposal_type != PROP_ELECT_PLANETARY_GOVERNOR" in turn and "CouncWin_set_vote(win, owner, win->faction_vote[owner]);" in turn),
    ("no synthetic vote choice", "native_ballots_incomplete" in turn),
    ("independent flushed Council file", "council.jsonl" in tele and "append_line(council_file, event.str())" in tele),
]
for name, ok in pairs:
    print(("PASS" if ok else "FAIL") + ": " + name)
if not all(ok for _, ok in pairs):
    raise SystemExit(1)
