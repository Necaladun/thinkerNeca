#!/usr/bin/env python3
"""Null Rendering + native quick-start static contracts. CI is not a Windows runtime."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
files={p:(r/p).read_text(encoding="utf-8") for p in (
 "src/autosim_render.cpp","src/autosim_render.h","src/gui_dialog.cpp",
 "src/gameturn.cpp","src/game.cpp","src/main.cpp","src/main.h",
 "Run Automation.ps1","src/patch.cpp","src/gui_dialog.h")}
def verify(label,ok):
    assert ok,label
    print("PASS",label)
render=files["src/autosim_render.cpp"]
dialog=files["src/gui_dialog.cpp"]
turn=files["src/gameturn.cpp"]
game=files["src/game.cpp"]
main=files["src/main.cpp"]
launcher=files["Run Automation.ps1"]
verify("seven explicit native rendering-only entry sites",
       all(x in render for x in ("0x46AF40","0x46AF90","0x46B040",
                                "0x46B140","0x46B190","0x46A550","0x46A4A0")))
verify("complete prologue preflight before all detours",
       "for (const auto& s : sites)" in render and
       "memcmp((const void*)s.address,s.expected,s.stolen)" in render and
       render.index("memcmp((const void*)s.address") < render.index("write_jump("))
verify("trampolines restore native drawing outside all-AI null mode",
       "VirtualAlloc(" in render and "FlushInstructionCache(" in render and
       all(x in render for x in ("original_map(a)","original_tile(x, y, mode)",
                                "original_mapwin_map(w,a)")) and
       render.count("if (!autosim_null_render_active())") == 7)
verify("no null rendering hooks in normal mode",
       "if (cf->autosim_render_mode == 2 && autosim_launcher_active)" in files["src/patch.cpp"])
verify("native visibility and AI mutation paths not intercepted",
       "synch_bit(" not in render and "is_visible(" not in render and
       "reveal(" not in render and "MapWin->cOwner =" not in render and
       "FactionStatus[0] =" not in render)
verify("phase-scoped null, operator dialogue restored",
       "autosim_null_render_suppress(autosim_ready());" in turn and
       'autosim_monitor_progress("ai_turn", current_id);' in turn and
       'autosim_monitor_progress("faction_upkeep", current_id);' in turn and
       'autosim_monitor_progress("turn_completed", 0);' in turn and
       'autosim_monitor_progress("operator_dialog", 0);' in turn and
       'autosim_null_render_suppress(false);' in turn)
verify("native launcher advertises game PID before its first status",
       "THINKERNECA_AUTOSIM_PROFILE" in (r/"src/launch.cpp").read_text()
       and '".pid"' in (r/"src/launch.cpp").read_text()
       and "$pidFile = $runProfilePath + '.pid'" in launcher)
verify("stalls have independently polled status, with no auto kill",
       'autosim_launch_profile_path + ".status"' in dialog and
       'autosim_launch_profile_path + ".command"' in dialog and
       'MoveFileExA(temporary.c_str(), destination.c_str()' in dialog and
       'Run-Monitor ([string]$script:selectedProfile)' in launcher and
       'No-progress warning threshold' in launcher and
       'No automatic termination' in launcher)
verify("operator may restore or reenable rendering, show game and screenshot",
       "autosim_null_override = true;" in dialog and
       "autosim_null_override = false;" in dialog and
       "Save-GameScreenshot" in launcher and
       "Show-GameWindow" in launcher)
verify("unknown X_pop decision preserved and exposes native popup",
       'autosim_null_render_popup(label);' in dialog and
       'return X_pop_9(ScriptFile, label' in dialog and
       "native_popup_restore_rendering" in dialog)
verify("profile/new-game fields accepted, guarded",
       all(x in main for x in ('"render_mode"','"world_size"','"world_ocean"',
       '"world_erosion"','"world_cloud"','"world_life"','"difficulty"',
       '"observer"','"monitor_seconds"')) and
       "autosim_auto_new_game = p.mode == \"new\"" in main and
       'p.observer < 1' in main)
verify("native quick start bypasses top menu",
       'else if (autosim_auto_new_game)' in game and
       'if (map_menu(1)' in game and
       game.index('else if (autosim_auto_new_game)') <
           game.index('else if (startup_load_path.size())') <
           game.index('else if (top_menu(0))'))
verify("AI is set before setup_game, native AI starts and no planetfall",
       'FactionStatus[0] = autosim_auto_new_game ? 0 : 1 << plr_id;' in game and
       'if (!*MultiplayerActive && !(flag & 2) && !autosim_auto_new_game)' in
            game[game.index('int __cdecl config_game('):] and
       'if (map_menu(1)' in game and
       '|| !is_alive(MapWin->cOwner)' in game)
verify("native quick setup does not persist modified player preferences",
       game.count('if (!autosim_auto_new_game) prefs_save(0);') >= 3 and
       'if (!autosim_auto_new_game) {' in game and
       'prefs_put("Latest Scenario"' in game)
verify("pre-game launcher replaces redundant all-AI first confirmation",
       'bool startup_confirmed = autosim_auto_new_game;' in turn)
verify("tabs and roughly 50pct larger launcher",
       'New-Object System.Windows.Forms.TabControl' in launcher and
       '1035,890' in launcher and
       all(x in launcher for x in ('Game & campaign','World generation',
                                  'Presentation & logs','Monitor & recovery')))
print("AUTO-NULL / AUTO-NEW static contracts passed; Windows acceptance pending")

# Native image must still be PAGE_EXECUTE_READWRITE while E9 detours are installed.
patch_src=files["src/patch.cpp"]
verify("null render hooks installed before game image returns to read/execute",
       patch_src.index("autosim_install_null_render_hooks()") <
       patch_src.index("VirtualProtect(AC_IMAGE_BASE, AC_IMAGE_LEN, PAGE_EXECUTE_READ, &attrs)"))
verify("game executable was first made writable for verified patch setup",
       patch_src.index("VirtualProtect(AC_IMAGE_BASE, AC_IMAGE_LEN, PAGE_EXECUTE_READWRITE, &attrs)") <
       patch_src.index("autosim_install_null_render_hooks()"))

# New Game Windows test follow-up, 9 November 2026: Save & Exit reused
# the persistent automatic-new-game branch and native X_pop left rendering
# re-enabled until the following cycle. Operator console initially had no focus.
game_src=files["src/game.cpp"]
gui_src=files["src/gui_dialog.cpp"]
verify("one automation launch = one game, never loop back into automatic new world",
       "if (autosim_launcher_active) {" in
       game_src[game_src.index("desktop_close();"):game_src.index("desktop_close();")+550]
       and "*ControlTurnB = 0;" in
       game_src[game_src.index("desktop_close();"):game_src.index("desktop_close();")+550]
       and "return cleanup_close();" in
       game_src[game_src.index("desktop_close();"):game_src.index("desktop_close();")+550])
verify("seven native X_pop/X_pops wrappers restore null after modal returns",
       gui_src.count("AutosimNativePopupRenderScope native_popup(label);") == 7
       and "if (had_null && autosim_control_active())" in gui_src
       and "autosim_null_render_suppress(true);" in gui_src
       and "~AutosimNativePopupRenderScope()" in gui_src)
verify("NULL command immediately reenables null while all-AI eligible",
       "if (autosim_control_active()) autosim_null_phase = true;" in gui_src)
verify("monitor exposes actual render selection and phase state",
       all(x in gui_src for x in ("configured_render=%d",
                                 "null_phase=%d", "null_override=%d")))
verify("monitor attempts foreground console on first AI processing phase",
       "GetConsoleWindow" in launcher and "consoleFocusAttempted" in launcher
       and "SetForegroundWindow($consoleWindow)" in launcher)
