#include "gui.h"
#include "empire_report.h"
#include "tech.h"

namespace {

// Native in-engine window retained across open/close cycles. No turn-event
// capture, custom save state or changes to the original upkeep pipeline.
GraphicWin report_window = {};
bool report_window_initialized = false;
char report_caption[] = "Empire Report";
// Keep report typography independent of MapWin zoom-dependent fonts.
Font report_font = {};
bool report_font_initialized = false;
const int REPORT_FONT_HEIGHT = 17;
int report_width = 0;
int report_height = 0;
int base_scroll = 0;
int wheel_remainder = 0;

const int BASE_FIRST_ROW_Y = 286;
const int BASE_ROW_HEIGHT = 22;

int visible_base_rows() {
    return max(1, (report_height - BASE_FIRST_ROW_Y - 36) / BASE_ROW_HEIGHT);
}

int report_faction() {
    if (*GameHalted) {
        return -1;
    }
    const int faction_id = MapWin->cOwner;
    return (faction_id > 0 && faction_id < MaxPlayerNum) ? faction_id : -1;
}

int count_bases(int faction_id) {
    int count = 0;
    for (int i = 0; i < *BaseCount; ++i) {
        if (Bases[i].faction_id == faction_id) {
            ++count;
        }
    }
    return count;
}

int max_base_scroll() {
    const int faction_id = report_faction();
    return faction_id >= 0
        ? max(0, count_bases(faction_id) - visible_base_rows()) : 0;
}

// The game text drawing entry point takes a writable char buffer.
void write_text(const char* text, int x, int y, int color = ColorWhite) {
    char line[256];
    snprintf(line, sizeof(line), "%s", text);
    Buffer_set_text_color(&report_window.oCanvas, color, 0, 1, 1);
    Buffer_write_l_3(&report_window.oCanvas, line, x, y, (int)strlen(line));
}

struct EmpireStats {
    int population = 0;
    int units = 0;
    int bases = 0;
    int nutrients = 0;
    int minerals = 0;
    int energy = 0;
    int economy = 0;
    int psych = 0;
    int labs = 0;
    int golden_ages = 0;
    int riots = 0;
    int starving = 0;
    int eco_bases = 0;
};

EmpireStats collect_empire_stats(int faction_id) {
    EmpireStats s;
    for (int i = 0; i < *BaseCount; ++i) {
        const BASE& base = Bases[i];
        if (base.faction_id != faction_id) {
            continue;
        }
        ++s.bases;
        s.population += base.pop_size;
        s.nutrients += base.nutrient_surplus;
        s.minerals += base.mineral_surplus;
        s.energy += base.energy_surplus;
        s.economy += base.economy_total;
        s.psych += base.psych_total;
        s.labs += base.labs_total;
        s.golden_ages += !!(base.state_flags & BSTATE_GOLDEN_AGE_ACTIVE);
        s.riots += !!(base.state_flags & BSTATE_DRONE_RIOTS_ACTIVE);
        s.starving += base.nutrient_surplus < 0;
        s.eco_bases += base.eco_damage > 0;
    }
    for (int i = 0; i < *VehCount; ++i) {
        s.units += Vehs[i].faction_id == faction_id;
    }
    return s;
}

void draw_empire_summary(int faction_id) {
    const Faction& faction = Factions[faction_id];
    const EmpireStats s = collect_empire_stats(faction_id);
    char line[256];

    snprintf(line, sizeof(line),
        "Bases: %d    Population: %d    Average: %d.%d    Units: %d",
        s.bases, s.population,
        s.bases ? s.population / s.bases : 0,
        s.bases ? (10 * (s.population % s.bases)) / s.bases : 0,
        s.units);
    write_text(line, 20, 57);

    snprintf(line, sizeof(line),
        "Treasury: %d EC    Base economy: %d EC    Psych: %d",
        faction.energy_credits, s.economy, s.psych);
    write_text(line, 20, 79);

    snprintf(line, sizeof(line),
        "Labs (last upkeep): %d    Energy surplus: %d    Commerce: %d    Facility maint: %d",
        faction.labs_total, s.energy, faction.turn_commerce_income, faction.facility_maint_total);
    write_text(line, 20, 101);

    snprintf(line, sizeof(line),
        "Nutrient surplus: %+d    Mineral surplus: %+d    Base labs: %d",
        s.nutrients, s.minerals, s.labs);
    write_text(line, 20, 123);

    snprintf(line, sizeof(line),
        "Golden Ages: %d    Drone riots: %d    Starving: %d    Eco-damage bases: %d",
        s.golden_ages, s.riots, s.starving, s.eco_bases);
    write_text(line, 20, 145, s.riots || s.starving ? ColorYellow : ColorWhite);
}

void draw_research(int faction_id) {
    const Faction& faction = Factions[faction_id];
    char line[256];
    const int tech_id = faction.tech_research_id;
    if (tech_id < 0 || tech_id >= TECH_TranT + 1) {
        write_text("Research: None selected", 20, 174, ColorCyan);
        return;
    }

    snprintf(line, sizeof(line), "Researching: %s", tech_str(tech_id));
    write_text(line, 20, 174, ColorCyan);

    const int cost = max(1, mod_tech_rate(faction_id));
    const int progress = max(0, faction.tech_accumulated);
    const int research_rating = clamp(faction.SE_research_pending, -5, 5);
    // mod_base_research applies a (+/-10% per rating point) modifier
    // separately for each base and carries its fractional remainder.
    // This whole-faction estimate may differ slightly due to rounding.
    const int64_t estimate = int64_t(faction.labs_total) * (10 + research_rating) / 10;
    const int64_t remaining = max(int64_t(0), int64_t(cost) - progress);

    snprintf(line, sizeof(line),
        "Progress: %d / %d labs    Labs last upkeep: %+d    Effective rate: ~%lld/turn",
        progress, cost, faction.labs_total, (long long)estimate);
    write_text(line, 20, 197);

    if (remaining == 0) {
        snprintf(line, sizeof(line), "Research threshold met");
    } else if (estimate > 0) {
        const int64_t turns = (remaining + estimate - 1) / estimate;
        snprintf(line, sizeof(line), "Estimated turns remaining: %lld (at current output)",
            (long long)turns);
    } else {
        snprintf(line, sizeof(line), "Estimated turns remaining: N/A (no recent lab output)");
    }
    write_text(line, 20, 219);
}

void draw_base_list(int faction_id) {
    const int total = count_bases(faction_id);
    const int rows = visible_base_rows();
    base_scroll = clamp(base_scroll, 0, max(0, total - rows));

    char line[256];
    snprintf(line, sizeof(line), "BASE OVERVIEW    %d bases    showing %d-%d",
        total, total ? base_scroll + 1 : 0, min(total, base_scroll + rows));
    write_text(line, 20, 242, ColorCyan);

    const int x_pop = report_width * 34 / 100;
    const int x_n = report_width * 40 / 100;
    const int x_m = report_width * 46 / 100;
    const int x_e = report_width * 52 / 100;
    const int x_ec = report_width * 60 / 100;
    const int x_labs = report_width * 68 / 100;
    const int x_status = report_width * 76 / 100;
    write_text("Base", 20, 262, ColorYellow);
    write_text("Pop", x_pop, 262, ColorYellow);
    write_text("N", x_n, 262, ColorYellow);
    write_text("M", x_m, 262, ColorYellow);
    write_text("E", x_e, 262, ColorYellow);
    write_text("EC", x_ec, 262, ColorYellow);
    write_text("Labs", x_labs, 262, ColorYellow);
    write_text("Status", x_status, 262, ColorYellow);

    int seen = 0;
    int drawn = 0;
    for (int i = 0; i < *BaseCount && drawn < rows; ++i) {
        const BASE& base = Bases[i];
        if (base.faction_id != faction_id) {
            continue;
        }
        if (seen++ < base_scroll) {
            continue;
        }
        const int y = BASE_FIRST_ROW_Y + BASE_ROW_HEIGHT * drawn++;
        write_text(base.name, 20, y);
        snprintf(line, sizeof(line), "%d", base.pop_size);
        write_text(line, x_pop, y);
        snprintf(line, sizeof(line), "%+d", base.nutrient_surplus);
        write_text(line, x_n, y, base.nutrient_surplus < 0 ? ColorRed : ColorWhite);
        snprintf(line, sizeof(line), "%+d", base.mineral_surplus);
        write_text(line, x_m, y, base.mineral_surplus < 0 ? ColorRed : ColorWhite);
        snprintf(line, sizeof(line), "%+d", base.energy_surplus);
        write_text(line, x_e, y);
        snprintf(line, sizeof(line), "%d", base.economy_total);
        write_text(line, x_ec, y);
        snprintf(line, sizeof(line), "%d", base.labs_total);
        write_text(line, x_labs, y);

        const char* status = "Normal";
        int status_color = ColorWhite;
        if (base.state_flags & BSTATE_DRONE_RIOTS_ACTIVE) {
            status = "Drone riot";
            status_color = ColorRed;
        } else if (base.nutrient_surplus < 0) {
            status = "Starving";
            status_color = ColorRed;
        } else if (base.state_flags & BSTATE_GOLDEN_AGE_ACTIVE) {
            status = "Golden Age";
            status_color = ColorGreen;
        } else if (base.eco_damage > 0) {
            status = "Eco-damage";
            status_color = ColorYellow;
        }
        write_text(status, x_status, y, status_color);
    }

    write_text("Scroll: mouse wheel / Up / Down / PgUp / PgDn / Home / End",
        20, report_height - 29, ColorCyan);
}

void __cdecl draw_report_window() {
    GraphicWin_fill(&report_window, 0);
    Buffer_set_font(&report_window.oCanvas, &report_font, 0, 0, 0);
    write_text("EMPIRE REPORT  -  live status", 20, 23, ColorCyan);

    const int faction_id = report_faction();
    if (faction_id < 0) {
        write_text("No active faction.", 20, 65);
        return;
    }
    draw_empire_summary(faction_id);
    draw_research(faction_id);
    draw_base_list(faction_id);
}

} // namespace

