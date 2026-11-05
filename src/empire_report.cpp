#include "gui.h"
#include "empire_report.h"

namespace {

// Retain the same in-engine window for repeated open/close tests.
GraphicWin report_window = {};
bool report_window_initialized = false;
char report_caption[] = "Empire Report - window prototype";

void write_line(char* text, int y) {
    Buffer_write_l_3(&report_window.oCanvas, text, 22, y, (int)strlen(text));
}

void __cdecl draw_report_window() {
    GraphicWin_fill(&report_window, 0);
    Buffer_set_font(&report_window.oCanvas, &MapWin->oFont1, 0, 0, 0);
    Buffer_set_text_color(&report_window.oCanvas, ColorWhite, 0, 1, 1);

    char heading[] = "Empire Report - native-window proof of concept";
    char instructions[] = "Press backtick or Esc to close.";
    write_line(heading, 26);
    write_line(instructions, 56);

    const int faction_id = MapWin->cOwner;
    if (faction_id > 0 && faction_id < MaxPlayerNum) {
        Faction* f = &Factions[faction_id];
        char details[128];
        snprintf(details, sizeof(details),
            "Bases: %d    Energy reserves: %d",
            f->base_count, f->energy_credits);
        write_line(details, 102);
    }
}

} // namespace

bool empire_report_visible() {
    return report_window_initialized
        && (report_window.winstate & WIN_ST_VISIBLE);
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
        const int width = 520;
        const int height = 190;
        const int x = max(0, (*ScreenWidth - width) / 2);
        const int y = max(0, (*ScreenHeight - height) / 2);

        GraphicWin_ctor(&report_window);
        GraphicWin_init(&report_window, x, y, width, height,
            report_caption, 0, &MapWin->oMainWin, nullptr, nullptr);
        report_window.cb_win_redraw = draw_report_window;
        report_window_initialized = true;
    }

    Win_show(&report_window, 1);
    Win_bring_to_top(&report_window);
    GraphicWin_redraw(&report_window);
}
