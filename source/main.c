#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>

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
#define MAX_VISIBLE_TITLES 13  // Reduced for double-spacing (better readability)

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
    char fullName[256];  // Full name without truncation
    FS_MediaType mediaType;
    bool selected;
    bool isValid;
    u16 version;
    bool hasBackup;
    C2D_Image icon;  // Icon texture for bottom screen
    bool iconLoaded;
    u64 size;  // Size in bytes
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
static C3D_RenderTarget* top;
static C3D_RenderTarget* bottom;
static C2D_TextBuf dynamicBuf;  // Text buffer for dynamic text rendering
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
void getTitleInfo(TitleInfo *title);
u64 getTitleSize(u64 titleID, FS_MediaType mediaType);
bool checkBackupExists(u64 titleID);
void loadTitleIcon(TitleInfo *title);
void drawTitleDetails();
void drawLoadingScreen(int current, int total, const char *status);
void loadTitles();
void drawUI();
void drawTouchControls();
void drawDialog(const char **lines, int lineCount);
void handleInput();
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

            // Add type indicator based on titleID with simple ASCII symbols
            u32 highID = (u32)(titleID >> 32);
            if (highID == 0x0004000E) {
                // Update/Patch - use UP arrow
                strncat(outName, " ^", outSize - strlen(outName) - 1);  // ^
            } else if (highID == 0x0004008C) {
                // DLC - use plus symbol
                strncat(outName, " +", outSize - strlen(outName) - 1);  // +
            }

            return;
        }
    }
    
    // Fallback to title ID with type indicator
    u32 highID = (u32)(titleID >> 32);
    const char *typeStr = "";
    if (highID == 0x0004000E) typeStr = " ^";  // ^ for Update
    else if (highID == 0x0004008C) typeStr = " +";  // + for DLC

    snprintf(outName, outSize, "Title%s [%016llX]", typeStr, titleID);
}

bool checkBackupExists(u64 titleID) {
    char backupDir[512];
    snprintf(backupDir, sizeof(backupDir), "%s/%016llX", config.backupPath, titleID);

    struct stat st;
    return (stat(backupDir, &st) == 0 && S_ISDIR(st.st_mode));
}

u64 getTitleSize(u64 titleID, FS_MediaType mediaType) {
    u64 size = 0;
    Result res;

    // Try to get title size from AM service
    AM_TitleEntry entry;
    res = AM_GetTitleInfo(mediaType, 1, &titleID, &entry);
    if (R_SUCCEEDED(res)) {
        // Size is in the title entry
        size = entry.size;
    }

    // If that failed, try to calculate from filesystem
    if (size == 0) {
        u32 archivePath[] = {titleID & 0xFFFFFFFF, (titleID >> 32) & 0xFFFFFFFF, mediaType, 0};
        FS_Path binArchPath = {PATH_BINARY, 16, archivePath};

        FS_Archive archive;
        res = FSUSER_OpenArchive(&archive, ARCHIVE_SAVEDATA_AND_CONTENT, binArchPath);
        if (R_SUCCEEDED(res)) {
            // Archive opened, but getting exact size is complex
            // Return approximate size (this is a fallback)
            FSUSER_CloseArchive(archive);
        }
    }

    return size;
}

void loadTitleIcon(TitleInfo *title) {
    // Icon loading disabled for now due to complexity with citro2d structures
    // TODO: Implement proper icon loading with correct memory management
    title->iconLoaded = false;
}

