#!/usr/bin/env python3
"""Synthetic host-side tests for TEL-001's offline report reader."""
import json
import subprocess
import sys
from pathlib import Path
import tempfile
import unittest

from campaign_report import generate, load_session, read_jsonl, summarize_session, scan_campaigns


def fixture(root, turns=(1, 2), incomplete=False):
    campaign = root / "campaign-test"
    session = campaign / "sessions" / "session-001"
    session.mkdir(parents=True)
    (session / "manifest.json").write_text(
        json.dumps({"schema": 1, "starting_turn": 0}), encoding="utf-8")
    (session / "status.json").write_text(
        json.dumps({"schema": 1, "state": "closed",
                    "last_committed_turn": turns[-1] if turns else -1}), encoding="utf-8")
    factions = [
        {"id": index, "name": f"Faction {index}", "bases": 1,
         "population": 2 + index, "units": 3, "credits": 50,
         "labs": 4, "energy_surplus": 2, "nutrient_surplus": 1,
         "maintenance": 0, "commerce": 0, "research_target": -1,
         "research_progress": 0, "research_cost_stored": 0,
         "alive": True, "human": index == 1}
        for index in range(1, 8)
    ]
    turn_lines = [
        json.dumps({"schema": 1, "phase": "completed_faction_cycle",
                    "turn": turn, "mission_year": 2100 + turn,
                    "world": {}, "factions": factions})
        for turn in turns
    ]
    base_lines = [
        json.dumps({"schema": 1, "phase": "completed_faction_cycle",
                    "turn": turn,
                    "bases": [{"slot": 0, "faction": 1, "name": "Alpha",
                               "x": 2, "y": 6, "population": 3,
                               "mineral_surplus": 2, "labs": 3, "eco_damage": 0}]})
        for turn in turns
    ]
    if incomplete:
        turn_lines.append('{"schema":1,"turn":')
    (session / "turns.jsonl").write_text("\n".join(turn_lines) + ("" if incomplete else "\n"), encoding="utf-8")
    (session / "bases.jsonl").write_text("\n".join(base_lines) + "\n", encoding="utf-8")
    return campaign, session


