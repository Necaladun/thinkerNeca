#!/usr/bin/env python3
"""Static migration invariants. NOT a substitute for engine/in-game tests."""
from pathlib import Path
import re


def source(path: str) -> str:
    return Path(path).read_text(encoding='utf-8')


def require(condition: bool, explanation: str) -> None:
    if not condition:
        raise AssertionError(explanation)


def body_between(text: str, start: str, end: str) -> str:
    i = text.find(start)
    j = text.find(end, i + len(start))
    require(i >= 0 and j > i, f'Missing section {start} -> {end}')
    return text[i:j]


def main() -> None:
    defs = source('src/savegame.h')
    store = source('src/savegame.cpp')
    cfg = source('src/main.cpp')
    mapgen = source('src/mapgen.cpp')
    yields = source('src/map.cpp')
    renderer = source('src/gui.cpp')
    patches = source('src/patch.cpp')
    ini = source('docs/thinker.ini')

    require('struct MinorTerrainTile' in defs and 'uint8_t type;' in defs
            and 'uint8_t code;' in defs
            and 'static_assert(sizeof(MinorTerrainTile) == 2' in defs,
            'Minor terrain must have an independent two-byte tile record')
    require('static const uint32_t MinorLandmarkVersion = 2;' in store,
            'Expected version-2 type/code saves')

    expected = {
        'Jungle': ('1', '0', '0', '1', 'MINOR_GENERATE_EARLY'),
        'Uranium': ('0', '0', '1', '2', 'MINOR_GENERATE_LATE'),
        'Ironstone': ('0', '1', '0', '0', 'MINOR_GENERATE_LATE'),
    }
    for name, fields in expected.items():
        pattern = re.escape('{"' + name + '", ' + ', '.join(fields) + '}')
        require(re.search(pattern, store) is not None,
                f'Missing shared yield, atlas and phase definition for {name}')

    for option in ('minor_jungle', 'minor_uranium', 'minor_ironstone'):
        require(f'MATCH("{option}")' in cfg and re.search(
            rf'^\s*{option}=1$', ini, re.M) is not None,
            f'Missing independent configuration for {option}')
    for old in ('jungle', 'uranium'):
        require(re.search(rf'^\s*{old}=0$', ini, re.M) is not None,
                f'Native {old} must be disabled for independent test maps')
    require(re.search(r'^\s*modified_landmarks=1$', ini, re.M) is not None,
            'Modified landmark world generation must remain enabled')

    early = mapgen.index('generate_minor_landmarks(MINOR_GENERATE_EARLY);')
    landmarks = mapgen.index('LMConfig lm;')
    climate = mapgen.rindex('world_climate();', 0, mapgen.index('generate_minor_landmarks(MINOR_GENERATE_LATE);'))
    late = mapgen.index('generate_minor_landmarks(MINOR_GENERATE_LATE);')
    require(early < landmarks < climate < late,
            'Jungle generation must precede landmarks and final rainfall')
    require('minor_landmark_code_at(x, y)' in mapgen
            and 'minor_landmark_at(x, y) == MINOR_JUNGLE' in mapgen,
            'Rainfall must consult independent Jungle type/code')
    require('if (conf.world_mirror_x || conf.world_mirror_y)' in
            body_between(mapgen, 'static void generate_minor_landmarks(', 'void __cdecl mod_world_diamond('),
            'Do not silently create asymmetric minor patches')

    jungle = body_between(mapgen, '// Use a smaller overall Jungle budget',
                          'void __cdecl mod_world_sargasso(')
    uranium = body_between(mapgen, '// Uranium is distributed as compact',
                           'void __cdecl mod_world_fresh(')
    require('minor_landmark_set(p.x, p.y, MINOR_JUNGLE' in jungle
            and 'bit2_set(p.x, p.y, LM_JUNGLE' not in jungle
            and 'code_set(p.x, p.y' not in jungle,
            'Modified Jungle must use independent storage only')
    require('minor_landmark_set(p.x, p.y, MINOR_URANIUM' in uranium
            and 'bit2_set(p.x, p.y, LM_URANIUM' not in uranium
            and 'code_set(p.x, p.y' not in uranium,
            'Modified Uranium must use independent storage only')
    require('new_landmark(' not in jungle + uranium,
            'Minor terrain may not create native named landmarks')
    require('minor_landmark_clear(tx, ty)' in mapgen,
            'Major landmark footprints must clear earlier minor terrain')

    for metric, func in [('nutrient', 'mod_crop_yield'),
                         ('mineral', 'mod_mine_yield'),
                         ('energy', 'mod_energy_yield')]:
        require(re.search(r'const int minor_\w+\s*=\s*minor_terrain_definition\('
                          r'minor_landmark_at\(x, y\)\)\.' + metric,
                          yields) is not None,
                f'Expected central minor {metric} bonus in {func}')
    require('N += minor.nutrient;' in yields
            and 'M += minor.mineral;' in yields
            and 'E += minor.energy;' in yields,
            'Thinker AI yield estimator must include all three types')

    require('write_jump(0x4688E0, (int)MapWin_gen_overlays);' in patches,
            'Alternative rendering must use the existing verified callback')
    require('draw_minor_terrain_tile(This, x, y);' in renderer,
            'Per-tile renderer mode missing')
    require('write_jump(0x4B6570' not in patches
            and 'write_call(0x4B6570' not in patches,
            'Do not restore a speculative native status-window hook')
    require('minor_native_jungle' in patches
            and all(f'0x{x}' in patches for x in
                    ('463CC5', '463D93', '464E78')),
            'Expected three stage-native Jungle flag checks')
    require('const byte original[6] = {0x81, 0xE2, 0x04, 0, 0, 0x80};' in patches
            and 'write_bytes(site.addr, original, redirect' in patches,
            'Never install native Jungle hooks without expected-byte validation')
    require('native Ironstone bonus_at site' in renderer
            and 'minor_native_ironstone_bonus_hook' in renderer
            and 'matches != 1' in patches,
            'Ironstone must require a unique verified native bonus_at call')
    require('write_call(site, (int)minor_native_ironstone_bonus_hook)' in patches
            and 'sprite_x, sprite_y, draw_scale, source_scale' in renderer,
            'Ironstone must paint at native tile sprite origin')
    require('frame[4] <= -8' not in renderer
            and 'MapWin_tile_to_pixel(win, x, y, &tile_px, &tile_py)' in renderer
            and 'const bool far_zoom = zoom <= -6;' in renderer
            and 'draw_scale, source_scale' in renderer
            and 'sprite_y = static_cast<int>(tile_py) - lift' in renderer,
            'Ironstone must draw at minimum zoom and remain tile-centred')
    require('minor_sprites[id].width > 0' in renderer
            and 'minor_sprites[id].height <= 56' in renderer,
            'Transparent trimming must not disable solid Ironstone art')
    require('minor_inspector_feature_line(const char* native, int x, int y)' in renderer
            and 'minor_terrain_name_at(x, y)' in renderer
            and 'line += ", ";' in renderer
            and 'minor_inspector_trace_hover(ptTile.x, ptTile.y);' in renderer,
            'Build a format-only feature-line seam without native landmark spoofing')
    require('minor_inspector_scan_native(fp);' in renderer
            and 'INSPECTOR SCAN' in renderer
            and '0x4B6570' in renderer
            and '0x4B8890' in renderer,
            'Status feature-line diagnostics must read native executable bytes')
    require('write_jump(0x4B6570' not in patches
            and 'Natural[13]' not in renderer
            and 'sq->landmarks |= LM_NEXUS' not in renderer,
            'Tile inspector diagnostics may not spoof native landmarks')
    require('minor_native_inspector=1' in ini
            and 'MATCH("minor_native_inspector")' in cfg,
            'Native Inspector hook needs a reversible configuration switch')
    require('const int site = 0x4B7A0C;' in patches
            and '0xE8, 0xCF, 0xD9, 0x18, 0x00' in patches
            and 'memcmp((const void*)site, expected, sizeof(expected))' in patches
            and 'write_call(site, (int)minor_native_inspector_strlen_hook);' in patches,
            'Only the verified native Inspector strlen call may be redirected')
    require('int __cdecl minor_native_inspector_strlen_hook()' in renderer
            and 'minor_inspector_length_inner(char* text, int tile_half_x, int tile_y)' in renderer
            and "const size_t capacity = 256;" in renderer
            and "memcpy(end, name, name_len + 1);" in renderer
            and '"INSPECTOR HOOK native feature strlen' in renderer,
            'Feature-line append needs native ABI, bounded output and logging')
    require('sq->is_known(MapWin->cOwner)' in renderer
            and 'length + separator_len >= capacity' in renderer
            and 'name_len >= capacity - length - separator_len' in renderer,
            'Native Inspector hook must respect fog and buffer capacity')
    print('Minor-terrain structural/source invariants passed')


if __name__ == '__main__':
    main()
