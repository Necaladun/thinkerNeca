#!/usr/bin/env python3
"""AUTO-002 operator input/stop-file source contract.

Static checks only: Windows runtime must verify key focus, modal blocking,
one-shot file deletion, Save Game dialog, and loaded-game continuation.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ui = (ROOT / "src/gui_dialog.cpp").read_text(encoding="utf-8")
ui_h = (ROOT / "src/gui_dialog.h").read_text(encoding="utf-8")
turn = (ROOT / "src/gameturn.cpp").read_text(encoding="utf-8")
veh = (ROOT / "src/veh_turn.cpp").read_text(encoding="utf-8")

def require(condition, reason):
    assert condition, reason

keys = ui[ui.index("static void autosim_poll_physical_shortcuts()"):
          ui.index("bool autosim_fast_combat_active()")]
require("GetForegroundWindow()" in keys
        and "GetWindowThreadProcessId" in keys
        and "foreground_process != GetCurrentProcessId()" in keys,
        "Physical hotkeys must only activate while game owns foreground window")
for key, action in (("VK_F9", "AUTOSIM_CTL_PAUSE"), ("VK_F10", "AUTOSIM_CTL_STOP")):
    require(f"GetAsyncKeyState({key}) & 0x8000" in keys,
            f"{key} physical polling missing")
    require(f"autosim_request_control({action});" in keys,
            f"{key} physical request does not latch")
require("PeekMessageA(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE)" in keys,
        "Existing queued keyboard route disappeared")
require("autosim_poll_physical_shortcuts();" in keys,
        "Physical key polling not part of periodic AI collection")
require("if (!autosim_control_active()) return;" in keys,
        "Operator controls must not intercept normal human play")

file_ctrl = ui[ui.index("void autosim_collect_stop_file()"):
               ui.index("bool autosim_fast_combat_active()")]
for token in ('GetModuleFileNameA(NULL, executable, MAX_PATH)',
              'path.find_last_of(', '"autosim-stop.txt"',
              'GetFileAttributesA(path.c_str())', 'DeleteFileA(path.c_str())',
              'autosim_request_control(AUTOSIM_CTL_STOP);'):
    require(token in file_ctrl, f"Missing external stop guard/operation: {token}")
require(file_ctrl.index("if (!autosim_control_active())") <
        file_ctrl.index("GetFileAttributesA"),
        "File flag must be ignored in human/multiplayer/noneligible sessions")
require('autosim_collect_stop_file();' in ui_h,
        "Header declaration for external stop missing")
require('if (!first_cycle_partial) {\n                autosim_collect_stop_file();' in turn,
        "Stop-file check must not execute at a loaded partial-turn boundary")
boundary = turn[turn.index("if (++current_id >= 8)"):
                turn.index("int __cdecl net_upkeep_phase(")]
require(boundary.index("autosim_collect_stop_file();") <
        boundary.index("autosim_take_control_request();"),
        "Stop-file command must be latched before completed-cycle request handling")
require(boundary.index("telemetry_record_completed_turn();") <
        boundary.index("autosim_take_control_request();"),
        "Completed turn must be recorded before operator stops")
require("save_game(0);" in boundary and "*ControlTurnA = 1;" in boundary,
        "Stop must use existing native Save Game -> menu route")
require(veh.count("autosim_collect_queued_keys();") >= 2,
        "AI unit processing must continue sampling keyboard state")
print("AUTO-002 physical shortcut/stop-file structural contract passed (not native runtime)")