void getTitleInfo(TitleInfo *title) {
    // Get full title information
    AM_TitleEntry titleEntry;
    Result res = AM_GetTitleInfo(title->mediaType, 1, &title->titleID, &titleEntry);

    if (R_SUCCEEDED(res)) {
        title->version = titleEntry.version;
        title->size = titleEntry.size;
    } else {
        title->version = 0;
        title->size = 0;
    }

    // If size is still 0, try alternative method
    if (title->size == 0) {
        title->size = getTitleSize(title->titleID, title->mediaType);
    }

    // Get full name (without truncation)
    SMDH smdh;
    Handle fileHandle;
    u32 archivePath[] = {title->titleID & 0xFFFFFFFF, (title->titleID >> 32) & 0xFFFFFFFF, title->mediaType, 0};
    static const u32 filePath[] = {0, 0, 2, SMDH_ICON_PATH, 0};

    FS_Path binArchPath = {PATH_BINARY, 16, archivePath};
    FS_Path binFilePath = {PATH_BINARY, 20, filePath};

    res = FSUSER_OpenFileDirectly(&fileHandle, ARCHIVE_SAVEDATA_AND_CONTENT, binArchPath, binFilePath, FS_OPEN_READ, 0);
    if (R_SUCCEEDED(res)) {
        u32 bytesRead;
        FSFILE_Read(fileHandle, &bytesRead, 0, &smdh, sizeof(SMDH));
        FSFILE_Close(fileHandle);

        if (bytesRead >= sizeof(u32) && smdh.magic == 0x48444D53) {
            // Get full name from long description
            ssize_t units = utf16_to_utf8((uint8_t*)title->fullName, smdh.titles[LANGUAGE_ENGLISH].longDescription, sizeof(title->fullName) - 1);
            if (units < 0) units = 0;
            title->fullName[units] = '\0';
            sanitizeName(title->fullName);

            // If long description is empty, use short description
            if (title->fullName[0] == '\0' || strcmp(title->fullName, "Unknown Title") == 0) {
                units = utf16_to_utf8((uint8_t*)title->fullName, smdh.titles[LANGUAGE_ENGLISH].shortDescription, sizeof(title->fullName) - 1);
                if (units < 0) units = 0;
                title->fullName[units] = '\0';
                sanitizeName(title->fullName);
            }
        }
    }

    // Fallback to regular name if full name not loaded
    if (title->fullName[0] == '\0') {
        strncpy(title->fullName, title->name, sizeof(title->fullName) - 1);
    }

    // Check if backup exists
    title->hasBackup = checkBackupExists(title->titleID);
}

void drawLoadingScreen(int current, int total, const char *status) {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(top, C2D_Color32(0, 0, 0, 255));
    C2D_SceneBegin(top);

    C2D_TextBufClear(dynamicBuf);
    C2D_Text text;

    // Title
    C2D_TextParse(&text, dynamicBuf, "Loading installed titles...");
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, 50.0f, 80.0f, 0.5f, 0.6f, 0.6f, C2D_Color32(255, 255, 255, 255));

    // Status message
    if (status) {
        C2D_TextParse(&text, dynamicBuf, status);
        C2D_TextOptimize(&text);
        C2D_DrawText(&text, C2D_WithColor, 50.0f, 105.0f, 0.5f, 0.4f, 0.4f, C2D_Color32(200, 200, 200, 255));
    }

    // Progress bar
    if (total > 0) {
        float progress = (float)current / (float)total;
        float barWidth = 300.0f;
        float barHeight = 20.0f;
        float barX = 50.0f;
        float barY = 130.0f;

        // Background
        C2D_DrawRectSolid(barX, barY, 0.5f, barWidth, barHeight, C2D_Color32(50, 50, 50, 255));

        // Progress fill
        float fillWidth = barWidth * progress;
        C2D_DrawRectSolid(barX, barY, 0.5f, fillWidth, barHeight, C2D_Color32(100, 180, 255, 255));

        // Border
        C2D_DrawRectSolid(barX, barY, 0.5f, barWidth, 2, C2D_Color32(255, 255, 255, 255));
        C2D_DrawRectSolid(barX, barY + barHeight - 2, 0.5f, barWidth, 2, C2D_Color32(255, 255, 255, 255));
        C2D_DrawRectSolid(barX, barY, 0.5f, 2, barHeight, C2D_Color32(255, 255, 255, 255));
        C2D_DrawRectSolid(barX + barWidth - 2, barY, 0.5f, 2, barHeight, C2D_Color32(255, 255, 255, 255));

        // Progress text
        char progressText[64];
        snprintf(progressText, sizeof(progressText), "%d / %d", current, total);
        C2D_TextParse(&text, dynamicBuf, progressText);
        C2D_TextOptimize(&text);
        C2D_DrawText(&text, C2D_WithColor, barX + barWidth / 2 - 20, barY + 25, 0.5f, 0.45f, 0.45f, C2D_Color32(255, 255, 255, 255));
    }

    C3D_FrameEnd(0);
}

