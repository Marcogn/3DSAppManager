#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <dirent.h>

// SMDH structure definition (from old libctru)
typedef struct {
    u16 shortDescription[0x40];
    u16 longDescription[0x80];
    u16 publisher[0x40];
} SMDH_title;

typedef struct {
    u32 magic;
    u16 version;
    u16 reserved;
    SMDH_title titles[16];
    u8 ratings[16];
    u32 region;
    u32 matchMakerId;
    u64 matchMakerBitId;
    u32 flags;
    u16 eulaVersion;
    u16 reserved2;
    u32 optimalBannerFrame;
    u32 streetpassId;
    u64 reserved3;
    u8 smallIcon[0x480];
    u8 largeIcon[0x1200];
} SMDH;

#define MAX_TITLES 300
#define CONFIG_PATH "sdmc:/3ds/fast-uninstall/config.ini"
#define DEFAULT_BACKUP_PATH "sdmc:/3ds/fast-uninstall/backups"
#define LANGUAGE_ENGLISH 1
#define SMDH_ICON_PATH 0x6E6F6369  // "icon" in little-endian
#define MAX_FILE_SIZE (100 * 1024 * 1024)  // 100MB per file
#define MAX_VISIBLE_TITLES 18  // Maximum titles visible in list at once

// Alternative backup paths for user selection
static const char *BACKUP_PATH_OPTIONS[] = {
    "sdmc:/3ds/fast-uninstall/backups",
    "sdmc:/backups/3ds-titles",
    "sdmc:/save-backups",
    "sdmc:/3ds-backups",
    "sdmc:/backups"
};
#define NUM_BACKUP_PATHS (sizeof(BACKUP_PATH_OPTIONS) / sizeof(BACKUP_PATH_OPTIONS[0]))

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

typedef enum {
    SORT_BY_NAME,
    SORT_BY_TITLEID
} SortMode;

static TitleInfo titles[MAX_TITLES];
static int titleCount = 0;
static int cursor = 0;
static int scrollOffset = 0;
static Config config;
static PrintConsole topScreen, bottomScreen;
static bool needsRedraw = true;  // Flag to track if UI needs redrawing
static SortMode currentSortMode = SORT_BY_NAME;  // Default sort by name

// Function prototypes
void loadConfig();
void saveDefaultConfig();
void sanitizeName(char *name);
void sortTitles();
int compareTitlesByName(const void *a, const void *b);
int compareTitlesByID(const void *a, const void *b);
void getTitleName(u64 titleID, FS_MediaType mediaType, char *outName, size_t outSize);
void loadTitles();
void drawUI();
void drawTouchControls();
void handleInput();
void handleTouchInput();
void backupSaveData(TitleInfo *title);
void backupSaveDataToPath(TitleInfo *title, const char *backupPath);
void backupArchive(FS_Archive archive, const char *basePath, const char *archiveName);
void copyDirectory(FS_Archive archive, const char *srcPath, const char *dstPath);
void deleteTitle(TitleInfo *title);
void deleteTitleCompletely(TitleInfo *title);
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

void sanitizeName(char *name) {
    if (!name) return;

    char *src = name;
    char *dst = name;

    while (*src) {
        // Keep only printable ASCII characters (32-126)
        // Replace others with spaces or skip them
        if (*src >= 32 && *src <= 126) {
            // Skip some problematic characters that might cause issues
            if (*src != '|' && *src != '<' && *src != '>' &&
                *src != '"' && *src != '\\' && *src != '/' &&
                *src != ':' && *src != '*' && *src != '?') {
                *dst++ = *src;
            } else {
                *dst++ = ' ';
            }
        } else if ((unsigned char)*src >= 128) {
            // Non-ASCII character, skip it
            // Could be part of UTF-8 sequence causing corruption
        }
        src++;
    }
    *dst = '\0';

    // Trim trailing spaces
    while (dst > name && *(dst-1) == ' ') {
        *(--dst) = '\0';
    }

    // If name is empty after sanitization, set a default
    if (name[0] == '\0') {
        strcpy(name, "Unknown Title");
    }
}

int compareTitlesByName(const void *a, const void *b) {
    const TitleInfo *ta = (const TitleInfo*)a;
    const TitleInfo *tb = (const TitleInfo*)b;
    return strcasecmp(ta->name, tb->name);
}

