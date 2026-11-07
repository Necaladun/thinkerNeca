#!/usr/bin/env python3
"""Offline TEL-001 campaign report. Python standard library only.

Usage: python tools/campaign_report.py campaigns/<campaign-id>
       python tools/campaign_report.py campaigns/<campaign-id> --session session-001

Creates reports/<session-id>/index.html, factions.csv and bases.csv. It never
modifies game files or the recorder's JSONL streams.
"""
import argparse
import csv
import html
import json
from pathlib import Path
import sys

SCHEMA = 1
METRICS = (
    ("population", "Population"),
    ("bases", "Bases"),
    ("units", "Units"),
    ("credits", "Energy credits"),
    ("labs", "Research output"),
    ("energy_surplus", "Energy surplus"),
    ("nutrient_surplus", "Nutrient surplus"),
    ("maintenance", "Facility maintenance"),
    ("commerce", "Commerce"),
)
BASE_FIELDS = (
    "turn", "slot", "faction", "name", "x", "y", "population",
    "nutrient_intake", "mineral_intake", "energy_intake",
    "nutrient_surplus", "mineral_surplus", "energy_surplus",
    "labs", "economy", "psych", "eco_damage",
    "production_item", "mineral_stock", "rioting", "golden_age",
)


def read_jsonl(path):
    """Read complete records; ignore an uncommitted, truncated final line."""
    records = []
    with path.open("rb") as source:
        lines = source.readlines()
    for line_no, raw in enumerate(lines, 1):
        if not raw.strip():
            continue
        try:
            records.append(json.loads(raw.decode("utf-8")))
        except (UnicodeDecodeError, json.JSONDecodeError) as exc:
            if line_no == len(lines) and not raw.endswith(b"\n"):
                print(f"Warning: ignoring truncated final line in {path}:{line_no}", file=sys.stderr)
                continue
            raise ValueError(f"{path}:{line_no}: invalid JSON: {exc}") from exc
    return records

def load_session(session_dir):
    manifest = json.loads((session_dir / "manifest.json").read_text(encoding="utf-8"))
    status = json.loads((session_dir / "status.json").read_text(encoding="utf-8"))
    if manifest.get("schema") != SCHEMA or status.get("schema") != SCHEMA:
        raise ValueError("Unsupported telemetry schema in manifest/status")
    committed = status["last_committed_turn"]
    turns = read_jsonl(session_dir / "turns.jsonl")
    bases = read_jsonl(session_dir / "bases.jsonl")
    for kind, records in (("turns", turns), ("bases", bases)):
        seen = set()
        last = -1
        for item in records:
            if item.get("schema") != SCHEMA:
                raise ValueError(f"{kind}: unsupported record schema")
            if item.get("phase") != "completed_faction_cycle":
                raise ValueError(f"{kind}: unexpected phase")
            turn = item.get("turn")
            if not isinstance(turn, int) or turn <= last or turn in seen:
                raise ValueError(f"{kind}: duplicate or out-of-order turn {turn}")
            seen.add(turn)
            last = turn
    # status.json is written only after both data streams flush successfully.
    turns = [record for record in turns if record["turn"] <= committed]
    bases = [record for record in bases if record["turn"] <= committed]
    if [x["turn"] for x in turns] != [x["turn"] for x in bases]:
        raise ValueError("Missing or mismatched committed turn/base snapshots")
    for record in turns:
        faction_ids = [f["id"] for f in record["factions"]]
        if faction_ids != list(range(1, 8)):
            raise ValueError(f"Turn {record['turn']}: expected all 7 faction slots")
    return manifest, status, turns, bases


def write_csv(path, fields, rows):
    with path.open("w", newline="", encoding="utf-8") as target:
        writer = csv.DictWriter(target, fieldnames=fields, extrasaction="ignore")
        writer.writeheader()
        for row in rows:
            writer.writerow(row)


