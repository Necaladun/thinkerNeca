#!/usr/bin/env python3
"""Synthetic PCX template-mask regression test (no game artwork)."""
from pathlib import Path
import tempfile
from PIL import Image
from prepare_titanium_sprite import normalize


with tempfile.TemporaryDirectory() as directory:
    source = Path(directory) / 'source.pcx'
    result = Path(directory) / 'clean.pcx'
    pixels = Image.new('P', (384, 64), 253)
    pixels.putpalette([i for p in range(256) for i in (p, p, p)])
    for x, y, value in ((2, 3, 252), (20, 30, 254), (30, 40, 41),
                        (134, 8, 252), (278, 8, 254), (44, 2, 253)):
        pixels.putpixel((x, y), value)
    pixels.save(source)
    result_count = normalize(source, result)
    a, b = Image.open(source), Image.open(result)
    assert result_count == {252: 1, 254: 1}
    assert b.getpixel((2, 3)) == 253
    assert b.getpixel((20, 30)) == 253
    assert b.getpixel((30, 40)) == 41
    assert b.getpixel((134, 8)) == 252
    assert b.getpixel((278, 8)) == 254
    assert b.getpalette() == a.getpalette()
    assert b.size == a.size and b.format == 'PCX'
print('Titanium source palette, ore pixels, transparency and untouched slots verified')
