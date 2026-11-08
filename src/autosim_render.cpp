#include "main.h"
#include "gui_dialog.h"
#include "patch.h"
#include "autosim_render.h"

// Windows x86 only. Preserve complete non-PC-relative native prologues in
// executable gateways; do not install on unknown/partly patched binaries.
// The original functions still execute for ordinary user/game presentation.
namespace {
struct RenderSite {
    uintptr_t address;
    const byte* expected;
    size_t stolen;
    void* detour;
    void** trampoline;
};
static Fdraw_tile original_tile = nullptr;
static Fdraw_tile_fixup original_fixup = nullptr;
static Fdraw_tile_fixup_2 original_fixup2 = nullptr;
static Fdraw_tiles original_tiles = nullptr;
static Fdraw_map original_map = nullptr;
static FMapWin_draw_map original_mapwin_map = nullptr;
static FMapWin_draw_tile original_mapwin_tile = nullptr;

static void __cdecl null_tile(int x, int y, int mode) {
    if (!autosim_null_render_active()) original_tile(x, y, mode);
}
static void __cdecl null_fixup(int x,int y,int a,int b) {
    if (!autosim_null_render_active()) original_fixup(x,y,a,b);
}
static void __cdecl null_fixup2(int x,int y,int a,int b,int c,int d) {
    if (!autosim_null_render_active()) original_fixup2(x,y,a,b,c,d);
}
static void __cdecl null_tiles(int x,int y,int mode) {
    if (!autosim_null_render_active()) original_tiles(x,y,mode);
}
static void __cdecl null_map(int a) {
    if (!autosim_null_render_active()) original_map(a);
}
static void __thiscall null_mapwin_map(MapWindow* w,int a) {
    if (!autosim_null_render_active()) original_mapwin_map(w,a);
}
static void __thiscall null_mapwin_tile(MapWindow* w,int x,int y,int a) {
    if (!autosim_null_render_active()) original_mapwin_tile(w,x,y,a);
}
} // namespace

bool autosim_install_null_render_hooks() {
#if defined(__GNUC__) && defined(__i386__)
    static const byte e_tile[] = {0x55,0x8B,0xEC,0x53,0x8B,0x5D,0x0C};
    static const byte e_fixup[] = {0x55,0x8B,0xEC,0xA0,0x94,0x64,0x9A,0x00};
    static const byte e_tiles[] = {0x55,0x8B,0xEC,0x53,0x8B,0x5D,0x0C};
    static const byte e_map[] = {0x55,0x8B,0xEC,0x56,0x57};
    static const byte e_mapwin[] = {0x55,0x8B,0xEC,0x83,0xEC,0x60};
    static const byte e_mapwin_tile[] = {0x55,0x8B,0xEC,0x8B,0x91,0xA8,0xDD,0x01,0x00};
    RenderSite sites[] = {
        {0x46AF40,e_tile,sizeof(e_tile),(void*)null_tile,(void**)&original_tile},
        {0x46AF90,e_fixup,sizeof(e_fixup),(void*)null_fixup,(void**)&original_fixup},
        {0x46B040,e_fixup,sizeof(e_fixup),(void*)null_fixup2,(void**)&original_fixup2},
        {0x46B140,e_tiles,sizeof(e_tiles),(void*)null_tiles,(void**)&original_tiles},
        {0x46B190,e_map,sizeof(e_map),(void*)null_map,(void**)&original_map},
        {0x46A550,e_mapwin,sizeof(e_mapwin),(void*)null_mapwin_map,(void**)&original_mapwin_map},
        {0x46A4A0,e_mapwin_tile,sizeof(e_mapwin_tile),(void*)null_mapwin_tile,(void**)&original_mapwin_tile},
    };
    for (const auto& s : sites) {
        if (memcmp((const void*)s.address,s.expected,s.stolen) != 0) {
            OutputDebugStringA("thinkerNeca: Null Rendering preflight failed; no hooks installed\n");
            return false;
        }
    }
    byte* gateways = (byte*)VirtualAlloc(NULL, 4096, MEM_COMMIT|MEM_RESERVE,
        PAGE_EXECUTE_READWRITE);
    if (!gateways) return false;
    for (size_t i=0; i<sizeof(sites)/sizeof(*sites); ++i) {
        const RenderSite& s=sites[i];
        byte* entry=gateways + i*32;
        memcpy(entry,(const void*)s.address,s.stolen);
        entry[s.stolen]=0xE9;
        const int32_t back=(int32_t)(s.address+s.stolen)
            -(int32_t)(uintptr_t)(entry+s.stolen+5);
        memcpy(entry+s.stolen+1,&back,4);
        *s.trampoline=(void*)entry;
    }
    FlushInstructionCache(GetCurrentProcess(),gateways,4096);
    for (const auto& s : sites) {
        // All seven entrypoints are 16-byte aligned. The first instruction
        // boundaries and preimages were disassembled from the supported EXE.
        write_jump((int32_t)s.address,(int32_t)(uintptr_t)s.detour);
    }
    FlushInstructionCache(GetCurrentProcess(),NULL,0);
    return true;
#else
    return false;
#endif
}
