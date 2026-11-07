# Unit combat, artillery and prototype economics

**Related review:** [aircraft, interception, SAM/AAA and transport design](aircraft-combat.md).

**Scope:** existing mechanics in the current thinkerNeca source, plus explicitly identified defects. This is **not** a specification of approved balance changes. See [BUG-004 and BAL-004](../WORK-QUEUE.md) and [exploratory military ideas](../IDEAS.md).

**Source authority:** [`src/veh.cpp`](../../src/veh.cpp) (cost and artillery eligibility), [`src/veh_combat.cpp`](../../src/veh_combat.cpp) (combat types, strengths and damage), [`src/engine_veh.h`](../../src/engine_veh.h) (unit HP), [`docs/alphax.txt`](../../docs/alphax.txt) (chassis, weapons, armour, rules), [`docs/thinker.ini`](../../docs/thinker.ini) (distributed test settings). The installed `thinker_user.ini` can override the distributed test profile. `master` includes PR #4 and PR #5; draft PR #10 changes ordinary bombardment only. Actual installed game settings can differ from the distributed profile.

## Baseline: chassis and direct combat

| Chassis | Speed in current `alphax.txt` | Cost factor | Existing distinction |
| --- | ---: | ---: | --- |
| Infantry | 1 | 1 | Inexpensive land chassis |
| Speeder/Rover | 2 | 2 | Mobile land chassis |
| Hovertank | 3 | 3 | Faster, all-terrain land chassis |
| Foil | 4 | 4 | Light naval chassis; cargo 2 |
| Cruiser | 6 | 6 | Faster naval chassis; cargo 4 |

There is **no intrinsic Tank or Cruiser maximum-HP bonus**. Roles involving conventional Tanks, slower heavy units, faster late Rovers, heavier Cruisers or custom chassis HP are ideas only (BAL-004).

In normal **conventional direct combat**, the attacker primarily uses **weapon** strength and the defender primarily uses **armour** strength, subject to morale, terrain, abilities, facilities and other situational modifiers. A 6-1 attacker therefore fights well on offence but remains weak when counterattacked. Offensive weapon strength does not automatically protect the defending unit.

Current `alphax.txt` includes +25% mobile attack in eligible open ground and +25% Infantry attack against bases. The mobile modifier is based on speed and terrain, **not** Rover identity: Hovertanks can qualify. An exclusive future Rover bonus would need a deliberate rule change. Land defence can exploit forest/rough terrain, sensors, bunkers, base defences and ECM against mobile land attackers, when applicable.

Open-sea units lack forest/rocky and anti-mobile-land ECM bonuses. They can still receive appropriate ocean sensor coverage (test profile: `sensor_defense_ocean=1`), AAA against aircraft, and defensive facilities when in a base. Crucially, **weapon-versus-weapon artillery duels do not use the ordinary armour/terrain defence branch**, so purchasing naval armour cannot be assumed to improve every naval engagement.

## Prototype costs and the offensive-unit incentive

`mod_proto_cost()` combines chassis, weapon, armour, reactor and special-ability cost factors. Its integer arithmetic creates ten-mineral price steps and rounding discontinuities. When **weapon and armour both exceed 1**, the formula adds a surcharge; for mobile land units it adds a further surcharge. Sea chassis instead halve the armour cost factor before calculating the main cost component. Higher reactors retain cost discounts even with `ignore_reactor_power=1`.

Illustrative Fission designs without special abilities or prototype surcharge, using current factors:

| Design | Infantry | Rover | Hovertank |
| --- | ---: | ---: | ---: |
| 4-1 Impact | 20 | 30 | 40 |
| 4-2 Impact/Synthmetal | 30 | 60 | 70 |
| 4-3 Impact/Plasma | 40 | 70 | 80 |

Thus **two 4-1 Rovers cost the same as one 4-2 Rover**, producing an incentive for inexpensive, weapon-heavy mobile attackers (often called *Impact Rover spam*). This is an economic tendency, not a universal claim that armoured units are useless: survival, repair, morale retention, defensive terrain and counterattacks matter.

The same distinction applies to ships, although naval armour is already cheaper through the cost formula. A costly armoured Cruiser may be very effective against **direct attacks** but get no benefit from that armour in a **weapon-versus-weapon duel**.

## Artillery: three distinct combat paths

