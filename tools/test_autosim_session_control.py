#!/usr/bin/env python3
"""AUTO-002 simulation-control/telemetry lifecycle source contracts.

Static contracts run on Linux; they do not simulate native Windows modal,
post-cycle serialization, game input focus, or save/reload correctness.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / "src/gameturn.cpp").read_text(encoding="utf-8")
dialog = (ROOT / "src/autosim_control.cpp").read_text(encoding="utf-8")
header = (ROOT / "src/autosim_control.h").read_text(encoding="utf-8")
telemetry = (ROOT / "src/telemetry.cpp").read_text(encoding="utf-8")
telemetry_h = (ROOT / "src/telemetry.h").read_text(encoding="utf-8")
game = (ROOT / "src/game.cpp").read_text(encoding="utf-8")
savegame = (ROOT / "src/savegame.cpp").read_text(encoding="utf-8")
reporter = (ROOT / "tools/campaign_report.py").read_text(encoding="utf-8")
picker = (ROOT / "tools/Campaign Analyzer.pyw").read_text(encoding="utf-8")

def require(statement, reason):
    assert statement, reason

boundary = source[source.index("if (++current_id >= 8)"):
                  source.index("int __cdecl net_upkeep_phase(")]
require(boundary.index("telemetry_record_completed_turn();") <
        boundary.index("autosim_show_control_dialog(&dialog);"),
        "A complete-turn snapshot must precede the new control window")
require("if (eligible && full_cycle) ++total_autosim_turns;" in boundary,
        "Partial initial loaded cycle must not count towards session total")
require("autosim_fill_dialog_state(&dialog, AUTOSIM_DIALOG_RUNNING);" in boundary
        and "state->additional_turns = conf.autosim_manual_turns > 0" in dialog,
        "Continue input must seed current requested count")
require("conf.autosim_manual_turns = dialog.additional_turns;" in boundary
        and "auto_driver.completed_turns = 0;" in boundary,
        "Continue must reset segment cap and take new user-requested turn count")
save_path = boundary[boundary.index("if (selection == AUTOSIM_DIALOG_SAVE_CONTINUE"):]
require(save_path.index("autosim_save_named(") < save_path.index(
        "auto_driver.completed_turns = 0;"),
        "Save & Continue must not advance segment after failed/cancelled save")
require('if (selection == AUTOSIM_DIALOG_CONTINUE)' in boundary,
        "Explicit no-save continuation must have its own completed-cycle path")
require("set_human(MapWin->cOwner, true);" in boundary and
        "handover_to_human_next_turn" in boundary,
        "Manual takeover must restore only observed human flag at full-cycle boundary")
require("if (!skip_human_turn && !autosim_ready() && !switched_to_manual)"
        in boundary, "Save-and-continue must not fall into native human Console while all-AI")
require("if (!autosim_save_named(dialog.save_name, &path, &error))" in boundary
        and "continue;" in boundary, "Save failure must keep control UI open")
require("AUTOSIM_DIALOG_FAILED" in boundary and "save_game(0);" in boundary,
        "Unconstructable Win32 control window must retain native save fallback")
require("GetClientRect(dialog, &bounds)" in dialog and
        "DialogBoxIndirectParamA(" in dialog and
        "EndDialog(dialog," in dialog, "Actual native modal Win32 controls missing")
require("GetDlgItemTextA" in dialog and "strtol(input.c_str()" in dialog
        and "parsed > 100000" in dialog and "parsed < 1" in dialog,
        "Additional turns must be validated as a typed 1-100000 integer")
require("ID_SAVE_CONTINUE" in dialog and "ID_SAVE_EXIT" in dialog and
        "ID_TAKE_CONTROL" in dialog and "ID_RESUME" in dialog,
        "Dialog action controls missing")
require("autosim_save_named" in header and "autosim_show_control_dialog" in header,
        "Dialog API declarations missing")
require("mod_save_daemon(temp_name)" in dialog and
        "MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH" in dialog and
        "GetFileAttributesExA(temp_name" in dialog,
        "Named saving must serialize to temp and validate/commit rather than truncating old save")
require(dialog.index("mod_save_daemon(temp_name)") <
        dialog.index("MoveFileExA(temp_name"), "Save commit must follow successful serializer")
require('"saves/" + stem + ".sav"' in dialog, "Named saves must be loadable game saves")
require("read_named_save_link(" in telemetry and
        '"thinkerNeca-campaign-v2"' in telemetry and
        "file_fingerprint(candidate.c_str()) != fingerprint" in telemetry and
        "valid_campaign_id" in telemetry and
        "MessageBoxA(*phWnd, prompt.c_str()" in telemetry,
        "Campaign relinking must be explicitly accepted and validate the sidecar")
require('campaign_id = prefix.str()' in telemetry and
        '"AI-Sim-" << folder_slug(' in telemetry,
        "New campaign directories must have readable names")
require('make_dir(session_dir, false)' in telemetry and
        '"session-%03d"' in telemetry, "Resume must create next free session")
require('display_name' in telemetry and
        'bool telemetry_set_campaign_name(' in telemetry and
        "bool telemetry_record_named_save(" in telemetry,
        "Editable campaign metadata and save link missing")
require("telemetry_note_loaded_save(filename);" in savegame and
        "telemetry_note_loaded_save(NULL);" in game and
        "telemetry_note_loaded_save" in telemetry_h,
        "Native Save Game load path must be tracked without inheriting stale identity")
require("campaign_display_name(campaign_dir)" in reporter and
        '"campaign_name": campaign_display_name(campaign_dir)' in reporter and
        '"campaign_name", "player"' in picker,
        "Analyzer must show selected human-readable name without losing stable path")
print("AUTO-002 session control and TEL-001 naming/lineage source checks passed")
