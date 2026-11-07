#include "../src/artillery_damage.h"

#include <cassert>
#include <iostream>

// Compile independently of the Windows game:
// g++ -std=c++11 -Wall -Wextra -pedantic tools/test_artillery_damage.cpp -o /tmp/test_artillery && /tmp/test_artillery

int main() {
    // Every existing >= 1:1 ratio keeps vanilla's uniform 0..floor(A/D) damage.
    for (int attack = 1; attack <= 100; ++attack) {
        for (int defense = 1; defense <= attack; ++defense) {
            const auto r = artillery_damage_roll(attack, defense);
            assert(r.weak_attack == -1);
            assert(r.range == attack / defense + 1);
            for (int roll = 0; roll < r.range; ++roll) {
                assert(r.damage(roll) == roll);
            }
        }
    }

    // WTP's sub-1:1 rule: exactly A rolls inflict 1 HP out of A+D outcomes.
    for (int attack = 0; attack <= 30; ++attack) {
        for (int defense = attack + 1; defense <= 100; ++defense) {
            const auto r = artillery_damage_roll(attack, defense);
            if (attack == 0) {
                assert(r.range == 1 && r.damage(0) == 0);
                continue;
            }
            assert(r.weak_attack == attack);
            assert(r.range == attack + defense);
            int hits = 0;
            for (int roll = 0; roll < r.range; ++roll) {
                assert(r.damage(roll) == 0 || r.damage(roll) == 1);
                hits += r.damage(roll);
            }
            assert(hits == attack);
        }
    }

    // The distributed game profile has a 3/2 artillery damage multiplier.
    // Laser (2) vs Plasma (3) is already at 1:1; Gun (1) vs Synthmetal (2) is not.
    const auto laser_vs_plasma = artillery_damage_roll(2 * 3, 3 * 2);
    assert(laser_vs_plasma.range == 2 && laser_vs_plasma.weak_attack == -1);
    const auto gun_vs_synthmetal = artillery_damage_roll(1 * 3, 2 * 2);
    assert(gun_vs_synthmetal.range == 7 && gun_vs_synthmetal.weak_attack == 3);
    const auto gun_vs_stasis = artillery_damage_roll(1 * 3, 12 * 2);
    assert(gun_vs_stasis.range == 27 && gun_vs_stasis.weak_attack == 3); // 1/9
    const auto laser_vs_stasis = artillery_damage_roll(2 * 3, 12 * 2);
    assert(laser_vs_stasis.range == 30 && laser_vs_stasis.weak_attack == 6); // 1/5

    // Reactor-normalised damage caps in stored-HP units.
    for (int reactor = 1; reactor <= 4; ++reactor) {
        int hp = 10 * reactor;
        assert(artillery_damage_floor(hp, reactor, 50) == 5 * reactor);
        assert(artillery_damage_floor(hp, reactor, 99) == reactor);
        assert(artillery_damage_floor(hp, reactor, 100) == 0);
        // Without ignored power, keep the original stored-HP percentage/ceil rule.
        assert(artillery_damage_floor(hp, 1, 50) == 5 * reactor);
        assert(artillery_damage_floor(hp, 1, 99) == 1);
        assert(artillery_damage_floor(hp, 1, 100) == 0);

        // Identical 2-effective-HP hits, independent of defender reactor.
        int damage = 2 * reactor;
        assert(damage * 10 / hp == 2);
        int open_floor = artillery_damage_floor(hp, reactor, 99);
        int damage_at_cap = std::min(damage, hp - open_floor);
        assert(damage_at_cap == damage);
        // An already-bombarded unit cannot cross the remaining-HP cap.
        int remaining = 2 * reactor;
        assert(std::max(0, std::min(damage, remaining - open_floor)) == reactor);
    }

    // Rare 1-HP artifacts are not reactor-scaled.
    assert(artillery_damage_floor(1, 1, 99) == 1);
    assert(artillery_damage_floor(1, 1, 100) == 0);

    std::cout << "artillery damage tests passed\n";
}
