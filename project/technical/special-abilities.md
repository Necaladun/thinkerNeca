# thinkerNeca — Special Abilities: Mechanics, AI Use, Balance and Deferred Ideas

**Status:** Deferred technical discussion reference. **Not an implementation specification, approved work queue, or request to change the game.**  
**Prepared:** 2026-11-07.  
**Repository basis:** [`Necaladun/thinkerNeca`](https://github.com/Necaladun/thinkerNeca), `master` at `32e0c61a56269d6ea1af494b7604823fa7536ca9` for inspected source and distributed game data.  
**Scope:** All 29 abilities in `docs/alphax.txt`; AI prototype creation, production evaluation and actual employment; closely connected naval-invasion observations.  
**Disposition:** **Park all ability and AI implementation work. This documentation-only integration authorises no gameplay, source, data, configuration or backlog changes.**

> **Evidence discipline.** “Source-confirmed” means the inspected implementation contains the behaviour; it is **not** a Windows runtime test. “Player-observed” means reported campaign experience. “Proposed” means an idea, **not** an approved decision. Original SMAC/X rules, inherited Thinker behaviour, thinkerNeca changes and the currently distributed experimental `thinker.ini` must not be conflated. `thinker_user.ini` and locally installed builds may differ. The general ability definitions and most AI scoring considered here are inherited/retained behaviours unless a specific fork modification is identified.

## 1. Overall design direction

The goal is **not** to rebalance every ability, maximise special-ability usage, or force homogeneous “best” prototypes. Most abilities have sensible roles. The larger potential gains are in **selecting appropriate designs for an actual job** and then employing them competently.

1. **Prototype design and production selection are different problems.** A positive production score is useless if no suitable prototype exists; an available prototype is also useless if tactical AI cannot exploit it.
2. **Context over unconditional bonuses.** Evaluate Empath against psi threats, Soporific against conventional forces, AAA against air threats, Non-Lethal against actual unsuppressed drones, and Clean against likely lifetime support expenditure.
3. **Preserve distinct production doctrines and price points.** Cheap/disposable units can be optimal precisely because they are cheap. The AI should not automatically equip every attacker with Clean, armour, morale and two specialist abilities. Expensive persistent units, defensive reserves, police specialists and sacrificial mass-assault units serve different purposes.
4. **Abilities need not all be equally prominent.** Specialized abilities such as Amphibious Pods may remain rare if their tactical payoff does not justify extensive new AI engineering.
5. **Prefer better AI use before mechanical power changes.** Avoid compensating for poor deployment merely by making abilities cheaper or stronger. Where an ability is itself economically excessive, discuss replacement instead.
6. **Preserve tactical counters and variety.** Mixed conventional/psi threats, different garrisons, specialist support and complementary fleets create meaningful decisions.
7. **No immediate work.** These are parked findings for a later, explicitly authorised design and implementation phase. Any eventual accepted task belongs in the established `project/WORK-QUEUE.md`, not a parallel task ledger.

## 2. Full ability inventory and current disposition

This inventory names **all 29 entries** in the current `#ABILITIES` data. “Retain” means no identified need to change the ability, **not** a formal permanent balance decision. “Review” and “candidate” are deferred proposals only.

| Ability | Discussion outcome | Deferred question / caveat |
| --- | --- | --- |
| **Super Former** | **Retain.** Twofold terraforming work rate for a nominal +25% cost is highly effective; the AI's +8 fixed ability weight seems justified. | None urgent; review only in larger Former AI overhaul. |
| **Deep Radar** | **Retain; optional redesign.** Free on sea/air, land cost premium; two-tile spotting can support explorers and artillery. | Earlier land-scout access (possibly Doctrine: Mobility), land cost and possible Unity Pod reconnaissance information; not agreed. |
| **Cloaking Device** | **Defer.** Useful stealth concept, but AI scouting, detection and targeting matter. | Assess actual stealth-capable designs and tactical AI before adjusting. |
| **Amphibious Pods** | **Retain, low AI priority.** Enables attacks from transports and immediate infantry assaults. | Thinker does not deliberately target defended shores for amphibious attacks; routine rover beachheads often suffice. |
| **Drop Pods** | **Retain; AI can use.** Source-confirmed drop routines for undefended enemy bases, threatened friendly bases and colony expansion. | Ensure suitable prototypes exist; coordinated airborne assaults are a larger optional project. |
| **Air Superiority** | **Retain.** Essential interceptor/SAM specialisation and counterplay. | Check composition and targeting in a wider air AI review; no proposed mechanics change. |
| **Deep Pressure Hull** | **Defer.** Submarine concealment is useful, but the AI must deploy and respond to it properly. | Tactical AI, detection and prototype availability. |
| **Carrier Deck** | **Defer.** Requires carrier-aware aircraft basing and fleet operations to realise its value. | Operational aircraft/carrier AI, not a quick weight tweak. |
| **AAA Tracking** | **Retain.** Strong, necessary defence against aircraft; AI already scores it positively. | More contextual distribution to threatened bases and forces. |
| **Comm Jammer (ECM)** | **Retain.** Valuable defence against fast conventional units; appropriate for certain garrisons. | Distribute based on actual mobile threats. |
| **Antigrav Struts** | **Retain; low-priority review.** Extra mobility is useful and broadly AI-exploitable. | Later mobility, cost and unit-role evaluation. |
| **Empath Song** | **Retain; improve AI context.** Powerful against psi, unnecessary on every conventional attacker. | Select specialist production based on native/psi threat rather than only a fixed weight. |
| **Polymorphic Encryption** | **Replacement/redesign candidate.** Increases enemy subversion cost, but is narrower than an actual defensive probe. | Consider repurposing as meaningful anti-probe protection; audit all ability ID/flag/faction references first. |
| **Fungicide Tanks** | **Retain; appears useful.** Nominal +25% for doubled fungus-clearance speed; most early/mid-game economies benefit from clearing fungus. | Verify prototype availability and AI use; do **not** assume Fusion is required for all Fungicide designs. |
| **High Morale (Trained)** | **Retain.** General combat benefit with diminishing value near morale caps. | Consider existing faction/facility morale and intended unit lifespan. |
| **Heavy Artillery** | **Retain land distinction; review naval pricing and AI scoring.** Land artillery loses normal attacks; ships retain them and gain extended range in the test profile. | Separate land/naval cost analysis; potentially reduce expensive cruiser surcharge, not necessarily global ability price. |
| **Clean Reactor** | **Retain; improve economic AI logic.** Lifetime mineral-support savings can be enormous; wrong for deliberately cheap expendable rush units. | Use support exposure, expected service life, role, construction premium, free slots and urgency. |
| **Blink Displacer** | **Retain.** Late-game bypass of base defences is valuable. | Check AI prototype availability/field use; +3 score seems broadly plausible. |
| **Hypnotic Trance** | **Retain.** Important psi-defence specialisation. | Threat-aware deployment; existing positive AI preference broadly sensible. |
| **Heavy Transport** | **Disabled in distributed data.** +50% cargo capacity offers a clear logistics role. | Whether enabling it would meaningfully improve naval logistics; not a current proposal. |
| **Nerve Gas Pods** | **Retain for now.** Major offensive benefit accompanied by atrocity/diplomatic consequences. | Investigate AI decision-making and the economic/diplomatic consequences in a separate review. |
| **Repair Bay** | **Defer.** Repairing embarked land units could give value to supporting ships. | Requires support-ship and transport operations AI. |
| **Non-Lethal Methods** | **Retain; improve garrison allocation.** Strong police effect, but only useful until actual drones are suppressed. | At +3 POLICE each equipped unit suppresses 3 drones vs 2 ordinary; mix with AAA/ECM/Trance based on need. |
| **Slow Unit** | **Leave as legacy.** Attached to the predefined Unity Foil; disabled as a normally available design ability. | It lowers movement but has **no direct construction discount** in the current cost formula. |
| **Soporific Gas Pods** | **Retain; improve offensive AI design/use.** Lowers enemy conventional morale by two levels; a strong general anti-conventional specialist. | Add/verify deliberate candidate designs and context-sensitive production; Dissociative Wave is a counter. |
| **Dissociative Wave** | **Retain.** Late-game counter to combat special abilities. | AI prototype availability and tactical valuation; not an immediate redesign. |
| **Marine Detachment** | **Strong replacement/removal candidate.** Repeated boarding chances and whole-stack capture can generate disproportionate free military assets. | Decide fundamental role and Pirates' compensation before any code or data change. |
| **Fuel Nanocells** | **Retain/defer.** Very late-game aircraft range benefit. | Evaluate actual AI range management and prototypes only if late air warfare is revisited. |
| **Algorithmic Enhancement** | **Retain.** Better probe success/survival, Hunter-Seeker bypass and high-PROBE infiltration make it genuinely valuable. | Investigate probe mission difficulty separately; do not weaken on the assumption that it is merely cosmetic. |

## 3. Offensive specialisation, defenders and unit roles

### Soporific versus Empath

- **Soporific Gas:** Reduces non-native defender morale by two levels (minimum Green), making it broadly useful against conventional military formations. Dissociative Wave counters relevant special-ability effects. It needs an AI-design and production path that actually yields and uses appropriate assault units.
- **Empath Song:** Strong psi-attack specialist (nominal +50% against psi). A faction facing heavy native-life pressure or an opponent fielding psi units should value it more than one in a purely conventional war.
- **Avoid simply putting both on every attacker.** Consider mutually useful specialist prototypes, the two-ability limit where applicable, and the actual mix of enemy targets. Proposed scoring changes are experiments, not selected constants.
- **High Morale** benefits both attack and defence, but its marginal value falls when units already enjoy strong training/morale effects or approach Elite.
- **Blink Displacer**, **Dissociative Wave** and **Nerve Gas** are late or specialised; the capability to field a prototype and the decision to deploy it should be checked separately. No broad rebalancing agreed.

### Defensive capabilities

- **AAA**, **ECM/Comm Jammer** and **Trance** are valuable counters and should not be indiscriminately reduced. Thinker's positive fixed scores are a useful baseline, but bases facing aircraft, fast ground units or psi forces warrant different combinations.
- **Air Superiority** is essential for air-interception and ground-based SAM roles; actual anti-air weapon/armour priorities differ from general attack priorities.
- **A cheap specialist garrison can be economically preferable** to adding another expensive facility, especially when its defensive role is also useful.

### Prototype quality is not a monotonic progression

The player reported a campaign in which **Clean armoured units and needlejets** were retained as valuable assets while **cheap, dirty offensive rovers** were mass-produced and deliberately sacrificed attacking the Spartans. Roughly a hundred rovers were lost, including intentional attacks intended partly to clear support expenditure and make room for newer Chaos-weapon prototypes.

That is a **player-observed economic doctrine**, not evidence that the AI already applies it. Its design implications extend beyond Clean Reactor:

- **Disposable rush forces:** cheap, high offensive output per construction mineral; no unnecessary Clean, armour or elite-cost premiums.
- **Standing reserves and expensive aircraft/ships:** survivability, morale and lifetime support can justify greater upfront investment.
- **Dedicated garrisons:** appropriate defensive abilities and police effects matter more than offensive value.
- **Long-service economic units:** Formers are particularly strong Clean candidates.
- **Preserve cheap prototypes alongside premium alternatives.** A newer, more expensive design should not automatically obsolete a deliberately economical military role.

## 4. Clean Reactor: future AI valuation

**Confirmed current rule:** Clean Reactor has a nominal **+50% construction-cost modifier** and removes the unit from ordinary mineral-support accounting. As a consequence, it also avoids consuming free-support slots that might be needed by other units. Effective savings depend on `SE_support`, base population, eligible unit type, applicable difficulty bonuses, unit home-base allocation and possible installed overrides.

**Current Thinker heuristic** (`src/build.cpp`): `unit_score()` adds **+16** to a Clean-equipped candidate if it is in the support-eligible plan range and either the faction's pending SUPPORT is below +3 or the producing base has nonzero mineral consumption. The score does **not** predict the individual unit's lifetime, disposable role, marginal free-support position or exact additional construction cost. Production-time penalties still enter separately.

A simple starting economic model for eventual investigation:

```
expected support saving ~=
    turns expected in active service
    × marginal support minerals avoided per turn

Compare with: clean prototype minerals - equivalent dirty prototype minerals
```

For example, a 30-mineral dirty rover versus a 45-mineral Clean rover costs an extra 15 minerals. At one otherwise chargeable mineral per turn, Clean has a **15-turn break-even**, ignoring other constraints; if the rover is lost after five turns, it does not repay itself. A Former still working after 60 turns may recover that premium many times over.

**Required qualifications:**

- **Cheapness can be the point.** Even with immediate support cost, disposable attackers may be more useful as additional bodies now than as fewer Clean units with future savings.
- **Free support changes marginal value.** A unit built under spare free capacity may not incur upkeep yet. Clean can nonetheless preserve capacity for future units.
- **High SUPPORT reduces urgency**, particularly at +3 where free support scales with base size; it does not make Clean universally worthless.
- **Mineral surplus measures immediate budget pressure, not lifetime return.** One mineral is a much larger fraction of a 5-mineral base's output than of a 100-mineral industrial base, but the latter may also be able to build and sacrifice dirty units faster. Do not evaluate by percentage alone.
- **Reassignment, upgrade, disbandment and obsolescence** alter future support exposure. Do not implement a complex survival simulator without a demonstrated AI benefit.

**Deferred direction:** Keep the ability's rules; later compare a modest role-aware heuristic against the present +16 rule, ensuring the AI sometimes makes **dirty** rush units even when Clean is available and appropriate for its longer-lived assets.

## 5. Police: use actual marginal suppression

Source-calculated POLICE effects for eligible garrison units:

| SE POLICE | Number of ordinary police units allowed | Suppression per ordinary unit | Suppression per Non-Lethal unit |
| --- | ---: | ---: | ---: |
| 0 | 1 | 1 | 2 |
| +1 | 2 | 1 | 2 |
| +2 | 3 | 1 | 2 |
| +3 | 3 | 2 | 3 |

At **+3 POLICE**, three ordinary units can suppress six drones; three Non-Lethal units can suppress **nine**. Mixed combinations can suppress **7** (one Non-Lethal), **8** (two), or **9** (three), **if those drones actually exist**. A size-7 base does not need nine-drone suppression; a larger base also may have fewer drones because of talents, facilities or other effects.

The remaining question is **opportunity cost**: a garrison equipped with ECM, Trance or AAA can suppress two drones at +3 POLICE while providing a stronger specialist combat counter. An additional Non-Lethal unit is useful only if it actually addresses otherwise unsuppressed drones or permits saved Psych/facility spending.

**Specific source caveat:** Thinker's `need_police()` excludes factions at +3 POLICE from the special Non-Lethal generation/score path. There may be a rationale (ordinary units already suppress two), but it could underprovide a third suppression point where valuable. The eventual AI should consider **existing police composition, number of remaining drones, policing limits and defensive threats**, not maximise the theoretical nine everywhere.

## 6. Terraforming abilities

### Super Former

- Nominal +25% ability cost for doubled work contribution; favourable long-service economics.
- Thinker's fixed `unit_score()` entry is **+8** (adds +64 score); no present case to reduce it.
- Thinker's custom late-game design routine generates Super Formers when eligible, but this is not a claim about every original-game design path.

### Fungicide Tanks

- Nominal +25%; removes fungus at doubled rate. Early and mid-game **fungus clearance is broadly useful**, while cultivating productive fungus often becomes attractive later in the technology tree.
- Thinker's additional combined **Super Former + Fungicide** design appears with a Fusion-or-better custom design path and the requisite ability eligibility.
- **Correction of an earlier inference:** The original game's `consider_designs()` still runs, with fork hooks. The player has encountered **ordinary Fungicide Formers before Super Formers/Fusion**. Therefore *Fusion is not established as a universal requirement for Fungicide prototypes*. No proven timing problem exists at present.
- Worth checking only later whether the AI maintains appropriate Fungicide units, how it prioritises fungus removal, and whether other terraforming plans make enough use of them. No immediate ability-price change.

## 7. Artillery: land and sea are not interchangeable

**Corrected distinction:** On **land**, the Heavy Artillery ability grants bombardment but removes the normal direct-attack option. On **sea**, conventional warships already have regular naval combat and bombardment capability; the ability provides extra ranged bombardment in the experimental `long_range_artillery=2` configuration **without removing normal ship attacks**. Mobile land artillery has tactical value for surge-bombard-withdraw behaviour, but remains a role trade-off, not a general upgrade.

The configured ability's cost factor is `-4`, tied to the chassis **cost/speed parameter**, not simply actual movement points. Nominal single-ability increments, before full prototype arithmetic and rounding:

| Chassis | Parameter | Increment | Interpretation |
| --- | ---: | ---: | --- |
| Infantry | 1 | 0% | Sensible for land unit losing normal attacks |
| Speeder/Rover | 2 | +25% | More useful mobile artillery |
| Hovertank | 3 | +50% | Even greater mobile flexibility |
| Foil | 4 | +75% | Ship retains ordinary attacks and gains range |
| Cruiser | 6 | +125% | Potentially excessive for extra range |

**Discussion direction:** Keep the **land cost progression** provisionally. Investigate a **separate naval artillery cost policy**, especially whether the Cruiser's +125% premium is justified relative to several cheaper Foils. Do not apply a flat +25% or +50% globally; that would change the land trade-off in an undesirable way. A naval-specific price adjustment would require targeted code or another explicit mechanism, not merely changing the common `alphax.txt` cost factor.

**AI scoring:** In `src/build.cpp`, ordinary artillery usually gets **−8**. Long-range naval artillery gets **+24** with the present `long_range_artillery=2`; an artillery-preference faction gets an **additional +20**, for a combined **+44** before other unit scores. User observations indicate expensive naval artillery is **not always used well by the AI**; the extra preference may be excessive, especially for Cruisers. Review pricing **and** production mix separately from tactical targeting. A high score is not justification for wasting minerals on a poorly employed role.

**Separate existing documentation:** PR [#6](https://github.com/Necaladun/thinkerNeca/pull/6) already discusses artillery-versus-artillery strength, armour, reactor-normalised HP, and the ordinary-bombardment reactor mismatch (`BUG-004`), along with chassis price experiments (`BAL-004`). **Do not duplicate or silently resolve those issues through this ability review.** Weapon-versus-weapon artillery duels are not improved merely by fitting higher armour; that is a distinct unapproved design question. Runtime validation remains necessary.

## 8. Reconnaissance, mobility, amphibious and airborne operations

### Deep Radar

- Spots two tiles; free on naval/air units in the data and incurs a land surcharge. It helps exploration and detecting concealed enemies.
- A proposed earlier land-reconnaissance role could make a **Deep Radar rover** useful for exploration and supporting two-tile artillery operations. A suitable two-ability unit could combine artillery and radar once allowed.
- **Unapproved ideas:** moving prerequisite earlier (Doctrine: Mobility was suggested), adjusting land cost, and giving scouts better *information about* Unity Pod risks rather than an automatic improved reward. These are distinct proposals; changing the `alphax.txt` prerequisite would also affect naval/air availability unless additional constraints exist.

### Amphibious Pods

- Allows land attacks directly from transports, without a preliminary exposed beachhead; infantry can strike immediately rather than spend its only movement point disembarking.
- A rover can often disembark onto an **empty** shore tile and still attack with remaining movement, reducing the tactical necessity of the ability. Direct ship-to-shore attacks can remain useful, though the transport itself remains vulnerable.
- Thinker's landing routine (`make_landing`) chooses legal, effectively unoccupied beaches and **does not deliberately search defended coast targets for amphibious assaults**. This does not prove every other AI path is incapable of an incidental amphibious attack.
- `unit_score()` assigns Amphibious **−2** fixed weight (−16 score). Do not spend disproportionate AI development effort on a niche ability while basic invasion survivability is more important.

### Drop Pods

- **Actual AI use is source-confirmed:** `airdrop_move()` redeploys to threatened friendly bases and targets **undefended enemy bases**; the colony expansion routine can also drop Colony Pods to suitable sites.
- Requires eligible Drop Pod unit, valid airbase departure, full moves and a permitted destination; enemy aerospace coverage can block landing. Normal range in distributed data is eight tiles; later orbital insertion expands it. The 50% post-drop attack penalty and inability to land on occupied enemy tiles constrain offensive use.
- `unit_score()` assigns +4 fixed weight; Thinker's *additional* designer does not explicitly generate all desired Drop Pod assault designs, although original engine design routines also exist.
- Future opportunity: check actual prototypes and deployment frequencies before attempting coordinated multi-unit airborne assaults. This appears more promising than extensive amphibious tactical AI.

### Other mobility / logistics

- **Antigrav Struts:** reasonable extra mobility; inspect only in later unit-role optimisation.
- **Cloaking Device / Deep Pressure Hull:** defer until detection and stealth tactics are investigated.
- **Carrier Deck / Repair Bay:** defer until operational support and aircraft basing AI can exploit them.
- **Fuel Nanocells:** late aircraft range enhancement; no immediate concern.
- **Heavy Transport:** disabled in the distributed data; capacity bonus is conceptually useful but not enough alone to justify an enablement decision.
- **Slow Unit:** the predefined Unity Foil carries the legacy flag; no direct cost reduction from it in `mod_proto_cost()`; no change proposed.

## 9. Probe abilities and counterintelligence

### Algorithmic Enhancement — retain

Detailed source review found that it matters even when ordinary missions have high success probability. It reduces relevant failure and loss probabilities, supports operations against the **Hunter-Seeker Algorithm**, and enables certain operations against otherwise highly resistant +PROBE targets. A veteran probe may complete an easy mission with certainty but still have a meaningful chance of loss without enhancement.

The user generally employs probe teams less frequently and accepted this as a source-supported capability rather than evidence from their own campaigns. **Do not rework or weaken Algorithmic Enhancement as part of this parking-lot exercise.**

### Polymorphic Encryption — questionable niche

It increases the cost of enemy unit subversion; it can influence the attractiveness of attacking that unit with probes, but it is not equivalent to having an actual defensive counterintelligence probe. Thinker's opportunistic target logic can avoid Polymorphic-equipped units, so it is not wholly without AI value.

**Strong candidate for redesign/replacement**, but the effect and destination slot are not selected. A more active probe-defence ability was one possibility; it is not an approved design.

### Defensive probe units

- The AI would benefit from **cheap infantry defensive probes** for important bases, separate from **rover offensive probes** and **naval probes**.
- Prototype availability, production quotas, actual deployment and base coverage need to be evaluated separately.
- Probe mission difficulty-zero cases may be easy even without enhancement, but chance of probe loss is separate. A mission-by-mission difficulty audit is deferred, not evidence the entire probe system needs rebalancing.

## 10. Marine Detachment: potential fundamental balance problem

This is the strongest discussed candidate for **removal or slot repurposing**, rather than a small weight adjustment.

**Source-confirmed combat path (`src/veh_combat.cpp`):** Eligible attacking conventional ships with Marine Detachment can attempt boarding after sufficient target damage. When the threshold is reached, **each subsequent qualifying successful damage round against a still-surviving defender** has a 50% capture check. On success, the implementation transfers **all units in the target's tile stack** to the attacking faction, including possible embarked units. Captured units can receive `home_base_id = -1`, avoiding normal home-base mineral support.

Two problems compound:

1. The capture check repeats after the threshold rather than being limited to a single opportunity.
2. Entire stacks may transfer, potentially producing several expensive ships and/or their cargo from one fight, often without subsequent support cost.

Even changing these two details could leave ship capture **economically overpowered**: taking a single high-value enemy cruiser can repay the Marine premium many times over. The ability is also part of the **Nautilus Pirates' faction identity** in the discussion, so removing it requires explicit consideration of any faction-granted ability and replacement benefit.

**Later design choice:** retain a much weaker boarding mechanic, replace Marine with a different naval ability, or repurpose the slot for something else (one speculative replacement was a **Robust** effective-endurance ability, itself unapproved and subject to reactor/HP engineering). No choice has been accepted. Audit ability IDs, AI generation, faction data, UI and same-build save/reload before implementing a repurpose.

## 11. Naval AI findings — separate from ability balance

The player's experience: Thinker sometimes executes genuinely surprising, effective **overseas invasions**, but its transports are **too often caught without adequate escort**. Do not discard the working invasion planner just because fleet protection is weak.

### Invasion transport and warship coordination

- `invasion_plan()` picks routes, start/end and beaches; `trans_move()` loads and transports land forces; `make_landing()` assigns beachhead movement.
- A loaded invasion transport can depart after loading enough units **without checking whether escorts are actually present**.
- `combat_escort` directs eligible warships toward the **invasion destination**, not necessarily to a selected, moving transport. They may arrive early, late, or pursue other priorities.
- **Deferred idea:** short escort assembly, explicit transport–escort association, coordinated travel, threat awareness and timeouts to prevent waiting forever. Unescorted low-risk invasions should remain possible.

### Transport armour selection

The current `find_proto()` scoring differs between the specialised ferry-production branch and general invasion transport production:

| Production path | Design evaluation | Armour-strength score per point | Speed score per movement point |
| --- | --- | ---: | ---: |
| Ferry from a sea base with economic land units needing transport | `DEF` | +16 (with other defensive modifiers) | +16 |
| General naval transport production / invasion | `ATT` | +2 | +32 |

The ferry branch addresses cases such as Colony Pods, Formers or Supply Crawlers at a sea base that need transport to land. This does **not** mean the AI always designs a new armoured transport prototype for the task; the selector chooses from available designs.

The weighting appears backwards relative to risk: military transports deliberately enter contested waters carrying valuable armies, whereas routine ferries may operate in safer water. **Deferred idea:** ensure appropriate armoured transport prototypes exist and give military transport selection a distinct armour/speed/cost evaluation. Do not merely set all transports to maximum armour, and do not presume escorts cease to matter if transports are armoured.

### Amphibious versus escorting priority

An expensive coordinated-amphibious-assault algorithm appears less immediately valuable than safeguarding existing landings. **Improving transport survivability and escort coordination takes precedence over amphibious special-ability optimisation** whenever naval AI work is eventually selected. None of that work is authorised now.

## 12. Proposed investigation order *when this topic is reopened*

This is **not** an active priority queue, implementation commitment or list of accepted tickets. The ordering reflects perceived payoff from the discussion and must be reassessed against GitHub's then-current source, existing PRs and work queue.

| Broad area | Why revisit | First evidence needed |
| --- | --- | --- |
| **AI design architecture / unit roles** | Context-sensitive specialists and cheap-vs-elite production underpin several changes | Current prototype inventory and AI production across factions/eras |
| **Soporific / Empath / defensive probes** | Clear tactical roles not fully represented by fixed generic scoring | Source audit of prototype generation and real AI unit composition |
| **Invasion transport survivability** | Large cargo losses can erase entire invasions | Transport prototype availability, escort proximity and loss patterns in Windows tests |
| **Marine Detachment balance** | Potentially disproportionate economic gain | Controlled captures, whole-stack and home-base support observations; faction-bonus audit |
| **Polymorphic replacement** | Niche effect may not justify an ability slot | Actual AI/player probe behaviour and viable replacement design |
| **Clean Reactor valuation** | Flat score misses disposable units, free support and lifetime economics | Comparisons of dirty rush, sustained reserve and Former production under varied SUPPORT |
| **Naval artillery pricing/scoring** | Cruiser premium may be excessive and AI tactical use poor | Full prototype prices, naval engagements and mix of foils/cruisers; cross-check existing PR #6 |
| **Non-Lethal garrison composition** | +3 POLICE gives 2 vs 3 drones per unit but marginal need is capped | Base-level drone counts, current garrison abilities and opportunity costs |
| **Radar, Drop Pods and niche mobility** | Existing mechanics offer specialised opportunities | Confirm tactical use/prototype availability before adding AI complexity |
| **Other advanced abilities** | Generally sensible but AI use not established | Narrow, evidence-led review only if relevant to another selected system |

**No constants, production weights, costs, ability flags or design changes are approved by this table.** A future promotion into the repository should produce **focused, independently testable tasks** with concrete acceptance criteria, not a wholesale “fix all special abilities” PR.

## 13. Correction and uncertainty ledger

These points were clarified during the conversation and should **not regress into stale documentation**:

1. **Land Heavy Artillery is not the same as naval Heavy Artillery.** The former sacrifices ordinary direct attacks; the latter adds range to ships that retain ordinary attacks. A universal fixed price ignores that difference.
2. **Artillery's `-4` cost is tied to chassis cost/speed parameter**, not the vehicle's actual current movement value; nominal +125% for Cruisers is an ability premium, not a claim every complete cruiser costs exactly 2.25 times a specified comparator after all rounding.
3. **Fungicide is not proven Fusion-gated.** Only Thinker's additional combined high-tech design is Fusion-gated; the original `consider_designs()` path still runs. Earlier ordinary Fungicide Formers have been player-observed.
4. **+3 POLICE does not make Non-Lethal equal to four suppression.** Source adds one from +3 POLICE and one from Non-Lethal to the normal baseline, giving **three** per equipped unit; mixed garrisons often make more sense.
5. **Clean does not become automatically correct just because support is charged.** Expendable rush forces are a counterexample; Clean also preserves free-support capacity for other units.
6. **A Drop Pods production weight is not proof of a suitable prototype.** Conversely, actual AI airdrop logic **does** exist and can capture undefended enemy bases.
7. **An Amphibious production penalty is not proof that all amphibious actions are impossible**, only that routine invasion code does not deliberately exploit direct shore assaults.
8. **Observed armoured transports are compatible with the AI's available-prototype scoring**, but the source inspection did not establish all paths that originally generated those armoured designs.
9. **Passing source/static checks is not runtime confirmation.** No new SMAC/X Windows behaviour has been tested in preparing this note.
10. **No ability proposal here is an established repository decision.** This draft stores goals and reasoning for a later session; it must not be interpreted as authorisation to modify a branch or existing PR.

## 14. Source references and future use

### Primary source locations (read at the cited repository snapshot, recheck before work)

- [`docs/alphax.txt`](https://github.com/Necaladun/thinkerNeca/blob/32e0c61a56269d6ea1af494b7604823fa7536ca9/docs/alphax.txt): all 29 ability definitions, prerequisites, costs and availability flags; chassis data; combat parameters.
- [`docs/thinker.ini`](https://github.com/Necaladun/thinkerNeca/blob/32e0c61a56269d6ea1af494b7604823fa7536ca9/docs/thinker.ini): **distributed experimental** `design_units=1`, `long_range_artillery=2`, `modify_unit_support=0`, `ignore_reactor_power=1`, and difficulty-specific support bonuses.
- [`src/build.cpp`](https://github.com/Necaladun/thinkerNeca/blob/32e0c61a56269d6ea1af494b7604823fa7536ca9/src/build.cpp): `unit_score()`, fixed special weights, Clean and police bonuses, `find_proto()`, ferry and general transport selection.
- [`src/plan.cpp`](https://github.com/Necaladun/thinkerNeca/blob/32e0c61a56269d6ea1af494b7604823fa7536ca9/src/plan.cpp): additional `design_units()`, naval and defensive prototypes, probe, police and Former variants.
- [`src/move.cpp`](https://github.com/Necaladun/thinkerNeca/blob/32e0c61a56269d6ea1af494b7604823fa7536ca9/src/move.cpp): drops, invasion planning, transport movement, landings, escort destination and combat target evaluation.
- [`src/veh.cpp`](https://github.com/Necaladun/thinkerNeca/blob/32e0c61a56269d6ea1af494b7604823fa7536ca9/src/veh.cpp): artillery eligibility, prototype costs, special-ability handling, movement and transport capacity.
- [`src/veh_combat.cpp`](https://github.com/Necaladun/thinkerNeca/blob/32e0c61a56269d6ea1af494b7604823fa7536ca9/src/veh_combat.cpp): direct/artillery combat and Marine Detachment capture path.
- [`src/veh_action.cpp`](https://github.com/Necaladun/thinkerNeca/blob/32e0c61a56269d6ea1af494b7604823fa7536ca9/src/veh_action.cpp): terraforming/Fungicide work and airdrop execution.
- [`src/base.cpp`](https://github.com/Necaladun/thinkerNeca/blob/32e0c61a56269d6ea1af494b7604823fa7536ca9/src/base.cpp), [`src/engine_base.h`](https://github.com/Necaladun/thinkerNeca/blob/32e0c61a56269d6ea1af494b7604823fa7536ca9/src/engine_base.h): actual support charging/free slots and police suppression.
- [`src/tech.cpp`](https://github.com/Necaladun/thinkerNeca/blob/32e0c61a56269d6ea1af494b7604823fa7536ca9/src/tech.cpp), [`src/patch.cpp`](https://github.com/Necaladun/thinkerNeca/blob/32e0c61a56269d6ea1af494b7604823fa7536ca9/src/patch.cpp): original `consider_designs()` is still invoked, with patched proposal and cost functions.

### Repository placement and future use

As checked on 2026-11-07, `PROJECT.md` and the `project/` documentation structure **are not on `master`**; they are introduced on **draft PR [#4](https://github.com/Necaladun/thinkerNeca/pull/4)**. The separate documentation-only artillery analysis is on PR [#6](https://github.com/Necaladun/thinkerNeca/pull/6), based on PR #4. Do not assume either has merged at the future integration date.

This review is stored at `project/technical/special-abilities.md` and linked from the project guide and ideas index. When the topic is reopened, reread `PROJECT.md`, the current `project/STATUS.md`, `WORK-QUEUE.md`, `DECISIONS.md`, `IDEAS.md`, `TESTING.md`, and `project/technical/unit-combat-and-costs.md`. **This is not a second task ledger.** Only subsequent approved implementation work belongs in `WORK-QUEUE.md` with acceptance criteria, and only subsequent material decisions belong in `DECISIONS.md`.

**Final disposition:** Documentation-only preservation of the discussion; **no gameplay/source/configuration/ability changes or backlog tasks**, no features approved for implementation and no runtime testing claimed. Defer the entire special-ability programme until explicitly revisited.