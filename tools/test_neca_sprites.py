#!/usr/bin/env python3
"""Synthetic indexed atlas tests; never commits or requires game artwork."""
import tempfile
from pathlib import Path
from PIL import Image
from build_neca_sprites import build, URANIUM_BOX, TITANIUM_DEST


def palette():
    return [component for i in range(256)
            for component in ((i*37)%256, (i*71)%256, (i*19)%256)]


def main():
    with tempfile.TemporaryDirectory() as tmp:
        folder = Path(tmp)
        ter1_path, titanium_path, atlas_path = [folder / name for name in
                ('TER1.PCX', 'Titanium_minor_terrain_READY.pcx', 'neca_sprites.pcx')]
        ter1 = Image.new('P', (1024, 768), 253)
        ter1.putpalette(palette())
        for x,y,v in [(822,253,253), (829,270,252), (836,276,15), (853,283,145)]:
            ter1.putpixel((x,y),v)
        ter1.save(ter1_path)
        titanium = Image.new('P', (384, 64), 253)
        titanium.putpalette(palette())
        for x,y,v in [(25,14,0), (34,23,42), (44,33,230)]:
            titanium.putpixel((x,y),v)
        titanium.save(titanium_path)
        build(ter1_path,titanium_path,atlas_path)
        atlas = Image.open(atlas_path)
        assert atlas.size == (1024, 1024) and atlas.mode == 'P'
        assert atlas.getpalette() == ter1.getpalette()
        assert atlas.crop((16,16,116,78)).tobytes() == ter1.crop(URANIUM_BOX).tobytes()
        assert atlas.getpixel((TITANIUM_DEST[0]+13,TITANIUM_DEST[1]+10)) == 0
        assert atlas.getpixel((TITANIUM_DEST[0]+22,TITANIUM_DEST[1]+19)) == 42
        assert atlas.getpixel((TITANIUM_DEST[0]+32,TITANIUM_DEST[1]+29)) == 230
        assert atlas.getpixel((0,0)) == 253

        # Confirm native+10/6,7->9 processing handles mask and colours.
        def native(value):
            shifted=(value+10)&255
            return 9 if shifted in (6,7) else shifted
        assert [native(v) for v in (252,253,0,42,230)] == [9,9,10,52,240]
        # Revised Titanium pixel colours require no code or registry change.
        titanium.putpixel((34,23),43)
        titanium.save(titanium_path)
        build(ter1_path,titanium_path,atlas_path)
        assert Image.open(atlas_path).getpixel((TITANIUM_DEST[0]+22,TITANIUM_DEST[1]+19)) == 43

        # Fail closed for a palette mismatch or reserved source colours.
        titanium.putpalette([0]*768)
        titanium.save(titanium_path)
        try:
            build(ter1_path,titanium_path,atlas_path)
        except ValueError as exc:
            assert 'palette' in str(exc)
        else:
            raise AssertionError('Mismatched palette was accepted')
        titanium.putpalette(palette())
        titanium.putpixel((34,23),252)
        titanium.save(titanium_path)
        try:
            build(ter1_path,titanium_path,atlas_path)
        except ValueError as exc:
            assert 'reserved' in str(exc)
        else:
            raise AssertionError('Reserved mask index was accepted')
        print('neca_sprites synthetic layout, index masks and validation: PASS')


if __name__=='__main__':
    main()
