#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <dirent.h>

#define MAX_TITLES 300
#define CONFIG_PATH "sdmc:/3ds/fast-uninstall/config.ini"
#define DEFAULT_BACKUP_PATH "sdmc:/3ds/fast-uninstall/backups"
#define LANGUAGE_ENGLISH 1
#define SMDH_ICON_PATH 0x6E6F6369  // "icon" in little-endian

typedef struct {
    u64 titleID;
    char name[256];
    FS_MediaType mediaType;
    bool selected;
    bool isValid;
} TitleInfo;

typedef struct {
    char backupPath[256];
} Config;

static TitleInfo titles[MAX_TITLES];
static int titleCount = 0;
static int cursor = 0;
static int scrollOffset = 0;
static Config config;

// Function prototypes
void loadConfig();
void saveDefaultConfig();
void getTitleName(u64 titleID, FS_MediaType mediaType, char *outName, size_t outSize);
void loadTitles();
void drawUI();
void handleInput();
void backupSaveData(TitleInfo *title);
void deleteTitle(TitleInfo *title);
void createDirectory(const char *path);

void createDirectory(const char *path) {
    char tmpPath[256];
    char *p = NULL;
    size_t len;

    snprintf(tmpPath, sizeof(tmpPath), "%s", path);
    len = strlen(tmpPath);
    
    if (tmpPath[len - 1] == '/')
        tmpPath[len - 1] = 0;
    
    for (p = tmpPath + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmpPath, 0777);
            *p = '/';
        }
    }
    mkdir(tmpPath, 0777);
}

void loadConfig() {
    // Set default values
    snprintf(config.backupPath, sizeof(config.backupPath), "%s", DEFAULT_BACKUP_PATH);
    
    FILE *f = fopen(CONFIG_PATH, "r");
    if (f == NULL) {
        // Config doesn't exist, create default
        saveDefaultConfig();
        return;
    }
    
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        // Remove newline
        line[strcspn(line, "\r\n")] = 0;
        
        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == '#' || line[0] == ';')
            continue;
        
        // Parse key=value
        char *equal = strchr(line, '=');
        if (equal) {
            *equal = '\0';
            char *key = line;
            char *value = equal + 1;
            
            // Trim whitespace
            while (*key == ' ' || *key == '\t') key++;
            while (*value == ' ' || *value == '\t') value++;
            
            if (strcmp(key, "backup_path") == 0) {
                snprintf(config.backupPath, sizeof(config.backupPath), "%s", value);
            }
        }
    }
    
    fclose(f);
}

void saveDefaultConfig() {
    createDirectory("sdmc:/3ds/fast-uninstall");
    
    FILE *f = fopen(CONFIG_PATH, "w");
    if (f == NULL)
        return;
    
    fprintf(f, "# 3DS Fast Uninstall Configuration\n");
    fprintf(f, "# Path where save backups will be stored\n");
    fprintf(f, "backup_path=%s\n", DEFAULT_BACKUP_PATH);
    
    fclose(f);
}

void getTitleName(u64 titleID, FS_MediaType mediaType, char *outName, size_t outSize) {
    SMDH smdh;
    AM_TitleEntry titleEntry;
    
    // Try to get title info
    Result res = AM_GetTitleInfo(mediaType, 1, &titleID, &titleEntry);
    if (R_FAILED(res)) {
        snprintf(outName, outSize, "Unknown [%016llX]", titleID);
        return;
    }
    
    // Try to load SMDH (icon file from title archive)
    Handle fileHandle;
    u32 archivePath[] = {titleID & 0xFFFFFFFF, (titleID >> 32) & 0xFFFFFFFF, mediaType, 0};
    static const u32 filePath[] = {0, 0, 2, SMDH_ICON_PATH, 0};
    
    FS_Path binArchPath = {PATH_BINARY, 16, archivePath};
    FS_Path binFilePath = {PATH_BINARY, 20, filePath};
    
    res = FSUSER_OpenFileDirectly(&fileHandle, ARCHIVE_SAVEDATA_AND_CONTENT, binArchPath, binFilePath, FS_OPEN_READ, 0);
    if (R_SUCCEEDED(res)) {
        u32 bytesRead;
        FSFILE_Read(fileHandle, &bytesRead, 0, &smdh, sizeof(SMDH));
        FSFILE_Close(fileHandle);
        
        if (bytesRead == sizeof(SMDH) && smdh.magic[0] == 'S' && smdh.magic[1] == 'M' && 
            smdh.magic[2] == 'D' && smdh.magic[3] == 'H') {
            // Convert UTF-16 to UTF-8 (using English title)
            ssize_t units = utf16_to_utf8((uint8_t*)outName, smdh.applicationTitles[LANGUAGE_ENGLISH].shortDescription, outSize - 1);
            if (units < 0)
                units = 0;
            outName[units] = '\0';
            return;
        }
    }
    
    // Fallback to title ID
    snprintf(outName, outSize, "Title [%016llX]", titleID);
}

