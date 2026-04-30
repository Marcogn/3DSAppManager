/* ============================================================
   main.c — application entry point and main loop
   ============================================================ */
#include "types.h"
#include "globals.h"
#include "config.h"
#include "titles.h"
#include "draw.h"
#include "input.h"

int main(void) {
    /* Graphics init */
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    top    = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    dynamicBuf = C2D_TextBufNew(4096);

    /* 3DS services */
    amInit();
    fsInit();
    cfguInit();

    /* Config — title list stays empty (lazy load) */
    loadConfig();

    bool running = true;
    while (aptMainLoop() && running) {
        hidScanInput();
        u32 keys = hidKeysDown();

        /* Global exit (main menu only) */
        if (appState == APP_MAIN_MENU && (keys & KEY_START)) {
            running = false;
            break;
        }

        /* --- Pre-frame: lazy-load titles (OUTSIDE C3D frame to avoid nesting) --- */
        if ((appState == APP_UNINSTALL || appState == APP_BACKUP || appState == APP_SYSINFO)
                && (titleCount == 0 || titlesNeedRefresh)) {
            loadTitles(); /* manages its own C3D frames for progress display */
            titlesNeedRefresh = false;
            if (appState == APP_UNINSTALL) {
                updateFilteredList();
                cursor = 0; scrollOffset = 0;
            }
            if (appState == APP_BACKUP) {
                buildBackupList();
                backupCursor = 0; backupScrollOffset = 0;
            }
        }

        /* --- Blocking flows: entered BEFORE opening a C3D frame --- */

        if (appState == APP_INSTALL) {
            runInstallFlow();
            appState = APP_MAIN_MENU;
            continue;
        }
        if (appState == APP_BACKUP) {
            buildBackupList(); /* ensure list reflects current titles */
            runBackupFlow();
            appState = APP_MAIN_MENU;
            continue;
        }
        /* KEY_X uninstall flow: blocking, must run outside C3D frame */
        if (appState == APP_UNINSTALL && (keys & KEY_X)) {
            runUninstallDeleteFlow();
            continue;
        }
        /* SysInfo title detail: blocking (Backup/Restore/Delete), outside C3D frame */
        if (appState == APP_SYSINFO && sysInfoMode != SYSINFO_OVERVIEW
                && sysInfoSubCount > 0 && (keys & KEY_A)) {
            sysInfoDetail.idx    = sysInfoSubIndices[sysInfoSubCursor];
            sysInfoDetail.cursor = 0;
            runSysInfoDetailFlow();
            continue;
        }

        /* --- Frame-by-frame flows --- */
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        switch (appState) {
            case APP_MAIN_MENU:
                handleMainMenuInput();
                drawMainMenu();
                break;
            case APP_UNINSTALL:
                handleUninstallInput();
                if (appState == APP_UNINSTALL) {
                    drawUI();
                } else {
                    /* handleUninstallInput set appState=APP_MAIN_MENU (KEY_B) */
                    drawMainMenu();
                }
                break;
            case APP_SYSINFO:
                handleSysInfoInput();
                if (appState == APP_SYSINFO) drawSysInfoScreen();
                else drawMainMenu();
                break;
            case APP_SETTINGS:
                handleSettingsInput();
                if (appState == APP_SETTINGS) drawSettingsScreen();
                else drawMainMenu();
                break;
            default:
                drawMainMenu();
                break;
        }
        /* SELECT help overlay — baked into current frame at higher z-depth */
        {
            u32 heldKeys = hidKeysHeld();
            if (heldKeys & KEY_SELECT) {
                showingHelpOverlay = true;
                drawHelpOverlay(appState);
            } else {
                showingHelpOverlay = false;
            }
        }
        C3D_FrameEnd(0);
    }

    /* Cleanup */
    C2D_TextBufDelete(dynamicBuf);
    C2D_Fini();
    C3D_Fini();
    cfguExit();
    fsExit();
    amExit();
    gfxExit();
    return 0;
}