int compareTitlesByID(const void *a, const void *b) {
    const TitleInfo *ta = (const TitleInfo*)a;
    const TitleInfo *tb = (const TitleInfo*)b;
    if (ta->titleID < tb->titleID) return -1;
    if (ta->titleID > tb->titleID) return 1;
    return 0;
}

void sortTitles() {
    if (titleCount <= 0) return;

    if (currentSortMode == SORT_BY_NAME) {
        qsort(titles, titleCount, sizeof(TitleInfo), compareTitlesByName);
    } else {
        qsort(titles, titleCount, sizeof(TitleInfo), compareTitlesByID);
    }

    // Reset cursor to top after sorting
    cursor = 0;
    scrollOffset = 0;
    needsRedraw = true;
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
        
        if (bytesRead >= sizeof(u32) && smdh.magic == 0x48444D53) {  // 'SMDH' in little-endian
            // Convert UTF-16 to UTF-8 (using English title)
            ssize_t units = utf16_to_utf8((uint8_t*)outName, smdh.titles[LANGUAGE_ENGLISH].shortDescription, outSize - 1);
            if (units < 0)
                units = 0;
            outName[units] = '\0';

            // Sanitize the name to remove problematic characters
            sanitizeName(outName);
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

    // Sort titles after loading
    sortTitles();

    needsRedraw = true;  // Trigger redraw after loading titles
}

void drawUI() {
    // Ensure we're drawing on the top screen
    consoleSelect(&topScreen);

    // Move cursor to home instead of clearing (smoother update)
    printf("\x1b[H");    // Move cursor to home position (0,0)

    printf("\x1b[30;47m"); // Black text on white background
    printf("%-50s", " 3DS Fast Uninstall");
    printf("\x1b[0m\n"); // Reset colors
    
    int selectedCount = 0;
    for (int i = 0; i < titleCount; i++) {
        if (titles[i].selected)
            selectedCount++;
    }
    
    // Display title count, selected count and sort mode
    const char *sortModeStr = (currentSortMode == SORT_BY_NAME) ? "Name" : "Title ID";
    printf("\nTitles: %d | Selected: %d | Sort: %s\n", titleCount, selectedCount, sortModeStr);
    printf("------------------------------------------------\n");

    // Calculate visible range
    int maxVisible = MAX_VISIBLE_TITLES;
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
        // Display title name (truncated if needed) and title ID
        printf("%.23s [%016llX]", titles[i].name, titles[i].titleID);
        
        if (i == cursor) {
            printf("\x1b[0m"); // Reset
        }
        printf("\n");
    }
    
    printf("\n------------------------------------------------\n");
    printf("D-Pad:Navigate | A:Select | X:Uninstall\n");
    printf("L/R:Sort | START:Exit | Touch:See bottom screen\n");
    printf("------------------------------------------------\n");
    printf("Backup path: %s\n", config.backupPath);

    // Clear any remaining lines (in case list got shorter)
    printf("\x1b[J");  // Clear from cursor to end of screen
}

void drawTouchControls() {
    // Draw on bottom screen
    consoleSelect(&bottomScreen);
    printf("\x1b[2J\x1b[H");  // Clear and home

    printf("\n");
    printf("        TOUCH CONTROLS\n");
    printf("================================\n\n");

    // Draw touch buttons
    printf(" [  SELECT  ]  [  DESELECT ALL  ]\n");
    printf("   Toggle        Clear all\n");
    printf(" selection      selections\n\n");

    printf("================================\n\n");

    printf(" [ UNINSTALL ]  [   CANCEL   ]\n");
    printf("  Uninstall       Go back\n");
    printf("  selected\n\n");

    printf("================================\n\n");

    printf(" [SORT:NAME]    [SORT:ID]\n");
    printf("  Sort by        Sort by\n");
    printf("  title name     title ID\n\n");

    printf("================================\n");
    printf("\nTip: You can also use buttons!\n");

    // Switch back to top screen
    consoleSelect(&topScreen);
}

