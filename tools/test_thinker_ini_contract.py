#!/usr/bin/env python3
"""Validate the distributed Thinker INI against this fork's option parser.

This tests configuration spelling/structure, NOT game balance or engine behavior.
"""
from collections import Counter
from pathlib import Path
import re


def main() -> None:
    parser = Path("src/main.cpp").read_text(encoding="utf-8")
    ini = Path("docs/thinker.ini").read_text(encoding="utf-8")

    accepted = set(re.findall(r'MATCH\("([^"]+)"\)', parser))
    entries = re.findall(r"(?m)^[ \t]*([A-Za-z_]\w*)[ \t]*=[ \t]*([^\r\n]*)", ini)
    assert ini.count("[thinker]") == 1, "Expected one [thinker] INI section"
    assert entries, "No active INI settings found"

    invalid = sorted({name for name, _ in entries} - accepted)
    assert not invalid, f"INI options not recognised by parser: {invalid}"

    multi_value = {"skip_event", "skip_faction", "skip_facility",
                   "faction_pair", "music_label", "script_label"}
    counts = Counter(name for name, _ in entries)
    duplicate = sorted(name for name, num in counts.items()
                       if num > 1 and name not in multi_value)
    assert not duplicate, f"Unexpected duplicated settings: {duplicate}"
    assert not re.search(r"(?m)^[ \t]*canyon[ \t]*=", ini), (
        "Retired Canyon generator must not reappear as an active INI option"
    )

    lists = {"spawn_free_units": 9, "tile_output_limit": 3,
             "world_sea_levels": 3, "cost_factor": 6,
             "tech_cost_factor": 6, "content_pop_player": 6,
             "content_pop_computer": 6}
    for name, length in lists.items():
        values = [v.split(";", 1)[0].strip() for k, v in entries if k == name]
        assert len(values) == 1, f"{name}: expected exactly one value"
        parts = [v.strip() for v in values[0].split(",")]
        assert len(parts) == length and all(re.fullmatch(r"-?\d+", v) for v in parts), (
            f"{name}: expected {length} integer values"
        )

    events = [int(v.split(";", 1)[0].strip())
              for k, v in entries if k == "skip_event"]
    assert len(events) == len(set(events)) and all(1 <= v <= 32 for v in events), (
        "Invalid or duplicated skip_event IDs"
    )
    print(f"Thinker INI contract passed: {len(entries)} entries, "
          f"{len(counts)} distinct recognised options")


if __name__ == "__main__":
    main()
