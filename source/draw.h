#pragma once
/* ============================================================
   draw.h — all drawing/rendering functions
   ============================================================ */
#include "types.h"

/* Loading/progress screens (manage own C3D frames) */
void drawLoadingScreen        (int current, int total, const char *status);
void drawInstallProgressScreen(const char *ciaPath, long done, long total);

/* Dialog screens (manage own C3D frames) */
void drawDialog                  (const char **lines, int lineCount);
void drawSelectedTitlesList      (void);
void drawDialogWithSelectedList  (const char **lines, int lineCount);

/* Main loop screens (NO C3D frame management) */
void drawMainMenu         (void);
void drawUI               (void);
void drawTouchControls    (void);
void drawFileBrowserScreen(void);
void drawBackupScreen     (void);
void drawSysInfoScreen    (void);
void drawSettingsScreen   (void);
void drawTitleDetails     (void);
void drawHelpOverlay      (AppState state);

