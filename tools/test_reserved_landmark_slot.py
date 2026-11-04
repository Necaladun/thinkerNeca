#!/usr/bin/env python3
"""Static guard for retired Nessus Canyon slot. Not an executable/rendering test."""
from pathlib import Path
import re


def read(path):
    encoding = "cp1252" if path == "docs/german/alphax.txt" else "utf-8"
    return Path(path).read_text(encoding=encoding)


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def main():
    enums = read("src/engine_enums.h")
    config = read("src/main.h")
    parser = read("src/main.cpp")
    mapgen = read("src/mapgen.cpp")
    mapgen_header = read("src/mapgen.h")
    yields = read("src/map.cpp")
    patches = read("src/patch.cpp")
    ini = read("docs/thinker.ini")

    require("LM_RESERVED_10 = 0x200" in enums,
            "Slot 10 must preserve its native 0x200 bit")
    require("int reserved_10 = 0;" in config,
            "Major-landmark configuration must retain an unused slot")
    require("LM_CANYON" not in "\n".join(
        (enums, mapgen, mapgen_header, yields, patches)),
        "Old LM_CANYON semantics must not remain in active source")
    require("mod_world_canyon" not in mapgen + mapgen_header
            and "WorldCanyonOffset" not in mapgen,
            "Retired Canyon generator must be removed")
    require("lm.canyon" not in mapgen and "landmarks.canyon" not in parser
            and 'MATCH("canyon")' not in parser,
            "Old Canyon configuration and world-builder dispatch must be removed")
    require(re.search(r"^\s*canyon\s*=", ini, re.M) is None,
            "Obsolete canyon= INI option must not remain")
    require(re.search(r"^\s*remove_call\(0x5C8903\);", patches, re.M)
            is not None,
            "Native WorldBuilder Canyon call must be disabled unconditionally")
    require("LM_RESERVED_10" not in mapgen + yields,
            "Reserved slot must not be generated or used for resource yields")

    for path in ("docs/alphax.txt", "docs/german/alphax.txt",
                 "docs/smac_mod/alphax.txt"):
        lines = read(path).splitlines()
        require(lines.count("#NATURAL") == 1,
                f"{path}: expected exactly one #NATURAL table")
        index = lines.index("#NATURAL")
        entries = lines[index + 1:index + 17]
        require(len(entries) == 16 and all("," in row for row in entries),
                f"{path}: preserve all sixteen native landmark entries")
        require(entries[9].strip() == "Reserved Landmark 10, Reserved",
                f"{path}: the tenth landmark must have a neutral placeholder")
        require(entries[8].startswith("Sunny Mesa,")
                and entries[10].startswith("Geothermal Shallows,"),
                f"{path}: landmark slot order must remain unchanged")

    print("Reserved major-landmark slot 10 source invariants passed")


if __name__ == "__main__":
    main()
