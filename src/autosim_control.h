#pragma once

#include <string>

// AUTO-002 operator controls. This is a modal Win32 dialog opened ONLY at a
// completed single-player all-AI faction boundary. It never runs inside a
// native combat, diplomacy, upkeep or save callback.
enum AutosimDialogChoice {
    AUTOSIM_DIALOG_FAILED = -1,
    AUTOSIM_DIALOG_RESUME = 1,
    AUTOSIM_DIALOG_SAVE_CONTINUE = 2,
    AUTOSIM_DIALOG_SAVE_EXIT = 3,
    AUTOSIM_DIALOG_TAKE_CONTROL = 4,
};

struct AutosimDialogState {
    int completed_segment = 0;
    int completed_total = 0;
    int start_year = 0;
    int end_year = 0;
    int start_bases = 0;
    int end_bases = 0;
    int start_units = 0;
    int end_units = 0;
    int alive_factions = 0;
    int additional_turns = 25;
    bool allow_resume = false;
    std::string campaign_name;
    std::string save_name;  // Basename without directory or .sav extension.
};

AutosimDialogChoice autosim_show_control_dialog(AutosimDialogState* state);

// Directly uses the same mod_save_daemon used by the patched native Save Game
// dialog, but commits through a temporary file so failed writes cannot
// truncate a prior save. Returns false on cancellation or any write error.
bool autosim_save_named(const std::string& name,
    std::string* output_path, std::string* error);

std::string autosim_default_save_name(const char* campaign, int year);
