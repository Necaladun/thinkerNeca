# thinkerNeca — AI Strategy, Operations, Formations and Faction Behaviour

**Status:** Discussion capture; deferred design reference — **not an approved implementation plan**  
**Prepared:** 2026-11-07  
**Primary repository:** <https://github.com/Necaladun/thinkerNeca>  
**Scope:** Findings from source inspection and game comparisons, plus ideas for future AI strategy, operations, tactical coordination and balance.

**Related active design:** [Campaign telemetry and AI-only automation](telemetry-automation.md) is an accepted but unimplemented development plan. Operational formations and AI doctrine remain deferred proposals.

> **Explicit exclusions:** Game-event logging/debug infrastructure is being handled in another discussion. Mineral-cost rounding, cost plateaus and free-upgrade redesign are a separate project. Neither should be implemented on the basis of this document. No AI modification or runtime validation was performed in this discussion.

## 1. Principal findings

The core hypothesis emerging from the discussion is that thinkerNeca handles **economic development, colonisation and individual military actions** reasonably well, while **sustained land operations and coordination between units** leave considerable room for improvement.

The AI is not simply mindlessly attacking the nearest enemy. It has faction-wide strategic information, unit-production heuristics, regional priorities, saved goals and some inherited multi-unit staging behaviour. Its naval invasion code already uses staging and multiple prospective landing sites. However, much of land combat appears to be decentralised: individual units choose movements and engagements using shared priorities, rather than belonging to a stable combined-arms group with a shared mission.

The proposed middle layer is a **formation**: a dynamic group of units that cooperates to accomplish one task, potentially as part of a larger **operation**. Formations should coexist with ordinary front-line fighting, opportunistic attacks and independent units. A complementary proposal is to recognise **operational failure** and change approach instead of feeding replacements indefinitely into an expensive chokepoint.

These are future design directions only. Existing systems require auditing before their replacement or extension.

## 2. Evidence and interpretation

Different statements below have different evidentiary status:

- **Source-inspected:** Behaviour found in thinkerNeca's code on the repository's default branch as examined during the discussion, including a fresh selective check on 2026-11-07. The presence of code does not guarantee runtime correctness or prove that a particular branch of logic was exercised in a save.
- **Save-derived:** Observations and comparisons from selected `.SAV` snapshots. Two snapshots show changes in state, not every intermediate event or its cause.
- **Gameplay reports:** Human observations such as effective chokepoints and disastrous enemy stacks. Useful reproduction scenarios but not controlled measurements.
- **Proposed:** Future approaches, preferences and architecture that have not been implemented or accepted as settled design.

Two **different games** were discussed, and their statistics must never be merged into one timeline:

| Game | Setup | Main coverage |
|---|---|---|
| Earlier faction comparison | 160×160 map, automated human Gaians versus six full AI factions; Gaian technology was later altered by the player | Approx. MY 2105–2190 |
| Seven-AI observer game | 128×128 map, seven AI-controlled factions | MY 2101–2218; emphasis on 2184–2218 |

No multiple-seed, completed-game benchmark was performed. Difficulty bonuses, map generation, research choices, installed configuration and military history can all distort comparisons.

## 3. Existing military AI: what it really does

### 3.1 Individual land combat

`src/move.cpp::combat_move()` handles unit-level movement and combat decisions. Its inputs include map priorities, local threats, possible enemy bases, combat odds, artillery situations, defence requirements, friendly concentrations and some flanking/avoidance opportunities. `target_priority()` supplies shared priorities for attractive enemy locations.

This is more capable than a nearest-target routine, but it does not by itself establish a persistent land **formation** containing multiple units and distinct roles.

A visible consequence can be a “conga line” of individually routed units approaching the same front. Units may arrive at different times, especially when artillery, attackers and escorts have different movement capabilities. A large economy can then sustain attacks that accomplish little against a heavily fortified line.

**Hypothesis, not yet benchmarked:** The AI's problem may often be coordination and operational cost-benefit assessment, rather than inability to build armies or evaluate a single battle.