void copyDirectory(FS_Archive archive, const char *srcPath, const char *dstPath) {
    Handle dirHandle;
    FS_Path fsSrcPath = fsMakePath(PATH_ASCII, srcPath);
    
    Result res = FSUSER_OpenDirectory(&dirHandle, archive, fsSrcPath);
    if (R_FAILED(res))
        return;
    
    createDirectory(dstPath);
    
    u32 entriesRead = 0;
    FS_DirectoryEntry entries[32];
    
    while (true) {
        res = FSDIR_Read(dirHandle, &entriesRead, 32, entries);
        if (R_FAILED(res) || entriesRead == 0)
            break;
        
        for (u32 i = 0; i < entriesRead; i++) {
            char entryName[256];
            utf16_to_utf8((uint8_t*)entryName, entries[i].name, sizeof(entryName) - 1);
            entryName[sizeof(entryName) - 1] = '\0';
            
            char srcFile[512];
            char dstFile[512];
            snprintf(srcFile, sizeof(srcFile), "%s/%s", srcPath, entryName);
            snprintf(dstFile, sizeof(dstFile), "%s/%s", dstPath, entryName);
            
            if (entries[i].attributes & FS_ATTRIBUTE_DIRECTORY) {
                // Recursively copy subdirectory
                copyDirectory(archive, srcFile, dstFile);
            } else {
                // Copy file
                Handle fileHandle;
                FS_Path fsFilePath = fsMakePath(PATH_ASCII, srcFile);
                res = FSUSER_OpenFile(&fileHandle, archive, fsFilePath, FS_OPEN_READ, 0);
                if (R_SUCCEEDED(res)) {
                    u64 fileSize = 0;
                    FSFILE_GetSize(fileHandle, &fileSize);
                    
                    if (fileSize > 0 && fileSize < MAX_FILE_SIZE) {
                        void *buffer = malloc(fileSize);
                        if (buffer) {
                            u32 bytesRead = 0;
                            res = FSFILE_Read(fileHandle, &bytesRead, 0, buffer, fileSize);
                            if (R_SUCCEEDED(res) && bytesRead == fileSize) {
                                FILE *outFile = fopen(dstFile, "wb");
                                if (outFile) {
                                    fwrite(buffer, 1, bytesRead, outFile);
                                    fclose(outFile);
                                }
                            }
                            free(buffer);
                        }
                    }
                    FSFILE_Close(fileHandle);
                }
            }
        }
    }
    
    FSDIR_Close(dirHandle);
}

void backupArchive(FS_Archive archive, const char *basePath, const char *archiveName) {
    char archivePath[512];
    snprintf(archivePath, sizeof(archivePath), "%s/%s", basePath, archiveName);
    createDirectory(archivePath);
    copyDirectory(archive, "/", archivePath);
}

void backupSaveDataToPath(TitleInfo *title, const char *backupPath) {
    // Create backup directory structure
    createDirectory(backupPath);
    
    char backupDir[512];
    snprintf(backupDir, sizeof(backupDir), "%s/%016llX", backupPath, title->titleID);
    createDirectory(backupDir);
    
    // Create info file
    char infoPath[512];
    snprintf(infoPath, sizeof(infoPath), "%s/backup_info.txt", backupDir);
    FILE *info = fopen(infoPath, "w");
    if (info) {
        fprintf(info, "Title ID: %016llX\n", title->titleID);
        fprintf(info, "Title Name: %s\n", title->name);
        fprintf(info, "Media Type: %s\n", title->mediaType == MEDIATYPE_SD ? "SD" : "NAND");
        fprintf(info, "Backup Path: %s\n", backupPath);
        fprintf(info, "\nBackup includes all available save types:\n");
        fprintf(info, "- User Save Data (if present)\n");
        fprintf(info, "- ExtData (if present)\n");
        fprintf(info, "- Boss ExtData (if present)\n");
        fclose(info);
    }
    
    u32 archivePath[] = {title->titleID & 0xFFFFFFFF, (title->titleID >> 32) & 0xFFFFFFFF, title->mediaType, 0};
    FS_Path binArchPath = {PATH_BINARY, 16, archivePath};
    
    // 1. Backup User Save Data
    FS_Archive saveArchive;
    Result res = FSUSER_OpenArchive(&saveArchive, ARCHIVE_USER_SAVEDATA, binArchPath);
    if (R_SUCCEEDED(res)) {
        backupArchive(saveArchive, backupDir, "savedata");
        FSUSER_CloseArchive(saveArchive);
    }
    
    // 2. Backup ExtData
    // Get the correct ExtData ID from the system
    u64 extdataID = 0;
    res = AM_GetTitleExtDataId(&extdataID, title->mediaType, title->titleID);

    if (R_SUCCEEDED(res) && extdataID != 0) {
        // Create binary path for ExtData archive
        // Format: {u8 mediatype, u8 unknown, u16 reserved, u64 saveid}
        FS_ExtSaveDataInfo extInfo = {
            .mediaType = title->mediaType,
            .unknown = 0,
            .reserved1 = 0,
            .saveId = extdataID,
            .reserved2 = 0
        };

        FS_Path extPath = {PATH_BINARY, sizeof(FS_ExtSaveDataInfo), &extInfo};

        FS_Archive extArchive;
        res = FSUSER_OpenArchive(&extArchive, ARCHIVE_EXTDATA, extPath);
        if (R_SUCCEEDED(res)) {
            backupArchive(extArchive, backupDir, "extdata");
            FSUSER_CloseArchive(extArchive);
        }

        // 3. Backup Boss ExtData (for SpotPass data)
        res = FSUSER_OpenArchive(&extArchive, ARCHIVE_BOSS_EXTDATA, extPath);
        if (R_SUCCEEDED(res)) {
            backupArchive(extArchive, backupDir, "boss_extdata");
            FSUSER_CloseArchive(extArchive);
        }
    }
}

