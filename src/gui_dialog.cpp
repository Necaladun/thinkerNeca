
#include "gui_dialog.h"
#include "autosim.h"


int __thiscall Popup_load_icon(Popup* This, Sprite* a2, int a3, char* a4) {
    // replace inlined function sub_559000
    if (This->dialogs.spriteBox.strings.count) {
        This->sprite_icon = 0;
    } else {
        This->sprite_icon = a2;
    }
    return SpriteBox_sprite((SpriteBox*)&This->dialogs.spriteBox, a2, a4, a3);
}


// This is a runtime guard: no user-controlled input is bypassed unless
// Scenario Editor has actually handed every faction to AI. No preferences
// or native game-mode flags are changed.
bool autosim_ui_active() {
    const int owner = MapWin->cOwner;
    return conf.autosim_suppress_notices
        && auto_turn_eligible(conf.autosim_manual_turns,
            (unsigned)FactionStatus[0], *MultiplayerActive != 0,
            *PbemActive != 0, (*GameState & STATE_GAME_DONE) != 0,
            owner, owner >= 1 && owner < MaxPlayerNum && is_alive(owner));
}

int autosim_notice_netmsg(NetMessage* msg, const char* label, int delay,
    int style, const char* image) {
    // Only called at individually audited, presentation-only sites.
    // Never wrap native NetMsg_pop globally: it is used for other modes
    // and could later have interactive labels.
    if (autosim_ui_active()) {
        telemetry_record_ui_notice("netmsg", label, "suppressed_notice");
        return 0;
    }
    return NetMsg_pop(msg, label, delay, style, image);
}

int autosim_notice_popp(const char* filename, const char* label,
    const char* image) {
    // Only use where caller *ignores* return value. Do not use for
    // diplomacy, policy, build, or other gameplay choices.
    if (autosim_ui_active()) {
        telemetry_record_ui_notice("popp", label, "suppressed_notice");
        return 0;
    }
    return popp(filename, label, 0, image, 0);
}

int __cdecl X_pop(const char* label, fp_none fn)
{
    if (autosim_ui_active()) {
        const AutoUiAction action = autosim_xpop_policy(label, fn != 0);
        if (action != AUTO_UI_NATIVE) {
            telemetry_record_ui_notice("x_pop", label,
                action == AUTO_UI_NOTICE ? "suppressed_notice" : "declined_optional_human_ui");
            return 0;
        }
    }
    if (!conf.warn_on_former_replace && !strcmp(label, "MIMIMI")) {
        return 1;
    }
    return X_pop_9(ScriptFile, label, -1, 0, 0, fn);
}

int __cdecl X_pop_2(const char* filename, const char* label, fp_none fn)
{
    if (autosim_ui_active()) {
        const AutoUiAction action = autosim_xpop_policy(label, fn != 0);
        if (action != AUTO_UI_NATIVE) {
            telemetry_record_ui_notice("x_pop_2", label,
                action == AUTO_UI_NOTICE ? "suppressed_notice" : "declined_optional_human_ui");
            return 0;
        }
    }
    return X_pop_9(filename, label, -1, 0, 0, fn);
}

int __cdecl X_pop_6(const char* label, int a2, fp_none fn)
{
    return X_pop_9(ScriptFile, label, -1, 0, a2, fn);
}

int __cdecl X_pops(const char* label, Sprite* a2, fp_none fn)
{
    return X_pops_18(ScriptFile, label, -1, 0, 0, a2, 1, 1, fn);
}

int __cdecl X_pops_11(const char* label, int a2, Sprite* a3, fp_none fn)
{
    return X_pops_18(ScriptFile, label, -1, 0, a2, a3, 1, 1, fn);
}

int __cdecl X_dialog(const char* label, int faction_id)
{
    return X_pops_18(ScriptFile, label, -1, 0, POPUP_UNK_100000,
        FactionPortraits[faction_id], 1, 1, pop_wait);
}

int __cdecl X_dialog_alt(const char* filename, const char* label, int faction_id)
{
    return X_pops_18(filename, label, -1, 0, POPUP_UNK_100000,
        FactionPortraits[faction_id], 1, 1, pop_wait);
}



