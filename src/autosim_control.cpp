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
        SetWindowTextA(dialog, "thinkerNeca - Simulation Control");
        const AutosimDialogState& s = *ctx->state;
        RECT bounds = {};
        GetClientRect(dialog, &bounds);
        const int width = bounds.right;
        const int height = bounds.bottom;
        const int left = 20;
        const int inner = width - 40;

        add_child(dialog, "STATIC", "AI simulation completed at a full turn boundary",
            SS_LEFT, 0, 0, left, 13, inner, 22);
        char summary[384] = {};
        snprintf(summary, sizeof(summary),
            "Turns: %d this segment / %d total.  Mission years: %d -> %d\r\n"
            "Colonies: %d -> %d    Units: %d -> %d    Living factions: %d",
            s.completed_segment, s.completed_total, s.start_year, s.end_year,
            s.start_bases, s.end_bases, s.start_units, s.end_units, s.alive_factions);
        add_child(dialog, "STATIC", summary, SS_LEFT, 0, 0, left, 43, inner, 45);

        add_child(dialog, "STATIC", "Campaign name (used in telemetry reports)",
            SS_LEFT, 0, 0, left, 99, inner, 20);
        add_child(dialog, "EDIT", s.campaign_name.c_str(),
            WS_TABSTOP | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ID_CAMPAIGN_NAME,
            left, 119, inner, 25);
        SendDlgItemMessageA(dialog, ID_CAMPAIGN_NAME, EM_LIMITTEXT, 88, 0);

        add_child(dialog, "STATIC", "Save filename (within saves/, .sav added automatically)",
            SS_LEFT, 0, 0, left, 157, inner, 20);
        add_child(dialog, "EDIT", s.save_name.c_str(),
            WS_TABSTOP | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ID_SAVE_NAME,
            left, 177, inner, 25);
        SendDlgItemMessageA(dialog, ID_SAVE_NAME, EM_LIMITTEXT, 72, 0);

        add_child(dialog, "STATIC", "Additional completed AI turns",
            SS_LEFT, 0, 0, left, 217, inner, 20);
        char count[24] = {};
        snprintf(count, sizeof(count), "%d", s.additional_turns);
        add_child(dialog, "EDIT", count,
            WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER, WS_EX_CLIENTEDGE,
            ID_MORE_TURNS, left, 237, 130, 26);
        SendDlgItemMessageA(dialog, ID_MORE_TURNS, EM_LIMITTEXT, 6, 0);

        add_child(dialog, "STATIC",
            "Saving uses the game serializer. Cancel/failure leaves this window open.",
            SS_LEFT, 0, 0, left, 278, inner, 30);

        const int buttons = s.allow_resume ? 4 : 3;
        const int gap = 8;
        const int button_width = (inner - gap * (buttons - 1)) / buttons;
        const int y = height - 60;
        const int ids[4] = {ID_SAVE_CONTINUE, ID_SAVE_EXIT, ID_TAKE_CONTROL, ID_RESUME};
        const char* captions[4] = {
            "Save && Continue", "Save && Exit", "Take Control", "Resume"
        };
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
        if (ctx && ctx->state->allow_resume) {
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
    && id != ID_TAKE_CONTROL && id != ID_RESUME) return FALSE;
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
    if (id != ID_TAKE_CONTROL && !is_safe_save_name(save)) {
        MessageBoxA(dialog,
            "Save name must be 1-72 characters using letters, digits,"
            " spaces, hyphens, underscores or dots. No paths/device names.",
            "thinkerNeca Simulation Control", MB_OK | MB_ICONWARNING);
        return TRUE;
    }
    int count = ctx->state->additional_turns;
    if (id == ID_SAVE_CONTINUE) {
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
    EndDialog(dialog, id == ID_SAVE_CONTINUE ? AUTOSIM_DIALOG_SAVE_CONTINUE
        : id == ID_SAVE_EXIT ? AUTOSIM_DIALOG_SAVE_EXIT : AUTOSIM_DIALOG_TAKE_CONTROL);
    return TRUE;
}

} // namespace

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
    tpl->cx = 460; tpl->cy = 345;
    // The trailing six bytes are zero (no menu, default dialog class,
    // zero-length title); set caption after creation in WM_INITDIALOG.
    DialogContext ctx = {state, false, false, false};
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
        (unsigned long)GetCurrentProcessId(), (unsigned long)GetTickCount());
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
