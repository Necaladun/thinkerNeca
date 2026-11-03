
#include "mapgen.h"
#include "lib/FastNoiseLite.h"


static void name_landmark(int x, int y, MapLandmark lm) {
    new_landmark(x, y, Natural[__builtin_ctz(lm)].name);
}

int __cdecl near_landmark(int x, int y) {
    for (int i = 0; i < TableRange[8]; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        MAP* sq = mapsq(x2, y2);
        if (!sq) {
            continue;
        }
        // Jungle and Uranium are small terrain features, not full landmarks.
        // Keep an eight-tile radius for other landmarks and a two-tile radius
        // for these features. Check flags to include tiles with code zero.
        const int minor = LM_JUNGLE
            | (conf.modified_landmarks ? LM_URANIUM : 0);
        const int landmarks = sq->lm_items();
        // Minor terrain has an independent ID; zero-code tiles still count.
        const bool minor_tile = minor_landmark_at(x2, y2) != MINOR_NONE;
        if ((sq->code_at() && (landmarks & ~minor))
            || (i < TableRange[2] && ((landmarks & minor) || minor_tile))) {
            return true;
        }
    }
    return false;
}

bool locate_landmark(int* x, int* y, bool ocean) {
    int attempts = 0;
    if (!mapsq(*x, *y)) {
        do {
            if (*MapAreaY > 17) {
                *y = map_rand.get(*MapAreaY - 16) + 8;
            } else {
                *y = 0;
            }
            if (*MapAreaX > 1) {
                *x = map_rand.get(*MapAreaX);
                *x = ((*x ^ *y) & 1) ^ *x;
            } else {
                *x = 0;
            }
            if (++attempts >= 1000) {
                return false;
            }
        } while (is_ocean(mapsq(*x, *y)) != ocean || near_landmark(*x, *y));
    }
    return true;
}

/*
Refresh climate effects each time terrain related features are changed on the map.
Note that world_temperature and world_rivers are refactored to use local random
instead of the global state since changing it would not be necessary.
*/
void __cdecl world_climate() {
    debug("world_climate_start\n");
    world_shorelines();
    world_temperature();
    world_rivers();
    world_rainfall();
    Path_continents(Paths);
    world_analysis();
    if (!*WorldSkipTerritory) {
        reset_territory();
    }
    if (!*GameHalted) {
        for (int i = 0; i < 8; i++) {
            if (MapWinPtr[i]->DrawToggleA) {
                MapWin_clear_terrain(MapWinPtr[i]);
            }
        }
        WorldWin_clear_terrain(WorldWin);
    }
    do_all_non_input();
    debug("world_climate_done\n");
}

void __cdecl world_temperature() {
    GameRandom loc_rnd;
    loc_rnd.reseed(*MapRandomSeed + 17);
    int solar_scale = *MapAreaY / WorldBuilder->solar_energy;
    int thermal_scale = *MapAreaY / WorldBuilder->thermal_band;
    int deviance_scale = *MapAreaY / WorldBuilder->thermal_deviance;
    int warming_scale = *MapAreaY / WorldBuilder->global_warming;

    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y & 1; x < *MapAreaX; x += 2) {
            MAP* sq = mapsq(x, y);
            if ((sq->landmarks & (LM_DISABLE | LM_CRATER)) == LM_CRATER && sq->code_at() < 21) {
                continue;
            }
            int dev_rand = loc_rnd.get(0, 2 * deviance_scale);
            int orb_rand = loc_rnd.get(0, *MapPlanetaryOrbit + 1);
            int lat_dist = abs(deviance_scale + *MapAreaY / 2 - dev_rand - y);
            int temp_raw = (thermal_scale / 2
                + 2 * (lat_dist - solar_scale * (*MapPlanetaryOrbit - 1)
                - warming_scale * *MapSeaLevelCouncil)) / thermal_scale;
            int temp_base;
            if (temp_raw > 2) {
                temp_base = (temp_raw <= 9) ? 2 : 1;
            } else {
                temp_base = 3;
            }
            int alt = sq->alt_level();
            int temp = temp_base;
            for (int i = 0; i < 9; i++) {
                int nx = wrap(x + TableOffsetX[i]);
                int ny = y + TableOffsetY[i];
                MAP* nsq = mapsq(nx, ny);
                if (nsq && (nsq->items & BIT_THERMAL_BORE)
                && (alt <= 4 || nsq->alt_level() >= alt)) {
                    temp = temp_base + 1;
                    break;
                }
            }
            if (temp > 1) {
                if (alt <= 5) {
                    if (alt > 4 && !orb_rand) {
                        temp--;
                    }
                } else if (*MapPlanetaryOrbit < 2 || !loc_rnd.get(0, 2)) {
                    temp--;
                }
            }
            temp_set(x, y, clamp(temp - 1, 0, 2));
        }
    }
}

void __cdecl world_rivers() {
    GameRandom loc_rnd;
    const int n_tiles = *MapAreaTiles;
    int river_count = 0;

    for (int i = 0; i < n_tiles; i++) {
        (*MapTiles)[i].items &= ~(BIT_RIVER_LAKE | BIT_RIVER);
    }
    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y & 1; x < *MapAreaX; x += 2) {
            MAP* const sq = mapsq(x, y);
            if (is_ocean(sq) || !(sq->items & BIT_RIVER_SRC)) {
                continue;
            }
            for (int i = 0; i < n_tiles; i++) {
                (*MapTiles)[i].landmarks &= ~LM_UNK_800000;
            }
            loc_rnd.reseed(31 * x + 47 * y);
            int dir = loc_rnd.get(0, 4);
            int cur_x = x;
            int cur_y = y;
            int cur_alt = sq->alt_level();
            int cur_contour = sq->contour;
            sq->landmarks |= LM_UNK_800000;

            int steps = 0;
            int in_water = 0;
            int alt_streak = 1;
            int make_lake = !(sq->items & BIT_THERMAL_BORE);

            while (make_lake && ++steps <= 1000) {
                int pref_dir = (dir - (steps & 1) + loc_rnd.get(0, 2)) & 3;
                int back_dir = dir ^ 2;
                int min_uphill = 9999;
                int dir_counter = 8;
                int num_choices = 0;
                int total_weight = 0;
                int choice_dir[3] = {};
                int choice_water[3] = {};
                int choice_weight[3] = {};

                for (int nd = 0; nd < 4; nd++) {
                    int nx = wrap(cur_x + NearOffsetX[nd]);
                    int ny = cur_y + NearOffsetY[nd];
                    MAP* const nsq = mapsq(nx, ny);
                    if (!nsq || nd == back_dir) {
                        continue;
                    }
                    if (nsq->items & BIT_RIVER) {
                        num_choices = 0;
                        make_lake = 0;
                        break;
                    }
                    if (nsq->items & BIT_RIVER_SRC) {
                        int src_count = 0;
                        for (int i = 0; i < 4; i++) {
                            int px = wrap(nx + NearOffsetX[i]);
                            int py = ny + NearOffsetY[i];
                            MAP* psq = mapsq(px, py);
                            if (psq && psq->items & BIT_RIVER_SRC) {
                                src_count++;
                            }
                        }
                        if (src_count > 1) {
                            num_choices = 0;
                            make_lake = 0;
                            break;
                        }
                    }
                    if (nsq->alt_level() > cur_alt) {
                        continue;
                    }
                    int nsq_contour = nsq->contour;
                    int score = 32 * clamp(cur_contour - nsq_contour + 2, 1, 9999)
                        + (nd == pref_dir ? 31 : 0);
                    int water_flag = 0;
                    int uphill_adj = 0;
                    if (nsq_contour > cur_contour) {
                        if (nsq_contour >  min_uphill) { continue; }
                        min_uphill = nsq_contour;
                        score = dir_counter++;
                    }
                    bool next_loop = false;
                    for (int i = 0; i < 4; i++) {
                        if (i == (nd ^ 2)) { continue; }
                        int px = wrap(nx + NearOffsetX[i]);
                        int py = ny + NearOffsetY[i];
                        MAP* psq = mapsq(px, py);
                        if (psq) {
                            if (is_ocean(psq) && !is_ocean(nsq) && alt_streak < 5) {
                                score /= 2;
                            }
                            if (psq->contour > nsq_contour) {
                                uphill_adj++;
                                if (psq->items & BIT_RIVER) {
                                    next_loop = true;
                                    break;
                                }
                            }
                            if (psq->landmarks & LM_UNK_800000) {
                                if (in_water || is_ocean(psq)) {
                                    next_loop = true;
                                    break;
                                }
                                water_flag = 1;
                                score /= 8;
                            }
                        }
                    }
                    if (next_loop) {
                        continue;
                    }
                    if (score) {
                        if (uphill_adj == 3) { score = 1; }
                        total_weight += score;
                        assert(num_choices < 3);
                        choice_weight[num_choices] = score;
                        choice_water[num_choices] = water_flag;
                        choice_dir[num_choices] = nd;
                        num_choices++;
                    }
                }
                if (num_choices <= 0) {
                    break;
                }
                int pick = loc_rnd.get(0, total_weight);
                int* wp = choice_weight;
                int sel = 0;
                dir = 0;
                while (sel < num_choices && pick >= *wp) {
                    pick -= *wp;
                    sel++;
                    wp++;
                }
                if (sel < num_choices) {
                    dir = choice_dir[sel];
                    in_water = choice_water[sel];
                }
                cur_x = wrap(cur_x + NearOffsetX[dir]);
                cur_y = cur_y + NearOffsetY[dir];
                MAP* cur_sq = mapsq(cur_x, cur_y);
                if (cur_alt != cur_sq->alt_level()) {
                    alt_streak = 0;
                }
                cur_alt = cur_sq->alt_level();
                int new_contour = cur_sq->contour;
                if (new_contour > cur_contour) {
                    new_contour = ElevDetail[cur_alt];
                    if (cur_contour - 1 > new_contour) {
                        new_contour = cur_contour - 1;
                    }
                    alt_put_detail(cur_x, cur_y, new_contour);
                }
                cur_contour = new_contour;
                cur_sq->landmarks |= LM_UNK_800000;
                alt_streak++;
                if (cur_sq->items & BIT_THERMAL_BORE) {
                    make_lake = 0;
                    break;
                }
            }
            if (make_lake && cur_alt >= 3) {
                MAP* cur_sq = mapsq(cur_x, cur_y);
                if (!(cur_sq->items & BIT_RIVER) && cur_sq->anything_at() < 0) {
                    cur_sq->items |= BIT_RIVER_LAKE;
                }
            }
            river_count++;
            for (int i = 0; i < n_tiles; i++) {
                if ((*MapTiles)[i].landmarks & LM_UNK_800000) {
                    (*MapTiles)[i].items |= BIT_RIVER;
                }
            }
        }
    }
    for (int i = 0; i < n_tiles; i++) {
        (*MapTiles)[i].landmarks &= ~LM_UNK_800000;
    }
}

