#!/usr/bin/env python3
"""Static contract for Sunny Mesa's centre-only energy bonus.

Does not replace in-game N/M/E tests.
"""
from pathlib import Path


def main():
    yields = Path("src/map.cpp").read_text(encoding="utf-8")
    generator = Path("src/mapgen.cpp").read_text(encoding="utf-8")

    actual = (
        "(sq->landmarks & LM_MESA && sq->code_at() < 9)"
        "\n        || sq->landmarks & (LM_URANIUM|LM_GEOTHERMAL|LM_RIDGE)"
    )
    projected = "if (sq->landmarks & LM_MESA && sq->code_at() < 9) { E++; }"
    assert actual in yields, "Actual Mesa energy must be centre-only (codes 0-8)"
    assert projected in yields, "Projected Mesa energy must match actual centre"
    assert "|| sq->landmarks & (LM_URANIUM|LM_GEOTHERMAL|LM_RIDGE|LM_MESA)" not in yields, (
        "Mesa must not receive energy across its full footprint"
    )

    start = generator.index("void __cdecl mod_world_mesa(")
    end = generator.index("void __cdecl mod_world_ridge(", start)
    mesa = generator[start:end]
    assert "i < TableRange[3]" in mesa and "bit2_set(x2, y2, LM_MESA, true);" in mesa, (
        "Keep full Mesa terrain/landmark footprint"
    )
    assert "code_set(x2, y2, i);" in mesa, (
        "Mesa tile code must preserve central indices 0-8"
    )
    # TableRange[2] is the 5x5 footprint (25 tiles), not the 3x3 core.
    # Preserve the original 7x7 Mesa flag/code footprint while giving the
    # nine central tiles the 2000m band, the next sixteen the 1000m band,
    # and the outer twenty-four the 0m band.
    assert "int altitude = ALT_SHORE_LINE;" in mesa, (
        "Outer 7x7 Mesa rim must remain in shoreline altitude band"
    )
    assert "if (i < 9) {\n                altitude = ALT_TWO_ABOVE_SEA;" in mesa, (
        "Central 3x3 Mesa plateau must be raised to 2000-2999m"
    )
    assert "else if (i < TableRange[2]) {\n                altitude = ALT_ONE_ABOVE_SEA;" in mesa, (
        "Middle 5x5 Mesa ring must be at 1000-1999m"
    )
    assert "world_alt_set(x2, y2, altitude, true);" in mesa, (
        "Mesa must keep the native elevation/detail and slope adjustments"
    )
    assert "BIT_BONUS_RES" not in mesa and "BIT_ENERGY_RES" not in mesa, (
        "Experiment must not add a guaranteed resource bonus"
    )
    print("Sunny Mesa 3x3/5x5/7x7 elevations and centre-only energy contract passed")


if __name__ == "__main__":
    main()