def render_html(campaign_name, session_name, manifest, status, turns, bases, notices=None):
    title = html.escape(f"{campaign_name} / {session_name}")
    last_turn = turns[-1]["turn"] if turns else "none"
    state = html.escape(str(status["state"]))
    data = json.dumps(turns, separators=(",", ":"), ensure_ascii=False).replace("<", "\\u003c")
    metrics = json.dumps(METRICS)
    latest = turns[-1]["factions"] if turns else []
    body_rows = "\n".join(
        "<tr><td>{id}</td><td>{name}</td><td>{bases}</td><td>{population}</td>"
        "<td>{units}</td><td>{credits}</td><td>{labs}</td></tr>".format(
            id=f["id"], name=html.escape(f["name"]),
            bases=f["bases"], population=f["population"],
            units=f["units"], credits=f["credits"], labs=f["labs"])
        for f in latest
    )
    notices = notices or []
    notice_rows = "\n".join(
        "<tr><td>{turn}</td><td>{faction}</td><td>{source}</td><td>{label}</td><td>{action}</td></tr>".format(
            turn=entry.get("turn", "?"), faction=entry.get("processing_faction", "?"),
            source=html.escape(str(entry.get("source", ""))),
            label=html.escape(str(entry.get("label", ""))),
            action=html.escape(str(entry.get("action", ""))))
        for entry in notices
    )
    if not notice_rows:
        notice_rows = '<tr><td colspan="5">No UI notices logged in this session</td></tr>'
    base_rows = ""
    if bases:
        for base in bases[-1]["bases"]:
            base_rows += "<tr><td>{}</td><td>{}</td><td>{}</td><td>{}</td><td>{}</td><td>{}</td></tr>\n".format(
                base["faction"], html.escape(base["name"]), base["population"],
                base["mineral_surplus"], base["labs"], base["eco_damage"]
            )
    return """<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>thinkerNeca Campaign Analyzer — %s</title>
<style>
:root{color-scheme:dark;font:15px system-ui,sans-serif}
body{max-width:1100px;margin:2rem auto;padding:0 1rem;background:#151922;color:#e5e7eb}
h1{font-size:1.7rem}h2{font-size:1.2rem;margin-top:2rem}
.muted{color:#aeb6c3}a{color:#93c5fd}select{padding:.4rem;color:inherit;background:#252d39;border:1px solid #546071;border-radius:5px}
.panel{background:#202733;padding:1.1rem;border-radius:10px;margin:1rem 0}
#chart{width:100%%;height:auto;background:#161c26;border-radius:7px}
#factions label{display:inline-block;margin:1rem .9rem .2rem 0}
table{border-collapse:collapse;width:100%%}td,th{text-align:left;padding:.5rem;border-bottom:1px solid #394251}
th{color:#bec9d8} .scroll{overflow-x:auto} 
</style></head><body>
<h1>Campaign Analyzer</h1>
<p><strong>%s</strong></p>
<p class="muted">Session status: %s · Completed snapshots: %d · Latest turn: %s · Starting turn: %s</p>
<div class="panel">
<label for="metric">Metric: </label><select id="metric"></select>
<div id="factions"></div>
<canvas id="chart" width="1000" height="440" aria-label="Faction metric history chart"></canvas>
</div>
<h2>Latest faction standings</h2><div class="scroll"><table><thead><tr>
<th>ID</th><th>Faction</th><th>Bases</th><th>Population</th><th>Units</th><th>Credits</th><th>Labs</th>
</tr></thead><tbody>%s</tbody></table></div>
<h2>Latest base snapshot</h2><div class="scroll"><table><thead><tr>
<th>Faction</th><th>Name</th><th>Population</th><th>Minerals (surplus)</th><th>Labs</th><th>Eco damage</th>
</tr></thead><tbody>%s</tbody></table></div>
<h2>AUTO-002 UI handling (%d recorded actions)</h2>
<p class="muted">Only explicitly classified notices and optional human UI are suppressed.
An unknown native modal can still interrupt simulation.</p>
<div class="scroll"><table><thead><tr>
<th>Turn</th><th>Processing faction</th><th>Source</th><th>Label</th><th>Handling</th>
</tr></thead><tbody>%s</tbody></table></div>
<p><a href="factions.csv">Export faction history CSV</a> · <a href="bases.csv">Export base history CSV</a> · <a href="ui_notices.csv">Export UI handling CSV</a></p>
<p class="muted">These are recorded states, not reconstructed events or proven AI motivations.
Engine base slots may be reused and are not persistent identities. Local/offline report.</p>
<script id="turn-data" type="application/json">%s</script>
<script>
const snapshots=JSON.parse(document.getElementById('turn-data').textContent);
const metrics=%s;
const colors=['#7bd389','#efce73','#8cabe8','#d49ce8','#f3a09c','#70cbd2','#d3d5da'];
const metric=document.getElementById('metric');
const checks=document.getElementById('factions');
for(const [key,label] of metrics){const opt=document.createElement('option');opt.value=key;opt.textContent=label;metric.appendChild(opt);}
const names=snapshots.length?snapshots[snapshots.length-1].factions:[];
for(const f of names){
 const label=document.createElement('label'),box=document.createElement('input');
 box.type='checkbox';box.value=f.id;box.checked=true;box.addEventListener('change',draw);
 label.append(box,document.createTextNode(' '+f.name));label.style.color=colors[f.id-1];checks.appendChild(label);
}
metric.addEventListener('change',draw);
function draw(){
 const canvas=document.getElementById('chart'),ctx=canvas.getContext('2d');
 const w=canvas.width,h=canvas.height,ml=72,mr=30,mt=28,mb=56;
 ctx.clearRect(0,0,w,h);ctx.fillStyle='#dce3ee';ctx.font='15px sans-serif';
 if(!snapshots.length){ctx.fillText('No committed turn snapshots',40,60);return;}
 const active=new Set([...checks.querySelectorAll('input:checked')].map(x=>Number(x.value)));
 const key=metric.value;
 const values=snapshots.flatMap(t=>t.factions.filter(f=>active.has(f.id)).map(f=>f[key]));
 let lower=Math.min(0,...values),upper=Math.max(1,...values);
 if(upper===lower)upper=lower+1;
 const xmin=snapshots[0].turn,xmax=Math.max(xmin+1,snapshots[snapshots.length-1].turn);
 const xp=x=>ml+(x-xmin)/(xmax-xmin)*(w-ml-mr);
 const yp=y=>h-mb-(y-lower)/(upper-lower)*(h-mt-mb);
 ctx.strokeStyle='#4e596a';ctx.lineWidth=1;ctx.beginPath();ctx.moveTo(ml,mt);ctx.lineTo(ml,h-mb);ctx.lineTo(w-mr,h-mb);ctx.stroke();
 for(let i=0;i<=4;i++){
   const v=lower+(upper-lower)*i/4,y=yp(v);
   ctx.fillText(Number(v.toFixed(1)).toLocaleString(),7,y+5);
   ctx.strokeStyle='#293545';ctx.beginPath();ctx.moveTo(ml,y);ctx.lineTo(w-mr,y);ctx.stroke();
 }
 ctx.fillText('Turn '+xmin,ml,h-20);ctx.fillText('Turn '+snapshots[snapshots.length-1].turn,w-130,h-20);
 for(let fc=1;fc<=7;fc++){
   if(!active.has(fc))continue;
   ctx.strokeStyle=colors[fc-1];ctx.lineWidth=2.5;ctx.beginPath();
   snapshots.forEach((t,i)=>{const v=t.factions[fc-1][key],x=xp(t.turn),y=yp(v);if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);});
   ctx.stroke();
 }
}
draw();
</script></body></html>
""" % (title, title, state, len(turns), last_turn, manifest["starting_turn"], body_rows, base_rows, len(notices), notice_rows, data, metrics)