void __cdecl world_rainfall() {
    const int player_id = *CurrentPlayerFaction;
    const int n_tiles = *MapAreaTiles;
    *dword_9B22E0 = -1;

    for (int i = 0; i < n_tiles; i++) {
        (*MapTiles)[i].unk_1 = 0;
    }

    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y & 1; x < *MapAreaX; x += 2) {
            MAP* const sq = mapsq(x, y);
            if (sq->items & BIT_CONDENSER) {
                for (int i = 0; i < 9; i++) {
                    int nx = wrap(x + TableOffsetX[i]);
                    int ny = y + TableOffsetY[i];
                    MAP* nsq = mapsq(nx, ny);
                    if (nsq) {
                        nsq->unk_1 |= (nsq->unk_1 & 1 ? 4 : 1);
                    }
                }
            }
            if (is_ocean(sq)) {
                for (int i = 0, px = x; i < 3; i++, px -= 2) {
                    MAP* nsq = mapsq(wrap(px - 2), y);
                    MAP* csq = mapsq(wrap(px), y);
                    if (nsq && csq
                    && nsq->alt_level() >= csq->alt_level()
                    && i <= *MapCloudCover) {
                        nsq->unk_1 &= 0xEF;
                    }
                }
                MAP* psq = mapsq(wrap(x + 2), y);
                if (psq) {
                    psq->unk_1 |= 0x80;
                }
                if (*MapCloudCover > 0) {
                    MAP* nsq = mapsq(wrap(x + 4), y);
                    if (nsq && ((nsq->climate & 7u) > 1 || *MapCloudCover > 1)) {
                        nsq->unk_1 |= 0x80;
                        if (*MapCloudCover > 1) {
                            MAP* csq = mapsq(wrap(x + 6), y);
                            if (csq && csq->climate & 7u) {
                                csq->unk_1 |= 0x80;
                            }
                        }
                    }
                }
            }
            int alt = sq->alt_level();
            if (alt <= 4) {
                if (alt >= 3 && sq->items & BIT_FOREST) {
                    for (int i = 0; i < 3; i++) {
                        MAP* nsq = mapsq(wrap(x + BaseOffsetX[i]), y + BaseOffsetY[i]);
                        if (nsq) {
                            nsq->unk_1 |= 0x40;
                        }
                    }
                    if (!(x & 1)) {
                        sq->unk_1 |= 0x40;
                    }
                }
            } else {
                int cloudmass = (alt <= 5) ? WorldBuilder->cloudmass_hills : WorldBuilder->cloudmass_peaks;
                int rainfall = WorldBuilder->rainfall_coeff * (*MapCloudCover - 1);
                int max_back_steps = cloudmass + rainfall;
                if (max_back_steps >= 1) {
                    for (int step = 1, bx = x - 2; step <= max_back_steps; ++step, bx -= 2) {
                        MAP* bsq = mapsq(wrap(bx), y);
                        if (!bsq || is_ocean(bsq) || bsq->alt_level() >= alt) {
                            break;
                        }
                        bsq->unk_1 |= 0x20;
                    }
                }
                int max_fwd_steps = cloudmass - rainfall;
                if (max_fwd_steps >= 1) {
                    for (int step = 1, fx = x + 2; step <= max_fwd_steps; ++step, fx += 2) {
                        MAP* fsq = mapsq(wrap(fx), y);
                        if (!fsq || is_ocean(fsq) || fsq->alt_level() >= alt) {
                            break;
                        }
                        fsq->unk_1 |= 0x10;
                    }
                }
            }
        }
    }

    int rain_steps = 0;
    int rain_flag = 0;
    int dry_steps = 0;

    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y & 1; x < *MapAreaX; x += 2) {
            MAP* const sq = mapsq(x, y);
            if (y <= 0 || y >= *MapAreaY - 1) {
                sq->climate &= 0xE7u;
                continue;
            }
            int alt = sq->alt_level();
            int landmarks = sq->landmarks;
            int temperature = sq->climate & 7;
            int moisture_mod = 0;
            int moisture = 0;
            int sloping = 0;
            if ((landmarks & (LM_DISABLE | LM_DUNES)) == LM_DUNES
            || (landmarks & (LM_DISABLE | LM_MESA))  == LM_MESA) {
                rain_flag = 1;
            }
            if (sq->unk_1 & 0x20) {
                moisture = 1;
                rain_flag = 1;
                dry_steps = 0;
                sloping = 1;
            }
            if (sq->unk_1 & 0x80) {
                rain_flag = (temperature > 1) ? 1 : 0;
                rain_steps = 0;
                dry_steps = 0;
                if (*MapCloudCover >= 1) {
                    moisture_mod = 1;
                }
            }
            if (sq->unk_1 & 0x10) {
                if (rain_flag) { --rain_flag; }
                --moisture_mod;
            }
            if ((sq->unk_1 & 0x40) && alt <= 4) {
                rain_steps = 0;
                dry_steps = 0;
                moisture_mod = max(0, moisture_mod);
                moisture = sloping;
            }
            if (alt > 3) {
                if (++rain_steps > 4 * *MapCloudCover * WorldBuilder->rainfall_coeff + 1 && rain_flag) {
                    --rain_flag;
                }
                moisture = sloping;
            } else {
                bool reset_moist = false;
                if (alt == 3) {
                    if (rain_flag) {
                        reset_moist = true;
                        if (rain_steps == 0 && ++dry_steps > 4 * *MapCloudCover + 2) {
                            rain_flag = 0;
                            dry_steps = 0;
                        }
                    }
                } else {
                    rain_flag = 1;
                    dry_steps = 0;
                }
                if (rain_steps > 0) {
                    reset_moist = true;
                    rain_steps = rain_steps - 1 - *MapCloudCover;
                    if (rain_steps < 1) {
                        rain_steps = 0;
                        dry_steps = 0;
                        if (*MapCloudCover > 0 || alt < 3) {
                            rain_flag = 1;
                        }
                    }
                }
                if (reset_moist) {
                    moisture = sloping;
                }
            }
            if (sq->items & BIT_RIVER) {
                if ((x >> 1) % 3 && moisture_mod < 0) {
                    moisture_mod = 0;
                }
                rain_flag = 1;
                dry_steps = 0;
            }
            if (sq->unk_1 & 2) {
                --moisture;
                rain_flag = 0;
            } else if (rain_flag) {
                moisture_mod += rain_flag;
            }
            if (alt <= (temperature != 0) + 4) {
                moisture += moisture_mod;
            } else if (*MapCloudCover - alt + temperature + 5 >= 2 && moisture_mod > 0) {
                ++moisture;
            }
            const bool minor_jungle = minor_landmark_at(x, y) == MINOR_JUNGLE;
            if (((landmarks & (LM_DISABLE | LM_JUNGLE)) == LM_JUNGLE || minor_jungle)
                && !is_ocean(sq) && alt < 6) {
                int code = minor_jungle ? minor_landmark_code_at(x, y) : landmarks >> 24;
                if (code < 0) { code = -code; }
                if (code >= 49) {
                    if (code >= 81 || alt >= 5) { ++moisture; }
                    else { moisture += 2; }
                } else {
                    moisture += 2;
                }
            }
            if ((landmarks & (LM_DISABLE | LM_DUNES)) == LM_DUNES) {
                moisture = 0;
            }
            if ((landmarks & (LM_DISABLE | LM_UNITY)) == LM_UNITY || moisture < 0) {
                moisture = 0;
            } else if (moisture > 2) {
                moisture = 2;
            }
            if (sq->unk_1 & 1) {
                rain_flag = 1;
                if (moisture < 2) { ++moisture; }
            }
            if (sq->unk_1 & 4) {
                rain_flag = 1;
                if (moisture < 2) { ++moisture; }
            }
            if (*CurrentTurn) {
                // action_terraform related popup TERRAMINE / TERRAYOURS
                // remove unused dword_9B22DC base_find with the popup changes
                int old_moisture = (sq->climate >> 3) & 3;
                if (old_moisture != moisture && *dword_9B22E0 < 0) {
                    int base_id = base_find_2(x, y, player_id);
                    if (base_id >= 0 && *BaseFindDist <= 2) {
                        *dword_9B22E0 = base_id;
                    }
                }
            }
            climate_set(x, y, moisture);
        }
    }

    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y & 1; x < *MapAreaX; x += 2) {
            MAP* sq = mapsq(x, y);
            if (sq && !sq->is_rainy_or_moist()) {
                for (int i = 0; i < 8; i++) {
                    int nx = wrap(x + BaseOffsetX[i]);
                    int ny = y + BaseOffsetY[i];
                    MAP* nsq = mapsq(nx, ny);
                    if (nsq && !is_ocean(nsq) && nsq->is_rainy()) {
                        climate_set(x, y, 1);
                        break;
                    }
                }
            }
        }
    }
}

void __cdecl world_analysis() {
    for (int i = 0; i < MaxRegionNum; i++) {
        Continents[i].open_terrain = 0;
    }
    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y & 1; x < *MapAreaX; x += 2) {
            MAP* const sq = mapsq(x, y);
            assert(sq->region < MaxRegionNum);
            if (!is_ocean(sq) && sq->is_rainy_or_moist()
            && !sq->is_rocky() && !sq->is_fungus()) {
                Continents[sq->region].open_terrain++;
            }
            sq->items &= ~(BIT_DOUBLE_SEA | BIT_CANAL_COAST);
            if (!is_ocean(sq)) {
                int last_is_ocean = -1;
                int changes_inner = 0;
                int changes_outer = 0;
                for (int i = 1; i <= 20; i++) {
                    int nx = wrap(x + TableOffsetX[i]);
                    int ny = y + TableOffsetY[i];
                    MAP* const nsq = mapsq(nx, ny);
                    if (!nsq) {
                        continue;
                    }
                    int ocean = is_ocean(nsq);
                    if (i <= 8) {
                        if (ocean != last_is_ocean) {
                            last_is_ocean = ocean;
                            changes_inner++;
                            if (changes_inner == 4 && Continents[sq->region].tile_count >= 80) {
                                sq->items |= BIT_DOUBLE_SEA;
                            }
                        }
                    } else {
                        if (ocean != last_is_ocean || i == 9) {
                            last_is_ocean = ocean;
                            changes_outer++;
                        }
                        if (ny > 4 && ny < *MapAreaY - 4) {
                            if (nsq->region < MaxRegionLandNum && nsq->region != sq->region
                            && Continents[nsq->region].tile_count >= 40
                            && Continents[sq->region].tile_count >= 40) {
                                sq->items |= BIT_CANAL_COAST;
                            }
                        }
                    }
                }
                if (changes_outer < 4) {
                    sq->items &= ~BIT_DOUBLE_SEA;
                }
            }
        }
    }
    for (int i = 0; i < *MapAreaTiles; i++) {
        (*MapTiles)[i].val2 &= 0xF;
    }
}

