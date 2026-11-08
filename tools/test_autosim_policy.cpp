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
    for (const char* label : {"REACTORUPGRADE", "THESECRET0", "THESECRET1", "TREETIME",
                              "WEKILLEDIT", "THEYKILLEDIT", "WEMUFFEDIT", "THEYMUFFEDIT",
                              "WEMUSTSAC", "THEYMUSTSAC", "WESHOTIT", "THEYSHOTIT",
                              "WESHOTIT2", "THEYSHOTIT2"}) {
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

static void test_transient_interlude_preferences() {
    const int flag = 0x40000;
    int prefs = 0xA125;
    {
        AutoTemporaryPrefFlag scoped(&prefs, flag);
        assert((prefs & flag) != 0);
        assert((prefs & ~flag) == 0xA125);
    }
    assert(prefs == 0xA125);
    prefs = flag | 0x82;
    {
        AutoTemporaryPrefFlag scoped(&prefs, flag);
        assert(prefs == (flag | 0x82));
    }
    assert(prefs == (flag | 0x82));
}

static void test_scoped_combat_visual_preferences() {
    const int fast = 0x8;
    const int nonquick = 0x4 | 0x8000;
    const int quick_all = 0x80;
    const int pause = 0x10;
    int prefs = nonquick | 0x400;
    int more = pause | 0x200;
    {
        AutoScopedCombatPrefs active(true, &prefs, &more,
            fast, nonquick, quick_all, pause);
        assert((prefs & fast) != 0);
        assert((prefs & nonquick) == 0);
        assert((more & quick_all) != 0);
        assert((more & pause) == 0);
    }
    assert(prefs == (nonquick | 0x400));
    assert(more == (pause | 0x200));

    {
        AutoScopedCombatPrefs inactive(false, &prefs, &more,
            fast, nonquick, quick_all, pause);
        assert(prefs == (nonquick | 0x400));
        assert(more == (pause | 0x200));
    }
    assert(prefs == (nonquick | 0x400));
    assert(more == (pause | 0x200));
}

static void test_full_turn_fast_visual_preferences() {
    // Match src/engine_enums.h: source policy uses these named masks.
    // This Win32-free host test checks nested/disabled preference restoration.
    constexpr int fast = 0x8;
    constexpr int no_center = 0x80000;
    constexpr int dont_quick_enemy = 0x4;
    constexpr int dont_quick_ally = 0x8000;
    constexpr int map_animations = 0x80;
    constexpr int edge_scroll = 0x1000;
    constexpr int quick_orders = 0x40;
    constexpr int quick_all = 0x80;
    constexpr int hide_path = 0x2000000;
    constexpr int pause_battles = 0x10;
    constexpr int blink = 0x200;
    constexpr int base_names = 0x2000;
    constexpr int base_production = 0x1000;
    constexpr int other_basic = 0x400 | 0x200;
    constexpr int other_more = 0x100 | 0x400;

    constexpr int basic_on = fast | no_center;
    constexpr int basic_off = dont_quick_enemy | dont_quick_ally
        | map_animations | edge_scroll;
    constexpr int more_on = quick_all | quick_orders | hide_path;
    constexpr int more_off = pause_battles | blink | base_names | base_production;

    const int original_basic = basic_off | other_basic;
    const int original_more = more_off | other_more;
    int basic = original_basic;
    int more = original_more;

    {
        AutoScopedCombatPrefs full_turn(true, &basic, &more,
            basic_on, basic_off, more_on, more_off);
        assert((basic & basic_on) == basic_on);
        assert((basic & basic_off) == 0);
        assert((more & more_on) == more_on);
        assert((more & more_off) == 0);
        assert((basic & other_basic) == other_basic);
        assert((more & other_more) == other_more);
        const int active_basic = basic;
        const int active_more = more;
        {
            AutoScopedCombatPrefs nested_battle(true, &basic, &more,
                basic_on, basic_off, more_on, more_off);
            assert(basic == active_basic);
            assert(more == active_more);
        }
        assert(basic == active_basic);
        assert(more == active_more);
    }
    // Native autosave / manual Save Game run outside the temporary scope.
    assert(basic == original_basic);
    assert(more == original_more);

    {
        AutoScopedCombatPrefs disabled(false, &basic, &more,
            basic_on, basic_off, more_on, more_off);
        assert(basic == original_basic);
        assert(more == original_more);
    }
    assert(basic == original_basic);
    assert(more == original_more);
}

int main() {
    test_entry_guards();
    test_full_turn_limit_and_reset();
    test_resume_mid_cycle_and_reenable_human();
    test_notice_suppression_guards();
    test_popup_allowlist_fails_closed();
    test_monument_new_event_only();
    test_ai_observer_hasty_attack_policy();
    test_transient_interlude_preferences();
    test_scoped_combat_visual_preferences();
    test_full_turn_fast_visual_preferences();
    std::puts("AUTO-001 host policy tests passed");
    return 0;
}