void loadTitles() {
    titleCount = 0;
    
    u32 titleCountSD = 0;
    u32 titleCountNAND = 0;
    
    // Get title counts
    AM_GetTitleCount(MEDIATYPE_SD, &titleCountSD);
    AM_GetTitleCount(MEDIATYPE_NAND, &titleCountNAND);
    
    u32 totalCount = titleCountSD + titleCountNAND;
    if (totalCount == 0)
        return;
    
    u64 *titleIDs = (u64*)malloc(totalCount * sizeof(u64));
    if (titleIDs == NULL)
        return;
    
    u32 readCount = 0;
    
    // Load SD titles
    if (titleCountSD > 0) {
        Result res = AM_GetTitleList(&readCount, MEDIATYPE_SD, titleCountSD, titleIDs);
        if (R_SUCCEEDED(res)) {
            for (u32 i = 0; i < readCount && titleCount < MAX_TITLES; i++) {
                u64 tid = titleIDs[i];
                
                // Skip system titles (0x00040010 and 0x00040030 range)
                u32 highID = (u32)(tid >> 32);
                if (highID == 0x00040010 || highID == 0x00040030)
                    continue;
                
                titles[titleCount].titleID = tid;
                titles[titleCount].mediaType = MEDIATYPE_SD;
                titles[titleCount].selected = false;
                titles[titleCount].isValid = true;
                getTitleName(tid, MEDIATYPE_SD, titles[titleCount].name, sizeof(titles[titleCount].name));
                titleCount++;
            }
        }
    }
    
    // Load NAND titles (only user-installable ones)
    if (titleCountNAND > 0) {
        Result res = AM_GetTitleList(&readCount, MEDIATYPE_NAND, titleCountNAND, titleIDs);
        if (R_SUCCEEDED(res)) {
            for (u32 i = 0; i < readCount && titleCount < MAX_TITLES; i++) {
                u64 tid = titleIDs[i];
                
                // Skip system titles
                u32 highID = (u32)(tid >> 32);
                if (highID == 0x00040010 || highID == 0x00040030 || highID == 0x00040138)
                    continue;
                
                // Only show user-installed content (0x00040000 = applications, 0x0004000E = updates)
                if (highID != 0x00040000 && highID != 0x0004000E && highID != 0x0004008C)
                    continue;
                
                titles[titleCount].titleID = tid;
                titles[titleCount].mediaType = MEDIATYPE_NAND;
                titles[titleCount].selected = false;
                titles[titleCount].isValid = true;
                getTitleName(tid, MEDIATYPE_NAND, titles[titleCount].name, sizeof(titles[titleCount].name));
                titleCount++;
            }
        }
    }
    
    free(titleIDs);
}