void __cdecl world_rocky() {
    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y & 1; x < *MapAreaX; x += 2) {
            MAP* sq = mapsq(x, y);
            bool flag = sq->landmarks & LM_DISABLE;
            if ((flag || !(sq->landmarks & LM_CRATER)) || sq->code_at() >= 21) {
                if ((flag || !(sq->landmarks & LM_UNITY))
                || (sq->code_at() >= 9
                && sq->code_at() != 10
                && sq->code_at() != 12
                && sq->code_at() != 15
                && sq->code_at() != 16
                && sq->code_at() != 19
                && sq->code_at() != 20)) {
                    int value = mod_minerals_at(x, y);
                    assert(value == minerals_at(x, y));
                    // Distributed Uranium patches keep their natural
                    // Flat/Rolling/Rocky mix; legacy Uranium remains Rolling+.
                    if (!conf.modified_landmarks
                        && (sq->landmarks & (LM_URANIUM | LM_DISABLE)) == LM_URANIUM) {
                        value = max(value, static_cast<int>(LEVEL_ROLLING));
                    }
                    rocky_set(x, y, value);
                    if (!(sq->landmarks & LM_FRESH) || !is_ocean(sq)) {
                        if (is_ocean(sq)) {
                            bit_set(x, y, BIT_ENERGY_RES, 1);
                        } else if (flag || (!(sq->landmarks & LM_JUNGLE)
                            && minor_landmark_at(x, y) != MINOR_JUNGLE)) {
                            if (sq->is_rainy() || sq->alt_level() > ALT_ONE_ABOVE_SEA) {
                                bit_set(x, y, BIT_MINERAL_RES, 1);
                            }
                        } else if (!map_rand.get(2)) {
                            bit_set(x, y, BIT_ENERGY_RES, 1);
                        } else if (sq->is_rainy() || sq->alt_level() > ALT_ONE_ABOVE_SEA) {
                            bit_set(x, y, BIT_MINERAL_RES, 1);
                        }
                    }
                }
            }
        }
    }
}

void __cdecl world_riverbeds() {
    for (int i = 0; i < *MapAreaTiles; i++) {
        (*MapTiles)[i].items &= ~BIT_RIVER_SRC;
    }
    uint32_t riverbed_count = 0;
    uint32_t max_riverbeds = (*MapAreaTiles * ((4 - *MapOceanCoverage) * (WorldBuilder->rivers_base
        + *MapCloudCover * WorldBuilder->rivers_rain_mod) / 3)) / 3200;
    for (int i = 0; i < 4000 && riverbed_count < max_riverbeds; i++) {
        int x = map_rand.get(*MapAreaX);
        int y = map_rand.get(*MapAreaY);
        x = ((x ^ y) & 1) ^ x;
        MAP* sq = mapsq(x, y);
        if (!is_ocean(sq) && !(sq->items & BIT_RIVER_SRC)) {
            int search_alt = 0;
            int x_search = -1;
            int y_search = -1;
            for (int j = 0; j < TableRange[4]; j++) {
                int x2 = wrap(x + TableOffsetX[j]);
                int y2 = y + TableOffsetY[j];
                if ((sq = mapsq(x2, y2))) {
                    if (sq->items & BIT_RIVER_SRC) {
                        x_search = -1;
                        break;
                    }
                    if (j < TableRange[3]) {
                        int alt = sq->alt_level();
                        if (alt > search_alt) {
                            search_alt = alt;
                            x_search = x2;
                            y_search = y2;
                        }
                    }
                }
            }
            if (x_search >= 0) {
                bit_set(x_search, y_search, BIT_RIVER_SRC, true);
                riverbed_count++;
            }
        }
    }
}

void __cdecl world_shorelines() {
    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y & 1; x < *MapAreaX; x += 2) {
            int alt = alt_natural(x, y);
            alt_set(x, y, alt);
            if ((*GameState & STATE_OMNISCIENT_VIEW) && *WorldAddTemperature) {
                if (alt >= 3) {
                    temp_set(x, y, clamp(alt - 3, 0, 2));
                }
                draw_tile(x, y, 2);
            }
        }
    }
    for (int alt = conf.altitude_limit; alt >= 2; alt--) {
        for (int y = 0; y < *MapAreaY; y++) {
            for (int x = y & 1; x < *MapAreaX; x += 2) {
                if (alt_at(x, y) == alt) {
                    world_alt_set(x, y, alt, 1);
                }
            }
        }
    }
    if ((*GameState & STATE_OMNISCIENT_VIEW) && *WorldAddTemperature) {
        MapWin_clear_terrain(MapWin);
        draw_map(1);
    }
}

void __cdecl world_polar_caps() {
    int yd = *MapAreaY / 2 * 2;
    for (int x = 0; x < *MapAreaX; x += 2) {
        world_alt_put_detail(x, 0);
        world_alt_put_detail(x + 1, yd - 1);
    }
    int limit = *MapAreaX / 16;
    for (int i = 0; i < limit; i++) {
        world_alt_put_detail(map_rand.get(*MapAreaX / 2) * 2, 0);
        world_alt_put_detail(map_rand.get(*MapAreaX / 2) * 2 + 1, 1);
        world_alt_put_detail(map_rand.get(*MapAreaX / 2) * 2 + 1, yd - 1);
        world_alt_put_detail(map_rand.get(*MapAreaX / 2) * 2, yd - 2);
    }
}

void __cdecl world_linearize_contours() {
    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y & 1; x < *MapAreaX; x += 2) {
            int alt = alt_natural(x, y);
            int alt_val = (alt >= conf.altitude_limit ? 10 : alt + 1);
            int div_val = AltDefault[alt + 1] - AltDefault[alt];
            assert(div_val > 0);
            uint8_t detail = ElevDetail[alt]
                + ((alt_detail_at(x, y) - AltDefault[alt])
                * (ElevDetail[alt_val] - ElevDetail[alt]) / div_val);
            alt_put_detail(x, y, detail);
        }
    }
    memcpy(AltNatural, ElevDetail, sizeof(ElevDetail));
}

void __cdecl mod_world_crater(int x, int y) {
    if (!locate_landmark(&x, &y, false)) {
        return;
    }
    world_alt_set(x, y, ALT_OCEAN_TRENCH, true);
    world_alt_set(x, y, ALT_THREE_ABOVE_SEA, true);
    world_alt_set(x, y, ALT_SHORE_LINE, true);
    world_alt_set(x, y, ALT_OCEAN_SHELF, true);
    world_alt_set(x, y, ALT_SHORE_LINE, true);
    uint8_t temperature = temp_at(x, y);
    for (int i = 0; i < TableRange[3]; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        if (mapsq(x2, y2)) {
            bit_set(x2, y2, BIT_RIVER_SRC, false);
            bit2_set(x2, y2, LM_CRATER, true);
            code_set(x2, y2, i);
            if (i < 21) {
                rocky_set(x2, y2, LEVEL_ROLLING);
                temp_set(x2, y2, temperature);
            }
        }
    }
    name_landmark(x, y, LM_CRATER);
}

