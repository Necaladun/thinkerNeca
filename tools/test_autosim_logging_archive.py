#!/usr/bin/env python3
"""Source contracts for AUTO-LOG and unified Automation Launcher.

Native Windows integration, performance, long-game stability and checkpoint
SAV reload are deliberately NOT claimed by these structural tests.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
def src(name):
    return (root / name).read_text(encoding="utf-8")

main = src("src/main.cpp")
hdr = src("src/main.h")
game = src("src/game.cpp")
turn = src("src/gameturn.cpp")
tele = src("src/telemetry.cpp")
tele_h = src("src/telemetry.h")
launcher = src("Run Automation.ps1")
report = src("tools/campaign_report.py")
workflow = src(".github/workflows/build-windows.yml")
thinker_ini = src("docs/thinker.ini")

def check(label, ok):
    assert ok, label
    print("PASS", label)

check("canonical 14 stock factions validated for all seven unique slots",
      all('"' + x + '"' in main for x in (
          "GAIANS", "HIVE", "UNIV", "MORGAN", "SPARTANS", "BELIEVE",
          "PEACE", "CYBORG", "PIRATES", "DRONE", "ANGELS",
          "FUNGBOY", "CARETAKE", "USURPER"))
      and "p.faction_slot_count != 0 && p.faction_slot_count != 7" in main
      and "p.faction_slots[i] == p.faction_slots[j]" in main)
check("explicit roster only for new games, before native setup",
      'autosim_new_roster[7]' in main
      and "if (autosim_auto_new_game && !autosim_new_roster[0].empty())" in game
      and game.index("if (autosim_auto_new_game && !autosim_new_roster[0].empty())")
      < game.index("load_flags();", game.index("int __cdecl config_game("))
      and "read_faction(fac, 0);" in game
      and "load_faction_art(i);" in game
      and "needs_expansion && !*ExpansionEnabled" in game)
check("UI saves seven specific faction identities; loaded saves keep roster",
      "New-Tab 'Faction roster'" in launcher
      and "for ($slot = 1; $slot -le 7; ++$slot)" in launcher
      and "Faction ' + $id + ' is assigned to multiple slots" in launcher
      and "$factionTab.Enabled=$newGame.Checked" in launcher
      and "faction_7 = 'PEACE'" in launcher)
check("timing is optional launcher-only and logs true milliseconds",
      "autosim_timing_enabled = 0;" in hdr
      and "cf->autosim_timing_enabled = p.timing;" in main
      and "TelemetryPhaseStamp telemetry_phase_begin()" in tele
      and "GetTickCount()" in tele
      and "elapsed_ms" in tele
      and "started_utc" in tele
      and "world_upkeep" in turn and "faction_upkeep" in turn
      and '"ai_turn"' in turn and '"full_faction_cycle"' in turn)
check("archive checkpoints require complete turn, preserve normal saves and RNG",
      "autosim_archive_interval = 0;" in hdr
      and "cf->autosim_archive_interval = 0;" in main
      and "cf->autosim_archive_interval = p.archive_interval;" in main
      and "telemetry_record_completed_turn();" in turn
      and "telemetry_archive_completed_turn();" in turn
      and turn.index("telemetry_record_completed_turn();") <
          turn.index("telemetry_archive_completed_turn();")
      and "*CurrentTurn != last_committed_turn" in tele
      and "RULES_IRONMAN" in tele
      and "skipped_ironman_rng" in tele
      and "mod_save_daemon(temp.c_str())" in tele
      and "MoveFileExA(temp.c_str(), target.c_str()" in tele
      and '"/checkpoints.jsonl"' in tele
      and "autosim_archive_interval" not in thinker_ini)
check("named operator and victory SAVs copied into campaign archive",
      "telemetry_archive_named_save(path.c_str());" in turn
      and "telemetry_archive_named_save(saved_path.c_str());" in game
      and "CopyFileA(saved_game_path, destination.c_str(), TRUE)" in tele
      and "operator_named" in tele
      and "telemetry_record_named_save" in tele)
check("event streams keep UTC timestamps, session names and roster provenance",
      "utc_stamp_ms()" in tele and "ui_notices.jsonl" in tele
      and "council.jsonl" in tele and "timings.jsonl" in tele
      and "checkpoints.jsonl" in tele
      and "MFactions[fc].filename" in tele
      and "MFactions[fc].search_key" in tele)
check("single launcher owns session picker, report actions and full ZIP",
      "New-Tab 'Campaign tools & archive'" in launcher
      and "New-Object System.Windows.Forms.ListView" in launcher
      and "Invoke-CampaignReporter -arguments @('--scan-json', $root)" in launcher
      and "Invoke-CampaignReporter -arguments @([string]$row.campaign_dir," in launcher
      and "Generate + open report" in launcher
      and "Open existing report" in launcher
      and "Open selected session folder" in launcher
      and "Choose folder..." in launcher
      and "ZipFile]::CreateFromDirectory" in launcher
      and "Pure PowerShell fallback:" in launcher
      and "Report scan/generation needs Python 3" in launcher
      and "Folder and ZIP actions remain available." in launcher
      and "Invoke-Item -LiteralPath $reportPath" in launcher
      and "campaign_report.py" in workflow
      and "Campaign Analyzer.pyw" not in launcher
      and "build/ci/tools/Campaign Analyzer.pyw" not in workflow
      and 'parser.add_argument("--scan-json"' in report
      and '"campaign_dir": str(row["campaign_dir"])' in report)
check("GUI session picker binds one campaign/session per clickable row",
      "Convert-CampaignSessionRows" in launcher
      and "Add-CampaignSessionNode" in launcher
      and "foreach ($row in $sessionList.Tag)" in launcher
      and "$sessionList.Tag = (New-Object 'System.Collections.Generic.List[object]')" in launcher
      and "$item.Tag = $row" in launcher
      and "session-002" in launcher
      and "Campaign picker collapsed independent sessions" in launcher
      and "Campaign picker multi-row filtering failed." in launcher
      and "Open selected session folder" in launcher
      and "$sessionFolder = Join-Path" in launcher)
check("offline CSV/HTML handles independent optional timing/save streams",
      "render_automation_diagnostics" in report
      and 'timings.jsonl' in report and 'checkpoints.jsonl' in report
      and '"timings.csv"' in report and '"checkpoints.csv"' in report
      and "not be added together" in report)

print("AUTO-LOG static contracts passed; game and WinForms runtime acceptance outstanding")