### 3.2 Legacy multi-unit staging: important finding

`src/faction_event.cpp::enemy_strategy()` already maintains `Faction.target_x` and `target_y`, generally a **friendly assembly base** used as a starting point for strategic activity in a region.

It can:

- Select land units meeting eligibility checks.
- Mark selected units with special state flags.
- Order those units to assemble at a friendly staging base.
- Evaluate whether enough designated units have assembled.
- Cancel or redirect that effort when its regional context changes.
- Stop one selection loop after **eight** units, a small fixed cap characteristic of the inherited older code.

This is a partial ancestor of formations, **not** a full operational controller. It lacks explicit combined-arms roles, dynamic mission membership and a sustained coordinated assault procedure. Its interaction with Thinker's newer movement/upkeep routines must be tested: code existence is not proof that it works in the intended way at runtime.

**Follow-up:** Audit legacy staging thoroughly before introducing a competing mechanism.

### 3.3 Naval invasions already provide a model

`src/move.cpp::invasion_plan()` and related functions select naval staging areas, destinations, prospective landing/embarkation sites and scouting goals. Naval missions use a share of eligible land, sea and air units rather than requiring the whole army.

`invasion_unit()` currently uses the **vehicle-array ID modulo 16** for approximate participation. This is sufficient for a coarse allocation method, but is not durable formation membership: vehicle indices change when the array is altered.

Saved faction goals include naval start/end/beach/scout and embarkation-related coordinates. The observer saves exposed plans such as multiple prospective landing locations. Thus the AI is already capable of something more deliberate than unit-by-unit land combat in one domain.

**Interesting future intelligence feature (not present):** A Probe Team could potentially discover enemy staging points or planned invasion areas, producing an intelligence report such as “enemy naval mobilisation identified off the northern coast.”

### 3.4 “Objectives” means several different things

Do not conflate the following:

| Mechanism | Meaning |
|---|---|
| `is_objective(base_id)` | Scenario victory objective; relatively unimportant to the intended fresh-game focus |
| `Faction.base_id_attack_target` | Preferred **enemy base** designated principally through diplomatic battle planning |
| `Faction.target_x/y` | Older autonomous **friendly staging location** |
| `AI_GOAL_ATTACK` | A saved location/priority goal; generated by the legacy strategic routines |
| `target_priority()` | Thinker's dynamic score for a target location |
| **Future operational objective** | A proposed explicit mission such as conquest, raid, subversion, defence or containment |

The two observer snapshots from MY 2184 and 2218 had `base_id_attack_target == -1` for all seven factions. **That does not mean the AI was fighting without targets.** The normal AI still identifies and attacks enemy bases using other mechanisms.

#### Diplomatic battle planning has an unexpected side effect

`src/faction_talk.cpp::battle_plans()` invokes `suggest_plan()` to choose preferred targets. **It sets the AI's preferred `base_id_attack_target` before the diplomatic dialogue has concluded.** If the player proposes another base, the code compares route and regional accessibility before agreeing to switch.

Consequently, a request to attack X can make the AI designate **Y**, its own preferred base, even if the user does not obtain agreement. The familiar dialogue “I'm already attacking Y; that's better” does not prove it had a long-standing plan to attack Y; that preference may be calculated during the conversation itself.

When the field is set, Thinker's `target_priority()` applies an area-wide priority bonus:

```cpp
score += 80 * clamp(6 - map_range(base.x, base.y, x, y), 0, 5);
```

That gives +400 at range 0–1, then +320/+240/+160/+80 at ranges 2–5. The older `enemy_strategy()` also references the designated base. The field is cleared or adjusted if its target is no longer valid.

**Development implication:** Reusable targeting ideas exist, but the diplomatic interface and `base_id_attack_target` should not automatically become the foundation of new autonomous operations.

## 4. Proposed hierarchy: strategy, operations, formations, tactics

The discussion converged on **formation** as the right term for dynamically coordinated groups of units. An **operation** is broader: it specifies an objective and method, and may involve one or several formations.