void __cdecl mod_world_monsoon(int x, int y) {
    MAP* sq;
    world_rainfall();
    if (!conf.modified_landmarks) {
        int attempts = 0;
        int land_count;
        if (!mapsq(x, y)) {
            do {
                y = *MapAreaY / 2 + map_rand.get(4) - 2;
                x = map_rand.get(*MapAreaX);
                x = ((x ^ y) & 1) ^ x;
                land_count = 0;
                for (int i = 0; i < TableRange[5]; i++) {
                    int x2 = wrap(x + TableOffsetX[i]);
                    int y2 = y + TableOffsetY[i];
                    if (!is_ocean(mapsq(x2, y2))) {
                        land_count++;
                    }
                }
                sq = mapsq(x, y);
                if (++attempts >= 1000) {
                    return;
                }
            } while (is_ocean(sq) || !is_coast(x, y, true)
            || land_count < 40 || !sq->is_rainy() || near_landmark(x, y));
        }
        for (int i = 0; i < TableRange[5]; i++) {
            int x2 = wrap(x + TableOffsetX[i]);
            int y2 = y + TableOffsetY[i];
            sq = mapsq(x2, y2);
            if (sq && abs(TableOffsetY[i]) <= 8) {
                if (i < 21 && is_ocean(sq)) {
                    world_alt_set(x2, y2, ALT_SHORE_LINE, true);
                }
                bit2_set(x2, y2, LM_JUNGLE, true);
                code_set(x2, y2, i);
            }
        }
        name_landmark(x, y, LM_JUNGLE);
        return;
    }
    // Use a smaller overall Jungle budget and grow several separated,
    // bounded patches instead of repeatedly stamping large random-walk areas.
    const int y_a = *MapAreaY * 5 / 16;
    const int y_b = *MapAreaY * 11 / 16;
    const int limit = max(1024, *MapAreaTiles) * (2 + *MapCloudCover) / 192;
    int num = 0;

    // An independent seed keeps changes to Jungle shape from shifting the
    // placement of later landmarks through the shared map generator RNG.
    GameRandom jungle_rand;
    jungle_rand.reseed(pair_hash((uint32_t)*MapRandomSeed ^ 0x4A554E47u,
        (uint32_t)*MapAreaTiles));

    auto eligible = [&](int tx, int ty) -> bool {
        MAP* tile = mapsq(tx, ty);
        return tile && ty >= y_a && ty <= y_b && !tile->lm_items()
            && minor_landmark_at(tx, ty) == MINOR_NONE
            && (tile->alt_level() == ALT_SHORE_LINE
                || tile->alt_level() == ALT_ONE_ABOVE_SEA);
    };

    // A candidate cannot be within two tiles of a previously completed
    // Jungle patch, leaving at least two ordinary tiles between patches.
    auto separated = [&](int tx, int ty) -> bool {
        for (auto& tile : iterate_tiles(tx, ty, 0, TableRange[2])) {
            if ((tile.sq->landmarks & LM_JUNGLE)
                || minor_landmark_at(tile.x, tile.y) == MINOR_JUNGLE) {
                return false;
            }
        }
        return true;
    };

    // Some candidate sites will be unusable on very dry or fragmented maps.
    // It is better to leave part of the budget unused than merge patches.
    for (int patch_no = 0; patch_no < 32 && num < limit; patch_no++) {
        if (limit - num < 6) {
            break; // Do not spend the remaining budget on tiny fragments
        }
        int seed_x = -1;
        int seed_y = -1;
        int best_score = -1;

        // Prefer pre-existing moist/rainy areas, but do not require them.
        // The mild coast bonus maintains Thinker's coastal-jungle tendency.
        for (int attempt = 0; attempt < 64; attempt++) {
            int sy = y_a + jungle_rand.get(y_b - y_a + 1);
            int sx = jungle_rand.get(*MapAreaX);
            sx = ((sx ^ sy) & 1) ^ sx;
            if (!eligible(sx, sy) || !separated(sx, sy)) {
                continue;
            }
            MAP* tile = mapsq(sx, sy);
            int score = jungle_rand.get(6)
                + (tile->is_rainy() ? 5 : (tile->is_moist() ? 3 : 0))
                + (is_coast(sx, sy, true) ? 1 : 0);
            if (score > best_score) {
                seed_x = sx;
                seed_y = sy;
                best_score = score;
            }
        }
        if (seed_x < 0) {
            break;
        }

        int target = 8 + jungle_rand.get(9); // Most patches: 8-16 tiles
        if (jungle_rand.get(6) == 0) {
            target = 17 + jungle_rand.get(6); // Occasionally: 17-22
        }
        target = min(target, limit - num);

        std::vector<Point> patch;
        std::set<Point> selected;
        patch.push_back({seed_x, seed_y});
        selected.insert({seed_x, seed_y});

        while ((int)patch.size() < target) {
            std::set<Point> frontier;
            for (const Point& from : patch) {
                for (auto& tile : iterate_tiles(from.x, from.y, 1, 9)) {
                    Point p = {tile.x, tile.y};
                    if (!selected.count(p) && eligible(p.x, p.y)
                        && separated(p.x, p.y)) {
                        frontier.insert(p);
                    }
                }
            }
            if (frontier.empty()) {
                break;
            }

            // Weight compact growth and naturally wet terrain, while
            // keeping enough randomness for irregular patch outlines.
            std::vector<std::pair<Point, int>> choices;
            int total_weight = 0;
            for (const Point& p : frontier) {
                int neighbours = 0;
                for (auto& tile : iterate_tiles(p.x, p.y, 1, 9)) {
                    neighbours += selected.count({tile.x, tile.y}) != 0;
                }
                MAP* tile = mapsq(p.x, p.y);
                int weight = 1 + (neighbours >= 2 ? 3 : 0)
                    + (tile->is_rainy() ? 2 : (tile->is_moist() ? 1 : 0));
                choices.push_back({p, weight});
                total_weight += weight;
            }

            int draw = jungle_rand.get(total_weight);
            Point chosen = choices.back().first;
            for (const auto& choice : choices) {
                if (draw < choice.second) {
                    chosen = choice.first;
                    break;
                }
                draw -= choice.second;
            }
            patch.push_back(chosen);
            selected.insert(chosen);
        }

        // Do not leave tiny isolated Jungle specks if a seed is blocked.
        if ((int)patch.size() < min(6, target)) {
            continue;
        }
        for (const Point& p : patch) {
            if (!minor_landmark_set(p.x, p.y, MINOR_JUNGLE,
                static_cast<uint8_t>(num % 121))) {
                debug("minor Jungle placement failed at %d,%d\n", p.x, p.y);
                return;
            }
            num++;
        }
    }
}

void __cdecl mod_world_sargasso(int x, int y) {
    if (!locate_landmark(&x, &y, true)) {
        return;
    }
    bool has_skipped = false;
    for (int i = 0; i < TableRange[4]; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        MAP* sq = mapsq(x2, y2);
        if (sq && (is_ocean(sq) || i < 21)) {
            world_alt_set(x2, y2, ALT_OCEAN_SHELF, true);
            bit2_set(x2, y2, LM_SARGASSO, true);
            code_set(x2, y2, i);
            if (i <= 4) {
                bit_set(x2, y2, BIT_SUPPLY_POD, true);
            }
            if ((has_skipped || i < TableRange[2] || map_rand.get(24))
            && (i < TableRange[3] || map_rand.get(3))) {
                bit_set(x2, y2, BIT_FUNGUS, true);
            } else {
                has_skipped = true;
            }
        }
    }
    name_landmark(x, y, LM_SARGASSO);
}

void __cdecl mod_world_ruin(int x, int y) {
    if (!locate_landmark(&x, &y, false)) {
        return;
    }
    for (int i = 0; i < TableRange[2]; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        MAP* sq = mapsq(x2, y2);
        if (sq) {
            world_alt_set(x2, y2, ALT_SHORE_LINE, true);
            if (is_ocean(sq)) {
                if (i && i < 9) {
                    bit_set(x2, y2, BIT_MONOLITH, true);
                }
            } else {
                bit_set(x2, y2, BIT_FUNGUS | BIT_SUPPLY_REMOVE, true);
                if (!i) {
                    bit_set(x2, y2, BIT_FUNGUS, false);
                    if (sq->is_rocky()) { // Remove rocky tiles from the center
                        rocky_set(x2, y2, LEVEL_FLAT);
                    }
                } else if (i < 9) {
                    bit_set(x2, y2, BIT_MONOLITH, true);
                }
            }
            bit2_set(x2, y2, LM_RUINS, true);
            code_set(x2, y2, i);
        }
    }
    name_landmark(x, y, LM_RUINS);
}

void __cdecl mod_world_dune(int x, int y) {
    world_rainfall();

    // Keep a solid 3x3 core, but let the perimeter vary between maps. Shape
    // randomness uses its own generator so it does not perturb landmark RNG.
    const bool fixed_location = mapsq(x, y) != NULL;
    const int attempts = fixed_location ? 1 : 1000;
    int oasis_i = -1;
    GameRandom shape_rand;

    for (int attempt = 0; attempt < attempts; attempt++) {
        if (!fixed_location) {
            int half_height = *MapAreaY / 2;
            y = map_rand.get(half_height) + half_height - *MapAreaY / 4;
            x = map_rand.get(*MapAreaX);
            x = ((x ^ y) & 1) ^ x;
        }
        MAP* center = mapsq(x, y);
        if (!center || is_ocean(center) || center->is_rainy_or_moist()
            || near_landmark(x, y)) {
            continue;
        }

        // The 3x3 core must exist entirely on land. Raising the two dune
        // crests would otherwise reclaim neighbouring ocean squares.
        bool valid = true;
        for (int i = 0; i < 9; i++) {
            MAP* tile = mapsq(wrap(x + TableOffsetX[i]), y + TableOffsetY[i]);
            if (!tile || is_ocean(tile) || (tile->lm_items() & ~LM_JUNGLE)) {
                valid = false;
                break;
            }
        }
        if (!valid) {
            continue;
        }
        const int crest1_x = wrap(x + TableOffsetX[2]);
        const int crest1_y = y + TableOffsetY[2];
        const int crest2_x = wrap(x + TableOffsetX[7]);
        const int crest2_y = y + TableOffsetY[7];
        if (is_coast(crest1_x, crest1_y, false)
            || is_coast(crest2_x, crest2_y, false)) {
            continue;
        }

        shape_rand.reseed(pair_hash(*MapRandomSeed ^ 0xD0E5u,
            (uint32_t(x) << 16) ^ uint32_t(y)));

        // Reserve a truly inland oasis before removing any perimeter tiles.
        // Prefer the core if a site is far enough from both raised crests.
        std::vector<int> inner_oases;
        std::vector<int> outer_oases;
        for (int i = 0; i < TableRange[2]; i++) {
            if (i == 2 || i == 7 || i == 4) {
                continue; // Three energy deposits
            }
            int x2 = wrap(x + TableOffsetX[i]);
            int y2 = y + TableOffsetY[i];
            MAP* tile = mapsq(x2, y2);
            if (!tile || is_ocean(tile) || (tile->lm_items() & ~LM_JUNGLE)
                || is_coast(x2, y2, false)
                || map_range(x2, y2, crest1_x, crest1_y) <= 1
                || map_range(x2, y2, crest2_x, crest2_y) <= 1) {
                continue;
            }
            if (i < 9) {
                inner_oases.push_back(i);
            } else {
                outer_oases.push_back(i);
            }
        }
        const std::vector<int>& candidates =
            !inner_oases.empty() ? inner_oases : outer_oases;
        if (candidates.empty()) {
            continue;
        }
        oasis_i = candidates[shape_rand.get((int)candidates.size())];
        break;
    }
    if (oasis_i < 0) {
        return;
    }

    std::vector<uint8_t> footprint(TableRange[3], 0);
    for (int i = 0; i < 9; i++) {
        footprint[i] = 1;
    }

    // Start with the available land in the 5x5 middle ring. Ocean and
    // other landmarks remain boundaries, while Jungle can be replaced.
    int middle_count = 0;
    for (int i = 9; i < TableRange[2]; i++) {
        MAP* tile = mapsq(wrap(x + TableOffsetX[i]), y + TableOffsetY[i]);
        if (tile && !is_ocean(tile) && !(tile->lm_items() & ~LM_JUNGLE)) {
            footprint[i] = 1;
            middle_count++;
        }
    }
    footprint[oasis_i] = 1;

    // Cut a few short, connected notches into the middle-ring boundary.
    // Coastal gaps count toward irregularity, so keep smaller coastal
    // rings more intact instead of making the Dunes disappear.
    int remove_budget = min(3 + shape_rand.get(3),
        max(0, middle_count - max(6, (middle_count * 2 + 2) / 3)));
    for (int notch = 0; notch < 3 && remove_budget > 0; notch++) {
        std::vector<int> anchors;
        for (int i = 9; i < TableRange[2]; i++) {
            if (footprint[i] && i != oasis_i) {
                anchors.push_back(i);
            }
        }
        if (anchors.empty()) {
            break;
        }
        int anchor = anchors[shape_rand.get((int)anchors.size())];
        footprint[anchor] = 0;
        remove_budget--;

        // Sometimes extend the notch by removing an adjacent outer tile.
        if (remove_budget > 0 && shape_rand.get(4) != 0) {
            std::vector<int> neighbours;
            int ax = wrap(x + TableOffsetX[anchor]);
            int ay = y + TableOffsetY[anchor];
            for (int j = 9; j < TableRange[2]; j++) {
                if (footprint[j] && j != oasis_i
                    && map_range(ax, ay, wrap(x + TableOffsetX[j]),
                        y + TableOffsetY[j]) <= 1) {
                    neighbours.push_back(j);
                }
            }
            if (!neighbours.empty()) {
                footprint[neighbours[shape_rand.get((int)neighbours.size())]] = 0;
                remove_budget--;
            }
        }
    }

    // Add one to four small, connected protrusions from the 7x7 ring.
    // Prefer tiles touching at least two existing Dunes tiles, avoiding
    // isolated fingers. Never fill coastal ocean or another landmark.
    int fringe_count = 1 + shape_rand.get(4);
    for (int added = 0; added < fringe_count; added++) {
        std::vector<int> good;
        std::vector<int> possible;
        for (int i = TableRange[2]; i < TableRange[3]; i++) {
            int x2 = wrap(x + TableOffsetX[i]);
            int y2 = y + TableOffsetY[i];
            MAP* tile = mapsq(x2, y2);
            if (!tile || is_ocean(tile) || (tile->lm_items() & ~LM_JUNGLE) || footprint[i]) {
                continue;
            }
            int adjacent = 0;
            for (int j = 0; j < TableRange[3]; j++) {
                if (footprint[j] && map_range(x2, y2,
                    wrap(x + TableOffsetX[j]), y + TableOffsetY[j]) <= 1) {
                    adjacent++;
                }
            }
            if (adjacent > 0) {
                possible.push_back(i);
                if (adjacent >= 2) {
                    good.push_back(i);
                }
            }
        }
        const std::vector<int>& choices = !good.empty() ? good : possible;
        if (choices.empty()) {
            break;
        }
        footprint[choices[shape_rand.get((int)choices.size())]] = 1;
    }

    // All chosen tiles are land before this point. Lower the footprint
    // first, then raise the two crests to retain their energy advantage.
    for (int i = 0; i < TableRange[3]; i++) {
        if (!footprint[i]) {
            continue;
        }
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        world_alt_set(x2, y2, ALT_SHORE_LINE, true);
        bit_set(x2, y2, BIT_FUNGUS, false);
        bit2_set(x2, y2, LM_DUNES, true);
        rocky_set(x2, y2, LEVEL_FLAT);
        code_set(x2, y2, i);

        // Only the three energy deposits and the freshwater oasis
        // may have resource bonuses anywhere inside Great Dunes.
        MAP* tile = mapsq(x2, y2);
        tile->items &= ~(BIT_BONUS_RES | BIT_NUTRIENT_RES
            | BIT_MINERAL_RES | BIT_ENERGY_RES);
        if (i == 2 || i == 7 || i == 4) {
            tile->items |= BIT_BONUS_RES | BIT_ENERGY_RES;
        }
    }
    for (int i = 0; i < 2; i++) {
        const int crest_i = i == 0 ? 2 : 7;
        world_alt_set(wrap(x + TableOffsetX[crest_i]),
            y + TableOffsetY[crest_i], ALT_ONE_ABOVE_SEA, true);
    }

    // The shallow inland lake is the sole nutrient resource.
    int oasis_x = wrap(x + TableOffsetX[oasis_i]);
    int oasis_y = y + TableOffsetY[oasis_i];
    world_alt_set(oasis_x, oasis_y, ALT_OCEAN_SHELF, true);
    bit2_set(oasis_x, oasis_y, LM_FRESH, true);
    bit_set(oasis_x, oasis_y, BIT_BONUS_RES | BIT_NUTRIENT_RES, true);
    name_landmark(x, y + 2, LM_DUNES);
}


