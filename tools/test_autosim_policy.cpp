#include "../src/autosim.h"

#include <cassert>
#include <cstdio>
#include <initializer_list>

static void test_entry_guards() {
    assert(!auto_turn_eligible(0, 0, false, false, false, 3, true));
    assert(!auto_turn_eligible(5, 1u << 3, false, false, false, 3, true));
    assert(!auto_turn_eligible(5, 1u << 1, false, false, false, 3, true));
    assert(!auto_turn_eligible(5, 0, true, false, false, 3, true));
    assert(!auto_turn_eligible(5, 0, false, true, false, 3, true));
    assert(!auto_turn_eligible(5, 0, false, false, true, 3, true));
    assert(!auto_turn_eligible(5, 0, false, false, false, 0, true));
    assert(!auto_turn_eligible(5, 0, false, false, false, 8, true));
    assert(!auto_turn_eligible(5, 0, false, false, false, 3, false));
    // Bit zero does not represent any of the seven faction slots.
    assert(auto_turn_eligible(5, 1u, false, false, false, 3, true));
    assert(auto_turn_eligible(5, 0, false, false, false, 3, true));
}

static void test_full_turn_limit_and_reset() {
    AutoTurnDriver driver;
    assert(driver.on_cycle(3, true, true) == AUTO_CYCLE_ADVANCE);
    assert(driver.completed_turns == 1);
    assert(driver.on_cycle(3, true, true) == AUTO_CYCLE_ADVANCE);
    assert(driver.completed_turns == 2);
    assert(driver.on_cycle(3, true, true) == AUTO_CYCLE_STOP);
    assert(driver.completed_turns == 3);
    assert(driver.on_cycle(3, true, true) == AUTO_CYCLE_STOP);
    assert(driver.completed_turns == 3);
    assert(driver.on_cycle(3, false, true) == AUTO_CYCLE_NORMAL);
    assert(driver.completed_turns == 0);
    assert(driver.on_cycle(3, true, true) == AUTO_CYCLE_ADVANCE);
    assert(driver.completed_turns == 1);
}

static void test_resume_mid_cycle_and_reenable_human() {
    AutoTurnDriver driver;
    assert(driver.on_cycle(2, true, false) == AUTO_CYCLE_ADVANCE);
    assert(driver.completed_turns == 0);
    assert(driver.on_cycle(2, true, true) == AUTO_CYCLE_ADVANCE);
    assert(driver.completed_turns == 1);
    assert(driver.on_cycle(2, false, true) == AUTO_CYCLE_NORMAL);
    assert(driver.completed_turns == 0);
    assert(driver.on_cycle(2, true, true) == AUTO_CYCLE_ADVANCE);
    assert(driver.on_cycle(2, true, true) == AUTO_CYCLE_STOP);
    assert(driver.on_cycle(0, true, true) == AUTO_CYCLE_NORMAL);
}


static void test_notice_suppression_guards() {
    assert(!auto_ui_eligible(false, 10, 0, false, false, false, 3, true));
    assert(!auto_ui_eligible(true, 0, 0, false, false, false, 3, true));
    assert(!auto_ui_eligible(true, 10, 1u << 3, false, false, false, 3, true));
    assert(!auto_ui_eligible(true, 10, 0, true, false, false, 3, true));
    assert(!auto_ui_eligible(true, 10, 0, false, true, false, 3, true));
    assert(!auto_ui_eligible(true, 10, 0, false, false, true, 3, true));
    assert(!auto_ui_eligible(true, 10, 0, false, false, false, 0, false));
    assert(auto_ui_eligible(true, 10, 0, false, false, false, 3, true));
}

static void test_popup_allowlist_fails_closed() {
    for (const char* label : {"REACTORUPGRADE", "THESECRET0", "THESECRET1", "TREETIME"}) {
        assert(autosim_xpop_policy(label, false) == AUTO_UI_NOTICE);
        assert(autosim_xpop_policy(label, true) == AUTO_UI_NATIVE);
    }
    for (const char* label : {"SOCIETY", "ASKSEEDESIGN"}) {
        assert(autosim_xpop_policy(label, false) == AUTO_UI_DECLINE_OPTIONAL);
        assert(autosim_xpop_policy(label, true) == AUTO_UI_NATIVE);
    }
    for (const char* label : {"MIMIMI", "ESCAPE", "DIPLO", "HASTY",
                              "UNKNOWN", "ASKSEEDESIGN2"}) {
        assert(autosim_xpop_policy(label, false) == AUTO_UI_NATIVE);
    }
    assert(autosim_xpop_policy(nullptr, false) == AUTO_UI_NATIVE);
}

static void test_monument_new_event_only() {
    assert(autosim_suppress_monument(true, 1));
    assert(autosim_suppress_monument(true, 2));
    assert(!autosim_suppress_monument(true, 0)); // explicit history view
    assert(!autosim_suppress_monument(false, 0));
    assert(!autosim_suppress_monument(false, 1)); // normal human game
}

static void test_ai_observer_hasty_attack_policy() {
    // Observed AI attack in a running automated game uses the AI move rule.
    assert(autosim_use_ai_hasty_rule(true, false, true, true, false));
    // Other sources of attacks, human-controlled play and non-autosim
    // games keep their previous native code paths.
    assert(!autosim_use_ai_hasty_rule(false, false, true, true, false));
    assert(!autosim_use_ai_hasty_rule(true, true, true, true, false));
    assert(!autosim_use_ai_hasty_rule(true, false, false, true, false));
    assert(!autosim_use_ai_hasty_rule(true, false, true, false, false));
    assert(!autosim_use_ai_hasty_rule(true, false, true, true, true));
}

int main() {
    test_entry_guards();
    test_full_turn_limit_and_reset();
    test_resume_mid_cycle_and_reenable_human();
    test_notice_suppression_guards();
    test_popup_allowlist_fails_closed();
    test_monument_new_event_only();
    test_ai_observer_hasty_attack_policy();
    std::puts("AUTO-001 host policy tests passed");
    return 0;
}