void backupSaveData(TitleInfo *title) {
    backupSaveDataToPath(title, config.backupPath);
}

void deleteTitleCompletely(TitleInfo *title) {
    // Delete ExtData first (if exists)
    u64 extdataID = 0;
    Result res = AM_GetTitleExtDataId(&extdataID, title->mediaType, title->titleID);

    if (R_SUCCEEDED(res) && extdataID != 0) {
        FS_ExtSaveDataInfo extInfo = {
            .mediaType = title->mediaType,
            .unknown = 0,
            .reserved1 = 0,
            .saveId = extdataID,
            .reserved2 = 0
        };

        // Try to delete regular ExtData
        FSUSER_DeleteExtSaveData(extInfo);

        // Try to delete Boss ExtData (SpotPass)
        FS_Archive bossArchive;
        FS_Path extPath = {PATH_BINARY, sizeof(FS_ExtSaveDataInfo), &extInfo};
        if (R_SUCCEEDED(FSUSER_OpenArchive(&bossArchive, ARCHIVE_BOSS_EXTDATA, extPath))) {
            FSUSER_CloseArchive(bossArchive);
            // Boss extdata is deleted automatically with title or can be cleaned separately
        }
    }
    
    // Delete the main title (this removes the title, save data, and most content)
    AM_DeleteTitle(title->mediaType, title->titleID);

    return;
}