/*
Independent late minor terrain generation. All minor types occupy their
own type/code per tile and never set native LM_* bits or landmark names.
Jungle uses the climate-aware mod_world_monsoon generator in an earlier pass.
*/
static void generate_minor_ironstone() {
    if (!*MapTiles || *MapAreaTiles <= 0 || *MapAreaX < 2 || *MapAreaY < 2) {
        return;
    }
    // The normal landmark generator skips mirror worlds. Keep the minor
    // generator disabled there until symmetrical patch placement is supported.
    if (conf.world_mirror_x || conf.world_mirror_y) {
        return;
    }

    // The same 3-9 weighted patch sizes as the distributed Uranium generator:
    // an exact expected patch size of six tiles.
    const int patch_sizes[17] = {
        3, 4, 4, 5, 5, 5, 6, 6, 6, 6, 6, 7, 7, 7, 8, 8, 9
    };
    const int patch_count = max(2, min(6, (*MapAreaTiles + 400) / 800));
    GameRandom iron_rand;
    iron_rand.reseed(pair_hash((uint32_t)*MapRandomSeed ^ 0x49524F4Eu,
        (uint32_t)*MapAreaTiles));

    auto eligible = [&](int tx, int ty) -> bool {
        MAP* tile = mapsq(tx, ty);
        // Preserve the original terrain and avoid all existing landmark
        // footprints, fungus, oceans and previously placed minor features.
        return tile && !is_ocean(tile) && tile->is_land_region()
            && !tile->is_fungus() && !tile->lm_items()
            && minor_landmark_at(tx, ty) == MINOR_NONE;
    };
    auto separated = [&](int tx, int ty) -> bool {
        // Two-tile buffer between completed minor patches and legacy
        // native Jungle/Uranium footprints.
        for (auto& tile : iterate_tiles(tx, ty, 0, TableRange[2])) {
            if (minor_landmark_at(tile.x, tile.y) != MINOR_NONE
                || (tile.sq->landmarks & (LM_JUNGLE | LM_URANIUM))) {
                return false;
            }
        }
        return true;
    };

    std::vector<Point> centers;
    int completed = 0;
    for (int attempt_patch = 0;
        attempt_patch < patch_count * 8 && completed < patch_count;
        attempt_patch++) {
        int seed_x = -1;
        int seed_y = -1;
        int best_score = -1;

        // Prefer naturally rolling/rocky areas and keep separate patches
        // geographically dispersed, without requiring particular rockiness.
        for (int attempt = 0; attempt < 96; attempt++) {
            int sy = iron_rand.get(*MapAreaY);
            int sx = iron_rand.get(*MapAreaX);
            sx = ((sx ^ sy) & 1) ^ sx;
            if (!eligible(sx, sy) || !separated(sx, sy)) {
                continue;
            }
            MAP* tile = mapsq(sx, sy);
            int score = iron_rand.get(8)
                + (tile->is_rocky() ? 6 : (tile->is_rolling() ? 4 : 0));
            if (!centers.empty()) {
                int min_distance = 9999;
                for (const Point& center : centers) {
                    min_distance = min(min_distance,
                        map_range(sx, sy, center.x, center.y));
                }
                score += 2 * min(min_distance, 40);
            }
            if (score > best_score) {
                best_score = score;
                seed_x = sx;
                seed_y = sy;
            }
        }
        if (seed_x < 0) {
            break;
        }

        const int target = patch_sizes[iron_rand.get(17)];
        std::vector<Point> patch;
        std::set<Point> selected;
        patch.push_back({seed_x, seed_y});
        selected.insert({seed_x, seed_y});

        while ((int)patch.size() < target) {
            std::set<Point> frontier;
            for (const Point& from : patch) {
                for (auto& tile : iterate_tiles(from.x, from.y, 1, 9)) {
                    Point p = {tile.x, tile.y};
                    if (!selected.count(p) && eligible(p.x, p.y)
                        && separated(p.x, p.y)) {
                        frontier.insert(p);
                    }
                }
            }
            if (frontier.empty()) {
                break;
            }

            // Compact irregular growth with a mild rocky/rolling preference.
            std::vector<std::pair<Point, int>> choices;
            int total_weight = 0;
            for (const Point& p : frontier) {
                int neighbors = 0;
                for (auto& tile : iterate_tiles(p.x, p.y, 1, 9)) {
                    neighbors += selected.count({tile.x, tile.y}) != 0;
                }
                MAP* tile = mapsq(p.x, p.y);
                int weight = 1 + (neighbors >= 2 ? 3 : 0)
                    + (tile->is_rocky() ? 3 : (tile->is_rolling() ? 2 : 0));
                choices.push_back({p, weight});
                total_weight += weight;
            }

            int draw = iron_rand.get(total_weight);
            Point chosen = choices.back().first;
            for (const auto& choice : choices) {
                if (draw < choice.second) {
                    chosen = choice.first;
                    break;
                }
                draw -= choice.second;
            }
            patch.push_back(chosen);
            selected.insert(chosen);
        }

        // Reject incomplete patches, rather than producing small fragments
        // that distort the intended average patch size.
        if ((int)patch.size() != target) {
            continue;
        }
        for (int i = 0; i < (int)patch.size(); ++i) {
            const Point& p = patch[i];
            if (!minor_landmark_set(p.x, p.y, MINOR_IRONSTONE,
                static_cast<uint8_t>(i))) {
                debug("minor Ironstone placement failed at %d,%d\n", p.x, p.y);
                return;
            }
        }
        centers.push_back({seed_x, seed_y});
        completed++;
        debug("minor Ironstone patch %d/%d: %d tiles at %d,%d\n",
            completed, patch_count, target, seed_x, seed_y);
    }
    debug("minor Ironstone generation complete: %d/%d patches\n",
        completed, patch_count);
}

// Late minor terrain is placed after final climate/rockiness. Jungle is
// generated before the native major landmarks so it can influence rainfall.
static void generate_minor_landmarks() {
    if (conf.modified_landmarks && conf.minor_uranium) {
        mod_world_diamond(-1, -1);
    }
    if (conf.minor_ironstone) {
        generate_minor_ironstone();
    }
}