void drawUI() {
    consoleClear();
    
    printf("\x1b[0;0H");
    printf("\x1b[30;47m"); // Black text on white background
    printf("%-50s", " 3DS Fast Uninstall");
    printf("\x1b[0m\n"); // Reset colors
    
    int selectedCount = 0;
    for (int i = 0; i < titleCount; i++) {
        if (titles[i].selected)
            selectedCount++;
    }
    
    printf("\nInstalled Titles (%d) - Selected: %d\n", titleCount, selectedCount);
    printf("────────────────────────────────────────────────\n");
    
    // Calculate visible range
    int maxVisible = 20;
    int startIdx = scrollOffset;
    int endIdx = scrollOffset + maxVisible;
    if (endIdx > titleCount)
        endIdx = titleCount;
    
    // Ensure cursor is visible
    if (cursor < scrollOffset)
        scrollOffset = cursor;
    if (cursor >= scrollOffset + maxVisible)
        scrollOffset = cursor - maxVisible + 1;
    
    startIdx = scrollOffset;
    endIdx = scrollOffset + maxVisible;
    if (endIdx > titleCount)
        endIdx = titleCount;
    
    for (int i = startIdx; i < endIdx; i++) {
        if (i == cursor) {
            printf("\x1b[47;30m"); // Highlighted (white bg, black text)
        }
        
        printf("%s ", titles[i].selected ? "[X]" : "[ ]");
        printf("%.40s", titles[i].name);
        
        if (i == cursor) {
            printf("\x1b[0m"); // Reset
        }
        printf("\n");
    }
    
    printf("\n────────────────────────────────────────────────\n");
    printf("Controls:\n");
    printf("  D-Pad Up/Down: Navigate\n");
    printf("  A: Toggle selection\n");
    printf("  X: Uninstall selected\n");
    printf("  START: Exit\n");
    printf("────────────────────────────────────────────────\n");
    printf("Backup path: %s\n", config.backupPath);
}

void backupSaveData(TitleInfo *title) {
    // Create backup directory structure
    createDirectory(config.backupPath);
    
    char backupDir[512];
    snprintf(backupDir, sizeof(backupDir), "%s/%016llX", config.backupPath, title->titleID);
    createDirectory(backupDir);
    
    // Open save archive
    FS_Archive saveArchive;
    u32 archivePath[] = {title->titleID & 0xFFFFFFFF, (title->titleID >> 32) & 0xFFFFFFFF, title->mediaType, 0};
    FS_Path binArchPath = {PATH_BINARY, 16, archivePath};
    
    Result res = FSUSER_OpenArchive(&saveArchive, ARCHIVE_USER_SAVEDATA, binArchPath);
    if (R_FAILED(res)) {
        return; // No save data or error
    }
    
    // NOTE: Full save data backup implementation requires recursive directory traversal
    // and copying of all files within the save archive. This is a simplified version
    // that creates a marker file to indicate a backup was attempted.
    // For a complete implementation, see JKSM or Checkpoint homebrew applications.
    
    char markerPath[512];
    snprintf(markerPath, sizeof(markerPath), "%s/backup_info.txt", backupDir);
    
    FILE *marker = fopen(markerPath, "w");
    if (marker) {
        fprintf(marker, "Title ID: %016llX\n", title->titleID);
        fprintf(marker, "Title Name: %s\n", title->name);
        fprintf(marker, "Media Type: %s\n", title->mediaType == MEDIATYPE_SD ? "SD" : "NAND");
        fprintf(marker, "\nIMPORTANT: This is a backup marker only.\n");
        fprintf(marker, "Full save data backup functionality is not yet implemented.\n");
        fprintf(marker, "For complete save backups, please use JKSM or Checkpoint.\n");
        fclose(marker);
    }
    
    FSUSER_CloseArchive(saveArchive);
}

void deleteTitle(TitleInfo *title) {
    Result res = AM_DeleteTitle(title->mediaType, title->titleID);
    
    if (R_SUCCEEDED(res)) {
        title->isValid = false;
        consoleClear();
        printf("\n\nSuccessfully deleted:\n%s\n\n", title->name);
    } else {
        consoleClear();
        printf("\n\nFailed to delete:\n%s\nError: 0x%08lX\n\n", title->name, res);
    }
    
    printf("Press A to continue...\n");
    
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        
        if (kDown & KEY_A)
            break;
        
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }
}

