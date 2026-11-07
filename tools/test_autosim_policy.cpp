#include "../src/autosim.h"

#include <cassert>
#include <cstdio>

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

int main() {
    test_entry_guards();
    test_full_turn_limit_and_reset();
    test_resume_mid_cycle_and_reenable_human();
    std::puts("AUTO-001 host policy tests passed");
    return 0;
}