void __cdecl mod_world_diamond(int x, int y) {
    // Preserve legacy generation for unmodified landmarks and fixed
    // locations (e.g. scenario/editor placement).
    if (!conf.modified_landmarks || mapsq(x, y)) {
        if (!locate_landmark(&x, &y, false)) {
            return;
        }
        for (int i = 0; i < 21; i++) {
            int x2 = wrap(x + TableOffsetX[i]);
            int y2 = y + TableOffsetY[i];
            MAP* sq = mapsq(x2, y2);
            if (sq) {
                if (i < 9 || !is_ocean(sq) || map_rand.get(3)) {
                    world_alt_set(x2, y2, ALT_SHORE_LINE, true);
                    bit2_set(x2, y2, LM_URANIUM, true);
                    rocky_set(x2, y2, LEVEL_FLAT);
                    bit_set(x2, y2, BIT_FUNGUS, false);
                    code_set(x2, y2, i);
                }
            }
        }
        name_landmark(x, y, LM_URANIUM);
        return;
    }

    // Uranium is distributed as compact, irregular geological deposits
    // rather than a single 21-tile landmark. Patch count scales by map area;
    // the weighted size distribution has a mean of six (range 3-9).
    const int patch_count = max(2, min(6, (*MapAreaTiles + 400) / 800));
    const int patch_sizes[17] = {
        3, 4, 4, 5, 5, 5, 6, 6, 6, 6, 6, 7, 7, 7, 8, 8, 9
    };
    GameRandom uranium_rand;
    uranium_rand.reseed(pair_hash((uint32_t)*MapRandomSeed ^ 0x5552414Eu,
        (uint32_t)*MapAreaTiles));

    auto eligible = [&](int tx, int ty) -> bool {
        MAP* tile = mapsq(tx, ty);
        // This excludes Jungle entirely, including its zero-code tiles.
        return tile && !is_ocean(tile) && !tile->lm_items()
            && minor_landmark_at(tx, ty) == MINOR_NONE;
    };

    // Keep two ordinary tiles between distinct Uranium patches, Jungle,
    // and previously generated landmarks. Current patch tiles are uncommitted,
    // so each patch can grow contiguously without blocking itself.
    auto separated = [&](int tx, int ty) -> bool {
        for (auto& tile : iterate_tiles(tx, ty, 0, TableRange[2])) {
            if (tile.sq->lm_items()
                || minor_landmark_at(tile.x, tile.y) != MINOR_NONE) {
                return false;
            }
        }
        return true;
    };

    std::vector<Point> centers;
    int completed = 0;
    for (int attempt_patch = 0;
        attempt_patch < patch_count * 8 && completed < patch_count;
        attempt_patch++) {
        int seed_x = -1;
        int seed_y = -1;
        int best_score = -1;

        // Seek widely separated patches, preferring lowland locations
        // without requiring a particular elevation or moisture level.
        for (int attempt = 0; attempt < 96; attempt++) {
            int sy = uranium_rand.get(*MapAreaY);
            int sx = uranium_rand.get(*MapAreaX);
            sx = ((sx ^ sy) & 1) ^ sx;
            if (!eligible(sx, sy) || !separated(sx, sy)) {
                continue;
            }
            MAP* tile = mapsq(sx, sy);
            int alt = tile->alt_level();
            int score = uranium_rand.get(8)
                + (alt == ALT_SHORE_LINE ? 6
                    : (alt == ALT_ONE_ABOVE_SEA ? 4
                    : (alt == ALT_TWO_ABOVE_SEA ? 2 : 0)));
            if (!centers.empty()) {
                int min_distance = 9999;
                for (const Point& center : centers) {
                    min_distance = min(min_distance,
                        map_range(sx, sy, center.x, center.y));
                }
                score += 2 * min(min_distance, 40);
            }
            if (score > best_score) {
                best_score = score;
                seed_x = sx;
                seed_y = sy;
            }
        }
        if (seed_x < 0) {
            break;
        }

        const int target = patch_sizes[uranium_rand.get(17)];
        std::vector<Point> patch;
        std::set<Point> selected;
        patch.push_back({seed_x, seed_y});
        selected.insert({seed_x, seed_y});

        while ((int)patch.size() < target) {
            std::set<Point> frontier;
            for (const Point& from : patch) {
                for (auto& tile : iterate_tiles(from.x, from.y, 1, 9)) {
                    Point p = {tile.x, tile.y};
                    if (!selected.count(p) && eligible(p.x, p.y)
                        && separated(p.x, p.y)) {
                        frontier.insert(p);
                    }
                }
            }
            if (frontier.empty()) {
                break;
            }

            std::vector<std::pair<Point, int>> choices;
            int total_weight = 0;
            for (const Point& p : frontier) {
                int neighbors = 0;
                for (auto& tile : iterate_tiles(p.x, p.y, 1, 9)) {
                    neighbors += selected.count({tile.x, tile.y}) != 0;
                }
                MAP* tile = mapsq(p.x, p.y);
                int alt = tile->alt_level();
                int weight = 1 + (neighbors >= 2 ? 3 : 0)
                    + (alt == ALT_SHORE_LINE ? 3
                        : (alt == ALT_ONE_ABOVE_SEA ? 2 : 0));
                choices.push_back({p, weight});
                total_weight += weight;
            }

            int draw = uranium_rand.get(total_weight);
            Point chosen = choices.back().first;
            for (const auto& choice : choices) {
                if (draw < choice.second) {
                    chosen = choice.first;
                    break;
                }
                draw -= choice.second;
            }
            patch.push_back(chosen);
            selected.insert(chosen);
        }

        // Only commit complete patches: retry instead of leaving a
        // truncated deposit that drags down the six-tile average.
        if ((int)patch.size() != target) {
            continue;
        }
        for (int i = 0; i < (int)patch.size(); i++) {
            const Point& p = patch[i];
            if (!minor_landmark_set(p.x, p.y, MINOR_URANIUM,
                static_cast<uint8_t>(i))) {
                debug("minor Uranium placement failed at %d,%d\n", p.x, p.y);
                return;
            }
            bit_set(p.x, p.y, BIT_FUNGUS, false);
        }
        centers.push_back({seed_x, seed_y});
        completed++;
    }
}

void __cdecl mod_world_fresh(int x, int y) {
    int region;
    if (mapsq(x, y)) {
        region = region_at(x, y);
        if (!is_ocean(mapsq(x, y))) {
            return;
        }
    } else {
        int tile_search = 0;
        int region_search = -1;
        for (int i = MaxRegionLandNum; i < MaxRegionNum; i++) {
            int tile_count = Continents[i].tile_count;
            if (tile_count >= 3 && tile_count <= 32 && tile_count >= tile_search) {
                tile_search = tile_count;
                region_search = i;
            }
        }
        if (region_search < 0) {
            return;
        }
        region = region_search;
    }
    int x_search = -1;
    bool has_landmark = false;
    for (int y_it = *MapAreaY - 1; y_it >= 0 ; y_it--) {
        for (int x_it = y_it & 1; x_it < *MapAreaX; x_it += 2) {
            if (region_at(x_it, y_it) == region) {
                bit2_set(x_it, y_it, LM_FRESH, true);
                if (x_search < 0) {
                    x_search = x_it;
                }
            } else if (!has_landmark && x_search >= 0) {
                int x_fresh = (x_search + x_it - 2) / 2;
                name_landmark(((x_fresh ^ y_it) & 1) ^ x_fresh, y_it, LM_FRESH);
                has_landmark = true;
            }
        }
    }
}

void __cdecl mod_world_volcano(int x, int y, int skip_label) {
    if (!locate_landmark(&x, &y, false)) {
        return;
    }
    world_alt_set(x, y, ALT_OCEAN_TRENCH, true);
    world_alt_set(x, y, ALT_THREE_ABOVE_SEA, true);
    for (int i = 0; i < TableRange[1]; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        if (mapsq(x2, y2)) {
            bit2_set(x2, y2, LM_VOLCANO, true);
            code_set(x2, y2, i);
            uint32_t items = bit_at(x2, y2) & ~(BIT_SUPPLY_POD | BIT_THERMAL_BORE
                | BIT_ECH_MIRROR | BIT_CONDENSER | BIT_SOIL_ENRICHER | BIT_FARM | BIT_RIVER_SRC
                | BIT_SOLAR | BIT_FUNGUS | BIT_MINE | BIT_MAGTUBE | BIT_ROAD);
            if (!i) {
                items &= ~(BIT_UNK_4000000 | BIT_UNK_8000000);
            }
            items |= BIT_SUPPLY_REMOVE;
            bit_put(x2, y2, items);
            rocky_set(x2, y2, LEVEL_ROCKY);
        }
    }
    *MountPlanetX = x;
    *MountPlanetY = y;
    if (!skip_label) {
        name_landmark(x, y, LM_VOLCANO);
    }
}

/*
Setup the 'Borehole Cluster' landmark. Added to SMAC in 3.0 patch.
*/
void __cdecl mod_world_borehole(int x, int y) {
    if (!locate_landmark(&x, &y, false)) {
        return;
    }
    // Replace inconsistent timeGetTime() values used for seeding
    uint32_t seed = map_rand.get(256);
    int val0 = (seed / 8) % 4;
    int val1 = 8;
    int val2 = ((seed % 8) / 3) + 5;
    int val3 = 3 - ((seed % 8) % 3);
    int val4 = -1;

    if (val0 & 2) {
        val2--;
        val3--;
        if (val0 & 1) {
            val1 = 8;
            val2++;
            val3++;
        } else {
            val1 = 6;
            val2--;
            val3--;
        }
        val1 = (val1 + 8) % 8 + 1;
        val2 = (val2 + 8) % 8 + 1;
        val3 = (val3 + 8) % 8 + 1;
    }
    if (conf.modified_landmarks) {
        val1 = 2;
        val2 = 4;
        val3 = 6;
        val4 = 8;
    }
    for (int i = 0; i < 9; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        if (mapsq(x2, y2)) {
            world_alt_set(x2, y2, ALT_SHORE_LINE, true);
            bit_set(x2, y2, BIT_SUPPLY_REMOVE, true);
            if (i == val1 || i == val2 || i == val3 || i == val4) {
                bit_set(x2, y2, BIT_THERMAL_BORE, true);
                bit2_set(x2, y2, LM_BOREHOLE, true);
                code_set(x2, y2, i);
            }
        }
    }
    for (auto offset : {val1, val2, val3, val4}) {
        if (offset > 0) {
            int x2 = wrap(x + TableOffsetX[offset]);
            int y2 = y + TableOffsetY[offset];
            for (int i = 1; i < 9; i++) {
                int x3 = wrap(x2 + TableOffsetX[i]);
                int y3 = y2 + TableOffsetY[i];
                if (mapsq(x3, y3) && is_ocean(mapsq(x3, y3))) {
                    world_alt_set(x3, y3, ALT_SHORE_LINE, true);
                }
            }
        }
    }
    bit2_set(x, y, LM_BOREHOLE, true);
    name_landmark(x, y, LM_BOREHOLE);
}

