# Ideas and design explorations

**Not commitments.** This is a parking place for possible gameplay/engineering directions. If a proposal is accepted for investigation or implementation, create/advance a stable ID in [WORK-QUEUE](WORK-QUEUE.md). Record only actual decisions in [DECISIONS](DECISIONS.md).

## Additional deferred reference reviews

- **Special abilities:** [29-ability review](technical/special-abilities.md) covers combat/economic value, AI selection, navy interactions and deferred balance concepts. No implementation is authorised by preserving the reference.
- **Air combat and aircraft design:** [aircraft mechanics review](technical/aircraft-combat.md) documents Air Superiority and SAM/AAA interactions, armour economics, transports and a low-priority proposed Heavy-Artillery Needlejet bomber. The armour-as-speed/weight proposal was considered but not adopted.
- **Operational AI and formations:** [strategy/operations discussion](technical/ai-strategy-operations.md) captures inherited staging, land-combat coordination, raids, operational failure, faction doctrine and observer-game evidence; this is **not** an approved implementation programme. The separate [telemetry/automation plan](technical/telemetry-automation.md) is accepted foundational work.

## Terraforming and terrain economics

- **Less universal Forests:** reduce Forest from 1 nutrient / 2 minerals / 1 energy to 1/1/1 to increase the relevance of dedicated Farms, Mines and Solar Collectors. Needs evaluation alongside Tree Farms, upkeep, terraforming turns and Thinker AI choices. See BAL-002.
- **Geology requiring extraction:** Titanium provides +1 mineral only with Mine; Uranium provides +1 energy with an appropriate improvement (Mine was suggested). Would make regions resource-specialised instead of all-purpose Forest terrain. Open cases: nonrocky mine legality, boreholes, base tiles, fungus, regular resource bonuses, crawler strategies. See BAL-003.
- **Solar still powerful:** keep elevation scaling as a feature, but teach Formers to build Solars at worthwhile elevations. Contrast projectable yields, worker demand and Former-turn opportunity costs. See AI-001.
- **Distinct geological environments (future):** Uranium and Titanium currently use identical placement rules, alternating one 3–9 tile patch per round, preferring lowlands and clearing fungus. After native rendering/placement is verified, consider Titanium preference for rolling/rocky terrain, allowing fungus to remain on Titanium (versus Uranium clearing it), or other ecological distinctions. Keep shared placement, separation and resource-yield machinery; evaluate actual map balance before changing rules.
- **More minor terrain features:** potential environmental/geological patches, but only once existing types have mature visuals, yield rules and balancing.

## Major landmarks

- **Slot 10 replacement:** now that Nessus Canyon is retired, design an attractive major landmark that does not inherit Canyon's broken visuals. Options range from geographic formations using existing terrain to unique mechanics requiring renderer hooks. See MAP-003.
- **Failed Colony:** explore a distinctive ruined human settlement/terraforming site as a major landmark. It needs an era-consistent explanation, actual tile benefits and an art plan before promotion to MAP-003.
- **Atoll/archipelago:** explore an ocean/coastal multi-tile landmark, with attention to sea/land transitions, AI colonisation, improvement rules and coast rendering; do not assume the native landmark renderer supports it.
- **Mount Planet / volcano split:** consider keeping a generic volcanic landmark and representing Mount Planet as an unusually high non-volcanic mountain or separate major. The identity, altitude generation, artwork, ocean cases and gameplay bonuses are unresolved.
- **Slots 3/4 replacements:** original Monsoon Jungle and Uranium Flats identities could be reused as majors in a future map design. Independent Jungle/Uranium already exist separately; native rendering currently still has legacy assumptions. See MAP-004.
- **Distinct regional incentives:** regions/landmarks could influence settlement and terraforming priorities without reverting to single-tile supply-pod-like rewards.
- **Sunny Mesa centre energy resource (unapproved):** could place one guaranteed normal +2 energy resource on its central tile, uniquely exempting the tile from the 3/3/3 energy cap. Current Mesa is restored to the 1,000–1,999 m 5×5 plateau after the failed 2,000 m bulge experiment (MAP-006). With its existing +1 landmark energy, a central energy resource would give 5 raw energy with Solar (absent other modifiers). **Not approved or implemented.**

## AI and developer experience

- **Candidate-improvement evaluator:** compare Forest/Farm/Mine/Solar/Borehole on actual future N/M/E, unlock techs, altitude, improvements, base needs and work turns. Could replace a collection of special-case Former heuristics after economic design stabilises.
- **Seeded visual/regression catalogue:** record a small number of representative map seeds and screenshots to check generation after refactors without committing saves. Develop counters for deposit placement/rejections may help.
- **Future release profile:** if the fork becomes redistributable, separate stable and diagnostic renderer settings and test a clean game installation first.

### Explicit non-ideas

