#pragma once

#include "main.h"

// Existing native main-interface frame resources, already used in
// control_turn() to draw framed MainInfc panels.
extern int* const dword_78D7F8;
extern int* const dword_78D870;

void __cdecl control_turn();
int __cdecl net_upkeep_phase(int value);
void __cdecl net_upkeep();
void __cdecl mash_planes();
void __cdecl net_not_my_turn();
void __cdecl net_end_of_turn();
void __cdecl net_control_turn();
int __cdecl next_player_turn();
int __cdecl not_my_turn();
void __cdecl clear_council_notify(int faction_id);
void __cdecl clear_council_notify_2();
void __cdecl random_events(int flag);
void __cdecl alien_fauna();
void __cdecl do_fungal_towers();
void __cdecl turn_upkeep();
void __cdecl faction_upkeep(int faction_id);
// Read-only native Council window diagnostic. Patches only a verified callsite
// and forwards to the original CouncWin_init with the same __thiscall ABI.
void __thiscall autosim_trace_CouncWin_init(CouncWindow* win, int faction_id, int faction_id_2);
int __cdecl autosim_council_announcement(const char* filename, const char* label,
    int a3, const char* pcx, fp_none callback);
int __cdecl autosim_council_human_vote_offer(const char* label, int flags,
    Sprite* portrait, fp_none callback);
int __thiscall autosim_council_modal_or_resolve(CouncWindow* win,
    int a2, void* a3, void* a4);
void __cdecl repair_phase(int faction_id);
void __cdecl production_phase(int faction_id);
void __cdecl allocate_energy(int faction_id);

