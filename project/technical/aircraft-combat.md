# Aircraft combat, anti-air design and transport roles — review draft

**Status:** Source-inspection findings and balance discussion; **no rule changes approved or implemented**.  
**Repository location:** `project/technical/aircraft-combat.md`, cross-referenced from [unit combat and costs](unit-combat-and-costs.md). Deferred bomber and armour alternatives are indexed in [Ideas](../IDEAS.md).  
**Scope:** SMAC/X mechanics as implemented in thinkerNeca's inspected C++ and distributed `alphax.txt`. Configuration, difficulty and runtime observations are separate matters. This is not a Windows gameplay test report.

## 1. Overall assessment

Current aircraft design has reasonably distinct roles and costs; there is **no agreed case for a general air-combat, aircraft-armour or chassis-cost rebalance**.

| Chassis | Current gameplay distinction | Principal limitation |
| --- | --- | --- |
| Needlejet | High operational movement and protection from ordinary attacks while in flight | Limited fuel endurance; usually one attack per turn; vulnerable to dedicated anti-air capabilities |
| Copter | Multiple attacks per turn; useful for repeated close air support | Can be attacked by ordinary ground units; exposed when operating forward; requires refuelling |
| Gravship | Unlimited fuel endurance and strategic freedom of movement | Extremely late technology; broader balance impact not yet established |

All three use **chassis cost factor 12**, not a flat 120-mineral price. The prototype formula then uses weapon, armour, reactor and abilities. Gravships may be more versatile, but their late arrival currently makes this a low-priority balance concern.

## 2. Conventional air combat and interception

### Normal versus weapon-only combat

In conventional **direct combat**, attack strength derives from the attacker's **weapon**, while defence derives from the defender's **armour**, subject to morale and other modifiers. In **weapon-versus-weapon** combat, the defending unit instead uses its *weapon* as its defence basis.

The key source condition in `src/veh_combat.cpp` applies weapon-versus-weapon when the **defending unit is an air-triad aircraft with Air Superiority** and the attacker is another non-missile aircraft. It does **not** require a particular scramble event. Thus, it also applies when a normal strike aircraft attacks an Air Superiority aircraft parked in a base, on an airbase, or with a carrier.

| Engagement | Base strength comparison | Important modifier |
| --- | --- | --- |
| Air Superiority aircraft attacks an ordinary aircraft | Attacker weapon vs target armour | **+100% attacking weapon strength** against non-AS aircraft in the current `alphax.txt` |
| Ordinary aircraft attacks an Air Superiority aircraft | Attacker weapon vs defender weapon | No additional +100% defender bonus in this weapon-only branch |
| Air Superiority aircraft attacks another AS aircraft | Weapon vs weapon | No +100% offensive AS-versus-non-AS bonus |
| Ground/naval SAM attacks an aircraft | SAM weapon vs aircraft armour | **No +100% aircraft Air Superiority attack bonus** |
| Ordinary aircraft attacks ground/naval SAM | Aircraft weapon vs SAM armour | Normal conventional defence; AAA can help |
| Ground unit attacks Copter/Gravship | Attacker weapon vs aircraft armour | Conventional ground attack is allowed; not restricted to SAM |

*Table assumes conventional, non-artillery combat and no additional situational modifiers. Psi and other special cases may differ.*

The +100% AS offensive bonus is frequently overwhelming against unarmoured aircraft, but it remains relevant against armoured aircraft. It is **not** applied to a defending interceptor in the weapon-versus-weapon path. An illustrative 6-1 Air Superiority fighter attacking a normal 6-1 Needlejet fights at a base **12:1**; a normal 6-1 Needlejet attacking the AS fighter fights at **6:6** instead, before morale and other effects.

### What interception actually means

ThinkerNeca's `interceptor()` routine may scramble eligible nearby Air Superiority aircraft from an airbase or carrier to protect an attacked tile. When the interceptor becomes the selected defending aircraft, weapon-versus-weapon applies. A *defending* AS aircraft therefore benefits from high weapon strength **instead of** high armour, but does **not** receive the +100% offensive AS bonus.

Interception is distinct from an interceptor **initiating** an attack against an enemy aircraft, where the normal weapon-versus-armour path and +100% AS bonus against non-AS aircraft apply.

There is a notable asymmetry: a high-weapon strike aircraft can resist interception effectively, even when unarmoured, whereas it is highly vulnerable to an interceptor initiating an attack on it. The current rule also means bomber payload weapon strength would count as defensive air-combat strength if an artillery bomber were introduced without special handling.

### Needlejets in flight

