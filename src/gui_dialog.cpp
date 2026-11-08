
#include "gui_dialog.h"
#include "autosim.h"


int __thiscall Popup_load_icon(Popup* This, Sprite* a2, int a3, char* a4) {
    // replace inlined function sub_559000
    if (This->dialogs.spriteBox.strings.count) {
        This->sprite_icon = 0;
    } else {
        This->sprite_icon = a2;
    }
    return SpriteBox_sprite((SpriteBox*)&This->dialogs.spriteBox, a2, a4, a3);
}


// This is a runtime guard: no user-controlled input is bypassed unless
// Scenario Editor has actually handed every faction to AI. No preferences
// or native game-mode flags are changed.
bool autosim_control_active() {
    const int owner = MapWin->cOwner;
    return autosim_launcher_active && auto_turn_eligible(conf.autosim_manual_turns,
        (unsigned)FactionStatus[0], *MultiplayerActive != 0,
        *PbemActive != 0, (*GameState & STATE_GAME_DONE) != 0,
        owner, owner >= 1 && owner < MaxPlayerNum && is_alive(owner));
}

bool autosim_ui_active() {
    return conf.autosim_suppress_notices && autosim_control_active();
}

int autosim_notice_netmsg(NetMessage* msg, const char* label, int delay,
    int style, const char* image) {
    // Only called at individually audited, presentation-only sites.
    // Never wrap native NetMsg_pop globally: it is used for other modes
    // and could later have interactive labels.
    if (autosim_ui_active()) {
        telemetry_record_ui_notice("netmsg", label, "suppressed_notice");
        return 0;
    }
    return NetMsg_pop(msg, label, delay, style, image);
}

// Source-audited, return-unused intelligence messages; do not intercept
// probe action menus or prompts with meaningful return values.
// Treaty and pact announcement call sites ignore return values. Do not
// use for any X_dialog path whose answer changes diplomacy or resources.
int autosim_notice_dialog(const char* label, int faction_id) {
    if (autosim_ui_active()) {
        telemetry_record_ui_notice("diplomatic_dialog", label, "suppressed_notice");
        return 0;
    }
    return X_dialog(label, faction_id);
}

int autosim_notice_netmsg2(const char* label, const char* image) {
    if (autosim_ui_active()) {
        telemetry_record_ui_notice("netmsg2", label, "suppressed_notice");
        return 0;
    }
    return NetMsg_pop_2(label, image);
}

// Third-party diplomacy reports retain foreign_treaty_popup behaviour when
// AUTO-002 is inactive. All source call sites ignore the returned integer.
int autosim_notice_diplomacy(NetMessage* msg, const char* label, int delay,
    int style, const char* image) {
    if (autosim_ui_active()) {
        telemetry_record_ui_notice("diplomacy", label, "suppressed_notice");
        return 0;
    }
    return mod_NetMsg_pop(msg, label, delay, style, image);
}

