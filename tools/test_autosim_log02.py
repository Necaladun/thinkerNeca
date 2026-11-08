#!/usr/bin/env python3
"""TEL-LOG-02 source-level and synthetic renderer checks, not Windows evidence."""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
read = lambda name: (ROOT / name).read_text(encoding="utf-8")
base, veh, combat, tech, tel = [read("src/" + x + ".cpp")
    for x in ("base", "veh", "veh_combat", "tech", "telemetry")]
report = read("tools/campaign_report.py")

def check(label, ok):
    assert ok, label
    print("PASS", label)

for label in ("production", "units", "combat", "research"):
    check(label + " JSONL created", '"/' + label + '.jsonl"' in tel)
check("unit born after native veh_drop",
    "veh_drop(veh_id, x, y);\n    telemetry_unit_created(veh_id);" in veh)
check("unit removal before array memmove",
    veh.index("telemetry_unit_destroyed(veh_id);") <
    veh.index("memmove(&Vehs[veh_id]"))
check("session-scoped unit ID shifts on compaction",
    "tel02_vehicle_ids.erase(tel02_vehicle_ids.begin() + slot);" in tel
    and "telemetry_unit_baseline();" in tel)
check("actual unit construction rather than queued intention",
    base.index("if (veh_id < 0)") <
    base.index("telemetry_production_unit(*CurrentBaseID"))
check("facility construction after game mutation",
    base.index("set_fac(facility_id, *CurrentBaseID, 1);") <
    base.index("telemetry_production_facility(*CurrentBaseID"))
check("project completion still after source-owned project assignment",
    base.index("SecretProjects[project_id] = *CurrentBaseID;") <
    base.index("telemetry_production_facility(*CurrentBaseID"))
check("no combat records from odds-only path",
    combat.index("if (!option) { // Only evaluate battle odds") <
    combat.index("telemetry_combat_exchange("))
check("artillery records after actual applied damage",
    combat.index("veh_def->damage_taken += damage_value;") <
    combat.index('telemetry_combat_exchange(veh_id_atk, veh_id_def, "artillery"'))
check("direct HP exchange records after fight",
    combat.index("bool def_alive = veh_def_last_hp > 0;") <
    combat.index('telemetry_combat_exchange(veh_id_atk, veh_id_def, "direct_or_duel"'))
check("research grant after actual owner bit update",
    tech.index("TechOwners[tech_id] |= (1 << faction_id);") <
    tech.index("telemetry_research_acquired("))
check("tech metadata is not applied to native map/commlink/prototype branches",
    tech.index("if (tech_id >= Tech_ID_Commlink) {") <
    tech.index("telemetry_research_acquired("))
check("research target is observed with cycle-time qualification",
    'completed_faction_cycle' in tel and 'telemetry_research_reconcile();' in tel)
check("no new engine RNG or AI work in telemetry",
    "game_rand(" not in tel and "random_get(" not in tel
    and "tech_advance(" not in tel)
check("UI notice stream remains distinct", "ui_notices.jsonl" in tel
    and "tel02_row(" in tel)

sys.path.insert(0, str(ROOT / "tools"))
from campaign_report import render_tel02_streams, TEL02_FIELDS

empty = {name: [] for name in TEL02_FIELDS}
check("missing legacy streams safe",
    "none recorded" in render_tel02_streams(empty))
events = {name: [] for name in TEL02_FIELDS}
events["research"] = [{
    "event_id": "session-001-evt-2", "mission_year": 2130,
    "event": "tech_acquired", "tech_name": "<Victory>"
}]
events["production"] = [{
    "event_id": "session-001-evt-1", "mission_year": 2129,
    "event": "unit_completed", "unit_name": "Scout"
}]
html = render_tel02_streams(events)
check("HTML escapes native text", "&lt;Victory&gt;" in html and "<Victory>" not in html)
check("cross-stream event order", html.index("Scout") < html.index("&lt;Victory&gt;"))
print("TEL-LOG-02 static/synthetic checks complete. Native Windows unverified.")