class CampaignReportTests(unittest.TestCase):
    def test_launcher_headless_scan_and_generate_cli(self):
        # Single launcher GUI consumes a JSON scan of the existing, unchanged
        # reporter metadata; generating a specific session remains CLI-only.
        with tempfile.TemporaryDirectory() as tmp:
            campaign, session = fixture(Path(tmp))
            script = Path(__file__).with_name("campaign_report.py")
            scan = subprocess.run(
                [sys.executable, str(script), "--scan-json", str(Path(tmp))],
                capture_output=True, text=True, check=True,
            )
            rows = json.loads(scan.stdout)
            self.assertEqual(len(rows), 1)
            self.assertEqual(rows[0]["campaign_dir"], str(campaign))
            self.assertEqual(rows[0]["session_id"], session.name)
            self.assertEqual(rows[0]["status"], "Closed")
            self.assertIsNone(rows[0]["error"])
            report = subprocess.run(
                [sys.executable, str(script), str(campaign),
                 "--session", session.name],
                capture_output=True, text=True, check=True,
            )
            self.assertIn("Created ", report.stdout)
            self.assertTrue((campaign / "reports" / session.name / "index.html").is_file())
            invalid = subprocess.run(
                [sys.executable, str(script), "--scan-json", str(Path(tmp)),
                 "--session", session.name],
                capture_output=True, text=True,
            )
            self.assertNotEqual(invalid.returncode, 0)
            self.assertIn("cannot be combined", invalid.stderr)


    def test_generate_complete_report(self):
        with tempfile.TemporaryDirectory() as tmp:
            campaign, _ = fixture(Path(tmp))
            output = generate(campaign)
            for name in ("index.html", "factions.csv", "bases.csv"):
                self.assertTrue((output / name).exists())
            html_text = (output / "index.html").read_text(encoding="utf-8")
            self.assertIn("Campaign Analyzer", html_text)
            self.assertIn("Faction 1", html_text)
            self.assertEqual(len((output / "factions.csv").read_text(encoding="utf-8").splitlines()), 15)

    def test_editable_name_and_multiple_sessions_share_one_campaign(self):
        with tempfile.TemporaryDirectory() as tmp:
            campaign, session1 = fixture(Path(tmp))
            (campaign / "campaign.json").write_text(json.dumps({
                "schema": 1, "campaign_id": campaign.name,
                "display_name": "Gaians - Long AI experiment",
                "created_utc": "20261108T090000Z",
            }), encoding="utf-8")
            # A resumed game writes a new session rather than overwriting
            # session-001 and can be selected separately by the reporter.
            session2 = campaign / "sessions" / "session-002"
            session2.mkdir()
            for file in ("manifest.json", "status.json", "turns.jsonl", "bases.jsonl"):
                (session2 / file).write_bytes((session1 / file).read_bytes())
            output = generate(campaign, session_id="session-002")
            content = (output / "index.html").read_text(encoding="utf-8")
            self.assertIn("Gaians - Long AI experiment / session-002", content)
            info = summarize_session(session1)
            self.assertEqual(info["campaign_name"], "Gaians - Long AI experiment")
            self.assertEqual(len(scan_campaigns(Path(tmp))), 2)
            self.assertTrue((session1 / "turns.jsonl").is_file())
            self.assertTrue((session2 / "turns.jsonl").is_file())

    def test_phase_timing_and_checkpoint_log_exports(self):
        with tempfile.TemporaryDirectory() as tmp:
            campaign, session = fixture(Path(tmp))
            timers = [
                {"schema": 1, "phase": "world_upkeep", "faction": 0,
                 "turn": 2, "mission_year": 2102,
                 "started_utc": "2026-11-09T00:00:00.001Z",
                 "ended_utc": "2026-11-09T00:00:00.021Z", "elapsed_ms": 20},
                {"schema": 1, "phase": "ai_turn", "faction": 2,
                 "turn": 2, "mission_year": 2102,
                 "started_utc": "2026-11-09T00:00:00.021Z",
                 "ended_utc": "2026-11-09T00:00:01.021Z", "elapsed_ms": 1000},
            ]
            checkpoints = [
                {"schema": 1, "turn": 2, "mission_year": 2102,
                 "utc": "2026-11-09T00:00:01.999Z",
                 "kind": "completed_cycle", "status": "saved",
                 "path": "campaign/saves/checkpoint-T00002-MY2102.sav",
                 "fingerprint": {"present": True, "bytes": 12345}},
            ]
            (session / "timings.jsonl").write_text(
                "".join(json.dumps(v) + "\n" for v in timers), encoding="utf-8")
            (session / "checkpoints.jsonl").write_text(
                "".join(json.dumps(v) + "\n" for v in checkpoints), encoding="utf-8")
            output = generate(campaign)
            self.assertEqual(len((output / "timings.csv").read_text().splitlines()), 3)
            self.assertEqual(len((output / "checkpoints.csv").read_text().splitlines()), 2)
            page = (output / "index.html").read_text(encoding="utf-8")
            self.assertIn("Native phase timings", page)
            self.assertIn("ai_turn", page)
            self.assertIn("1,000", page)
            self.assertIn("checkpoint-T00002-MY2102.sav", page)
            self.assertIn("timings.csv", page)

    def test_old_campaign_without_label_uses_folder_name(self):
        with tempfile.TemporaryDirectory() as tmp:
            campaign, session = fixture(Path(tmp))
            self.assertEqual(summarize_session(session)["campaign_name"], campaign.name)
            html_text = (generate(campaign) / "index.html").read_text(encoding="utf-8")
            self.assertIn(campaign.name, html_text)

    def test_duplicate_turns_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            _, session = fixture(Path(tmp), turns=(1, 1))
            with self.assertRaisesRegex(ValueError, "duplicate or out-of-order"):
                load_session(session)

    def test_missing_base_turn_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            _, session = fixture(Path(tmp))
            base_file = session / "bases.jsonl"
            base_file.write_text(base_file.read_text(encoding="utf-8").splitlines()[0] + "\n",
                                 encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "Missing or mismatched"):
                load_session(session)

    def test_schema_mismatch_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            _, session = fixture(Path(tmp))
            status = session / "status.json"
            doc = json.loads(status.read_text(encoding="utf-8"))
            doc["schema"] = 99
            status.write_text(json.dumps(doc), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "Unsupported"):
                load_session(session)

    def test_ui_notice_log_export_and_html(self):
        with tempfile.TemporaryDirectory() as tmp:
            campaign, session = fixture(Path(tmp))
            notices = [
                {"schema": 1, "turn": 2, "processing_faction": 3,
                 "source": "netmsg", "label": "GOODYRESOURCE",
                 "action": "suppressed_notice"},
                {"schema": 1, "turn": 2, "faction": 3,
                 "source": "x_pop", "label": "ASKSEEDESIGN",
                 "action": "declined_optional_human_ui"},
            ]
            (session / "ui_notices.jsonl").write_text(
                "\n".join(json.dumps(v) for v in notices) + "\n", encoding="utf-8")
            output = generate(campaign)
            import csv
            with (output / "ui_notices.csv").open(encoding="utf-8", newline="") as fp:
                rows = list(csv.DictReader(fp))
            self.assertEqual(len(rows), 2)
            self.assertEqual(rows[0]["label"], "GOODYRESOURCE")
            self.assertEqual(rows[1]["action"], "declined_optional_human_ui")
            html_text = (output / "index.html").read_text(encoding="utf-8")
            self.assertIn("AUTO-002 UI handling (2 recorded actions)", html_text)
            self.assertIn("GOODYRESOURCE", html_text)
            self.assertIn("ui_notices.csv", html_text)

    def test_legacy_session_without_ui_log_remains_supported(self):
        with tempfile.TemporaryDirectory() as tmp:
            campaign, _ = fixture(Path(tmp))
            output = generate(campaign)
            self.assertEqual(
                (output / "ui_notices.csv").read_text(encoding="utf-8").splitlines(),
                ["utc,turn,mission_year,processing_faction,source,label,action"],
            )

    def test_ui_notice_log_bad_schema_fails_report(self):
        with tempfile.TemporaryDirectory() as tmp:
            campaign, session = fixture(Path(tmp))
            (session / "ui_notices.jsonl").write_text(
                '{"schema":99,"turn":1}\n', encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "UI notice"):
                generate(campaign)

    def test_gui_summary_reads_human_year_range_and_session(self):
        with tempfile.TemporaryDirectory() as tmp:
            campaign, session = fixture(Path(tmp), turns=(12, 13))
            info = summarize_session(session)
            self.assertIsNone(info["error"])
            self.assertEqual(info["player"], "Faction 1")
            self.assertEqual(info["years"], "MY 2112–2113")
            self.assertEqual(info["turns"], "2 span")
            self.assertEqual(info["status"], "Closed")
            self.assertEqual(info["source_save"], "Not recorded")
            rows = scan_campaigns(Path(tmp))
            self.assertEqual(len(rows), 1)
            self.assertEqual(rows[0]["campaign_dir"], campaign)
            self.assertEqual(rows[0]["session_id"], "session-001")

    def test_gui_summary_respects_last_committed_turn(self):
        with tempfile.TemporaryDirectory() as tmp:
            _, session = fixture(Path(tmp), turns=(12, 13, 14))
            path = session / "status.json"
            doc = json.loads(path.read_text(encoding="utf-8"))
            doc["last_committed_turn"] = 13
            path.write_text(json.dumps(doc), encoding="utf-8")
            info = summarize_session(session)
            self.assertEqual(info["years"], "MY 2112–2113")
            self.assertEqual(info["turns"], "2 span")

    def test_gui_summary_handles_truncated_tail(self):
        with tempfile.TemporaryDirectory() as tmp:
            _, session = fixture(Path(tmp), turns=(12, 13), incomplete=True)
            info = summarize_session(session)
            self.assertIsNone(info["error"])
            self.assertEqual(info["years"], "MY 2112–2113")

    def test_gui_summary_reports_corrupt_session_without_crashing_list(self):
        with tempfile.TemporaryDirectory() as tmp:
            campaign, session = fixture(Path(tmp))
            (session / "status.json").write_text("bad JSON", encoding="utf-8")
            info = scan_campaigns(campaign.parent)
            self.assertEqual(len(info), 1)
            self.assertEqual(info[0]["status"], "Error")
            self.assertIn("Expecting value", info[0]["error"])

    def test_gui_summary_handles_empty_telemetry_session(self):
        with tempfile.TemporaryDirectory() as tmp:
            _, session = fixture(Path(tmp), turns=())
            row = summarize_session(session)
            self.assertIsNone(row["error"])
            self.assertEqual(row["player"], "Awaiting first turn")
            self.assertEqual(row["years"], "No snapshots")

    def test_trailing_incomplete_record_ignored(self):
        with tempfile.TemporaryDirectory() as tmp:
            _, session = fixture(Path(tmp), incomplete=True)
            self.assertEqual(len(read_jsonl(session / "turns.jsonl")), 2)
            self.assertEqual(len(load_session(session)[2]), 2)


if __name__ == "__main__":
    unittest.main()
