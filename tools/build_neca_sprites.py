#!/usr/bin/env python3
"""Assemble local indexed PCX sources into the thinkerNeca diagnostic atlas.

Usage: python tools/build_neca_sprites.py TER1.PCX Titanium_minor_terrain_READY.pcx neca_sprites.pcx

This deliberately requires user-provided SMAC artwork. Do not commit its
output: the built atlas contains copyrighted original-game Uranium pixels.
The Titanium source is the 384x64 prepared indexed sheet, not a generic RGB.

Current layout (1024x1024, raw TER1 palette indices):
  uranium_test:  (16,16,100,62), from TER1 (822,253,100,62)
  titanium_test: (144,16,100,62), prepared sprite crop (12,4,104,56)
                 pasted at sheet coordinates (155,19); transparent border
                 maintains the verified tile alignment.
"""
import argparse
from pathlib import Path
from PIL import Image

ATLAS_SIZE = (1024, 1024)
TRANSPARENT_SOURCE = 253
URANIUM_BOX = (822, 253, 922, 315)
URANIUM_DEST = (16, 16)
TITANIUM_BOX = (12, 4, 116, 60)
TITANIUM_DEST = (155, 19)


def load_indexed(path: Path, size: tuple[int, int]) -> Image.Image:
    image = Image.open(path)
    if image.format != 'PCX' or image.mode != 'P' or image.size != size:
        raise ValueError(f'{path}: expected {size} 8-bit indexed PCX')
    if len(image.getpalette() or []) != 768:
        raise ValueError(f'{path}: expected complete 256-colour palette')
    return image


def build(ter1_path: Path, titanium_path: Path, output_path: Path) -> None:
    if output_path.resolve() in (ter1_path.resolve(), titanium_path.resolve()):
        raise ValueError('Output must differ from source art')
    ter1 = load_indexed(ter1_path, (1024, 768))
    titanium = load_indexed(titanium_path, (384, 64))
    if titanium.getpalette() != ter1.getpalette():
        raise ValueError('Titanium source palette differs from TER1; do not silently quantize')

    sample = titanium.crop(TITANIUM_BOX)
    pixels = list(sample.tobytes())
    visible = [value for value in pixels if value != TRANSPARENT_SOURCE]
    if not visible:
        raise ValueError('Titanium sprite has no visible pixels')
    invalid = sorted({value for value in visible if value > 235})
    if invalid:
        raise ValueError(f'Titanium uses reserved source indices: {invalid}')
    if sample.getpixel((0, 0)) != TRANSPARENT_SOURCE:
        raise ValueError('Titanium crop must have a transparent index-253 corner')

    atlas = Image.new('P', ATLAS_SIZE, TRANSPARENT_SOURCE)
    atlas.putpalette(ter1.getpalette())
    uranium = ter1.crop(URANIUM_BOX)
    atlas.paste(uranium, URANIUM_DEST)
    atlas.paste(sample, TITANIUM_DEST)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    atlas.save(output_path, format='PCX')
    verified = load_indexed(output_path, ATLAS_SIZE)
    if verified.getpalette() != atlas.getpalette() or verified.tobytes() != atlas.tobytes():
        raise ValueError('PCX save/reload changed indexed pixels or palette')
    print(f'Built {output_path} ({ATLAS_SIZE[0]}x{ATLAS_SIZE[1]}): '
          f'{len(visible)} visible Titanium pixels, '
          f'{len(pixels)-len(visible)} index-253 mask pixels')


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('ter1', type=Path)
    parser.add_argument('titanium', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    build(args.ter1, args.titanium, args.output)


if __name__ == '__main__':
    main()
