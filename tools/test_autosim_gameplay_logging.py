#!/usr/bin/env python3
"""TEL-LOG-01: game-event capture is independent of automation UI.

These assertions guard call order/suppression isolation. Successful CI does
NOT substitute for a Windows native Council or project lifecycle test.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
src = lambda path: (root / path).read_text(encoding="utf-8")
turn = src("src/gameturn.cpp")
tele = src("src/telemetry.cpp")
header = src("src/telemetry.h")
base = src("src/base.cpp")
patch = src("src/patch.cpp")
report = src("tools/campaign_report.py")

def check(label, value):
    assert value, label
    print("PASS:", label)

check("Council native call and modal hooks are unmodified",
      'autosim_trace_council_call("enter", faction_id);\n            call_council(faction_id);\n            autosim_trace_council_call("exit", faction_id);' in turn
      and "write_call(0x4284CA, (int)autosim_trace_CouncWin_init);" in patch
      and "write_bytes(0x4282F2, old_modal, new_modal, sizeof(old_modal));" in patch
      and "CouncWin_init(win, faction_id, faction_id_2);" in turn
      and "const int resolved = CouncWin_make_resolution(win, 1);" in turn)
check("Council event logging never calls native AI vote functions",
      "council_get_vote(" not in tele
      and "CouncWin_set_vote(" not in tele
      and "CouncWin_make_resolution(" not in tele
      and 'council_record("ballots")' in tele
      and 'council_record("resolution")' in tele
      and "win->faction_vote[fc]" in tele
      and "win->faction_vote_count[fc]" in tele)
check("Council sponsor and reason remain explicitly unknown",
      'proposer_faction\\":null' in tele
      and 'reason_status\\":\\\"native_ai_reason_not_instrumented' in tele
      and 'convening_faction' in tele
      and "council_calling_faction" in tele)
check("Council presentation diagnostics are not Council history",
      "ui_notices.jsonl" in tele
      and '"modal_fallback"' in tele
      and 'telemetry_record_ui_notice("council", phase' in tele
      and 'telemetry_record_council_event("call_council"' not in turn
      and 'telemetry_record_council_event("CouncWin_init"' not in turn
      and "council.jsonl" in tele)
check("Secret Project starts and changes observed at real queue mutation",
      "base->queue_items[0] = item_id;" in base
      and "telemetry_project_queue_changed(base_id, old_item, item_id);" in base
      and base.index("base->queue_items[0] = item_id;") <
          base.index("telemetry_project_queue_changed(base_id, old_item, item_id);")
      and "project_queues" in tele)
check("true project completion comes after ownership assignment and before UI",
      "SecretProjects[project_id] = *CurrentBaseID;" in base
      and "telemetry_project_completed(*CurrentBaseID, facility_id);" in base
      and base.index("SecretProjects[project_id] = *CurrentBaseID;") <
          base.index("telemetry_project_completed(*CurrentBaseID, facility_id);") <
          base.index('if (autosim_ui_active()) {\n                    telemetry_record_ui_notice("secret_project"'))
check("direct queue changes and base-slot compaction are reconciled",
      "telemetry_project_reconcile_completed_turn();" in tele
      and "std::map<std::pair<int,int>, ProjectQueueState> project_queues;" in tele
      and "present_bases" in tele
      and '"base_captured"' in tele
      and '"base_removed"' in tele
      and 'project_queues.clear();' in tele)
check("project logging doesn't write game state or decide production",
      "telemetry_project_completed" in header
      and "telemetry_project_queue_changed" in header
      and "SecretProjects[project_id] = *CurrentBaseID;" not in tele
      and "select_build(" not in tele
      and "game_rand(" not in tele)
check("existing campaign reporters and diagnostic CSV stay readable",
      "build_gameplay_records" in report
      and "council_meetings.csv" in report
      and "council_ballots.csv" in report
      and "projects.csv" in report
      and "ui_notices.csv" in report
      and '"event" not in row and "phase" in row' in report
      and 'Automation diagnostics' in report)
print("TEL-LOG-01 structural contracts passed; Windows native acceptance pending")
