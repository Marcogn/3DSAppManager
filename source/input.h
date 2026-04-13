#pragma once
/* ============================================================
   input.h — input handlers and blocking flow controllers
   ============================================================ */
#include "types.h"

/* Per-frame input handlers (called inside C3D frame) */
void handleMainMenuInput (void);
void handleUninstallInput(void);
void handleSysInfoInput  (void);
void handleSettingsInput (void);

/* Blocking flows (called BEFORE C3D_FrameBegin) */
void buildBackupList         (void);
void runInstallFlow          (void);
void runBackupFlow           (void);
void runUninstallDeleteFlow  (void);
void runSysInfoDetailFlow    (void);

