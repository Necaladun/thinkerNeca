#include "autosim_control.h"
#include "main.h"

#include <cctype>
#include <cerrno>
#include <climits>

namespace {

enum AutosimChildId {
    ID_CAMPAIGN_NAME = 6001,
    ID_SAVE_NAME = 6002,
    ID_MORE_TURNS = 6003,
    ID_SAVE_CONTINUE = 6004,
    ID_SAVE_EXIT = 6005,
    ID_TAKE_CONTROL = 6006,
    ID_RESUME = 6007,
    ID_START = 6008,
    ID_ARM = 6009,
    ID_CONTINUE = 6010,
    ID_CANCEL = 6011,
    ID_SUPPRESS = 6012,
    ID_FAST_COMBAT = 6013,
};

struct DialogContext {
    AutosimDialogState* state;
    bool ready = false;
    bool save_name_dirty = false;
    bool refreshing = false;
};

std::string trim_text(const std::string& value) {
    const size_t left = value.find_first_not_of(" \t\r\n");
    if (left == std::string::npos) return "";
    const size_t right = value.find_last_not_of(" \t\r\n");
    return value.substr(left, right - left + 1);
}

std::string save_slug(const char* text) {
    std::string slug;
    bool pending_dash = false;
    if (!text) text = "";
    for (const unsigned char* p = (const unsigned char*)text; *p; ++p) {
        const unsigned char ch = *p;
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
        || (ch >= '0' && ch <= '9') || ch == '_' || ch == '-') {
            if (pending_dash && !slug.empty() && slug.back() != '-') slug.push_back('-');
            pending_dash = false;
            slug.push_back((char)ch);
        } else {
            pending_dash = true;
        }
        if (slug.size() >= 48) break;
    }
    while (!slug.empty() && slug.back() == '-') slug.pop_back();
    return slug.empty() ? "AI-Sim" : slug;
}

bool is_safe_save_name(const std::string& text) {
    // Do not allow absolute/relative paths or Win32 reserved device names.
    if (text.empty() || text.size() > 72 || text == "." || text == ".."
    || text.back() == '.' || text.back() == ' ') return false;
    for (unsigned char c : text) {
        if (!(std::isalnum(c) || c == ' ' || c == '_' || c == '-' || c == '.'))
            return false;
    }
    std::string stem = text.substr(0, text.find('.'));
    for (char& c : stem) c = (char)std::toupper((unsigned char)c);
    if (stem == "CON" || stem == "PRN" || stem == "AUX" || stem == "NUL"
    || (stem.size() == 4 && (stem.substr(0, 3) == "COM"
        || stem.substr(0, 3) == "LPT")
        && stem[3] >= '1' && stem[3] <= '9')) return false;
    return true;
}