def generate(campaign_dir, session_id=None, output_dir=None):
    session_root = campaign_dir / "sessions"
    sessions = sorted(path for path in session_root.iterdir() if path.is_dir())
    if not sessions:
        raise ValueError(f"No sessions in {session_root}")
    if session_id is None:
        if len(sessions) > 1:
            print("Multiple sessions found; defaulting to last directory. Use --session to select.", file=sys.stderr)
        session_dir = sessions[-1]
    else:
        session_dir = session_root / session_id
        if session_dir not in sessions:
            raise ValueError(f"Session not found: {session_id}")
    manifest, status, turns, bases = load_session(session_dir)
    output = output_dir or campaign_dir / "reports" / session_dir.name
    output.mkdir(parents=True, exist_ok=True)
    notices_file = session_dir / "ui_notices.jsonl"
    notices = read_jsonl(notices_file) if notices_file.is_file() else []
    for notice in notices:
        if notice.get("schema") != SCHEMA:
            raise ValueError("UI notice: unsupported record schema")
    write_csv(output / "ui_notices.csv",
              ("turn", "processing_faction", "source", "label", "action"), notices)
    faction_rows = (
        {"turn": record["turn"], "mission_year": record["mission_year"], **f}
        for record in turns for f in record["factions"]
    )
    faction_fields = ("turn", "mission_year", "id", "name", "alive", "human",
                      "bases", "population", "units", "credits", "labs",
                      "energy_surplus", "nutrient_surplus", "maintenance",
                      "commerce", "research_target", "research_progress",
                      "research_cost_stored")
    write_csv(output / "factions.csv", faction_fields, faction_rows)
    base_rows = ({"turn": record["turn"], **base} for record in bases for base in record["bases"])
    write_csv(output / "bases.csv", BASE_FIELDS, base_rows)
    (output / "index.html").write_text(
        render_html(campaign_dir.name, session_dir.name, manifest, status, turns, bases, notices),
        encoding="utf-8")
    return output



