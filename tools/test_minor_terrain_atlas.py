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
        native_titanium = atlas.crop((12, 4, 116, 60))
        native_uranium = atlas.crop((2*128+12, 4, 2*128+116, 60))
        assert native_titanium.tobytes() == native_uranium.tobytes(), (
            'Titanium placeholder must exactly match Uranium art')
        print('Native-format source: three sprites, dimensions and transparency verified')

        # The repository's 128x64 legacy Titanium asset is also a valid
        # source; no installed game artwork is needed for CI/dev previews.
        legacy = Path('assets/minor_terrain.pcx')
        if legacy.exists():
            fallback = Path(tmp) / 'minor_terrain_fallback.pcx'
            subprocess.run([sys.executable, str(generator), str(legacy),
                            str(fallback)], check=True)
            preview = Image.open(fallback)
            assert preview.mode == 'P' and preview.size == (384, 64)
            source_sprite = Image.open(legacy).crop((12, 4, 116, 60))
            titanium_sprite = preview.crop((12, 4, 116, 60))
            assert titanium_sprite.tobytes() != source_sprite.tobytes(), (
                'Fallback Titanium must replace ghostly legacy artwork')
            opaque = sum(1 for pixel in titanium_sprite.getdata() if pixel != 253)
            assert opaque > 0, ('Titanium placeholder is empty', opaque)
            uranium_sprite = preview.crop((2*128+12, 4, 2*128+116, 60))
            assert titanium_sprite.tobytes() == uranium_sprite.tobytes(), (
                'Fallback Titanium must copy Uranium sprite exactly')
            assert titanium_sprite.getpixel((0, 0)) == 253, (
                'Transparent border must be retained')
            for slot in range(3):
                subimage = preview.crop((slot*128+12, 4, slot*128+116, 60))
                assert any(pixel != 253 for pixel in subimage.getdata()), slot
            print('Legacy asset: regenerated solid Titanium and valid atlas verified')


if __name__ == '__main__':
    main()
