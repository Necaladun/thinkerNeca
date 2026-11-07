#!/usr/bin/env python3
"""Synthetic host-side tests for TEL-001's offline report reader."""
import json
from pathlib import Path
import tempfile
import unittest

from campaign_report import generate, load_session, read_jsonl


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

    def test_trailing_incomplete_record_ignored(self):
        with tempfile.TemporaryDirectory() as tmp:
            _, session = fixture(Path(tmp), incomplete=True)
            self.assertEqual(len(read_jsonl(session / "turns.jsonl")), 2)
            self.assertEqual(len(load_session(session)[2]), 2)


if __name__ == "__main__":
    unittest.main()