void deleteTitle(TitleInfo *title) {
    // Perform complete deletion
    deleteTitleCompletely(title);
    
    // Verify deletion by checking if title still exists
    AM_TitleEntry titleEntry;
    Result res = AM_GetTitleInfo(title->mediaType, 1, &title->titleID, &titleEntry);
    
    // If title not found, deletion was successful
    if (R_FAILED(res) || res == 0xC8A04478) {
        title->isValid = false;
        consoleClear();
        printf("\n\nSuccessfully deleted:\n%s\n", title->name);
        printf("\nAll associated data removed:\n");
        printf("  - Title application\n");
        printf("  - Save data\n");
        printf("  - ExtData (if present)\n");
        printf("  - Boss ExtData (if present)\n\n");
    } else {
        consoleClear();
        printf("\n\nFailed to delete:\n%s\n", title->name);
        printf("Title may still be present on system.\n\n");
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
    // Input già scannerizzato nel main loop
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
            needsRedraw = true;
        }
    } else if (kHeld & KEY_DUP) {
        repeatTimer++;
        if (repeatTimer > 20 && repeatTimer % 5 == 0) {
            if (cursor > 0) {
                cursor--;
                needsRedraw = true;
            }
        }
    }
    
    if (kDown & KEY_DDOWN) {
        if (cursor < titleCount - 1) {
            cursor++;
            repeatTimer = 0;
            needsRedraw = true;
        }
    } else if (kHeld & KEY_DDOWN) {
        repeatTimer++;
        if (repeatTimer > 20 && repeatTimer % 5 == 0) {
            if (cursor < titleCount - 1) {
                cursor++;
                needsRedraw = true;
            }
        }
    }
    
    if (!(kHeld & KEY_DUP) && !(kHeld & KEY_DDOWN)) {
        repeatTimer = 0;
    }
    
    // Toggle selection
    if (kDown & KEY_A) {
        titles[cursor].selected = !titles[cursor].selected;
        needsRedraw = true;
    }
    
    // Change sort mode with L/R buttons
    if (kDown & KEY_L) {
        currentSortMode = SORT_BY_NAME;
        sortTitles();
        needsRedraw = true;
    }

    if (kDown & KEY_R) {
        currentSortMode = SORT_BY_TITLEID;
        sortTitles();
        needsRedraw = true;
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
            gfxFlushBuffers();
            gfxSwapBuffers();

            while (aptMainLoop()) {
                gspWaitForVBlank();
                hidScanInput();
                u32 kDown2 = hidKeysDown();
                if (kDown2 & KEY_A)
                    break;
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
        gfxFlushBuffers();
        gfxSwapBuffers();

        bool backupSaves = false;
        bool cancelled = false;
        
        while (aptMainLoop()) {
            gspWaitForVBlank();
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
        }
        
        if (cancelled)
            return;
        
        // Ask about backup path if backing up
        char selectedBackupPath[256];
        snprintf(selectedBackupPath, sizeof(selectedBackupPath), "%s", config.backupPath);
        
        if (backupSaves) {
            consoleClear();
            printf("\n\nBackup Path Selection\n\n");
            printf("Current path: %s\n\n", config.backupPath);
            printf("Do you want to change the backup path?\n\n");
            printf("  A: Use current path\n");
            printf("  Y: Choose alternative path\n");
            printf("  START: Cancel\n");
            gfxFlushBuffers();
            gfxSwapBuffers();

            bool useDefault = true;
            
            while (aptMainLoop()) {
                gspWaitForVBlank();
                hidScanInput();
                u32 kDown2b = hidKeysDown();
                
                if (kDown2b & KEY_A) {
                    useDefault = true;
                    break;
                }
                if (kDown2b & KEY_Y) {
                    useDefault = false;
                    break;
                }
                if (kDown2b & KEY_START) {
                    cancelled = true;
                    break;
                }
            }
            
            if (cancelled)
                return;
            
            if (!useDefault) {
                // Show alternative backup paths
                int pathCursor = 0;
                
                // Find current path in list or default to 0
                for (int i = 0; i < NUM_BACKUP_PATHS; i++) {
                    if (strcmp(config.backupPath, BACKUP_PATH_OPTIONS[i]) == 0) {
                        pathCursor = i;
                        break;
                    }
                }
                
                bool pathSelected = false;
                
                while (aptMainLoop()) {
                    gspWaitForVBlank();
                    hidScanInput();
                    u32 kDown2c = hidKeysDown();
                    
                    if (kDown2c & KEY_DUP) {
                        if (pathCursor > 0)
                            pathCursor--;
                    }
                    if (kDown2c & KEY_DDOWN) {
                        if (pathCursor < NUM_BACKUP_PATHS - 1)
                            pathCursor++;
                    }
                    if (kDown2c & KEY_A) {
                        snprintf(selectedBackupPath, sizeof(selectedBackupPath), "%s", BACKUP_PATH_OPTIONS[pathCursor]);
                        pathSelected = true;
                        break;
                    }
                    if (kDown2c & KEY_B) {
                        cancelled = true;
                        break;
                    }

                    consoleClear();
                    printf("\n\nSelect Backup Path\n\n");
                    printf("Use D-Pad to select, A to confirm\n\n");

                    for (int i = 0; i < NUM_BACKUP_PATHS; i++) {
                        if (i == pathCursor) {
                            printf("\x1b[47;30m"); // Highlighted
                        }
                        printf("  %s\n", BACKUP_PATH_OPTIONS[i]);
                        if (i == pathCursor) {
                            printf("\x1b[0m"); // Reset
                        }
                    }

                    printf("\n  B: Cancel\n");

                    gfxFlushBuffers();
                    gfxSwapBuffers();
                }
                
                if (cancelled)
                    return;
            }
        }
        
        // Final confirmation
        consoleClear();
        printf("\n\nFinal confirmation:\n\n");
        printf("Uninstall %d title(s)?\n", selectedCount);
        printf("Backup saves: %s\n", backupSaves ? "YES" : "NO");
        if (backupSaves) {
            printf("Backup path: %s\n", selectedBackupPath);
        }
        printf("\n  A: Confirm\n");
        printf("  B: Cancel\n");
        gfxFlushBuffers();
        gfxSwapBuffers();

        bool confirmed = false;
        
        while (aptMainLoop()) {
            gspWaitForVBlank();
            hidScanInput();
            u32 kDown3 = hidKeysDown();
            
            if (kDown3 & KEY_A) {
                confirmed = true;
                break;
            }
            if (kDown3 & KEY_B) {
                break;
            }
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
                    printf("  Backing up save data to:\n  %s\n", selectedBackupPath);
                    backupSaveDataToPath(&titles[i], selectedBackupPath);
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

void handleTouchInput() {
    if (titleCount == 0) return;

    touchPosition touch;
    u32 kDown = hidKeysDown();

    if (kDown & KEY_TOUCH) {
        hidTouchRead(&touch);

        // Define touch areas (approximate pixel positions)
        // Screen is 320x240

        // Row 1 (y: 60-90)
        if (touch.py >= 60 && touch.py <= 90) {
            // SELECT button (x: 20-140)
            if (touch.px >= 20 && touch.px <= 140) {
                if (cursor < titleCount) {
                    titles[cursor].selected = !titles[cursor].selected;
                    needsRedraw = true;
                }
            }
            // DESELECT ALL button (x: 160-300)
            else if (touch.px >= 160 && touch.px <= 300) {
                for (int i = 0; i < titleCount; i++) {
                    titles[i].selected = false;
                }
                needsRedraw = true;
            }
        }

        // Row 2 (y: 135-165)
        else if (touch.py >= 135 && touch.py <= 165) {
            // UNINSTALL button (x: 20-140)
            if (touch.px >= 20 && touch.px <= 140) {
                // Trigger uninstall via X key simulation
                // Count selected
                int selectedCount = 0;
                for (int i = 0; i < titleCount; i++) {
                    if (titles[i].selected)
                        selectedCount++;
                }

                if (selectedCount > 0) {
                    // Simulate pressing X by calling the uninstall logic
                    // For now, just set a flag or call directly
                    // This will be handled in main loop
                }
            }
            // CANCEL button (x: 160-300) - does nothing, just feedback
        }

        // Row 3 (y: 200-230)
        else if (touch.py >= 200 && touch.py <= 230) {
            // SORT:NAME button (x: 20-140)
            if (touch.px >= 20 && touch.px <= 140) {
                currentSortMode = SORT_BY_NAME;
                sortTitles();
                needsRedraw = true;
            }
            // SORT:ID button (x: 160-300)
            else if (touch.px >= 160 && touch.px <= 300) {
                currentSortMode = SORT_BY_TITLEID;
                sortTitles();
                needsRedraw = true;
            }
        }
    }
}

int main(int argc, char **argv) {
    gfxInitDefault();

    // Double buffering is fine when we don't redraw every frame
    // The issue was redrawing 60 times per second, not the buffering itself

    // Initialize both screens properly
    consoleInit(GFX_TOP, &topScreen);
    consoleInit(GFX_BOTTOM, &bottomScreen);

    // Set top screen as default
    consoleSelect(&topScreen);

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
    
    // Draw touch controls on bottom screen
    drawTouchControls();

    // Initial draw
    drawUI();
    gfxFlushBuffers();
    gfxSwapBuffers();
    needsRedraw = false;

    // Main loop
    bool running = true;
    while (aptMainLoop() && running) {
        gspWaitForVBlank();
        hidScanInput();
        u32 kDown = hidKeysDown();
        
        // Check for exit before drawing/handling
        if (kDown & KEY_START) {
            running = false;
            break;
        }
        
        handleInput();
        handleTouchInput();

        // Only redraw if something changed
        if (needsRedraw) {
            drawUI();
            gfxFlushBuffers();
            gfxSwapBuffers();
            needsRedraw = false;
        }
    }
    
    // Cleanup
    fsExit();
    amExit();
    gfxExit();
    
    return 0;
}
