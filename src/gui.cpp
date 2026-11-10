
#include "gui.h"
#include "empire_report.h"
#include "autosim_control.h"

const void* MainWinHandle = &MapWin->oMainWin; // 0x939444
bool minor_native_geology_underlay_installed = false;

char label_pop_size[StrBufLen] = "Pop: %d / %d / %d / %d";
char label_pop_boom[StrBufLen] = "Population Boom";
char label_nerve_staple[StrBufLen] = "Nerve Staple: %d turns";
char label_captured_base[StrBufLen] = "Captured Base: %d turns";
char label_stockpile_energy[StrBufLen] = "Stockpile: %d per turn";
char label_sat_nutrient[StrBufLen] = "N +%d";
char label_sat_mineral[StrBufLen] = "M +%d";
char label_sat_energy[StrBufLen] = "E +%d";
char label_eco_damage[StrBufLen] = "Eco-Damage: %d%%";
char label_base_surplus[StrBufLen] = "Surplus: %d / %d / %d";
char label_unit_reactor[4][StrBufLen] = {};

std::string video_player_path = "";
std::string video_player_args = "";
std::string video_player_extn = "";

struct ConsoleState {
    const int ScrollMin = 1;
    const int ScrollMax = 20;
    const int ListScrollDelta = 1;
    POINT ScreenSize;
    bool MouseOverTileInfo = true;
    bool Scrolling = false;
    bool RightButtonDown = false;
    bool ScrollDragging = false;
    double ScrollOffsetX = 0.0;
    double ScrollOffsetY = 0.0;
    POINT ScrollDragPos = {0, 0};
} CState;

/*
The following lists contain definitions copied directly from PRACX (e.g. functions with _F suffix).
These are mostly provided for reference and using them should be avoided instead of engine.h definitions.
*/

typedef int(__stdcall *START_F)(HINSTANCE, HINSTANCE, LPSTR, int);
typedef int(__thiscall *CCANVAS_CREATE_F)(Buffer* This);
typedef int(__stdcall *WNDPROC_F)(HWND, int, WPARAM, LPARAM);
typedef int(__thiscall *CMAIN_ZOOMPROCESSING_F)(Console* This);
typedef int(__thiscall *PROC_ZOOM_KEY_F)(Console* This, int iZoomType, int iZero);
typedef int(__thiscall *CMAIN_TILETOPT_F)(Console* This, int iTileX, int iTileY, long* piX, long* piY);
typedef int(__thiscall *CMAIN_MOVEMAP_F)(Console* This, int iXPos, int iYPos, int a4);
typedef int(__thiscall *CMAIN_REDRAWMAP_F)(Console* This, int a2);
typedef int(__thiscall *CMAIN_DRAWMAP_F)(Console* This, int iOwner, int fUnitsOnly);
typedef int(__thiscall *CMAIN_PTTOTILE_F)(Console* This, POINT p, long* piTileX, long* piTileY);
typedef int(__cdecl *PAINTHANDLER_F)(RECT *prRect, int a2);
typedef int(__cdecl *PAINTMAIN_F)(RECT *pRect);
typedef int(__thiscall *CSPRITE_FROMCANVASRECTTRANS_F)(Sprite* This, Buffer *poCanvas,
    int iTransparentIndex, int iLeft, int iTop, int iWidth, int iHeight, int a8);
typedef int(__thiscall *CCANVAS_DESTROY4_F)(Buffer* This);
typedef int (__thiscall *CSPRITE_STRETCHCOPYTOCANVAS_F)
    (Sprite* This, Buffer *poCanvasDest, int cTransparentIndex, int iLeft, int iTop);
typedef int(__thiscall *CSPRITE_STRETCHCOPYTOCANVAS1_F)
    (Sprite* This, Buffer *poCanvasDest, int cTransparentIndex, int iLeft, int iTop, int iDestScale, int iSourceScale);
typedef int (__thiscall *CSPRITE_STRETCHDRAWTOCANVAS2_F)
    (Sprite* This, Buffer *poCanvas, int a1, int a2, int a3, int a4, int a7, int a8);
typedef int(__thiscall *CWINBASE_ISVISIBLE_F)(Win* This);
typedef int(__thiscall *CTIMER_STARTTIMER_F)(Time* This, int a2, int a3, int iDelay, int iElapse, int uResolution);
typedef int(__thiscall *CTIMER_STOPTIMER_F)(Time* This);
typedef int(__thiscall *DRAWCITYMAP_F)(Win* This, int a2);
typedef int(__cdecl *GETFOODCOUNT_F)(int iForFaction, int a2, int iTileX, int iTileY, bool fWithFarm);
typedef int(__cdecl *GETPRODCOUNT_F)(int iForFaction, int a2, int iTileX, int iTileY, bool fWithMine);
typedef int(__cdecl *GETENERGYCOUNT_F)(int iForFaction, int a2, int iTileX, int iTileY, bool fWithSolar);
typedef int(__thiscall *IMAGEFROMCANVAS_F)(CImage* This, Buffer *poCanvasSource, int iLeft, int iTop, int iWidth, int iHeight, int a7);
typedef int(__cdecl *GETELEVATION_F)(int iTileX, int iTileY);
typedef int(__thiscall *CIMAGE_COPYTOCANVAS2_F)(CImage* This, Buffer *poCanvasDest, int x, int y, int a5, int a6, int a7);
typedef int(__thiscall *CMAINMENU_ADDSUBMENU_F)(Menu* This, int iMenuID, int iMenuItemID, char *lpString);
typedef int(__thiscall *CMAINMENU_ADDBASEMENU_F)(Menu* This, int iMenuItemID, const char *pszCaption, int a4);
typedef int(__thiscall *CMAINMENU_ADDSEPARATOR_F)(Menu* This, int iMenuItemID, int iSubMenuItemID);
typedef int(__thiscall *CMAINMENU_UPDATEVISIBLE_F)(Menu* This, int a2);
typedef int(__thiscall *CMAINMENU_RENAMEMENUITEM_F)(Menu* This, int iMenuItemID, int iSubMenuItemID, const char *pszCaption);
typedef long(__thiscall *CMAP_GETCORNERYOFFSET_F)(Console* This, int iTileX, int iTileY, int iCorner);

START_F                        pfncWinMain =                    (START_F                       )0x45F950;
HDC*                           phDC =                           (HDC*                          )0x9B7B2C;
HWND*                          phWnd =                          (HWND*                         )0x9B7B28;
HPALETTE*                      phPallete =                      (HPALETTE*                     )0x9B8178;
HINSTANCE*                     phInstance =                     (HINSTANCE*                    )0x9B7B14;
WNDPROC_F                      pfncWinProc =                    (WNDPROC_F                     )0x5F0650;
CMAIN_ZOOMPROCESSING_F         pfncZoomProcessing =             (CMAIN_ZOOMPROCESSING_F        )0x462980;
Console*                       pMain =                          (Console*                      )0x9156B0;
int*                           piMaxTileX =                     (int*                          )0x949870;
int*                           piMaxTileY =                     (int*                          )0x949874;
PROC_ZOOM_KEY_F                pfncProcZoomKey =                (PROC_ZOOM_KEY_F               )0x5150D0;
CMAIN_TILETOPT_F               pfncTileToPoint =                (CMAIN_TILETOPT_F              )0x462F00;
RECT*                          prVisibleTiles =                 (RECT*                         )0x7D3C28;
int*                           piMapFlags =                     (int*                          )0x94988C;
CMAIN_MOVEMAP_F                pfncMoveMap =                    (CMAIN_MOVEMAP_F               )0x46B1F0;
CMAIN_REDRAWMAP_F              pfncRedrawMap =                  (CMAIN_REDRAWMAP_F             )0x46A550;
CMAIN_DRAWMAP_F                pfncDrawMap =                    (CMAIN_DRAWMAP_F               )0x469CA0;
CMAIN_PTTOTILE_F               pfncPtToTile =                   (CMAIN_PTTOTILE_F              )0x463040;
StatusWindow*                  pInfoWin =                       (StatusWindow*                 )0x8C5568;
FStatusWin                     pfncDrawTileInfo =               (FStatusWin                    )0x4B8890;
Console**                      ppMain =                         (Console**                     )0x7D3C3C;
PAINTHANDLER_F                 pfncPaintHandler =               (PAINTHANDLER_F                )0x5F7320;
PAINTMAIN_F                    pfncPaintMain =                  (PAINTMAIN_F                   )0x5EFD20;
CSPRITE_FROMCANVASRECTTRANS_F  pfncSpriteFromCanvasRectTrans =  (CSPRITE_FROMCANVASRECTTRANS_F )0x5E39A0;
Buffer*                        poLoadingCanvas =                (Buffer*                       )0x798668;
CCANVAS_DESTROY4_F             pfncCanvasDestroy4 =             (CCANVAS_DESTROY4_F            )0x5D7470;
CSPRITE_STRETCHCOPYTOCANVAS_F  pfncSpriteStretchCopyToCanvas =  (CSPRITE_STRETCHCOPYTOCANVAS_F )0x5E4B9A;
CSPRITE_STRETCHCOPYTOCANVAS1_F pfncSpriteStretchCopyToCanvas1 = (CSPRITE_STRETCHCOPYTOCANVAS1_F)0x5E4B4A;
CSPRITE_STRETCHDRAWTOCANVAS2_F pfncSpriteStretchDrawToCanvas2 = (CSPRITE_STRETCHDRAWTOCANVAS2_F)0x5E3E00;
Sprite*                        pSprResourceIcons =              (Sprite*                       )0x7A72A0;
Win*                           pCityWindow =                    (Win*                          )0x6A7628;
Win*                           pAnotherWindow =                 (Win*                          )0x8A6270;
Win*                           pAnotherWindow2 =                (Win*                          )0x8C6E68;
int*                           pfGameNotStarted =               (int*                          )0x68F21C;
CWINBASE_ISVISIBLE_F           pfncWinIsVisible =               (CWINBASE_ISVISIBLE_F          )0x5F7E90;
CTIMER_STARTTIMER_F            pfncStartTimer =                 (CTIMER_STARTTIMER_F           )0x616350;
CTIMER_STOPTIMER_F             pfncStopTimer =                  (CTIMER_STOPTIMER_F            )0x616780;
DRAWCITYMAP_F                  pfncDrawCityMap =                (DRAWCITYMAP_F                 )0x40F0F0;
int*                           piZoomNum =                      (int*                          )0x691E6C;
int*                           piZoomDenom =                    (int*                          )0x691E70;
int*                           piSourceScale =                  (int*                          )0x696D1C;
int*                           piDestScale =                    (int*                          )0x696D18;
int*                           piResourceExtra =                (int*                          )0x90E998;
int*                           piTilesPerRow =                  (int*                          )0x68FAF0;
CTile**                        paTiles =                        (CTile**                       )0x94A30C;
GETFOODCOUNT_F                 pfncGetFoodCount =               (GETFOODCOUNT_F                )0x4E6E50;
GETPRODCOUNT_F                 pfncGetProdCount =               (GETPRODCOUNT_F                )0x4E7310;
GETENERGYCOUNT_F               pfncGetEnergyCount =             (GETENERGYCOUNT_F              )0x4E7750;
IMAGEFROMCANVAS_F              pfncImageFromCanvas =            (IMAGEFROMCANVAS_F             )0x619710;
GETELEVATION_F                 pfncGetElevation =               (GETELEVATION_F                )0x5919C0;
CIMAGE_COPYTOCANVAS2_F         pfncImageCopytoCanvas2 =         (CIMAGE_COPYTOCANVAS2_F        )0x6233C0;
CMAINMENU_ADDSUBMENU_F         pfncMainMenuAddSubMenu =         (CMAINMENU_ADDSUBMENU_F        )0x5FB100;
CMAINMENU_ADDBASEMENU_F        pfncMainMenuAddBaseMenu =        (CMAINMENU_ADDBASEMENU_F       )0x5FAEF0;
CMAINMENU_ADDSEPARATOR_F       pfncMainMenuAddSeparator =       (CMAINMENU_ADDSEPARATOR_F      )0x5FB160;
CMAINMENU_UPDATEVISIBLE_F      pfncMainMenuUpdateVisible =      (CMAINMENU_UPDATEVISIBLE_F     )0x460DD0;
CMAINMENU_RENAMEMENUITEM_F     pfncMainMenuRenameMenuItem =     (CMAINMENU_RENAMEMENUITEM_F    )0x5FB700;
CMAP_GETCORNERYOFFSET_F        pfncMapGetCornerYOffset =        (CMAP_GETCORNERYOFFSET_F       )0x46FE70;

// End of PRACX definitions


bool shift_key_down() {
    return GetAsyncKeyState(VK_SHIFT) < 0;
}

bool ctrl_key_down() {
    return GetAsyncKeyState(VK_CONTROL) < 0;
}

bool alt_key_down() {
    return GetAsyncKeyState(VK_MENU) < 0;
}

bool win_has_focus() {
    return GetFocus() == *phWnd;
}

int __thiscall Win_is_visible(Win* This) {
    return (This->winstate & WIN_ST_VISIBLE)
        && (!This->parent || Win_is_visible(This->parent));
}

/*
Returns GW_World only when the world map is visible and has focus
and other large modal windows are not blocking it.
Other modal windows with the exception of BaseWin are already
covered by checking Win_get_key_window condition.
*/
static GameWinState current_window() {
    if (!*GameHalted) {
        void* state = (void*)Win_get_key_window();
        if (state == MainWinHandle) {
            return Win_is_visible(BaseWin) ? GW_Base : GW_World;
        } else if (state == DesignWin) {
            return GW_Design;
        }
    }
    return GW_None;
}

#ifndef BUILD_REL
static void minor_inspector_trace_hover(int x, int y);
#endif

void mouse_over_tile(POINT* p) {
    static POINT ptLastTile = {0, 0};
    POINT ptTile;
    if (CState.MouseOverTileInfo
    && !MapWin->InVehMode
    && p->x >= 0 && p->x < CState.ScreenSize.x
    && p->y >= 0 && p->y < (CState.ScreenSize.y - ConsoleHeight)
    && MapWin_pixel_to_tile(MapWin, p->x, p->y, &ptTile.x, &ptTile.y) == 0
    && memcmp(&ptTile, &ptLastTile, sizeof(POINT)) != 0) {
        StatusWin->tile_x = ptTile.x;
        StatusWin->tile_y = ptTile.y;
#ifndef BUILD_REL
        minor_inspector_trace_hover(ptTile.x, ptTile.y);
#endif
        StatusWin_on_redraw(StatusWin);
        memcpy(&ptLastTile, &ptTile, sizeof(POINT));
    }
}