Old-save/scenario compatibility is not a target. Strong Solar output by itself is not a bug. Changing `tile_output_limit` to 3/3/3 is **already an active trial** (BAL-001), not merely a hypothetical.


## Military chassis, artillery and unit economics — exploratory (BAL-004)

**Status: ideas only.** The existing mechanics are documented in [unit combat and costs](technical/unit-combat-and-costs.md). None of the following chassis bonuses, technology progression, artillery rules or cost formulas are implemented or agreed balance values. BUG-004 is a *separate existing correctness defect*, not an argument for a reactor-based HP bonus.

### Economic problem and intended roles

Current construction costs strongly reward high-weapon/low-armour units, particularly Impact and Missile Rovers. An armoured mobile unit can cost as much as two naked attackers, while a surviving durable unit can save replacement minerals, preserve experience and continue an offensive campaign. Analyse **single-battle mineral efficiency** separately from **campaign endurance and repair**.

| Chassis | Proposed specialisation |
| --- | --- |
| Infantry | Economical positional defence, garrisons and base assaults |
| Rover | Fast, replaceable mobile attacker, exploitation and raiding |
| Tank | Costly durable land unit designed to survive counterattacks and sustained offensives |
| Foil | Light patrol/interception/attack vessel |
| Cruiser | More expensive durable fleet unit, escort and transport support |

This does not assume the current Tank or Cruiser has extra HP. Existing mobile open-ground attack bonuses are **speed-based** and can apply to Tanks; making them Rover-only would require an explicit combat change.

### Possible land mobility progression

- Early Rover speed 2 (existing baseline).
- Unlock a conventional Tank earlier than the current Hovertank, possibly speed 2 with normal terrain movement.
- Later allow Rover speed 3, without granting early access to greatly increased Unity Pod exploration.
- Later allow Tank speed 2 **all-terrain** movement rather than simply giving it superior top speed.
- Determine technology prerequisites, coexistence with current chassis, pathfinding and AI valuation **before** any rule changes.

### Chassis endurance versus armour economics

Test **+1 and +2 effective HP** for Tanks and perhaps Cruisers **independently** of any cost reduction. In an idealised equal-strength 10-HP-versus-10-HP round-by-round battle, moving one participant to 11 HP changes its win chance from 50% to about 58.8%; 12 HP gives about 66.8%. Those are analytic illustrations, not runtime measurements.

The bonus supports repeated engagements and survival of expensive experienced units, but also improves **unarmoured** offensive Tanks. A naked 6-1 Tank should not simply replace a cheaper Rover. Test costs, movement, exposed counterattacks, repairs and AI design decisions, not win probability alone.

An **alternative**, not an automatic additional bonus, is removing/reducing the mobile combined weapon-and-armour cost surcharges for Tanks. Under the current Fission formula a 4-2 Impact Tank costs 70 minerals; a hypothetical removal of both surcharge increments would give 50. Compare this with a +HP Tank before stacking benefits.

Another **lower-priority alternative** is an armour-derived offensive Tank bonus (for example +5% per armour point above 1). This risks making armour a cheap surrogate for weapons at higher reactors, and introduces another specialist modifier. No coefficient is approved.

### Land and naval artillery-duel armour

Current artillery duels use weapon versus weapon, ignoring installed armour as the defender's ordinary resistance. This particularly undermines armoured Cruisers fighting cheap armed Foils at range, but also affects **land counterbattery artillery**.

Possible *symmetric* duel changes to investigate:
- Add a **fraction of armour above 1** to each participant's effective duel strength, with a cap or diminishing returns if needed.
- Make armour reduce incoming duel damage rather than modify per-round win probability.
- Consider a bounded counterbattery-protection bonus for armour-equipped artillery.

A possible mathematical **example only**, not a settled design: `duel strength = weapon + 0.5 × (armour - 1)`. Under it, 6-1 uses 6, 6-3 uses 7, and 6-8 uses 9.5 (before any chosen integer/fixed-point implementation). Test both combatants symmetrically, compare morale/altitude, and avoid changing ordinary direct attacks or air intercepts accidentally.

Naval armour is already cheaper in the existing cost formula, but ships lack forest/rocky or anti-Rover ECM defences. Qualifying sensors, AAA and port defences remain relevant; **these do not protect an ordinary open-sea weapon-only artillery duel through the standard armour path**. Before altering Cruiser HP or price, compare direct naval attacks, ship artillery duels, bombardment against transports and air threats. Extra HP can help duels without making armour itself useful, so it does not solve the whole problem.

### Other related construction ideas

- Investigate finer-than-ten-mineral unit price granularity only if the game and AI can consistently support it.
- Measure rounding plateaus and nominally free equipment upgrades across Fission/Fusion/Quantum/Singularity.
- Compare component-based costing models such as WTP's for inspiration, **without** importing their entire balance assumptions.
- Separately evaluate limited mineral overflow carryover or multi-unit production where cheap-unit spam causes production inefficiencies.
- Avoid altering these unrelated systems in the first chassis experiment.

