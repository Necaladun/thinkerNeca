#!/usr/bin/env python3
"""Regenerate a rust-red indexed PCX Ironstone placeholder from SMAC ter1.pcx.

Usage: python tools/make_minor_terrain.py path/to/ter1.pcx assets/minor_terrain.pcx
Requires Pillow. Source graphics are not redistributed in this repository.
"""
import sys
from pathlib import Path
from PIL import Image


def main() -> None:
    if len(sys.argv) != 3:
        sys.exit("Usage: make_minor_terrain.py <ter1.pcx> <output.pcx>")
    source, output = Path(sys.argv[1]), Path(sys.argv[2])
    image = Image.open(source)
    if image.mode != "P" or image.size != (1024, 768):
        sys.exit("Expected 1024x768 indexed ter1.pcx")
    palette = image.getpalette()
    palette_rgb = [tuple(palette[3 * i:3 * i + 3]) for i in range(256)]
    layer = image.crop((815, 256, 919, 312))
    key = 253
    replacements = {}
    for index in set(layer.getdata()):
        if index in (252, 253, 254, 255):
            replacements[index] = key
            continue
        red, green, blue = palette_rgb[index]
        light = 0.299 * red + 0.587 * green + 0.114 * blue
        target = (min(245, int(24 + 1.10 * light)),
                  min(152, int(4 + 0.43 * light)),
                  min(118, int(3 + 0.32 * light)))
        replacements[index] = min(range(250),
            key=lambda candidate: sum((palette_rgb[candidate][c] - target[c]) ** 2 for c in range(3)))
    sheet = Image.new("P", (128, 64), key)
    sheet.putpalette(palette)
    sheet.paste(layer.point([replacements.get(i, key) for i in range(256)]), (12, 4))
    output.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(output)
    print(f"Wrote {output} ({output.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
