#!/usr/bin/env python3
"""Prepare a user's edited 384x64 Titanium sprite atlas for custom drawing.

Input: indexed PCX, slots Titanium/Jungle/Uranium (128x64 each).
SMAC/X ter1.pcx's native Uranium source includes atlas-template indices
252 (purple V) and 254 (cyan divider), which must not be painted by
the custom Sprite_extract_2(Sprite, ..., 0x1FD) path. Map both to
transparent index 253 only in Titanium's first slot.

All metallic-ore pixels, the original palette and the other two slots
are preserved exactly. This does not recolour or generate artwork.
"""
import argparse
from pathlib import Path
from PIL import Image


def normalize(source: Path, output: Path):
    original = Image.open(source)
    if original.format != 'PCX' or original.mode != 'P' or original.size != (384, 64):
        raise ValueError('Expected indexed 384x64 PCX minor_terrain.pcx')
    palette = original.getpalette()
    if not palette or len(palette) != 768:
        raise ValueError('Expected full 256-entry indexed palette')
    out = original.copy()
    pixels = out.load()
    changed = {252: 0, 254: 0}
    for y in range(64):
        for x in range(128):
            value = pixels[x, y]
            if value in changed:
                changed[value] += 1
                pixels[x, y] = 253
    out.putpalette(palette)
    output.parent.mkdir(parents=True, exist_ok=True)
    out.save(output, format='PCX')
    verified = Image.open(output)
    if verified.mode != 'P' or verified.size != original.size:
        raise ValueError('Output PCX failed structural verification')
    if verified.getpalette() != palette:
        raise ValueError('Output palette differs from input')
    for y in range(64):
        for x in range(384):
            old, new = original.getpixel((x, y)), verified.getpixel((x, y))
            expected = 253 if x < 128 and old in changed else old
            if new != expected:
                raise ValueError(f'Unexpected change at ({x},{y}): {old} -> {new}')
    return changed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    if args.source.resolve() == args.output.resolve():
        parser.error('Choose a different output path; retain original for comparison')
    changed = normalize(args.source, args.output)
    print(f'Prepared {args.output}: 252->253 {changed[252]} px, 254->253 {changed[254]} px')


if __name__ == '__main__':
    main()