ULONGLONG get_ms_count() {
    static LONGLONG llFrequency = 0;
    static ULONGLONG ullLast = 0;
    static ULONGLONG ullHigh = 0;
    ULONGLONG ullRet;
    if (llFrequency == 0 && !QueryPerformanceFrequency((LARGE_INTEGER*)&llFrequency)) {
        llFrequency = -1LL;
    }
    if (llFrequency > 0) {
        QueryPerformanceCounter((LARGE_INTEGER*)&ullRet);
        ullRet *= 1000;
        ullRet /= (ULONGLONG)llFrequency;
    } else if (llFrequency < 0) {
        ullRet = GetTickCount();
        if (ullRet < ullLast) {
            ullHigh += 0x100000000ULL;
        }
        ullLast = ullRet;
        ullRet += ullHigh;
    }
    return ullRet;
}

bool do_scroll(double x, double y) {
    bool fScrolled = false;
    int mx = *MapAreaX;
    int my = *MapAreaY;
    int i;
    int d;
    if (x && MapWin->TileDistEvenX + MapWin->TileDistOddX < mx) {
        if (x < 0 && (!map_is_flat() || MapWin->MapTileLeft > 0)) {
            i = (int)CState.ScrollOffsetX;
            CState.ScrollOffsetX -= x;
            fScrolled = fScrolled || (i != (int)CState.ScrollOffsetX);
            while (CState.ScrollOffsetX >= MapWin->PixelsPerTileX) {
                CState.ScrollOffsetX -= MapWin->PixelsPerTileX;
                MapWin->TileX -= 2;
                if (MapWin->TileX < 0) {
                    if (map_is_flat()) {
                        MapWin->TileX = 0;
                        MapWin->TileY &= ~1;
                        CState.ScrollOffsetX = 0;
                    } else {
                        MapWin->TileX += mx;
                    }
                }
            }
        } else if (x < 0 && map_is_flat()) {
            fScrolled = true;
            CState.ScrollOffsetX = 0;
        }
        if (x > 0 && (!map_is_flat()
        || MapWin->MapTileLeft + MapWin->TileDistEvenX + MapWin->TileDistOddX <= mx)) {
            i = (int)CState.ScrollOffsetX;
            CState.ScrollOffsetX -= x;
            fScrolled = fScrolled || (i != (int)CState.ScrollOffsetX);
            while (CState.ScrollOffsetX <= -MapWin->PixelsPerTileX) {
                CState.ScrollOffsetX += MapWin->PixelsPerTileX;
                MapWin->TileX += 2;
                if (MapWin->TileX > mx) {
                    if (map_is_flat()) {
                        MapWin->TileX = mx;
                        MapWin->TileY &= ~1;
                        CState.ScrollOffsetX = 0;
                    } else {
                        MapWin->TileX -= mx;
                    }
                }
            }
        } else if (x > 0 && map_is_flat()) {
            fScrolled = true;
            CState.ScrollOffsetX = 0;
        }
    }
    if (y && MapWin->TileDistEvenY + MapWin->TileDistOddY < my) {
        int MinTileY = MapWin->TileDistOddY - 2;
        int MaxTileY = my + 4 - MapWin->TileDistOddY;
        while (MapWin->TileY < MinTileY) {
            MapWin->TileY += 2;
        }
        while (MapWin->TileY > MaxTileY) {
            MapWin->TileY -= 2;
        }
        d = (MapWin->TileY - MinTileY) * MapWin->PixelsPerHalfTileY - (int)CState.ScrollOffsetY;
        if (y < 0 && d > 0 ) {
            if (y < -d) { y = -d; }
            i = (int)CState.ScrollOffsetY;
            CState.ScrollOffsetY -= y;
            fScrolled = fScrolled || (i != (int)CState.ScrollOffsetY);
            while (CState.ScrollOffsetY >= MapWin->PixelsPerTileY && MapWin->TileY - 2 >= MinTileY) {
                CState.ScrollOffsetY -= MapWin->PixelsPerTileY;
                MapWin->TileY -= 2;
            }
        }
        d = (MaxTileY - MapWin->TileY + 1) * MapWin->PixelsPerHalfTileY + (int)CState.ScrollOffsetY;
        if (y > 0 && d > 0) {
            if (y > d) { y = d; }
            i = (int)CState.ScrollOffsetY;
            CState.ScrollOffsetY -= y;
            fScrolled = fScrolled || (i != (int)CState.ScrollOffsetY);
            while (CState.ScrollOffsetY <= -MapWin->PixelsPerTileY && MapWin->TileY + 2 <= MaxTileY) {
                CState.ScrollOffsetY += MapWin->PixelsPerTileY;
                MapWin->TileY += 2;
            }
        }
    }
    if (fScrolled) {
        MapWin_draw_map(MapWin, 0);
        Win_update_screen(NULL, 0);
        Win_flip(NULL);
        ValidateRect(*phWnd, NULL);
    }
    return fScrolled;
}

void check_scroll() {
    POINT p;
    if (CState.Scrolling || (!GetCursorPos(&p) && !(CState.ScrollDragging && CState.RightButtonDown))) {
        return;
    }
    CState.Scrolling = true;
    int w = CState.ScreenSize.x;
    int h = CState.ScreenSize.y;
    static ULONGLONG ullDeactiveTimer = 0;
    ULONGLONG ullOldTickCount = 0;
    ULONGLONG ullNewTickCount = 0;
    BOOL fScrolled;
    BOOL fScrolledAtAll = false;
    BOOL fLeftButtonDown = (GetAsyncKeyState(VK_LBUTTON) < 0);
    int ScrollArea = conf.scroll_area * CState.ScreenSize.x / 1024;

    if (CState.RightButtonDown && GetAsyncKeyState(VK_RBUTTON) < 0) {
        if (hypot((double)(p.x-CState.ScrollDragPos.x), (double)(p.y-CState.ScrollDragPos.y)) > 2.5) {
            CState.ScrollDragging = true;
            SetCursor(LoadCursor(0, IDC_HAND));
        }
    }
    CState.ScrollOffsetX = MapWin->MapPixelLeft;
    CState.ScrollOffsetY = MapWin->MapPixelTop;
    ullNewTickCount = get_ms_count();
    ullOldTickCount = ullNewTickCount;
//    debug("scroll_check %d %d %d\n", CState.Scrolling, (int)CState.ScrollDragPos.x, (int)CState.ScrollDragPos.y);
    do {
        double dTPS = -1;
        double dx = 0;
        double dy = 0;
        fScrolled = false;
        if (CState.ScrollDragging && CState.RightButtonDown) {
            fScrolled = true;
            dx = CState.ScrollDragPos.x - p.x;
            dy = CState.ScrollDragPos.y - p.y;
            memcpy(&CState.ScrollDragPos, &p, sizeof(POINT));

        } else if (ullNewTickCount - ullDeactiveTimer > 100 && !CState.ScrollDragging) {
            double dMin = (double)CState.ScrollMin;
            double dMax = (double)CState.ScrollMax;
            double dArea = (double)ScrollArea;
            if (p.x <= ScrollArea && p.x >= 0) {
                fScrolled = true;
                dTPS = dMin + (dArea - (double)p.x) / dArea * (dMax - dMin);
                dx = (double)(ullNewTickCount - ullOldTickCount) * dTPS * (double)MapWin->PixelsPerTileX / -1000.0;

            } else if ((w - p.x) <= ScrollArea && w >= p.x) {
                fScrolled = true;
                dTPS = dMin + (dArea - (double)(w - p.x)) / dArea * (dMax - dMin);
                dx = (double)(ullNewTickCount - ullOldTickCount) * dTPS * (double)MapWin->PixelsPerTileX / 1000.0;
            }
            if (p.y <= ScrollArea && p.y >= 0) {
                fScrolled = true;
                dTPS = dMin + (dArea - (double)p.y) / dArea * (dMax - dMin);
                dy = (double)(ullNewTickCount - ullOldTickCount) * dTPS * (double)MapWin->PixelsPerTileY / -1000.0;

            } else if (h - p.y <= ScrollArea && h >= p.y &&
            // These extra conditions will stop movement when the mouse is over the bottom middle console.
            (p.x <= (CState.ScreenSize.x - ConsoleWidth) / 2 ||
             p.x >= (CState.ScreenSize.x - ConsoleWidth) / 2 + ConsoleWidth ||
             h - p.y <= 8 * CState.ScreenSize.y / 768)) {
                fScrolled = true;
                dTPS = dMin + (dArea - (double)(h - p.y)) / dArea * (dMax - dMin);
                dy = (double)(ullNewTickCount - ullOldTickCount) * dTPS * (double)MapWin->PixelsPerTileY / 1000.0;
            }
        }
        if (fScrolled) {
            ullOldTickCount = ullNewTickCount;
            if (do_scroll(dx, dy)) {
                fScrolledAtAll = true;
            } else {
                Sleep(0);
            }
            if (DEBUG) {
                Sleep(5);
            }
            ullNewTickCount = get_ms_count();
            if (CState.RightButtonDown) {
                CState.RightButtonDown = (GetAsyncKeyState(VK_RBUTTON) < 0);
            }
            debug("scroll_move  x=%d y=%d scx=%.4f scy=%.4f dx=%.4f dy=%.4f dTPS=%.4f\n",
                (int)p.x, (int)p.y, CState.ScrollOffsetX, CState.ScrollOffsetY, dx, dy, dTPS);
        }

    } while (fScrolled && (GetCursorPos(&p) || (CState.ScrollDragging && CState.RightButtonDown)));

    if (fScrolledAtAll) {
        MapWin->DrawOnlyCursor = 1;
        MapWin_set_center(MapWin, MapWin->TileX, MapWin->TileY, 1);
        MapWin->DrawOnlyCursor = 0;
        for (int i = 1; i < 8; i++) {
            if (MapWinPtr[i] && MapWinPtr[i]->DrawToggleA
            && (!fLeftButtonDown || MapWinPtr[i]->field_1DD80)
            && MapWinPtr[i]->TileDistOddX + MapWinPtr[i]->TileDistEvenX < *MapAreaX) {
                MapWin_set_center(MapWinPtr[i], MapWin->TileX, MapWin->TileY, 1);
            }
        }
        if (CState.ScrollDragging) {
            ullDeactiveTimer = ullNewTickCount;
        }
    }
    mouse_over_tile(&p);
    CState.Scrolling = false;
    flushlog();
}

#ifndef BUILD_REL
static void minor_graphics_trace(const char* message, int x, int y, int extra);
static void draw_minor_terrain_map(Console* win);
static void draw_minor_terrain_tile(Console* win, int x, int y);
#endif

int __thiscall mod_gen_map(Console* This, int faction_id, int units_only) {
#ifndef BUILD_REL
    static bool first_map_draw = true;
    static int last_zoom = -99999;
    static int last_partial = -1;
    static int zoom_reports = 0;
    if (first_map_draw) {
        first_map_draw = false;
        minor_graphics_trace("MapWin_gen_map call routed via Thinker", 0, 0, units_only);
    }
    // Zoom can redraw the map through both full and units-only passes.
    // Record transitions (not individual frames) to diagnose lost overlays.
    if (This == MapWin && zoom_reports < 40
        && (This->ZoomFactor != last_zoom || units_only != last_partial)) {
        last_zoom = This->ZoomFactor;
        last_partial = units_only;
        zoom_reports++;
        minor_graphics_trace("map redraw zoom / tile-pixels / units_only",
            This->ZoomFactor, This->PixelsPerTileX, units_only);
    }
#endif
    // With smooth scrolling disabled, this must behave exactly like the
    // original MapWin_gen_map call, apart from the development overlay.
    if (!conf.smooth_scrolling) {
        MapWin_gen_map(This, faction_id, units_only);
#ifndef BUILD_REL
        if (This == MapWin) {
            // Redraw the minor layer after both full and partial passes.
            // Temporary until minor terrain has a dedicated engine layer.
            if (conf.minor_render_mode == 0
                && (!conf.minor_native_jungle || !conf.minor_native_uranium
                    || !conf.minor_native_titanium)) {
                draw_minor_terrain_map(This);
            }
        }
#endif
        return 0;
    }
    if (This == MapWin) {
        // Save these values to restore them later
        int MapPixelLeft = This->MapPixelLeft;
        int MapPixelTop = This->MapPixelTop;
        int MapTileLeft = This->MapTileLeft;
        int MapTileTop = This->MapTileTop;
        int TileDistOddX = This->TileDistOddX;
        int TileDistOddY = This->TileDistOddY;
        int TileDistEvenX = This->TileDistEvenX;
        int TileDistEvenY = This->TileDistEvenY;
        // These are just aliased to save typing and are not modified
        int mx = *MapAreaX;
        int my = *MapAreaY;

        if (TileDistOddX + TileDistEvenX < mx && !map_is_flat()) {
            if (MapPixelLeft > 0) {
                This->MapPixelLeft -= This->PixelsPerTileX;
                This->MapTileLeft -= 2;
                This->TileDistEvenX++;
                This->TileDistOddX++;
                if (This->MapTileLeft < 0)
                    This->MapTileLeft += mx;
            } else if (MapPixelLeft < 0 ) {
                This->TileDistEvenX++;
                This->TileDistOddX++;
            }
        }
        if (TileDistOddY + TileDistEvenY < my) {
            if (MapPixelTop > 0) {
                This->MapPixelTop -= This->PixelsPerTileY;
                This->MapTileTop -= 2;
                This->TileDistEvenY++;
                This->TileDistOddY++;
            } else if (MapPixelTop < 0) {
                This->TileDistEvenY++;
                This->TileDistOddY++;
            }
        }
        MapWin_gen_map(This, faction_id, units_only);
#ifndef BUILD_REL
        // Reapply the custom layer after a partial redraw as well.
        if (conf.minor_render_mode == 0
            && (!conf.minor_native_jungle || !conf.minor_native_uranium
                || !conf.minor_native_titanium)) {
            draw_minor_terrain_map(This);
        }
#endif
        // Restore This's original values
        This->MapPixelLeft = MapPixelLeft;
        This->MapPixelTop = MapPixelTop;
        This->MapTileLeft = MapTileLeft;
        This->MapTileTop = MapTileTop;
        This->TileDistOddX = TileDistOddX;
        This->TileDistOddY = TileDistOddY;
        This->TileDistEvenX = TileDistEvenX;
        This->TileDistEvenY = TileDistEvenY;
    } else {
        MapWin_gen_map(This, faction_id, units_only);
    }
    return 0;
}