`can_arty()` in `src/veh.cpp` recognises eligible positive-weapon conventional ships as artillery-capable even without the Heavy Artillery special ability. For land units, the ability is normally required; Spore Launchers have a special exception. The distributed `long_range_artillery=2` grants **additional range** to eligible Heavy-Artillery-equipped ships, not universal artillery capability.

| Combat path | Strengths compared | Damage mechanism | Current reactor normalisation |
| --- | --- | --- | --- |
| Conventional direct attack | Attacker weapon vs defender armour | Repeated combat rounds | Yes when configured |
| Weapon-versus-weapon artillery duel | Attacker weapon vs defender weapon | Repeated combat rounds, potentially lethal | Yes when configured |
| Ordinary ranged bombardment | Artillery offence vs target's relevant defence | Random damage and location-specific cap | **Source fix in draft PR #10; Windows validation pending (BUG-004)** |

The source chooses `CT_WEAPON_ONLY` for applicable ranged artillery duels when both units can use artillery. A normal adjacent attack between ships in open ocean is not automatically a weapon-only artillery duel; the chosen attack mode matters. The duel path can eliminate a participant and does not use the ordinary bombardment damage caps.

The defending artillery unit's **weapon**, not purchased armour, currently sets its duel strength. This applies to land and naval counterbattery combat. Whether armour should improve artillery-duel resistance is an **unapproved balance proposal**, not BUG-004.

In the ordinary bombardment path, the game calculates damage from artillery offence against relevant defence, then enforces a location-dependent floor on the target's remaining HP. The distributed `alphax.txt` has maximum cumulative bombardment damage of **50% in a base/bunker, 99% in open terrain, and 100% at sea**. Integer rounding of the HP floor matters near the limit.

## Reactors: intended effective HP versus stored HP

**Established design policy (DEC-012):** `ignore_reactor_power=1` means reactor technology does not confer extra **effective combat endurance**. Reactor cost discounts remain.

`VEH::max_hitpoints()` stores `10 * reactor_type()`; ordinary combat then scales damage to the damaged unit's reactor level. For a full-health conventional unit:

| Reactor level | Stored maximum HP | Stored damage per lost ordinary combat round | Effective lost rounds to destruction |
| --- | ---: | ---: | ---: |
| Fission (1) | 10 | 1 | 10 |
| Fusion (2) | 20 | 2 | 10 |
| Quantum (3) | 30 | 3 | 10 |
| Singularity (4) | 40 | 4 | 10 |

The larger stored number **is not intended as additional real HP**. The same round-damage scaling occurs in the weapon-versus-weapon duel path.

### BUG-004: ordinary bombardment violates the policy on the master baseline

Before draft PR #10, `mod_battle_fight_2()` computed `damage_value` from `offense_out`, `defense_out` and `artillery_dmg_numerator/denominator`, applied its HP cap, and then executed:

```cpp
veh_def->damage_taken += damage_value;
```

It does **not** multiply `damage_value` by the **target's** reactor level, even with `ignore_reactor_power=1`. Bombardment offence/defence calculations do not otherwise cancel the target's larger stored HP. Identical raw damage consequently removes a smaller effective fraction from higher-reactor defenders.

Illustrative **fixed raw 2-point bombardment result** (not a claim that every hit inflicts two points):

| Target | Raw 2 points as % of stored maximum HP |
| --- | ---: |
| Fission (10 stored HP) | 20% |
| Fusion (20 stored HP) | 10% |
| Quantum (30 stored HP) | 6.7% |
| Singularity (40 stored HP) | 5% |

An intended two-*effective*-HP bombardment hit should remove **20%** from all four. Damage caps and RNG mean real combat outcomes vary.

