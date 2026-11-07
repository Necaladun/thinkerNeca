#pragma once

#include <algorithm>
#include <cassert>

/*
Preserve vanilla's uniform 0..floor(attack/defense) bombardment damage
when the ratio is at least 1. Below 1, permit exactly one point with
probability attack/(attack+defense), matching vanilla's 50% at 1:1.
The caller supplies the game's random roll so this is independently testable.
*/
struct ArtilleryDamageRoll {
    int range;
    int weak_attack; // -1 for vanilla uniform damage, >= 0 for fractional 0/1

    int damage(int roll) const {
        assert(roll >= 0 && roll < range);
        return weak_attack >= 0 ? (roll < weak_attack ? 1 : 0) : roll;
    }
};

inline ArtilleryDamageRoll artillery_damage_roll(int attack, int defense) {
    assert(attack >= 0 && defense > 0);
    if (attack == 0) {
        // Keep the old no-damage/no-RNG outcome for zero attack strength.
        return {1, 0};
    }
    if (attack < defense) {
        return {attack + defense, attack};
    }
    return {std::min(999, std::max(1, attack / defense + 1)), -1};
}

/*
When ignoring reactor power, apply the artillery cap to effective HP
before converting it to the defender's stored HP. Otherwise a 99% cap
leaves 1 stored HP for every reactor (and therefore less effective HP
for Fusion/Quantum/Singularity than for Fission).
The caller passes scale=1 when reactor power is not ignored.
*/
inline int artillery_damage_floor(int stored_max_hp, int reactor_scale, int max_damage_percent) {
    assert(reactor_scale >= 1 && stored_max_hp >= reactor_scale);
    assert(max_damage_percent >= 0 && max_damage_percent <= 100);
    const int effective_max_hp = stored_max_hp / reactor_scale;
    return ((effective_max_hp * (100 - max_damage_percent) + 99) / 100) * reactor_scale;
}