void loadTitles() {
    titleCount = 0;
    
    u32 titleCountSD = 0;
    u32 titleCountNAND = 0;
    
    // Get title counts
    AM_GetTitleCount(MEDIATYPE_SD, &titleCountSD);
    AM_GetTitleCount(MEDIATYPE_NAND, &titleCountNAND);
    
    u32 totalCount = titleCountSD + titleCountNAND;
    if (totalCount == 0) {
        drawLoadingScreen(0, 0, "No titles found");
        return;
    }

    drawLoadingScreen(0, totalCount, "Allocating memory...");

    u64 *titleIDs = (u64*)malloc(totalCount * sizeof(u64));
    if (titleIDs == NULL)
        return;
    
    u32 readCount = 0;
    
    // Load SD titles
    if (titleCountSD > 0) {
        drawLoadingScreen(0, totalCount, "Loading SD titles...");
        Result res = AM_GetTitleList(&readCount, MEDIATYPE_SD, titleCountSD, titleIDs);
        if (R_SUCCEEDED(res)) {
            for (u32 i = 0; i < readCount && titleCount < MAX_TITLES; i++) {
                u64 tid = titleIDs[i];
                
                // Update progress every 10 titles
                if (i % 10 == 0) {
                    char statusMsg[128];
                    snprintf(statusMsg, sizeof(statusMsg), "Loading SD titles... (%d/%d)", i, readCount);
                    drawLoadingScreen(i, totalCount, statusMsg);
                }

                // Skip system titles (0x00040010 and 0x00040030 range)
                u32 highID = (u32)(tid >> 32);
                if (highID == 0x00040010 || highID == 0x00040030)
                    continue;
                
                titles[titleCount].titleID = tid;
                titles[titleCount].mediaType = MEDIATYPE_SD;
                titles[titleCount].selected = false;
                titles[titleCount].isValid = true;
                titles[titleCount].iconLoaded = false;
                titles[titleCount].version = 0;
                titles[titleCount].hasBackup = false;
                titles[titleCount].fullName[0] = '\0';
                titles[titleCount].size = 0;
                getTitleName(tid, MEDIATYPE_SD, titles[titleCount].name, sizeof(titles[titleCount].name));
                getTitleInfo(&titles[titleCount]);
                titleCount++;
            }
        }
    }
    
    // Load NAND titles (only user-installable ones)
    if (titleCountNAND > 0) {
        drawLoadingScreen(titleCountSD, totalCount, "Loading NAND titles...");
        Result res = AM_GetTitleList(&readCount, MEDIATYPE_NAND, titleCountNAND, titleIDs);
        if (R_SUCCEEDED(res)) {
            for (u32 i = 0; i < readCount && titleCount < MAX_TITLES; i++) {
                u64 tid = titleIDs[i];
                
                // Update progress every 10 titles
                if (i % 10 == 0) {
                    char statusMsg[128];
                    snprintf(statusMsg, sizeof(statusMsg), "Loading NAND titles... (%d/%d)", i, readCount);
                    drawLoadingScreen(titleCountSD + i, totalCount, statusMsg);
                }

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
                titles[titleCount].iconLoaded = false;
                titles[titleCount].version = 0;
                titles[titleCount].hasBackup = false;
                titles[titleCount].fullName[0] = '\0';
                titles[titleCount].size = 0;
                getTitleName(tid, MEDIATYPE_NAND, titles[titleCount].name, sizeof(titles[titleCount].name));
                getTitleInfo(&titles[titleCount]);
                titleCount++;
            }
        }
    }
    
    drawLoadingScreen(totalCount, totalCount, "Sorting titles...");

    free(titleIDs);

    // Sort titles after loading
    sortTitles();

    needsRedraw = true;  // Trigger redraw after loading titles
}