| Level | What it decides | Example |
|---|---|---|
| Strategic priority | Which threat, front or enemy matters | Defend western frontier over overseas conquest |
| Operation | What outcome is sought | Capture base, raid economy, defeat native swarm |
| Formation | Which units cooperate, in which roles, where | Wave #5, mobile raiders, artillery battery |
| Unit tactics | Exact movement, bombardment, defence and attack | Select attackable tile, find cover, engage defender |

There should be **multiple simultaneous operations with priorities**. The AI might defend one front, prepare a major assault on another, send a fast raiding party through a flank, and commit a Probe Team to subvert an exposed base. Emergency responses may temporarily outrank planned operations.

### 4.1 Dynamic formation membership

Formation requirements worth exploring:

- Units may join, reinforce, be reassigned, withdraw, or leave because of destruction or injury.
- A formation can disband, merge, split into detachments or change its current mission.
- Its **shared objective and cohesion constraints** outlive an individual unit's turn.
- Role assignment is based on **capabilities**, not just unit design names: assault, artillery, armour/cover, anti-air, anti-psi, mobility, probe support, transport.
- Individual units retain some tactical initiative; formation constraints prevent them from charging off independently when coordination matters.
- Existing garrisons and necessary reserves should not be stripped to fill a formation.
- Units are not tied to a single tile: formations can occupy an area, deploy artillery behind the front and maintain a protected route.

**Wave #5** illustrates an intermediate implementation: even without a full operational planner, AI reinforcements heading to one front could be grouped so their attackers, artillery and escorts arrive together more often.

### 4.2 Size must scale to circumstance

All previous numerical examples—ten units, five attackers/two artillery/three defenders, mobilisation over 5–10 turns—were **illustrative, not required constants**.

A ten-unit attack is major in MY 2110 and trivial later. Size should derive from available mobile forces, production capacity, estimated resistance, mission priority, required capabilities and accepted risk. The AI should have both a **desired** force level and a **minimum viable launch** condition, with the ability to exploit suddenly weak targets immediately.

Avoid deadlocks when the last missing unit is destroyed or unable to reach staging. The group must be able to resize, cancel or proceed with a sensible substitute.

### 4.3 Normal warfare must continue alongside formations

Not all war should be deliberate, staged offensives. The AI still needs to send units promptly to threatened fronts, defend cities, attack exposed units and capture undefended bases.

A formation is **additional coordination**, not a requirement that every unit join one. A campaign may continue ordinary pressure while a specialised formation assembles, and fresh reinforcements should not blindly become another conga-line meatgrinder.

## 5. Possible operations and objectives

The set should be extensible; these are proposals, not an approved fixed taxonomy.

| Proposed operation | Objective and character |
|---|---|
| Concentrated assault | Assemble force to take a defended city or position |
| Artillery preparation | Defeat artillery and degrade conventional defence before assault |
| Mobile strike / manoeuvre | Exploit an exposed flank, vulnerable base or opening |
| Economic raiding | Destroy Formers, Crawlers, boreholes, farms, sensors or logistical assets |
| Probe operation | Infiltrate, steal, sabotage or subvert, with support as appropriate |
| Defensive concentration | Hold a choke, city, coastal landing area or economic district |
| Counterattack | Strike after the enemy has depleted itself in an offensive |
| Airborne assault | Use drop-capable forces for a particular objective |
| Amphibious operation | Extend naval invasion planning and landing coordination |
| Containment | Hold an expensive enemy position in place while investing elsewhere |
| Native-life response | Use suited troops and patrols against Planet's forces |

**Specialisations** should be requirements attached to an operation, not necessarily a hard-coded new operation type for each combination. For example, a native-life emergency may request Trance defence and Empath assault; an air threat may request SAM/AAA coverage; artillery supremacy may bias the composition of a conventional offensive.

### 5.1 Raiding is a potentially useful early experiment

Raids have a distinct success condition: **damage inflicted relative to own cost**, rather than returning safely or capturing territory.

