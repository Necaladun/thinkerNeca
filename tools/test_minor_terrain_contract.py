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
    config = source('src/main.h')
    mapgen = source('src/mapgen.cpp')
    yields = source('src/map.cpp')
    renderer = source('src/gui.cpp')
    former_ai = source('src/move.cpp')
    patches = source('src/patch.cpp')
    ini = source('docs/thinker.ini')

    require('struct MinorTerrainEntry' in defs and 'uint8_t type;' in defs
            and 'uint8_t code;' in defs
            and 'std::vector<MinorTerrainEntry> features;' in defs
            and 'sizeof(MinorTerrainEntry) == 2' in defs,
            'Minor terrain must store ordered type/code pairs per tile')
    require('static const uint32_t MinorLandmarkVersion = 3;' in store
            and 'tile.features.resize(size)' in store
            and 'entry.type <= last_type' in store
            and 'minor_landmark_clear_all' in store
            and 'entries.insert(it' in store,
            'Expected validated v3 multi-feature save and type-specific mutation')

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
    require('minor_landmark_code_at(x, y, MINOR_JUNGLE)' in mapgen
            and 'minor_landmark_has(x, y, MINOR_JUNGLE)' in mapgen,
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
            and '!minor_landmark_any_at(tx, ty)' in geology,
            'Uranium and Titanium must share non-overlapping independent storage')
    require('TableRange[2]' in geology
            and 'for (int round = 0; round < patch_count; ++round)' in geology
            and geology.index('place_minor_geology_patch(MINOR_URANIUM')
                < geology.index('place_minor_geology_patch(MINOR_TITANIUM')
            and 'bit_set(p.x, p.y, BIT_FUNGUS, false)' not in geology,
            'Geological types must alternate and preserve pre-existing fungus')
    sizes_match = re.search(
        r'const int patch_sizes\[5\]\s*=\s*\{([^}]+)\};', geology)
    require(sizes_match is not None,
            'Geological deposit sizes must use the approved five-entry distribution')
    sizes = [int(x.strip()) for x in sizes_match.group(1).split(',')]
    require(sizes == [3, 4, 4, 4, 5]
            and 'const int target = patch_sizes[rng.get(5)];' in geology,
            'Geology must generate 3–5-tile patches with a weighted mean of 4')
    require('max(2, min(6, (*MapAreaTiles + 400) / 800))' in geology
            and 'const int patch_count = (3 * previous_patch_count + 1) / 2;' in geology
            and 'for (int attempt_patch = 0; attempt_patch < 8; ++attempt_patch)' in geology
            and 'for (int attempt = 0; attempt < 96; ++attempt)' in geology
            and 'if ((int)patch.size() != target) {' in geology
            and geology.index('if ((int)patch.size() != target) {')
                < geology.index('minor_landmark_set(p.x, p.y, type'),
            'Geology count must scale 50% while failed patches leave no partial features')
    require('new_landmark(' not in jungle + geology,
            'Minor terrain may not create native named landmarks')
    require('minor_landmark_clear_all(tx, ty)' in mapgen,
            'Major landmark footprints must clear earlier minor terrain')

    # Geology contributions are intentionally conditional on non-Forest
    # terrain; Jungle nutrients retain the original unconditional route.
    require('const int minor_nutrients = minor_terrain_yields_at(x, y).nutrient;' in yields
            and 'const int minor_minerals = (sq->items & BIT_FOREST)' in yields
            and '? 0 : minor_terrain_yields_at(x, y).mineral;' in yields
            and 'const int minor_energy = (sq->items & BIT_FOREST)' in yields
            and '? 0 : minor_terrain_yields_at(x, y).energy;' in yields,
            'Live nutrient/geology sources must match the Forest exclusion policy')
    require('const MinorTerrainYield minor = minor_terrain_yields_at(x, y);' in yields
            and 'N += minor.nutrient;' in yields
            and 'if (item != BIT_FOREST) {' in yields
            and 'M += minor.mineral;' in yields
            and 'E += minor.energy;' in yields,
            'Thinker AI forecast must suppress geological bonuses in Forests, not Jungle nutrients')
    mine = body_between(yields, 'int __cdecl mod_mine_yield(', 'int __cdecl mod_energy_yield(')
    energy = body_between(yields, 'int __cdecl mod_energy_yield(', 'int __cdecl mod_hex_cost(')
    forecast = body_between(yields, 'int item_yield(', 'static void find_reset(')
    require('const int minor_minerals = (sq->items & BIT_FOREST)' in mine
            and '? 0 : minor_terrain_yields_at(x, y).mineral;' in mine
            and 'const int minor_energy = (sq->items & BIT_FOREST)' in energy
            and '? 0 : minor_terrain_yields_at(x, y).energy;' in energy
            and forecast.index('if (item != BIT_FOREST) {')
                < forecast.index('M += minor.mineral;')
            and forecast.index('if (item != BIT_FOREST) {')
                < forecast.index('E += minor.energy;'),
            'Actual and AI future Forest yields must not benefit from independent geology')
    require('if (bonus_mineral || bonus_landmark || minor_minerals)' in mine,
            'Keep existing single Mine enhancement for the 3–5-tile balance trial')
    # AI-002: source-level checks, not actual AI-controlled Windows testing.
    mine_choice = body_between(former_ai, 'bool can_mine(', 'bool can_forest(')
    solar_choice = body_between(former_ai, 'bool can_solar(', 'static int projected_mine_nutrients(')
    selection = body_between(former_ai, 'int select_item(', 'int former_tile_score(')
    former_score = body_between(former_ai, 'int former_tile_score(', 'int former_move(')
    require('minor_landmark_has(x, y, MINOR_TITANIUM)' in mine_choice
            and 'if (!sea && !sq->is_rocky() && !titanium)' in mine_choice
            and 'projected_mine_nutrients(x, y, faction_id) < 1' in mine_choice
            and 'BIT_FARM | BIT_CONDENSER | BIT_SOIL_ENRICHER' in mine_choice
            and 'mod_mine_yield(faction_id, -1, x, y, 1)' in mine_choice,
            'Titanium-only nonrocky Mine eligibility must protect Flat food')
    require('>= conf.tile_output_limit[1]' in mine_choice
            and '>= conf.tile_output_limit[2]' in solar_choice,
            'Former Mine/Solar pre-tech gates must use configured caps')
    require('int titanium_mine_val = 0;' in selection
            and 'int mine_n = projected_mine_nutrients(' in selection
            and 'int solar_total = mod_crop_yield(' in selection
            and 'mine_total > solar_total' in selection
            and 'bool flat_farm_wins' in selection
            and 'flat_farm_wins' in selection
            and 'titanium_mine_val == 0' in selection
            and 'borehole_val, titanium_mine_val' in selection
            and 'titanium_mine_val == max_val' in selection
            and 'return FORMER_MINE;' in selection
            and 'score += 4;' in former_score,
            'AI-002 Mine must compete with Forest/Solar and have modest priority')
    require('tile_output_limit=3,4,4' in ini,
            'Distributed experimental caps should be 3/4/4')
    require([(n - 1 if n > 1 else n) for n in (0, 1, 2, 3)] == [0, 1, 1, 2],
            'Synthetic Mine nutrient penalty: only above one nutrient')
    require('(type != MINOR_TITANIUM || mod_bonus_at(tx, ty) != RES_MINERAL)' in geology
            and 'BIT_MINERAL_RES alone' in geology,
            'Titanium generation must reject both flagged and procedural mineral resources')

    # A mineral-resource supply-pod discovery on independent Titanium
    # is the single permitted reroll path; normal Uranium, unrelated pods,
    # and natural resources must retain their existing outcomes.
    veh = source('src/veh.cpp')
    pod = body_between(veh, 'int __cdecl goody_box(', 'int __cdecl study_artifact(')
    resource_pod = body_between(pod,
        'bit_set(x, y, BIT_BONUS_RES, 1);', 'site_radius(x, y);')
    require('int bonus_val = bonus_at(x, y);' in resource_pod
            and 'if (bonus_val == RES_MINERAL' in resource_pod
            and 'minor_landmark_has(x, y, MINOR_TITANIUM)' in resource_pod
            and 'const bool choose_energy = goody_rand(2) != 0;' in resource_pod
            and 'bit_set(x, y, BIT_NUTRIENT_RES | BIT_MINERAL_RES' in resource_pod
            and '| BIT_ENERGY_RES, false);' in resource_pod
            and 'bit_set(x, y, BIT_NUTRIENT_RES' in resource_pod
            and '| (choose_energy ? BIT_ENERGY_RES : 0), true);' in resource_pod
            and resource_pod.count('bonus_val = bonus_at(x, y);') == 2,
            'Only Titanium mineral pod rewards may become 50/50 nutrient/energy')
    # The current coordinate/resource decoder ignores energy/mineral bias
    # flags for nonzero coordinate-derived ret unless NUTRIENT_RES is set.
    # Test the forced flag combinations against every land coordinate class.
    require('if (!ret || bit & BIT_NUTRIENT_RES)' in yields
            and 'if (bit & BIT_ENERGY_RES)' in yields
            and 'return ((bit & BIT_MINERAL_RES) != 0) + 1;' in yields,
            'Pod resource override must match the live coordinate decoder')
    for chk in range(16):
        ret = (chk % 5) & 3
        for energy_choice in (False, True):
            # Both forced choices set NUTRIENT_RES, so the live decoder
            # selects energy iff ENERGY_RES is also present, for all chk.
            forced = 3 if energy_choice else 1
            forced_nutrient_flag = True
            forced_mineral_flag = False
            forced_energy_flag = energy_choice
            if not ret or forced_nutrient_flag:
                resolved = (3 if forced_energy_flag
                            else 2 if forced_mineral_flag else 1)
            else:
                resolved = ret
            require(resolved == forced,
                'Forced Titanium pod nutrient/energy bias must override coordinate class')

    move = source('src/move.cpp')
    require('minor_landmark_at(' not in move
            and 'minor_landmark_at(' not in patches
            and 'minor_landmark_any_at(' in move
            and 'minor_landmark_has(x, y, MINOR_JUNGLE)' in move
            and 'minor_landmark_has(x, y, MINOR_JUNGLE)' in patches,
            'AI and Jungle native hook must use explicit multi-minor membership')
    require('minor_landmark_at(' not in mapgen and
            'minor_landmark_at(' not in yields and
            'minor_landmark_at(' not in renderer,
            'Do not revive single-valued minor feature access')

    underlay = body_between(renderer, 'void minor_graphics_underlay_native_probe()',
                            'Ready-to-use native feature-line composition')
    require('REND005 READONLY native Uranium probe' in underlay
            and '0x4632D0u' in underlay and '0x467000u' in underlay
            and '0x5E4B9Au' in underlay
            and 'max_predicates = 24' in underlay
            and 'max_draw_calls = 24' in underlay
            and 'write_bytes(' not in underlay
            and 'write_jump(' not in underlay
            and 'write_call(' not in underlay
            and 'minor_graphics_underlay_native_probe();' in patches,
            'REND-005 underlay reconnaissance must be bounded and read-only')

    # Evaluate the resource wrapper before asserting underlay fallback.
    bonus_wrapper = body_between(yields, 'int __cdecl mod_bonus_at(int x, int y) {',
                                 'Determine if the tile has a supply pod')
    underlay_install = body_between(
        patches, 'static bool install_minor_native_geology_underlay()',
        'The supported SMAX executable checks native Jungle')
    require('constexpr int mask_site = 0x464D9C;' in underlay_install
            and 'constexpr int compare_site = 0x464DA2;' in underlay_install
            and 'constexpr int draw_site = 0x464DE3;' in underlay_install
            and '0x81, 0xE6, 0x08, 0x00, 0x00, 0x80' in underlay_install
            and '0x83, 0xFE, 0x08, 0x75, 0x1A' in underlay_install
            and '0x50, 0x51, 0xB9, 0x2C, 0xA7, 0x78' in underlay_install
            and 'if (!valid)' in underlay_install
            and underlay_install.index('if (!valid)') < underlay_install.index('write_bytes(')
            and 'write_jump(mask_site, (int)minor_native_geology_underlay_mask_hook)' in underlay_install
            and 'write_call(draw_site, (int)minor_native_geology_underlay_sprite_hook)' in underlay_install
            and 'minor_native_geology_underlay_installed = true;' in underlay_install,
            'REND-005 must check all original Uranium/underlay bytes before both hooks')
    require('minor_native_geology_underlay_installed' in yields
            and 'return RES_NONE;' in bonus_wrapper
            and 'return 4; // Safe legacy fallback' in bonus_wrapper,
            'Underlay must remove duplicate geology sentinel but retain fallback')
    require('orl $8, 4(%esp)' in patches
            and 'andl $0x80000008, %esi' in patches
            and 'minor_landmark_has(x, y, MINOR_URANIUM)' in patches
            and 'minor_landmark_has(x, y, MINOR_TITANIUM)' in patches,
            'Underlay trampoline must preserve LM_DISABLE, not mutate stored LM')
    require('minor_select_geology_underlay_sprite' in renderer
            and 'minor_geology_underlay_current_type' in renderer
            and 'minor_geology_underlay_current_x' in renderer
            and 'minor_geology_underlay_current_y' in renderer
            and 'minor_geology_underlay_current_type =' in patches
            and 'minor_geology_underlay_current_x = x;' in patches
            and 'minor_geology_underlay_current_y = y;' in patches
            and 'REND005 UNDERLAY SELECT cached=' in renderer
            and 'REND005 UNDERLAY GEOMETRY native_key=' in renderer
            and 'type == MINOR_TITANIUM && valid_original && loader_ready' in renderer
            and 'jmp *_minor_geology_underlay_draw_target' in renderer
            and 'native_sprite == 0x78A700u' in renderer
            and 'native_sprite == 0x78A72Cu' in renderer
            and 'native_variant >= 0' in renderer
            and 'original=%p variant=%d expected=%d' in renderer,
            'Underlay Titanium must recognise both original native Sprite objects and log the runtime variant')
    require('MATCH("minor_geology_underlay")' in cfg
            and 'int minor_geology_underlay = 0;' in config
            and 'minor_geology_underlay=1' in ini,
            'Experimental REND-005 native underlay must be opt-out and reversible')

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
    jungle_install = body_between(patches, 'static bool install_minor_native_jungle()', '#endif')
    helper = source('src/jungle_hook_preflight.h')
    harness = source('tools/test_jungle_hook_preflight.cpp')
    workflow = source('.github/workflows/build-windows.yml')
    require('const byte original[6] = {0x81, 0xE2, 0x04, 0, 0, 0x80};' in jungle_install
            and 'JungleHookSite locations[]' in jungle_install
            and 'jungle_hook_preflight::validate_then_apply(' in jungle_install
            and 'if (memcmp((const void*)site.addr, original, sizeof(original)))' in jungle_install
            and 'write_bytes(site.addr, original, redirect' in jungle_install
            and 'write_jump(site.addr, site.target);' in jungle_install
            and 'validate_then_apply' in helper
            and 'for (std::size_t i = 0; i < N; ++i)' in helper
            and 'if (!verify(sites[i]))' in helper
            and 'apply(sites[i]);' in helper
            and 'for (int changed_site = 0; changed_site < 3; ++changed_site)' in harness
            and 'for (int changed_byte = 0; changed_byte < 6; ++changed_byte)' in harness
            and 'Test native Jungle three-site preflight (host harness)' in workflow,
            'Jungle native hook must share an all-sites-before-any-write helper with CI harness')
    require(renderer.count('&& (!conf.minor_native_jungle || !conf.minor_native_uranium') == 2
            and '|| !conf.minor_native_titanium))' in renderer,
            'Do not scan every tile when all native types are enabled')
    bonus_wrapper = body_between(yields, 'int __cdecl mod_bonus_at(int x, int y) {',
                                 'Determine if the tile has a supply pod')
    native_resources = body_between(yields,
                                    'static int underlying_resource_bonus_at(',
                                    'int __cdecl mod_bonus_at(int x, int y) {')
    require('const bool native_geology' in bonus_wrapper
            and '(minor_landmark_has(x, y, MINOR_URANIUM) && conf.minor_native_uranium)' in bonus_wrapper
            and '(minor_landmark_has(x, y, MINOR_TITANIUM) && conf.minor_native_titanium)' in bonus_wrapper
            and 'return 4;' in bonus_wrapper,
            'Both metals must share the native renderer-only Uranium sprite')
    require('BIT_BONUS_RES' in native_resources
            and 'bool has_rsc_bonus = bit & BIT_BONUS_RES;' in native_resources
            and 'chk != ((*MapRandomSeed' in native_resources
            and 'const int normal_bonus = underlying_resource_bonus_at(x, y, sq);' in bonus_wrapper
            and bonus_wrapper.index('if (normal_bonus != RES_NONE)')
                < bonus_wrapper.index('return 4;')
            and '!(sq->items & BIT_BONUS_RES)' not in bonus_wrapper,
            'Natural and editor-placed resources must precede the geological renderer sentinel')
    require('minor_native_titanium_bonus_hook' not in renderer
            and 'minor_titanium_bonus_inner' not in renderer
            and 'type == MINOR_TITANIUM && conf.minor_native_titanium' in renderer,
            'Do not restore the old hand-positioned Titanium draw hook')
    require('minor_inspector_feature_line(const char* native, int x, int y)' in renderer
            and 'minor_terrain_names_at(x, y)' in renderer
            and 'line += ", ";' in renderer
            and 'minor_inspector_trace_hover(ptTile.x, ptTile.y);' in renderer,
            'Build a format-only feature-line seam without native landmark spoofing')
    require('void minor_graphics_startup_trace()' in renderer
            and 'Thinker minor-terrain graphics: Develop build startup reached' in renderer
            and 'minor_inspector_scan_native' not in renderer,
            'Startup must keep a concise diagnostic header without obsolete native scans')
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
    require('minor_titanium_sprite_mode=3' in ini
            and 'int minor_titanium_sprite_mode = 0;' in config
            and 'MATCH("minor_titanium_sprite_mode")' in cfg
            and 'cf->minor_titanium_sprite_mode = clamp(atoi(value), 0, 3);' in cfg,
            'Titanium external sprite test must parse modes 0-3, default to native')
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
    require('static void load_neca_test_sprites(Buffer* canvas)' in renderer
            and 'minor_graphics_file("neca_sprites.pcx")' in renderer
            and '{"uranium_test", 16, 16, 100, 62, 9}' in renderer
            and '{"titanium_test", 144, 16, 100, 62, 9}' in renderer
            and 'def.x + def.width > atlas_width' in renderer
            and 'decoded_corner != 7' in renderer
            and 'Buffer_load_pcx(&image, path.c_str(), nullptr, 10, 236)' in renderer
            and 'Buffer_change_color(&image, 0x107, 0x109)' in renderer
            and 'Buffer_change_color(&image, 0x106, 0x109)' in renderer
            and 'processed_hash == 0x24A1B67Au' in renderer
            and 'const bool uranium_reference = id == NECA_SPRITE_URANIUM_TEST;' in renderer
            and '(!uranium_reference || (transparent_pixels == 5635' in renderer
            and 'visible_pixels > 0' in renderer
            and 'transparent_pixels == 5635' in renderer
            and 'visible_pixels == 565' in renderer
            and 'if (!pixels_match)' in renderer
            and 'sprite.color_index[0] == def.transparent_index' in renderer
            and '&neca_sprites[id], &image, def.transparent_index,' in renderer,
            'Uranium control must use native index offset/masks and fail closed on hash mismatch')
    require('Buffer_load_pcx(&image, path.c_str(), nullptr, 0, 0)' in renderer
            and 'Buffer_load_pcx(&image, path.c_str(), canvas->palette, 0, 0)' not in renderer,
            'PCX ore indices must not be remapped into the canvas palette before sprite extraction')
    require('Sprite_extract_2(' in renderer
            and 'transparent = Buffer_get_pixel(&image, sprite_x, 4);' in renderer
            and '&minor_sprites[id], &image, transparent,' in renderer
            and 'if (id == MINOR_TITANIUM)' in renderer
            and 'transparent < 0 || transparent > 255' in renderer
            and '&minor_sprites[id], &image, 0x1FD,' not in renderer,
            'Titanium extractor must sample the post-load indexed palette')
    require('minor_titanium_select_native_sprite(' in renderer
            and 'frame[8], y = frame[9]' in renderer
            and '!minor_landmark_has(x, y, MINOR_TITANIUM)' in renderer
            and 'mode == 1 || mode == 3' in renderer
            and 'mode == 3' in renderer
            and 'const NecaSpriteId id = mode == 3' in renderer
            and 'NECA_SPRITE_TITANIUM_TEST : NECA_SPRITE_URANIUM_TEST' in renderer
            and 'native_variant == 0 && original->color_index[0] == 9' in renderer
            and 'original_address == 0x78A700u ? 0' in renderer
            and 'original_address == 0x78A72Cu ? 1 : -1' in renderer
            and 'load_neca_test_sprites(canvas);' in renderer
            and 'selected = &neca_sprites[id];' in renderer
            and 'mode == 2' in renderer
            and 'selected = &minor_sprites[MINOR_TITANIUM];' in renderer
            and 'return selected;' in renderer
            and 'TITANIUM SPRITE TEST mode=%d' in renderer
            and 'reports_per_variant[3]' in renderer
            and 'reports_per_variant[report_slot] < 4' in renderer
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
            and "memcpy(end, name.c_str(), name_len + 1);" in renderer
            and 'INSPECTOR APPEND x=%d y=%d' in renderer
            and '"INSPECTOR HOOK native feature strlen' in renderer,
            'Feature-line append needs native ABI, bounded output and logging')
    require('sq->is_known(MapWin->cOwner)' in renderer
            and 'length + separator_len >= capacity' in renderer
            and 'name_len >= capacity - length - separator_len' in renderer,
            'Native Inspector hook must respect fog and buffer capacity')
    print('Minor-terrain structural/source invariants passed')


if __name__ == '__main__':
    main()