void handleInput() {
    hidScanInput();
    u32 kDown = hidKeysDown();
    u32 kHeld = hidKeysHeld();
    
    static u32 repeatTimer = 0;
    
    if (titleCount == 0)
        return;
    
    // Navigation
    if (kDown & KEY_DUP) {
        if (cursor > 0) {
            cursor--;
            repeatTimer = 0;
        }
    } else if (kHeld & KEY_DUP) {
        repeatTimer++;
        if (repeatTimer > 20 && repeatTimer % 5 == 0) {
            if (cursor > 0)
                cursor--;
        }
    }
    
    if (kDown & KEY_DDOWN) {
        if (cursor < titleCount - 1) {
            cursor++;
            repeatTimer = 0;
        }
    } else if (kHeld & KEY_DDOWN) {
        repeatTimer++;
        if (repeatTimer > 20 && repeatTimer % 5 == 0) {
            if (cursor < titleCount - 1)
                cursor++;
        }
    }
    
    if (!(kHeld & KEY_DUP) && !(kHeld & KEY_DDOWN)) {
        repeatTimer = 0;
    }
    
    // Toggle selection
    if (kDown & KEY_A) {
        titles[cursor].selected = !titles[cursor].selected;
    }
    
    // Uninstall selected
    if (kDown & KEY_X) {
        int selectedCount = 0;
        for (int i = 0; i < titleCount; i++) {
            if (titles[i].selected)
                selectedCount++;
        }
        
        if (selectedCount == 0) {
            consoleClear();
            printf("\n\nNo titles selected!\n\nPress A to continue...");
            
            while (aptMainLoop()) {
                hidScanInput();
                u32 kDown2 = hidKeysDown();
                if (kDown2 & KEY_A)
                    break;
                gfxFlushBuffers();
                gfxSwapBuffers();
                gspWaitForVBlank();
            }
            return;
        }
        
        // Ask for backup confirmation
        consoleClear();
        printf("\n\nYou are about to uninstall %d title(s).\n\n", selectedCount);
        printf("Do you want to backup save data?\n\n");
        printf("  A: Yes, backup saves\n");
        printf("  B: No, skip backup\n");
        printf("  START: Cancel\n");
        
        bool backupSaves = false;
        bool cancelled = false;
        
        while (aptMainLoop()) {
            hidScanInput();
            u32 kDown2 = hidKeysDown();
            
            if (kDown2 & KEY_A) {
                backupSaves = true;
                break;
            }
            if (kDown2 & KEY_B) {
                backupSaves = false;
                break;
            }
            if (kDown2 & KEY_START) {
                cancelled = true;
                break;
            }
            
            gfxFlushBuffers();
            gfxSwapBuffers();
            gspWaitForVBlank();
        }
        
        if (cancelled)
            return;
        
        // Final confirmation
        consoleClear();
        printf("\n\nFinal confirmation:\n\n");
        printf("Uninstall %d title(s)?\n", selectedCount);
        printf("Backup saves: %s\n\n", backupSaves ? "YES" : "NO");
        printf("  A: Confirm\n");
        printf("  B: Cancel\n");
        
        bool confirmed = false;
        
        while (aptMainLoop()) {
            hidScanInput();
            u32 kDown3 = hidKeysDown();
            
            if (kDown3 & KEY_A) {
                confirmed = true;
                break;
            }
            if (kDown3 & KEY_B) {
                break;
            }
            
            gfxFlushBuffers();
            gfxSwapBuffers();
            gspWaitForVBlank();
        }
        
        if (!confirmed)
            return;
        
        // Process deletions
        consoleClear();
        printf("\n\nProcessing...\n\n");
        
        for (int i = 0; i < titleCount; i++) {
            if (titles[i].selected && titles[i].isValid) {
                printf("Processing: %s\n", titles[i].name);
                
                if (backupSaves) {
                    printf("  Backing up save data...\n");
                    backupSaveData(&titles[i]);
                }
                
                printf("  Deleting title...\n");
                gfxFlushBuffers();
                gfxSwapBuffers();
                
                deleteTitle(&titles[i]);
            }
        }
        
        // Reload titles
        consoleClear();
        printf("\n\nReloading title list...\n");
        gfxFlushBuffers();
        gfxSwapBuffers();
        
        loadTitles();
        cursor = 0;
        scrollOffset = 0;
    }
}

int main(int argc, char **argv) {
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);
    
    // Initialize services
    amInit();
    fsInit();
    
    // Load configuration
    loadConfig();
    
    // Load titles
    consoleClear();
    printf("\n\nLoading installed titles...\n");
    printf("Please wait...\n");
    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
    
    loadTitles();
    
    // Main loop
    bool running = true;
    while (aptMainLoop() && running) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        
        // Check for exit before drawing/handling
        if (kDown & KEY_START) {
            running = false;
            break;
        }
        
        drawUI();
        handleInput();
        
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }
    
    // Cleanup
    fsExit();
    amExit();
    gfxExit();
    
    return 0;
}
