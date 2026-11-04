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
        'Titanium': ('0', '1', '0', '0', 'MINOR_GENERATE_LATE'),
    }
    for name, fields in expected.items():
        pattern = re.escape('{"' + name + '", ' + ', '.join(fields) + '}')
        require(re.search(pattern, store) is not None,
                f'Missing shared yield, atlas and phase definition for {name}')

    for option in ('minor_jungle', 'minor_uranium', 'minor_titanium'):
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
    geology = body_between(mapgen, 'static bool place_minor_geology_patch(',
                           '// Each type selects a generation phase')
    require('minor_landmark_set(p.x, p.y, MINOR_JUNGLE' in jungle
            and 'bit2_set(p.x, p.y, LM_JUNGLE' not in jungle
            and 'code_set(p.x, p.y' not in jungle,
            'Modified Jungle must use independent storage only')
    require('minor_landmark_set(p.x, p.y, type' in geology
            and 'MINOR_URANIUM' in geology and 'MINOR_TITANIUM' in geology
            and 'minor_landmark_at(tx, ty) == MINOR_NONE' in geology,
            'Uranium and Titanium must share non-overlapping independent storage')
    require('TableRange[2]' in geology
            and 'for (int round = 0; round < patch_count; ++round)' in geology
            and geology.index('place_minor_geology_patch(MINOR_URANIUM')
                < geology.index('place_minor_geology_patch(MINOR_TITANIUM')
            and 'bit_set(p.x, p.y, BIT_FUNGUS, false)' in geology,
            'The two geological types must alternate, keep separation, and clear fungus')
    require('new_landmark(' not in jungle + geology,
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
    require('const bool native_geology' in yields
            and '(minor_type == MINOR_URANIUM && conf.minor_native_uranium)' in yields
            and '(minor_type == MINOR_TITANIUM && conf.minor_native_titanium)' in yields
            and 'return 4;' in yields,
            'Both metals must share the native renderer-only Uranium sprite')
    require('minor_native_titanium_bonus_hook' not in renderer
            and 'minor_titanium_bonus_inner' not in renderer
            and 'type == MINOR_TITANIUM && conf.minor_native_titanium' in renderer,
            'Do not restore the old hand-positioned Titanium draw hook')
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
    require('minor_native_titanium=1' in ini
            and 'MATCH("minor_native_titanium")' in cfg,
            'Titanium native rendering switch must be supported')
    require('static bool install_minor_native_titanium_sprite()' in patches
            and 'constexpr int bonus_site = 0x465DFC;' in patches
            and 'constexpr int type_four_site = 0x465E0E;' in patches
            and 'constexpr int sprite_site = 0x465E2E;' in patches
            and 'constexpr int draw_site = 0x465E37;' in patches
            and 'constexpr int target = 0x5E4B9A;' in patches
            and 'bonus_expected' in patches
            and 'type_four_expected' in patches
            and 'sprite_expected' in patches
            and 'draw_expected' in patches
            and 'after_expected' in patches
            and 'if (!valid)' in patches
            and 'write_call(draw_site, (int)minor_native_titanium_sprite_hook);' in patches,
            'Titanium hook must target only the verified native resource-type-4 call')
    require('Sprite_extract_2(' in renderer
            and 'transparent = Buffer_get_pixel(&image, sprite_x, 4);' in renderer
            and '&minor_sprites[id], &image, transparent,' in renderer
            and 'if (id == MINOR_TITANIUM)' in renderer
            and 'transparent < 0 || transparent > 255' in renderer
            and '&minor_sprites[id], &image, 0x1FD,' not in renderer,
            'Titanium extractor must sample the post-load indexed palette')
    require('minor_titanium_select_native_sprite(' in renderer
            and 'frame[8], y = frame[9]' in renderer
            and 'minor_landmark_at(x, y) != MINOR_TITANIUM' in renderer
            and 'return &minor_sprites[MINOR_TITANIUM];' in renderer
            and 'jmp *_minor_titanium_native_draw_target' in renderer
            and '"1: movl %eax, 24(%esp)' in renderer
            and '"movl %eax, 28(%esp)' not in renderer
            and '"cmpl 24(%esp), %eax' in renderer
            and '"je 1f' in renderer
            and '"movzbl 8(%eax), %edx' in renderer
            and '"movl %edx, 44(%esp)' in renderer
            and 'TITANIUM COLOR KEY x=%d y=%d native=%u loaded=%u' in renderer
            and 'static_cast<uintptr_t>(frame[3])' not in renderer
            and 'MapWin_tile_to_pixel(win, x, y, &tile_px, &tile_py)' not in renderer,
            'Titanium native wrapper must change saved ECX, not EDX, with verified tile coordinates')
    require('minor_titanium_native_draw_target = (uintptr_t)target;' in patches
            and 'minor_native_titanium_sprite_installed = true;' in patches
            and 'minor_graphics_titanium_hook_result' in patches,
            'Titanium hook must publish original target and install diagnostics')
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