def _first_snapshot(path):
    """Read only the first complete JSONL record for fast GUI discovery."""
    with path.open("rb") as stream:
        for raw in stream:
            if not raw.strip():
                continue
            try:
                return json.loads(raw.decode("utf-8"))
            except (UnicodeError, json.JSONDecodeError) as exc:
                raise ValueError(f"{path}: invalid first snapshot: {exc}") from exc
    return None


def _last_committed_snapshot(path, committed_turn):
    """Read backwards in chunks; ignore an uncommitted partial last line.

    The UI only needs a compact preview, not the full multi-year log. A bound
    prevents scanning an enormous corrupted recording in the UI thread.
    """
    if committed_turn < 0:
        return None
    with path.open("rb") as stream:
        stream.seek(0, 2)
        remaining = stream.tell()
        tail = b""
        examined = 0
        while remaining and examined < 2 * 1024 * 1024:
            chunk = min(65536, remaining)
            remaining -= chunk
            examined += chunk
            stream.seek(remaining)
            tail = stream.read(chunk) + tail
            lines = tail.split(b"\n")
            # The leading line is incomplete until we reach offset zero.
            candidates = lines if remaining == 0 else lines[1:]
            for raw in reversed(candidates):
                if not raw.strip():
                    continue
                try:
                    record = json.loads(raw.decode("utf-8"))
                except (UnicodeError, json.JSONDecodeError):
                    continue  # Partial uncommitted tail or interrupted write.
                turn = record.get("turn")
                if (record.get("schema") == SCHEMA and isinstance(turn, int)
                        and turn <= committed_turn):
                    return record
            tail = lines[0]
    return None