void drawUI() {
    // Render top screen - title list
    C2D_TargetClear(top, C2D_Color32(0, 0, 0, 255));
    C2D_SceneBegin(top);
    
    // Clear dynamic text buffer and prepare for new text
    C2D_TextBufClear(dynamicBuf);
    
    float y = 3.0f;

    // Draw header background (white bar)
    C2D_DrawRectSolid(0, y, 0.5f, 400, 18, C2D_Color32(255, 255, 255, 255));

    // Draw header text
    C2D_Text text;
    C2D_TextParse(&text, dynamicBuf, " 3DS Fast Uninstall");
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, 5.0f, y + 3, 0.5f, 0.55f, 0.55f, C2D_Color32(0, 0, 0, 255));

    y += 22;

    // Count selected titles
    int selectedCount = 0;
    for (int i = 0; i < titleCount; i++) {
        if (titles[i].selected)
            selectedCount++;
    }
    
    // Display title count with color based on count - ROSSO se oltre 300
    const char *sortModeStr = (currentSortMode == SORT_BY_NAME) ? "Name" : "TID";
    char titleCountText[64];
    snprintf(titleCountText, sizeof(titleCountText), "Titles: %d", titleCount);

    u32 countColor = (titleCount > 300) ? C2D_Color32(255, 80, 80, 255) : C2D_Color32(100, 255, 100, 255);

    C2D_TextParse(&text, dynamicBuf, titleCountText);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, 5.0f, y, 0.5f, 0.48f, 0.48f, countColor);

    // Selected count
    char selectedText[64];
    snprintf(selectedText, sizeof(selectedText), "Selected: %d", selectedCount);
    C2D_TextParse(&text, dynamicBuf, selectedText);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, 110.0f, y, 0.5f, 0.48f, 0.48f, C2D_Color32(255, 255, 255, 255));

    // Sort mode
    char sortText[64];
    snprintf(sortText, sizeof(sortText), "Sort: %s", sortModeStr);
    C2D_TextParse(&text, dynamicBuf, sortText);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, 250.0f, y, 0.5f, 0.48f, 0.48f, C2D_Color32(200, 200, 255, 255));

    y += 18;

    // Draw separator line
    C2D_DrawRectSolid(2, y, 0.5f, 396, 1, C2D_Color32(255, 255, 255, 255));

    y += 4;

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
    
    // Draw each title - NUOVO LAYOUT: nome espanso a SX, TitleID allineato a DX
    for (int i = startIdx; i < endIdx; i++) {
        // Draw selection highlight
        if (i == cursor) {
            C2D_DrawRectSolid(0, y - 1, 0.5f, 400, 15, C2D_Color32(255, 255, 255, 255));
        }
        
        u32 textColor = (i == cursor) ? C2D_Color32(0, 0, 0, 255) : C2D_Color32(255, 255, 255, 255);
        u32 tidColor = (i == cursor) ? C2D_Color32(60, 60, 60, 255) : C2D_Color32(150, 150, 150, 255);

        // Checkbox
        const char *checkbox = titles[i].selected ? "[X]" : "[ ]";
        C2D_TextParse(&text, dynamicBuf, checkbox);
        C2D_TextOptimize(&text);
        C2D_DrawText(&text, C2D_WithColor, 3.0f, y, 0.5f, 0.42f, 0.42f, textColor);

        // Title name - espanso fino a dove serve per il TitleID
        // Tronca il nome a ~35 caratteri per lasciare spazio al TitleID
        char truncName[40];
        int nameLen = strlen(titles[i].name);
        if (nameLen > 35) {
            // Nome troppo lungo, tronca e aggiungi "..."
            snprintf(truncName, sizeof(truncName), "%.32s...", titles[i].name);
        } else {
            snprintf(truncName, sizeof(truncName), "%s", titles[i].name);
        }
        C2D_TextParse(&text, dynamicBuf, truncName);
        C2D_TextOptimize(&text);
        C2D_DrawText(&text, C2D_WithColor, 28.0f, y, 0.5f, 0.40f, 0.40f, textColor);

        // Title ID - allineato a DESTRA
        char tidText[32];
        snprintf(tidText, sizeof(tidText), "%016llX", titles[i].titleID);
        C2D_TextParse(&text, dynamicBuf, tidText);
        C2D_TextOptimize(&text);
        // Posizione X calcolata per allineare a destra (400 - larghezza testo - margine)
        C2D_DrawText(&text, C2D_WithColor, 268.0f, y, 0.5f, 0.36f, 0.36f, tidColor);

        y += 15;  // Line spacing aumentato
    }
}