**Source correction in [draft PR #10](https://github.com/Necaladun/thinkerNeca/pull/10):** multiply ordinary bombardment damage by the *defender's* reactor level when `ignore_reactor_power=1`. Apply base/bunker (50%), open terrain (99%) and sea (100%) damage floors in 10 effective HP first, then convert to stored HP. This matters at the 99% floor: on the master baseline a Fusion defender may be left at 1/20 stored HP, whereas correct ignored-reactor behaviour leaves 2/20; Quantum/Singularity floors are 3/30 and 4/40. `ignore_reactor_power=0` retains the pre-existing raw-damage and stored-HP cap behaviour. The source fix is host unit-tested but **not Windows runtime-verified**; audit special/native artillery, repairs, old damage states and UI before closure.

### Native units and Spore Launchers

The default native roster uses predefined psi units rather than ordinary technological reactor upgrades. Native lifecycle/morale and psi bonuses do not make native units equivalent to conventional Fusion-reactor designs.

Spore Launchers are explicitly special-cased in the artillery/psi calculation. A Spore Launcher **bombarding a Fusion conventional defender** can still encounter BUG-004, because the unscaled stored damage is subtracted from the *conventional target*. Standard native attacks resolved as combat rounds use the separate reactor-normalised hit mechanism.

**Player observation:** late-game Spore Launcher bombardment felt ineffective against Fusion targets. BUG-004 provides a plausible source explanation, **not yet a controlled Windows confirmation** that it caused every reported case.

## Experimental fractional weak bombardment — BAL-005 (draft PR #10)

This is a **new balance rule**, not a correction required to preserve original SMAC/X behaviour. The current `alphax.txt` artillery numerator/denominator is **3/2**. The previous bombardment RNG was uniform `0..floor(A/D)`, where `A = offense_out × numerator` and `D = defense_out × denominator`; adjusted morale, terrain, sensors, facilities, psi effects and other modifiers already enter `offense_out` and `defense_out`.

Draft PR #10 retains that entire distribution for **A ≥ D**. Only for **A < D**, it allows 1 effective HP with probability **A/(A+D)**, otherwise zero. This matches the 50% 1-damage probability at A=D and does **not** use WTP's different full-range stochastic-rounding variant. The weak-attack mechanism is adapted from [WTP/thinker-doer](https://github.com/tnevolin/thinker-doer/blob/master/src/wtp_mod.cpp).

| Unmodified weapon vs armour | Previous expected HP per shot | Proposed expected HP per shot |
| --- | ---: | ---: |
| Gun 1 vs Synthmetal 2 | 0 | 0.429 |
| Gun 1 vs Plasma 3 | 0 | 0.333 |
| Gun 1 vs Stasis 12 | 0 | 0.111 |
| Laser 2 vs Plasma 3 | 0.5 | 0.5 (unchanged) |
| Laser 2 vs Silksteel 4 | 0 | 0.429 |
| Laser 2 vs Stasis 12 | 0 | 0.200 |

These are **illustrative unmodified relative strengths**, not exact odds in arbitrary terrain. Cheap 1-weapon Fission infantry Artillery costs approximately 10 minerals and 2-weapon Laser Artillery approximately 20. Ten Gun Artillery bombarding an otherwise unmodified Stasis target would average ~1.11 effective HP damage per full volley; multiple defenders on one tile are each eligible for bombardment. More advanced weapons can be more mineral-efficient, but massed cheap guns may become strategically useful. Explicitly test the stack/massed-battery exploit risk, counterbattery survival and repairs before approving the balance outcome.

This experiment does **not** alter artillery duels, land versus naval artillery prices, direct attacks, production AI weights or artillery attack range.

## Proposed chassis HP: engineering constraints only

A hypothetical +1/+2 **effective** Tank/Cruiser HP bonus would need internally consistent reactor-scaled storage and damage. Changing a single displayed number would not suffice. Audit `VEH::max_hitpoints()`, `cur_hitpoints()`, damage thresholds, repair rates, monolith healing, conventional/psi/artillery damage, collateral damage, combat-odds displays, defender scoring, AI unit design, serialization and same-build save/reload.

No custom chassis-HP rule is currently implemented. Numerical balance examples and design options live in [IDEAS](../IDEAS.md); specific implementation and runtime acceptance work must be promoted from [BAL-004](../WORK-QUEUE.md) before changing game rules.

## Evidence boundary

The reactor discrepancy was a **source-inspection finding**; [draft PR #10](https://github.com/Necaladun/thinkerNeca/pull/10) contains a proposed correction with C++11 regression and Windows Develop/Release compilation passed ([CI run 42339591860](https://github.com/Necaladun/thinkerNeca/actions/runs/42339591860)). **User Windows Develop preliminary smoke (7 Nov):** Impact 4 artillery occasionally damaged armour 8; Fusion level-2 defenders sometimes lost comparable effective HP to Fission level-1; the 50% base/bunker cap held; and Spore Launchers damaged Fusion targets. This is a user-reported qualitative test, not source/CI proof or measured statistical parity. **Formal new-game same-build save/reload, 99% open/100% sea caps, alternate reactor mode and broader counterbattery/stack cases remain unconfirmed.** See [TESTING](../TESTING.md). The installed INI or `thinker_user.ini` can override the distribution.