A fast Rover or aircraft may be worth sacrificing if it disrupts valuable economic infrastructure. The user reported a strike-fighter attack destroying ten boreholes in one turn. A fully utilised standard borehole provides 6 minerals and 6 energy of nominal tile yields; ten such losses could deny 60 minerals and 60 energy per turn of raw potential. **Actual damage depends on whether the squares are worked or convoyed, output modifiers and recovery time.** Nutrient-raiding can indirectly cause population losses.

The AI could deliberately select economic targets, calculate expected impact and retreat or continue according to mission viability. A successful raid may also compel the defender to redeploy units away from the main front.

A Probe Team could be a formation member for intelligence, sabotage or subversion, not just a special case for Zone of Control behaviour.

## 6. Formations, stacks, artillery and ZOC

SMAC's stack rules create genuine tactical complexity:

- Putting offensive Rovers beside a better-armoured defender can protect the group in conventional combat.
- Artillery often needs a rear position with escorts, not simply the shortest route to the enemy.
- Too much concentration on one tile creates exposure to artillery and stack-destruction effects.
- ZOC, move order, unit speed, terrain, roads and fungus all constrain coherent movement.

The user described destroying a **53-unit Spartan stack in one attack**, followed by an enormous amount of artillery fire. It is an extreme example of the risks of concentration, not evidence that dispersal is always optimal.

The user also observes that friendly occupancy can allow movement routes that would otherwise be ZOC-restricted, with Probe Teams sometimes opening routes for other units. `combat_move()` has explicit ZOC exceptions for probes, but the precise friendly-occupancy mechanism and its implications for mixed formation movement still need a targeted runtime test.

**Requirement:** Formation pathing must model what each member can legally traverse, and must avoid exposing artillery or escorts because one specialist can move somewhere others cannot.

## 7. Operational failure and adaptation

A major desired improvement is for the AI to recognise **a tactic that is not working**.

User example: a four-base fortified position near the Unity Ruins reportedly withstood a Hive empire of 40+ cities for decades and destroyed roughly 200–300 attacking Hive units. The human defender exploited a narrow front with effective local garrisons and timely reinforcement, while the rest of the empire advanced its research and economy.

This illustrates the gap between *producing a large army* and *using it efficiently*. An offensive may accomplish little while consuming far more resources than the defender spends.

A capable operational AI should ask whether continuing the **same** attack corridor yields enough strategic progress. Possible alternative responses:

- Assemble a larger, coordinated assault with the right roles.
- Seek artillery superiority or different unit types.
- Go around the defence through another land route, naval landing or air drop.
- Raid productive rear areas, weakening the defender economically.
- Contain the fortress with a smaller force and direct the bulk of the army elsewhere.
- Pause until technology or circumstances materially change.

The decision should be based on the faction's **relative capacity** and the actual strategic results—not fixed thresholds such as “100 casualties” or “ten turns.” A small empire cannot absorb the losses tolerated by a 60-base industrial power. Destroyed units should be valued by actual capability/replacement costs, while success can include enemy losses, objectives taken, territory opened, infrastructure damage or valuable forces tied down.

Nor should failure permanently forbid another attempt. A formerly impregnable choke may become vulnerable after a defender redeploys or an attacker gains new technology. Adaptation should change the **approach**, not create a permanent geographical taboo.

The observation that AI wars often appear to continue indefinitely is worth testing across longer full-game runs. It is not established as a universal mechanic.

## 8. Faction personality and doctrine

**Open design question:** How much should emerge from the existing AI Fight/Power/Tech/Wealth/Growth priorities, and how much should be hard-coded to a named faction?

Provisional direction: **generic competent operational intelligence, modulated by faction priorities, circumstances and limited flavour biases**. Santiago should not have exclusive access to mobile strike formations, nor Miriam be forced to conduct suicidal assaults when they cannot work.

| Faction | Illustrative preferred approach (proposal only) |
|---|---|
| Hive | Waves of infantry, methodical assaults and artillery support |
| Believers | Morale-oriented aggressive offensives, high casualty tolerance |
| Spartans | Elite commandos, mobility, flanking and concentrated strikes |
| Gaians | Native life, fungus routes, unconventional pressure |
| University | Advanced weapons and technological/firepower advantage |
| Morganites | Economic efficiency, probes, selective engagements and disruption |
| Peacekeepers | Balanced combined arms and resilient defence |