/*
Setup 'The Manifold Nexus' landmark. Added to SMAC in 4.0 patch.
*/
void __cdecl mod_world_temple(int x, int y) {
    if (!locate_landmark(&x, &y, false)) {
        return;
    }
    for (int i = 0; i < TableRange[1]; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        if (mapsq(x2, y2)) {
            world_alt_set(x2, y2, ALT_SHORE_LINE, true);
            bit_set(x2, y2, BIT_SUPPLY_REMOVE, true);
            bit2_set(x2, y2, LM_NEXUS, true);
            code_set(x2, y2, i);
        }
    }
    name_landmark(x, y, LM_NEXUS);
}

/*
Setup the 'Unity Wreckage' landmark (SMACX only).
*/
void __cdecl mod_world_unity(int x, int y) {
    if (!locate_landmark(&x, &y, false)) {
        return;
    }
    x--;
    y--;
    for (int i = 0; i < TableRange[2]; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        if (mapsq(x2, y2)) {
            world_alt_set(x2, y2, ALT_ONE_ABOVE_SEA, true);
        }
    }
    x += 2;
    y += 2;
    for (int i = 0; i < TableRange[2]; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        if (mapsq(x2, y2)) {
            world_alt_set(x2, y2, ALT_ONE_ABOVE_SEA, true);
        }
    }
    x--;
    y--;
    for (int i = 0; i < TableRange[2]; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        if (mapsq(x2, y2)) {
            rocky_set(x2, y2, LEVEL_FLAT);
            bit_set(x2, y2, BIT_RIVER_SRC, false);
            bit_set(x2, y2, BIT_FUNGUS, false);
            bit2_set(x2, y2, LM_UNITY, true);
            code_set(x2, y2, i);
            if (!i || i == 8 || i == 10 || i == 19) {
                bit_set(x2, y2, BIT_SUPPLY_POD, true);
            } else {
                bit_set(x2, y2, BIT_SUPPLY_REMOVE, true);
            }
        }
    }
    name_landmark(x, y, LM_UNITY);
}

/*
Setup the 'Fossil Ridge' landmark (SMACX only).
*/
void __cdecl mod_world_fossil(int x, int y) {
    if (!locate_landmark(&x, &y, true)) {
        return;
    }
    for (int i = 0; i < 6; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        MAP* sq = mapsq(x2, y2);
        if (sq && is_ocean(sq)) {
            world_alt_set(x2, y2, ALT_OCEAN, true);
            if (conf.modified_landmarks) {
                world_alt_set(x2, y2, ALT_OCEAN_SHELF, true);
            }
            bit2_set(x2, y2, LM_FOSSIL, true);
            code_set(x2, y2, i);
        }
    }
    name_landmark(x, y, LM_FOSSIL);
}

void __cdecl mod_world_canyon(int x, int y) {
    if (!locate_landmark(&x, &y, false)) {
        return;
    }
    const int WorldCanyonOffset[12] = {26, 27, 9, 1, 2, 0, 3, 4, 17, 23, 36, 35};
    for (int i = 0; i < 12; i++) {
        int x2 = wrap(x + TableOffsetX[WorldCanyonOffset[i]]);
        int y2 = y + TableOffsetY[WorldCanyonOffset[i]];
        if (mapsq(x2, y2)) {
            world_alt_set(x2, y2, ALT_ONE_ABOVE_SEA, true);
            world_alt_set(x2, y2, ALT_SHORE_LINE, true);
            bit2_set(x2, y2, LM_CANYON, true); // rolled these two into single loop with
            code_set(x2, y2, WorldCanyonOffset[i]); // the above world_alt_set()
        }
    }
    name_landmark(x, y + 2, LM_CANYON);
}

void __cdecl mod_world_mesa(int x, int y) {
    if (!locate_landmark(&x, &y, false)) {
        return;
    }
    for (int i = 0; i < TableRange[3]; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        if (mapsq(x2, y2)) {
            world_alt_set(x2, y2, (i < TableRange[2]) + ALT_SHORE_LINE, true);
            bit2_set(x2, y2, LM_MESA, true);
            code_set(x2, y2, i);
        }
    }
    name_landmark(x, y + 2, LM_MESA);
}

void __cdecl mod_world_ridge(int x, int y) {
    if (!locate_landmark(&x, &y, false)) {
        return;
    }
    const int WorldRidgeOffset[13] = {47, 44, 24, 20, 8, 7, 0, 5, 4, 17, 23, 35, 45};
    for (int i = 0; i < 13; i++) {
        int x2 = wrap(x + TableOffsetX[WorldRidgeOffset[i]]);
        int y2 = y + TableOffsetY[WorldRidgeOffset[i]];
        if (mapsq(x2, y2)) {
            world_alt_set(x2, y2, ALT_TWO_ABOVE_SEA, true);
            bit2_set(x2, y2, LM_RIDGE, true); // rolled these two into single loop with
            code_set(x2, y2, WorldRidgeOffset[i]); // the above world_alt_set()
        }
    }
    name_landmark(x, y + 2, LM_RIDGE);
}

void __cdecl mod_world_geothermal(int x, int y) {
    if (!locate_landmark(&x, &y, true)) {
        return;
    }
    bool has_skipped = false;
    for (int i = 0; i < TableRange[2]; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        MAP* sq = mapsq(x2, y2);
        if (sq && (is_ocean(sq) || i < 9)) {
            if ((has_skipped || !i || map_rand.get(25)) && (i < 9 || map_rand.get(3))) {
                world_alt_set(x2, y2, ALT_OCEAN_SHELF, true);
                bit2_set(x2, y2, LM_GEOTHERMAL, true);
                code_set(x2, y2, i);
            } else {
                has_skipped = true;
            }
        }
    }
    name_landmark(x, y, LM_GEOTHERMAL);
}

static float world_fractal(FastNoiseLite& noise, float w, int x, int y) {
    float value = 1.0f * noise.GetNoise(1.5f*w*x, w*y)
        + 0.3f * (2 + *MapErosiveForces) * noise.GetNoise(-6.0f*x, -4.0f*y);
    if (x < 8 && *MapAreaX >= 16 && !map_is_flat()) {
        value = x/8.0f * value + (1.0f - x/8.0f) * world_fractal(noise, w, *MapAreaX + x, y);
    }
    return value;
}

/*
Replace original WorldBuilder fungus placement method. This still produces similar
fungus density given the same parameters but the placement patterns are altered.
*/
static void mod_world_fungus(FastNoiseLite& noise) {
    int WBvalue = WorldBuilder->fungus * (*MapNativeLifeForms - 1);
    int FTvalue = conf.spawn_fungal_towers > 1 ? clamp(conf.spawn_fungal_towers, 8, 1024) : 40;
    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y&1; x < *MapAreaX; x+=2) {
            MAP* sq = mapsq(x, y);
            int value = clamp((int)(50.0f + 50.0f*noise.GetNoise(-6.0f*(256+x), 6.0f*y)), 0, 100);
            if ((sq->landmarks & (LM_VOLCANO|LM_DISABLE)) != LM_VOLCANO || sq->code_at() >= 0x90) {
                if ((value < 20 - WBvalue || value > 2 * WBvalue + 25)
                && (value < 40 - WBvalue || value > 2 * WBvalue + 45)
                && (value < 60 - WBvalue || value > 2 * WBvalue + 65)) {
                    if (y && y != *MapAreaY - 1) {
                        continue;
                    }
                    if (!map_rand.get(2)) {
                        continue;
                    }
                }
                if (!sq->is_rocky()) { // Later world_rocky can change the tile
                    sq->items |= BIT_FUNGUS;
                    sq->items &= ~TerraformRules[FORMER_PLANT_FUNGUS][1];
                    if (!map_rand.get(FTvalue) && !is_ocean(sq)
                    && *ExpansionEnabled && conf.spawn_fungal_towers) {
                        veh_init(BSC_FUNGAL_TOWER, 0, x, y);
                    }
                }
            }
        }
    }
}

void __cdecl mod_world_build() {
    if (!conf.new_world_builder) {
        ThinkerVars->map_random_value = 0;
        world_build();
        return;
    }
    world_generate(next_rand());
}

void console_world_generate(uint32_t seed) {
    if (*GameState & STATE_SCENARIO_EDITOR && *GameState & STATE_OMNISCIENT_VIEW) {
        *VehCount = 0;
        *BaseCount = 0;
        MapWin->InVehMode = 0;
        MapWin->VehID = -1;
        *GameState |= STATE_UNK_4;
        *GameState &= ~STATE_OMNISCIENT_VIEW;
        world_generate(seed);
        *GameState |= STATE_OMNISCIENT_VIEW;
        draw_map(1);
        GraphicWin_redraw(WorldWin);
    }
}