### Investigation and test requirements

BAL-004 first needs a cost and win-rate comparison spanning weapon/armour combinations, reactors, terrain, artillery, ship/land roles and repair opportunities. Then select one minimal experimental rule set and verify actual gameplay, AI behaviour, same-build save/reload and adverse edge cases. See [WORK-QUEUE](WORK-QUEUE.md) for the single actionable entry.

## Native ecology and fungal forests — future exploration

**Ideas only; not approved implementation work.** This records the November 2026 *Landmark Ideas Comparison* discussion for later design. It does **not** add tasks to the work queue, change gameplay, or assert that the following systems exist. The intended **Fungal Forest** is a living native-ecology region, **not a major landmark**, not the independent Jungle minor terrain, and not the ordinary human-built Forest improvement. It should not consume a landmark slot.

### Desired experience

- Make Planet's fungus and wildlife feel like a **living, locally reactive ecosystem** rather than isolated random worm encounters. Its reaction should resemble an **immune response**: no humanlike strategy, faction recognition, omniscient revenge, or sapient decision to attack a particular colony.
- Have distinct **Nests**, **Fungal Forests**, and **Strongholds** with recognisable territorial depth. Illustrative, **unapproved** sizes discussed were a Nest of roughly 5–10 tiles with one tower, a Forest around 25–40 tiles with several towers, and a larger Stronghold with many towers. These are scale sketches, not generation constants.
- Differentiate fungal ground into **ordinary**, **mature**, and **dense/hardened** fungus. Dense growth would depend on surrounding fungal territory and towers; in extreme cases it might obstruct non-native movement while remaining traversable to native life. This requires explicit movement/pathfinding rules, not merely an art swap.
- Make alien forests **something the player can fight, contain, bypass, or exploit indirectly**. Conquering a region opens land, access and possibly existing resources or supply pods; no automatic new landmark-style tile yield is assumed.

### Local behavioural states (conceptual, not AI implemented)

| State | Possible behaviour |
| --- | --- |
| **Undisturbed** | Slow growth/maturation, low baseline native activity, limited defensive expenditure. |
| **Encroaching** | Territory expands at its frontier; mature fungus and towers may spread or reproduce; moderate native activity. |
| **Threatened** | Nearby destruction/clearing/combat creates local stress; the organism may shift resources away from expansion into defence, repair and native spawning. |

State should be **local to an ecological network/region**, not a single global “Planet is angry” flag. Attribution is deliberately crude: the ecosystem reacts to disturbances **regardless of which human faction caused them**. An army might intentionally stir wildlife in an opponent's territory, but the fungus would not know or care who initiated it.

### Finite ecology and native-life population

- Explore a **finite local biological reserve/biomass budget**, supported by fungal extent, maturity, towers and recovery. Expenditure competes among new fungal growth, thickening, tower reproduction, regeneration and native-unit spawning.
- **Native spawns, combat losses and captures** must be accounted for; killing or harvesting worms should not produce an endlessly replenished stream of attackers or easy captured units/planetpearls. How captured natives affect the wild budget remains open.
- Local stress could redirect a reserve into defence, but **destroying units or towers should impose a real cost** on the ecosystem. A depleted region should have a recovery period rather than infinite retaliation.
- **Perihelion** could accelerate growth, activity or stress responses, but its effect must respect the chosen reserve model; avoid free, unbounded spawning.
- Decide later whether regional networks are contiguous fungus, tower-linked clusters or a simpler bounded catchment. Save/reload of any new persistent state is essential, including continuing new games on the same build.

### Possible wildlife roles — not new unit commitments

- **Existing native units:** mind worms, spore launchers, locusts, Isles, sealurks and fungal towers could interact differently with regional ecology; preserve their existing roles unless deliberately redesigned.
- **Fungal Guardian (proposed):** slow, durable local defender or living barrier, especially near dense growth/towers.
- **Fungal Seeder (proposed):** slow, hard-to-stop native creature that plants/spreads fungus or helps a damaged network re-establish.
- **Spore artillery/defence (proposed role):** investigate whether the existing Spore Launcher already provides the desired ranged pressure before designing a duplicate new unit.
- No assumed graphics, executable hooks, costs, stats, production rules or AI controls for these hypothetical roles.

### How fighting a fungal forest might work