// The UI event handler only posts a request; the driver consumes it after
// a completed turn, never inside combat, diplomacy or base upkeep.
static AutosimControlRequest autosim_control_request = AUTOSIM_CTL_NONE;
void autosim_request_control(AutosimControlRequest request) {
    if (!autosim_control_active()) return;
    if (request == AUTOSIM_CTL_STOP || autosim_control_request != AUTOSIM_CTL_STOP)
        autosim_control_request = request;
}
// Native popup/engine dispatch can consume WM_KEYDOWN before our polling
// loop sees it. Sample the physical key state as an independent, focus-gated
// input path; no game message pumping or asynchronous state mutation occurs.
// The key must be held until sampled during AI processing. A stop-file
// request (below) works without any key or window focus.
static void autosim_poll_physical_shortcuts() {
    static bool f9_was_down = false;
    static bool f10_was_down = false;
    static bool menu_was_down = false;
    if (!autosim_control_active()) {
        f9_was_down = false;
        f10_was_down = false;
        menu_was_down = false;
        return;
    }
    DWORD foreground_process = 0;
    HWND foreground = GetForegroundWindow();
    if (foreground) {
        GetWindowThreadProcessId(foreground, &foreground_process);
    }
    if (foreground_process != GetCurrentProcessId()) {
        f9_was_down = false;
        f10_was_down = false;
        menu_was_down = false;
        return;
    }
    const bool f9_down = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;
    const bool f10_down = (GetAsyncKeyState(VK_F10) & 0x8000) != 0;
    const bool menu_down = (GetAsyncKeyState(VK_F12) & 0x8000) != 0
        && (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0
        && (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0
        && (GetAsyncKeyState(VK_MENU) & 0x8000) == 0;
    if (f9_down && !f9_was_down) {
        autosim_request_control(AUTOSIM_CTL_PAUSE);
        telemetry_record_ui_notice("autosim_input", "F9", "physical_latched");
    }
    if (f10_down && !f10_was_down) {
        autosim_request_control(AUTOSIM_CTL_STOP);
        telemetry_record_ui_notice("autosim_input", "F10", "physical_latched");
    }
    if (menu_down && !menu_was_down) {
        autosim_request_control(AUTOSIM_CTL_MENU);
        telemetry_record_ui_notice("autosim_input", "CTRL_SHIFT_F12", "physical_latched");
    }
    f9_was_down = f9_down;
    f10_was_down = f10_down;
    menu_was_down = menu_down;
}

// During a continuous all-AI control_turn() invocation the ordinary Win32
// message loop may not dispatch keyboard events. Read only queued keyboard
// messages (never dispatch them into native gameplay) so F9/F10 can latch.
// Key-up, repeats, printable keys and legacy gameplay shortcuts are ignored
// while no human faction is under control. This does NOT pump arbitrary game
// messages or create a reentrant modal/AI action.
void autosim_collect_queued_keys() {
    if (!autosim_control_active()) return;
    autosim_poll_physical_shortcuts();
    MSG msg = {};
    while (PeekMessageA(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE)) {
        // Windows treats F10 as a system key: its queued message may be
        // WM_SYSKEYDOWN rather than WM_KEYDOWN. Capture both, but keep
        // ordinary keys out of gameplay during unattended AI turns.
        if ((msg.message != WM_KEYDOWN && msg.message != WM_SYSKEYDOWN)
        || (msg.lParam & (1u << 30)))
            continue;
        if (msg.wParam == VK_F10) {
            autosim_request_control(AUTOSIM_CTL_STOP);
            telemetry_record_ui_notice("autosim_input", "F10", "queued_latched");
        } else if (msg.wParam == VK_F9) {
            autosim_request_control(AUTOSIM_CTL_PAUSE);
            telemetry_record_ui_notice("autosim_input", "F9", "queued_latched");
        } else if (msg.wParam == VK_F12
        && (GetAsyncKeyState(VK_CONTROL) & 0x8000)
        && (GetAsyncKeyState(VK_SHIFT) & 0x8000)
        && !(GetAsyncKeyState(VK_MENU) & 0x8000)) {
            autosim_request_control(AUTOSIM_CTL_MENU);
            telemetry_record_ui_notice("autosim_input", "CTRL_SHIFT_F12", "queued_latched");
        }
    }
}

// Independent external stop mechanism: creating autosim-stop.txt beside
// thinker.exe/terranx.exe requests the existing Save Game -> menu path at the
// NEXT completed AI cycle. This never enters a native dialog from an incomplete
// AI faction/combat/upkeep action. It does NOT interrupt an already blocking
// Council modal: the caller must regain control before the file is checked.
// Remove the file on receipt so the next simulation is not stopped by stale
// requests. Resolve relative to the executable, not the launcher's CWD.
void autosim_collect_stop_file() {
    if (!autosim_control_active()) return;
    char executable[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameA(NULL, executable, MAX_PATH);
    if (!length || length >= MAX_PATH) return;
    std::string path(executable, length);
    const size_t slash = path.find_last_of("\\/");
    if (slash == std::string::npos) return;
    path.replace(slash + 1, std::string::npos, "autosim-stop.txt");
    const DWORD attributes = GetFileAttributesA(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES
    || (attributes & FILE_ATTRIBUTE_DIRECTORY)) return;
    const bool removed = DeleteFileA(path.c_str()) != 0;
    autosim_request_control(AUTOSIM_CTL_STOP);
    telemetry_record_ui_notice("autosim", "EXTERNAL_STOP",
        removed ? "queued_file_consumed" : "queued_file_could_not_be_removed");
}

bool autosim_fast_combat_active() {
    return conf.autosim_fast_combat != 0 && autosim_control_active();
}

AutosimControlRequest autosim_take_control_request() {
    const AutosimControlRequest value = autosim_control_request;
    autosim_control_request = AUTOSIM_CTL_NONE;
    return value;
}

int autosim_notice_popp(const char* filename, const char* label,
    const char* image) {
    // Only use where caller *ignores* return value. Do not use for
    // diplomacy, policy, build, or other gameplay choices.
    if (autosim_ui_active()) {
        telemetry_record_ui_notice("popp", label, "suppressed_notice");
        return 0;
    }
    return popp(filename, label, 0, image, 0);
}

// Only wrap explicitly audited one-way POP2 call sites; the global POP2
// routine is also used by gameplay events whose behaviour is unclassified.
void autosim_notice_pop2(const char* label, const char* image, int recipient) {
    if (autosim_ui_active()) {
        telemetry_record_ui_notice("pop2", label, "suppressed_notice");
        return;
    }
    POP2(label, image, recipient);
}

// Native interlude() handles event-history as well as presentation. Run the
// native handler with its *existing* disabled-interludes preference enabled
// only for this synchronous call, then restore exactly the previous flags.
// The option is not written to any preference file or SAV by this wrapper.
// Whether the native handler maintains its event flags with this preference
// enabled still needs same-build Windows save/reload inspection.
void autosim_interlude(int event_id, const char* text, int a3, int a4) {
    if (!autosim_ui_active()) {
        interlude(event_id, text, a3, a4);
        return;
    }
    const unsigned prior_interlude_state = (unsigned)*GameInterludeState;
    {
        AutoTemporaryPrefFlag disable_ui(GamePreferences, PREF_AV_INTERLUDES_DISABLED);
        interlude(event_id, text, a3, a4);
    }
    char label[40];
    char action[96];
    snprintf(label, sizeof(label), "INTERLUDE_%d", event_id);
    snprintf(action, sizeof(action), "native_disable_requested_state_%08X_to_%08X",
        prior_interlude_state, (unsigned)*GameInterludeState);
    telemetry_record_ui_notice("interlude", label, action);
}

int __cdecl X_pop(const char* label, fp_none fn)
{
    if (autosim_ui_active()) {
        const AutoUiAction action = autosim_xpop_policy(label, fn != 0);
        if (action != AUTO_UI_NATIVE) {
            telemetry_record_ui_notice("x_pop", label,
                action == AUTO_UI_NOTICE ? "suppressed_notice" : "declined_optional_human_ui");
            return 0;
        }
    }
    if (!conf.warn_on_former_replace && !strcmp(label, "MIMIMI")) {
        return 1;
    }
    return X_pop_9(ScriptFile, label, -1, 0, 0, fn);
}

int __cdecl X_pop_2(const char* filename, const char* label, fp_none fn)
{
    if (autosim_ui_active()) {
        const AutoUiAction action = autosim_xpop_policy(label, fn != 0);
        if (action != AUTO_UI_NATIVE) {
            telemetry_record_ui_notice("x_pop_2", label,
                action == AUTO_UI_NOTICE ? "suppressed_notice" : "declined_optional_human_ui");
            return 0;
        }
    }
    return X_pop_9(filename, label, -1, 0, 0, fn);
}

int __cdecl X_pop_6(const char* label, int a2, fp_none fn)
{
    return X_pop_9(ScriptFile, label, -1, 0, a2, fn);
}

int __cdecl X_pops(const char* label, Sprite* a2, fp_none fn)
{
    return X_pops_18(ScriptFile, label, -1, 0, 0, a2, 1, 1, fn);
}

int __cdecl X_pops_11(const char* label, int a2, Sprite* a3, fp_none fn)
{
    return X_pops_18(ScriptFile, label, -1, 0, a2, a3, 1, 1, fn);
}

int __cdecl X_dialog(const char* label, int faction_id)
{
    return X_pops_18(ScriptFile, label, -1, 0, POPUP_UNK_100000,
        FactionPortraits[faction_id], 1, 1, pop_wait);
}

int __cdecl X_dialog_alt(const char* filename, const char* label, int faction_id)
{
    return X_pops_18(filename, label, -1, 0, POPUP_UNK_100000,
        FactionPortraits[faction_id], 1, 1, pop_wait);
}



