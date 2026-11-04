#!/usr/bin/env python3
"""Read Thinker's independent minor-terrain layer in a SMAC/X SAV.

Pure Python; no game installation needed. This does not interpret the native
status window or calculate full engine resource yields.

Usage: python tools/inspect_minor_save.py game.SAV 133,65 90,30
"""
import argparse
from collections import Counter
from pathlib import Path
import struct

MAGIC = b'THMIN001'
VERSION = 2
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
    if count <= 0 or len(raw) < offset + 16 + 2*count:
        raise ValueError('Truncated or invalid minor-terrain array')

    # Map format: 0xAA4 game metadata + count 44-byte MAP records +
    # map-abstract bytes + THMIN001 extension. Work backwards from the
    # extension rather than guessing where the map begins in the SAV.
    dimensions = []
    for half_width in range(1, count + 1):
        if count % half_width:
            continue
        width, height = 2*half_width, count // half_width
        if not (8 <= width <= 4096 and 8 <= height <= 4096):
            continue
        ax = (width + 4) // 5
        ay = (height + 4) // 5
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
    types = raw[offset + 16:offset + 16 + 2*count]
    tiles = {}
    for i in range(count):
        y = i // (width//2)
        x = 2*(i % (width//2)) + (y & 1)
        start = map_start + MAP_TILE_BYTES*i
        climate = raw[start]
        items, native_lm = struct.unpack_from('<II', raw, start + 8)
        terrain_type, code = types[i*2:i*2+2]
        tiles[(x, y)] = dict(type=terrain_type, code=code,
                             native_landmarks=native_lm & 0xFFFF,
                             items=items, altitude=climate >> 5,
                             rainfall=(climate >> 3) & 3)
    return width, height, version, tiles


def components(tiles, feature, width):
    unseen = {pos for pos, tile in tiles.items() if tile['type'] == feature}
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
    counts = Counter(tile['type'] for tile in tiles.values())
    errors = []
    overlaps = []
    for pos, tile in tiles.items():
        feature, code = tile['type'], tile['code']
        if feature not in range(len(NAMES)):
            errors.append((pos, f'unknown type {feature}'))
        if feature == 0 and code:
            errors.append((pos, f'empty tile has code {code}'))
        if feature == 1 and code > 120:
            errors.append((pos, f'Jungle code {code} outside 0..120'))
        if feature in (2, 3) and code > 8:
            errors.append((pos, f'geological code {code} outside 0..8'))
        if feature and tile['native_landmarks']:
            overlaps.append((pos, tile['native_landmarks']))
        if feature and tile['altitude'] < 3:
            errors.append((pos, 'minor terrain generated in ocean'))
    lines = [f'Map {width}x{height} ({len(tiles)} playable tiles); THMIN001 v{version}']
    for feature in (1, 2, 3):
        groups = components(tiles, feature, width)
        lengths = sorted((len(g) for g in groups), reverse=True)
        lines.append(f'{NAMES[feature]}: {counts[feature]} tiles, '
                     f'{len(groups)} connected patches; sizes {lengths}')
    lines.append(f'Minor tiles overlapping native landmarks: {len(overlaps)}')
    lines.append(f'Invalid type/code/ocean entries: {len(errors)}')
    for pos, issue in errors[:10]:
        lines.append(f'  ERROR {pos}: {issue}')
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
        feature = tile['type']
        name = NAMES[feature] if 0 <= feature < len(NAMES) else f'Unknown {feature}'
        print(f'({x},{y}): {name}, code={tile["code"]}, '
              f'altitude={tile["altitude"]}, rainfall={tile["rainfall"]}, '
              f'items=0x{tile["items"]:08X}, native_LM=0x{tile["native_landmarks"]:04X}')


if __name__ == '__main__':
    main()
