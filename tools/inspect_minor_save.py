#!/usr/bin/env python3
"""Inspect THINKER's independent multi-feature THMIN001 v3 save layer.

Does not calculate full engine resource output or require a game install.
Usage: python tools/inspect_minor_save.py game.SAV 133,65 90,30
"""
import argparse
from collections import Counter
from pathlib import Path
import struct

MAGIC = b'THMIN001'
VERSION = 3
HEADER_SIZE = 0xAA4
MAP_TILE_BYTES = 44
NAMES = ('None', 'Jungle', 'Uranium', 'Titanium')
NEIGHBOURS = ((2, 0), (-2, 0), (0, 2), (0, -2),
              (1, 1), (-1, 1), (1, -1), (-1, -1))


def read_save(raw):
    offset = raw.find(MAGIC)
    if offset < 0:
        raise ValueError('No independent minor-terrain THMIN001 extension')
    if len(raw) < offset + 16:
        raise ValueError('Truncated minor-terrain extension header')
    version, count = struct.unpack_from('<II', raw, offset + 8)
    if version != VERSION:
        raise ValueError(f'Unsupported minor-terrain version {version} (expected {VERSION})')
    if count <= 0 or count > 10000000:
        raise ValueError('Invalid map tile count')

    # Original native map format: 0xAA4 metadata + 44 bytes per tile
    # + compact abstract map, followed by independent THMIN001.
    dimensions = []
    for half_width in range(1, count + 1):
        if count % half_width:
            continue
        width, height = 2*half_width, count // half_width
        if not (8 <= width <= 4096 and 8 <= height <= 4096):
            continue
        ax, ay = (width + 4)//5, (height + 4)//5
        abstract_size = ay*((ax + 1)//2)
        header = offset - abstract_size - count*MAP_TILE_BYTES - HEADER_SIZE
        if header < 0 or header + 8 > len(raw):
            continue
        if struct.unpack_from('<ii', raw, header) == (width, height):
            dimensions.append((width, height, header))
    if len(dimensions) != 1:
        raise ValueError(f'Could not uniquely identify native map dimensions: {dimensions}')
    width, height, header = dimensions[0]
    map_start = header + HEADER_SIZE

    pos = offset + 16
    tiles = {}
    for i in range(count):
        if pos >= len(raw):
            raise ValueError(f'Truncated minor tile entry {i}')
        n = raw[pos]
        pos += 1
        if n >= len(NAMES):
            raise ValueError(f'Invalid feature count {n} at tile index {i}')
        if pos + 2*n > len(raw):
            raise ValueError(f'Truncated minor feature array at tile index {i}')
        features = {}
        previous = 0
        for _ in range(n):
            typ, code = raw[pos], raw[pos + 1]
            pos += 2
            if typ <= previous or typ >= len(NAMES):
                raise ValueError(f'Unsorted, duplicate or unknown type {typ} at tile index {i}')
            if (typ == 1 and code > 120) or (typ in (2, 3) and code > 8):
                raise ValueError(f'Invalid feature code type={typ} code={code}')
            features[typ] = code
            previous = typ

        y = i // (width//2)
        x = 2*(i % (width//2)) + (y & 1)
        start = map_start + MAP_TILE_BYTES*i
        climate = raw[start]
        items, native_lm = struct.unpack_from('<II', raw, start + 8)
        tiles[(x, y)] = dict(features=features,
                             native_landmarks=native_lm & 0xFFFF,
                             items=items, altitude=climate >> 5,
                             rainfall=(climate >> 3) & 3)
    return width, height, version, tiles


def components(tiles, feature, width):
    unseen = {pos for pos, tile in tiles.items() if feature in tile['features']}
    groups = []
    while unseen:
        seed = unseen.pop()
        group = {seed}
        frontier = [seed]
        for x, y in frontier:
            for dx, dy in NEIGHBOURS:
                other = ((x + dx) % width, y + dy)
                if other in unseen:
                    unseen.remove(other)
                    group.add(other)
                    frontier.append(other)
        groups.append(group)
    return groups


def summary(width, height, version, tiles):
    counts = Counter(typ for tile in tiles.values() for typ in tile['features'])
    overlaps = [(pos, tile['native_landmarks']) for pos, tile in tiles.items()
                if tile['features'] and tile['native_landmarks']]
    stacks = [pos for pos, tile in tiles.items() if len(tile['features']) > 1]
    ocean = [pos for pos, tile in tiles.items()
             if tile['features'] and tile['altitude'] < 3]
    lines = [f'Map {width}x{height} ({len(tiles)} playable tiles); THMIN001 v{version}']
    for feature in (1, 2, 3):
        groups = components(tiles, feature, width)
        lengths = sorted((len(g) for g in groups), reverse=True)
        lines.append(f'{NAMES[feature]}: {counts[feature]} tiles, '
                     f'{len(groups)} connected patches; sizes {lengths}')
    lines.append(f'Stacked independent tiles: {len(stacks)}')
    lines.append(f'Minor tiles overlapping native landmarks: {len(overlaps)}')
    lines.append(f'Minor tiles stored in ocean: {len(ocean)}')
    lines.append(f'Invalid type/code/ocean entries: {len(ocean)}')
    for pos, flags in overlaps[:10]:
        lines.append(f'  OVERLAP {pos}: native LM flags 0x{flags:x}')
    return '\n'.join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('save', type=Path)
    parser.add_argument('coordinates', nargs='*', metavar='X,Y')
    args = parser.parse_args()
    width, height, version, tiles = read_save(args.save.read_bytes())
    print(summary(width, height, version, tiles))
    for coordinate in args.coordinates:
        try:
            x, y = map(int, coordinate.split(','))
        except ValueError:
            parser.error(f'Invalid coordinate {coordinate!r}; use X,Y')
        tile = tiles.get((x, y))
        if tile is None:
            print(f'({x},{y}): invalid map coordinate')
            continue
        names = ', '.join(f'{NAMES[t]}(code={c})' for t, c in tile['features'].items())
        print(f'({x},{y}): {names or "None"}, '
              f'altitude={tile["altitude"]}, rainfall={tile["rainfall"]}, '
              f'items=0x{tile["items"]:08X}, native_LM=0x{tile["native_landmarks"]:04X}')


if __name__ == '__main__':
    main()
