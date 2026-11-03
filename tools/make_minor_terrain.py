#!/usr/bin/env python3
"""Build a three-type PCX atlas from game art or the existing Ironstone asset.

Usage: python tools/make_minor_terrain.py <source.pcx> <minor_terrain.pcx>
Sources: installed SMAC ter1.pcx (1024x768), or legacy minor_terrain.pcx
(128x64). Requires Pillow. The original game artwork is not committed.

Layout: three 128x64 slots: Ironstone, Jungle, Uranium.
Each sprite uses slot + (12,4), size 104x56. Transparency index 253.
"""
import random
import sys
from pathlib import Path
from PIL import Image, ImageDraw

SPRITE_W, SPRITE_H = 104, 56
SLOT_W, SLOT_H = 128, 64
TRANSPARENT = 253


def main() -> None:
    if len(sys.argv) != 3:
        sys.exit('Usage: make_minor_terrain.py <ter1.pcx|legacy minor_terrain.pcx> <output.pcx>')
    source, output = Path(sys.argv[1]), Path(sys.argv[2])
    original = Image.open(source)
    if original.mode != 'P' or original.size not in ((1024, 768), (128, 64)):
        sys.exit('Expected 1024x768 ter1.pcx or 128x64 legacy minor_terrain.pcx')
    native_source = original.size == (1024, 768)

    palette = original.getpalette()
    if not palette or len(palette) < 768:
        sys.exit('Expected a 256-entry indexed palette')
    colors = [tuple(palette[3*i:3*i+3]) for i in range(256)]
    nearest_cache = {}

    def palette_index(rgb):
        if rgb not in nearest_cache:
            # Exclude game transparency/control entries 250-255.
            nearest_cache[rgb] = min(range(250), key=lambda index:
                sum((colors[index][j] - rgb[j]) ** 2 for j in range(3)))
        return nearest_cache[rgb]

    clean = list(range(256))
    for index in (252, 253, 254, 255):
        clean[index] = TRANSPARENT

    def tint(sprite, colour):
        # Map pixel luminance to the source palette while preserving
        # transparency. This creates distinguishable *test* art only.
        recolor = []
        for index in range(256):
            if index in (252, 253, 254, 255):
                recolor.append(TRANSPARENT)
            else:
                red, green, blue = colors[index]
                light = 0.299*red + 0.587*green + 0.114*blue
                recolor.append(palette_index(tuple(
                    max(0, min(245, int(base + multiplier*light)))
                    for base, multiplier in colour)))
        return sprite.point(recolor)

    if native_source:
        # Native Uranium art from the user's local installation.
        uranium = original.crop((815, 256, 919, 312)).point(clean)
        ironstone = tint(original.crop((815, 256, 919, 312)),
                         ((24, 1.10), (4, 0.43), (3, 0.32)))
    else:
        # Existing Ironstone art lives in the repository; no game art is
        # extracted or redistributed. Fallback Uranium is a provisional
        # yellow-green recolour, NOT the native graphic.
        ironstone = original.crop((12, 4, 116, 60)).point(clean)
        uranium = tint(ironstone, ((15, 0.55), (35, 0.94), (6, 0.37)))

    # Provisional canopy marker; this is NOT extracted native Jungle art.
    # Transparent gaps leave much of the normal terrain visible.
    jungle = Image.new('P', (SPRITE_W, SPRITE_H), TRANSPARENT)
    jungle.putpalette(palette)
    d = ImageDraw.Draw(jungle)
    rng = random.Random(0x4A554E47)
    ground = palette_index((47, 74, 33))
    shadow = palette_index((22, 51, 25))
    foliage = [palette_index(c) for c in
               [(39, 85, 38), (52, 103, 42), (68, 115, 52), (86, 129, 58)]]
    light = palette_index((126, 150, 69))
    for index in range(17):
        x = rng.randint(13, 92)
        y = rng.randint(11, 43)
        radius_x = rng.randint(5, 11)
        radius_y = rng.randint(3, 6)
        d.ellipse((x-radius_x, y-radius_y+2, x+radius_x, y+radius_y+2), fill=shadow)
        d.ellipse((x-radius_x, y-radius_y, x+radius_x, y+radius_y),
                  fill=foliage[index % len(foliage)], outline=ground)
        d.ellipse((x-2, y-radius_y+1, x+3, y-1), fill=light)
    for _ in range(38):
        x = rng.randint(10, 96)
        y = rng.randint(10, 47)
        d.point((x, y), fill=foliage[rng.randrange(len(foliage))])

    atlas = Image.new('P', (3*SLOT_W, SLOT_H), TRANSPARENT)
    atlas.putpalette(palette)
    for slot, sprite in enumerate((ironstone, jungle, uranium)):
        atlas.paste(sprite, (slot*SLOT_W+12, 4))
    output.parent.mkdir(parents=True, exist_ok=True)
    atlas.save(output)
    print(f'Wrote {output}: {atlas.width}x{atlas.height} ({output.stat().st_size} bytes)')


if __name__ == '__main__':
    main()