int __thiscall mod_calc_dim(Console* This) {
    static POINT ptOldTile = {-1, -1};
    POINT ptOldCenter;
    POINT ptNewCenter;
    POINT ptNewTile;
    POINT ptScale;
    int zoom;
    int dx, dy;
    bool fx, fy;
    if (This == MapWin) {
        zoom = This->LastZoomFactor;
        ptNewTile.x = This->TileX;
        ptNewTile.y = This->TileY;
        fx = (ptNewTile.x == ptOldTile.x);
        fy = (ptNewTile.y == ptOldTile.y);
        memcpy(&ptOldTile, &ptNewTile, sizeof(POINT));
        MapWin_tile_to_pixel(This, ptNewTile.x, ptNewTile.y, &ptOldCenter.x, &ptOldCenter.y);
        MapWin_calculate_dimensions(This);

        if (CState.Scrolling) {
            This->MapPixelLeft = (int)CState.ScrollOffsetX;
            This->MapPixelTop = (int)CState.ScrollOffsetY;
        } else if (zoom != -9999) {
            ptScale.x = This->PixelsPerTileX;
            ptScale.y = This->PixelsPerTileY;
            MapWin_tile_to_pixel(This, ptNewTile.x, ptNewTile.y, &ptNewCenter.x, &ptNewCenter.y);
            dx = ptOldCenter.x - ptNewCenter.x;
            dy = ptOldCenter.y - ptNewCenter.y;
            if (!This->MapPixelLeft && fx && dx > -ptScale.x * 2 && dx < ptScale.x * 2) {
                This->MapPixelLeft = dx;
            }
            if (!This->MapPixelTop && fy && dy > -ptScale.y * 2 && dy < ptScale.y * 2
            && (dy + *ScreenHeight) / ptScale.y < (*MapAreaY - This->MapTileTop) / 2) {
                This->MapPixelTop = dy;
            }
        }
    } else {
        MapWin_calculate_dimensions(This);
    }
    return 0;
}

int __cdecl mod_blink_timer() {
    if (!*GameHalted && !VehBattleState[1]) {
        if (Win_is_visible(BaseWin)) {
            return TutWin_draw_arrow(TutWin);
        }
        if (Win_is_visible(SocialWin)) {
            return TutWin_draw_arrow(TutWin);
        }
        PlanWin_blink(PlanWin);
        StringBox_clip_ids(&MainInfc->stringBox, 150);

        if ((!MapWin->field_23BE8 && (!*MultiplayerActive || !(*GameState & STATE_UNK_2))) || *ControlTurnA) {
            MapWin->field_23BFC = 0;
            return 0;
        }
        if (MapWin->field_23BE4 || (*MultiplayerActive && *GameState & STATE_UNK_2)) {
            MapWin->field_23BF8 = !MapWin->field_23BF8;
            draw_cursor();
            bool mouse_btn_down = GetAsyncKeyState(VK_LBUTTON) < 0;

            if ((int)*phInstance == GetWindowLongA(GetFocus(), GWL_HINSTANCE) && !Win_is_visible(TutWin)) {
                if (MapWin->field_23D80 != -1
                && MapWin->field_23D84 != -1
                && MapWin->field_23D88 != -1
                && MapWin->field_23D8C != -1) {
                    if (*GamePreferences & PREF_BSC_MOUSE_EDGE_SCROLL_VIEW || mouse_btn_down || *WinModalState != 0) {
                        CState.ScreenSize.x = *ScreenWidth;
                        CState.ScreenSize.y = *ScreenHeight;
                        check_scroll();
                    }
                }
            }
        }
    }
    return 0;
}