Needlejets in ordinary flight generally cannot be deliberately attacked by units lacking Air Superiority; this is enforced in the movement/attack eligibility checks in `src/veh_action.cpp`. Important exceptions concern aircraft at bases, airbases and carrier-supported locations. Copters and Gravships are not protected by the same in-flight targeting restriction and can be attacked by ordinary ground forces.

## 3. Air Superiority, ground SAM and AAA are different roles

- **Air Superiority on aircraft:** enables air-to-air interception/attacks, reduces aircraft movement in `proto_speed()` (approximately 20–25% before rounding), provides the +100% *offensive* bonus when attacking non-AS aircraft, and causes weapon-versus-weapon defence when the AS aircraft itself defends against another aircraft.
- **SAM (Air Superiority) on ground or naval units:** enables attacks against aircraft, including Needlejets in flight, but **does not** receive that aircraft-only +100% offensive combat bonus. Its attack is ordinarily weapon vs aircraft armour.
- **AAA Tracking:** a separate defensive ability. Current `alphax.txt` gives **+100% defence against conventional air attacks** when the AAA-equipped unit is defending in normal weapon-versus-armour combat; it does not double the strength of a SAM unit *initiating* an attack, nor does it apply through the weapon-only defence branch.

Consequently, high weapon strength is particularly valuable on **air interceptors**, because their weapons also provide defence against incoming aircraft. **Ground SAM hunters** may instead use comparatively inexpensive weapons against the commonly unarmoured Needlejets. AAA and good armour matter when those ground units themselves need to withstand enemy aircraft attacks.

## 4. Practical design incentives

| Design | Intended role | Rationale and limitation |
| --- | --- | --- |
| High-weapon, lightly armoured AS Needlejet | Air-defence/interceptor aircraft | Weapon strength determines defensive interception resistance; armour often does not. Cost, movement reduction and technology still matter. |
| Cheap Impact SAM Rover (4-1-2) | Mobile aircraft hunter | 4 weapon vs the usual 1 aircraft armour is strong for its price; mobile enough to chase aircraft. Vulnerable to counterattack and not guaranteed to win every engagement. |
| Impact SAM + AAA Rover | Mobile hunter with improved air-attack resistance | AAA protects when attacked by aircraft, not when initiating a SAM strike. Adding armour improves durability but increases mineral cost substantially. |
| Armoured assault Copter | Repeated close air support | Armour is more useful because ordinary ground units can counterattack it; multiple attacks may justify preserving an expensive veteran aircraft. |
| Lightly armoured strike Needlejet | Reach and economical strike capability | Ordinary ground units generally cannot attack it in flight; additional armour helps against offensive anti-air attacks but **not** when its own strike is intercepted weapon-vs-weapon. |

**Illustrative Fission mineral costs**, from the current prototype formula and default component factors, without prototype surcharges or other abilities:

| Prototype | Mineral cost |
| --- | ---: |
| 6-1 Needlejet (no special ability) | 50 |
| 6-3 Needlejet (no special ability) | 70 |
| 6-6 Needlejet (no special ability) | 110 |
| 4-1 Impact Rover | 30 |
| 4-1 Impact SAM Rover | 40 |
| 4-1 Impact SAM + AAA Rover | 50 |
| 4-3 Impact SAM + AAA Rover | 120 |

A 4:1 combat strength advantage is **not an automatic win**. Morale, damage, terrain, sensors, base facilities, modifiers and remaining movement can affect combat. A mobile ground unit can make further attacks while sufficient movement remains; remaining movement and hasty-attack rules must be considered rather than assuming Rover movement always guarantees two full-strength attacks.

## 5. Armour economics and reactor interactions

For **all air-triad chassis**—Needlejet, Copter and Gravship—`mod_proto_cost()` applies an armour cost surcharge:

```cpp
if (triad == TRIAD_AIR && armor_cost > 1) {
    armor_cost *= reactor_id * 2;
}
```

This is an intermediate **cost factor**, not a direct multiplication of the final mineral price. Reactor level also increases aircraft movement via `proto_speed()` (`+2` speed per reactor level, before applicable penalties); it does not itself increase the Needlejet's fuel-turn count. The same air-armour cost treatment across different chassis is a design simplification, not evidence of a current bug.

Armour is less attractive on Needlejets because it is expensive and does not affect weapon-versus-weapon defence during their own intercepted strikes. It has a clearer tactical use on Copters and Gravships, which can be attacked by ordinary ground units.

**Considered but not adopted:** remove or reduce the reactor multiplier for aircraft armour and instead impose a speed penalty as a proxy for weight. This could differentiate protected close-support aircraft from long-range light aircraft, but is **not approved**. It risks making advanced-reactor armour almost mineral-free through rounding/minimum-cost effects, penalising aircraft whose armour already does little during interception, and changing Copter/Gravship balance unintentionally. **Current conclusion: retain the existing aircraft armour and chassis cost rules**, absent new gameplay evidence.

