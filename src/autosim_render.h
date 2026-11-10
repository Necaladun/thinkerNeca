#pragma once
#include <stdint.h>

// Native Windows x86 presentation hooks; no gameplay or AI-visibility hooks.
bool autosim_install_null_render_hooks();

// Diagnostics only: never used as control-flow or gameplay inputs.
struct AutosimRenderCounters {
    uint32_t draw_skipped;
    uint32_t draw_forwarded;
    uint32_t screen_skipped;
    uint32_t screen_forwarded;
};
void autosim_get_render_counters(AutosimRenderCounters* result);