void world_generate(uint32_t seed) {
    if (DEBUG) {
        *GameState |= STATE_DEBUG_MODE;
    }
    MAP* sq;
    map_wipe();
    ThinkerVars->map_random_value = seed;
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetSeed(seed);
    map_rand.reseed(seed);

    memcpy(AltNatural, AltDefault, sizeof(AltDefault));
    *WorldAddTemperature = 1;
    *WorldSkipTerritory = 1;
    /*
    MapSizePlanet (this should not be unset, but checked regardless)
    MapOceanCoverage
    MapLandCoverage
    MapErosiveForces
    MapPlanetaryOrbit
    MapCloudCover
    MapNativeLifeForms
    */
    for (int i = 0; i < 7; i++) {
        if (MapSizePlanet[i] < 0) {
            MapSizePlanet[i] = map_rand.get(3);
        }
        if (i > 0) {
            MapSizePlanet[i] = clamp(MapSizePlanet[i], 0, 2);
        }
    }
    if (*GameState & STATE_OMNISCIENT_VIEW) {
        MapWin_clear_terrain(MapWin);
        draw_map(1);
    }
    *MapRandomSeed = (seed % 0x7fff) + 1; // Must be non-zero, supply pod placement
    *MapLandCoverage = 2 - *MapOceanCoverage;

    int levels[256] = {};
    int x = 0, y = 0, i = 0;

    const float L = AltNatural[ALT_SHORE_LINE]; // Default shoreline altitude = 45
    const float Wmid = (50 - conf.world_sea_levels[*MapOceanCoverage])*0.01f;
    const float Wsea = 0.1f + 0.01f * conf.world_ocean_mod;
    const float Wland = 0.1f + 0.01f * conf.world_hills_mod + 0.25f * (*MapErosiveForces);
    const float Warea = (conf.world_continents && *MapAreaY >= 32 ? 144.0f : 240.0f)
        / clamp(*MapAreaSqRoot, 120, 240);

    for (y = 0; y < *MapAreaY; y++) {
        float Wcaps = 1.0f - min(1.0f, (min(y, *MapAreaY - y) / (max(1.0f, *MapAreaY * 0.2f))));

        for (x = y&1; x < *MapAreaX; x+=2) {
            float value = world_fractal(noise, Warea, x, y) + Wmid - 0.5f*Wcaps;
            if (value > 0) {
                value = value * Wland;
            } else {
                value = value * Wsea;
            }
            sq = mapsq(x, y);
            sq->contour = clamp((int)(L + L*value), 0, 255);
        }
    }
    if (conf.world_mirror_x) {
        const int ky = 2 - (*MapAreaY & 1);
        for (y = 0; y < *MapAreaY/2; y++) {
            for (x = y&1; x < *MapAreaX; x+=2) {
                MAP* src = mapsq(x, y);
                MAP* tgt = mapsq(x, *MapAreaY - y - ky);
                tgt->contour = src->contour;
            }
        }
    }
    if (conf.world_mirror_y) {
        const int kx = 2 - (*MapAreaX & 1);
        for (y = 0; y < *MapAreaY; y++) {
            for (x = y&1; x < *MapAreaX/2; x+=2) {
                MAP* src = mapsq(x, y);
                MAP* tgt = mapsq(*MapAreaX - x - kx, y);
                tgt->contour = src->contour;
            }
        }
    }
    for (y = 0; y < *MapAreaY; y++) {
        for (x = y&1; x < *MapAreaX; x+=2) {
            sq = mapsq(x, y);
            levels[sq->contour]++;
        }
    }
    int level_sum = 0;
    int level_mod = 0;
    for (i = 0; i < 256; i++) {
        level_sum += levels[i];
        if (level_sum >= *MapAreaTiles * conf.world_sea_levels[*MapOceanCoverage] / 100) {
            level_mod = AltNatural[ALT_SHORE_LINE] - i;
            break;
        }
    }
    for (y = 0; y < *MapAreaY; y++) {
        for (x = y&1; x < *MapAreaX; x+=2) {
            sq = mapsq(x, y);
            sq->contour = clamp(sq->contour + level_mod, 0, 255);
        }
    }
    debug("world_build seed: %10u size: %d x: %d y: %d "\
    "ocean: %d hills: %d native: %d cloud: %d sea_level: %d level_mod: %d\n",
    seed, *MapSizePlanet, *MapAreaX, *MapAreaY, *MapOceanCoverage, *MapErosiveForces,
    *MapNativeLifeForms, *MapCloudCover, conf.world_sea_levels[*MapOceanCoverage], level_mod);

    if (conf.world_polar_caps) {
        world_polar_caps();
    }
    world_linearize_contours();
    world_shorelines();
    Path_continents(Paths);
    Points bridges;

    for (y = 3; y < *MapAreaY - 3; y++) {
        for (x = y&1; x < *MapAreaX; x+=2) {
            sq = mapsq(x, y);
            if (is_ocean(sq)) {
                continue;
            }
            uint64_t sea = 0;
            int land_count = Continents[sq->region].tile_count;
            if (conf.world_islands_mod > 0) {
                if (land_count < conf.world_islands_mod && land_count < *MapAreaTiles/8) {
                    bridges.insert({x, y});
                    if (DEBUG) mapdata[{x, y}].overlay = -1;
                    continue;
                }
            }
            for (auto& m : iterate_tiles(x, y, 1, 13)) {
                if (is_ocean(m.sq)) {
                    if (Continents[m.sq->region].tile_count >= 30) {
                        sea |= 1 << (m.sq->region & 0x3f);
                    }
                }
            }
            if (bit_count(sea) > 1) {
                if (land_count > *MapAreaTiles/8 || pair_hash(seed^(x/8), y/8) & 1) {
                    bridges.insert({x, y});
                    if (DEBUG) mapdata[{x, y}].overlay = -2;
                }
            }
        }
    }
    for (auto& p : bridges) {
        world_alt_set(p.x, p.y, ALT_OCEAN, 1);
        world_alt_set(p.x, p.y, ALT_OCEAN_SHELF, 1);
    }
    world_temperature();
    world_riverbeds();
    mod_world_fungus(noise);
    Path_continents(Paths);

    // Climate-aware Jungle exists before the major landmarks and final
    // world_climate pass. The old "jungle" knob controls native landmarks
    // only; minor_jungle is independently enabled by default.
    if (conf.modified_landmarks && conf.minor_jungle) {
        mod_world_monsoon(-1, -1);
    }

    LMConfig lm;
    memcpy(&lm, &conf.landmarks, sizeof(lm));

    for (i = 0; i < 16; i++) {
        if (conf.world_mirror_x || conf.world_mirror_y) {
            break;
        }
        if (!conf.modified_landmarks && lm.jungle > 0) {
            mod_world_monsoon(-1, -1);
            lm.jungle--;
        }
        if (lm.crater > 0) {
            mod_world_crater(-1, -1);
            lm.crater--;
        }
        if (lm.volcano > 0) {
            mod_world_volcano(-1, -1, 0);
            lm.volcano--;
        }
        if (lm.mesa > 0) {
            mod_world_mesa(-1, -1);
            lm.mesa--;
        }
        if (lm.ridge > 0) {
            mod_world_ridge(-1, -1);
            lm.ridge--;
        }
        if (!conf.modified_landmarks && lm.uranium > 0) {
            mod_world_diamond(-1, -1);
            lm.uranium--;
        }
        if (lm.ruins > 0) {
            mod_world_ruin(-1, -1);
            lm.ruins--;
        }
        /*
        Unity Wreckage and Fossil Field Ridge are always expansion only content.
        Manifold Nexus and Borehole Cluster were also added to vanilla SMAC
        in the patches, even though those landmarks are technically expansion content.
        */
        if (*ExpansionEnabled) {
            if (lm.unity > 0) {
                mod_world_unity(-1, -1);
                lm.unity--;
            }
            if (lm.fossil > 0) {
                mod_world_fossil(-1, -1);
                lm.fossil--;
            }
        }
        if (lm.canyon > 0) {
            mod_world_canyon(-1, -1);
            lm.canyon--;
        }
        if (lm.nexus > 0) {
            mod_world_temple(-1, -1);
            lm.nexus--;
        }
        if (lm.borehole > 0) {
            mod_world_borehole(-1, -1);
            lm.borehole--;
        }
        if (lm.sargasso > 0) {
            mod_world_sargasso(-1, -1);
            lm.sargasso--;
        }
        if (lm.dunes > 0) {
            mod_world_dune(-1, -1);
            lm.dunes--;
        }
        if (lm.fresh > 0) {
            mod_world_fresh(-1, -1);
            lm.fresh--;
        }
        if (lm.geothermal > 0) {
            mod_world_geothermal(-1, -1);
            lm.geothermal--;
        }
    }
    // Remove minor terrain where a major landmark has taken precedence.
    // The native Jungle/Uranium flags below remain reserved for vanilla-mode
    // worlds and the map editor; they are not assigned by new minor generators.
    for (int tile_id = 0; tile_id < *MapAreaTiles; ++tile_id) {
        MAP& tile = (*MapTiles)[tile_id];
        if (tile.lm_items() && *MapHalfX > 0) {
            const int ty = tile_id / *MapHalfX;
            const int tx = 2 * (tile_id % *MapHalfX) + (ty & 1);
            if (minor_landmark_at(tx, ty) != MINOR_NONE) {
                minor_landmark_clear(tx, ty);
            }
        }
    }
    // Jungle and Uranium are mutually exclusive. Later major landmarks
    // replace either terrain feature in their footprints, but preserve it
    // on surrounding tiles rather than stacking special resource bonuses.
    const int minor_landmarks = LM_JUNGLE
        | (conf.modified_landmarks ? LM_URANIUM : 0);
    for (int tile_id = 0; tile_id < *MapAreaTiles; tile_id++) {
        MAP& tile = (*MapTiles)[tile_id];
        if (conf.modified_landmarks && (tile.landmarks & LM_JUNGLE)) {
            tile.landmarks &= ~LM_URANIUM;
        }
        if ((tile.landmarks & minor_landmarks)
            && (tile.lm_items() & ~minor_landmarks)) {
            tile.landmarks &= ~minor_landmarks;
        }
    }
    fixup_landmarks();
    *WorldAddTemperature = 0;
    *WorldSkipTerritory = 0; // If this flag is false, reset_territory is run in world_climate
    world_climate(); // Run Path::continents
    world_rocky();
    // Run the late minor-terrain pass after final climate and geology.
    generate_minor_landmarks();

#ifdef BUILD_DEBUG
    // Generated minor features must not consume native landmark flags.
    // This summary also makes independent Jungle/Uranium testable while
    // their separate sprite rendering and tile-inspector path are unfinished.
    int minor_counts[4] = {};
    int minor_native_overlap = 0;
    int minor_bad_codes = 0;
    if (*MapHalfX > 0) {
        for (int tile_id = 0; tile_id < *MapAreaTiles; ++tile_id) {
            const int ty = tile_id / *MapHalfX;
            const int tx = 2 * (tile_id % *MapHalfX) + (ty & 1);
            const MinorLandmark type = minor_landmark_at(tx, ty);
            if (type == MINOR_NONE) {
                continue;
            }
            const int index = static_cast<int>(type);
            if (index < 1 || index > 3) {
                minor_bad_codes++;
                continue;
            }
            minor_counts[index]++;
            if ((*MapTiles)[tile_id].lm_items()) {
                minor_native_overlap++;
            }
            const int code = minor_landmark_code_at(tx, ty);
            if ((type == MINOR_JUNGLE && code > 120)
                || (type != MINOR_JUNGLE && code > 8)) {
                minor_bad_codes++;
            }
        }
    }
    debug("minor generation Jungle=%d Uranium=%d Ironstone=%d native_overlap=%d bad_codes=%d\n",
        minor_counts[MINOR_JUNGLE], minor_counts[MINOR_URANIUM],
        minor_counts[MINOR_IRONSTONE], minor_native_overlap, minor_bad_codes);
#endif

    if (!*GameHalted) {
        MapWin_clear_terrain(MapWin);
    }
    flushlog();
}