A faction's *available technology, resources, enemy composition and geography* should outweigh style when necessary. In particular, **anti-mindworm responses** are about attacks from Planet, not necessarily fighting a native-life-oriented rival faction.

## 9. Economic AI, Social Engineering and phase thresholds

### 9.1 ECONOMY and GROWTH breakpoints

The distributed `docs/alphax.txt` contains important non-linear thresholds:

| Rating | Important effect |
|---|---|
| +1 ECONOMY | +1 energy per base |
| **+2 ECONOMY** | **+1 energy per worked square** |
| +3 ECONOMY | Additional commerce rating |
| +4 ECONOMY | Further base energy and commerce bonuses |
| **+6 GROWTH** | Population boom threshold; e.g. +4 GROWTH plus a Children's Creche's +2 |

`src/faction.cpp::social_score()` explicitly rewards ECONOMY ≥ 2 and further rewards ≥ 4. It also assesses growth/boom potential and other ratings.

**Morgan's structural advantage:** His native +1 ECONOMY combines with Wealth's +1 to reach +2 without Free Market. He gains the square-level energy bonus while avoiding Free Market's enormous POLICE/PLANET penalties, and can pair Wealth with other Economics models subject to restrictions. Wealth also provides +1 INDUSTRY at a MORALE cost. This may be a significant **balance concern**, but no nerf was decided.

The difference between *getting one additional rating point* and *crossing a threshold* is large. This interacts strongly with intensive expansion, many worked squares and rapid population growth.

### 9.2 Fusion/Quantum as heuristic phase changes

Thinker sometimes treats **Fusion Power** as evidence that a faction has reached a more developed economic and military phase. `src/build.cpp::select_colony()` contains a local `cheap` flag tied to availability of ship construction and Fusion-or-higher reactors; it raises the tendency to build sea Colony Pods. Related reactor-era conditions influence design of armoured probes, Crawlers, Formers, artillery and naval units.

This is an interesting and generally useful simplification. It might inspire other **economic phase proxies**, but should not be blindly transplanted into unrelated code.

The user explicitly considers mineral-cost plateaus and free equipment upgrades a **separate, substantial project**, not part of this document.

### 9.3 Colonisation and ICS

A typical land Colony Pod in the examined settings costs ~30 minerals plus one population from the originating colony. Population booming can replace the population quickly, especially at a size cap. A new base produces resources immediately and claims territory, making intensive settlement (ICS) unusually potent.

Therefore a weak early colony may still be economically worthwhile. The stronger site-quality question is **whether another available location would have produced a better return**, after considering spacing, danger, terrain and future growth.

## 10. Colonisation AI: interesting successes, not a major redesign target

In the seven-AI observer run, total bases rose from **189 (MY 2184)** to **239 (MY 2218)**. Sea bases rose from **zero** to **25**, notably University 9 and Peacekeepers 8. None of the factions had Fusion Power by 2218, demonstrating that this maritime shift was **not** exclusively caused by the Fusion `cheap` heuristic.

| Faction | Bases 2184 | Bases 2218 | Sea bases 2218 |
|---|---:|---:|---:|
| University | 28 | 44 | 9 |
| Peacekeepers | 24 | 40 | 8 |
| Hive | 36 | 44 | 4 |
| Gaians | 22 | 30 | 2 |
| Spartans | 25 | 28 | 1 |
| Believers | 20 | 20 | 1 |
| Morganites | 34 | 33 | 0 |

Significant sites:

- **U.N. Commerce Committee, Unity Ruins:** Eight monoliths in a fungal area provide strong production and unit-training opportunities, despite being relatively isolated. A thematic research/native-life focus would fit the setting; this was an interest for roleplay, not a detected AI specialisation.
- **The Marine Institute:** Appropriately named and economically productive University base in a sheltered freshwater sea.
- **Eternal Kingdom:** Newly settled Believer foothold on another landmass; weak present output does not negate territorial value. Nearby Spartan-owned *He Walked on Water* bears a Believer-style name, suggesting conquest but not establishing the exact sequence.
- **U.N. Global Governance:** Weak current output and much fungus. Its cost may still be low enough to justify settlement; whether its exact site was optimal remains open.

**Provisional conclusion:** Colony scoring and naval expansion appear capable. Preserve what works; investigate concrete failures rather than redesigning wholesale.

## 11. Faction-comparison results relevant to AI

### Earlier 160×160 game, MY 2190

| Faction | Bases | Net minerals/turn | Labs/turn |
|---|---:|---:|---:|
| Hive | 53 | 322 | 137 |
| Believers | 54 | 245 | 124 |
| University | 40 | 206 | 182 |
| Morganites | 38 | 240 | **327** |
| Spartans | 44 | 177 | 113 |
| Peacekeepers | 42 | 226 | 124 |

The Hive and Believers expanded explosively. The Hive's research path left it without Planetary Networks, preventing Planned Economics. Morgan's Wealth transition, economic infrastructure and Crawlers helped produce extraordinary labs despite fewer bases. Spartans recovered strongly after a slower start. The Believers' apparently premature borehole possession remained an unresolved source/rules discrepancy; it is not proof of an AI bug.

### Separate seven-AI game, MY 2218

Net minerals/turn: Hive 288, Gaians 275, University 265, Peacekeepers 240, Morganites 236, Spartans 183, Believers 126. Labs/turn: Peacekeepers 303, University 259, Morganites 238, Gaians 221, Hive 169, Spartans 85, Believers 57.

Factions did *not* retain the same rankings across the two games. No long-run strength hierarchy has been established.

### Military escalation in the seven-AI observer game

- Combat losses recorded by MY 2184: **237** cumulative units.
- Additional combat losses MY 2184–2218: **1,063** units in 34 years.
- Approximate nominal mineral replacement value: **33,970 minerals** across the seven factions; this is not verified historical expenditure. AI cost discounts and other modifiers can materially reduce true replacement costs.
- Combat includes losses to Planet, not necessarily only inter-faction warfare.
- Hive losses in the 34-year interval: 93 units; the Hive also gained three Believer bases by conquest. Losses cannot all be attributed to that war.

The shift toward heavily contested territory coincides with land settlement becoming constrained. It is plausible roleplay material for Planet's emerging **World War era**, but this causal explanation is an interpretation, not proof.

A major strategic lesson: **conquest and attrition must be measured against alternative uses of production**. Winning a war need not mean profiting from it, and a defensive stalemate can be an excellent economic/technological victory for the defender.

## 12. Open engineering questions and tests for later

1. **Legacy staging:** Does `enemy_strategy()` actually assemble its marked eight-unit groups reliably under the modern Thinker unit-order processing?
2. **Formation identity:** How should membership be represented without relying on mutable vehicle-array indices?
3. **Save/reload:** If operations become persistent, how are active objectives and memberships maintained across a fresh game's save/reload cycle?
4. **Competing objectives:** How are strategic priority, threatened bases, raids, probes and multiple offensives reconciled without depriving essential garrisons?
5. **Composition and capability:** How should the AI assess artillery superiority, mobility, armour, Trance/Empath, AA and probe support relative to a specific enemy?
6. **Formation positioning:** Can artillery stay protected while attackers advance without catastrophic stacking?
7. **ZOC interactions:** Reproduce the friendly-occupied-tile/Probe Team movement interaction before relying on it in pathfinding.
8. **Raiding payoff:** Distinguish worked/convoyed improvements from unused ones and measure damage to production, not only improvements destroyed.
9. **Failure assessment:** When does a costly front warrant a larger assault, a bypass, containment or withdrawal? How should old failures decay after technology or troop changes?
10. **Faction doctrine:** Which biases belong to generic AI priority settings versus faction-specific identity?
11. **War termination:** Do AI wars genuinely tend to continue indefinitely? Evaluate full-game outcomes, not just limited observer snapshots.
12. **Balance:** How frequently does Morgan's early +2 ECONOMY translate into victory, and how do other factions catch up later?