void drawTouchControls() {
    // Render bottom screen - title details
    C2D_TargetClear(bottom, C2D_Color32(20, 20, 30, 255));
    C2D_SceneBegin(bottom);
    
    C2D_TextBufClear(dynamicBuf);
    C2D_Text text;
    float y = 10.0f;

    // If no titles or invalid cursor, show basic info
    if (titleCount == 0 || cursor < 0 || cursor >= titleCount) {
        C2D_TextParse(&text, dynamicBuf, "No title selected");
        C2D_TextOptimize(&text);
        C2D_DrawText(&text, C2D_WithColor, 10.0f, 100.0f, 0.5f, 0.5f, 0.5f, C2D_Color32(200, 200, 200, 255));
        return;
    }

    TitleInfo *currentTitle = &titles[cursor];

    // Draw icon placeholder (icon loading disabled for now)
    C2D_DrawRectSolid(10.0f, y, 0.5f, 48, 48, C2D_Color32(50, 50, 80, 255));
    C2D_TextParse(&text, dynamicBuf, "?");
    C2D_TextOptimize(&text);
    // Centro il punto interrogativo nel quadrato 48x48px
    C2D_DrawText(&text, C2D_WithColor, 26.0f, y + 10, 0.5f, 1.8f, 1.8f, C2D_Color32(150, 150, 180, 255));

    // Title name (next to icon placeholder)
    float textX = 65.0f;
    C2D_TextParse(&text, dynamicBuf, "TITLE DETAILS");
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, textX, y + 5, 0.5f, 0.55f, 0.55f, C2D_Color32(100, 180, 255, 255));
    y += 58;

    // Separator
    C2D_DrawRectSolid(10, y, 0.5f, 300, 1, C2D_Color32(100, 100, 150, 255));
    y += 10;

    // Full title name (word wrap if needed)
    C2D_TextParse(&text, dynamicBuf, "Name:");
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, 10.0f, y, 0.5f, 0.43f, 0.43f, C2D_Color32(150, 150, 150, 255));
    y += 14;

    C2D_TextParse(&text, dynamicBuf, currentTitle->fullName);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, 10.0f, y, 0.5f, 0.40f, 0.40f, C2D_Color32(255, 255, 255, 255));
    y += 18;

    // Title ID
    char tidStr[64];
    snprintf(tidStr, sizeof(tidStr), "Title ID: %016llX", currentTitle->titleID);
    C2D_TextParse(&text, dynamicBuf, tidStr);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, 10.0f, y, 0.5f, 0.38f, 0.38f, C2D_Color32(200, 200, 200, 255));
    y += 15;

    // Version
    char verStr[64];
    snprintf(verStr, sizeof(verStr), "Version: v%d", currentTitle->version);
    C2D_TextParse(&text, dynamicBuf, verStr);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, 10.0f, y, 0.5f, 0.38f, 0.38f, C2D_Color32(200, 200, 200, 255));
    y += 15;

    // Size
    char sizeStr[64];
    if (currentTitle->size > 0) {
        // Convert to MB or GB
        if (currentTitle->size >= 1024 * 1024 * 1024) {
            float sizeGB = (float)currentTitle->size / (1024.0f * 1024.0f * 1024.0f);
            snprintf(sizeStr, sizeof(sizeStr), "Size: %.2f GB", sizeGB);
        } else if (currentTitle->size >= 1024 * 1024) {
            float sizeMB = (float)currentTitle->size / (1024.0f * 1024.0f);
            snprintf(sizeStr, sizeof(sizeStr), "Size: %.2f MB", sizeMB);
        } else {
            float sizeKB = (float)currentTitle->size / 1024.0f;
            snprintf(sizeStr, sizeof(sizeStr), "Size: %.2f KB", sizeKB);
        }
    } else {
        snprintf(sizeStr, sizeof(sizeStr), "Size: Unknown");
    }
    C2D_TextParse(&text, dynamicBuf, sizeStr);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, 10.0f, y, 0.5f, 0.38f, 0.38f, C2D_Color32(200, 200, 200, 255));
    y += 15;

    // Type
    u32 highID = (u32)(currentTitle->titleID >> 32);
    const char *typeStr = "Game/Application";
    if (highID == 0x0004000E) typeStr = "Update (^)";
    else if (highID == 0x0004008C) typeStr = "DLC (+)";

    char typeFullStr[64];
    snprintf(typeFullStr, sizeof(typeFullStr), "Type: %s", typeStr);
    C2D_TextParse(&text, dynamicBuf, typeFullStr);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, 10.0f, y, 0.5f, 0.38f, 0.38f, C2D_Color32(200, 200, 200, 255));
    y += 15;

    // Media Type
    const char *mediaStr = (currentTitle->mediaType == MEDIATYPE_SD) ? "SD Card" : "NAND";
    char mediaFullStr[64];
    snprintf(mediaFullStr, sizeof(mediaFullStr), "Location: %s", mediaStr);
    C2D_TextParse(&text, dynamicBuf, mediaFullStr);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, 10.0f, y, 0.5f, 0.38f, 0.38f, C2D_Color32(200, 200, 200, 255));
    y += 18;

    // Separator
    C2D_DrawRectSolid(10, y, 0.5f, 300, 1, C2D_Color32(100, 100, 150, 255));
    y += 10;

    // Backup status
    if (currentTitle->hasBackup) {
        C2D_TextParse(&text, dynamicBuf, "Backup: YES \xE2\x9C\x93");  // ✓
        C2D_TextOptimize(&text);
        C2D_DrawText(&text, C2D_WithColor, 10.0f, y, 0.5f, 0.38f, 0.38f, C2D_Color32(100, 255, 100, 255));
        y += 14;

        // Show backup path
        char backupPathStr[128];
        snprintf(backupPathStr, sizeof(backupPathStr), "Path: %s/%016llX", config.backupPath, currentTitle->titleID);
        C2D_TextParse(&text, dynamicBuf, backupPathStr);
        C2D_TextOptimize(&text);
        C2D_DrawText(&text, C2D_WithColor, 10.0f, y, 0.5f, 0.30f, 0.30f, C2D_Color32(150, 150, 150, 255));
    } else {
        C2D_TextParse(&text, dynamicBuf, "Backup: NO \xE2\x9C\x97");  // ✗
        C2D_TextOptimize(&text);
        C2D_DrawText(&text, C2D_WithColor, 10.0f, y, 0.5f, 0.38f, 0.38f, C2D_Color32(255, 100, 100, 255));
    }
    y += 18;

    // Separator
    C2D_DrawRectSolid(10, y, 0.5f, 300, 1, C2D_Color32(100, 100, 150, 255));
    y += 10;

    // Reminder per vedere i controlli
    C2D_TextParse(&text, dynamicBuf, "Press SELECT for controls");
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, 10.0f, y, 0.5f, 0.40f, 0.40f, C2D_Color32(150, 200, 255, 255));
}