void set_font(HWND child) {
    if (child) SendMessageA(child, WM_SETFONT,
        (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
}

void add_child(HWND parent, const char* class_name, const char* text,
    DWORD style, DWORD ex_style, int id, int x, int y, int w, int h) {
    HWND child = CreateWindowExA(ex_style, class_name, text,
        WS_CHILD | WS_VISIBLE | style, x, y, w, h, parent,
        (HMENU)(intptr_t)id, GetModuleHandleA(NULL), NULL);
    set_font(child);
}

std::string read_field(HWND dialog, int id) {
    char buffer[128] = {};
    GetDlgItemTextA(dialog, id, buffer, sizeof(buffer));
    return trim_text(buffer);
}

INT_PTR CALLBACK autosim_dialog_proc(HWND dialog, UINT message,
    WPARAM wparam, LPARAM lparam) {
    if (message == WM_INITDIALOG) {
        DialogContext* ctx = reinterpret_cast<DialogContext*>(lparam);
        SetWindowLongPtrA(dialog, DWLP_USER, (LONG_PTR)ctx);
        const AutosimDialogState& s = *ctx->state;
        SetWindowTextA(dialog, s.mode == AUTOSIM_DIALOG_SETUP
            ? "thinkerNeca - Simulation Setup"
            : "thinkerNeca - Simulation Control");
        RECT bounds = {};
        GetClientRect(dialog, &bounds);
        const int width = bounds.right;
        const int height = bounds.bottom;
        const int left = 20;
        const int inner = width - 40;

        const char* intro = s.mode == AUTOSIM_DIALOG_SETUP
            ? "Prepare a simulation. Hand over all factions in Scenario Editor, then End Turn."
            : s.mode == AUTOSIM_DIALOG_STARTUP
            ? "All factions are AI-controlled. Confirm before starting unattended turns."
            : "Simulation paused at a completed full-turn boundary.";
        add_child(dialog, "STATIC", intro,
            SS_LEFT, 0, 0, left, 13, inner, 23);
        char summary[384] = {};
        snprintf(summary, sizeof(summary),
            "Observed faction: %d    Humans mask: %02X    Living factions: %d\r\n"
            "Turns: %d segment / %d this run.  Mission years: %d -> %d\r\n"
            "Colonies: %d -> %d    Units: %d -> %d",
            s.viewed_faction, (unsigned)(s.human_mask & 0xFF), s.alive_factions,
            s.completed_segment, s.completed_total, s.start_year, s.end_year,
            s.start_bases, s.end_bases, s.start_units, s.end_units);
        add_child(dialog, "STATIC", summary, SS_LEFT, 0, 0, left, 43, inner, 66);

        char session[256] = {};
        snprintf(session, sizeof(session),
            "Campaign: %.60s\r\nSession: %.32s    Telemetry: %s    Rejoined: %s",
            s.campaign_id.empty() ? "(not recording)" : s.campaign_id.c_str(),
            s.session_id.empty() ? "(none)" : s.session_id.c_str(),
            s.telemetry_enabled ? "ON" : "OFF",
            s.resumed_campaign ? "Yes" : "No");
        add_child(dialog, "STATIC", session, SS_LEFT, 0, 0, left, 111, inner, 43);

        add_child(dialog, "STATIC", "Campaign display name (does not change campaign/session ID)",
            SS_LEFT, 0, 0, left, 157, inner, 20);
        add_child(dialog, "EDIT", s.campaign_name.c_str(),
            WS_TABSTOP | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ID_CAMPAIGN_NAME,
            left, 179, inner, 25);
        SendDlgItemMessageA(dialog, ID_CAMPAIGN_NAME, EM_LIMITTEXT, 88, 0);

        add_child(dialog, "STATIC", "Save filename (saves/, .sav added automatically)",
            SS_LEFT, 0, 0, left, 215, inner, 20);
        add_child(dialog, "EDIT", s.save_name.c_str(),
            WS_TABSTOP | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ID_SAVE_NAME,
            left, 237, inner, 25);
        SendDlgItemMessageA(dialog, ID_SAVE_NAME, EM_LIMITTEXT, 72, 0);

        add_child(dialog, "STATIC", "Completed AI turns per segment (1-100000)",
            SS_LEFT, 0, 0, left, 273, inner, 20);
        char count[24] = {};
        snprintf(count, sizeof(count), "%d", s.additional_turns);
        add_child(dialog, "EDIT", count,
            WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER, WS_EX_CLIENTEDGE,
            ID_MORE_TURNS, left, 295, 140, 26);
        SendDlgItemMessageA(dialog, ID_MORE_TURNS, EM_LIMITTEXT, 6, 0);

        add_child(dialog, "BUTTON", "Suppress audited, noninteractive notices",
            WS_TABSTOP | BS_AUTOCHECKBOX, 0, ID_SUPPRESS, left, 342, inner, 23);
        SendDlgItemMessageA(dialog, ID_SUPPRESS, BM_SETCHECK,
            s.suppress_notices ? BST_CHECKED : BST_UNCHECKED, 0);
        add_child(dialog, "BUTTON", "Fast AI combat/presentation",
            WS_TABSTOP | BS_AUTOCHECKBOX, 0, ID_FAST_COMBAT, left, 370, inner, 23);
        SendDlgItemMessageA(dialog, ID_FAST_COMBAT, BM_SETCHECK,
            s.fast_combat ? BST_CHECKED : BST_UNCHECKED, 0);

        add_child(dialog, "STATIC",
            "Changes here apply in memory only. Session logging is configured at game load.",
            SS_LEFT, 0, 0, left, 402, inner, 28);

        const int gap = 8;
        const int y = height - 59;
        int ids[5] = {};
        const char* captions[5] = {};
        int buttons = 0;
        if (s.mode == AUTOSIM_DIALOG_SETUP) {
            ids[0] = ID_ARM; captions[0] = "Arm Simulation";
            ids[1] = ID_CANCEL; captions[1] = "Close";
            buttons = 2;
        } else if (s.mode == AUTOSIM_DIALOG_STARTUP) {
            ids[0] = ID_START; captions[0] = "Start Simulation";
            ids[1] = ID_TAKE_CONTROL; captions[1] = "Take Control";
            buttons = 2;
        } else if (s.mode == AUTOSIM_DIALOG_RUNNING) {
            ids[0] = ID_SAVE_CONTINUE; captions[0] = "Save && Continue";
            ids[1] = ID_SAVE_EXIT; captions[1] = "Save && Exit";
            ids[2] = ID_TAKE_CONTROL; captions[2] = "Take Control";
            ids[3] = ID_CONTINUE; captions[3] = "Continue";
            buttons = 4;
            if (s.allow_resume) {
                ids[4] = ID_RESUME; captions[4] = "Resume Unchanged";
                buttons = 5;
            }
        }
        const int button_width = (inner - gap * (buttons - 1)) / buttons;
        for (int i = 0; i < buttons; ++i) {
            add_child(dialog, "BUTTON", captions[i],
                WS_TABSTOP | (i == 0 ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON),
                0, ids[i], left + i * (button_width + gap),
                y, button_width, 31);
        }
        ctx->ready = true;
        return TRUE;
    }
    DialogContext* ctx = reinterpret_cast<DialogContext*>(
        GetWindowLongPtrA(dialog, DWLP_USER));
    if (message == WM_CLOSE) {
        if (ctx && ctx->state->mode == AUTOSIM_DIALOG_SETUP) {
            EndDialog(dialog, AUTOSIM_DIALOG_CANCEL);
        } else if (ctx && ctx->state->allow_resume) {
            EndDialog(dialog, AUTOSIM_DIALOG_RESUME);
        } else {
            MessageBeep(MB_ICONINFORMATION);
        }
        return TRUE;
    }
    if (message != WM_COMMAND || !ctx) return FALSE;
    const int id = LOWORD(wparam);
    if (ctx->ready && HIWORD(wparam) == EN_CHANGE) {
        if (id == ID_SAVE_NAME && !ctx->refreshing) {
            ctx->save_name_dirty = true;
        } else if (id == ID_CAMPAIGN_NAME && !ctx->save_name_dirty) {
            const std::string name = read_field(dialog, ID_CAMPAIGN_NAME);
            const std::string suggested = autosim_default_save_name(
                name.c_str(), ctx->state->end_year);
            ctx->refreshing = true;
            SetDlgItemTextA(dialog, ID_SAVE_NAME, suggested.c_str());
            ctx->refreshing = false;
        }
        return FALSE;
    }
    if (id != ID_SAVE_CONTINUE && id != ID_SAVE_EXIT
    && id != ID_TAKE_CONTROL && id != ID_RESUME
    && id != ID_START && id != ID_ARM && id != ID_CONTINUE
    && id != ID_CANCEL) return FALSE;
    if (id == ID_CANCEL) {
        EndDialog(dialog, AUTOSIM_DIALOG_CANCEL);
        return TRUE;
    }
    if (id == ID_RESUME) {
        EndDialog(dialog, AUTOSIM_DIALOG_RESUME);
        return TRUE;
    }
    const std::string campaign = read_field(dialog, ID_CAMPAIGN_NAME);
    if (campaign.empty() || campaign.size() > 88) {
        MessageBoxA(dialog, "Enter a campaign name of 1-88 characters.",
            "thinkerNeca Simulation Control", MB_OK | MB_ICONWARNING);
        return TRUE;
    }
    for (unsigned char c : campaign) {
        if (c < 32 || c == 127) {
            MessageBoxA(dialog, "Campaign names cannot contain control characters.",
                "thinkerNeca Simulation Control", MB_OK | MB_ICONWARNING);
            return TRUE;
        }
    }
    const std::string save = read_field(dialog, ID_SAVE_NAME);
    if ((id == ID_SAVE_CONTINUE || id == ID_SAVE_EXIT)
    && !is_safe_save_name(save)) {
        MessageBoxA(dialog,
            "Save name must be 1-72 characters using letters, digits,"
            " spaces, hyphens, underscores or dots. No paths/device names.",
            "thinkerNeca Simulation Control", MB_OK | MB_ICONWARNING);
        return TRUE;
    }
    int count = ctx->state->additional_turns;
    if (id == ID_SAVE_CONTINUE || id == ID_SAVE_EXIT
    || id == ID_START || id == ID_ARM || id == ID_CONTINUE) {
        const std::string input = read_field(dialog, ID_MORE_TURNS);
        if (input.empty() || input.size() > 6) count = 0;
        else {
            char* end = NULL;
            errno = 0;
            long parsed = strtol(input.c_str(), &end, 10);
            if (errno || !end || *end || parsed < 1 || parsed > 100000) count = 0;
            else count = (int)parsed;
        }
        if (count <= 0) {
            MessageBoxA(dialog, "Enter 1-100000 additional whole turns.",
                "thinkerNeca Simulation Control", MB_OK | MB_ICONWARNING);
            return TRUE;
        }
    }
    ctx->state->campaign_name = campaign;
    if (id != ID_TAKE_CONTROL) ctx->state->save_name = save;
    ctx->state->additional_turns = count;
    ctx->state->suppress_notices =
        SendDlgItemMessageA(dialog, ID_SUPPRESS, BM_GETCHECK, 0, 0) == BST_CHECKED;
    ctx->state->fast_combat =
        SendDlgItemMessageA(dialog, ID_FAST_COMBAT, BM_GETCHECK, 0, 0) == BST_CHECKED;
    EndDialog(dialog,
        id == ID_SAVE_CONTINUE ? AUTOSIM_DIALOG_SAVE_CONTINUE
        : id == ID_SAVE_EXIT ? AUTOSIM_DIALOG_SAVE_EXIT
        : id == ID_TAKE_CONTROL ? AUTOSIM_DIALOG_TAKE_CONTROL
        : id == ID_START ? AUTOSIM_DIALOG_START
        : id == ID_ARM ? AUTOSIM_DIALOG_ARM : AUTOSIM_DIALOG_CONTINUE);
    return TRUE;
}

} // namespace

void autosim_fill_dialog_state(AutosimDialogState* state, AutosimDialogMode mode) {
    if (!state) return;
    state->mode = mode;
    state->human_mask = (unsigned)FactionStatus[0];
    state->viewed_faction = MapWin ? MapWin->cOwner : 0;
    state->telemetry_enabled = conf.telemetry_enabled != 0;
    state->resumed_campaign = telemetry_resumed_linked_campaign();
    state->suppress_notices = conf.autosim_suppress_notices != 0;
    state->fast_combat = conf.autosim_fast_combat != 0;
    state->campaign_id = telemetry_get_campaign_id();
    state->session_id = telemetry_get_session_id();
    state->campaign_name = telemetry_get_campaign_name();
    state->save_name = autosim_default_save_name(
        state->campaign_name.c_str(), *CurrentMissionYear);
    state->additional_turns = conf.autosim_manual_turns > 0
        ? conf.autosim_manual_turns : 25;
    state->start_year = state->end_year = *CurrentMissionYear;
    state->start_bases = state->end_bases = *BaseCount;
    state->start_units = state->end_units = *VehCount;
    for (int i = 1; i < MaxPlayerNum; ++i) {
        if (is_alive(i)) ++state->alive_factions;
    }
}

bool autosim_apply_dialog_options(const AutosimDialogState& state) {
    if (!telemetry_set_campaign_name(state.campaign_name.c_str()))
        return false;
    // Runtime-only knobs. No thinker.ini or save-game preference changes.
    conf.autosim_suppress_notices = state.suppress_notices ? 1 : 0;
    conf.autosim_fast_combat = state.fast_combat ? 1 : 0;
    conf.autosim_manual_turns = state.additional_turns;
    char action[120] = {};
    snprintf(action, sizeof(action), "turns=%d notices=%d fast=%d",
        conf.autosim_manual_turns, conf.autosim_suppress_notices ? 1 : 0,
        conf.autosim_fast_combat ? 1 : 0);
    telemetry_record_ui_notice("autosim", "SETTINGS", action);
    return true;
}

void autosim_show_manual_setup() {
    if (*GameHalted || *MultiplayerActive || *PbemActive || !MapWin)
        return;
    AutosimDialogState state;
    autosim_fill_dialog_state(&state, AUTOSIM_DIALOG_SETUP);
    if (autosim_show_control_dialog(&state) != AUTOSIM_DIALOG_ARM)
        return;
    if (!autosim_apply_dialog_options(state)) {
        MessageBoxA(*phWnd, "Could not update the campaign name. Check campaigns/ permissions.",
            "thinkerNeca Simulation Setup", MB_OK | MB_ICONWARNING);
        return;
    }
    const bool all_ai = ((unsigned)FactionStatus[0] & 0xFEu) == 0;
    MessageBoxA(*phWnd,
        all_ai ? "Simulation armed. The game will ask for startup confirmation at the next safe turn dispatch."
               : "Simulation armed. In Scenario Editor, transfer all factions to AI, then End Turn. The startup confirmation appears before autonomous AI processing.",
        "thinkerNeca Simulation Setup", MB_OK | MB_ICONINFORMATION);
}

std::string autosim_default_save_name(const char* campaign, int year) {
    char suffix[24] = {};
    snprintf(suffix, sizeof(suffix), "_MY%d", year);
    return save_slug(campaign) + suffix;
}

AutosimDialogChoice autosim_show_control_dialog(AutosimDialogState* state) {
    if (!state || !*phWnd) return AUTOSIM_DIALOG_FAILED;
    // DLGTEMPLATE fixed header followed by menu, class, title zero WORDs.
    // Standard Win32 structure packing gives an 18-byte DLGTEMPLATE.
    static_assert(sizeof(DLGTEMPLATE) == 18, "Unexpected dialog template packing");
    alignas(DWORD) unsigned char bytes[sizeof(DLGTEMPLATE) + 6] = {};
    DLGTEMPLATE* tpl = reinterpret_cast<DLGTEMPLATE*>(bytes);
    tpl->style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_CENTER;
    tpl->dwExtendedStyle = 0;
    tpl->cdit = 0;
    tpl->x = 0; tpl->y = 0;
    // Fit an 800x600 game viewport at standard Windows dialog font metrics.
    tpl->cx = 420; tpl->cy = 310;
    // The trailing six bytes are zero (no menu, default dialog class,
    // zero-length title); set caption after creation in WM_INITDIALOG.
    DialogContext ctx = {};
    ctx.state = state;
    const INT_PTR result = DialogBoxIndirectParamA(GetModuleHandleA(NULL), tpl,
        *phWnd, autosim_dialog_proc, (LPARAM)&ctx);
    if (result == -1) return AUTOSIM_DIALOG_FAILED;
    return static_cast<AutosimDialogChoice>(result);
}

bool autosim_save_named(const std::string& name, std::string* output_path,
    std::string* error) {
    std::string stem = trim_text(name);
    if (stem.size() > 4 && !_stricmp(stem.c_str() + stem.size() - 4, ".sav"))
        stem.resize(stem.size() - 4);
    if (!is_safe_save_name(stem)) {
        if (error) *error = "Invalid game save filename.";
        return false;
    }
    const DWORD dir = GetFileAttributesA("saves");
    if (dir == INVALID_FILE_ATTRIBUTES) {
        if (!CreateDirectoryA("saves", NULL)
        && GetLastError() != ERROR_ALREADY_EXISTS) {
            if (error) *error = "Could not create saves/ directory.";
            return false;
        }
    } else if (!(dir & FILE_ATTRIBUTE_DIRECTORY)) {
        if (error) *error = "saves exists but is not a directory.";
        return false;
    }
    const std::string target = "saves/" + stem + ".sav";
    const DWORD previous = GetFileAttributesA(target.c_str());
    if (previous != INVALID_FILE_ATTRIBUTES) {
        if (previous & FILE_ATTRIBUTE_DIRECTORY) {
            if (error) *error = "Save path is a directory.";
            return false;
        }
        if (MessageBoxA(*phWnd, ("Replace existing save?\n\n" + target).c_str(),
            "thinkerNeca Simulation Control", MB_YESNO | MB_DEFBUTTON2
                | MB_ICONQUESTION) != IDYES) {
            if (error) *error = "Save overwrite cancelled.";
            return false;
        }
    }
    char temp_name[120] = {};
    snprintf(temp_name, sizeof(temp_name), "saves/.autosim-%lu-%lu.tmp.sav",
        GetCurrentProcessId(), GetTickCount());
    // mod_save_daemon is the same version-12 patched serializer invoked by
    // native save_game(). It returns 0 on success, unlike save_game().
    if (mod_save_daemon(temp_name) != 0) {
        DeleteFileA(temp_name);
        if (error) *error = "Game serializer failed; previous save is untouched.";
        return false;
    }
    WIN32_FILE_ATTRIBUTE_DATA info = {};
    if (!GetFileAttributesExA(temp_name, GetFileExInfoStandard, &info)
    || (info.nFileSizeHigh == 0 && info.nFileSizeLow < 4096)) {
        DeleteFileA(temp_name);
        if (error) *error = "Serializer produced a missing or implausibly small save.";
        return false;
    }
    if (!MoveFileExA(temp_name, target.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileA(temp_name);
        if (error) *error = "Could not commit saved game under requested filename.";
        return false;
    }
    if (output_path) *output_path = target;
    if (error) error->clear();
    return true;
}