LRESULT WINAPI ModWinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    const bool debug_cmd = DEBUG && !*GameHalted && msg == WM_CHAR;
    const bool is_editor = !*GameHalted
        && *GameState & STATE_SCENARIO_EDITOR && *GameState & STATE_OMNISCIENT_VIEW;
    static int delta_accum = 0;
    POINT p;
    MAP* sq;

    // Dispatch only our registered, one-shot production request. This
    // message is queued before BaseWin_zoom enters its modal loop, so the
    // picker opens *over* the colony, not after the colony is closed.
    if (msg == empire_report_production_message()) {
        empire_report_dispatch_production((int)wParam, (uint32_t)lParam);
        return 0;
    }

    // Dedicated operator shortcut, separate from native F9/F10.
    // Never enter a modal dialog inside active AI processing. The turn driver
    // consumes its request at the next complete faction-cycle boundary.
    if (msg == WM_KEYDOWN && wParam == VK_F12 && !*GameHalted
    && !*MultiplayerActive && !*PbemActive
    && ctrl_key_down() && shift_key_down() && !alt_key_down()) {
        if (!(lParam & (1u << 30))) {
            if (autosim_control_active()) {
                autosim_request_control(AUTOSIM_CTL_MENU);
            } else if (current_window() == GW_World && win_has_focus()
            && !*WinModalState) {
                autosim_show_manual_setup();
            }
        }
        return 0;
    }

    // AUTO-002: controls only queue a request, never interrupt a live AI
    // action. The turn driver handles it after the current full turn.
    // Repeated keydown events must not re-arm the request.
    if ((msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN)
    && !*GameHalted && autosim_control_active()
    && (wParam == VK_F9 || wParam == VK_F10)) {
        // F10 often arrives via WM_SYSKEYDOWN; previously it could be
        // missed by the WM_KEYDOWN-only native window-procedure path.
        if (!(lParam & (1u << 30))) {
            autosim_request_control(wParam == VK_F10
                ? AUTOSIM_CTL_STOP : AUTOSIM_CTL_PAUSE);
            telemetry_record_ui_notice("autosim_input",
                wParam == VK_F10 ? "F10" : "F9", "winproc_latched");
        }
        return 0;
    }

    // Experimental, deliberately independent of production/upkeep processing.
    if (msg == WM_KEYDOWN && empire_report_visible()) {
        if (wParam == VK_ESCAPE || wParam == VK_OEM_3) {
            empire_report_hide();
            return 0;
        }
        if (wParam == VK_UP || wParam == VK_DOWN) {
            empire_report_scroll(wParam == VK_UP ? -1 : 1);
            return 0;
        }
        if (wParam == VK_PRIOR || wParam == VK_NEXT) {
            empire_report_scroll_page(wParam == VK_PRIOR ? -1 : 1);
            return 0;
        }
        if (wParam == VK_HOME || wParam == VK_END) {
            empire_report_scroll(wParam == VK_HOME ? -MaxBaseNum : MaxBaseNum);
            return 0;
        }
    }
    if (msg == WM_MOUSEWHEEL && empire_report_visible()) {
        empire_report_scroll_wheel(GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;
    }
    if (msg == WM_KEYDOWN && wParam == VK_OEM_3 && !*GameHalted
    && current_window() == GW_World && win_has_focus()
    && !ctrl_key_down() && !alt_key_down() && !shift_key_down()
    && !(lParam & (1u << 30))) {
        empire_report_show();
        return 0;
    }
    if (msg == WM_CHAR && (wParam == 0x60 || wParam == '~')
    && (empire_report_visible() || current_window() == GW_World)) {
        // Swallow the printable character generated by the shortcut.
        return 0;
    }

    if (msg == WM_ACTIVATEAPP && conf.auto_minimise && !conf.reduced_mode) {
        if (!LOWORD(wParam)) {
            //If window has just become inactive e.g. ALT+TAB
            //wParam is 0 if the window has become inactive.
            set_minimised(true);
        } else {
            set_minimised(false);
        }
        return WinProc(hwnd, msg, wParam, lParam);

    } else if (msg == WM_MOVIEOVER) {
        conf.playing_movie = false;
        set_video_mode(1);

    } else if (msg == WM_KEYDOWN && LOWORD(wParam) == VK_RETURN
    && GetAsyncKeyState(VK_MENU) < 0 && !conf.reduced_mode) {
        if (conf.video_mode == VM_Custom) {
            set_windowed(true);
        } else if (conf.video_mode == VM_Window) {
            set_windowed(false);
        }

    } else if (msg == WM_WINDOWED && !conf.reduced_mode) {
        RECT window_rect;
        WINDOWPLACEMENT wp;
        memset(&wp, 0, sizeof(wp));
        wp.length = sizeof(wp);
        GetWindowPlacement(*phWnd, &wp);
        wp.flags = 0;
        wp.showCmd = SW_SHOWNORMAL;
        SetRect(&window_rect, 0, 0, conf.window_width, conf.window_height);
        wp.rcNormalPosition = window_rect;
        SetWindowPlacement(*phWnd, &wp);
        SetWindowPos(*phWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
            SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_FRAMECHANGED);

    } else if (msg == WM_MOUSEWHEEL && win_has_focus()) {
        int wheel_delta = GET_WHEEL_DELTA_WPARAM(wParam) + delta_accum;
        delta_accum = wheel_delta % WHEEL_DELTA;
        wheel_delta /= WHEEL_DELTA;
        bool zoom_in = (wheel_delta >= 0);
        wheel_delta = abs(wheel_delta);
        GameWinState state = current_window();

        if (state == GW_World) {
            int zoom_type = (zoom_in ? 515 : 516);
            for (int i = 0; i < wheel_delta; i++) {
                if (MapWin->ZoomFactor > -8 || zoom_in) {
                    Console_zoom(MapWin, zoom_type, 0);
                }
            }
        } else if (state == GW_Base && conf.render_high_detail) {
            BaseWin_support_zoom(zoom_in);
        } else {
            int key;
            if (state == GW_Design) {
                key = (zoom_in ? VK_LEFT : VK_RIGHT);
            } else {
                key = (zoom_in ? VK_UP : VK_DOWN);
            }
            wheel_delta *= CState.ListScrollDelta;
            for (int i = 0; i < wheel_delta; i++) {
                PostMessage(hwnd, WM_KEYDOWN, key, 0);
                PostMessage(hwnd, WM_KEYUP, key, 0);
            }
            return 0;
        }

    } else if (msg == WM_KEYDOWN && (wParam == VK_UP || wParam == VK_DOWN)
    && conf.render_high_detail && ctrl_key_down() && current_window() == GW_Base) {
        BaseWin_support_zoom(wParam == VK_UP);

    } else if (msg == WM_KEYDOWN && (wParam == VK_LEFT || wParam == VK_RIGHT)
    && ctrl_key_down() && current_window() == GW_Base) {
        int32_t value = BaseWin->res_window_tab;
        if (wParam == VK_LEFT) {
            value = (value + 1) % 3;
        } else {
            value = (value + 2) % 3;
        }
        BaseWin->res_window_tab = value;
        GraphicWin_redraw(BaseWin);

    } else if (msg == WM_KEYDOWN && wParam == 'H' && ctrl_key_down()
    && !*MultiplayerActive && current_window() == GW_Base && *CurrentBaseID >= 0
    && Bases[*CurrentBaseID].faction_id == MapWin->cOwner) {
        BASE* base = &Bases[*CurrentBaseID];
        Faction* f = &Factions[base->faction_id];
        int mins = max(0, mineral_cost(*CurrentBaseID, base->item()) - base->minerals_accumulated);
        int cost = hurry_cost(*CurrentBaseID, base->item(), mins);
        if (base->can_hurry_item() && cost > 0 && mins > 0) {
            if (max(0, f->energy_credits - f->hurry_cost_total) >= cost) {
                f->energy_credits -= cost;
                base->minerals_accumulated += mins;
                base->state_flags |= BSTATE_HURRY_PRODUCTION;
                GraphicWin_redraw(BaseWin);
                ok_callback();
            } else {
                wave_it(9); // Insufficient energy
            }
        } else if (cost > 0 && mins > 0) {
            wave_it(8); // Cannot execute order
        }

    } else if (conf.smooth_scrolling && msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) {
        if (current_window() != GW_World || !win_has_focus()) {
            CState.RightButtonDown = false;
            CState.ScrollDragging = false;
            return WinProc(hwnd, msg, wParam, lParam);

        } else if (msg == WM_RBUTTONDOWN) {
            CState.RightButtonDown = true;
            GetCursorPos(&p);
            memcpy(&CState.ScrollDragPos, &p, sizeof(POINT));

        } else if (msg == WM_RBUTTONUP) {
            CState.RightButtonDown = false;
            if (CState.ScrollDragging) {
                CState.ScrollDragging = false;
                SetCursor(LoadCursor(0, IDC_ARROW));
            } else {
                WinProc(hwnd, WM_RBUTTONDOWN, wParam | MK_RBUTTON, lParam);
                return WinProc(hwnd, WM_RBUTTONUP, wParam, lParam);
            }
        } else if (CState.RightButtonDown) {
            check_scroll();
        } else {
            return WinProc(hwnd, msg, wParam, lParam);
        }

    } else if (conf.smooth_scrolling && msg == WM_CHAR && wParam == 'r' && alt_key_down()) {
        CState.MouseOverTileInfo = !CState.MouseOverTileInfo;

    } else if (!conf.reduced_mode && msg == WM_CHAR && wParam == 't' && alt_key_down()) {
        show_mod_menu();

    } else if (conf.reduced_mode && msg == WM_CHAR && wParam == 'h' && alt_key_down()) {
        show_mod_menu();

    } else if (msg == WM_CHAR && wParam == 'o' && alt_key_down() && is_editor) {
        uint32_t seed = ThinkerVars->map_random_value;
        int value = pop_ask_number_4("modmenu", "MAPGEN", seed, 0);
        if (!value) { // OK button pressed
            console_world_generate(ParseNumTable[0]);
        }

    } else if (msg == WM_CHAR && wParam == 'l' && alt_key_down() && is_editor
    && *ReplayEventSize > 0) {
        show_replay();

    } else if (DEBUG && !*GameHalted && msg == WM_KEYDOWN && wParam == 'Q'
    && ctrl_key_down() && shift_key_down()) {
        net_game_close(); // Close network multiplayer if active
        *ControlTurnA = 1; // Return to main menu without dialog
        *ControlTurnB = 1;

    } else if (DEBUG && msg == WM_CHAR && wParam == 'd' && alt_key_down()) {
        conf.debug_mode = !conf.debug_mode;
        if (conf.debug_mode) {
            for (int i = 1; i < MaxPlayerNum; i++) {
                Faction& f = Factions[i];
                if (!f.base_count) {
                    memset(f.goals, 0, sizeof(f.goals));
                    memset(f.sites, 0, sizeof(f.sites));
                }
            }
            *GameState |= STATE_DEBUG_MODE;
            *GamePreferences |= PREF_ADV_FAST_BATTLE_RESOLUTION;
        } else {
            *GameState &= ~STATE_DEBUG_MODE;
        }
        if (!*GameHalted) {
            MapWin_draw_map(MapWin, 0);
            InvalidateRect(hwnd, NULL, false);
            parse_says(0, MOD_VERSION, -1, -1);
            parse_says(1, (conf.debug_mode ?
                "Debug mode enabled." : "Debug mode disabled."), -1, -1);
            popp("modmenu", "GENERIC", 0, 0, 0);
        }

    } else if (debug_cmd && wParam == 'm' && alt_key_down()) {
        conf.debug_verbose = !conf.debug_verbose;
        parse_says(0, MOD_VERSION, -1, -1);
        parse_says(1, (conf.debug_verbose ?
            "Verbose mode enabled." : "Verbose mode disabled."), -1, -1);
        popp("modmenu", "GENERIC", 0, 0, 0);

    } else if (debug_cmd && wParam == 'b' && alt_key_down() && Win_is_visible(BaseWin)) {
        conf.base_psych = !conf.base_psych;
        base_compute(1);
        BaseWin_on_redraw(BaseWin);

    } else if (debug_cmd && wParam == 'p' && alt_key_down()) {
        parse_says(0, "diplomatic patience", -1, -1);
        int value = pop_ask_number_4("modmenu", "ASKNUMBER", conf.diplo_patience, 0);
        if (!value) { // OK button pressed
            conf.diplo_patience = max(0, ParseNumTable[0]);
        }

    } else if (debug_cmd && wParam == 'c' && alt_key_down() && is_editor
    && (sq = mapsq(MapWin->TileX, MapWin->TileY)) && sq->lm_items()) {
        uint32_t prev_state = MapWin->DrawFlags;
        MapWin->DrawFlags |= MAPWIN_DRAW_GOALS;
        refresh_overlay(code_at);
        int value = pop_ask_number_4("modmenu", "MAPGEN", sq->code_at(), 0);
        if (!value) { // OK button pressed
            code_set(MapWin->TileX, MapWin->TileY, ParseNumTable[0]);
        }
        refresh_overlay(clear_overlay);
        MapWin->DrawFlags = prev_state;
        draw_map(1);

    } else if (debug_cmd && wParam == 'y' && alt_key_down()) {
        static int draw_diplo = 0;
        draw_diplo = !draw_diplo;
        if (draw_diplo) {
            MapWin->DrawFlags |= MAPWIN_DRAW_DIPLO_STATE;
            *GameState |= STATE_DEBUG_MODE;
        } else {
            MapWin->DrawFlags &= ~MAPWIN_DRAW_DIPLO_STATE;
        }
        MapWin_draw_map(MapWin, 0);
        InvalidateRect(hwnd, NULL, false);

    } else if (debug_cmd && wParam == 'v' && alt_key_down()) {
        MapWin->DrawFlags |= MAPWIN_DRAW_GOALS;
        refresh_overlay(clear_overlay);
        static int ts_type = 0;
        int i = 0;
        TileSearch ts;
        ts_type = (ts_type+1) % (MaxTileSearchType+1);
        ts.init(MapWin->TileX, MapWin->TileY, ts_type, 0);
        while (ts.get_next() != NULL) {
            mapdata[{ts.rx, ts.ry}].overlay = ++i;
        }
        mapdata[{MapWin->TileX, MapWin->TileY}].overlay = -ts_type;
        MapWin_draw_map(MapWin, 0);
        InvalidateRect(hwnd, NULL, false);

    } else if (debug_cmd && wParam == 'f' && alt_key_down()
    && (sq = mapsq(MapWin->TileX, MapWin->TileY)) && sq->is_owned()) {
        MapWin->DrawFlags |= MAPWIN_DRAW_GOALS;
        move_upkeep(sq->owner, UM_Visual);
        MapWin_draw_map(MapWin, 0);
        InvalidateRect(hwnd, NULL, false);

    } else if (debug_cmd && wParam == 'x' && alt_key_down()) {
        MapWin->DrawFlags |= MAPWIN_DRAW_GOALS;
        static int px = 0, py = 0;
        int x = MapWin->TileX, y = MapWin->TileY;
        int unit_id = is_ocean(mapsq(x, y)) ? BSC_UNITY_FOIL : BSC_UNITY_ROVER;
        show_path_cost(px, py, x, y, unit_id, MapWin->cOwner);
        px=x;
        py=y;
        MapWin_draw_map(MapWin, 0);
        InvalidateRect(hwnd, NULL, false);

    } else if (debug_cmd && wParam == 'z' && alt_key_down()) {
        int x = MapWin->TileX, y = MapWin->TileY;
        int base_id;
        if ((base_id = base_at(x, y)) >= 0) {
            print_base(base_id);
        }
        print_map(x, y);
        for (int k = 0; k < *VehCount; k++) {
            VEH* veh = &Vehs[k];
            if (veh->x == x && veh->y == y) {
                Vehs[k].state |= VSTATE_UNK_40000;
                Vehs[k].state &= ~VSTATE_UNK_2000;
                print_veh(k);
            }
        }
        flushlog();

    } else {
        return WinProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int __cdecl mod_Win_init_class(const char* lpWindowName)
{
    int value = Win_init_class(lpWindowName);
    set_video_mode(0);
    return value;
}

void __cdecl amovie_project(int movie_id)
{
    if (!text_open(conf.smac_only ? MovlistFile : ExpMovlistFile, "PROJECTS")) {
        text_get();
        while (movie_id > 0) {
            text_get();
            --movie_id;
        }
        mod_amovie_project(text_item());
    }
}

void __cdecl mod_amovie_project(const char* name)
{
    // Cinematic presentation only. Do not change the player's movie
    // preference or the project's production/achievement processing.
    if (autosim_ui_active()) {
        telemetry_record_ui_notice("movie", name ? name : "",
            "suppressed_project_movie");
        return;
    }
    if (!strlen(name) || !conf.video_player) {
        return;
    } else if (conf.video_player == 1) {
        conf.playing_movie = true;
        amovie_project_2(name);
    } else if (conf.video_player == 2) {
        conf.playing_movie = true;
        PROCESS_INFORMATION pi = {};
        STARTUPINFO si = {};
        std::string base_path = ".\\movies\\" + std::string(name);
        std::string movie_path = base_path + ".wve";
        if (!video_player_extn.empty()) {
            std::string custom_extn = video_player_extn;
            if (custom_extn[0] != '.') custom_extn = "." + custom_extn;
            std::string custom_path = base_path + custom_extn;
            if (FileExists(custom_path.c_str())) {
                movie_path = custom_path;
            }
        }
        std::string cmd = "\"" + video_player_path + "\" " + video_player_args
            + " " + movie_path;
        std::vector<char> cmd_buf(cmd.begin(), cmd.end());
        cmd_buf.push_back('\0');
        if (CreateProcessA(NULL, cmd_buf.data(),
        NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }
    if (*phWnd) {
        PostMessage(*phWnd, WM_MOVIEOVER, 0, 0);
    }
}

void restore_video_mode()
{
    ChangeDisplaySettings(NULL, 0);
}

void set_video_mode(bool reset_window)
{
    if (conf.video_mode != VM_Window
    && (conf.window_width != conf.screen_width || conf.window_height != conf.screen_height)) {
        DEVMODE dv = {};
        dv.dmPelsWidth = conf.window_width;
        dv.dmPelsHeight = conf.window_height;
        dv.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
        dv.dmSize = sizeof(dv);
        ChangeDisplaySettings(&dv, CDS_FULLSCREEN);
    }
    else if (conf.video_mode == VM_Window && reset_window && *phWnd) {
        // Restore window layout after movie playback has ended
        restore_video_mode();
        SetWindowLong(*phWnd, GWL_STYLE, AC_WS_WINDOWED);
        PostMessage(*phWnd, WM_WINDOWED, 0, 0);
    }
}

void set_minimised(bool minimise)
{
    if (conf.minimised != minimise && *phWnd) {
        conf.minimised = minimise;
        if (minimise) {
            restore_video_mode();
            ShowWindow(*phWnd, SW_MINIMIZE);
        } else {
            set_video_mode(0);
            ShowWindow(*phWnd, SW_RESTORE);
        }
    }
}

void set_windowed(bool windowed)
{
    if (!conf.playing_movie && *phWnd) {
        if (conf.video_mode == VM_Custom && windowed) {
            conf.video_mode = VM_Window;
            restore_video_mode();
            SetWindowLong(*phWnd, GWL_STYLE, AC_WS_WINDOWED);
            PostMessage(*phWnd, WM_WINDOWED, 0, 0);
        }
        else if (conf.video_mode == VM_Window && !windowed) {
            conf.video_mode = VM_Custom;
            set_video_mode(0);
            SetWindowLong(*phWnd, GWL_STYLE, AC_WS_FULLSCREEN);
            SetWindowPos(*phWnd, HWND_TOPMOST, 0, 0, conf.window_width, conf.window_height,
                         SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE | SWP_FRAMECHANGED);
            ShowWindow(*phWnd, SW_RESTORE);
        }
    }
}

#ifndef BUILD_REL
// Resolve relative resources and diagnostics against the executable folder,
// not the process working directory (which may differ in some installations).
static std::string minor_graphics_file(const char* name)
{
    char path[MAX_PATH] = {};
    DWORD len = GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (len && len < MAX_PATH) {
        char* end = strrchr(path, '\\');
        if (end) {
            *(end + 1) = '\0';
            return std::string(path) + name;
        }
    }
    return name;
}


/*
Create the Develop diagnostic log at startup. The earlier broad native
instruction scanners have been retired: they were read-only exploratory
tools and flooded every run with obsolete hex dumps. Exact supported hook
sites have their own expected-byte preflight in patch.cpp.
*/
void minor_graphics_startup_trace()
{
    const std::string path = minor_graphics_file("minor_terrain_render.log");
    FILE* fp = fopen(path.c_str(), "w");
    if (fp) {
        fprintf(fp, "Thinker minor-terrain graphics: Develop build startup reached\n");
        fclose(fp);
    }
}


/*
REND-005 native Uranium diagnostic, NOT a rendering patch. The only
verified type-4 sprite call (0x465E37) is NOT proven to be the separate
landmark-underlay call used for Uranium plus ordinary resources.

Read only a tightly bounded portion of MapWin_gen_terrain_poly after
Scient's patch but before experimental native hooks. Print candidate
Uranium landmark flag predicates and confirmed Sprite_draw_3 call targets,
with short opcode context for reproducible disassembly on the user's exe.
Never guess a hook from a mask alone; no engine memory writes.
*/
void minor_graphics_underlay_native_probe()
{
#if defined(__GNUC__) && defined(__i386__)
    FILE* fp = fopen(minor_graphics_file("minor_terrain_render.log").c_str(), "a");
    if (!fp) {
        return;
    }
    constexpr uintptr_t first = 0x4632D0u;
    constexpr uintptr_t last = 0x467000u;
    constexpr uintptr_t sprite_draw_3 = 0x5E4B9Au;
    constexpr int max_predicates = 24;
    constexpr int max_draw_calls = 24;
    int predicates = 0, draw_calls = 0;

    fprintf(fp, "REND005 READONLY native Uranium probe begin range=0x%08X..0x%08X\n",
        (unsigned)first, (unsigned)last);

    // Reproducible original branch context: bonus_at, 4-branch, native
    // Sprite* selection, draw and the original jump over other resources.
    const uintptr_t branch_start = 0x465DF0u;
    const byte* branch = reinterpret_cast<const byte*>(branch_start);
    for (unsigned offset = 0; offset < 0xA0u; offset += 16) {
        fprintf(fp, "REND005 BRANCH 0x%08X", (unsigned)(branch_start + offset));
        for (unsigned j = 0; j < 16; ++j) {
            fprintf(fp, " %02X", (unsigned)branch[offset + j]);
        }
        fputc('\n', fp);
    }

    for (uintptr_t site = first; site + 7u < last; ++site) {
        const byte* p = reinterpret_cast<const byte*>(site);
        uint32_t mask = 0;
        bool possible_mask = false;
        // Immediate AND / TEST of a register: 81 E0..E7 imm32,
        // F7 C0..C7 imm32. These are candidate starts, not decoded
        // instructions; overlapping byte patterns need disassembly.
        if ((p[0] == 0x81 && p[1] >= 0xE0 && p[1] <= 0xE7)
            || (p[0] == 0xF7 && p[1] >= 0xC0 && p[1] <= 0xC7)) {
            memcpy(&mask, p + 2, sizeof(mask));
            possible_mask = mask == 0x00000008u
                || mask == 0x80000008u || mask == 0x8000000Cu;
        }
        // Short AND / TEST reg,imm8. These are especially common for
        // checking single-bit LM flags.
        const bool possible_short =
            ((p[0] == 0x83 && p[1] >= 0xE0 && p[1] <= 0xE7)
            || (p[0] == 0xF6 && p[1] >= 0xC0 && p[1] <= 0xC7))
            && p[2] == 0x08;

        if ((possible_mask || possible_short) && predicates < max_predicates) {
            ++predicates;
            fprintf(fp, "REND005 CANDIDATE %02d at=0x%08X op=%02X %02X mask=0x%08X context=",
                predicates, (unsigned)site, (unsigned)p[0],
                (unsigned)p[1], possible_short ? 8u : (unsigned)mask);
            const uintptr_t context = site >= first + 8 ? site - 8 : first;
            for (uintptr_t addr = context; addr < site + 16; ++addr) {
                fprintf(fp, "%02X", (unsigned)*reinterpret_cast<const byte*>(addr));
            }
            fputc('\n', fp);
        }

        // E8 rel32 calls resolving to the already known sprite draw
        // routine. Report potential *other* calls separately from the
        // verified 0x465E37 resource-only call.
        if (p[0] == 0xE8) {
            int32_t relative = 0;
            memcpy(&relative, p + 1, sizeof(relative));
            const uintptr_t target =
                static_cast<uintptr_t>(static_cast<intptr_t>(site + 5)
                    + static_cast<intptr_t>(relative));
            if (target == sprite_draw_3 && draw_calls < max_draw_calls) {
                ++draw_calls;
                fprintf(fp, "REND005 SPRITE_CALL %02d at=0x%08X target=0x%08X context=",
                    draw_calls, (unsigned)site, (unsigned)target);
                const uintptr_t context = site >= first + 8 ? site - 8 : first;
                for (uintptr_t addr = context; addr < site + 16; ++addr) {
                    fprintf(fp, "%02X", (unsigned)*reinterpret_cast<const byte*>(addr));
                }
                fputc('\n', fp);
            }
        }
    }
    fprintf(fp, "REND005 READONLY native Uranium probe end candidates=%d sprite_calls=%d"
        " caps=%d,%d (no hooks installed)\n",
        predicates, draw_calls, max_predicates, max_draw_calls);
    fclose(fp);
#endif
}


/*
Ready-to-use native feature-line composition, kept outside the engine
until the original local text-buffer callsite and capacity are verified.
No native LM bits, Natural[] entries or MAP fields are ever modified.
*/
static std::string minor_inspector_feature_line(const char* native, int x, int y)
{
    std::string line = native ? native : "";
    const std::string minor = minor_terrain_names_at(x, y);
    if (!minor.empty()) {
        if (!line.empty()) {
            line += ", ";
        }
        line += minor;
    }
    return line;
}

// Report whether the verified native inspector callsite was installed.
void minor_graphics_inspector_hook_result(int addr, bool active)
{
    const std::string path = minor_graphics_file("minor_terrain_render.log");
    FILE* fp = fopen(path.c_str(), "a");
    if (fp) {
        fprintf(fp, "INSPECTOR HOOK native feature strlen site=0x%08X active=%d\n",
            (unsigned)addr, active ? 1 : 0);
        fclose(fp);
    }
}

#if defined(__GNUC__) && defined(__i386__)
// At 0x4B7A0C, native ESI is tile x/2 and EDI is tile y, as verified by
// the immediately preceding native MAP index calculation. The original
// function has completed the River/Forest/Fungus line in StrBuffer and
// calls strlen to decide whether to draw that line. We add only the
// independent feature name, without modifying MAP flags, #NATURAL names,
// the whole StatusWin function or its layout/line position.
extern "C" int __cdecl minor_inspector_length_inner(char* text, int tile_half_x, int tile_y)
{
    if (!text) {
        return 0;
    }
    const size_t length = strlen(text);
    // OpenSMAC's reconstructed original globals.h: StrBuffer is char[256]
    // at 0x9B86A0. Accept only this exact native buffer/callsite.
    const size_t capacity = 256;
    if (text != reinterpret_cast<char*>(0x9B86A0)
        || length >= capacity || !*MapTiles || !*MapHalfX
        || tile_half_x < 0 || tile_half_x >= *MapHalfX
        || tile_y < 0 || tile_y >= *MapAreaY
        || MapWin->cOwner < 0 || MapWin->cOwner >= MaxPlayerNum) {
        return static_cast<int>(length);
    }
    const int x = 2 * tile_half_x + (tile_y & 1);
    MAP* sq = mapsq(x, tile_y);
    if (!sq || !(sq->is_known(MapWin->cOwner)
        || (*GameState & STATE_OMNISCIENT_VIEW))) {
        return static_cast<int>(length);
    }
    const std::string name = minor_terrain_names_at(x, tile_y);
    if (name.empty()) {
        return static_cast<int>(length);
    }
    const size_t name_len = name.size();
    const size_t separator_len = length ? 2 : 0;
    if (length + separator_len >= capacity
        || name_len >= capacity - length - separator_len) {
        return static_cast<int>(length);
    }
    char* end = text + length;
    if (separator_len) {
        *end++ = ',';
        *end++ = ' ';
    }
    memcpy(end, name.c_str(), name_len + 1);
    // Unlike hover-only diagnostics, this verifies the native Inspector
    // hook actually appended the label. Limit writes on frequent redraws.
    static int append_reports = 0;
    if (append_reports < 24) {
        ++append_reports;
        FILE* fp = fopen(minor_graphics_file("minor_terrain_render.log").c_str(), "a");
        if (fp) {
            fprintf(fp, "INSPECTOR APPEND x=%d y=%d minor=%s native_len=%u\n",
                x, tile_y, name.c_str(), (unsigned)length);
            fclose(fp);
        }
    }
    return static_cast<int>(length + separator_len + name_len);
}

// Native caller already pushed one cdecl char* parameter; at shim entry
// ESI/EDI belong to StatusWin_draw_status. Pass those saved coordinates
// to the safe C++ append helper and retain the normal strlen return/ABI.
extern "C" __attribute__((naked, used))
int __cdecl minor_native_inspector_strlen_hook()
{
    __asm__ __volatile__(
        "movl 4(%esp), %eax\n\t"
        "pushl %edi\n\t"
        "pushl %esi\n\t"
        "pushl %eax\n\t"
        "call _minor_inspector_length_inner\n\t"
        "addl $12, %esp\n\t"
        "ret\n\t"
    );
}
#endif

// Confirm the independent registry returns the intended text for the
// exact hovered/inspected tile. The optional native hook is separate.
static void minor_inspector_trace_hover(int x, int y)
{
    static int reports = 0, previous_x = -1, previous_y = -1;
    if (reports >= 20 || (x == previous_x && y == previous_y)) {
        return;
    }
    MAP* sq = mapsq(x, y);
    if (!sq || MapWin->cOwner < 0 || MapWin->cOwner >= MaxPlayerNum
        || !(sq->is_known(MapWin->cOwner)
            || (*GameState & STATE_OMNISCIENT_VIEW))) {
        return;
    }
    const std::string name = minor_terrain_names_at(x, y);
    if (name.empty()) {
        return;
    }
    previous_x = x;
    previous_y = y;
    ++reports;
    const std::string expected = minor_inspector_feature_line("Forest", x, y);
    const std::string path = minor_graphics_file("minor_terrain_render.log");
    FILE* fp = fopen(path.c_str(), "a");
    if (fp) {
        fprintf(fp, "INSPECTOR HOVER x=%d y=%d minor=%s sample_feature_line=%s\n",
            x, y, name.c_str(), expected.c_str());
        fclose(fp);
    }
}

// Temporary graphics diagnostic; record only transition events.
static void minor_graphics_trace(const char* message, int x, int y, int extra)
{
    const std::string path = minor_graphics_file("minor_terrain_render.log");
    FILE* fp = fopen(path.c_str(), "a");
    if (fp) {
        fprintf(fp, "%s x=%d y=%d extra=%d\n", message, x, y, extra);
        fclose(fp);
    }
}
#endif

/*
Render custom debug overlays with original and additional goals.
*/
void __thiscall MapWin_gen_overlays(Console* This, int x, int y)
{
#ifndef BUILD_REL
    static bool first_overlay_call = true;
    if (first_overlay_call) {
        first_overlay_call = false;
        minor_graphics_trace("original overlay callback entered", x, y, This == MapWin);
    }
    // Use an existing Thinker per-tile draw callback as a development
    // alternative to the current post-map paint. Its order relative to
    // native base/unit passes still needs in-game validation.
    if (This == MapWin && conf.minor_render_mode == 1) {
        draw_minor_terrain_tile(This, x, y);
    }
    // Diagnostic labels are painted on all discovered minor tiles
    // at readable zoom levels. Hovering a tile does not itself guarantee
    // a full map redraw, so do not depend on StatusWin's cursor position.
    // This does not touch the native status panel or landmark table.
    if (This == MapWin && conf.minor_debug_tile_names
        && This->PixelsPerTileX >= 60) {
        const std::string name = minor_terrain_names_at(x, y);
        MAP* sq = mapsq(x, y);
        if (!name.empty() && sq && This->cOwner >= 0
            && This->cOwner < MaxPlayerNum && sq->is_known(This->cOwner)) {
            RECT label_rect;
            MapWin_tile_to_pixel(This, x, y, &label_rect.left, &label_rect.top);
            label_rect.right = label_rect.left + This->PixelsPerTileX;
            label_rect.bottom = label_rect.top + This->PixelsPerTileY;
            char label[64] = {};
            strcpy_n(label, sizeof(label), name.c_str());
            Buffer_set_text_color(&This->oMainWin.oCanvas, ColorWhite, 0, 1, 1);
            Buffer_set_font(&This->oMainWin.oCanvas, &This->oFont2, 0, 0, 0);
            Buffer_write_cent_l_7(&This->oMainWin.oCanvas, label, &label_rect, 20);
        }
    }
#endif
    Buffer* Canvas = &This->oMainWin.oCanvas;
    RECT rt;
    if (*GameState & STATE_OMNISCIENT_VIEW && This->DrawFlags & MAPWIN_DRAW_GOALS)
    {
        MapWin_tile_to_pixel(This, x, y, &rt.left, &rt.top);
        rt.right = rt.left + This->PixelsPerTileX;
        rt.bottom = rt.top + This->PixelsPerHalfTileY;

        char buf[20] = {};
        bool found = false;
        int color = 255;
        int value = mapdata[{x, y}].overlay;

        for (int faction = 1; faction < MaxPlayerNum && !found; faction++) {
            Faction& f = Factions[faction];
            MFaction& m = MFactions[faction];
            if (!f.base_count) {
                continue;
            }
            for (int i = 0; i < MaxGoalsNum && !found; i++) {
                Goal& goal = Factions[faction].goals[i];
                if (goal.x == x && goal.y == y && goal.priority > 0
                && goal.type != AI_GOAL_UNUSED ) {
                    found = true;
                    buf[0] = m.filename[0];
                    switch (goal.type) {
                        case AI_GOAL_ATTACK:
                            buf[1] = 'a';
                            color = ColorRed;
                            break;
                        case AI_GOAL_DEFEND:
                            buf[1] = 'd';
                            color = ColorYellow;
                            break;
                        case AI_GOAL_SCOUT:
                            buf[1] = 's';
                            color = ColorMagenta;
                            break;
                        case AI_GOAL_UNK_1:
                            buf[1] = 'n';
                            color = ColorBlue;
                            break;
                        case AI_GOAL_COLONIZE:
                            buf[1] = 'c';
                            color = ColorCyan;
                            break;
                        case AI_GOAL_TERRAFORM_LAND:
                            buf[1] = 'f';
                            color = ColorGreen;
                            break;
                        case AI_GOAL_UNK_4:
                            buf[1] = '^';
                            break;
                        case AI_GOAL_RAISE_LAND:
                            buf[1] = 'r';
                            break;
                        case AI_GOAL_NAVAL_BEACH:
                        case AI_GOAL_NAVAL_START:
                        case AI_GOAL_NAVAL_END:
                            buf[1] = 'n';
                            break;
                        default:
                            buf[1] = (goal.type < Thinker_Goal_ID_First ? '*' : 'g');
                            break;
                    }
                    snprintf(&buf[2], sizeof(buf) - 2, "%d", goal.priority);
                }
            }
        }
        if (!found && value != 0) {
            color = (value >= 0 ? ColorWhite : ColorYellow);
            snprintf(buf, sizeof(buf), "%d", value);
        }
        if (found || value) {
            Buffer_set_text_color(Canvas, color, 0, 1, 1);
            Buffer_set_font(Canvas, &This->oFont2, 0, 0, 0);
            Buffer_write_cent_l_7(Canvas, buf, &rt, 20);
        }
    }
}

void __cdecl mod_turn_timer()
{
    /*
    Timer calls this function every 500ms.
    Used for multiplayer related screen updates in turn_timer().
    */
    static uint32_t iter = 0;
    static uint32_t prev_time = 0;
    turn_timer(0);
    if (++iter & 1) {
        return;
    }
    if (*GameHalted && !*MapRandomSeed) {
        ThinkerVars->game_time_spent = 0;
        prev_time = 0;
    } else {
        uint32_t now = GetTickCount();
        if (prev_time && now - prev_time > 0) {
            ThinkerVars->game_time_spent += now - prev_time;
        }
        prev_time = now;
    }
}

void popup_homepage()
{
    ShellExecute(NULL, "open", "https://github.com/induktio/thinker", NULL, NULL, SW_SHOWNORMAL);
}

void show_mod_stats()
{
    int total_pop = 0,
        total_minerals = 0,
        total_energy = 0,
        faction_bases = 0,
        faction_pop = 0,
        faction_units = 0,
        faction_minerals = 0,
        faction_energy = 0;

    for (int i = 0; i < *BaseCount; ++i) {
        BASE* b = &Bases[i];
        int mindiv = (has_project(FAC_SPACE_ELEVATOR, b->faction_id)
             && (b->item() == -FAC_ORBITAL_DEFENSE_POD
             || b->item() == -FAC_NESSUS_MINING_STATION
             || b->item() == -FAC_ORBITAL_POWER_TRANS
             || b->item() == -FAC_SKY_HYDRO_LAB) ? 2 : 1);
        if (b->faction_id == MapWin->cOwner) {
            faction_bases++;
            faction_pop += b->pop_size;
            faction_minerals += b->mineral_intake_2 / mindiv;
            faction_energy += b->energy_intake_2;
        }
        total_pop += b->pop_size;
        total_minerals += b->mineral_intake_2 / mindiv;
        total_energy += b->energy_intake_2;
    }
    for (int i = 0; i < *VehCount; i++) {
        VEH* v = &Vehs[i];
        if (v->faction_id == MapWin->cOwner) {
            faction_units++;
        }
    }
    ParseNumTable[0] = *BaseCount;
    ParseNumTable[1] = *VehCount;
    ParseNumTable[2] = total_pop;
    ParseNumTable[3] = total_minerals;
    ParseNumTable[4] = total_energy;
    ParseNumTable[5] = faction_bases;
    ParseNumTable[6] = faction_units;
    ParseNumTable[7] = faction_pop;
    ParseNumTable[8] = faction_minerals;
    ParseNumTable[9] = faction_energy;
    popp("modmenu", "STATS", 0, "markbm_sm.pcx", 0);
}

int show_mod_config()
{
    enum {
        MapGen = 1,
        MapContinents = 2,
        MapLandmarks = 4,
        MapPolarCaps = 8,
        MapMirrorX = 16,
        MapMirrorY = 32,
        AutoBases = 64,
        AutoUnits = 128,
        FormerReplace = 256,
        MapBaseInfo = 512,
        TreatyPopup = 1024,
        AutoMinimise = 2048,
    };
    *DialogChoices = 0
        | (conf.new_world_builder ? MapGen : 0)
        | (conf.world_continents ? MapContinents : 0)
        | (conf.modified_landmarks ? MapLandmarks : 0)
        | (conf.world_polar_caps ? MapPolarCaps: 0)
        | (conf.world_mirror_x ? MapMirrorX : 0)
        | (conf.world_mirror_y ? MapMirrorY : 0)
        | (conf.manage_player_bases ? AutoBases : 0)
        | (conf.manage_player_units ? AutoUnits : 0)
        | (conf.warn_on_former_replace ? FormerReplace : 0)
        | (conf.render_base_info ? MapBaseInfo : 0)
        | (conf.foreign_treaty_popup ? TreatyPopup : 0)
        | (conf.auto_minimise ? AutoMinimise : 0);

    // Return value is equal to choices bitfield if OK pressed, -1 otherwise.
    int value = X_pop_9("modmenu", "OPTIONS", -1, 0, POPUP_CHECKBOX|POPUP_BTN_CANCEL, 0);
    if (value < 0) {
        return 0;
    }
    conf.new_world_builder = !!(value & MapGen);
    WritePrivateProfileStringA(ModAppName, "new_world_builder",
        (conf.new_world_builder ? "1" : "0"), ModIniFile);

    conf.modified_landmarks = !!(value & MapLandmarks);
    WritePrivateProfileStringA(ModAppName, "modified_landmarks",
        (conf.modified_landmarks ? "1" : "0"), ModIniFile);

    conf.world_continents = !!(value & MapContinents);
    WritePrivateProfileStringA(ModAppName, "world_continents",
        (conf.world_continents ? "1" : "0"), ModIniFile);

    conf.world_polar_caps = !!(value & MapPolarCaps);
    WritePrivateProfileStringA(ModAppName, "world_polar_caps",
        (conf.world_polar_caps ? "1" : "0"), ModIniFile);

    conf.world_mirror_x = !!(value & MapMirrorX);
    WritePrivateProfileStringA(ModAppName, "world_mirror_x",
        (conf.world_mirror_x ? "1" : "0"), ModIniFile);

    conf.world_mirror_y = !!(value & MapMirrorY);
    WritePrivateProfileStringA(ModAppName, "world_mirror_y",
        (conf.world_mirror_y ? "1" : "0"), ModIniFile);

    conf.manage_player_bases = !!(value & AutoBases);
    WritePrivateProfileStringA(ModAppName, "manage_player_bases",
        (conf.manage_player_bases ? "1" : "0"), ModIniFile);

    conf.manage_player_units = !!(value & AutoUnits);
    WritePrivateProfileStringA(ModAppName, "manage_player_units",
        (conf.manage_player_units ? "1" : "0"), ModIniFile);

    conf.warn_on_former_replace = !!(value & FormerReplace);
    WritePrivateProfileStringA(ModAppName, "warn_on_former_replace",
        (conf.warn_on_former_replace ? "1" : "0"), ModIniFile);

    conf.render_base_info = !!(value & MapBaseInfo);
    WritePrivateProfileStringA(ModAppName, "render_base_info",
        (conf.render_base_info ? "1" : "0"), ModIniFile);

    conf.foreign_treaty_popup = !!(value & TreatyPopup);
    WritePrivateProfileStringA(ModAppName, "foreign_treaty_popup",
        (conf.foreign_treaty_popup ? "1" : "0"), ModIniFile);

    conf.auto_minimise = !!(value & AutoMinimise);
    WritePrivateProfileStringA(ModAppName, "auto_minimise",
        (conf.auto_minimise ? "1" : "0"), ModIniFile);

    draw_map(1);
    if (Win_is_visible(BaseWin)) {
        BaseWin_on_redraw(BaseWin);
    }
    return 0;
}

int show_mod_menu()
{
    parse_says(0, MOD_VERSION, -1, -1);
    parse_says(1, MOD_DATE, -1, -1);

    if (*GameHalted) {
        int ret = popp("modmenu", "MAINMENU", 0, "stars_sm.pcx", 0);
        if (ret == 1) {
            show_mod_config();
        }
        else if (ret == 2) {
            show_rules_menu();
        }
        else if (ret == 3) {
            popup_homepage();
        }
        return 0; // Close dialog
    }
    uint64_t seconds = ThinkerVars->game_time_spent / 1000;
    ParseNumTable[0] = seconds / 3600;
    ParseNumTable[1] = (seconds / 60) % 60;
    ParseNumTable[2] = seconds % 60;
    int ret = popp("modmenu", "GAMEMENU", 0, "stars_sm.pcx", 0);

    if (ret == 1 && !*PbemActive && !*MultiplayerActive) {
        show_mod_stats();
    }
    else if (ret == 2) {
        show_mod_config();
    }
    else if (ret == 3) {
        popup_homepage();
    }
    return 0; // Close dialog
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

int __thiscall SetupWin_buffer_draw(Buffer* src, Buffer* dst, int a3, int a4, int a5, int a6, int a7)
{
    if (conf.window_width >= 1024) {
        const int moon_positions[][4] = {
            {8, 287, 132, 132},
            {221, 0, 80, 51},
            {348, 94, 55, 57},
        };
        for (auto& p : moon_positions) {
            int x = conf.window_width  * p[0] / 1024;
            int y = conf.window_height * p[1] / 768;
            int w = conf.window_width  * p[2] / 1024;
            int h = conf.window_height * p[3] / 768;
            Buffer_copy_3(src, dst, p[0], p[1], p[2], p[3], x, y, w, h);
        }
        return 0;
    } else {
        return Buffer_draw_2(src, dst, a3, a4, a5, a6, a7);
    }
}

int __thiscall SetupWin_buffer_copy(Buffer* src, Buffer* dst,
int xSrc, int ySrc, int xDst, int yDst, int wSrc, int hSrc)
{
    if (conf.window_width >= 1024) {
        int wDst = conf.window_width * wSrc / 1024;
        return Buffer_copy_3(src, dst, xSrc, ySrc, wSrc, hSrc,
            conf.window_width - wDst, yDst, wDst, conf.window_height);
    } else {
        return Buffer_copy_4(src, dst, xSrc, ySrc, xDst, yDst, wSrc, hSrc);
    }
}

int __thiscall SetupWin_soft_update3(GraphicWin* This, int a2, int a3, int a4, int a5)
{
    // Update whole screen instead of partial regions
    return GraphicWin_soft_update_2(This);
}

int __thiscall window_scale_load_pcx(Buffer* This, char* filename, Palette* a3, int a4, int a5)
{
    int value;
    if (conf.window_width >= 1024) {
        Buffer image;
        Buffer_ctor(&image);
        value = Buffer_load_pcx(&image, filename, a3, a4, a5);
        Buffer_resize(This, conf.window_width, conf.window_height);
        Buffer_copy_3(&image, This, 0, 0, image.clipRect.right, image.clipRect.bottom,
            0, 0, conf.window_width, conf.window_height);
        Buffer_dtor(&image);
    } else {
        value = Buffer_load_pcx(This, filename, a3, a4, a5);
    }
    if (*GameHalted) {
        char buf[StrBufLen];
        snprintf(buf, StrBufLen, "%s%s%s%s",
            MOD_VERSION, " / ", MOD_DATE, (conf.smac_only ? " / SMAC" : ""));
        Buffer_set_text_color(This, ColorProdName, 0, 1, 1);
        Buffer_set_font(This, &MapWin->oFont1, 0, 0, 0);
        Buffer_write_l_3(This, buf, 20, conf.window_height-32, 100);
    }
    return value;
}

int __thiscall Credits_GraphicWin_init(
GraphicWin* This, int a2, int a3, int a4, int a5, char* a6, int a7, Win* a8, Menu* a9, BorderSizing* a10)
{
    if (conf.window_width >= 1024) {
        return GraphicWin_init(This,
            a2 * conf.window_width / 1024,
            a3 * conf.window_height / 768,
            a4 * conf.window_width / 1024,
            a5 * conf.window_height / 768,
            a6, a7, a8, a9, a10);
    } else {
        return GraphicWin_init(This, a2, a3, a4, a5, a6, a7, a8, a9, a10);
    }
}

#pragma GCC diagnostic pop

void __cdecl ReportWin_draw_ops_strcat(char* dst, char* UNUSED(src))
{
    BASE* base = *CurrentBase;
    uint32_t gov = base->governor_flags;
    size_t len = 0;
    dst[0] = '\0';

    if (base->faction_id == MapWin->cOwner && gov & GOV_ACTIVE) {
        const char* type;
        if (gov & GOV_PRIORITY_EXPLORE) {
            type = label_get(TL_Explore);
        } else if (gov & GOV_PRIORITY_DISCOVER) {
            type = label_get(TL_Discover);
        } else if (gov & GOV_PRIORITY_BUILD) {
            type = label_get(TL_Build);
        } else if (gov & GOV_PRIORITY_CONQUER) {
            type = label_get(TL_Conquer);
        } else {
            type = label_get(TL_Governor);
        }
        len += snprintf(dst, StrBufLen, "%s ", type);
    }
    if (strlen(label_base_surplus) && len < StrBufLen) {
        snprintf(dst + len, StrBufLen - len, label_base_surplus,
            base->nutrient_surplus, base->mineral_surplus, base->energy_surplus);
    }
}

void __thiscall ReportWin_draw_ops_color(Buffer* This, int UNUSED(a2), int a3, int a4, int a5)
{
    BASE* base = *CurrentBase;
    int color = (base->mineral_surplus < 0 || base->nutrient_surplus < 0
        ? ColorRed : ColorMediumBlue);
    Buffer_set_text_color(This, color, a3, a4, a5);
}

int __thiscall mod_MapWin_focus(MapWindow* This, int x, int y)
{
    // Return value is non-zero when the map is recentered offscreen
    if (MapWin_focus(This, x, y)) {
        This->DrawOnlyCursor = 0;
        draw_map(1);
    }
    return 0;
}

void __thiscall mod_MapWin_set_center(MapWindow* This, int x, int y, int flag)
{
    // Make sure the whole screen is refreshed when clicking on map tiles
    if (!in_box(x, y, RenderTileBounds)) {
        This->DrawOnlyCursor = 0;
    }
    return MapWin_set_center(This, x, y, flag);
}

/*
This is called when ReportWin is closing and is used to refresh base labels
on any bases where workers have been adjusted from the base list window.
*/
void __thiscall ReportWin_close_handler(SubInterface* This)
{
    SubInterface_release_iface_mode(This);
    draw_map(1);
}

/*
Fix foreign base names being visible in unexplored tiles when issuing move to or patrol
orders to the tiles. This version adds visibility checks for all base tiles.
*/
void __cdecl say_loc(char* dest, int x, int y, int a4, int a5, int a6)
{
    int base_id = -1;
    MAP* sq;
    if ((sq = mapsq(x, y)) && sq->is_base()
    && (*GameState & STATE_SCENARIO_EDITOR || sq->is_visible(MapWin->cOwner))) {
        base_id = base_at(x, y);
    }
    const char* prefix = "";
    const char* prefix_space = "";
    if (a4 != 0 && base_id < 0) {
        base_id = base_find_3(x, y, -1, -1, -1, MapWin->cOwner);
        if (base_id >= 0) {
            a6 = 0;
            prefix = label_get(TL_Near);
            prefix_space = " ";
        }
    }
    const char* base_name = "";
    const char* trail_space = "";
    if (base_id >= 0) {
        if (a6) {
            prefix = label_get(TL_At);
            prefix_space = " ";
        }
        base_name = Bases[base_id].name;
        if (a5) {
            trail_space = " ";
        }
    }
    size_t prev = strnlen(dest, StrBufLen);
    size_t len = StrBufLen - prev;
    if (a5 == 1 || (a5 == 2 && base_id < 0)) {
        snprintf(dest + prev, len, "%s%s%s%s(%d, %d)",
            prefix, prefix_space, base_name, trail_space, x, y);
    } else {
        snprintf(dest + prev, len, "%s%s%s%s",
            prefix, prefix_space, base_name, trail_space);
    }
}

/*
Fix diplomacy dialog to show the missing response messages (GAVEENERGY) when gifting
energy credits to another faction. The error happens when the label is written to
StrBuffer in make_gift but diplomacy_caption overwrites it with other data.
*/
void __cdecl mod_diplomacy_caption(int faction1, int faction2)
{
    char buf[StrBufLen];
    strcpy_n(buf, StrBufLen, StrBuffer);
    diplomacy_caption(faction1, faction2);
    strcpy_n(StrBuffer, StrBufLen, buf);
}

/*
Fix foreign_treaty_popup displaying same treaty changes multiple times per turn.
In any case, these events will be displayed as a non-persistent message in map window.
*/
static char netmsg_label[StrBufLen] = {};
static char netmsg_item0[StrBufLen] = {};
static char netmsg_item1[StrBufLen] = {};

void __cdecl reset_netmsg_status()
{
    netmsg_label[0] = '\0';
    netmsg_item0[0] = '\0';
    netmsg_item1[0] = '\0';
}

int __thiscall mod_NetMsg_pop(NetMessage* This, const char* label, int delay, int a4, const char* filename)
{
    if (!conf.foreign_treaty_popup) {
        return NetMsg_pop(This, label, delay, a4, filename);
    }
    if (!strcmp(label, "GOTMYPROBE")) {
        return NetMsg_pop(This, label, -1, a4, filename);
    }
    if (!strcmp(label, netmsg_label)
    && !strcmp(ParseStrBuffer[0].str, netmsg_item0)
    && !strcmp(ParseStrBuffer[1].str, netmsg_item1)) {
        // Skip additional popup windows
        return NetMsg_pop(This, label, delay, a4, filename);
    }
    strcpy_n(netmsg_label, StrBufLen, label);
    strcpy_n(netmsg_item0, StrBufLen, ParseStrBuffer[0].str);
    strcpy_n(netmsg_item1, StrBufLen, ParseStrBuffer[1].str);
    return NetMsg_pop(This, label, -1, a4, filename);
}

int __thiscall mod_BasePop_start(
BasePop* This, const char* filename, const char* label, int a4, char* a5, int a6, GraphicWin* a7)
{
    if (movedlabels.count(label)) {
        return BasePop_start(This, "modmenu", label, a4, a5, a6, a7);
    }
    return BasePop_start(This, filename, label, a4, a5, a6, a7);
}

/*
Modify Unit Workshop command to open prototype for the currently selected unit.
Normally unit_id can be set when ASKSEEDESIGN popup is opened from tech_achieved.
*/
int __cdecl mod_design_new_veh(int faction_id, int unit_id) {
    if (unit_id < 0 && MapWin->VehID >= 0) {
        VEH* veh = &Vehs[MapWin->VehID];
        if (((veh->unit_id < MaxProtoFactionNum
        && has_tech(Units[veh->unit_id].preq_tech, faction_id))
        || veh->unit_id / MaxProtoFactionNum == faction_id)
        && Units[veh->unit_id].icon_offset < 0
        && veh->x == MapWin->TileX && veh->y == MapWin->TileY) {
            return DesignWin_exec(DesignWin, faction_id, veh->unit_id);
        }
    }
    return DesignWin_exec(DesignWin, faction_id, unit_id);
}



/*
Independent minor-terrain FALLBACK renderer. The atlas has 128x64 slots:
Titanium, Jungle, Uranium. With native options enabled, both geological
types bypass this atlas and use the game's original Uranium sprite.
Only the experimental fallback is a Develop-only late post-map overlay;
its layering over units and PRACX support remain unresolved.
*/
#ifndef BUILD_REL
static Sprite minor_sprites[MINOR_TYPE_COUNT] = {};
static bool minor_sprite_ready[MINOR_TYPE_COUNT] = {};
static bool minor_sprites_attempted = false;

// The reference Uranium and experimental blue-grey Titanium use independent
// atlas rectangles and validation. Reuse the existing native draw hook;
// original-game art is provided locally and never redistributed in Git.
enum NecaSpriteId {
    NECA_SPRITE_URANIUM_TEST,
    NECA_SPRITE_TITANIUM_TEST,
    NECA_SPRITE_COUNT
};
struct NecaSpriteDef {
    const char* name;
    int x;
    int y;
    int width;
    int height;
    int transparent_index;
};
static const NecaSpriteDef neca_sprite_defs[NECA_SPRITE_COUNT] = {
    {"uranium_test", 16, 16, 100, 62, 9},
    {"titanium_test", 144, 16, 100, 62, 9},
};
static Sprite neca_sprites[NECA_SPRITE_COUNT] = {};
static bool neca_sprite_ready[NECA_SPRITE_COUNT] = {};
static bool neca_sprites_attempted = false;

// Native original LM_URANIUM underlay draw calls the same Sprite_draw_3
// routine from an earlier stage, independently of ordinary resource icons.
extern "C" {
uintptr_t minor_geology_underlay_draw_target = 0;
int minor_geology_underlay_current_type = MINOR_NONE;
int minor_geology_underlay_current_x = -1;
int minor_geology_underlay_current_y = -1;
}

void minor_graphics_underlay_hook_result(int mask_site, int draw_site, bool active)
{
    FILE* fp = fopen(minor_graphics_file("minor_terrain_render.log").c_str(), "a");
    if (fp) {
        fprintf(fp, "REND005 UNDERLAY HOOK mask=0x%08X draw=0x%08X active=%d\n",
            (unsigned)mask_site, (unsigned)draw_site, active ? 1 : 0);
        fclose(fp);
    }
}

// Native sprite substitution is strictly scoped to one verified resource
// drawing call in the supported terranx.exe, if that call can be found.
// The actual resource sprite rendering position remains the game's own.
bool minor_native_titanium_sprite_installed = false;
extern "C" { uintptr_t minor_titanium_native_draw_target = 0; }

void minor_graphics_titanium_hook_result(int bonus_site, int draw_site,
    int candidates, int target)
{
    const std::string path = minor_graphics_file("minor_terrain_render.log");
    FILE* fp = fopen(path.c_str(), "a");
    if (fp) {
        fprintf(fp, "TITANIUM DRAW HOOK bonus=0x%08X draw=0x%08X matches=%d target=0x%08X active=%d\n",
            (unsigned)bonus_site, (unsigned)draw_site, candidates,
            (unsigned)target, minor_native_titanium_sprite_installed ? 1 : 0);
        fclose(fp);
    }
}

static void load_minor_terrain_sprites(Buffer* canvas)
{
    if (minor_sprites_attempted || !canvas || !canvas->palette) {
        return;
    }
    minor_sprites_attempted = true;
    const std::string path = minor_graphics_file("minor_terrain.pcx");
    FILE* fp = fopen(path.c_str(), "rb");
    if (!fp) {
        minor_graphics_trace("minor terrain PCX unavailable", 0, 0, GetLastError());
        return;
    }
    fclose(fp);

    Buffer image;
    Buffer_ctor(&image);
    // Source art is already indexed against ter1.pcx's 256-colour
    // palette (verified byte-for-byte against the user's terrain PCX).
    // Passing the map canvas palette invokes an optional colour-table
    // conversion in the native PCX loader, changing the sprite's
    // individual ore pixels. Keep the original palette indices here.
    // Native Buffer_load_pcx_2 at 0x5E29C9 skips palette remapping when
    // the Palette* parameter is null, leaving the 8-bit PCX bytes.
    const int loaded = Buffer_load_pcx(&image, path.c_str(), nullptr, 0, 0);
    minor_graphics_trace("minor terrain atlas loaded / width / height",
        loaded, image.clipRect.right, image.clipRect.bottom);

    // Retain legacy Titanium at x=12 so existing 128x64 assets continue
    // to render it. The new 384x64 atlas adds Jungle and Uranium to the right.
    for (int id = MINOR_JUNGLE; id < MINOR_TYPE_COUNT; ++id) {
        const int slot = minor_terrain_definition(static_cast<MinorLandmark>(id)).sprite_slot;
        const int sprite_x = 12 + slot * 128;
        if (slot >= 0 && image.clipRect.right >= sprite_x + 104
            && image.clipRect.bottom >= 60) {
            Sprite_ctor(&minor_sprites[id]);
            // Sprite_extract_2 masks its colour key to 8 bits on entry
            // (confirmed at terranx.exe 0x5E39A0). Thus 0x1FD and 253
            // are equivalent keys here; earlier diagnoses blaming the
            // upper byte were incorrect. Use explicit 253 for clarity.
            // Buffer_load_pcx(..., canvas->palette, ...) may remap the
            // source PCX's transparent index 253 to an active-palette
            // index. The prepared Titanium PCX has a transparent
            // pixel at (sprite_x, 4). Sample it AFTER loading.
            int transparent = 253;
            if (id == MINOR_TITANIUM) {
                transparent = Buffer_get_pixel(&image, sprite_x, 4);
                minor_graphics_trace("Titanium PCX loaded key / source key",
                    transparent, 253, loaded);
            }
            if (transparent < 0 || transparent > 255) {
                minor_graphics_trace("minor sprite invalid transparency key",
                    id, transparent, loaded);
                continue;
            }
            const int result = Sprite_extract_2(
                &minor_sprites[id], &image, transparent,
                sprite_x, 4, 104, 56, nullptr);
            // The sprite extractor trims transparent borders while retaining
            // offset_x/offset_y for the final blit. Requiring width==104
            // wrongly rejected properly cropped artwork.
            minor_sprite_ready[id] = minor_sprites[id].data != nullptr
                && minor_sprites[id].width > 0
                && minor_sprites[id].width <= 104
                && minor_sprites[id].height > 0
                && minor_sprites[id].height <= 56;
            minor_graphics_trace("minor sprite id / extract result / ready",
                id, result, minor_sprite_ready[id]);
        } else {
            minor_graphics_trace("minor sprite missing from atlas", id,
                image.clipRect.right, image.clipRect.bottom);
        }
    }
    Buffer_dtor(&image);
}

/*
External Uranium reference and trial Titanium, loaded in modes 1/3.
Missing/invalid artwork falls back to native Uranium. Both use native
TER1.PCX index decoding and mask processing. Uranium remains checksum-
locked as a known reference; Titanium validates structurally so its
source pixel colours can be revised later without recompiling.
*/
static void load_neca_test_sprites(Buffer* canvas)
{
    if (neca_sprites_attempted || !canvas || !canvas->palette) {
        return;
    }
    neca_sprites_attempted = true;
    const std::string path = minor_graphics_file("neca_sprites.pcx");
    FILE* fp = fopen(path.c_str(), "rb");
    if (!fp) {
        minor_graphics_trace("NECA SPRITE PCX missing / fallback",
            0, 0, GetLastError());
        return;
    }
    fclose(fp);

    Buffer image;
    Buffer_ctor(&image);

    // Exact native TER1.PCX path, verified in the supported terranx.exe:
    // Buffer_load_pcx(..., nullptr, 10, 236), then Buffer_change_color
    // 0x107->0x109 and 0x106->0x109, then Sprite_extract_2 with key 9.
    // PCX loader adds 10 modulo 256 to each decoded 8-bit pixel:
    // raw 253 -> 7, raw 252 -> 6; both are remapped to 9.
    const int loaded = Buffer_load_pcx(&image, path.c_str(), nullptr, 10, 236);
    const int atlas_width = image.clipRect.right;
    const int atlas_height = image.clipRect.bottom;
    minor_graphics_trace("NECA SPRITE atlas result / width / height",
        loaded, atlas_width, atlas_height);

    // Buffer_load_pcx uses 0 for success. Avoid touching an invalid
    // image, and do not try to guess an alternate graphics source.
    if (loaded == 0) {
        bool rectangle_valid = true;
        for (int id = 0; id < NECA_SPRITE_COUNT; ++id) {
            const NecaSpriteDef& def = neca_sprite_defs[id];
            if (def.x < 0 || def.y < 0 || def.width <= 0 || def.height <= 0
                || def.x + def.width > atlas_width
                || def.y + def.height > atlas_height) {
                minor_graphics_trace("NECA SPRITE invalid source rectangle",
                    id, atlas_width, atlas_height);
                rectangle_valid = false;
            }
        }

        if (rectangle_valid) {
            // Verify every source rectangle's raw index-253 background
            // became index 7 after the +10 offset, before remapping to 9.
            for (int id = 0; id < NECA_SPRITE_COUNT; ++id) {
                const NecaSpriteDef& def = neca_sprite_defs[id];
                const int decoded_corner = Buffer_get_pixel(&image, def.x, def.y);
                if (decoded_corner != 7) {
                    minor_graphics_trace("NECA SPRITE unexpected offset corner",
                        id, decoded_corner, 7);
                    rectangle_valid = false;
                }
            }
        }

        if (rectangle_valid) {
            const int change_7 = Buffer_change_color(&image, 0x107, 0x109);
            const int change_6 = change_7 == 0
                ? Buffer_change_color(&image, 0x106, 0x109) : -1;
            minor_graphics_trace("NECA SPRITE remap 7to9 / 6to9",
                change_7, change_6, 0);
            if (change_7 == 0 && change_6 == 0) {
                for (int id = 0; id < NECA_SPRITE_COUNT; ++id) {
                    const NecaSpriteDef& def = neca_sprite_defs[id];
                    const int corner = Buffer_get_pixel(&image, def.x, def.y);
                    int transparent_pixels = 0;
                    int visible_pixels = 0;
                    uint32_t processed_hash = 2166136261u;
                    for (int y = def.y; y < def.y + def.height; ++y) {
                        for (int x = def.x; x < def.x + def.width; ++x) {
                            const int pixel = Buffer_get_pixel(&image, x, y);
                            if (pixel == def.transparent_index) {
                                ++transparent_pixels;
                            } else {
                                ++visible_pixels;
                            }
                            processed_hash = (processed_hash
                                ^ static_cast<uint8_t>(pixel)) * 16777619u;
                        }
                    }

                    // Only the unmodified Uranium control must match the
                    // verified checksum exactly. Titanium is user artwork:
                    // require a nonempty, nonopaque sprite with index-9
                    // background, but merely LOG its checksum for comparison.
                    const bool uranium_reference = id == NECA_SPRITE_URANIUM_TEST;
                    const bool pixels_match = corner == def.transparent_index
                        && transparent_pixels > 0 && visible_pixels > 0
                        && (!uranium_reference || (transparent_pixels == 5635
                            && visible_pixels == 565
                            && processed_hash == 0x24A1B67Au));
                    fp = fopen(minor_graphics_file("minor_terrain_render.log").c_str(), "a");
                    if (fp) {
                        fprintf(fp, "NECA SPRITE processed name=%s "
                            "corner=%d transparent=%d visible=%d "
                            "hash=%08X expected=%08X verified=%d\n",
                            def.name, corner, transparent_pixels, visible_pixels,
                            (unsigned)processed_hash,
                            (unsigned)(uranium_reference ? 0x24A1B67Au : 0u),
                            pixels_match ? 1 : 0);
                        fclose(fp);
                    }
                    if (!pixels_match) {
                        minor_graphics_trace("NECA SPRITE processed mismatch / fallback",
                            id, transparent_pixels, visible_pixels);
                        continue;
                    }

                    Sprite_ctor(&neca_sprites[id]);
                    const int result = Sprite_extract_2(
                        &neca_sprites[id], &image, def.transparent_index,
                        def.x, def.y, def.width, def.height, nullptr);
                    const Sprite& sprite = neca_sprites[id];
                    neca_sprite_ready[id] = result == 0 && sprite.data != nullptr
                        && sprite.width > 0 && sprite.width <= def.width
                        && sprite.height > 0 && sprite.height <= def.height
                        && sprite.color_index[0] == def.transparent_index;
                    fp = fopen(minor_graphics_file("minor_terrain_render.log").c_str(), "a");
                    if (fp) {
                        fprintf(fp, "NECA SPRITE extract name=%s result=%d ready=%d "
                            "sprite=%dx%d offset=%d,%d pitch=%d key=%u\n",
                            def.name, result, neca_sprite_ready[id] ? 1 : 0,
                            sprite.width, sprite.height, sprite.offset_x,
                            sprite.offset_y, sprite.pitch,
                            (unsigned)sprite.color_index[0]);
                        fclose(fp);
                    }
                }
            }
        }
    }
    Buffer_dtor(&image);
}

/*
Select the Develop-only Titanium sprite test at the original native
resource drawing call. Mode 0=original, 1=external Uranium reference,
2=legacy Titanium, 3=external blue-grey Titanium from neca_sprites.pcx.
The original renderer still owns position, scale, clipping and layer.
When substituting, the x86 shim forwards the selected sprite's colour
key; missing assets retain the working native Uranium fallback.
*/
#if defined(__GNUC__) && defined(__i386__)
extern "C" Sprite* __cdecl minor_titanium_select_native_sprite(
    Sprite* original, Buffer* canvas, const int* frame)
{
    if (!minor_native_titanium_sprite_installed || !conf.minor_native_titanium
        || !original || !canvas || !canvas->palette || !frame) {
        return original;
    }

    // Verified from the user's executable: EBP+0x20 and EBP+0x24
    // hold x/y at 0x465E37. EBP+0x0C is NOT Thinker's MapWin pointer;
    // do not reject a valid resource draw using that unrelated argument.
    const int x = frame[8], y = frame[9];
    MAP* tile = mapsq(x, y);
    if (!tile || !minor_landmark_has(x, y, MINOR_TITANIUM)
        || (tile->items & BIT_BONUS_RES)) {
        // Normal icons (including procedural ones) are prioritised by
        // mod_bonus_at before the renderer reaches this type-4 draw branch.
        // The explicit-bit guard remains a safety check for this callback.
        return original;
    }
    const int mode = conf.minor_titanium_sprite_mode;
    Sprite* selected = original;
    // 0x465E2E selects 0x78A700 (variant 0) or 0x78A72C (variant 1).
    // The user atlas currently covers only the first native sprite
    // variant's source geometry. Leave all other variants native.
    const uintptr_t original_address = reinterpret_cast<uintptr_t>(original);
    const int native_variant = original_address == 0x78A700u ? 0
        : original_address == 0x78A72Cu ? 1 : -1;
    if (mode == 1 || mode == 3) {
        if (native_variant == 0 && original->color_index[0] == 9) {
            load_neca_test_sprites(canvas);
            const NecaSpriteId id = mode == 3
                ? NECA_SPRITE_TITANIUM_TEST : NECA_SPRITE_URANIUM_TEST;
            if (neca_sprite_ready[id]) {
                selected = &neca_sprites[id];
            }
        }
    } else if (mode == 2) {
        load_minor_terrain_sprites(canvas);
        if (minor_sprite_ready[MINOR_TITANIUM]) {
            selected = &minor_sprites[MINOR_TITANIUM];
        }
    }
    // The wrapper compares the returned pointer against original. A
    // non-replacement leaves ECX and the original draw key unchanged.
    // Capture the native geometry of each variant independently. Variant 1
    // may first occur well after the initial Titanium draws; an overall
    // log quota would hide the evidence needed to build its atlas entry.
    static int reports_per_variant[3] = {};
    const int report_slot = native_variant == 0 ? 0
        : native_variant == 1 ? 1 : 2;
    if (reports_per_variant[report_slot] < 4) {
        ++reports_per_variant[report_slot];
        minor_graphics_trace(selected != original
                ? "native Titanium sprite substitute"
                : "native Titanium sprite fallback",
            x, y, reports_per_variant[report_slot]);
        const std::string path = minor_graphics_file("minor_terrain_render.log");
        FILE* fp = fopen(path.c_str(), "a");
        if (fp) {
            fprintf(fp, "TITANIUM SPRITE TEST mode=%d variant=%d x=%d y=%d swapped=%d "
                "native=%p key=%u sprite=%dx%d offset=%d,%d pitch=%d "
                "selected=%p key=%u sprite=%dx%d offset=%d,%d pitch=%d\n",
                mode, native_variant, x, y, selected != original ? 1 : 0,
                (void*)original, (unsigned)original->color_index[0],
                original->width, original->height, original->offset_x,
                original->offset_y, original->pitch,
                (void*)selected, (unsigned)selected->color_index[0],
                selected->width, selected->height, selected->offset_x,
                selected->offset_y, selected->pitch);
            if (selected != original) {
                fprintf(fp, "TITANIUM COLOR KEY x=%d y=%d native=%u loaded=%u sprite=%dx%d\n",
                    x, y, (unsigned)original->color_index[0],
                    (unsigned)selected->color_index[0],
                    selected->width, selected->height);
            }
            fclose(fp);
        }
    }
    return selected;
}

// Thiscall native resource sprite: ECX holds Sprite*, [ESP+4] holds
// Buffer*. Preserve original registers/flags and all stack arguments.
// After addl $12 removes all three selector arguments, ESP points to
// the pushal register block. Saved ECX is at +24 (EDX is at +20).
// Native resource drawing passes originalSprite->color_index[0] as a
// separate argument at hook-entry ESP+8. When replacing the sprite,
// forward the newly extracted sprite's *actual loaded* colour index.
// After addl $12, saved ECX is at +24 and entry ESP+8 is at +44.
// When no substitution occurs, both original arguments are untouched.
// No native screen position, zoom or clipping parameters change.
extern "C" __attribute__((naked, used))
void minor_native_titanium_sprite_hook()
{
    __asm__ __volatile__(
        "pushfl\n\t"
        "pushal\n\t"
        "pushl %ebp\n\t"
        "pushl 44(%esp)\n\t"
        "pushl 32(%esp)\n\t"
        "call _minor_titanium_select_native_sprite\n\t"
        "addl $12, %esp\n\t"
        "cmpl 24(%esp), %eax\n\t"
        "je 1f\n\t"
        "movzbl 8(%eax), %edx\n\t"
        "movl %edx, 44(%esp)\n\t"
        "1: movl %eax, 24(%esp)\n\t"
        "popal\n\t"
        "popfl\n\t"
        "jmp *_minor_titanium_native_draw_target\n\t"
    );
}
#endif

/*
REND-005: native Uranium underlay draw at 0x464DE3, ahead of the ordinary
resource icon pass. Never reject BIT_BONUS_RES here: the whole purpose
is to show *both* geology and normal resource art on the same tile.
The original instruction stream references 0x78A72C, but the supported
Windows underlay callback actually receives 0x78A700 (native variant 0):
REND005 log x=67,y=67 cached=3, original=0078a700, expected=0.
Allow either known native Uranium sprite object, while rejecting any other
sprite pointer. The existing Titanium art has already worked on variant 0.
The existing indexed Titanium artwork can be substituted through its own
Sprite* and key; all position, zoom, clip and normal resources stay native.
*/
#if defined(__GNUC__) && defined(__i386__)
extern "C" Sprite* __cdecl minor_select_geology_underlay_sprite(
    Sprite* original, Buffer* canvas, const int* frame)
{
    // On the prior build, all of these silent guards could return native
    // Uranium without reaching the first diagnostic. The mask hook already
    // knows the actual tile at EBP+0x20/+0x24. Use that cached classification
    // for this *earlier* landmark draw rather than copying assumptions from
    // the later resource-sprite call.
    const int type = minor_geology_underlay_current_type;
    const int x = minor_geology_underlay_current_x;
    const int y = minor_geology_underlay_current_y;
    const uintptr_t native_sprite = reinterpret_cast<uintptr_t>(original);
    // Windows evidence (REND005 log 2) shows native 0x78A700 on all
    // tested Titanium underlay calls, with valid frame/canvas and key 9.
    // The earlier static draw-site setup referenced 0x78A72C. Both are
    // the existing engine Uranium Sprite objects; no arbitrary addresses.
    const int native_variant = native_sprite == 0x78A700u ? 0
        : native_sprite == 0x78A72Cu ? 1 : -1;
    const bool valid_original = original && native_variant >= 0;
    const bool valid_canvas = canvas && canvas->palette;
    // Sprite extraction only needs a valid palette-bearing canvas as a
    // readiness guard. The user's main map canvas is the fallback if the
    // native landmark call passes a different Buffer than the resource pass.
    Buffer* loader_canvas = valid_canvas ? canvas : &MapWin->oMainWin.oCanvas;
    const bool loader_ready = loader_canvas && loader_canvas->palette;
    const int mode = conf.minor_titanium_sprite_mode;

    Sprite* selected = original;
    if (minor_native_geology_underlay_installed && conf.minor_native_titanium
        && type == MINOR_TITANIUM && valid_original && loader_ready) {
        if (mode == 1 || mode == 3) {
            load_neca_test_sprites(loader_canvas);
            const NecaSpriteId id = mode == 3
                ? NECA_SPRITE_TITANIUM_TEST : NECA_SPRITE_URANIUM_TEST;
            if (neca_sprite_ready[id]) {
                selected = &neca_sprites[id];
            }
        } else if (mode == 2) {
            load_minor_terrain_sprites(loader_canvas);
            if (minor_sprite_ready[MINOR_TITANIUM]) {
                selected = &minor_sprites[MINOR_TITANIUM];
            }
        }
    }

    // Log selector ENTRY, not just successful substitutions. Bounded
    // separately for Titanium and control draws; no huge per-tile scan.
    static int titanium_reports = 0, control_reports = 0;
    const bool report_titanium = type == MINOR_TITANIUM
        && titanium_reports++ < 12;
    const bool report_control = type != MINOR_TITANIUM
        && control_reports++ < 3;
    if (report_titanium || report_control) {
        FILE* fp = fopen(minor_graphics_file("minor_terrain_render.log").c_str(), "a");
        if (fp) {
            const int frame_x = frame ? frame[8] : -1;
            const int frame_y = frame ? frame[9] : -1;
            fprintf(fp, "REND005 UNDERLAY SELECT cached=%d x=%d y=%d "
                "frame_x=%d frame_y=%d original=%p variant=%d expected=%d canvas=%p "
                "canvas_palette=%d loader_palette=%d enabled=%d mode=%d "
                "atlas_attempted=%d atlas_ready=%d swapped=%d\n",
                type, x, y, frame_x, frame_y, (void*)original,
                native_variant, valid_original ? 1 : 0, (void*)canvas,
                valid_canvas ? 1 : 0, loader_ready ? 1 : 0,
                minor_native_geology_underlay_installed ? 1 : 0, mode,
                neca_sprites_attempted ? 1 : 0,
                neca_sprite_ready[NECA_SPRITE_TITANIUM_TEST] ? 1 : 0,
                selected != original ? 1 : 0);
            if (original && type == MINOR_TITANIUM) {
                fprintf(fp, "REND005 UNDERLAY GEOMETRY native_key=%u "
                    "native=%dx%d offset=%d,%d pitch=%d "
                    "selected_key=%u selected=%dx%d offset=%d,%d pitch=%d\n",
                    (unsigned)original->color_index[0], original->width,
                    original->height, original->offset_x, original->offset_y,
                    original->pitch,
                    (unsigned)selected->color_index[0], selected->width,
                    selected->height, selected->offset_x, selected->offset_y,
                    selected->pitch);
            }
            fclose(fp);
        }
    }
    return selected;
}

// This native __thiscall Sprite_draw_3 has the same ABI as the
// previously tested type-4 resource draw hook: ECX carries Sprite*,
// entry ESP+4 carries Buffer* and entry ESP+8 carries the colour key.
// Only saved ECX and the corresponding saved colour-key argument change.
extern "C" __attribute__((naked, used))
void minor_native_geology_underlay_sprite_hook()
{
    __asm__ __volatile__(
        "pushfl\n\t"
        "pushal\n\t"
        "pushl %ebp\n\t"
        "pushl 44(%esp)\n\t"
        "pushl 32(%esp)\n\t"
        "call _minor_select_geology_underlay_sprite\n\t"
        "addl $12, %esp\n\t"
        "cmpl 24(%esp), %eax\n\t"
        "je 1f\n\t"
        "movzbl 8(%eax), %edx\n\t"
        "movl %edx, 44(%esp)\n\t"
        "1: movl %eax, 24(%esp)\n\t"
        "popal\n\t"
        "popfl\n\t"
        "jmp *_minor_geology_underlay_draw_target\n\t"
    );
}
#endif

// This Draw-only preview uses the game's *already loaded* Monsoon Jungle
// textures, native RainTransition neighbour mask and native polygon blitter.
// It doesn't modify map LM flags or ship texture.pcx-derived artwork.
// Correct drawing order still needs an instruction-verified terrain-stage hook.
static void draw_native_minor_jungle(Console* win, int x, int y)
{
    constexpr uintptr_t kTransitions = 0x685484;
    constexpr uintptr_t kJungleTextures = 0x789C28;
    constexpr uintptr_t kPolyX = 0x685F20;
    constexpr uintptr_t kPolyY = 0x685F50;
    constexpr uintptr_t kPolyCorner = 0x685F80;
    constexpr uintptr_t kPolyUV = 0x686090;
    constexpr uintptr_t kTextureKey = 0x9BB548;

    unsigned mask = 0;
    for (int k = 0; k < 8; ++k) {
        const int dir = (k - 1) & 7;
        const int nx = wrap(x + NearbyTiles[dir][0]);
        const int ny = y + NearbyTiles[dir][1];
        MAP* n = mapsq(nx, ny);
        if (!n || n->alt_level() < ALT_SHORE_LINE) {
            continue;
        }
        const bool native = (n->landmarks & (LM_DISABLE | LM_JUNGLE)) == LM_JUNGLE;
        if (native || minor_landmark_has(nx, ny, MINOR_JUNGLE)) {
            mask |= 1u << k;
        }
    }
    const uint8_t* transitions = reinterpret_cast<const uint8_t*>(kTransitions);
    const unsigned variant = transitions[2 * mask];
    const unsigned rotation = transitions[2 * mask + 1];
    if (variant >= 15 || rotation >= 4) {
        minor_graphics_trace("invalid Jungle variant/rotation",
            x, y, (int)(variant * 10 + rotation));
        return;
    }
    const int* poly_x = reinterpret_cast<const int*>(kPolyX);
    const int* poly_y = reinterpret_cast<const int*>(kPolyY);
    const int* poly_corner = reinterpret_cast<const int*>(kPolyCorner);
    const int (*poly_uv)[2] = reinterpret_cast<const int (*)[2]>(kPolyUV);
    const int zoom_unit = *piZoomNum > 0 && *piZoomDenom > 0
        ? (*piZoomNum * 50 / *piZoomDenom)
        : ((win->ZoomFactor + 16) * 50 / 16);
    const int q = (zoom_unit + 1) / 4;
    const int q2 = 2 * q;
    if (q <= 0) {
        return;
    }
    long px = 0, py = 0;
    MapWin_tile_to_pixel(win, x, y, &px, &py);
    long lift = 0;
    for (int corner = 1; corner <= 4; ++corner) {
        lift += pfncMapGetCornerYOffset(win, x, y, corner);
    }
    lift >>= 2;
    Vert points[8] = {};
    Vert uv[8] = {};
    for (int corner = 0; corner < 4; ++corner) {
        const int i = corner * 3 + 1;
        points[2 * corner] = (int)px + q2 * 2 + poly_x[i] * q2;
        points[2 * corner + 1] = (int)py + (int)lift + q2
            + poly_y[i] * q
            - pfncMapGetCornerYOffset(win, x, y, poly_corner[i]);
        const int j = ((rotation + corner) & 3) * 3 + 1;
        uv[2 * corner] = poly_uv[j][0];
        uv[2 * corner + 1] = poly_uv[j][1];
    }
    Texture* tex = reinterpret_cast<Texture*>(kJungleTextures + variant * 0x70);
    int* texture_key = reinterpret_cast<int*>(kTextureKey);
    const int old_key = *texture_key;
    *texture_key = 9;
    Texture_draw_upper_threshold_trans(tex, &win->oMainWin.oCanvas, points, 4, uv);
    *texture_key = old_key;
}

static void draw_minor_terrain_tile(Console* win, int x, int y)
{
    if (conf.reduced_mode || win != MapWin || !*MapTiles
        || win->cOwner < 0 || win->cOwner >= MaxPlayerNum
        || !minor_landmark_any_at(x, y)) {
        return;
    }
    MAP* tile = mapsq(x, y);
    if (!tile || !(tile->is_known(win->cOwner)
        || (*GameState & STATE_OMNISCIENT_VIEW))
        || tile->is_base() || (tile->items & BIT_VEH_IN_TILE)) {
        return;
    }
    for (const MinorTerrainEntry& entry : minor_landmark_entries_at(x, y)) {
        const MinorLandmark type = static_cast<MinorLandmark>(entry.type);
        if ((type == MINOR_JUNGLE && conf.minor_native_jungle)
            || (type == MINOR_URANIUM && conf.minor_native_uranium)
            || (type == MINOR_TITANIUM && conf.minor_native_titanium)) {
            continue;
        }
        if (type == MINOR_JUNGLE && conf.minor_native_jungle_preview) {
            draw_native_minor_jungle(win, x, y);
            continue;
        }
        Buffer* canvas = &win->oMainWin.oCanvas;
        load_minor_terrain_sprites(canvas);
        if (!minor_sprite_ready[type]) {
            continue;
        }
        long px = 0, py = 0;
        MapWin_tile_to_pixel(win, x, y, &px, &py);
        const int tile_width = win->PixelsPerTileX;
        if (tile_width > 0 && win->PixelsPerTileY > 0) {
            const int draw_width = max(1, tile_width * 3 / 4);
            Sprite_draw_2(&minor_sprites[type], canvas, 253,
                (int)px + (tile_width - draw_width) / 2,
                (int)py + win->PixelsPerTileY / 8, draw_width, 104);
        }
    }
}

/*
Draw minor terrain after the existing map pass in Develop builds. The
tile-inspector path is separate and must not be patched by spoofing LM_*.
*/
static void draw_minor_terrain_map(Console* win)
{
    if (conf.reduced_mode || win != MapWin || !*MapTiles
        || *MapAreaTiles <= 0 || *MapHalfX <= 0) {
        return;
    }
    static bool first_scan = true;
    int counts[MINOR_TYPE_COUNT] = {};
    for (int id = 0; id < *MapAreaTiles; ++id) {
        const int y = id / *MapHalfX;
        const int x = 2 * (id % *MapHalfX) + (y & 1);
        if (minor_landmark_any_at(x, y)) {
            for (const MinorTerrainEntry& entry : minor_landmark_entries_at(x, y)) {
                if (entry.type > MINOR_NONE && entry.type < MINOR_TYPE_COUNT) {
                    ++counts[entry.type];
                }
            }
            draw_minor_terrain_tile(win, x, y);
        }
    }

    if (first_scan) {
        first_scan = false;
        for (int id = MINOR_JUNGLE; id < MINOR_TYPE_COUNT; ++id) {
            minor_graphics_trace("minor terrain scan id / total tiles",
                id, counts[id], *MapAreaTiles);
        }
    }
}
#endif

int __cdecl MapWin_right_menu_arty(int veh_id, int x, int y)
{
    VEH* veh = &Vehs[veh_id];
    return map_range(veh->x, veh->y, x, y) <= arty_range(veh->unit_id);
}

void __thiscall Console_arty_cursor_on(Console* This, int cursor_type, int veh_id)
{
    int veh_range = arty_range(Vehs[veh_id].unit_id);
    Console_cursor_on_2(This, cursor_type, veh_range);
}




