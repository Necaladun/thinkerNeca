#!/usr/bin/env python3
"""Exercise atlas generation without requiring or distributing game artwork."""
import subprocess
import sys
import tempfile
from pathlib import Path
from PIL import Image


def main() -> None:
    generator = Path('tools/make_minor_terrain.py')
    with tempfile.TemporaryDirectory() as tmp:
        src = Path(tmp) / 'ter1.pcx'
        out = Path(tmp) / 'minor_terrain.pcx'
        original = Image.new('P', (1024, 768), 253)
        original.putpalette([component for i in range(256) for component in
                             ((i*39) % 256, (i*67) % 256, (i*13) % 256)])
        for y in range(260, 306):
            for x in range(825, 911):
                original.putpixel((x, y), ((x + 13*y) % 245))
        original.save(src)
        subprocess.run([sys.executable, str(generator), str(src), str(out)], check=True)
        atlas = Image.open(out)
        assert atlas.mode == 'P', atlas.mode
        assert atlas.size == (384, 64), atlas.size
        assert atlas.getpixel((0, 0)) == 253
        for slot in range(3):
            pixels = atlas.crop((slot*128+12, 4, slot*128+116, 60))
            assert any(pixel != 253 for pixel in pixels.getdata()), slot
        print('Three sprites, indexed palette, dimensions and transparency verified')


if __name__ == '__main__':
    main()
