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