### Useful future test scenarios

- Four-city fortified chokepoint against a larger empire: evaluate concentrated assault versus frontal attrition versus bypass/raiding.
- Fast raiding force against active boreholes/Crawlers; compare actual economic disruption with force cost.
- Mixed artillery/assault/escort group versus independent arrivals.
- Large single-tile stack under artillery and conventional attack versus distributed formation.
- Native-life threat against an unprepared frontier: evaluate specialist defensive response.
- Early versus late era formation sizing (no fixed 10-unit target).
- Naval invasion and legacy assembly tests through actual Windows SMAC/X save/reload.
- Multiple full AI games completed to late-game or victory, varying map seed and holding configuration constant within comparisons.

### Provisional indicators of success

- Higher strategic gain per mineral-equivalent casualty, not merely more kills.
- Fewer repeated unproductive assaults against the same unchanged defensive line.
- Formations arrive with appropriate escorts/support and retain useful tactical initiative.
- No significant reduction in urgent defence, normal reinforcement or opportunistic captures.
- No serious new pathfinding loops, stack losses, performance regressions or save/reload defects.

## 13. Parked development sequence (suggestion only)

1. **Audit the existing mechanics**: legacy staging, shared targeting, naval invasion planning, and their interaction with current `combat_move()`.
2. **Prototype a limited dynamic formation** using existing units and one shared objective, without production-queue changes initially.
3. **Try raiding and small combined-arms operations**, with distinct mission-success definitions.
4. **Extend to multiple concurrent objectives** and formation priorities, including defence and native-life responses.
5. **Explore operational adaptation** after failed campaigns, with alternative tactics and theatres.
6. **Only then consider production requests, richer doctrine and persistent strategic plans** if benefits are demonstrated.
7. **Validate under real game conditions**, including new-game save/reload and repeated controlled comparisons.

This is a roadmap for *possible investigation*, not a commitment. Detailed analytical logging is an independent foundational effort and is not specified here.

## 14. Source orientation and repository status

Source files inspected during the discussion:

- [`src/move.cpp`](https://github.com/Necaladun/thinkerNeca/blob/master/src/move.cpp) — `target_priority()`, `combat_move()`, invasion planning and unit allocation.
- [`src/faction_event.cpp`](https://github.com/Necaladun/thinkerNeca/blob/master/src/faction_event.cpp) — inherited `enemy_strategy()`, staging, regions and attack goals.
- [`src/faction_talk.cpp`](https://github.com/Necaladun/thinkerNeca/blob/master/src/faction_talk.cpp) — `suggest_plan()` and diplomatic `battle_plans()`.
- [`src/engine_types.h`](https://github.com/Necaladun/thinkerNeca/blob/master/src/engine_types.h) — saved strategic fields, goals and faction state.
- [`src/goal.cpp`](https://github.com/Necaladun/thinkerNeca/blob/master/src/goal.cpp) — goal creation and priority handling.
- [`src/plan.cpp`](https://github.com/Necaladun/thinkerNeca/blob/master/src/plan.cpp) — AI prototype and force planning.
- [`src/build.cpp`](https://github.com/Necaladun/thinkerNeca/blob/master/src/build.cpp) — unit/colony production heuristics.
- [`src/faction.cpp`](https://github.com/Necaladun/thinkerNeca/blob/master/src/faction.cpp) — Social Engineering assessment.
- [`docs/alphax.txt`](https://github.com/Necaladun/thinkerNeca/blob/master/docs/alphax.txt) — distributed SE effects and game rules.

The user explicitly does **not** want to start these AI changes for some time. When development resumes, re-check the actual default branch, open PRs, documentation and installed configuration before accepting any implementation claims here as current. Source inspection and save analysis are **not runtime verification**.

**Classification:** Discussion/reference draft for later documentation integration; no repository files changed.