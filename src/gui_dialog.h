#pragma once

#include "main.h"
#include "autosim.h"

enum DiploProposal {
    DiploProposalMakeGift = 1,
    DiploProposalMakePact = 2,
    DiploProposalMakeTreaty = 3,
    DiploProposalTechTrade = 4,
    DiploProposalBuyTech = 5, // mention related prototype?
    DiploProposalNeedEnergy = 6,
    DiploProposalTradeMaps = 7,
    DiploProposalJointAttack = 8,
    DiploProposalBaseSwap = 9,
    DiploProposalCloseDialog = 10,
    DiploProposalRepayLoan = 11,
    DiploProposalCouncilVote = 12,
    DiploProposalTradeCommlink = 13,
};

enum DiploCounter {
    DiploCounterFriendship = 1, // goodwill and friendship
    DiploCounterNameAPrice = 2, // need but name your price
    DiploCounterThreaten = 3, // threaten with attack or cancel pact
    DiploCounterResearchData = 4, // valuable research data
    DiploCounterEnergyPayment = 5, // modest sum of energy credits
    DiploCounterLoanPayment = 6, // schedule of loan payments
    DiploCounterMakeGift = 7, // grant all research data
    DiploCounterGiveBase = 8, // turn over one of my bases
};

int __thiscall Popup_load_icon(Popup* This, Sprite* a2, int a3, char* a4);
// AUTO-002 classified, opt-in source-side presentation wrappers.
bool autosim_control_active();
bool autosim_ui_active();
int autosim_notice_netmsg(NetMessage* msg, const char* label, int delay,
    int style, const char* image);
int autosim_notice_popp(const char* filename, const char* label, const char* image);
int autosim_notice_netmsg2(const char* label, const char* image);
int autosim_notice_dialog(const char* label, int faction_id);
int autosim_notice_diplomacy(NetMessage* msg, const char* label, int delay,
    int style, const char* image);
enum AutosimControlRequest { AUTOSIM_CTL_NONE, AUTOSIM_CTL_PAUSE, AUTOSIM_CTL_STOP, AUTOSIM_CTL_MENU };
void autosim_request_control(AutosimControlRequest request);
void autosim_collect_queued_keys();
void autosim_collect_stop_file();
bool autosim_fast_combat_active();
// Null rendering suppresses native map and tile drawing only while the
// all-AI driver is processing; all other gameplay and map-visibility state
// remains in native code. The external launcher may request an override.
bool autosim_null_render_active();
void autosim_null_render_suppress(bool active);
void autosim_null_render_popup(const char* label);
void autosim_monitor_progress(const char* phase, int faction);
void autosim_monitor_commands();

// AUTO-002: fast native presentation throughout all-AI turn phases, not
// solely during combat. All normal human/opt-out preferences are unchanged.
// Scope this to processing phases only: keep autosave and manual Save Game
// outside the temporary override, so no modified preferences are serialized.
struct AutoScopedAutosimFastPrefs : AutoScopedCombatPrefs {
    AutoScopedAutosimFastPrefs()
        : AutoScopedCombatPrefs(autosim_fast_combat_active(),
            GamePreferences, GameMorePreferences,
            PREF_ADV_FAST_BATTLE_RESOLUTION | PREF_ADV_NO_CENTER_VEH_ORDERS,
            PREF_BSC_DONT_QUICK_MOVE_ENEMY_VEH
                | PREF_BSC_DONT_QUICK_MOVE_ALLY_VEH | PREF_AV_MAP_ANIMATIONS
                | PREF_BSC_MOUSE_EDGE_SCROLL_VIEW,
            MPREF_ADV_QUICK_MOVE_ALL_VEH | MPREF_ADV_QUICK_MOVE_VEH_ORDERS
                | MPREF_MAP_HIDE_ACTIVE_VEH_GOTO_PATH,
            MPREF_ADV_PAUSE_AFTER_BATTLES | MPREF_AV_WHOLE_VEH_BLINKS
                | MPREF_MAP_SHOW_BASE_NAMES | MPREF_MAP_SHOW_PROD_WITH_BASE_NAMES) {}
};
AutosimControlRequest autosim_take_control_request();
void autosim_notice_pop2(const char* label, const char* image, int recipient);
void autosim_interlude(int event_id, const char* text, int a3, int a4);
int __cdecl X_pop(const char* label, fp_none fn);
int __cdecl X_pop_2(const char* filename, const char* label, fp_none fn);
int __cdecl X_pop_6(const char* label, int a2, fp_none fn);
int __cdecl X_pops(const char* label, Sprite* a2, fp_none fn);
int __cdecl X_pops_11(const char* label, int a2, Sprite* a3, fp_none fn);
int __cdecl X_dialog(const char* label, int faction_id);
int __cdecl X_dialog_alt(const char* filename, const char* label, int faction_id);