bool empire_report_visible() {
    return report_window_initialized
        && (report_window.winstate & WIN_ST_VISIBLE);
}

void empire_report_scroll(int rows) {
    if (!empire_report_visible()) {
        return;
    }
    base_scroll = clamp(base_scroll + rows, 0, max_base_scroll());
    GraphicWin_redraw(&report_window);
}

void empire_report_scroll_page(int pages) {
    empire_report_scroll(pages * visible_base_rows());
}

void empire_report_scroll_wheel(int delta) {
    wheel_remainder += delta;
    const int notches = wheel_remainder / WHEEL_DELTA;
    wheel_remainder -= notches * WHEEL_DELTA;
    if (notches) {
        empire_report_scroll(-3 * notches);
    }
}

void empire_report_hide() {
    if (empire_report_visible()) {
        Win_hide(&report_window);
    }
}

void empire_report_show() {
    if (empire_report_visible()) {
        GraphicWin_redraw(&report_window);
        return;
    }
    if (!report_window_initialized) {
        report_width = min(900, max(560, *ScreenWidth - 64));
        report_height = min(660, max(440, *ScreenHeight - 64));
        const int x = max(0, (*ScreenWidth - report_width) / 2);
        const int y = max(0, (*ScreenHeight - report_height) / 2);

        GraphicWin_ctor(&report_window);
        GraphicWin_init(&report_window, x, y, report_width, report_height,
            report_caption, 0, &MapWin->oMainWin, nullptr, nullptr);
        report_window.cb_win_redraw = draw_report_window;
        // The original prototype used the map font, which changes size
        // with the world-map zoom. Use a separate 17-pixel UI font instead.
        Font_ctor(&report_font);
        report_font_initialized = !!Font_init(&report_font, "Arial", REPORT_FONT_HEIGHT, 0);
        if (!report_font_initialized) {
            // Keep the native window usable if the font could not initialize.
            // StatusWindow's text font is not tied to world-map tile zoom.
            Font_close(&report_font);
            report_font = StatusWin->oFontNorm;
        }
        report_window_initialized = true;
    }

    base_scroll = clamp(base_scroll, 0, max_base_scroll());
    Win_show(&report_window, 1);
    Win_bring_to_top(&report_window);
    GraphicWin_redraw(&report_window);
}