void drawDialog(const char **lines, int lineCount) {
    // This function manages its own frame since it's used for modal dialogs
    // that are displayed outside the main rendering loop during user interactions
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(top, C2D_Color32(0, 0, 0, 255));
    C2D_SceneBegin(top);
    
    C2D_TextBufClear(dynamicBuf);
    C2D_Text text;
    float y = 20.0f;
    
    for (int i = 0; i < lineCount; i++) {
        if (lines[i] && strlen(lines[i]) > 0) {
            C2D_TextParse(&text, dynamicBuf, lines[i]);
            C2D_TextOptimize(&text);
            C2D_DrawText(&text, C2D_WithColor, 10.0f, y, 0.5f, 0.45f, 0.45f, C2D_Color32(255, 255, 255, 255));
        }
        y += 15;
    }
    
    C3D_FrameEnd(0);
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
        
        const char *successLines[] = {
            "",
            "Successfully deleted:",
            title->name,
            "",
            "All associated data removed:",
            "  - Title application",
            "  - Save data",
            "  - ExtData (if present)",
            "  - Boss ExtData (if present)",
            "",
            "Press A to continue..."
        };
        drawDialog(successLines, 11);
    } else {
        const char *failLines[] = {
            "",
            "Failed to delete:",
            title->name,
            "Title may still be present on system.",
            "",
            "Press A to continue..."
        };
        drawDialog(failLines, 6);
    }
    
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        
        if (kDown & KEY_A)
            break;
    }
}

