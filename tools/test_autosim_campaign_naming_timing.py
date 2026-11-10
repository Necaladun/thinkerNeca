#!/usr/bin/env python3
"""AUTO-TIME: source contract for named campaign folders and active-run timing.

Structural regression only: exact in-game duration and Windows folder paths
must still be validated with a real native automated campaign.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
read = lambda p: (root / p).read_text(encoding="utf-8")
telemetry = read("src/telemetry.cpp")
header = read("src/telemetry.h")
turn = read("src/gameturn.cpp")
game = read("src/game.cpp")
control = read("src/autosim_control.cpp")
report = read("tools/campaign_report.py")
launcher = read("Run Automation.ps1")

def check(label, value):
    assert value, label
    print("PASS:", label)

creation = telemetry[telemetry.index("void telemetry_begin_session("):
                     telemetry.index("static std::ostringstream orbit_record(")]
check("unique new campaign folder includes chosen name and local date/time",
      "GetLocalTime(&local);" in creation
      and '"%04u%02u%02u-%02u%02u%02u"' in creation
      and 'folder_slug(campaign_display_name.c_str()) << "-" << date_time' in creation
      and 'make_dir(campaign_dir, false)' in creation)
check("linked-save campaign name/ID and sidecar lookup unchanged",
      'campaign_id = linked_id;' in creation
      and 'campaign_display_name = linked_name;' in creation
      and 'campaign_dir = "campaigns/" + campaign_id;' in creation
      and 'read_named_save_link(source' in creation
      and "if (!joined)" in creation
      and 'write_campaign_metadata()' in creation)
check("clock state is reset per process session, independent of recording",
      'void telemetry_run_clock_set_active(bool active);' in header
      and 'bool telemetry_run_clock_started();' in header
      and 'unsigned long long telemetry_run_elapsed_ms();' in header
      and "simulation_elapsed_completed_ms = 0;" in creation
      and "simulation_clock_active = false;" in creation
      and "simulation_clock_started = false;" in creation)
check("monotonic active-time accumulation excludes paused wall time",
      'const DWORD now = GetTickCount();' in telemetry
      and "(DWORD)(now - simulation_active_start_tick)" in telemetry
      and "simulation_elapsed_completed_ms +=" in telemetry
      and "if (simulation_clock_active == active) return;" in telemetry
      and "GetTickCount() - simulation_active_start_tick" in telemetry)
check("AI clock starts after startup confirmation and not while manual",
      'if (!autosim_ready()) startup_confirmed = false;' in turn
      and 'telemetry_run_clock_set_active(autosim_ready());' in turn
      and turn.index('telemetry_run_clock_set_active(autosim_ready());') <
          turn.index('autosim_monitor_progress("cycle_start", 0);'))
check("operator turn-cap/pause menu freezes active time before display",
      turn.index("telemetry_run_clock_set_active(false);") <
          turn.index('autosim_monitor_progress("operator_dialog", 0);')
      and 'telemetry_run_clock_set_active(false);' in turn)
record = telemetry[telemetry.index("void telemetry_record_terminal_result("):
                   telemetry.index("void telemetry_resume_after_victory(")]
check("victory clock freezes before any terminal file and post-victory UI",
      record.index("telemetry_run_clock_set_active(false);") <
          record.index("if (!recording) return;")
      and 'simulation_elapsed_ms' in record
      and 'victories.jsonl' in record
      and game.index('telemetry_record_terminal_result(NULL, "operator_decision_pending");') <
          game.index('autosim_monitor_progress("victory_paused", 0);') <
          game.index('autosim_fill_dialog_state(&dialog, AUTOSIM_DIALOG_VICTORY);'))
check("explicit post-victory continuation resumes; human takeover stops",
      'telemetry_run_clock_set_active(true);' in
          telemetry[telemetry.index("void telemetry_resume_after_victory("):
                    telemetry.index("void telemetry_end_session(")]
      and "telemetry_resume_after_victory();" in game
      and 'set_human(MapWin->cOwner, true);\n                    // Post-victory human play' in game
      and 'telemetry_run_clock_set_active(false);' in game)
check("status and terminal result expose duration without fabricated values",
      'simulation_elapsed_ms' in
          telemetry[telemetry.index("std::string status_json("):
                    telemetry.index("bool write_campaign_metadata(")]
      and "if (simulation_clock_started)" in record
      and "else result << \"null\";" in record
      and "if (simulation_clock_started) out << telemetry_run_elapsed_ms();" in telemetry)
check("victory and operator window display the same runtime",
      'AI runtime: %u:%02u:%02u (operator pauses excluded)' in control
      and 'telemetry_run_elapsed_ms() / 1000' in control
      and 'if (telemetry_run_clock_started())' in control)
check("integrated Campaign Analyzer and launcher show runtime",
      "format_simulation_duration" in report
      and 'status.get("simulation_elapsed_ms")' in report
      and 'Active AI run time:' in report
      and '"runtime": "Not recorded"' in report
      and "AI runtime: $($row.runtime)" in launcher)
print("AUTO-TIME campaign naming and active run timer source contract passed")
