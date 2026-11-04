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
    assert "(i < TableRange[2]) + ALT_SHORE_LINE" in mesa, (
        "Keep the existing elevated Mesa centre"
    )
    print("Sunny Mesa centre-only energy source contract passed")


if __name__ == "__main__":
    main()