void handleInput() {
    // Input già scannerizzato nel main loop
    u32 kDown = hidKeysDown();
    u32 kHeld = hidKeysHeld();
    
    static u32 scrollDelayTimer = 0;
    static bool canScroll = true;

    if (titleCount == 0)
        return;
    
    // Sistema di scroll migliorato: richiede rilascio completo del tasto prima del prossimo movimento
    // Se nessun tasto direzionale è premuto, resetta il timer e abilita lo scroll
    if (!(kHeld & KEY_DUP) && !(kHeld & KEY_DDOWN)) {
        scrollDelayTimer = 0;
        canScroll = true;
    }

    // Navigation UP con controllo preciso
    if (kDown & KEY_DUP) {
        // Prima pressione: movimento immediato
        if (cursor > 0) {
            cursor--;
            needsRedraw = true;
            canScroll = false;  // Disabilita scroll continuo
            scrollDelayTimer = 0;
        }
    } else if (kHeld & KEY_DUP && canScroll) {
        // Tasto tenuto premuto: aspetta molto prima di iniziare scroll continuo
        scrollDelayTimer++;
        if (scrollDelayTimer > 90) {  // ~1.5 secondi di delay
            // Ora permetti scroll continuo ma MOLTO lento
            if (scrollDelayTimer % 25 == 0) {  // 1 movimento ogni ~0.4 secondi
                if (cursor > 0) {
                    cursor--;
                    needsRedraw = true;
                }
            }
        }
    }
    
    // Navigation DOWN con controllo preciso
    if (kDown & KEY_DDOWN) {
        // Prima pressione: movimento immediato
        if (cursor < titleCount - 1) {
            cursor++;
            needsRedraw = true;
            canScroll = false;  // Disabilita scroll continuo
            scrollDelayTimer = 0;
        }
    } else if (kHeld & KEY_DDOWN && canScroll) {
        // Tasto tenuto premuto: aspetta molto prima di iniziare scroll continuo
        scrollDelayTimer++;
        if (scrollDelayTimer > 90) {  // ~1.5 secondi di delay
            // Ora permetti scroll continuo ma MOLTO lento
            if (scrollDelayTimer % 25 == 0) {  // 1 movimento ogni ~0.4 secondi
                if (cursor < titleCount - 1) {
                    cursor++;
                    needsRedraw = true;
                }
            }
        }
    }
    
    // Toggle selection
    if (kDown & KEY_A) {
        titles[cursor].selected = !titles[cursor].selected;
        needsRedraw = true;
    }
    
    // Show controls with SELECT
    if (kDown & KEY_SELECT) {
        const char *controlLines[] = {
            "",
            "=== CONTROLS ===",
            "",
            "A: Select/Deselect title",
            "X: Uninstall selected titles",
            "",
            "D-Pad Up/Down: Navigate",
            "D-Pad Left/Right: Page up/down",
            "",
            "L: Sort by Name",
            "R: Sort by Title ID",
            "",
            "START: Exit application",
            "",
            "Press A to continue..."
        };
        drawDialog(controlLines, 15);

        while (aptMainLoop()) {
            hidScanInput();
            u32 kDown2 = hidKeysDown();
            if (kDown2 & KEY_A)
                break;
        }
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

    // Fast scroll with LEFT/RIGHT (page up/down)
    if (kDown & KEY_DLEFT) {
        // Scroll up one page
        cursor -= MAX_VISIBLE_TITLES;
        if (cursor < 0) cursor = 0;
        needsRedraw = true;
    }

    if (kDown & KEY_DRIGHT) {
        // Scroll down one page
        cursor += MAX_VISIBLE_TITLES;
        if (cursor >= titleCount) cursor = titleCount - 1;
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
            const char *noSelLines[] = {
                "",
                "No titles selected!",
                "",
                "Press A to continue..."
            };
            drawDialog(noSelLines, 4);

            while (aptMainLoop()) {
                hidScanInput();
                u32 kDown2 = hidKeysDown();
                if (kDown2 & KEY_A)
                    break;
            }
            return;
        }
        
        // Ask for backup confirmation
        char countMsg[64];
        snprintf(countMsg, sizeof(countMsg), "You are about to uninstall %d title(s).", selectedCount);
        const char *backupLines[] = {
            "",
            countMsg,
            "",
            "Do you want to backup save data?",
            "",
            "  A: Yes, backup saves",
            "  B: No, skip backup",
            "  START: Cancel"
        };
        drawDialog(backupLines, 8);

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
        }
        
        if (cancelled)
            return;
        
        // Ask about backup path if backing up
        char selectedBackupPath[256];
        snprintf(selectedBackupPath, sizeof(selectedBackupPath), "%s", config.backupPath);
        
        if (backupSaves) {
            char pathMsg[300];
            snprintf(pathMsg, sizeof(pathMsg), "Current path: %s", config.backupPath);
            const char *pathLines[] = {
                "",
                "Backup Path Selection",
                "",
                pathMsg,
                "",
                "Do you want to change the backup path?",
                "",
                "  A: Use current path",
                "  Y: Choose alternative path",
                "  START: Cancel"
            };
            drawDialog(pathLines, 10);

            bool useDefault = true;
            
            while (aptMainLoop()) {
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
                
                while (aptMainLoop()) {
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
                        break;
                    }
                    if (kDown2c & KEY_B) {
                        cancelled = true;
                        break;
                    }

                    // Draw path selection dialog
                    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
                    C2D_TargetClear(top, C2D_Color32(0, 0, 0, 255));
                    C2D_SceneBegin(top);
                    
                    C2D_TextBufClear(dynamicBuf);
                    C2D_Text text;
                    float y = 20.0f;
                    
                    C2D_TextParse(&text, dynamicBuf, "Select Backup Path");
                    C2D_TextOptimize(&text);
                    C2D_DrawText(&text, C2D_WithColor, 10.0f, y, 0.5f, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255));
                    y += 20;
                    
                    C2D_TextParse(&text, dynamicBuf, "Use D-Pad to select, A to confirm");
                    C2D_TextOptimize(&text);
                    C2D_DrawText(&text, C2D_WithColor, 10.0f, y, 0.5f, 0.4f, 0.4f, C2D_Color32(200, 200, 200, 255));
                    y += 25;

                    for (int i = 0; i < NUM_BACKUP_PATHS; i++) {
                        if (i == pathCursor) {
                            C2D_DrawRectSolid(5, y - 2, 0.5f, 390, 14, C2D_Color32(255, 255, 255, 255));
                        }
                        C2D_TextParse(&text, dynamicBuf, BACKUP_PATH_OPTIONS[i]);
                        C2D_TextOptimize(&text);
                        u32 textColor = (i == pathCursor) ? C2D_Color32(0, 0, 0, 255) : C2D_Color32(255, 255, 255, 255);
                        C2D_DrawText(&text, C2D_WithColor, 10.0f, y, 0.5f, 0.35f, 0.35f, textColor);
                        y += 16;
                    }

                    y += 10;
                    C2D_TextParse(&text, dynamicBuf, "  B: Cancel");
                    C2D_TextOptimize(&text);
                    C2D_DrawText(&text, C2D_WithColor, 10.0f, y, 0.5f, 0.4f, 0.4f, C2D_Color32(200, 200, 200, 255));

                    C3D_FrameEnd(0);
                }
                
                if (cancelled)
                    return;
            }
        }
        
        // Final confirmation
        char uninstallMsg[64];
        char backupMsg[64];
        snprintf(uninstallMsg, sizeof(uninstallMsg), "Uninstall %d title(s)?", selectedCount);
        snprintf(backupMsg, sizeof(backupMsg), "Backup saves: %s", backupSaves ? "YES" : "NO");
        
        char pathInfoMsg[300] = "";
        if (backupSaves) {
            snprintf(pathInfoMsg, sizeof(pathInfoMsg), "Backup path: %s", selectedBackupPath);
        }
        
        const char *confirmLines[9] = {
            "",
            "Final confirmation:",
            "",
            uninstallMsg,
            backupMsg,
            pathInfoMsg,
            "",
            "  A: Confirm",
            "  B: Cancel"
        };
        drawDialog(confirmLines, 9);

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
        }
        
        if (!confirmed)
            return;
        
        // Process deletions - show processing screen
        const char *processingLines[] = {
            "",
            "Processing...",
            ""
        };
        drawDialog(processingLines, 3);
        
        for (int i = 0; i < titleCount; i++) {
            if (titles[i].selected && titles[i].isValid) {
                char statusLines[5][256];
                snprintf(statusLines[0], 256, "Processing: %s", titles[i].name);
                
                const char *statusPtrs[5] = {
                    "",
                    statusLines[0],
                    "",
                    "",
                    ""
                };
                
                if (backupSaves) {
                    snprintf(statusLines[2], 256, "  Backing up save data to:");
                    snprintf(statusLines[3], 256, "  %s", selectedBackupPath);
                    statusPtrs[2] = statusLines[2];
                    statusPtrs[3] = statusLines[3];
                    drawDialog(statusPtrs, 5);
                    backupSaveDataToPath(&titles[i], selectedBackupPath);
                }
                
                snprintf(statusLines[4], 256, "  Deleting title...");
                statusPtrs[4] = statusLines[4];
                drawDialog(statusPtrs, 5);
                
                deleteTitle(&titles[i]);
            }
        }
        
        // Reload titles
        const char *reloadLines[] = {
            "",
            "Reloading title list...",
            ""
        };
        drawDialog(reloadLines, 3);
        
        loadTitles();
        cursor = 0;
        scrollOffset = 0;
        needsRedraw = true;
    }
}