def summarize_session(session_dir):
    """Metadata for the campaign picker. Never writes/changes recorder data.

    Returns a row even for malformed sessions so the GUI can show the fault.
    This does not validate every turn; generate() performs full validation.
    """
    campaign_dir = session_dir.parent.parent
    info = {
        "campaign_dir": campaign_dir,
        "session_id": session_dir.name,
        "folder": campaign_dir.name,
        "player": "Unknown",
        "years": "No snapshots",
        "turns": "0",
        "bases": "—",
        "units": "—",
        "status": "Unreadable",
        "started": "Unknown",
        "updated": "Unknown",
        "source_save": "Not recorded",
        "factions": "",
        "error": None,
    }
    try:
        manifest = json.loads((session_dir / "manifest.json").read_text(encoding="utf-8"))
        status = json.loads((session_dir / "status.json").read_text(encoding="utf-8"))
        if manifest.get("schema") != SCHEMA or status.get("schema") != SCHEMA:
            raise ValueError("Unsupported telemetry schema")
        committed = status.get("last_committed_turn")
        if not isinstance(committed, int):
            raise ValueError("Missing last_committed_turn")
        first = _first_snapshot(session_dir / "turns.jsonl")
        last = _last_committed_snapshot(session_dir / "turns.jsonl", committed)

        def local_time(value):
            if not value:
                return "Unknown"
            from datetime import datetime, timezone
            value = datetime.strptime(value, "%Y%m%dT%H%M%SZ")
            return value.replace(tzinfo=timezone.utc).astimezone().strftime("%d %b %Y %H:%M")

        info["started"] = local_time(manifest.get("started_utc"))
        info["updated"] = local_time(status.get("updated_utc"))
        info["source_save"] = manifest.get("source_save") or "Not recorded"
        info["status"] = str(status.get("state", "unknown")).replace("_", " ").title()
        if last:
            if first and first.get("turn", 999999999) <= committed:
                info["turns"] = str(last["turn"] - first["turn"] + 1) + " span"
                first_year = first.get("mission_year")
            else:
                info["turns"] = "Unknown"
                first_year = None
            last_year = last.get("mission_year")
            if first_year is not None and last_year is not None:
                info["years"] = f"MY {first_year}–{last_year}"
            else:
                info["years"] = f"Turn {last['turn']}"
            world = last.get("world", {})
            info["bases"] = str(world.get("base_count", "—"))
            info["units"] = str(world.get("vehicle_count", "—"))
            factions = last.get("factions", [])
            info["factions"] = ", ".join(
                faction.get("name", f"Faction {faction.get('id', '?')}")
                for faction in factions if faction.get("alive", False)
            )
            humans = [f.get("name", f"Faction {f.get('id', '?')}")
                      for f in factions if f.get("human")]
            info["player"] = ", ".join(humans) if humans else "All AI / observer"
        elif committed >= 0:
            raise ValueError("Committed turn not found in turns.jsonl")
        else:
            # A game can still be running before its first completed turn.
            info["player"] = "Awaiting first turn"
    except (OSError, ValueError, KeyError, TypeError) as exc:
        info["error"] = str(exc)
        info["status"] = "Error"
    return info


def scan_campaigns(root):
    """Discover recorded sessions below a campaigns/ directory."""
    root = Path(root)
    if not root.is_dir():
        return []
    rows = []
    for campaign_dir in root.iterdir():
        if not campaign_dir.is_dir():
            continue
        sessions_dir = campaign_dir / "sessions"
        if not sessions_dir.is_dir():
            continue
        for session_dir in sessions_dir.iterdir():
            if session_dir.is_dir():
                rows.append(summarize_session(session_dir))
    # Filesystem mtime is used for sorting only, never for game chronology.
    rows.sort(key=lambda item: (item["campaign_dir"] / "sessions"
              / item["session_id"] / "status.json").stat().st_mtime
              if (item["campaign_dir"] / "sessions" / item["session_id"]
                  / "status.json").exists() else -1, reverse=True)
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("campaign", type=Path, help="Campaign directory")
    parser.add_argument("--session", help="Session directory name (default: last session)")
    parser.add_argument("--output", type=Path, help="Destination directory")
    args = parser.parse_args()
    try:
        output = generate(args.campaign, args.session, args.output)
    except (OSError, ValueError, KeyError, TypeError) as exc:
        parser.exit(1, f"Campaign report error: {exc}\n")
    print(f"Created {output / 'index.html'}")


if __name__ == "__main__":
    main()
