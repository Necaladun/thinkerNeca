#include "gui.h"
#include "empire_report.h"
#include "basewin.h"
#include "gameturn.h"
#include "tech.h"
#include <string>

namespace {

// Native in-engine window retained across open/close cycles. No turn-event
// capture, custom save state or changes to the original upkeep pipeline.
GraphicWin report_window = {};
bool report_window_initialized = false;
char report_caption[] = "Empire Report";
// Keep report typography independent of MapWin zoom-dependent fonts.
Font report_font = {};
Font* report_text_font = &report_font;
const int REPORT_FONT_HEIGHT = 17;
int report_width = 0;
int report_height = 0;
int base_scroll = 0;
int wheel_remainder = 0;
// One-shot request consumed from the game's modal colony message pump.
enum ColonyProductionAction { COLONY_CHANGE_BUILD, COLONY_EDIT_QUEUE };
int pending_production_base_id = -1;
ColonyProductionAction pending_production_action = COLONY_CHANGE_BUILD;
uint32_t pending_production_ticket = 0;
uint32_t next_production_ticket = 0;
int report_faction();  // Defined below; used by the deferred production shortcut.

const int REPORT_DESIRED_WIDTH = 1350;  // 150% of the previous 900-pixel target
const int REPORT_DESIRED_HEIGHT = 990;  // 150% of the previous 660-pixel target
const int REPORT_SCREEN_MARGIN = 64;
const int BASE_ROW_HEIGHT = 22;
const int BASE_BOTTOM_RESERVE = 70;
enum BaseListView { VIEW_STATUS, VIEW_PRODUCTION, VIEW_CITIZENS, VIEW_BEST, VIEW_GOVERNORS, VIEW_GARRISON, VIEW_ATTENTION, VIEW_GROWTH, VIEW_ECONOMY, VIEW_DEFENSES, VIEW_UNITS, VIEW_MESSAGES };
// The main level is deliberately independent of individual report views.
// Save the current subtab within each group, but start on the overview.
enum ReportSection { SECTION_OVERVIEW, SECTION_COLONIES, SECTION_MILITARY, SECTION_INTELLIGENCE, SECTION_RECORDS };
ReportSection report_section = SECTION_OVERVIEW;
BaseListView base_view = VIEW_STATUS;
BaseListView last_colony_view = VIEW_STATUS;
BaseListView last_military_view = VIEW_GARRISON;
const int SECTION_NAV_Y = 63;
const int SECTION_NAV_STEP = 30;
const int REPORT_SECOND_TAB_OFFSET = 28;
struct SectionTab { ReportSection section; const char* label; int width; };
const SectionTab SECTION_TABS[] = {
    { SECTION_OVERVIEW, "Overview", 112 },
    { SECTION_COLONIES, "Colonies", 111 },
    { SECTION_MILITARY, "Military", 105 },
    { SECTION_INTELLIGENCE, "Intelligence", 136 },
    { SECTION_RECORDS, "Records", 105 }
};
struct ReportTab { BaseListView view; const char* label; int width; };
const ReportTab REPORT_TABS[] = {
    { VIEW_STATUS, "Status", 78 },
    { VIEW_PRODUCTION, "Production", 118 },
    { VIEW_CITIZENS, "Citizens", 95 },
    { VIEW_BEST, "Best", 65 },
    { VIEW_GOVERNORS, "Gov", 52 },
    { VIEW_GARRISON, "Garrison", 100 },
    { VIEW_ATTENTION, "Attention", 106 },
    { VIEW_GROWTH, "Growth", 82 },
    { VIEW_ECONOMY, "Economy", 102 },
    { VIEW_DEFENSES, "Defenses", 112 },
    { VIEW_UNITS, "Units", 80 },
    { VIEW_MESSAGES, "Messages", 110 },
};

ReportSection section_for_view(BaseListView view) {
    switch (view) {
    case VIEW_GARRISON:
    case VIEW_DEFENSES:
    case VIEW_UNITS: return SECTION_MILITARY;
    case VIEW_MESSAGES: return SECTION_RECORDS;
    default: return SECTION_COLONIES;
    }
}
int section_nav_position(ReportSection section, int* out_y) {
    int x = 20;
    int y = SECTION_NAV_Y;
    for (const SectionTab& tab : SECTION_TABS) {
        if (x > 20 && x + tab.width > report_width - 10) {
            x = 20;
            y += SECTION_NAV_STEP;
        }
        if (tab.section == section) {
            if (out_y) *out_y = y;
            return x;
        }
        x += tab.width + 8;
    }
    if (out_y) *out_y = SECTION_NAV_Y;
    return 20;
}
int section_nav_rows() {
    int y = SECTION_NAV_Y;
    section_nav_position(SECTION_RECORDS, &y);
    return 1 + (y - SECTION_NAV_Y) / SECTION_NAV_STEP;
}
int report_subtab_y() {
    return SECTION_NAV_Y + section_nav_rows() * SECTION_NAV_STEP + 12;
}
int report_tab_position(BaseListView view, int* out_y) {
    int x = 20;
    int y = report_subtab_y();
    for (const ReportTab& tab : REPORT_TABS) {
        if (section_for_view(tab.view) != report_section) continue;
        if (x > 20 && x + tab.width > report_width - 10) {
            x = 20;
            y += REPORT_SECOND_TAB_OFFSET;
        }
        if (tab.view == view) {
            if (out_y) *out_y = y;
            return x;
        }
        x += tab.width + 8;
    }
    if (out_y) *out_y = report_subtab_y();
    return 20;
}
int report_tab_y(BaseListView view) {
    int y = report_subtab_y();
    report_tab_position(view, &y);
    return y;
}
int report_tab_rows() {
    if (report_section == SECTION_OVERVIEW || report_section == SECTION_INTELLIGENCE)
        return 0;
    int max_y = report_subtab_y();
    for (const ReportTab& tab : REPORT_TABS) {
        if (section_for_view(tab.view) != report_section) continue;
        max_y = max(max_y, report_tab_y(tab.view));
    }
    return 1 + (max_y - report_subtab_y()) / REPORT_SECOND_TAB_OFFSET;
}
int report_column_y() {
    return report_subtab_y() + report_tab_rows() * REPORT_SECOND_TAB_OFFSET + 35;
}
int report_first_row_y() {
    return report_column_y() + 23;
}
int report_list_heading_y() {
    return report_column_y() - 26;
}

enum UnitFilter { UNIT_ALL, UNIT_LAND, UNIT_SEA, UNIT_AIR, UNIT_DAMAGED, UNIT_NO_ORDER };
UnitFilter unit_filter = UNIT_ALL;
const char* const UNIT_FILTER_NAMES[] = {
    "All", "Land", "Sea", "Air", "Damaged", "No order"
};
const int UNIT_FILTER_COUNT = 6;
bool unit_filter_match(VEH& veh, int faction_id) {
    if (veh.faction_id != faction_id
        || veh.unit_id < 0 || veh.unit_id >= MaxProtoNum) return false;
    switch (unit_filter) {
    case UNIT_LAND: return veh.triad() == TRIAD_LAND;
    case UNIT_SEA: return veh.triad() == TRIAD_SEA;
    case UNIT_AIR: return veh.triad() == TRIAD_AIR;
    case UNIT_DAMAGED: return veh.damage_taken > 0;
    case UNIT_NO_ORDER: return veh.order == ORDER_NONE;
    default: return true;
    }
}
int unit_count(int faction_id) {
    int count = 0;
    for (int i = 0; i < *VehCount; ++i) {
        if (unit_filter_match(Vehs[i], faction_id)) ++count;
    }
    return count;
}
int unit_at(int faction_id, int ordinal) {
    for (int i = 0; i < *VehCount; ++i) {
        if (unit_filter_match(Vehs[i], faction_id) && ordinal-- == 0) return i;
    }
    return -1;
}

// MessageWindow exposes native lists, but the listbox can hold reference
// records without direct text data. The original reader used seek_pos and
// showed a placeholder for every non-text row. Inspect both native lists,
// and follow their declared linked nodes read-only. Never assume that the
// existence of a native entry means it has a directly readable message.
struct NativeMessageSourceStatus {
    int listbox_entries = 0;
    int listbox_texts = 0;
    int detail_entries = 0;
    int detail_texts = 0;
};
NativeMessageSourceStatus message_status = {};
std::vector<std::string> native_messages;

// The native linked lists are only provisionally understood. Avoid chasing
// stale or out-of-range pointers during report redraw on different executables.
size_t readable_native_region(const void* ptr) {
    if (!ptr) return 0;
    MEMORY_BASIC_INFORMATION page = {};
    if (VirtualQuery(ptr, &page, sizeof(page)) != sizeof(page)
        || page.State != MEM_COMMIT
        || (page.Protect & (PAGE_GUARD | PAGE_NOACCESS))) {
        return 0;
    }
    const uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    const uintptr_t start = reinterpret_cast<uintptr_t>(page.BaseAddress);
    const size_t offset = addr - start;
    return offset < page.RegionSize ? page.RegionSize - offset : 0;
}
bool readable_native_struct(const void* ptr, size_t bytes) {
    return readable_native_region(ptr) >= bytes;
}
bool copy_native_message_text(const char* value, std::string& out) {
    const size_t limit = min((size_t)1024, readable_native_region(value));
    if (!limit) return false;
    size_t len = 0;
    while (len < limit && value[len]) {
        const unsigned char c = (unsigned char)value[len];
        if (c < 32 && c != '\n' && c != '\r' && c != '\t') return false;
        ++len;
    }
    // Only complete, null-terminated text from a readable region is accepted.
    if (!len || len == limit) return false;
    out.assign(value, len);
    return true;
}

void read_native_message_list(StringStruct* list, int& raw_count,
                              int& readable_count) {
    if (!list || list->count < 0 || list->count > 1024) return;
    raw_count = list->count;
    if (!list->head || !list->count) return;

    StringStructNode* node = list->head;
    const int max_nodes = min(list->count + 2, 1026);
    for (int i = 0; i < max_nodes && node; ++i) {
        if (!readable_native_struct(node, sizeof(*node))) break;
        if (readable_native_struct(node->data, sizeof(*node->data))) {
            std::string value;
            if (copy_native_message_text(node->data->text, value)) {
                ++readable_count;
                if (native_messages.size() < 512) {
                    native_messages.emplace_back(std::move(value));
                }
            }
        }
        StringStructNode* next = node->next;
        if (!next || next == node || next == list->head) break;
        node = next;
    }
}

// Rebuild on redraw; no snapshot survives as saved game data. Prefer the
// listbox (event collection). The StringBox may only contain the currently
// selected message, so use it only if no listbox texts are accessible.
void refresh_native_messages() {
    native_messages.clear();
    message_status = {};
    read_native_message_list(&MessageWin->listBox.dialog.strings,
        message_status.listbox_entries, message_status.listbox_texts);
    if (!native_messages.empty()) return;
    read_native_message_list(&MessageWin->stringBox.strings,
        message_status.detail_entries, message_status.detail_texts);
}

int native_message_count() {
    return (int)native_messages.size();
}

bool native_message_text(int index, char* out, size_t capacity) {
    if (!capacity) return false;
    out[0] = 0;
    if (index < 0 || index >= (int)native_messages.size()) return false;
    snprintf(out, capacity, "%s", native_messages[index].c_str());
    return true;
}
const int MESSAGE_ROW_HEIGHT = 46;
int report_row_height() {
    return base_view == VIEW_MESSAGES ? MESSAGE_ROW_HEIGHT : BASE_ROW_HEIGHT;
}

// Like F4's Best Bases readout, each entry is the single leading base
// in its category, not a sortable ranking of all bases.
enum BestMetric {
    BEST_MINERALS,
    BEST_NUTRIENTS,
    BEST_ENERGY,
    BEST_LABS,
    BEST_PSYCH,
    BEST_POPULATION,
    BEST_GARRISON,
    BEST_INFRASTRUCTURE,
    BEST_COUNT
};
const char* BEST_METRIC_NAMES[BEST_COUNT] = {
    "Minerals", "Nutrients", "Energy", "Labs", "Psych",
    "Population", "Garrison", "Infrastructure"
};

struct GarrisonStats {
    int here = 0;       // All own units physically on the base square
    int land = 0;
    int sea = 0;
    int air = 0;
    int combat = 0;     // Uses the native prototype combat classification
    int police = 0;     // Eligible unit types; not actual police-limit contribution
    int home = 0;       // Units assigned this base as their home, anywhere
    int away = 0;       // Home-assigned units not presently stationed here
};

GarrisonStats collect_garrison(const BASE& base, int base_id, int faction_id) {
    GarrisonStats stats;
    for (int i = 0; i < *VehCount; ++i) {
        VEH& veh = Vehs[i];
        if (veh.faction_id != faction_id) {
            continue;
        }
        const bool stationed = veh.x == base.x && veh.y == base.y;
        if (base_id >= 0 && veh.home_base_id == base_id) {
            ++stats.home;
            stats.away += !stationed;
        }
        if (!stationed) {
            continue;
        }
        ++stats.here;
        switch (veh.triad()) {
        case TRIAD_LAND: ++stats.land; break;
        case TRIAD_SEA:  ++stats.sea;  break;
        case TRIAD_AIR:  ++stats.air;  break;
        }
        stats.combat += veh.is_combat_unit();
        stats.police += veh.is_police_unit();
    }
    return stats;
}

int garrison_on_base(const BASE& base, int faction_id) {
    return collect_garrison(base, -1, faction_id).here;
}

int built_facilities(int base_id) {
    int count = 0;
    // Secret Projects and orbital units are not base facilities.
    for (int i = Fac_ID_First; i <= Fac_ID_Last; ++i) {
        count += !!has_fac_built((FacilityId)i, base_id);
    }
    return count;
}

int best_metric_value(int base_id, BestMetric metric) {
    const BASE& base = Bases[base_id];
    switch (metric) {
    case BEST_MINERALS:       return base.mineral_intake_2;  // after bonuses, before unit support
    case BEST_NUTRIENTS:      return base.nutrient_intake_2; // before consumption
    case BEST_ENERGY:         return base.economy_total;     // credits produced
    case BEST_LABS:           return base.labs_total;
    case BEST_PSYCH:          return base.psych_total;
    case BEST_POPULATION:     return base.pop_size;
    case BEST_GARRISON:       return garrison_on_base(base, base.faction_id);
    case BEST_INFRASTRUCTURE: return built_facilities(base_id);
    default:                  return 0;
    }
}

struct BestBase {
    int base_id;
    int value;
};
BestBase best_base_for(int faction_id, BestMetric metric) {
    BestBase result = { -1, 0 };
    for (int i = 0; i < *BaseCount; ++i) {
        if (Bases[i].faction_id != faction_id) {
            continue;
        }
        const int value = best_metric_value(i, metric);
        // Preserve a deterministic tie-breaker: earliest matching base ID.
        if (result.base_id < 0 || value > result.value) {
            result.base_id = i;
            result.value = value;
        }
    }
    return result;
}

// Available for all bases via the Gov tab. Use existing native validation,
// locking, dialog and side-effect logic; do not duplicate governor flags.
void open_governor_options(int base_id) {
    empire_report_hide();
    BaseWin_soft_zoom(BaseWin, base_id);
    if (*CurrentBaseID == base_id
        && *CurrentBase == &Bases[base_id]
        && BaseWin->cur_base_id == base_id) {
        BaseWin_gov_options(BaseWin, 0);
    } else {
        // An unsupported base context must not operate on a different colony.
        empire_report_show();
    }
}

// BaseWin_zoom owns a modal event loop and does not return until the colony
// closes. Post a one-shot private window message to enter the appropriate
// native editor while the selected colony screen is open. The report never
// edits its queue directly: cancellation and insertion use native behavior.
void open_colony_production(int base_id, ColonyProductionAction action) {
    if (base_id < 0 || base_id >= *BaseCount
        || Bases[base_id].faction_id != report_faction()
        || pending_production_base_id >= 0) return;
    const UINT message = empire_report_production_message();
    HWND hwnd = *phWnd;
    if (!message || !hwnd || !IsWindow(hwnd)) return;

    uint32_t ticket = ++next_production_ticket;
    if (!ticket) ticket = ++next_production_ticket;
    pending_production_base_id = base_id;
    pending_production_action = action;
    pending_production_ticket = ticket;

    empire_report_hide();
    if (!PostMessage(hwnd, message, (WPARAM)base_id, (LPARAM)ticket)) {
        pending_production_base_id = -1;
        pending_production_ticket = 0;
        empire_report_show();
        return;
    }
    BaseWin_zoom(BaseWin, base_id, 1);
    // The colony was closed without consumption, or the picker completed.
    // Any still-queued stale message must never act on a later base.
    pending_production_base_id = -1;
    pending_production_ticket = 0;
}

// Pure, live-state warnings. No event recording or hidden unit scanning.
enum AttentionFlags {
    ATT_RIOT = 1, ATT_STARVING = 2, ATT_STALLED = 4,
    ATT_ECO = 8, ATT_HAB_CAP = 16,
};
int base_attention_flags(int base_id) {
    const BASE& base = Bases[base_id];
    int flags = 0;
    if (base.state_flags & BSTATE_DRONE_RIOTS_ACTIVE) flags |= ATT_RIOT;
    if (base.nutrient_surplus < 0) flags |= ATT_STARVING;
    if (base.queue_items[0] != -FAC_STOCKPILE_ENERGY && base.mineral_surplus <= 0
        && base.minerals_accumulated < mineral_cost(base_id, base.queue_items[0])) {
        flags |= ATT_STALLED;
    }
    if (base.eco_damage > 0) flags |= ATT_ECO;
    if (base.nutrient_surplus > 0 && !has_fac_built(FAC_HABITATION_DOME, base_id)) {
        const int modifier = (has_project(FAC_ASCETIC_VIRTUES, base.faction_id) ? 2 : 0)
            - MFactions[base.faction_id].rule_population;
        const int limit = (has_fac_built(FAC_HAB_COMPLEX, base_id)
            ? Rules->pop_limit_wo_hab_dome : Rules->pop_limit_wo_hab_complex) + modifier;
        if (base.pop_size >= limit) flags |= ATT_HAB_CAP;
    }
    return flags;
}
int attention_severity(int flags) {
    if (flags & (ATT_RIOT | ATT_STARVING)) return 3;
    if (flags & (ATT_STALLED | ATT_HAB_CAP)) return 2;
    return (flags & ATT_ECO) ? 1 : 0;
}
int count_attention_bases(int faction_id) {
    int count = 0;
    for (int i = 0; i < *BaseCount; ++i) {
        if (Bases[i].faction_id == faction_id && base_attention_flags(i)) ++count;
    }
    return count;
}
// Resolve again on each click. Base IDs can shift after captures or losses.
// Critical cases come first, then warnings, then ecological advisories.
int attention_base_at(int faction_id, int ordinal) {
    for (int severity = 3; severity >= 1; --severity) {
        for (int i = 0; i < *BaseCount; ++i) {
            if (Bases[i].faction_id == faction_id
                && attention_severity(base_attention_flags(i)) == severity) {
                if (ordinal-- == 0) return i;
            }
        }
    }
    return -1;
}

int visible_base_rows() {
    return max(1, (report_height - report_first_row_y() - BASE_BOTTOM_RESERVE)
        / report_row_height());
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

int report_row_count(int faction_id) {
    const int bases = count_bases(faction_id);
    if (base_view == VIEW_BEST) return bases ? BEST_COUNT : 0;
    if (base_view == VIEW_ATTENTION) return count_attention_bases(faction_id);
    if (base_view == VIEW_UNITS) return unit_count(faction_id);
    return base_view == VIEW_MESSAGES ? native_message_count() : bases;
}

int max_base_scroll() {
    const int faction_id = report_faction();
    return faction_id >= 0
        ? max(0, report_row_count(faction_id) - visible_base_rows()) : 0;
}

// The game text drawing entry point takes a writable char buffer.
void write_text(const char* text, int x, int y, int color = ColorWhite) {
    char line[256];
    snprintf(line, sizeof(line), "%s", text);
    Buffer_set_text_color(&report_window.oCanvas, color, 0, 1, 1);
    Buffer_write_l_3(&report_window.oCanvas, line, x, y, (int)strlen(line));
}

// Small vector/category pictograms drawn into the report buffer.
// These are deliberately NOT claimed to be native production-item artwork:
// the native ProdPicker rendering path has state that is not yet understood.
void marker_stroke(int x1, int y1, int x2, int y2, int color) {
    Buffer_line_2(&report_window.oCanvas, x1, y1, x2, y2, color);
}

void draw_production_marker(int item, int x, int y) {
    enum MarkerKind { MARKER_LAND, MARKER_SEA, MARKER_AIR,
        MARKER_FACILITY, MARKER_PROJECT, MARKER_STOCKPILE, MARKER_UNKNOWN };
    MarkerKind kind = MARKER_UNKNOWN;
    int color = ColorWhite;
    if (item >= 0 && item < MaxProtoNum) {
        const int triad = Units[item].triad();
        kind = triad == TRIAD_SEA ? MARKER_SEA
            : triad == TRIAD_AIR ? MARKER_AIR : MARKER_LAND;
        color = ColorCyan;
    } else if (item == -FAC_STOCKPILE_ENERGY) {
        kind = MARKER_STOCKPILE;
        color = ColorEnergy;
    } else if (item <= -SP_ID_First && item >= -SP_ID_Last) {
        kind = MARKER_PROJECT;
        color = ColorYellow;
    } else if (item <= -Fac_ID_First && item >= -FAC_ORBITAL_DEFENSE_POD) {
        kind = MARKER_FACILITY;
        color = ColorGreen;
    }

    RECT outline = { x, y, x + 20, y + 20 };
    Buffer_box_2(&report_window.oCanvas, &outline, color, color);
    switch (kind) {
    case MARKER_LAND: // Tank hull, turret and tracks.
        marker_stroke(x + 4, y + 13, x + 16, y + 13, color);
        marker_stroke(x + 5, y + 10, x + 15, y + 10, color);
        marker_stroke(x + 8, y + 7, x + 12, y + 7, color);
        marker_stroke(x + 11, y + 7, x + 16, y + 7, color);
        marker_stroke(x + 5, y + 15, x + 15, y + 15, color);
        break;
    case MARKER_SEA: // Ship hull and mast.
        marker_stroke(x + 4, y + 11, x + 16, y + 11, color);
        marker_stroke(x + 4, y + 11, x + 8, y + 15, color);
        marker_stroke(x + 8, y + 15, x + 13, y + 15, color);
        marker_stroke(x + 13, y + 15, x + 16, y + 11, color);
        marker_stroke(x + 10, y + 5, x + 10, y + 11, color);
        break;
    case MARKER_AIR: // Aircraft fuselage, wings and tail.
        marker_stroke(x + 10, y + 4, x + 10, y + 16, color);
        marker_stroke(x + 4, y + 10, x + 16, y + 10, color);
        marker_stroke(x + 7, y + 15, x + 13, y + 15, color);
        break;
    case MARKER_FACILITY: // Building outline and windows.
        marker_stroke(x + 5, y + 6, x + 15, y + 6, color);
        marker_stroke(x + 5, y + 6, x + 5, y + 16, color);
        marker_stroke(x + 15, y + 6, x + 15, y + 16, color);
        marker_stroke(x + 5, y + 16, x + 15, y + 16, color);
        marker_stroke(x + 8, y + 9, x + 8, y + 11, color);
        marker_stroke(x + 12, y + 9, x + 12, y + 11, color);
        break;
    case MARKER_PROJECT: // Star for a Secret Project.
        marker_stroke(x + 10, y + 4, x + 13, y + 15, color);
        marker_stroke(x + 13, y + 15, x + 4, y + 8, color);
        marker_stroke(x + 4, y + 8, x + 16, y + 8, color);
        marker_stroke(x + 16, y + 8, x + 7, y + 15, color);
        marker_stroke(x + 7, y + 15, x + 10, y + 4, color);
        break;
    case MARKER_STOCKPILE: // Energy bolt.
        marker_stroke(x + 12, y + 4, x + 7, y + 11, color);
        marker_stroke(x + 7, y + 11, x + 13, y + 11, color);
        marker_stroke(x + 13, y + 11, x + 8, y + 17, color);
        break;
    default:
        write_text("?", x + 5, y + 1, color);
        break;
    }
}

void write_fitted_text(const char* value, int x, int y, int available_width,
                       int color = ColorWhite) {
    char text[128];
    snprintf(text, sizeof(text), "%s", value);
    if (Font_width(report_text_font, text) > available_width) {
        const int length = (int)strlen(text);
        for (int n = length - 1; n >= 1; --n) {
            snprintf(text, sizeof(text), "%.*s...", n, value);
            if (Font_width(report_text_font, text) <= available_width) {
                break;
            }
            text[0] = '\0';
        }
    }
    write_text(text, x, y, color);
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

void draw_empire_summary(int faction_id, int top_y) {
    const Faction& faction = Factions[faction_id];
    const EmpireStats s = collect_empire_stats(faction_id);
    char line[256];

    snprintf(line, sizeof(line),
        "Bases: %d    Population: %d    Average: %d.%d    Units: %d",
        s.bases, s.population,
        s.bases ? s.population / s.bases : 0,
        s.bases ? (10 * (s.population % s.bases)) / s.bases : 0,
        s.units);
    write_text(line, 20, top_y + 0);

    snprintf(line, sizeof(line),
        "Treasury: %d EC    Base economy: %d EC    Psych: %d",
        faction.energy_credits, s.economy, s.psych);
    write_text(line, 20, top_y + 22);

    snprintf(line, sizeof(line),
        "Labs (last upkeep): %d    Energy surplus: %d    Commerce: %d    Facility maint: %d",
        faction.labs_total, s.energy, faction.turn_commerce_income, faction.facility_maint_total);
    write_text(line, 20, top_y + 44);

    snprintf(line, sizeof(line),
        "Nutrient surplus: %+d    Mineral surplus: %+d    Base labs: %d",
        s.nutrients, s.minerals, s.labs);
    write_text(line, 20, top_y + 66);

    snprintf(line, sizeof(line),
        "Golden Ages: %d    Drone riots: %d    Starving: %d    Eco-damage bases: %d",
        s.golden_ages, s.riots, s.starving, s.eco_bases);
    write_text(line, 20, top_y + 88, s.riots || s.starving ? ColorYellow : ColorWhite);

    // These are live faction values. Keep the existing five-line summary
    // intact on narrower screens instead of overlapping its labels.
    const bool side_by_side = report_width >= 1200;
    {
        const int x = side_by_side ? report_width * 60 / 100 : 20;
        const int detail_top = side_by_side ? top_y : top_y + 115;
        snprintf(line, sizeof(line), "Allocation: Econ %d%%  Labs %d%%  Psych %d%%",
            100 - 10 * faction.SE_alloc_labs - 10 * faction.SE_alloc_psych,
            10 * faction.SE_alloc_labs, 10 * faction.SE_alloc_psych);
        write_fitted_text(line, x, detail_top + 0, report_width - x - 20, ColorCyan);

        snprintf(line, sizeof(line), "Sanctions: %d turns   Orbit N/M/E/D: %d/%d/%d/%d",
            faction.sanction_turns, faction.satellites_nutrient,
            faction.satellites_mineral, faction.satellites_energy, faction.satellites_ODP);
        write_fitted_text(line, x, detail_top + 22, report_width - x - 20,
            faction.sanction_turns ? ColorYellow : ColorWhite);

        const int models[4] = { faction.SE_Politics, faction.SE_Economics,
            faction.SE_Values, faction.SE_Future };
        const char* names[4] = { "?", "?", "?", "?" };
        for (int i = 0; i < 4; ++i) {
            if (models[i] >= 0 && models[i] < 4 && SocialField[i].soc_name[models[i]]) {
                names[i] = SocialField[i].soc_name[models[i]];
            }
        }
        snprintf(line, sizeof(line), "SE: %s / %s / %s / %s",
            names[0], names[1], names[2], names[3]);
        write_fitted_text(line, x, detail_top + 66, report_width - x - 20);
        snprintf(line, sizeof(line), "SE ratings: Eff %+d  Growth %+d  Police %+d",
            faction.SE_effic, faction.SE_growth, faction.SE_police);
        write_fitted_text(line, x, detail_top + 88, report_width - x - 20);
    }
}

void draw_research(int faction_id, int top_y) {
    const Faction& faction = Factions[faction_id];
    char line[256];
    const int tech_id = faction.tech_research_id;
    if (tech_id < 0 || tech_id >= TECH_TranT + 1) {
        write_text("Research: None selected", 20, top_y + 0, ColorCyan);
        return;
    }

    snprintf(line, sizeof(line), "Researching: %s", tech_str(tech_id));
    write_text(line, 20, top_y + 0, ColorCyan);

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
    write_text(line, 20, top_y + 23);

    if (remaining == 0) {
        snprintf(line, sizeof(line), "Research threshold met");
    } else if (estimate > 0) {
        const int64_t turns = (remaining + estimate - 1) / estimate;
        snprintf(line, sizeof(line), "Estimated turns remaining: %lld (at current output)",
            (long long)turns);
    } else {
        snprintf(line, sizeof(line), "Estimated turns remaining: N/A (no recent lab output)");
    }
    write_text(line, 20, top_y + 45);
}

void draw_report_panel(int left, int top, int right, int bottom) {
    if (right - left < 12 || bottom - top < 12) return;
    RECT outer = { left, top, right, bottom };
    RECT inner = { left + 3, top + 3, right - 3, bottom - 3 };
    // The exact two-pass native screen-panel sequence used by control_turn()
    // for MainInfc panels. Reuse game-owned BoxSpriteParams, not imitated lines
    // or guessed layouts. The caller supplies only bounded destination rects.
    Buffer_box_sprite(&report_window.oCanvas, &outer, dword_78D870);
    Buffer_box_sprite(&report_window.oCanvas, &inner, dword_78D7F8);
}
void write_native_heading(const char* text, int x, int y, int color) {
    Font* bold = &StatusWin->oFontBold;
    // Status-window fonts are from the game, not MapWin's zoom-scaled font.
    // Fall back to the existing stable report font if geometry is unexpected.
    if (bold->fontObj && bold->lineHeight >= 12 && bold->lineHeight <= 24) {
        Buffer_set_font(&report_window.oCanvas, bold, 0, 0, 0);
        write_text(text, x, y, color);
        Buffer_set_font(&report_window.oCanvas, report_text_font, 0, 0, 0);
    } else {
        write_text(text, x, y, color);
    }
}
// A live Stockpile Energy order is not proof of a production completion;
// a turn-specific completed-items list will require UI-002 event capture.
int overview_stockpile_base_at(int faction_id, int ordinal) {
    if (ordinal < 0) return -1;
    for (int id = 0; id < *BaseCount; ++id) {
        if (Bases[id].faction_id == faction_id
            && Bases[id].item() == -FAC_STOCKPILE_ENERGY) {
            if (ordinal-- == 0) return id;
        }
    }
    return -1;
}
int overview_stockpile_count(int faction_id) {
    int result = 0;
    for (int id = 0; id < *BaseCount; ++id)
        result += Bases[id].faction_id == faction_id
            && Bases[id].item() == -FAC_STOCKPILE_ENERGY;
    return result;
}

struct OverviewPane { int left, top, right, bottom; };
struct OverviewLowerLayout { OverviewPane attention, stockpile; };
OverviewLowerLayout overview_lower_layout() {
    const int top = report_subtab_y() + 13;
    const int summary_height = report_width >= 1200 ? 145 : 242;
    const int lower_top = top + summary_height + 14 + 118;
    const int bottom = report_height - 66;
    if (report_width >= 850) {
        const int centre = report_width / 2;
        return {
            { 12, lower_top, centre - 6, bottom },
            { centre + 6, lower_top, report_width - 12, bottom }
        };
    }
    const int middle = lower_top + (bottom - lower_top) / 2;
    return {
        { 12, lower_top, report_width - 12, middle - 6 },
        { 12, middle + 6, report_width - 12, bottom }
    };
}
int overview_pane_rows(const OverviewPane& pane) {
    return max(0, (pane.bottom - pane.top - 74) / BASE_ROW_HEIGHT);
}
int overview_row_hit(const OverviewPane& pane, int x, int y) {
    if (x < pane.left + 8 || x >= pane.right - 8
        || y < pane.top + 34 || y >= pane.bottom - 16) return -1;
    const int row = (y - pane.top - 34) / BASE_ROW_HEIGHT;
    return row < overview_pane_rows(pane) ? row : -1;
}
void draw_overview_attention(int faction_id, const OverviewPane& pane) {
    if (pane.bottom - pane.top < 56) return;
    draw_report_panel(pane.left, pane.top, pane.right, pane.bottom);
    const int flagged = count_attention_bases(faction_id);
    char label[80];
    snprintf(label, sizeof(label), "COLONIES REQUIRING ATTENTION: %d", flagged);
    write_native_heading(label, pane.left + 12, pane.top + 8,
        flagged ? ColorYellow : ColorGreen);
    if (!flagged) {
        write_fitted_text("No colonies currently flagged.", pane.left + 12,
            pane.top + 36, pane.right - pane.left - 24, ColorGreen);
        return;
    }
    const int visible = min(flagged, overview_pane_rows(pane));
    const int detail_x = pane.left + (pane.right - pane.left) * 57 / 100;
    for (int row = 0; row < visible; ++row) {
        const int base_id = attention_base_at(faction_id, row);
        if (base_id < 0) continue;
        const int flags = base_attention_flags(base_id);
        const int y = pane.top + 34 + BASE_ROW_HEIGHT * row;
        write_fitted_text(Bases[base_id].name, pane.left + 12, y,
            detail_x - pane.left - 20, ColorCyan);
        const char* condition = flags & ATT_RIOT ? "Riot"
            : flags & ATT_STARVING ? "Starving"
            : flags & ATT_STALLED ? "Build stalled"
            : flags & ATT_HAB_CAP ? "Hab cap" : "Eco-damage";
        write_fitted_text(condition, detail_x, y,
            pane.right - detail_x - 12,
            attention_severity(flags) == 3 ? ColorRed : ColorYellow);
    }
    if (flagged > visible) {
        snprintf(label, sizeof(label), "...and %d more; see Colonies / Attention",
            flagged - visible);
        write_fitted_text(label, pane.left + 12, pane.bottom - 27,
            pane.right - pane.left - 24, ColorYellow);
    }
}
void draw_overview_stockpile(int faction_id, const OverviewPane& pane) {
    if (pane.bottom - pane.top < 56) return;
    draw_report_panel(pane.left, pane.top, pane.right, pane.bottom);
    const int count = overview_stockpile_count(faction_id);
    char label[96];
    snprintf(label, sizeof(label), "STOCKPILE ENERGY: %d bases", count);
    write_native_heading(label, pane.left + 12, pane.top + 8,
        count ? ColorYellow : ColorGreen);
    if (!count) {
        write_fitted_text("No bases currently using Stockpile Energy.",
            pane.left + 12, pane.top + 36,
            pane.right - pane.left - 24, ColorGreen);
        return;
    }
    const int visible = min(count, overview_pane_rows(pane));
    const int action_x = pane.left + (pane.right - pane.left) * 57 / 100;
    for (int row = 0; row < visible; ++row) {
        const int base_id = overview_stockpile_base_at(faction_id, row);
        if (base_id < 0) continue;
        const int y = pane.top + 34 + BASE_ROW_HEIGHT * row;
        write_fitted_text(Bases[base_id].name, pane.left + 12, y,
            action_x - pane.left - 20, ColorCyan);
        write_fitted_text("Change build", action_x, y,
            pane.right - action_x - 12, ColorCyan);
    }
    write_fitted_text(count > visible ? "More stockpiles in Colonies / Production"
        : "Current orders; turn completions are not yet tracked.",
        pane.left + 12, pane.bottom - 27, pane.right - pane.left - 24,
        ColorYellow);
}
void draw_empire_overview(int faction_id) {
    const int top = report_subtab_y() + 13;
    const bool wide = report_width >= 1200;
    const int summary_height = wide ? 145 : 242;
    const int summary_y = top + 28;
    const int research_panel_y = top + summary_height + 14;
    const int research_text_y = research_panel_y + 27;
    const OverviewLowerLayout lower = overview_lower_layout();
    draw_report_panel(12, top, report_width - 12, top + summary_height);
    write_native_heading("FACTION STATUS", 24, top + 7, ColorYellow);
    draw_empire_summary(faction_id, summary_y);
    draw_report_panel(12, research_panel_y, report_width - 12,
        research_panel_y + 110);
    write_native_heading("RESEARCH", 24, research_panel_y + 5, ColorYellow);
    draw_research(faction_id, research_text_y);
    draw_overview_attention(faction_id, lower.attention);
    draw_overview_stockpile(faction_id, lower.stockpile);
    write_text("Colonies / Production edits current orders. Turn event history is planned.",
        20, report_height - 39, ColorCyan);
}
void draw_intelligence_placeholder() {
    const int y = report_subtab_y() + 13;
    draw_report_panel(12, y, report_width - 12, min(report_height - 65, y + 155));
    write_native_heading("INTELLIGENCE", 24, y + 9, ColorYellow);
    write_text("Future area: known faction standings, diplomacy and espionage.",
        24, y + 42);
    write_text("No hidden data is collected or displayed by this prototype.",
        24, y + 70, ColorCyan);
    write_text("Existing colony, military and Messages reports remain available.",
        24, y + 98);
}
void draw_section_navigation() {
    for (const SectionTab& tab : SECTION_TABS) {
        const int x = section_nav_position(tab.section, nullptr);
        int y = SECTION_NAV_Y;
        section_nav_position(tab.section, &y);
        const bool active = report_section == tab.section;
        if (active) {
            RECT rect = { x - 4, y - 3, x + tab.width - 4, y + 21 };
            Buffer_box_2(&report_window.oCanvas, &rect, ColorCyan, ColorCyan);
        }
        write_text(tab.label, x + 3, y, active ? ColorYellow : ColorCyan);
    }
}

void draw_base_list(int faction_id) {
    const int total = count_bases(faction_id);
    const int rows = visible_base_rows();
    const int count = report_row_count(faction_id);
    const EmpireStats totals = collect_empire_stats(faction_id);
    base_scroll = clamp(base_scroll, 0, max(0, count - rows));

    char line[256];
    if (base_view == VIEW_MESSAGES) {
        snprintf(line, sizeof(line), "NATIVE MESSAGES    %d retained    showing %d-%d",
            count, count ? base_scroll + 1 : 0, min(count, base_scroll + rows));
    } else if (base_view == VIEW_UNITS) {
        snprintf(line, sizeof(line), "UNIT REGISTER    %d units    showing %d-%d",
            count, count ? base_scroll + 1 : 0, min(count, base_scroll + rows));
    } else if (base_view == VIEW_ATTENTION) {
        snprintf(line, sizeof(line), "ATTENTION    %d flagged bases    showing %d-%d",
            count, count ? base_scroll + 1 : 0, min(count, base_scroll + rows));
    } else if (base_view == VIEW_BEST) {
        snprintf(line, sizeof(line), "BEST BASES    %d categories    showing %d-%d",
            count, count ? base_scroll + 1 : 0, min(count, base_scroll + rows));
    } else {
        snprintf(line, sizeof(line), "BASE OVERVIEW    %d bases    showing %d-%d",
            total, total ? base_scroll + 1 : 0, min(total, base_scroll + rows));
    }
    write_text(line, 20, report_list_heading_y(), ColorCyan);


    const int x_icon = report_width * 26 / 100;
    const int x_prod = report_width * 32 / 100;
    const int x_next = report_width * 50 / 100;
    const int x_mins = report_width * 67 / 100;
    const int x_turns = report_width * 82 / 100;
    const int x_hurry = report_width * 91 / 100;
    const int x_pop = report_width * 34 / 100;
    const int x_stat_pop = report_width * 29 / 100;
    const int x_stat_n = report_width * 35 / 100;
    const int x_stat_m = report_width * 41 / 100;
    const int x_stat_e = report_width * 47 / 100;
    const int x_stat_ec = report_width * 53 / 100;
    const int x_stat_labs = report_width * 60 / 100;
    const int x_stat_loss = report_width * 68 / 100;
    const int x_stat_eco = report_width * 76 / 100;
    const int x_stat_ga = report_width * 84 / 100;
    const int x_stat_status = report_width * 89 / 100;
    const int x_workers = report_width * 45 / 100;
    const int x_talents = report_width * 57 / 100;
    const int x_drones = report_width * 69 / 100;
    const int x_specialists = report_width * 80 / 100;
    const int x_riot = report_width * 91 / 100;
    const int x_best_base = report_width * 30 / 100;
    const int x_best_value = report_width * 70 / 100;
    const int x_gov_options = report_width * 89 / 100;
    const int x_gov_citizens = report_width * 48 / 100;
    const int x_gov_production = report_width * 66 / 100;
    const int x_gov_priority = report_width * 79 / 100;
    const int x_here = report_width * 29 / 100;
    const int x_land = report_width * 37 / 100;
    const int x_sea = report_width * 45 / 100;
    const int x_air = report_width * 53 / 100;
    const int x_combat = report_width * 61 / 100;
    const int x_police = report_width * 72 / 100;
    const int x_home = report_width * 82 / 100;
    const int x_away = report_width * 91 / 100;
    const int x_growth_cap = report_width * 38 / 100;
    const int x_growth_stored = report_width * 49 / 100;
    const int x_growth_surplus = report_width * 62 / 100;
    const int x_growth_eta = report_width * 72 / 100;
    const int x_growth_mode = report_width * 82 / 100;
    const int x_growth_ga = report_width * 93 / 100;

    // A separate tab line leaves the base-count heading unobscured on narrow screens.
    for (const ReportTab& tab : REPORT_TABS) {
        if (section_for_view(tab.view) != report_section) continue;
        char caption[32];
        if (base_view == tab.view) {
            snprintf(caption, sizeof(caption), "[%s]", tab.label);
        } else {
            snprintf(caption, sizeof(caption), "%s", tab.label);
        }
        const int tab_x = report_tab_position(tab.view, nullptr);
        write_text(caption, tab_x, report_tab_y(tab.view),
            base_view == tab.view ? ColorYellow : ColorCyan);
    }

    if (base_view == VIEW_MESSAGES) {
        write_text("Messages currently retained in the game's native UI",
            20, report_column_y(), ColorYellow);
        if (!count) {
            write_text("No readable native message text available.",
                20, report_first_row_y(), ColorYellow);
            write_text("Message records may be references rather than text.",
                20, report_first_row_y() + 24, ColorCyan);
        }
        snprintf(line, sizeof(line), "Native list: %d entries / %d texts; detail: %d / %d",
            message_status.listbox_entries, message_status.listbox_texts,
            message_status.detail_entries, message_status.detail_texts);
        write_fitted_text(line, 20, report_height - 64,
            report_width - 36, ColorCyan);
        for (int row = 0; row < rows && base_scroll + row < count; ++row) {
            char raw[1024] = {};
            if (!native_message_text(base_scroll + row, raw, sizeof(raw))) {
                continue;
            }
            // Preserve the text, but normalize embedded newlines/tabs so
            // the report can display an entry across two 17px font rows.
            std::string message(raw);
            for (char& ch : message) {
                if (ch == '\r' || ch == '\n' || ch == '\t') ch = ' ';
            }
            const int y = report_first_row_y() + MESSAGE_ROW_HEIGHT * row;
            snprintf(line, sizeof(line), "%d.", base_scroll + row + 1);
            write_text(line, 20, y, ColorCyan);
            const int x = 55;
            const int width = report_width - x - 16;
            size_t split = 0;
            size_t last_space = 0;
            while (split < message.size()) {
                if (message[split] == ' ') last_space = split;
                const std::string test = message.substr(0, split + 1);
                if (Font_width(report_text_font, test.c_str()) > width) break;
                ++split;
            }
            if (split < message.size() && last_space > 0) split = last_space;
            if (!split && !message.empty()) split = 1;
            const std::string top = message.substr(0, split);
            const std::string remaining = message.substr(split);
            write_fitted_text(top.c_str(), x, y, width);
            if (!remaining.empty()) {
                const size_t first = remaining.find_first_not_of(' ');
                const std::string next = first == std::string::npos
                    ? "" : remaining.substr(first);
                write_fitted_text(next.c_str(), x, y + 19, width, ColorWhite);
            }
        }
        write_text("Native UI snapshot only; not an archive or a complete event log.",
            20, report_height - 40, ColorCyan);
        return;
    }

    if (base_view == VIEW_UNITS) {
        const int name_x = 20;
        const int triad_x = report_width * 33 / 100;
        const int loc_x = report_width * 42 / 100;
        const int home_x = report_width * 53 / 100;
        const int hp_x = report_width * 72 / 100;
        const int morale_x = report_width * 81 / 100;
        const int order_x = report_width * 91 / 100;
        snprintf(line, sizeof(line), "Filter: %s  (click to cycle)",
            UNIT_FILTER_NAMES[unit_filter]);
        write_fitted_text(line, report_width * 66 / 100, report_list_heading_y(),
            report_width * 34 / 100 - 15, ColorCyan);
        write_text("Unit / design", name_x, report_column_y(), ColorYellow);
        write_text("Domain", triad_x, report_column_y(), ColorYellow);
        write_text("Position", loc_x, report_column_y(), ColorYellow);
        write_text("Home", home_x, report_column_y(), ColorYellow);
        write_text("HP", hp_x, report_column_y(), ColorYellow);
        write_text("Morale", morale_x, report_column_y(), ColorYellow);
        write_text("Order", order_x, report_column_y(), ColorYellow);
        for (int row = 0; row < rows && base_scroll + row < count; ++row) {
            const int id = unit_at(faction_id, base_scroll + row);
            if (id < 0) continue;
            VEH& unit = Vehs[id];
            const int y = report_first_row_y() + BASE_ROW_HEIGHT * row;
            write_fitted_text(unit.name(), name_x, y, triad_x - name_x - 12);
            write_text(unit.triad() == TRIAD_LAND ? "Land"
                : unit.triad() == TRIAD_SEA ? "Sea" : "Air", triad_x, y);
            snprintf(line, sizeof(line), "%d,%d", unit.x, unit.y);
            write_text(line, loc_x, y);
            if (unit.home_base_id >= 0 && unit.home_base_id < *BaseCount
                && Bases[unit.home_base_id].faction_id == faction_id) {
                write_fitted_text(Bases[unit.home_base_id].name,
                    home_x, y, hp_x - home_x - 12);
            } else {
                write_text("--", home_x, y);
            }
            snprintf(line, sizeof(line), "%d/%d",
                unit.cur_hitpoints(), unit.max_hitpoints());
            write_text(line, hp_x, y, unit.damage_taken ? ColorYellow : ColorWhite);
            if (unit.morale <= MORALE_ELITE && Morale[unit.morale].name) {
                write_fitted_text(Morale[unit.morale].name, morale_x, y,
                    order_x - morale_x - 7);
            } else {
                snprintf(line, sizeof(line), "%d", unit.morale);
                write_text(line, morale_x, y);
            }
            const int order = unit.order & ~VehOrderResetFlag;
            const char* label = order <= ORDER_MAGTUBE_TO && Order[order].order
                ? Order[order].order : "?";
            write_fitted_text(label, order_x, y,
                report_width - order_x - 10);
        }
        write_text("Live owned units. Cycle filter above; no orders are issued.",
            20, report_height - 40, ColorCyan);
        return;
    }

    if (base_view == VIEW_ATTENTION) {
        const int x_reason = report_width * 37 / 100;
        const int x_severity = report_width * 86 / 100;
        write_text("Base", 20, report_column_y(), ColorYellow);
        write_text("Current warnings", x_reason, report_column_y(), ColorYellow);
        write_text("Level", x_severity, report_column_y(), ColorYellow);
        if (!count) write_text("No bases currently flagged.", 20, report_first_row_y(), ColorGreen);
        for (int row = 0; row < rows && base_scroll + row < count; ++row) {
            const int base_id = attention_base_at(faction_id, base_scroll + row);
            if (base_id < 0) continue;
            const BASE& base = Bases[base_id];
            const int flags = base_attention_flags(base_id);
            const int severity = attention_severity(flags);
            const int y = report_first_row_y() + BASE_ROW_HEIGHT * row;
            write_fitted_text(base.name, 20, y, x_reason - 30, ColorCyan);
            std::string issues;
            if (flags & ATT_RIOT) issues += "Riot  ";
            if (flags & ATT_STARVING) issues += "Starving  ";
            if (flags & ATT_STALLED) issues += "Production stalled  ";
            if (flags & ATT_HAB_CAP) issues += "Hab cap  ";
            if (flags & ATT_ECO) {
                snprintf(line, sizeof(line), "Eco: %d%%", base_eco_display_percent(base_id));
                issues += line;
            }
            write_fitted_text(issues.c_str(), x_reason, y,
                x_severity - x_reason - 12, severity == 3 ? ColorRed : ColorYellow);
            write_text(severity == 3 ? "Critical" : severity == 2 ? "Warning" : "Advisory",
                x_severity, y, severity == 3 ? ColorRed
                : severity == 2 ? ColorYellow : ColorCyan);
        }
        write_text("Current-state warnings only; click a base to open it.",
            20, report_height - 40, ColorCyan);
        return;
    }

    if (base_view == VIEW_BEST) {
        write_text("Category", 20, report_column_y(), ColorYellow);
        write_text("Best Base", x_best_base, report_column_y(), ColorYellow);
        write_text("Value", x_best_value, report_column_y(), ColorYellow);
        write_text("Action", x_gov_options, report_column_y(), ColorYellow);
        for (int row = 0; row < rows && base_scroll + row < count; ++row) {
            const BestMetric metric = (BestMetric)(base_scroll + row);
            const BestBase best = best_base_for(faction_id, metric);
            if (best.base_id < 0) {
                continue;
            }
            const int y = report_first_row_y() + BASE_ROW_HEIGHT * row;
            write_text(BEST_METRIC_NAMES[metric], 20, y, ColorYellow);
            write_fitted_text(Bases[best.base_id].name, x_best_base, y,
                x_best_value - x_best_base - 14, ColorCyan);
            snprintf(line, sizeof(line), "%d", best.value);
            write_text(line, x_best_value, y);
            write_text("Gov", x_gov_options, y, ColorCyan);
        }
        write_text("Click a leader to visit the base; Gov opens its native settings",
            20, report_height - 40, ColorCyan);
        return;
    }

    write_text("Base", 20, report_column_y(), ColorYellow);
    if (base_view == VIEW_GARRISON) {
        write_text("Here", x_here, report_column_y(), ColorYellow);
        write_text("Land", x_land, report_column_y(), ColorYellow);
        write_text("Sea", x_sea, report_column_y(), ColorYellow);
        write_text("Air", x_air, report_column_y(), ColorYellow);
        write_text("Combat", x_combat, report_column_y(), ColorYellow);
        write_text("Police", x_police, report_column_y(), ColorYellow);
        write_text("Home", x_home, report_column_y(), ColorYellow);
        write_text("Away", x_away, report_column_y(), ColorYellow);
    } else if (base_view == VIEW_GROWTH) {
        write_text("Pop", x_here, report_column_y(), ColorYellow);
        write_text("Cap", x_growth_cap, report_column_y(), ColorYellow);
        write_text("Stored", x_growth_stored, report_column_y(), ColorYellow);
        write_text("N/turn", x_growth_surplus, report_column_y(), ColorYellow);
        write_text("ETA", x_growth_eta, report_column_y(), ColorYellow);
        write_text("Growth", x_growth_mode, report_column_y(), ColorYellow);
        write_text("GA", x_growth_ga, report_column_y(), ColorYellow);
    } else if (base_view == VIEW_ECONOMY) {
        write_text("EC", x_here, report_column_y(), ColorYellow);
        write_text("EC%", x_land, report_column_y(), ColorYellow);
        write_text("Labs", x_sea, report_column_y(), ColorYellow);
        write_text("Lab%", x_air, report_column_y(), ColorYellow);
        write_text("Psych", x_combat, report_column_y(), ColorYellow);
        write_text("Psy%", x_police, report_column_y(), ColorYellow);
        write_text("Loss", x_home, report_column_y(), ColorYellow);
        write_text("E surplus", x_away, report_column_y(), ColorYellow);
    } else if (base_view == VIEW_DEFENSES) {
        write_text("Perim", x_here, report_column_y(), ColorYellow);
        write_text("Tach", x_land, report_column_y(), ColorYellow);
        write_text("Cmd", x_sea, report_column_y(), ColorYellow);
        write_text("Naval", x_air, report_column_y(), ColorYellow);
        write_text("Aero", x_combat, report_column_y(), ColorYellow);
        write_text("Bio", x_police, report_column_y(), ColorYellow);
        write_text("GSP", x_home, report_column_y(), ColorYellow);
        write_text("Active", x_away, report_column_y(), ColorYellow);
    } else if (base_view == VIEW_GOVERNORS) {
        write_text("Governor", x_pop, report_column_y(), ColorYellow);
        write_text("Citizens", x_gov_citizens, report_column_y(), ColorYellow);
        write_text("Builds", x_gov_production, report_column_y(), ColorYellow);
        write_text("Priority", x_gov_priority, report_column_y(), ColorYellow);
        write_text("Action", x_gov_options, report_column_y(), ColorYellow);
    } else if (base_view == VIEW_PRODUCTION) {
        write_text("Icon", x_icon, report_column_y(), ColorYellow);
        write_text("Building", x_prod, report_column_y(), ColorYellow);
        write_text("Next in queue", x_next, report_column_y(), ColorYellow);
        write_text("Minerals", x_mins, report_column_y(), ColorYellow);
        write_text("Turns", x_turns, report_column_y(), ColorYellow);
        write_text("Action", x_hurry, report_column_y(), ColorYellow);
    } else if (base_view == VIEW_CITIZENS) {
        write_text("Pop", x_pop, report_column_y(), ColorYellow);
        write_text("Workers", x_workers, report_column_y(), ColorYellow);
        write_text("Talents", x_talents, report_column_y(), ColorYellow);
        write_text("Drones", x_drones, report_column_y(), ColorYellow);
        write_text("Specs", x_specialists, report_column_y(), ColorYellow);
        write_text("Riot", x_riot, report_column_y(), ColorYellow);
    } else {
        write_text("Pop", x_stat_pop, report_column_y(), ColorYellow);
        write_text("N", x_stat_n, report_column_y(), ColorYellow);
        write_text("M", x_stat_m, report_column_y(), ColorYellow);
        write_text("E", x_stat_e, report_column_y(), ColorYellow);
        write_text("EC", x_stat_ec, report_column_y(), ColorYellow);
        write_text("Labs", x_stat_labs, report_column_y(), ColorYellow);
        write_text("Loss", x_stat_loss, report_column_y(), ColorYellow);
        write_text("Eco%", x_stat_eco, report_column_y(), ColorYellow);
        write_text("GA", x_stat_ga, report_column_y(), ColorYellow);
        write_text("Status", x_stat_status, report_column_y(), ColorYellow);
    }

    int seen = 0;
    int drawn = 0;
    for (int i = 0; i < *BaseCount && drawn < rows; ++i) {
        BASE& base = Bases[i];
        if (base.faction_id != faction_id) {
            continue;
        }
        if (seen++ < base_scroll) {
            continue;
        }
        const int y = report_first_row_y() + BASE_ROW_HEIGHT * drawn++;
        // Cyan names are clickable; resolve by current array index after scrolling.
        const int base_name_end = base_view == VIEW_PRODUCTION ? x_icon
            : (base_view == VIEW_GARRISON || base_view == VIEW_GROWTH
                || base_view == VIEW_ECONOMY || base_view == VIEW_DEFENSES) ? x_here
            : base_view == VIEW_STATUS ? x_stat_pop : x_pop;
        write_fitted_text(base.name, 20, y, base_name_end - 30, ColorCyan);

        if (base_view == VIEW_GROWTH) {
            const bool has_dome = has_fac_built(FAC_HABITATION_DOME, i);
            const int modifier = (has_project(FAC_ASCETIC_VIRTUES, faction_id) ? 2 : 0)
                - MFactions[faction_id].rule_population;
            const int cap = (has_fac_built(FAC_HAB_COMPLEX, i)
                ? Rules->pop_limit_wo_hab_dome : Rules->pop_limit_wo_hab_complex) + modifier;
            const bool capped = base.pop_size >= MaxBasePopSize
                || (!has_dome && base.pop_size >= cap);
            const int nutrient_cost = (base.pop_size + 1)
                * mod_cost_factor(faction_id, RSC_NUTRIENT, i);
            snprintf(line, sizeof(line), "%d", base.pop_size);
            write_text(line, x_here, y);
            if (has_dome) {
                write_text("Dome", x_growth_cap, y, ColorGreen);
            } else {
                snprintf(line, sizeof(line), "%d", cap);
                write_text(line, x_growth_cap, y,
                    capped ? ColorYellow : ColorWhite);
            }
            snprintf(line, sizeof(line), "%d/%d",
                max(0, base.nutrients_accumulated), nutrient_cost);
            write_fitted_text(line, x_growth_stored, y,
                x_growth_surplus - x_growth_stored - 7);
            snprintf(line, sizeof(line), "%+d", base.nutrient_surplus);
            write_text(line, x_growth_surplus, y,
                base.nutrient_surplus < 0 ? ColorRed : ColorWhite);
            const char* mode = "Normal";
            const char* eta = "--";
            int color = ColorWhite;
            char eta_buffer[32];
            if (base.nutrient_surplus < 0) {
                mode = "Starve";
                color = ColorRed;
            } else if (capped) {
                mode = "Capped";
                color = ColorYellow;
            } else if (base_pop_boom(i) && base.nutrients_accumulated >= 0) {
                mode = "Boom";
                eta = "1";
                color = ColorGreen;
            } else if (base.SE_growth(SE_Pending) <= -3) {
                mode = "Blocked";
                color = ColorYellow;
            } else if (base.nutrient_surplus > 0) {
                const int needed = max(0, nutrient_cost - base.nutrients_accumulated);
                // Approximate; native growth checks accumulation before the
                // current turn's nutrient surplus, so do not imply an exact ETA.
                snprintf(eta_buffer, sizeof(eta_buffer), "~%d",
                    1 + (needed + base.nutrient_surplus - 1) / base.nutrient_surplus);
                eta = eta_buffer;
            } else {
                mode = "Stalled";
            }
            write_text(eta, x_growth_eta, y, color);
            write_fitted_text(mode, x_growth_mode, y,
                x_growth_ga - x_growth_mode - 5, color);
            write_text(base.golden_age_active() ? "GA" : "--", x_growth_ga, y,
                base.golden_age_active() ? ColorGreen : ColorWhite);
            continue;
        }
        if (base_view == VIEW_ECONOMY) {
            const int values[3] = { base.economy_total, base.labs_total, base.psych_total };
            const int sums[3] = { totals.economy, totals.labs, totals.psych };
            const int value_x[3] = { x_here, x_sea, x_combat };
            const int share_x[3] = { x_land, x_air, x_police };
            for (int k = 0; k < 3; ++k) {
                snprintf(line, sizeof(line), "%d", values[k]);
                write_text(line, value_x[k], y);
                const int pct = sums[k] > 0
                    ? (int)(int64_t(values[k]) * 100 / sums[k]) : 0;
                snprintf(line, sizeof(line), "%d%%", pct);
                write_text(line, share_x[k], y);
            }
            snprintf(line, sizeof(line), "%d", base.energy_inefficiency);
            write_text(line, x_home, y,
                base.energy_inefficiency > 0 ? ColorRed : ColorWhite);
            snprintf(line, sizeof(line), "%+d", base.energy_surplus);
            write_text(line, x_away, y,
                base.energy_surplus < 0 ? ColorRed : ColorWhite);
            continue;
        }
        if (base_view == VIEW_DEFENSES) {
            // B = physical facility, * = available via an empire-wide effect.
            // Do not claim an identical combat bonus in every type of battle.
            const FacilityId facilities[] = { FAC_PERIMETER_DEFENSE,
                FAC_TACHYON_FIELD, FAC_COMMAND_CENTER, FAC_NAVAL_YARD,
                FAC_AEROSPACE_COMPLEX, FAC_BIOENHANCEMENT_CENTER,
                FAC_GEOSYNC_SURVEY_POD };
            const int columns[] = { x_here, x_land, x_sea, x_air,
                x_combat, x_police, x_home };
            int active = 0;
            for (int k = 0; k < 7; ++k) {
                const bool built = !!has_fac_built(facilities[k], i);
                const bool available = built || has_facility(facilities[k], i);
                active += available;
                write_text(built ? "B" : available ? "*" : "--",
                    columns[k], y, built ? ColorGreen
                    : available ? ColorCyan : ColorWhite);
            }
            snprintf(line, sizeof(line), "%d/7", active);
            write_text(line, x_away, y);
            continue;
        }
        if (base_view == VIEW_GARRISON) {
            const GarrisonStats g = collect_garrison(base, i, faction_id);
            snprintf(line, sizeof(line), "%d", g.here);
            write_text(line, x_here, y, g.here ? ColorWhite : ColorYellow);
            snprintf(line, sizeof(line), "%d", g.land);
            write_text(line, x_land, y);
            snprintf(line, sizeof(line), "%d", g.sea);
            write_text(line, x_sea, y);
            snprintf(line, sizeof(line), "%d", g.air);
            write_text(line, x_air, y);
            snprintf(line, sizeof(line), "%d", g.combat);
            write_text(line, x_combat, y);
            snprintf(line, sizeof(line), "%d", g.police);
            write_text(line, x_police, y);
            snprintf(line, sizeof(line), "%d", g.home);
            write_text(line, x_home, y);
            snprintf(line, sizeof(line), "%d", g.away);
            write_text(line, x_away, y);
            continue;
        }
        if (base_view == VIEW_PRODUCTION) {
            const int item = base.item();
            const bool stockpile = item == -FAC_STOCKPILE_ENERGY;
            draw_production_marker(item, x_icon, y - 1);
            // Cyan denotes separately clickable native Change and Queue
            // actions. Neither report action writes queue_items directly.
            write_fitted_text(prod_name(item), x_prod, y,
                x_next - x_prod - 10, ColorCyan);
            // The queue size counts entries after the current item. Never
            // inspect queue_items[1] for an empty or corrupt queue.
            if (base.queue_size > 0 && base.queue_size < 10) {
                const int next_item = base.queue_items[1];
                if (next_item >= -SP_ID_Last && next_item < MaxProtoNum) {
                    write_fitted_text(prod_name(next_item), x_next, y,
                        x_mins - x_next - 10, ColorCyan);
                } else {
                    write_text("Invalid", x_next, y, ColorRed);
                }
            } else {
                write_text(base.queue_size == 0 ? "None" : "Invalid",
                    x_next, y, base.queue_size == 0 ? ColorCyan : ColorRed);
            }
            if (!stockpile) {
                const int total_minerals = mineral_cost(i, item);
                const int needed = max(0, total_minerals - base.minerals_accumulated);
                snprintf(line, sizeof(line), "%d/%d (+%d)",
                    base.minerals_accumulated, total_minerals, base.mineral_surplus);
                write_fitted_text(line, x_mins, y, x_turns - x_mins - 8);
                if (needed == 0) {
                    write_text("Ready", x_turns, y, ColorGreen);
                } else if (base.mineral_surplus > 0) {
                    snprintf(line, sizeof(line), "~%d",
                        (needed + base.mineral_surplus - 1) / base.mineral_surplus);
                    write_text(line, x_turns, y);
                } else {
                    write_text("Stalled", x_turns, y, ColorRed);
                }
                if (needed > 0 && base.can_hurry_item()) {
                    write_text("Hurry", x_hurry, y, ColorCyan);
                }
            } else {
                write_text("--", x_mins, y);
                write_text("--", x_turns, y);
            }
            continue;
        }
        if (base_view == VIEW_GOVERNORS) {
            const uint32_t flags = base.governor_flags;
            write_text((flags & GOV_ACTIVE) ? "On" : "Off", x_pop, y,
                (flags & GOV_ACTIVE) ? ColorGreen : ColorYellow);
            write_text((flags & GOV_MANAGE_CITIZENS) ? "Auto" : "Manual",
                x_gov_citizens, y);
            write_text((flags & GOV_MANAGE_PRODUCTION) ? "Auto" : "Manual",
                x_gov_production, y);

            // Four independent priorities may be combined in Thinker.
            char priorities[12];
            snprintf(priorities, sizeof(priorities), "%c%c%c%c",
                (flags & GOV_PRIORITY_EXPLORE) ? 'E' : '-',
                (flags & GOV_PRIORITY_DISCOVER) ? 'D' : '-',
                (flags & GOV_PRIORITY_BUILD) ? 'B' : '-',
                (flags & GOV_PRIORITY_CONQUER) ? 'C' : '-');
            write_text(priorities, x_gov_priority, y);
            write_text("Edit", x_gov_options, y, ColorCyan);
            continue;
        }
        if (base_view == VIEW_CITIZENS) {
            // Matches the breakdown used by the native base-screen economy display.
            // These are live counts only; citizen assignment stays in BaseWin.
            const int workers = base.pop_size
                - base.talent_total - base.drone_total - base.specialist_total;
            snprintf(line, sizeof(line), "%d", base.pop_size);
            write_text(line, x_pop, y);
            snprintf(line, sizeof(line), "%d", workers);
            write_text(line, x_workers, y, workers < 0 ? ColorRed : ColorWhite);
            snprintf(line, sizeof(line), "%d", base.talent_total);
            write_text(line, x_talents, y);
            snprintf(line, sizeof(line), "%d", base.drone_total);
            write_text(line, x_drones, y, base.drone_total > 0 ? ColorYellow : ColorWhite);
            snprintf(line, sizeof(line), "%d", base.specialist_total);
            write_text(line, x_specialists, y);
            if (base.state_flags & BSTATE_DRONE_RIOTS_ACTIVE) {
                write_text("Riot", x_riot, y, ColorRed);
            }
            continue;
        }
        snprintf(line, sizeof(line), "%d", base.pop_size);
        write_text(line, x_stat_pop, y);
        snprintf(line, sizeof(line), "%+d", base.nutrient_surplus);
        write_text(line, x_stat_n, y, base.nutrient_surplus < 0 ? ColorRed : ColorWhite);
        snprintf(line, sizeof(line), "%+d", base.mineral_surplus);
        write_text(line, x_stat_m, y, base.mineral_surplus < 0 ? ColorRed : ColorWhite);
        snprintf(line, sizeof(line), "%+d", base.energy_surplus);
        write_text(line, x_stat_e, y, base.energy_surplus < 0 ? ColorRed : ColorWhite);
        snprintf(line, sizeof(line), "%d", base.economy_total);
        write_text(line, x_stat_ec, y);
        snprintf(line, sizeof(line), "%d", base.labs_total);
        write_text(line, x_stat_labs, y);
        // Positive inefficiency represents energy lost, not a beneficial gain.
        snprintf(line, sizeof(line), "%d", base.energy_inefficiency);
        write_text(line, x_stat_loss, y,
            base.energy_inefficiency > 0 ? ColorRed : ColorWhite);
        // Match the colony screen: below 100% is clean-mineral pressure,
        // while the raw eco_damage field can still be zero.
        snprintf(line, sizeof(line), "%d%%", base_eco_display_percent(i));
        write_text(line, x_stat_eco, y,
            base.eco_damage > 0 ? ColorYellow : ColorWhite);
        write_text((base.state_flags & BSTATE_GOLDEN_AGE_ACTIVE) ? "GA" : "--",
            x_stat_ga, y,
            (base.state_flags & BSTATE_GOLDEN_AGE_ACTIVE) ? ColorGreen : ColorWhite);

        const char* status = "Normal";
        int status_color = ColorWhite;
        if (base.state_flags & BSTATE_DRONE_RIOTS_ACTIVE) {
            status = "Riot";
            status_color = ColorRed;
        } else if (base.nutrient_surplus < 0) {
            status = "Starving";
            status_color = ColorRed;
        } else if (base.item() != -FAC_STOCKPILE_ENERGY
            && base.mineral_surplus <= 0
            && base.minerals_accumulated < mineral_cost(i, base.item())) {
            status = "Stalled";
            status_color = ColorRed;
        } else if (base.eco_damage > 0) {
            status = "Eco-damage";
            status_color = ColorYellow;
        }
        write_fitted_text(status, x_stat_status, y,
            report_width - x_stat_status - 10, status_color);
    }

    if (base_view == VIEW_PRODUCTION) {
        write_text("Building = Change Production; Next (including None) = edit Queue.",
            20, report_height - 40, ColorCyan);
    } else if (base_view == VIEW_GROWTH) {
        write_text("ETA is approximate; Dome removes the normal habitation limit.",
            20, report_height - 40, ColorCyan);
    } else if (base_view == VIEW_ECONOMY) {
        write_text("Each percentage is the base's share of current empire output.",
            20, report_height - 40, ColorCyan);
    } else if (base_view == VIEW_DEFENSES) {
        write_text("B = built here; * = available via another effect; -- = unavailable.",
            20, report_height - 40, ColorCyan);
    } else if (base_view == VIEW_GARRISON) {
        write_text("Here = on base tile; Home/Away = assigned home base, not support cost",
            20, report_height - 40, ColorCyan);
    } else {
        write_text("Scroll: wheel / Up / Down / PgUp / PgDn / Home / End",
            20, report_height - 40, ColorCyan);
    }
}

// Native GraphicWin mouse callback; coordinates are relative to the report.
// Only the base-name column responds, not the rest of the row.
void __cdecl on_report_left_button_up(int x, int y) {
    const int faction_id = report_faction();
    if (faction_id < 0 || !empire_report_visible()) {
        return;
    }
    // Main section buttons always take precedence over data hitboxes.
    for (const SectionTab& section : SECTION_TABS) {
        int section_y = SECTION_NAV_Y;
        const int section_x = section_nav_position(section.section, &section_y);
        if (x >= section_x - 4 && x < section_x + section.width - 4
            && y >= section_y - 4 && y < section_y + 22) {
            if (report_section != section.section) {
                report_section = section.section;
                if (report_section == SECTION_COLONIES) base_view = last_colony_view;
                if (report_section == SECTION_MILITARY) base_view = last_military_view;
                if (report_section == SECTION_RECORDS) base_view = VIEW_MESSAGES;
                base_scroll = 0;
                GraphicWin_redraw(&report_window);
            }
            return;
        }
    }
    if (report_section == SECTION_OVERVIEW) {
        const OverviewLowerLayout lower = overview_lower_layout();
        const int attention_row = overview_row_hit(lower.attention, x, y);
        if (attention_row >= 0 && attention_row < count_attention_bases(faction_id)) {
            const int base_id = attention_base_at(faction_id, attention_row);
            if (base_id >= 0 && base_id < *BaseCount
                && Bases[base_id].faction_id == faction_id) {
                empire_report_hide();
                BaseWin_zoom(BaseWin, base_id, 1);
            }
            return;
        }
        const int stockpile_row = overview_row_hit(lower.stockpile, x, y);
        if (stockpile_row >= 0 && stockpile_row < overview_stockpile_count(faction_id)) {
            const int base_id = overview_stockpile_base_at(faction_id, stockpile_row);
            if (base_id >= 0 && base_id < *BaseCount
                && Bases[base_id].faction_id == faction_id
                && Bases[base_id].item() == -FAC_STOCKPILE_ENERGY) {
                open_colony_production(base_id, COLONY_CHANGE_BUILD);
            }
            return;
        }
        return;
    }
    if (report_section == SECTION_INTELLIGENCE) return;
    if (base_view == VIEW_UNITS && y >= report_list_heading_y() - 4 && y < report_list_heading_y() + 21
        && x >= report_width * 66 / 100 && x < report_width - 10) {
        unit_filter = (UnitFilter)((unit_filter + 1) % UNIT_FILTER_COUNT);
        base_scroll = 0;
        GraphicWin_redraw(&report_window);
        return;
    }
    // Resolve tab hitboxes using the same flow geometry as drawing.
    for (const ReportTab& tab : REPORT_TABS) {
        if (section_for_view(tab.view) != report_section) continue;
        const int tab_x = report_tab_position(tab.view, nullptr);
        const int tab_top = report_tab_y(tab.view) - 4;
        if (y >= tab_top && y < tab_top + 27
            && x >= tab_x && x < tab_x + tab.width) {
            if (base_view != tab.view) {
                base_view = tab.view;
                if (report_section == SECTION_COLONIES) last_colony_view = tab.view;
                if (report_section == SECTION_MILITARY) last_military_view = tab.view;
                base_scroll = 0;
                GraphicWin_redraw(&report_window);
            }
            return;
        }
    }
    if (y < report_first_row_y() - 3) return;
    if (base_view == VIEW_UNITS || base_view == VIEW_MESSAGES) return;  // Read-only views.
    const int name_right = (base_view == VIEW_PRODUCTION
        ? report_width * 26 / 100 // Stop before the new icon column.
        : base_view == VIEW_GARRISON || base_view == VIEW_STATUS
            || base_view == VIEW_GROWTH || base_view == VIEW_ECONOMY
            || base_view == VIEW_DEFENSES ? report_width * 29 / 100
        : base_view == VIEW_ATTENTION ? report_width * 37 / 100
        : report_width * 34 / 100) - 8;
    const int hurry_left = report_width * 91 / 100;
    const int gov_left = report_width * 89 / 100;
    const int build_left = report_width * 26 / 100 - 4;
    const int build_right = report_width * 50 / 100 - 8;
    const int queue_left = report_width * 50 / 100 - 4;
    const int queue_right = report_width * 67 / 100 - 8;
    const bool is_name_click = x >= 20 && x < name_right;
    const bool is_change_click = base_view == VIEW_PRODUCTION
        && x >= build_left && x < build_right;
    const bool is_queue_click = base_view == VIEW_PRODUCTION
        && x >= queue_left && x < queue_right;
    const bool is_hurry_click = base_view == VIEW_PRODUCTION
        && x >= hurry_left && x < report_width - 10;
    const bool is_gov_click = (base_view == VIEW_GOVERNORS || base_view == VIEW_BEST)
        && x >= gov_left && x < report_width - 10;
    const bool is_best_base_click = base_view == VIEW_BEST
        && x >= 20 && x < gov_left;
    if ((!is_name_click && !is_change_click && !is_queue_click
        && !is_hurry_click && !is_gov_click && !is_best_base_click)
        || y < report_first_row_y() - 3) {
        return;
    }
    const int row = (y - (report_first_row_y() - 3)) / BASE_ROW_HEIGHT;
    const int count = report_row_count(faction_id);
    const int rows = visible_base_rows();
    if (row < 0 || row >= rows) {
        return;
    }
    const int ordinal = clamp(base_scroll, 0, max(0, count - rows)) + row;
    if (ordinal >= count) {
        return;
    }
    int base_id = -1;
    if (base_view == VIEW_BEST) {
        base_id = best_base_for(faction_id, (BestMetric)ordinal).base_id;
    } else if (base_view == VIEW_ATTENTION) {
        base_id = attention_base_at(faction_id, ordinal);
    } else {
        // Resolve live IDs on demand because base arrays shift after captures.
        int seen = 0;
        for (int i = 0; i < *BaseCount; ++i) {
            if (Bases[i].faction_id == faction_id && seen++ == ordinal) {
                base_id = i;
                break;
            }
        }
    }
    if (base_id < 0 || base_id >= *BaseCount
        || Bases[base_id].faction_id != faction_id) {
        return;
    }
    if (is_gov_click) {
        open_governor_options(base_id);
        return;
    }
    if (is_change_click) {
        open_colony_production(base_id, COLONY_CHANGE_BUILD);
        return;
    }
    if (is_queue_click) {
        open_colony_production(base_id, COLONY_EDIT_QUEUE);
        return;
    }
    if (is_hurry_click) {
        BASE& base = Bases[base_id];
        if (base.item() == -FAC_STOCKPILE_ENERGY
            || !base.can_hurry_item()
            || base.minerals_accumulated >= mineral_cost(base_id, base.item())) {
            return;
        }
        empire_report_hide();
        BaseWin_soft_zoom(BaseWin, base_id);
        if (*CurrentBaseID == base_id && *CurrentBase == &Bases[base_id]) {
            BaseWin_hurry(BaseWin);
        }
        return;
    }
    // Keep established base-name navigation; Best also allows the category
    // label to select its leader.
    empire_report_hide();
    BaseWin_zoom(BaseWin, base_id, 1);
    return;
}

void __cdecl draw_report_window() {
    GraphicWin_fill(&report_window, 0);
    Buffer_set_font(&report_window.oCanvas, report_text_font, 0, 0, 0);
    write_native_heading("EMPIRE REPORT", 20, 23, ColorCyan);
    if (report_width >= 900) {
        char heading[128];
        snprintf(heading, sizeof(heading), "Year %d    Perihelion: %s",
            game_year(*CurrentTurn),
            (*GameState & STATE_PERIHELION_ACTIVE) ? "ACTIVE" : "inactive");
        write_fitted_text(heading, report_width * 58 / 100, 23,
            report_width * 42 / 100 - 20,
            (*GameState & STATE_PERIHELION_ACTIVE) ? ColorYellow : ColorCyan);
    }

    const int faction_id = report_faction();
    if (faction_id < 0) {
        write_text("No active faction.", 20, 65);
        return;
    }
    draw_section_navigation();
    if (report_section == SECTION_OVERVIEW) {
        draw_empire_overview(faction_id);
    } else if (report_section == SECTION_INTELLIGENCE) {
        draw_intelligence_placeholder();
    } else {
        if (base_view == VIEW_MESSAGES) refresh_native_messages();
        // Leave a clear gap around the current subtab row and the footer.
        // Do not enclose Messages: its native metadata is in the footer.
        if (base_view != VIEW_MESSAGES) {
            draw_report_panel(12, report_list_heading_y() - 11,
                report_width - 12, report_height - 65);
        }
        draw_base_list(faction_id);
    }
}

// The Windows Develop diagnostic identified Queue as flatButtons2[20]:
// button ID 20, visible and enabled, name "queue". Every native base
// FlatButton had cb_left_click == nullptr, including working controls.
// BaseWin_on_button_clicked is the verified native window-level dispatch:
// user tested both populated and empty Queue targets on Windows (7 Nov).
std::string native_button_label(const char* source) {
    std::string value;
    if (!copy_native_message_text(source, value)) return "";
    for (char& c : value) {
        if (c >= 'A' && c <= 'Z') c = (char)(c + ('a' - 'A'));
    }
    const size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

void invoke_native_queue_button() {
    // Verify identity and active state before entering the native handler.
    // ID 20 is confirmed for the tested executable; fail closed if changed.
    int queue_id = -1;
    for (FlatButton& button : BaseWin->flatButtons2) {
        if (native_button_label(button.name) != "queue") continue;
        if (button.button_id != 20 || button.disabled
            || !Win_is_visible(&button) || queue_id >= 0) {
            queue_id = -1;
            break;
        }
        queue_id = button.button_id;
    }
#ifndef BUILD_REL
    if (queue_id < 0) {
        static bool warned = false;
        if (!warned) {
            warned = true;
            MessageBoxA(*phWnd,
                "Native Queue control did not match the verified button "
                "(name=queue, ID=20, visible/enabled). No action was taken.",
                "ThinkerNeca Queue routing", MB_OK | MB_ICONWARNING);
        }
    }
#endif
    if (queue_id >= 0) {
        // Windows-verified on populated and empty queues (7 Nov 2026).
        BaseWin_on_button_clicked(BaseWin, queue_id);
    }
}

} // namespace

// A registered window message avoids colliding with the game's WM_USER
// commands or those of another installed renderer/mod.
UINT empire_report_production_message() {
    static const UINT message = RegisterWindowMessageA(
        "thinkerNeca.EmpireReport.OpenColonyProduction.v1");
    return message;
}

// The engine's existing ModWinProc receives this within BaseWin_zoom's
// modal message pump. Consume before entering the native picker, verify
// that the requested colony is still the active and visible owned base,
// and never process the same request twice.
void empire_report_dispatch_production(int base_id, uint32_t ticket) {
    if (base_id != pending_production_base_id
        || ticket != pending_production_ticket) return;
    const ColonyProductionAction action = pending_production_action;
    pending_production_base_id = -1;
    pending_production_action = COLONY_CHANGE_BUILD;
    pending_production_ticket = 0;
    if (*GameHalted || base_id < 0 || base_id >= *BaseCount
        || !Win_is_visible(BaseWin)
        || *CurrentBaseID != base_id
        || *CurrentBase != &Bases[base_id]
        || BaseWin->cur_base_id != base_id
        || Bases[base_id].faction_id != MapWin->cOwner) return;
    if (action == COLONY_CHANGE_BUILD) {
        BaseWin_production(BaseWin, 0, 0);
    } else {
        // Route the verified Queue ID through the existing native
        // window-level button handler; the coordinate/callback paths failed.
        invoke_native_queue_button();
    }
}

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
        // Larger 150% target, but preserve the game's desktop margins.
        // Small displays retain the previous size constraints; never
        // overflow a screen when its usable dimensions fall below the minima.
        const int available_width = max(1, *ScreenWidth - REPORT_SCREEN_MARGIN);
        const int available_height = max(1, *ScreenHeight - REPORT_SCREEN_MARGIN);
        report_width = min(REPORT_DESIRED_WIDTH, available_width);
        report_height = min(REPORT_DESIRED_HEIGHT, available_height);
        const int x = max(0, (*ScreenWidth - report_width) / 2);
        const int y = max(0, (*ScreenHeight - report_height) / 2);

        GraphicWin_ctor(&report_window);
        GraphicWin_init(&report_window, x, y, report_width, report_height,
            report_caption, 0, &MapWin->oMainWin, nullptr, nullptr);
        report_window.cb_win_redraw = draw_report_window;
        report_window.cb_win_l_button_up = on_report_left_button_up;
        // The original prototype used the map font, which changes size
        // with the world-map zoom. Use a separate 17-pixel UI font instead.
        Font_ctor(&report_font);
        Font_init(&report_font, "Arial", REPORT_FONT_HEIGHT, 0);
        // Inspect the resource rather than assuming Font_init's return
        // convention; retain a game UI font as fallback.
        if (!report_font.fontObj) {
            report_text_font = &StatusWin->oFontNorm;
        }
        report_window_initialized = true;
    }

    base_scroll = clamp(base_scroll, 0, max_base_scroll());
    Win_show(&report_window, 1);
    Win_bring_to_top(&report_window);
    GraphicWin_redraw(&report_window);
}
