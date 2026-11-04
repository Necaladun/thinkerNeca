# Ideas and design explorations

**Not commitments.** This is a parking place for possible gameplay/engineering directions. If a proposal is accepted for investigation or implementation, create/advance a stable ID in [WORK-QUEUE](WORK-QUEUE.md). Record only actual decisions in [DECISIONS](DECISIONS.md).

## Terraforming and terrain economics

- **Less universal Forests:** reduce Forest from 1 nutrient / 2 minerals / 1 energy to 1/1/1 to increase the relevance of dedicated Farms, Mines and Solar Collectors. Needs evaluation alongside Tree Farms, upkeep, terraforming turns and Thinker AI choices. See BAL-002.
- **Geology requiring extraction:** Ironstone provides +1 mineral only with Mine; Uranium provides +1 energy with an appropriate improvement (Mine was suggested). Would make regions resource-specialised instead of all-purpose Forest terrain. Open cases: nonrocky mine legality, boreholes, base tiles, fungus, regular resource bonuses, crawler strategies. See BAL-003.
- **Solar still powerful:** keep elevation scaling as a feature, but teach Formers to build Solars at worthwhile elevations. Contrast projectable yields, worker demand and Former-turn opportunity costs. See AI-001.
- **More minor terrain features:** potential environmental/geological patches, but only once existing types have mature visuals, yield rules and balancing.

## Major landmarks

- **Slot 10 replacement:** now that Nessus Canyon is retired, design an attractive major landmark that does not inherit Canyon's broken visuals. Options range from geographic formations using existing terrain to unique mechanics requiring renderer hooks. See MAP-003.
- **Slots 3/4 replacements:** original Monsoon Jungle and Uranium Flats identities could be reused as majors in a future map design. Independent Jungle/Uranium already exist separately; native rendering currently still has legacy assumptions. See MAP-004.
- **Distinct regional incentives:** regions/landmarks could influence settlement and terraforming priorities without reverting to single-tile supply-pod-like rewards.
- **Sunny Mesa centre energy resource (unapproved):** could place one guaranteed normal +2 energy resource on its central tile, uniquely exempting the tile from the 3/3/3 energy cap. Current Mesa is restored to the 1,000–1,999 m 5×5 plateau after the failed 2,000 m bulge experiment (MAP-006). With its existing +1 landmark energy, a central energy resource would give 5 raw energy with Solar (absent other modifiers). **Not approved or implemented.**

## AI and developer experience

- **Candidate-improvement evaluator:** compare Forest/Farm/Mine/Solar/Borehole on actual future N/M/E, unlock techs, altitude, improvements, base needs and work turns. Could replace a collection of special-case Former heuristics after economic design stabilises.
- **Seeded visual/regression catalogue:** record a small number of representative map seeds and screenshots to check generation after refactors without committing saves. Develop counters for deposit placement/rejections may help.
- **Future release profile:** if the fork becomes redistributable, separate stable and diagnostic renderer settings and test a clean game installation first.

### Explicit non-ideas

Old-save/scenario compatibility is not a target. Strong Solar output by itself is not a bug. Changing `tile_output_limit` to 3/3/3 is **already an active trial** (BAL-001), not merely a hypothetical.
