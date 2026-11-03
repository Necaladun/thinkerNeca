#!/usr/bin/env python3
"""Build a three-type PCX atlas with an opaque custom Ironstone outcrop.

Usage: python tools/make_minor_terrain.py <source.pcx> <minor_terrain.pcx>
Sources: installed SMAC ter1.pcx (1024x768), or legacy minor_terrain.pcx
(128x64). Requires Pillow. The original game artwork is not committed.

Layout: three 128x64 slots: Ironstone, Jungle, Uranium.
Each sprite uses slot + (12,4), size 104x56. Transparency index 253.
Ironstone is original, procedural high-contrast ore artwork drawn using
the installed game's palette (or fallback asset palette).
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
            # Only use the game's regular non-control palette entries:
            # 0 is shadow; 246-255 are Windows/control/transparency indices.
            nearest_cache[rgb] = min(range(10, 246), key=lambda index:
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


    def make_ironstone():
        # Geologically distinct angular ferruginous outcrops, not a sparse
        # recolour of Uranium. Indexed PCX remains entirely opaque wherever
        # rock is drawn; only the outside background is transparent 253.
        sprite = Image.new('P', (SPRITE_W, SPRITE_H), TRANSPARENT)
        sprite.putpalette(palette)
        d = ImageDraw.Draw(sprite)
        c = {name: palette_index(rgb) for name, rgb in {
            'shade': (40, 28, 25),
            'low': (84, 43, 31),
            'rust': (129, 64, 38),
            'mid': (179, 93, 53),
            'warm': (210, 134, 73),
            'bright': (244, 193, 117),
            'vein': (71, 79, 82),
            'veinlit': (184, 184, 156),
        }.items()}
        # Ground contact and shallow talus rather than an elevated icon.
        d.ellipse((10, 40, 93, 54), fill=c['shade'])
        d.polygon([(9, 47), (27, 42), (36, 47), (57, 42),
                   (79, 46), (96, 48), (88, 53), (19, 53)], fill=c['low'])
        # Left rock face.
        d.polygon([(14, 44), (24, 25), (35, 21), (46, 29),
                   (48, 42), (39, 49), (19, 48)], fill=c['shade'])
        d.polygon([(19, 42), (26, 27), (35, 23), (39, 31),
                   (36, 43)], fill=c['rust'])
        d.polygon([(35, 23), (43, 30), (46, 40), (36, 43),
                   (39, 31)], fill=c['mid'])
        d.line([(24, 28), (27, 33), (24, 41)], fill=c['warm'], width=2)
        # Tall central ironstone ridge with a dark mineral seam.
        d.polygon([(31, 46), (39, 22), (54, 12), (65, 20),
                   (69, 39), (57, 50), (40, 50)], fill=c['shade'])
        d.polygon([(39, 42), (44, 24), (54, 16), (58, 29),
                   (54, 44), (44, 49)], fill=c['rust'])
        d.polygon([(54, 16), (62, 23), (66, 39), (57, 47),
                   (54, 44), (58, 29)], fill=c['mid'])
        d.polygon([(45, 25), (53, 17), (51, 32),
                   (44, 41)], fill=c['warm'])
        d.line([(54, 19), (58, 29), (54, 44)], fill=c['bright'], width=2)
        d.line([(40, 45), (45, 36), (49, 41), (51, 49)],
               fill=c['vein'], width=3)
        d.line([(45, 36), (49, 41)], fill=c['veinlit'], width=1)
        # Right-hand outcrop and ferric highlights.
        d.polygon([(62, 46), (67, 30), (79, 25), (91, 34),
                   (92, 46), (84, 51), (69, 50)], fill=c['shade'])
        d.polygon([(68, 44), (70, 33), (79, 28), (83, 37),
                   (79, 47)], fill=c['rust'])
        d.polygon([(79, 28), (88, 35), (89, 44), (80, 48),
                   (83, 37)], fill=c['mid'])
        d.line([(74, 35), (80, 31)], fill=c['bright'], width=2)
        d.line([(25, 47), (38, 44), (51, 48), (65, 46),
                (77, 48)], fill=c['vein'], width=3)
        d.line([(35, 43), (46, 46), (52, 45)],
               fill=c['veinlit'], width=1)
        for point in [(16, 49), (30, 51), (60, 51), (87, 49)]:
            d.point(point, fill=c['bright'])
        return sprite

    if native_source:
        # Native Uranium art from the user's local installation.
        uranium = original.crop((815, 256, 919, 312)).point(clean)
        ironstone = make_ironstone()
    else:
        # Generate the same solid Ironstone in CI from the palette of
        # the legacy asset; tint legacy art only for fallback Uranium.
        ironstone = make_ironstone()
        legacy = original.crop((12, 4, 116, 60)).point(clean)
        uranium = tint(legacy, ((15, 0.55), (35, 0.94), (6, 0.37)))

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
