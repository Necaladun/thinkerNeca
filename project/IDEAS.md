# Ideas and design explorations

**Not commitments.** This is a parking place for possible gameplay/engineering directions. If a proposal is accepted for investigation or implementation, create/advance a stable ID in [WORK-QUEUE](WORK-QUEUE.md). Record only actual decisions in [DECISIONS](DECISIONS.md).

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

## Empire Report — deferred Production actions

These are user-requested possibilities for a **later** UI iteration, not approved for implementation in the current artwork/appearance pass. The existing native base-name navigation, Hurry dialog and queue rules must continue to work.

- **Direct Change Production:** clicking the current build name, its pictogram/artwork, or a separate action could open the game's native Change Production picker **for the correct base without first entering the colony screen**. Decide the click target and verify native base context, modal return/focus and production queue effects before implementing. The icon need not become an implicit button unless that is later selected.
- **Next in Queue preview:** show the base's next queued production item alongside the current build, including a clear empty-queue state. Investigate the game's actual production-queue ordering and special cases (for example Stockpile Energy, queued projects, automation and queue updates) rather than assuming array index semantics. A preview alone must not alter the queue.

Promote either proposal to a concrete [UI-001 follow-up](WORK-QUEUE.md) or new work ID only when selected for implementation.

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

## Future Empire Report information (proposals only)

Requested: colour-coded values; Garrison facility indicators; eco damage, growth ETA/boom, habitation cap, maintenance and Golden Age; empire integrity/might, commerce, council votes, energy sliders, game year/perihelion and current Social Engineering; eventual Diplomacy standings, relationships, mood, votes and category leaders. Do not implement these yet.

Garrison facility indicators proposed: Perimeter Defense, Tachyon Field, Command Center, Naval Yard (sea training), Aerospace Complex, Bioenhancement Center and Geosynchronous Survey Pod (GSP). Growth display should distinguish normal growth, population boom, stagnation, starvation and Hab Complex/Dome caps.

Further candidates: energy inefficiency, stalled production, satellite totals, sanctions remaining, missing-facility filters and unit support. Historical trends require retained event history.

### Empire Report candidates confirmed for later evaluation (5 Nov)

- Prioritise visible energy inefficiency, stalled base production, sanctions remaining, home-base unit support burden, and each base's percentage contribution to faction economy, labs and psych. These are readouts/calculations over existing state, with costs and upkeep timing to be checked.
- Consider a missing-facility checklist/filter, without committing to its layout.
- Consider a dedicated Orbital view for existing satellite totals and associated effects.
- A Problems/Attention view could filter starving, rioting, stalled and eco-damaged bases, with optional severity sorting; it should not replace the ordinary base list.
- Personal empire trends are desired, but population/income/labs/minerals trend lines need turn-by-turn snapshots unless existing history can be verified. FactionTurnMight exists, but does not establish the availability of these other series. Future design must specify post-upkeep sampling, bounded retention and same-build save/reload persistence; do not implement history yet.