- **Formers as principal tools:** clear *ordinary perimeter fungus* to cut the surrounding network and weaken dense central growth; combat units escort and protect the Formers.
- Consider **Trance Formers/high-morale Formers** as specialist expedition assets, subject to actual unit-design and combat rules.
- Artillery and aircraft might attack towers, defenders or growth indirectly; feasibility depends on the real engine's unit/terrain attack and targeting systems. Do not assume terrain bombardment already works.
- Alternative strategies could include containment, clearing corridors, maintaining conventional Forest or open-land buffers, avoiding strongholds until better technology, or provoking an ecological response near an enemy. Risk and benefit must remain local and non-omniscient.
- Avoid turning ecology into a simple exponential death spiral that makes the map unplayable, especially on higher native-life settings or during Perihelion.

### Later design/research questions (not queued tasks)

- **Existing baseline:** inspect `src/gameturn.cpp` native spawning / `do_fungal_towers()`, `src/mapgen.cpp` fungus/tower placement, `src/veh.cpp` native lifecycle and pod events, native-unit counts, configuration and AI. The current branch already has random/event-based wild spawning and tower-driven fungus spread; this proposed territorial-budget system does **not** exist.
- **Map and simulation:** how to derive nest/forest/stronghold identity, maturity and network membership cheaply across map sizes; whether it emerges dynamically or receives generation-time seeds; what happens at coastlines, roads, bases, landmark/minor-terrain overlaps and destroyed/rebuilt fungus.
- **Budget accounting:** exact accounting for captured wild units, faction-built natives, cross-region movement, dormant units, native losses, towers, eco-damage events, unit limits and repeatable farming. Avoid spawning economies with an exploitable positive feedback loop.
- **AI:** when human and native AI should defend, clear, contain, flee or expand; pathfinding through hardened fungus; how Gaian/native-friendly factions interact without effortless free recruitment or immunity.
- **Combat/UI:** movement legality, artillery/air interactions, native psi combat, fog-of-war and visual clarity; whether existing art can distinguish maturity/density without breaking native draw ordering.
- **Validation if adopted later:** same-build save/reload, deterministic seed and budget tests, long-run ecological stability, speed on huge maps, balance across native-life settings, Perihelion and different faction playstyles. Preserve a way to disable experimental ecology for A/B comparison.

**Scope boundary:** Keep this as exploratory design until explicitly selected for a later project phase. Do not blend it into the ongoing PR #4 terrain-rendering acceptance work or put speculative subtasks in `WORK-QUEUE.md`.

## Empire Turn Report (UI-002) — future workflow, not present in PR #5

The native Empire Report (UI-001, [merged PR #5](https://github.com/Necaladun/thinkerNeca/pull/5)) is a manually opened **live-state** view. The desired **start-of-turn Turn Report** is distinct: finish all relevant faction-wide upkeep first, then present the **results and pending player decisions together**, instead of asking for sequential colony orders while energy and labs are still changing.

- Headline: **“X colonies finished production and need new orders.”** For each genuine completion, list the colony, exact item completed, the next queue/current order and a native **Change Production / Queue** action. It must handle 30 or more completions with independent scrolling, not rely on the current Stockpile Energy listing.
- Show opening/closing treasury, credited income/maintenance, research progress and breakthroughs, population changes, major incidents, riots/starvation, and actionable colony exceptions. Distinguish routine advisories such as habitation caps from urgent decisions.
- **Stockpile Energy** already serves as a native fallback for some unqueued facility/Secret Project completions; keep that behaviour while deferring later choices. A base intentionally stockpiling does *not* prove that it completed production this turn. Do not overwrite valid queued items or governor decisions.
- **Secret Project completed** is an exceptional notification. Offer **“View movie?”** using the existing playback route; current game preferences gate the automatic playback. The proposal must not suppress project rewards, council/victory effects, interludes or normal project completion rules.
- Implement in stages: **capture true events and snapshots without changing upkeep**, then distinguish safe routine notifications from mechanically significant decisions, then integrate an automatic reopenable post-upkeep report. The game's `production_phase() -> mod_base_upkeep()` processes colonies sequentially and can invoke native `BaseWin_zoom()` during processing; a mid-loop report would show partial totals. Guard same-build save/reload, eliminated bases, research breakthroughs, upkeep order and multiplayer/PBEM differences.

This design is deferred under [UI-002](WORK-QUEUE.md) and **does not hold up merging the currently functional manual Empire Report**.


## Further manual Empire Report interactions — exploratory

The source history of PR #5 discusses possible **contextual right-click actions** on base/unit rows, a missing-facility checklist, a separate orbital/satellite summary, and additional filters for stalled production, energy inefficiency, sanctions and unit support. These are **unapproved candidates**, not UI-001 acceptance requirements; existing native Change Production, Queue, Hurry, Governor and the current twelve report views must not be mistaken for unfinished work. Trends in population, economy, research or orbital investment would require explicit event/history capture and save/reload policy, potentially reusing the later TEL-* recorder. Preserve player-known fog-of-war information for any future threat/proximity display. UI-002's actual post-upkeep completion summary remains the separately deferred design above.