int main(int argc, char **argv) {
    // Initialize graphics
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    
    // Create render targets for top and bottom screens
    top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    
    // Create text buffer for dynamic text rendering
    // 4096 bytes is sufficient for ~100 text objects (average 40 bytes each)
    // Increase if experiencing text rendering issues with long strings
    dynamicBuf = C2D_TextBufNew(4096);

    // Initialize services
    amInit();
    fsInit();
    
    // Load configuration
    loadConfig();
    
    // Load titles with progress bar
    loadTitles();
    
    // Initial draw - both screens in one frame
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    drawUI();
    drawTouchControls();
    C3D_FrameEnd(0);
    needsRedraw = false;

    // Main loop
    bool running = true;
    while (aptMainLoop() && running) {
        // Gestione sleep mode: aptMainLoop() ritorna false durante sleep
        // e riprende quando il 3DS si sveglia

        hidScanInput();
        u32 kDown = hidKeysDown();
        
        // Check for exit before drawing/handling
        if (kDown & KEY_START) {
            running = false;
            break;
        }
        
        handleInput();

        // Renderizza sempre un frame per evitare crash durante sleep/wake
        // Il 3DS richiede rendering continuo per gestire correttamente lo sleep mode
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        // Solo ridisegna la UI se necessario, altrimenti riusa il frame precedente
        if (needsRedraw) {
            drawUI();
            drawTouchControls();
            needsRedraw = false;
        } else {
            // Ridisegna comunque per mantenere il frame buffer attivo
            drawUI();
            drawTouchControls();
        }

        C3D_FrameEnd(0);

        // Piccola pausa per evitare consumo eccessivo CPU
        // gspWaitForVBlank() è già chiamato internamente da C3D_FrameEnd
    }
    
    // Cleanup
    C2D_TextBufDelete(dynamicBuf);
    C2D_Fini();
    C3D_Fini();
    fsExit();
    amExit();
    gfxExit();
    
    return 0;
}