## 6. Air transports: separate question

Air transport modules have marked speed penalties. In `proto_speed()`, a transport-equipped aircraft loses one speed point before its air speed is halved. A basic Fission Needlejet's nominal movement changes from **10 to 4** before other modifiers.

The discussed **Unload** capability may make transport aircraft tactically distinct from ordinary ferries by allowing unusual troop deployment; however, the precise landing conditions, troop readiness/attack penalties, carrier interactions and AI use were **not verified from the inspected unloading source path** for this review. Do not document unrestricted airborne unloading as a confirmed engine feature without targeted source and Windows testing.

Transport balance and AI escort/deployment capability are separate questions from the aircraft armour and interception rules. No change proposed here.

## 7. Deferred idea: bomber/stack-attack aircraft

**Priority: low; exploratory only.** Rather than adding a new chassis, a proposed bomber would use the existing **Needlejet chassis + Heavy Artillery ability**, with ordinary movement-initiated attacks resolving as **stack bombardment** using existing artillery damage and caps instead of selecting a single defender. This follows the existing land-artillery convention of attacking a neighbouring enemy square by movement rather than using the `F` artillery command.

Potential constraints and unresolved questions:

- The current `can_arty()` rejects air-triad units, and current Heavy Artillery ability flags do not permit aircraft. Combat routing, prototype legality, AI targeting and UI would need changes.
- Decide whether bomber artillery has range **0** for the `F` command (movement-only attack) or another explicit targeting rule. Existing Needlejet fuel/refuelling and interception must remain intact.
- A Heavy Artillery Needlejet should not also be an Air Superiority interceptor, if a specialised bomber role is adopted.
- The current Heavy Artillery ability cost scales with chassis cost factor (`-4` in `alphax.txt`), making it disproportionately expensive on a cost-factor-12 Needlejet. Aircraft-specific ability pricing may be needed; global artillery pricing need not change.
- Weapon strength currently serves both conventional attack power and weapon-versus-weapon defence. A high-power bomber could therefore fight through interception better than its role suggests. A bomber-specific defensive air-combat rule or ability flag is **an idea, not a requirement or implementation decision**.
- Existing artillery/counterbattery routing and the independently documented reactor-normalisation defect ([BUG-004](../WORK-QUEUE.md)) must be checked before introducing additional bombardment paths.
- Ground artillery with both Heavy Artillery and Air Superiority acting as counter-bomber artillery is a separate optional idea, not existing automatic counterfire.

No new bomber design, price, special flag, AI behaviour or testing milestone is approved.

## 8. Source-level discrepancy to investigate if air combat is revisited

`find_defender()` in `src/veh_combat.cpp` evaluates **ground SAM** candidates under `CT_WEAPON_ONLY` when the attacker is an aircraft, but later combat resolution only automatically sets that air-defence weapon-only condition when **both attacker and chosen defender are aircraft** (other special conditions, such as artillery duels, are separate). That suggests defender selection may rate a ground SAM using its weapon and then resolve the fight using its armour.

This is a **source-level inconsistency**, not yet an observed in-game failure. Check representative mixed stacks and actual chosen defenders before assigning a bug ID or proposing a fix.

## 9. Documentation integration and evidence

- **`project/technical/unit-combat-and-costs.md`:** cross-reference this focused aircraft reference and preserve the separate conventional artillery analysis.
- **`project/IDEAS.md`:** record only the deferred bomber proposal and the discarded-for-now armour-as-weight alternative. Do not imply implementation approval.
- **`project/WORK-QUEUE.md`:** **no new actionable task yet**. Promote the defender-selection discrepancy or transport test only after choosing an investigation and acceptance criteria. Existing `BUG-004` remains separate.
- **`project/TESTING.md`:** add a focused matrix only if an air-combat implementation or bug investigation is opened; no runtime evidence is claimed here.
- **`project/DECISIONS.md`:** no new settled combat-design decision required merely to document the current preference to leave balance unchanged.

**Inspected source paths:** `src/veh.cpp` (`proto_speed`, `mod_proto_cost`), `src/veh_combat.cpp` (`find_defender`, `mod_battle_compute`, `mod_battle_fight_2`, `interceptor`), `src/veh_action.cpp` (flight targeting eligibility), `src/engine_veh.h` (chassis and fuel classification), `docs/alphax.txt` (chassis, abilities and combat values). **Verification level:** repository source inspection and arithmetic examples only; no build, fresh-game test or installed-config audit was performed for this document.