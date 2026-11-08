#!/usr/bin/env python3
"""AUTO-002 unified operator menu/startup and linked-save settings contracts.

Source checks only; actual Win32 modal focus, DX UI, game state transitions
and same-build save/reload must be validated with the native game.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
dialog_h = (ROOT / "src/autosim_control.h").read_text(encoding="utf-8")
dialog = (ROOT / "src/autosim_control.cpp").read_text(encoding="utf-8")
gui = (ROOT / "src/gui.cpp").read_text(encoding="utf-8")
keys = (ROOT / "src/gui_dialog.cpp").read_text(encoding="utf-8")
keys_h = (ROOT / "src/gui_dialog.h").read_text(encoding="utf-8")
turn = (ROOT / "src/gameturn.cpp").read_text(encoding="utf-8")
tele = (ROOT / "src/telemetry.cpp").read_text(encoding="utf-8")
tele_h = (ROOT / "src/telemetry.h").read_text(encoding="utf-8")
game = (ROOT / "src/game.cpp").read_text(encoding="utf-8")

# Dedicated, modifier-gated shortcut works in ordinary interactive world
# play and requests a boundary-only menu during live all-AI automation.
wnd = gui[gui.index("LRESULT WINAPI ModWinProc("):]
assert "msg == WM_KEYDOWN && wParam == VK_F12" in wnd
assert "ctrl_key_down() && shift_key_down() && !alt_key_down()" in wnd
assert "autosim_request_control(AUTOSIM_CTL_MENU);" in wnd
assert "current_window() == GW_World && win_has_focus()" in wnd
assert "!*WinModalState" in wnd and "autosim_show_manual_setup();" in wnd
assert "wParam == VK_F9 || wParam == VK_F10" in wnd  # retained aliases
assert "AUTOSIM_CTL_MENU" in keys_h
for key in ("VK_F12", "VK_CONTROL", "VK_SHIFT", "VK_MENU"):
    assert f"GetAsyncKeyState({key})" in keys
assert "autosim_poll_physical_shortcuts();" in keys
assert "autosim_request_control(AUTOSIM_CTL_MENU);" in keys
assert "if (!autosim_control_active()) return;" in keys

# Three intentional modes share one Win32 DialogBox, rather than two
# independent control models or any periodic UI pumping from combat.
for label in ("AUTOSIM_DIALOG_SETUP", "AUTOSIM_DIALOG_STARTUP",
              "AUTOSIM_DIALOG_RUNNING", "AUTOSIM_DIALOG_START",
              "AUTOSIM_DIALOG_ARM", "AUTOSIM_DIALOG_CONTINUE",
              "AUTOSIM_DIALOG_CANCEL"):
    assert label in dialog_h
    assert label in dialog
assert dialog.count("DialogBoxIndirectParamA(") == 1
for field in ("Campaign:", "Session:", "Telemetry:",
              "Campaign display name", "Completed AI turns per segment",
              "Suppress audited, noninteractive notices",
              "Fast AI combat/presentation", "Save && Continue",
              "Save && Exit", "Take Control", "Start Simulation",
              "Arm Simulation", "Continue", "Resume Unchanged"):
    assert field in dialog
assert "if (id == ID_SAVE_CONTINUE || id == ID_SAVE_EXIT" in dialog
assert "parsed < 1 || parsed > 100000" in dialog
assert "if ((id == ID_SAVE_CONTINUE || id == ID_SAVE_EXIT)" in dialog
assert 'SendDlgItemMessageA(dialog, ID_SUPPRESS, BM_GETCHECK' in dialog
assert 'SendDlgItemMessageA(dialog, ID_FAST_COMBAT, BM_GETCHECK' in dialog
assert "conf.autosim_manual_turns = state.additional_turns;" in dialog
assert "conf.autosim_suppress_notices = state.suppress_notices ? 1 : 0;" in dialog
assert "conf.autosim_fast_combat = state.fast_combat ? 1 : 0;" in dialog
assert 'telemetry_record_ui_notice("autosim", "SETTINGS", action);' in dialog
assert "void autosim_show_manual_setup()" in dialog
assert "if (autosim_show_control_dialog(&state) != AUTOSIM_DIALOG_ARM)" in dialog

# Consent at initial all-AI handover, including a fingerprint-linked loaded
# session whose local INI no longer contains a positive turn budget.
loop = turn[turn.index("void __cdecl control_turn()"):]
assert "bool startup_confirmed = false;" in loop
assert "autosim_preflight_ready = [&]()" in loop
assert "((unsigned)FactionStatus[0] & 0xFEu) == 0" in loop
assert "if (autosim_preflight_ready() && !startup_confirmed)" in loop
assert "autosim_fill_dialog_state(&preflight, AUTOSIM_DIALOG_STARTUP);" in loop
assert "if (answer == AUTOSIM_DIALOG_START)" in loop
assert "if (!autosim_apply_dialog_options(preflight))" in loop
assert "startup_confirmed = true;" in loop
assert "set_human(MapWin->cOwner, true);" in loop
assert loop.index("if (autosim_preflight_ready() && !startup_confirmed)") < loop.index("if (run_upkeep)")

# At completed-turn boundaries, no-save Continue starts a fresh budget,
# save actions retain the temp/native serializer and normal F9/F10.
boundary = loop[loop.index("if (++current_id >= 8)"):loop.index("int __cdecl net_upkeep_phase(")]
assert "operator_menu = eligible && request == AUTOSIM_CTL_MENU;" in boundary
assert "autosim_fill_dialog_state(&dialog, AUTOSIM_DIALOG_RUNNING);" in boundary
assert boundary.index("telemetry_record_completed_turn();") < boundary.index(
    "autosim_show_control_dialog(&dialog);")
assert "if (selection == AUTOSIM_DIALOG_CONTINUE)" in boundary
assert "auto_driver.completed_turns = 0;" in boundary
assert "if (selection == AUTOSIM_DIALOG_SAVE_CONTINUE" in boundary
assert "if (!autosim_save_named(dialog.save_name, &path, &error))" in boundary
assert "if (selection == AUTOSIM_DIALOG_SAVE_EXIT)" in boundary
assert "if (selection == AUTOSIM_DIALOG_RESUME)" in boundary
assert "handover_to_human_next_turn" in boundary

# Campaign/session identity is distinct from user-entered display name.
for term in ("telemetry_get_campaign_id", "telemetry_get_session_id",
             "telemetry_resumed_linked_campaign"):
    assert term in tele_h and term in tele
assert "state->campaign_id = telemetry_get_campaign_id();" in dialog
assert "state->session_id = telemetry_get_session_id();" in dialog
assert "state->resumed_campaign = telemetry_resumed_linked_campaign();" in dialog

# New sidecars append an optional runtime profile while preserving old
# fingerprint-bound v2 linked-save format. Restore profile ONLY after the
# operator accepts the existing explicit Yes/No rejoin prompt.
assert 'fgets(options, sizeof(options), fp)' in tele
assert 'sscanf(options, "autosim-v1 %d %d %d %c"' in tele
assert '"autosim-v1 %d %d %d\\n"' in tele
begin = tele[tele.index("void telemetry_begin_session("):
             tele.index("void telemetry_record_ui_notice(")]
assert begin.index("== IDYES") < begin.index("resumed_linked_campaign = true;")
assert begin.index("resumed_linked_campaign = true;") < begin.index("if (linked_turns > 0)")
assert "conf.autosim_manual_turns = linked_turns;" in begin
assert "conf.autosim_suppress_notices = linked_notices;" in begin
assert "conf.autosim_fast_combat = linked_fast;" in begin
assert "file_fingerprint(candidate.c_str()) != fingerprint" in tele
assert "if (MessageBoxA(*phWnd, prompt.c_str()" in tele
assert "telemetry_begin_session(telemetry_source_save.c_str());" in game

print("AUTO-002 unified Simulation Control/startup source contract passed (not native Windows runtime)")
