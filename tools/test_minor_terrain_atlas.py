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
        native_iron = atlas.crop((12, 4, 116, 60))
        assert sum(px != 253 for px in native_iron.getdata()) > 1600, (
            'Native-source Ironstone should be opaque and distinguishable')
        print('Native-format source: three sprites, dimensions and transparency verified')

        # The repository's 128x64 legacy Ironstone asset is also a valid
        # source; no installed game artwork is needed for CI/dev previews.
        legacy = Path('assets/minor_terrain.pcx')
        if legacy.exists():
            fallback = Path(tmp) / 'minor_terrain_fallback.pcx'
            subprocess.run([sys.executable, str(generator), str(legacy),
                            str(fallback)], check=True)
            preview = Image.open(fallback)
            assert preview.mode == 'P' and preview.size == (384, 64)
            source_sprite = Image.open(legacy).crop((12, 4, 116, 60))
            iron_sprite = preview.crop((12, 4, 116, 60))
            assert iron_sprite.tobytes() != source_sprite.tobytes(), (
                'Fallback Ironstone must replace ghostly legacy artwork')
            opaque = sum(1 for pixel in iron_sprite.getdata() if pixel != 253)
            assert opaque > 1600, ('Ironstone outcrop is too sparse', opaque)
            assert iron_sprite.getpixel((0, 0)) == 253, (
                'Transparent border must be retained')
            for slot in range(3):
                subimage = preview.crop((slot*128+12, 4, slot*128+116, 60))
                assert any(pixel != 253 for pixel in subimage.getdata()), slot
            print('Legacy asset: regenerated solid Ironstone and valid atlas verified')


if __name__ == '__main__':
    main()
