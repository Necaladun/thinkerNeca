#pragma once

// AUTO-001: pure turn-boundary policy; never mutates game data or calls RNG.
// The engine supplies current human-control bits and single-player state.
enum AutoCycleAction {
    AUTO_CYCLE_NORMAL,
    AUTO_CYCLE_ADVANCE,
    AUTO_CYCLE_STOP
};

inline bool auto_turn_eligible(int requested_turns, unsigned human_faction_mask,
    bool multiplayer, bool pbem, bool game_done, int view_faction, bool view_alive) {
    // bit 0 is not a playable faction; 1..7 must all be computer-controlled.
    return requested_turns > 0 && !(human_faction_mask & 0xFEu)
        && !multiplayer && !pbem && !game_done
        && view_faction >= 1 && view_faction <= 7 && view_alive;
}

struct AutoTurnDriver {
    int completed_turns = 0;

    // A loaded game may begin partway through a faction cycle. It may advance
    // without human input, but that initial partial cycle does not count
    // toward the requested number of fully simulated turns.
    AutoCycleAction on_cycle(int requested_turns, bool eligible, bool full_cycle) {
        if (!eligible || requested_turns <= 0) {
            completed_turns = 0;
            return AUTO_CYCLE_NORMAL;
        }
        if (full_cycle && completed_turns < requested_turns) {
            completed_turns++;
        }
        return completed_turns >= requested_turns ? AUTO_CYCLE_STOP : AUTO_CYCLE_ADVANCE;
    }
};


// AUTO-002: deliberate allowlist, not a global popup auto-clicker.
// The only handled choices decline *optional human-only UI*, so AI decisions
// continue through existing code. Every unknown label remains native/modal.
#include <cstring>

inline bool auto_ui_eligible(bool configured, int requested_turns,
    unsigned human_faction_mask, bool multiplayer, bool pbem, bool game_done,
    int view_faction, bool view_alive) {
    return configured && auto_turn_eligible(requested_turns, human_faction_mask,
        multiplayer, pbem, game_done, view_faction, view_alive);
}

// Source-owned monuments record history before presenting a new-event screen.
// Keep explicit monument browsing (is_new_event == 0) intact.
inline bool autosim_suppress_monument(bool ui_active, int is_new_event) {
    return ui_active && is_new_event != 0;
}

// AI-controlled observer units must use the existing AI hasty-move
// threshold, not the human-owned confirmation path. This does not authorize
// an attack: order_veh still decides whether to proceed.
inline bool autosim_use_ai_hasty_rule(bool ui_active, bool human_controlled,
    bool observed_faction, bool direct_attack, bool moving_with_delay) {
    return ui_active && !human_controlled && observed_faction
        && direct_attack && !moving_with_delay;
}

// Restore the user's in-memory setting even if native UI code unwinds.
// Does not persist to INI or SAV; designed for one synchronous native call.
struct AutoTemporaryPrefFlag {
    int* const field;
    const int before;
    AutoTemporaryPrefFlag(int* value, int flag) : field(value), before(*value) {
        *field |= flag;
    }
    ~AutoTemporaryPrefFlag() { *field = before; }
    AutoTemporaryPrefFlag(const AutoTemporaryPrefFlag&) = delete;
    AutoTemporaryPrefFlag& operator=(const AutoTemporaryPrefFlag&) = delete;
};

// Temporary native combat presentation profile. All original preference
// bits are restored at scope exit, including early combat returns. This is
// not a permanent configuration change, save format change, or battle rule.
struct AutoScopedCombatPrefs {
    int* const basic;
    int* const more;
    const int saved_basic;
    const int saved_more;
    const bool enabled;
    AutoScopedCombatPrefs(bool active, int* basic_prefs, int* more_prefs,
        int set_basic, int clear_basic, int set_more, int clear_more)
        : basic(basic_prefs), more(more_prefs), saved_basic(*basic_prefs),
          saved_more(*more_prefs), enabled(active) {
        if (enabled) {
            *basic = (*basic | set_basic) & ~clear_basic;
            *more = (*more | set_more) & ~clear_more;
        }
    }
    ~AutoScopedCombatPrefs() {
        if (enabled) {
            *basic = saved_basic;
            *more = saved_more;
        }
    }
    AutoScopedCombatPrefs(const AutoScopedCombatPrefs&) = delete;
    AutoScopedCombatPrefs& operator=(const AutoScopedCombatPrefs&) = delete;
};

enum AutoUiAction {
    AUTO_UI_NATIVE,
    AUTO_UI_NOTICE,
    AUTO_UI_DECLINE_OPTIONAL
};

inline AutoUiAction autosim_xpop_policy(const char* label, bool has_callback) {
    if (!label || has_callback) {
        return AUTO_UI_NATIVE;
    }
    if (std::strcmp(label, "GOTUNITDEFEND") == 0
        || std::strcmp(label, "GOTUNITOFFEND") == 0
        || std::strcmp(label, "REACTORUPGRADE") == 0
        || std::strcmp(label, "THESECRET0") == 0
        || std::strcmp(label, "THESECRET1") == 0
        || std::strcmp(label, "TREETIME") == 0) {
        return AUTO_UI_NOTICE;
    }
    if (std::strcmp(label, "SOCIETY") == 0
        || std::strcmp(label, "ASKSEEDESIGN") == 0) {
        return AUTO_UI_DECLINE_OPTIONAL;
    }
    return AUTO_UI_NATIVE;
}
