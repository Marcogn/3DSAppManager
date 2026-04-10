#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/statvfs.h>
#include <time.h>
/* SMDH structure definition */
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
#define MAX_TITLES        500
#define CONFIG_PATH       "sdmc:/3ds/fast-uninstall/config.ini"
#define DEFAULT_BACKUP_PATH "sdmc:/3ds/fast-uninstall/backups"
#define LANGUAGE_ENGLISH  1
#define SMDH_ICON_PATH    0x6E6F6369
#define MAX_FILE_SIZE     (100 * 1024 * 1024)
#define MAX_VISIBLE_TITLES 13
#define MAX_FILES         256
#define CHUNK_SIZE        0x10000
#define ALIGN64(x)        (((u32)(x) + 63) & ~63U)
/* ---- Enums ---- */
typedef enum {
    APP_MAIN_MENU,
    APP_INSTALL,
    APP_BACKUP,
    APP_UNINSTALL,
    APP_SYSINFO,
    APP_SETTINGS
} AppState;
typedef enum {
    SORT_BY_NAME,
    SORT_BY_SIZE,
    SORT_BY_TITLEID
} SortMode;
typedef enum {
    FILTER_ALL,
    FILTER_UPDATES,
    FILTER_DLC
} FilterMode;
typedef enum {
    SYSINFO_OVERVIEW,
    SYSINFO_GAMES,
    SYSINFO_UPDATES,
    SYSINFO_DLC
} SysInfoMode;
/* ---- Structs ---- */
typedef struct {
    u64 titleID;
    char name[256];
    char fullName[256];
    FS_MediaType mediaType;
    bool selected;
    bool isValid;
    u16 version;
    bool hasBackup;
    C2D_Image icon;
    bool iconLoaded;
    u64 size;
} TitleInfo;
typedef struct {
    char backupPath[256];
    bool forceBackup;
    bool skipUninstallConfirm;
    bool forceRestore;
    bool skipInstallConfirm;
    /* All CIA installs always target SD card — no NAND option */
} Config;
typedef struct {
    char name[256];
    bool isDir;
    bool isCIA;
    u64  size;
} FileEntry;
/* Alternative backup paths */
static const char *BACKUP_PATH_OPTIONS[] = {
    "sdmc:/3ds/fast-uninstall/backups",
    "sdmc:/backups/3ds-titles",
    "sdmc:/save-backups",
    "sdmc:/3ds-backups",
    "sdmc:/backups"
};
#define NUM_BACKUP_PATHS (sizeof(BACKUP_PATH_OPTIONS)/sizeof(BACKUP_PATH_OPTIONS[0]))
/* ============================================================
   SECTION 2: Global Variables
   ============================================================ */
static TitleInfo titles[MAX_TITLES];
static int titleCount      = 0;
static int cursor          = 0;
static int scrollOffset    = 0;
static Config config;
static C3D_RenderTarget *top;
static C3D_RenderTarget *bottom;
static C2D_TextBuf dynamicBuf;
static bool needsRedraw    = true;
static SortMode  currentSortMode   = SORT_BY_NAME;
static FilterMode currentFilterMode = FILTER_ALL;
static int filteredIndices[MAX_TITLES];
static int filteredCount   = 0;
/* App state */
static AppState appState   = APP_MAIN_MENU;
static int menuCursor      = 0;
/* File browser */
static FileEntry fileEntries[MAX_FILES];
static int fileCount       = 0;
static int fileCursor      = 0;
static int fileScrollOffset = 0;
static char currentPath[512];
/* Backup flow */
static int backupCursor      = 0;
static int backupScrollOffset = 0;
/* System Info */
static SysInfoMode sysInfoMode         = SYSINFO_OVERVIEW;
static int sysInfoCursor               = 0;
static int sysInfoSubCursor            = 0;
static int sysInfoSubScrollOffset      = 0;
static int sysInfoSubIndices[MAX_TITLES];
static int sysInfoSubCount             = 0;
/* Settings */
static int settingsCursor = 0;
/* Flag: set to true after a CIA install so next Uninstall/Backup/SysInfo entry reloads */
static bool titlesNeedRefresh = false;
/* ============================================================
   SECTION 3: Function Prototypes
   ============================================================ */
void createDirectory(const char *path);
void loadConfig(void);
void saveConfig(void);
void saveDefaultConfig(void);
void sanitizeName(char *name);
void formatSize(u64 size, char *buf, size_t bufSize);
void sortTitles(void);
void updateFilteredList(void);
int  compareTitlesByName(const void *a, const void *b);
int  compareTitlesBySize(const void *a, const void *b);
int  compareTitlesByID(const void *a, const void *b);
void getTitleName(u64 titleID, FS_MediaType mediaType, char *outName, size_t outSize);
void getTitleInfo(TitleInfo *title);
u64  getTitleSize(u64 titleID, FS_MediaType mediaType);
bool checkBackupExists(u64 titleID);
bool findBackupDir(u64 titleID, char *outDir, size_t outSize);
void getBackupDirName(u64 titleID, const char *titleName, char *outName, size_t outSize);
bool getBackupLastDate(u64 titleID, char *outDate, size_t outSize);
void loadTitleIcon(TitleInfo *title);
void drawLoadingScreen(int current, int total, const char *status);
void loadTitles(void);
void findRelatedTitles(u64 baseTitleID, int selfIdx, int *outIdx, int *outCount);
bool getSDFreeSpace(u64 *freeBytes, u64 *totalBytes);
/* Backup / Restore / Install */
void copyDirectory(FS_Archive archive, const char *srcPath, const char *dstPath);
void backupArchive(FS_Archive archive, const char *basePath, const char *archiveName);
void backupSaveDataToPath(TitleInfo *title, const char *backupPath);
void backupSaveData(TitleInfo *title);
void restoreDirectory(FS_Archive archive, const char *srcPath, const char *dstPath);
bool restoreSaveData(TitleInfo *title);
u64  getCIATitleID(const char *ciaPath);
int  scanDirectory(const char *path);
bool installCIA(const char *ciaPath, FS_MediaType dest);
/* Delete */
void deleteTitleCompletely(TitleInfo *title);
void deleteTitle(TitleInfo *title);
/* Draw */
void drawMainMenu(void);
void drawUI(void);
void drawSelectedTitlesList(void);
void drawTouchControls(void);
void drawDialog(const char **lines, int lineCount);
void drawDialogWithSelectedList(const char **lines, int lineCount);
void drawControlsOverlay(void);
void drawFileBrowserScreen(void);
void drawBackupScreen(void);
void drawSysInfoScreen(void);
void drawSettingsScreen(void);
void drawTitleDetails(void);
/* Input / Flows */
void handleMainMenuInput(void);
void handleUninstallInput(void);
void runInstallFlow(void);
void runBackupFlow(void);
void handleSysInfoInput(void);
void handleSettingsInput(void);
/* ============================================================
   SECTION 4: Utility Functions
   ============================================================ */
void createDirectory(const char *path) {
    char tmp[256];
    char *p = NULL;
    snprintf(tmp, sizeof(tmp), "%s", path);
    size_t len = strlen(tmp);
    if (tmp[len-1] == '/') tmp[len-1] = 0;
    for (p = tmp+1; *p; p++) {
        if (*p == '/') { *p=0; mkdir(tmp,0777); *p='/'; }
    }
    mkdir(tmp, 0777);
}
void sanitizeName(char *name) {
    if (!name) return;
    char *src = name, *dst = name;
    while (*src) {
        if ((unsigned char)*src >= 32 && (unsigned char)*src <= 126) {
            if (*src!='|'&&*src!='<'&&*src!='>'&&*src!='"'&&
                *src!='\\'&&*src!='/'&&*src!=':'&&*src!='*'&&*src!='?')
                *dst++ = *src;
            else *dst++ = ' ';
        }
        src++;
    }
    *dst = '\0';
    while (dst > name && *(dst-1)==' ') *(--dst)='\0';
    if (name[0]=='\0') strcpy(name,"Unknown Title");
}
void formatSize(u64 size, char *buf, size_t bufSize) {
    if (size >= (u64)1024*1024*1024)
        snprintf(buf, bufSize, "%.2f GB", (float)size/(1024.0f*1024.0f*1024.0f));
    else if (size >= 1024*1024)
        snprintf(buf, bufSize, "%.2f MB", (float)size/(1024.0f*1024.0f));
    else if (size >= 1024)
        snprintf(buf, bufSize, "%.2f KB", (float)size/1024.0f);
    else
        snprintf(buf, bufSize, "%llu B", (unsigned long long)size);
}
/* ============================================================
   SECTION 5: Config Functions
   ============================================================ */
void loadConfig(void) {
    snprintf(config.backupPath, sizeof(config.backupPath), "%s", DEFAULT_BACKUP_PATH);
    config.forceBackup          = false;
    config.skipUninstallConfirm = false;
    config.forceRestore         = false;
    config.skipInstallConfirm   = false;
    FILE *f2 = fopen(CONFIG_PATH, "r");
    if (!f2) { saveDefaultConfig(); return; }
    char line[512];
    while (fgets(line, sizeof(line), f2)) {
        line[strcspn(line,"\r\n")] = 0;
        if (line[0]=='\0'||line[0]=='#'||line[0]==';') continue;
        char *eq = strchr(line,'=');
        if (!eq) continue;
        *eq='\0';
        char *key=line, *val=eq+1;
        while (*key==' '||*key=='\t') key++;
        while (*val==' '||*val=='\t') val++;
        if (strcmp(key,"backup_path")==0)
            snprintf(config.backupPath,sizeof(config.backupPath),"%s",val);
        else if (strcmp(key,"force_backup")==0)
            config.forceBackup=(strcmp(val,"1")==0||strcmp(val,"true")==0);
        else if (strcmp(key,"skip_uninstall_confirm")==0)
            config.skipUninstallConfirm=(strcmp(val,"1")==0||strcmp(val,"true")==0);
        else if (strcmp(key,"force_restore")==0)
            config.forceRestore=(strcmp(val,"1")==0||strcmp(val,"true")==0);
        else if (strcmp(key,"skip_install_confirm")==0)
            config.skipInstallConfirm=(strcmp(val,"1")==0||strcmp(val,"true")==0);
        /* install_dest obsolete — ignored */
    }
    fclose(f2);
}
void saveConfig(void) {
    createDirectory("sdmc:/3ds/fast-uninstall");
    FILE *f2 = fopen(CONFIG_PATH,"w");
    if (!f2) return;
    fprintf(f2,"# 3DS Fast Uninstall Configuration\n");
    fprintf(f2,"backup_path=%s\n", config.backupPath);
    fprintf(f2,"force_backup=%d\n", config.forceBackup?1:0);
    fprintf(f2,"skip_uninstall_confirm=%d\n", config.skipUninstallConfirm?1:0);
    fprintf(f2,"force_restore=%d\n", config.forceRestore?1:0);
    fprintf(f2,"skip_install_confirm=%d\n", config.skipInstallConfirm?1:0);
    fclose(f2);
}
void saveDefaultConfig(void) {
    createDirectory("sdmc:/3ds/fast-uninstall");
    FILE *f2 = fopen(CONFIG_PATH,"w");
    if (!f2) return;
    fprintf(f2,"# 3DS Fast Uninstall Configuration\n");
    fprintf(f2,"backup_path=%s\n", DEFAULT_BACKUP_PATH);
    fprintf(f2,"force_backup=0\n");
    fprintf(f2,"skip_uninstall_confirm=0\n");
    fprintf(f2,"force_restore=0\n");
    fprintf(f2,"skip_install_confirm=0\n");
    fclose(f2);
}
/* ============================================================
   SECTION 6: Backup Directory Helpers
   ============================================================ */
void getBackupDirName(u64 titleID, const char *titleName, char *outName, size_t outSize) {
    char safe[80];
    strncpy(safe, titleName, 79); safe[79]='\0';
    for (char *p=safe; *p; p++) {
        if (*p=='/'||*p=='\\'||*p==':'||*p=='*'||*p=='?'||
            *p=='"'||*p=='<'||*p=='>'||*p=='|'||*p=='\n'||*p=='\r')
            *p='_';
    }
    /* Limit name part to 60 chars */
    if (strlen(safe)>60) safe[60]='\0';
    snprintf(outName, outSize, "%016llX-%s", (unsigned long long)titleID, safe);
}
bool findBackupDir(u64 titleID, char *outDir, size_t outSize) {
    char prefix[20];
    snprintf(prefix, sizeof(prefix), "%016llX", (unsigned long long)titleID);
    DIR *d = opendir(config.backupPath);
    if (!d) return false;
    struct dirent *entry;
    while ((entry=readdir(d))!=NULL) {
        if (strncmp(entry->d_name, prefix, 16)==0) {
            snprintf(outDir, outSize, "%s/%s", config.backupPath, entry->d_name);
            closedir(d);
            return true;
        }
    }
    closedir(d);
    return false;
}
bool getBackupLastDate(u64 titleID, char *outDate, size_t outSize) {
    char backupDir[512];
    if (!findBackupDir(titleID, backupDir, sizeof(backupDir))) return false;
    char infoPath[512];
    snprintf(infoPath, sizeof(infoPath), "%s/backup_info.txt", backupDir);
    FILE *f2 = fopen(infoPath,"r");
    if (!f2) { snprintf(outDate,outSize,"??/??/??"); return true; }
    char line[256];
    while (fgets(line,sizeof(line),f2)) {
        if (strncmp(line,"Backup Date:",12)==0) {
            char *d2=line+12;
            while(*d2==' ') d2++;
            d2[strcspn(d2,"\r\n")]=0;
            snprintf(outDate,outSize,"%s",d2);
            fclose(f2);
            return true;
        }
    }
    fclose(f2);
    snprintf(outDate,outSize,"unknown");
    return true;
}
bool checkBackupExists(u64 titleID) {
    char backupDir[512];
    return findBackupDir(titleID, backupDir, sizeof(backupDir));
}
bool getSDFreeSpace(u64 *freeBytes, u64 *totalBytes) {
    struct statvfs st;
    if (statvfs("sdmc:/",&st)!=0) { *freeBytes=0; *totalBytes=0; return false; }
    *freeBytes  = (u64)st.f_bavail * st.f_bsize;
    *totalBytes = (u64)st.f_blocks * st.f_bsize;
    return true;
}
/* ============================================================
   SECTION 7: Sort / Filter / Title Management
   ============================================================ */
int compareTitlesByName(const void *a, const void *b) {
    return strcasecmp(((TitleInfo*)a)->name, ((TitleInfo*)b)->name);
}
int compareTitlesBySize(const void *a, const void *b) {
    const TitleInfo *ta=(const TitleInfo*)a, *tb=(const TitleInfo*)b;
    if (ta->size > tb->size) return -1;
    if (ta->size < tb->size) return  1;
    return 0;
}
int compareTitlesByID(const void *a, const void *b) {
    const TitleInfo *ta=(const TitleInfo*)a, *tb=(const TitleInfo*)b;
    if (ta->titleID < tb->titleID) return -1;
    if (ta->titleID > tb->titleID) return  1;
    return 0;
}
void sortTitles(void) {
    if (titleCount<=0) return;
    if (currentSortMode==SORT_BY_NAME)
        qsort(titles,titleCount,sizeof(TitleInfo),compareTitlesByName);
    else if (currentSortMode==SORT_BY_SIZE)
        qsort(titles,titleCount,sizeof(TitleInfo),compareTitlesBySize);
    else
        qsort(titles,titleCount,sizeof(TitleInfo),compareTitlesByID);
    cursor=0; scrollOffset=0; needsRedraw=true;
}
void updateFilteredList(void) {
    filteredCount=0;
    for (int i=0;i<titleCount;i++) {
        u32 hi=(u32)(titles[i].titleID>>32);
        bool show=false;
        if (currentFilterMode==FILTER_ALL) show=true;
        else if (currentFilterMode==FILTER_UPDATES && hi==0x0004000E) show=true;
        else if (currentFilterMode==FILTER_DLC    && hi==0x0004008C) show=true;
        if (show) filteredIndices[filteredCount++]=i;
    }
    if (cursor>=filteredCount && filteredCount>0) cursor=filteredCount-1;
    if (cursor<0 && filteredCount>0) cursor=0;
}
void getTitleName(u64 titleID, FS_MediaType mediaType, char *outName, size_t outSize) {
    SMDH smdh;
    AM_TitleEntry te;
    Result res=AM_GetTitleInfo(mediaType,1,&titleID,&te);
    if (R_FAILED(res)) { snprintf(outName,outSize,"Unknown [%016llX]",(unsigned long long)titleID); return; }
    Handle fh;
    u32 ap[]={(u32)(titleID&0xFFFFFFFF),(u32)((titleID>>32)&0xFFFFFFFF),mediaType,0};
    static const u32 fp[]={0,0,2,0x6E6F6369,0};
    FS_Path ap2={PATH_BINARY,16,ap}, fp2={PATH_BINARY,20,fp};
    res=FSUSER_OpenFileDirectly(&fh,ARCHIVE_SAVEDATA_AND_CONTENT,ap2,fp2,FS_OPEN_READ,0);
    if (R_SUCCEEDED(res)) {
        u32 br=0;
        FSFILE_Read(fh,&br,0,&smdh,sizeof(SMDH));
        FSFILE_Close(fh);
        if (br>=sizeof(u32) && smdh.magic==0x48444D53) {
            u8 lang=CFG_LANGUAGE_EN;
            CFGU_GetSystemLanguage(&lang);
            int order[3]={(lang<12)?lang:1,1,0};
            bool found=false;
            for (int a=0;a<3&&!found;a++) {
                if (a>0&&order[a]==order[0]) continue;
                ssize_t u=utf16_to_utf8((uint8_t*)outName,smdh.titles[order[a]].shortDescription,outSize-1);
                if (u>0) { outName[u]='\0'; bool ok=false; for(int i=0;i<u;i++){if(outName[i]!=' '&&outName[i]!='\0'){ok=true;break;}} if(ok){found=true;} }
            }
            if (!found) {
                for (int i=0;i<12;i++) {
                    ssize_t u=utf16_to_utf8((uint8_t*)outName,smdh.titles[i].shortDescription,outSize-1);
                    if (u>0){outName[u]='\0';bool ok=false;for(int j=0;j<u;j++){if(outName[j]!=' '&&outName[j]!='\0'){ok=true;break;}}if(ok)break;}
                }
            }
            /* strip filesystem-unsafe chars */
            char *s=outName,*d=outName;
            while(*s){if(*s!='|'&&*s!='<'&&*s!='>'&&*s!='"'&&*s!='\\'&&*s!='/'&&*s!=':'&&*s!='*'&&*s!='?')*d++=*s;s++;}*d='\0';
            return;
        }
    }
    u32 hi=(u32)(titleID>>32);
    const char *t="";
    if (hi==0x0004000E) t=" Upd"; else if (hi==0x0004008C) t=" DLC";
    snprintf(outName,outSize,"Title%s [%016llX]",t,(unsigned long long)titleID);
}
u64 getTitleSize(u64 titleID, FS_MediaType mediaType) {
    u64 sz=0; AM_TitleEntry e;
    if (R_SUCCEEDED(AM_GetTitleInfo(mediaType,1,&titleID,&e))) sz=e.size;
    return sz;
}
void loadTitleIcon(TitleInfo *title) { title->iconLoaded=false; }
void getTitleInfo(TitleInfo *title) {
    AM_TitleEntry te;
    if (R_SUCCEEDED(AM_GetTitleInfo(title->mediaType,1,&title->titleID,&te))) {
        title->version=te.version; title->size=te.size;
    } else { title->version=0; title->size=0; }
    if (title->size==0) title->size=getTitleSize(title->titleID,title->mediaType);
    SMDH smdh; Handle fh;
    u32 ap[]={(u32)(title->titleID&0xFFFFFFFF),(u32)((title->titleID>>32)&0xFFFFFFFF),title->mediaType,0};
    static const u32 fp[]={0,0,2,0x6E6F6369,0};
    FS_Path ap2={PATH_BINARY,16,ap},fp2={PATH_BINARY,20,fp};
    Result res=FSUSER_OpenFileDirectly(&fh,ARCHIVE_SAVEDATA_AND_CONTENT,ap2,fp2,FS_OPEN_READ,0);
    if (R_SUCCEEDED(res)) {
        u32 br=0; FSFILE_Read(fh,&br,0,&smdh,sizeof(SMDH)); FSFILE_Close(fh);
        if (br>=sizeof(u32)&&smdh.magic==0x48444D53) {
            ssize_t u=utf16_to_utf8((uint8_t*)title->fullName,smdh.titles[LANGUAGE_ENGLISH].longDescription,sizeof(title->fullName)-1);
            if (u<0) u=0; title->fullName[u]='\0'; sanitizeName(title->fullName);
            if (title->fullName[0]=='\0'||strcmp(title->fullName,"Unknown Title")==0) {
                u=utf16_to_utf8((uint8_t*)title->fullName,smdh.titles[LANGUAGE_ENGLISH].shortDescription,sizeof(title->fullName)-1);
                if (u<0) u=0; title->fullName[u]='\0'; sanitizeName(title->fullName);
            }
        }
    }
    if (title->fullName[0]=='\0') strncpy(title->fullName,title->name,sizeof(title->fullName)-1);
    title->hasBackup=checkBackupExists(title->titleID);
}
void drawLoadingScreen(int current, int total, const char *status) {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(top,C2D_Color32(0,0,0,255)); C2D_SceneBegin(top);
    C2D_TextBufClear(dynamicBuf); C2D_Text t;
    C2D_TextParse(&t,dynamicBuf,"Loading titles..."); C2D_TextOptimize(&t);
    C2D_DrawText(&t,C2D_WithColor,50,80,0.5f,0.6f,0.6f,C2D_Color32(255,255,255,255));
    if (status) {
        C2D_TextParse(&t,dynamicBuf,status); C2D_TextOptimize(&t);
        C2D_DrawText(&t,C2D_WithColor,50,105,0.5f,0.4f,0.4f,C2D_Color32(200,200,200,255));
    }
    if (total>0) {
        float p=(float)current/total, bw=300,bh=20,bx=50,by=130;
        C2D_DrawRectSolid(bx,by,0.5f,bw,bh,C2D_Color32(50,50,50,255));
        C2D_DrawRectSolid(bx,by,0.5f,bw*p,bh,C2D_Color32(100,180,255,255));
        C2D_DrawRectSolid(bx,by,0.5f,bw,2,C2D_Color32(255,255,255,255));
        C2D_DrawRectSolid(bx,by+bh-2,0.5f,bw,2,C2D_Color32(255,255,255,255));
        C2D_DrawRectSolid(bx,by,0.5f,2,bh,C2D_Color32(255,255,255,255));
        C2D_DrawRectSolid(bx+bw-2,by,0.5f,2,bh,C2D_Color32(255,255,255,255));
        char pt[64]; snprintf(pt,sizeof(pt),"%d / %d",current,total);
        C2D_TextParse(&t,dynamicBuf,pt); C2D_TextOptimize(&t);
        C2D_DrawText(&t,C2D_WithColor,bx+bw/2-20,by+25,0.5f,0.45f,0.45f,C2D_Color32(255,255,255,255));
    }
    C2D_TargetClear(bottom,C2D_Color32(20,20,30,255)); C2D_SceneBegin(bottom);
    C3D_FrameEnd(0);
}
void loadTitles(void) {
    titleCount=0;
    u32 cntSD=0,cntNAND=0;
    AM_GetTitleCount(MEDIATYPE_SD,&cntSD);
    AM_GetTitleCount(MEDIATYPE_NAND,&cntNAND);
    u32 total=cntSD+cntNAND;
    if (total==0){drawLoadingScreen(0,0,"No titles found");return;}
    drawLoadingScreen(0,total,"Allocating memory...");
    u64 *ids=(u64*)malloc(total*sizeof(u64));
    if (!ids) return;
    u32 rc=0;
    if (cntSD>0) {
        drawLoadingScreen(0,total,"Loading SD titles...");
        if (R_SUCCEEDED(AM_GetTitleList(&rc,MEDIATYPE_SD,cntSD,ids))) {
            for (u32 i=0;i<rc&&titleCount<MAX_TITLES;i++) {
                if (i%10==0){char s[128];snprintf(s,sizeof(s),"SD %d/%d",i,rc);drawLoadingScreen(i,total,s);}
                u64 tid=ids[i]; u32 hi=(u32)(tid>>32);
                if (hi==0x00040010||hi==0x00040030) continue;
                titles[titleCount].titleID=tid; titles[titleCount].mediaType=MEDIATYPE_SD;
                titles[titleCount].selected=false; titles[titleCount].isValid=true;
                titles[titleCount].iconLoaded=false; titles[titleCount].version=0;
                titles[titleCount].hasBackup=false; titles[titleCount].fullName[0]='\0'; titles[titleCount].size=0;
                getTitleName(tid,MEDIATYPE_SD,titles[titleCount].name,sizeof(titles[titleCount].name));
                getTitleInfo(&titles[titleCount]); titleCount++;
            }
        }
    }
    if (cntNAND>0) {
        drawLoadingScreen(cntSD,total,"Loading NAND titles...");
        if (R_SUCCEEDED(AM_GetTitleList(&rc,MEDIATYPE_NAND,cntNAND,ids))) {
            for (u32 i=0;i<rc&&titleCount<MAX_TITLES;i++) {
                if (i%10==0){char s[128];snprintf(s,sizeof(s),"NAND %d/%d",i,rc);drawLoadingScreen(cntSD+i,total,s);}
                u64 tid=ids[i]; u32 hi=(u32)(tid>>32);
                if (hi==0x00040010||hi==0x00040030||hi==0x00040138) continue;
                if (hi!=0x00040000&&hi!=0x0004000E&&hi!=0x0004008C) continue;
                titles[titleCount].titleID=tid; titles[titleCount].mediaType=MEDIATYPE_NAND;
                titles[titleCount].selected=false; titles[titleCount].isValid=true;
                titles[titleCount].iconLoaded=false; titles[titleCount].version=0;
                titles[titleCount].hasBackup=false; titles[titleCount].fullName[0]='\0'; titles[titleCount].size=0;
                getTitleName(tid,MEDIATYPE_NAND,titles[titleCount].name,sizeof(titles[titleCount].name));
                getTitleInfo(&titles[titleCount]); titleCount++;
            }
        }
    }
    free(ids);
    drawLoadingScreen(total,total,"Sorting...");
    sortTitles(); needsRedraw=true;
}
void findRelatedTitles(u64 baseTitleID, int selfIdx, int *outIdx, int *outCount) {
    u32 uid=(u32)(baseTitleID&0xFFFFFFFF);
    *outCount=0;
    for (int i=0;i<titleCount;i++) {
        if (i==selfIdx) continue;
        if ((u32)(titles[i].titleID&0xFFFFFFFF)==uid && titles[i].titleID!=baseTitleID && titles[i].isValid) {
            outIdx[(*outCount)++]=i;
            if (*outCount>=20) break;
        }
    }
}
/* ============================================================
   SECTION 8: Backup / Restore / Install
   ============================================================ */
void copyDirectory(FS_Archive archive, const char *srcPath, const char *dstPath) {
    Handle dh; FS_Path fsp=fsMakePath(PATH_ASCII,srcPath);
    if (R_FAILED(FSUSER_OpenDirectory(&dh,archive,fsp))) return;
    createDirectory(dstPath);
    FS_DirectoryEntry ent[32]; u32 er=0;
    while (true) {
        if (R_FAILED(FSDIR_Read(dh,&er,32,ent))||er==0) break;
        for (u32 i=0;i<er;i++) {
            char nm[256]; utf16_to_utf8((uint8_t*)nm,ent[i].name,255); nm[255]='\0';
            char sf[512],df[512];
            snprintf(sf,sizeof(sf),"%s/%s",srcPath,nm);
            snprintf(df,sizeof(df),"%s/%s",dstPath,nm);
            if (ent[i].attributes&FS_ATTRIBUTE_DIRECTORY) { copyDirectory(archive,sf,df); }
            else {
                Handle fh; FS_Path fp2=fsMakePath(PATH_ASCII,sf);
                if (R_SUCCEEDED(FSUSER_OpenFile(&fh,archive,fp2,FS_OPEN_READ,0))) {
                    u64 fsz=0; FSFILE_GetSize(fh,&fsz);
                    if (fsz>0&&fsz<MAX_FILE_SIZE) {
                        void *buf=malloc(fsz);
                        if (buf) {
                            u32 br=0;
                            if (R_SUCCEEDED(FSFILE_Read(fh,&br,0,buf,fsz))&&br==fsz) {
                                FILE *out=fopen(df,"wb");
                                if (out){fwrite(buf,1,br,out);fclose(out);}
                            }
                            free(buf);
                        }
                    }
                    FSFILE_Close(fh);
                }
            }
        }
    }
    FSDIR_Close(dh);
}
void backupArchive(FS_Archive archive, const char *basePath, const char *archiveName) {
    char p[512]; snprintf(p,sizeof(p),"%s/%s",basePath,archiveName);
    createDirectory(p); copyDirectory(archive,"/",p);
}
void backupSaveDataToPath(TitleInfo *title, const char *backupPath) {
    createDirectory(backupPath);
    /* Build new-format dir name: TitleID-Name */
    char dirName[320];
    char cleanName[80]; strncpy(cleanName,title->name,79); cleanName[79]='\0';
    /* strip type suffixes */
    char *p1=strstr(cleanName," Upd"); if(p1)*p1='\0';
    char *p2=strstr(cleanName," DLC"); if(p2)*p2='\0';
    getBackupDirName(title->titleID, cleanName, dirName, sizeof(dirName));
    char backupDir[512];
    snprintf(backupDir,sizeof(backupDir),"%s/%s",backupPath,dirName);
    createDirectory(backupDir);
    /* Timestamp */
    time_t now=time(NULL); struct tm *tm2=localtime(&now);
    char dateBuf[32]="??/??/?? ??:??";
    if (tm2) snprintf(dateBuf,sizeof(dateBuf),"%02d/%02d/%04d %02d:%02d",
        tm2->tm_mday,tm2->tm_mon+1,tm2->tm_year+1900,tm2->tm_hour,tm2->tm_min);
    char infoPath[512]; snprintf(infoPath,sizeof(infoPath),"%s/backup_info.txt",backupDir);
    FILE *info=fopen(infoPath,"w");
    if (info) {
        fprintf(info,"Title ID: %016llX\n",(unsigned long long)title->titleID);
        fprintf(info,"Title Name: %s\n",title->name);
        fprintf(info,"Media Type: %s\n",title->mediaType==MEDIATYPE_SD?"SD":"NAND");
        fprintf(info,"Backup Date: %s\n",dateBuf);
        fprintf(info,"Backup Path: %s\n",backupPath);
        fclose(info);
    }
    u32 ap[]={(u32)(title->titleID&0xFFFFFFFF),(u32)((title->titleID>>32)&0xFFFFFFFF),title->mediaType,0};
    FS_Path bap={PATH_BINARY,16,ap};
    FS_Archive sa;
    if (R_SUCCEEDED(FSUSER_OpenArchive(&sa,ARCHIVE_USER_SAVEDATA,bap))) {
        backupArchive(sa,backupDir,"savedata"); FSUSER_CloseArchive(sa);
    }
    u64 extID=0;
    if (R_SUCCEEDED(AM_GetTitleExtDataId(&extID,title->mediaType,title->titleID)) && extID!=0) {
        FS_ExtSaveDataInfo ei={.mediaType=title->mediaType,.unknown=0,.reserved1=0,.saveId=extID,.reserved2=0};
        FS_Path ep={PATH_BINARY,sizeof(FS_ExtSaveDataInfo),&ei};
        FS_Archive ea;
        if (R_SUCCEEDED(FSUSER_OpenArchive(&ea,ARCHIVE_EXTDATA,ep))) {
            backupArchive(ea,backupDir,"extdata"); FSUSER_CloseArchive(ea);
        }
        if (R_SUCCEEDED(FSUSER_OpenArchive(&ea,ARCHIVE_BOSS_EXTDATA,ep))) {
            backupArchive(ea,backupDir,"boss_extdata"); FSUSER_CloseArchive(ea);
        }
    }
    title->hasBackup=true;
}
void backupSaveData(TitleInfo *title) {
    backupSaveDataToPath(title,config.backupPath);
}
void restoreDirectory(FS_Archive archive, const char *srcPath, const char *dstPath) {
    DIR *d=opendir(srcPath); if (!d) return;
    struct dirent *ent;
    while ((ent=readdir(d))!=NULL) {
        if (strcmp(ent->d_name,".")==0||strcmp(ent->d_name,"..")==0) continue;
        char sf[512],df[512];
        snprintf(sf,sizeof(sf),"%s/%s",srcPath,ent->d_name);
        snprintf(df,sizeof(df),"%s/%s",dstPath,ent->d_name);
        struct stat st2; stat(sf,&st2);
        if (S_ISDIR(st2.st_mode)) {
            FS_Path fp2=fsMakePath(PATH_ASCII,df);
            FSUSER_CreateDirectory(archive,fp2,0);
            restoreDirectory(archive,sf,df);
        } else {
            FILE *inf=fopen(sf,"rb"); if (!inf) continue;
            fseek(inf,0,SEEK_END); long fsz=ftell(inf); fseek(inf,0,SEEK_SET);
            if (fsz>0&&fsz<MAX_FILE_SIZE) {
                void *buf=malloc(fsz);
                if (buf) {
                    if (fread(buf,1,fsz,inf)==(size_t)fsz) {
                        FS_Path fp2=fsMakePath(PATH_ASCII,df); Handle fh2;
                        if (R_SUCCEEDED(FSUSER_OpenFile(&fh2,archive,fp2,FS_OPEN_WRITE|FS_OPEN_CREATE,0))) {
                            u32 bw=0; FSFILE_Write(fh2,&bw,0,buf,fsz,FS_WRITE_FLUSH); FSFILE_Close(fh2);
                        }
                    }
                    free(buf);
                }
            }
            fclose(inf);
        }
    }
    closedir(d);
}
bool restoreSaveData(TitleInfo *title) {
    char backupDir[512];
    if (!findBackupDir(title->titleID,backupDir,sizeof(backupDir))) return false;
    char savedataPath[512]; snprintf(savedataPath,sizeof(savedataPath),"%s/savedata",backupDir);
    struct stat st2; if (stat(savedataPath,&st2)!=0) return false;
    u32 ap[]={(u32)(title->titleID&0xFFFFFFFF),(u32)((title->titleID>>32)&0xFFFFFFFF),title->mediaType,0};
    FS_Path bap={PATH_BINARY,16,ap};
    FS_Archive sa;
    Result res=FSUSER_OpenArchive(&sa,ARCHIVE_USER_SAVEDATA,bap);
    if (R_FAILED(res)) return false;
    restoreDirectory(sa,savedataPath,"/");
    FSUSER_CloseArchive(sa);
    return true;
}
u64 getCIATitleID(const char *ciaPath) {
    FILE *f2=fopen(ciaPath,"rb"); if (!f2) return 0;
    u32 hdrSz=0; fread(&hdrSz,4,1,f2);
    fseek(f2,8,SEEK_SET);
    u32 certSz=0,tikSz=0,tmdSz=0;
    fread(&certSz,4,1,f2); fread(&tikSz,4,1,f2); fread(&tmdSz,4,1,f2);
    if (tmdSz<0x200){fclose(f2);return 0;}
    u32 tmdOff=ALIGN64(hdrSz)+ALIGN64(certSz)+ALIGN64(tikSz);
    fseek(f2,tmdOff,SEEK_SET);
    u32 sigType=0; fread(&sigType,4,1,f2);
    u32 sigSz=0x100,padSz=0x3C;
    if (sigType==0x00010003){sigSz=0x200;}
    else if (sigType==0x00010005){sigSz=0x3C;padSz=0x40;}
    u32 bodyOff=tmdOff+4+sigSz+padSz;
    fseek(f2,bodyOff+0x4C,SEEK_SET);
    u8 tb[8]={0}; fread(tb,8,1,f2); fclose(f2);
    u64 tid=0; for(int i=0;i<8;i++) tid=(tid<<8)|tb[i];
    return tid;
}
int scanDirectory(const char *path) {
    fileCount=0;
    snprintf(currentPath,sizeof(currentPath),"%s",path);
    int plen=(int)strlen(path);
    /* Add ".." entry if not root */
    if (strcmp(path,"sdmc:/")!=0) {
        strcpy(fileEntries[0].name,".."); fileEntries[0].isDir=true;
        fileEntries[0].isCIA=false; fileEntries[0].size=0; fileCount=1;
    }
    /* Build full path helper: avoid double-slash when base ends with '/' */
#define MKFP(fp,base,name) \
    do { int _l=(int)strlen(base); \
         if (_l>0&&(base)[_l-1]=='/') snprintf(fp,sizeof(fp),"%s%s",base,name); \
         else snprintf(fp,sizeof(fp),"%s/%s",base,name); } while(0)

    /* Pass 1: directories (separate opendir — avoid rewinddir issues on CTRU) */
    DIR *d=opendir(path);
    if (d) {
        struct dirent *ent;
        while ((ent=readdir(d))!=NULL&&fileCount<MAX_FILES) {
            if (strcmp(ent->d_name,".")==0||strcmp(ent->d_name,"..")==0) continue;
            bool isDir=(ent->d_type==DT_DIR);
            if (!isDir) {
                char fp2[512]; MKFP(fp2,path,ent->d_name);
                struct stat st2;
                if (stat(fp2,&st2)==0&&S_ISDIR(st2.st_mode)) isDir=true;
            }
            if (!isDir) continue;
            strncpy(fileEntries[fileCount].name,ent->d_name,255);
            fileEntries[fileCount].name[255]='\0';
            fileEntries[fileCount].isDir=true; fileEntries[fileCount].isCIA=false;
            fileEntries[fileCount].size=0; fileCount++;
        }
        closedir(d);
    }
    /* Pass 2: .cia files (new opendir to avoid rewinddir unreliability) */
    d=opendir(path);
    if (d) {
        struct dirent *ent;
        while ((ent=readdir(d))!=NULL&&fileCount<MAX_FILES) {
            char fp2[512]; MKFP(fp2,path,ent->d_name);
            bool isDir=(ent->d_type==DT_DIR);
            if (!isDir) {
                struct stat st2;
                if (stat(fp2,&st2)==0&&S_ISDIR(st2.st_mode)) isDir=true;
            }
            if (isDir) continue;
            const char *ext=strrchr(ent->d_name,'.');
            if (!ext||strcasecmp(ext,".cia")!=0) continue;
            strncpy(fileEntries[fileCount].name,ent->d_name,255);
            fileEntries[fileCount].name[255]='\0';
            fileEntries[fileCount].isDir=false; fileEntries[fileCount].isCIA=true;
            struct stat st2;
            fileEntries[fileCount].size=(stat(fp2,&st2)==0)?(u64)st2.st_size:0;
            fileCount++;
        }
        closedir(d);
    }
#undef MKFP
    (void)plen;
    return fileCount;
}
bool installCIA(const char *ciaPath, FS_MediaType dest) {
    FILE *f2=fopen(ciaPath,"rb"); if (!f2) return false;
    fseek(f2,0,SEEK_END); long fsize=ftell(f2); fseek(f2,0,SEEK_SET);
    if (fsize<=0){fclose(f2);return false;}
    Handle cia;
    if (R_FAILED(AM_StartCiaInstall(dest,&cia))){fclose(f2);return false;}
    u8 *buf=(u8*)malloc(CHUNK_SIZE); if (!buf){AM_CancelCIAInstall(cia);fclose(f2);return false;}
    long done=0; bool ok=true;
    while (done<fsize) {
        int toRead=(int)((fsize-done>CHUNK_SIZE)?CHUNK_SIZE:fsize-done);
        int br=(int)fread(buf,1,toRead,f2);
        if (br<=0){ok=false;break;}
        u32 bw=0;
        if (R_FAILED(FSFILE_Write(cia,&bw,done,buf,br,FS_WRITE_FLUSH))){ok=false;break;}
        done+=br;
        /* progress display */
        {
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
            C2D_TargetClear(top,C2D_Color32(0,0,0,255)); C2D_SceneBegin(top);
            C2D_TextBufClear(dynamicBuf); C2D_Text t;
            C2D_TextParse(&t,dynamicBuf,"Installing..."); C2D_TextOptimize(&t);
            C2D_DrawText(&t,C2D_WithColor,10,20,0.5f,0.55f,0.55f,C2D_Color32(255,255,255,255));
            char nb[256]; int nameStart=(int)(strlen(ciaPath)-1);
            while(nameStart>0&&ciaPath[nameStart-1]!='/'&&ciaPath[nameStart-1]!=':') nameStart--;
            snprintf(nb,sizeof(nb),"%.50s",&ciaPath[nameStart]);
            C2D_TextParse(&t,dynamicBuf,nb); C2D_TextOptimize(&t);
            C2D_DrawText(&t,C2D_WithColor,10,40,0.5f,0.42f,0.42f,C2D_Color32(200,200,200,255));
            float p=(float)done/fsize,bw2=300,bh=20,bx=50,by=80;
            C2D_DrawRectSolid(bx,by,0.5f,bw2,bh,C2D_Color32(50,50,50,255));
            C2D_DrawRectSolid(bx,by,0.5f,bw2*p,bh,C2D_Color32(100,200,100,255));
            C2D_DrawRectSolid(bx,by,0.5f,bw2,2,C2D_Color32(255,255,255,255));
            C2D_DrawRectSolid(bx,by+bh-2,0.5f,bw2,2,C2D_Color32(255,255,255,255));
            char ps[64]; char szDone[32],szTotal[32];
            formatSize(done,szDone,sizeof(szDone)); formatSize(fsize,szTotal,sizeof(szTotal));
            snprintf(ps,sizeof(ps),"%s / %s",szDone,szTotal);
            C2D_TextParse(&t,dynamicBuf,ps); C2D_TextOptimize(&t);
            C2D_DrawText(&t,C2D_WithColor,bx+60,by+25,0.5f,0.42f,0.42f,C2D_Color32(255,255,255,255));
            C2D_TargetClear(bottom,C2D_Color32(20,20,30,255)); C2D_SceneBegin(bottom);
            C3D_FrameEnd(0);
        }
    }
    free(buf); fclose(f2);
    if (!ok){AM_CancelCIAInstall(cia);return false;}
    return R_SUCCEEDED(AM_FinishCiaInstall(cia));
}
void deleteTitleCompletely(TitleInfo *title) {
    u64 extID=0;
    Result res=AM_GetTitleExtDataId(&extID,title->mediaType,title->titleID);
    if (R_SUCCEEDED(res)&&extID!=0) {
        FS_ExtSaveDataInfo ei={.mediaType=title->mediaType,.unknown=0,.reserved1=0,.saveId=extID,.reserved2=0};
        FSUSER_DeleteExtSaveData(ei);
        FS_Archive ba; FS_Path ep={PATH_BINARY,sizeof(FS_ExtSaveDataInfo),&ei};
        if (R_SUCCEEDED(FSUSER_OpenArchive(&ba,ARCHIVE_BOSS_EXTDATA,ep))) FSUSER_CloseArchive(ba);
    }
    AM_DeleteTitle(title->mediaType,title->titleID);
}
void deleteTitle(TitleInfo *title) {
    deleteTitleCompletely(title);
    AM_TitleEntry te;
    Result res=AM_GetTitleInfo(title->mediaType,1,&title->titleID,&te);
    if (R_FAILED(res)||res==0xC8A04478) {
        title->isValid=false;
        const char *sl[]={""," Successfully deleted:",title->name,"","All data removed.","","Press A to continue..."};
        drawDialog(sl,7);
    } else {
        const char *fl[]={""," Failed to delete:",title->name,"","Title may still be present.","","Press A to continue..."};
        drawDialog(fl,7);
    }
    while (aptMainLoop()){hidScanInput();if(hidKeysDown()&KEY_A)break;}
}

/* ============================================================
   SECTION 9: Draw Functions
   ============================================================ */

/* Color palette */
#define CLR_BG        C2D_Color32(20,20,30,255)
#define CLR_HEADER    C2D_Color32(40,60,100,255)
#define CLR_SELECTED  C2D_Color32(60,100,180,255)
#define CLR_WHITE     C2D_Color32(255,255,255,255)
#define CLR_GRAY      C2D_Color32(160,160,160,255)
#define CLR_RED       C2D_Color32(220,60,60,255)
#define CLR_GREEN     C2D_Color32(60,200,60,255)
#define CLR_YELLOW    C2D_Color32(255,220,60,255)
#define CLR_CYAN      C2D_Color32(100,220,255,255)
#define CLR_BACKUP_OK C2D_Color32(255,200,50,255)

/* Helper: draw text via dynamic buffer (buffer must be cleared by caller) */
static void dt(float x, float y, float z, float s, u32 c, const char *str) {
    C2D_Text t;
    C2D_TextParse(&t, dynamicBuf, str);
    C2D_TextOptimize(&t);
    C2D_DrawText(&t, C2D_WithColor, x, y, z, s, s, c);
}

/* Internal: render dialog box content on current scene (top).
   Caller must have called C2D_SceneBegin(top) and C2D_TextBufClear. */
static void _renderDialogBox(const char **lines, int lineCount) {
    float bh = (float)lineCount * 20.0f + 30.0f;
    if (bh < 60.0f) bh = 60.0f;
    float bw = 340.0f;
    float bx = (400.0f - bw) / 2.0f;
    float by = (240.0f - bh) / 2.0f;
    C2D_DrawRectSolid(bx,       by,        0.5f, bw, bh, C2D_Color32(10,10,20,245));
    C2D_DrawRectSolid(bx,       by,        0.5f, bw, 2,  CLR_WHITE);
    C2D_DrawRectSolid(bx,       by+bh-2,   0.5f, bw, 2,  CLR_WHITE);
    C2D_DrawRectSolid(bx,       by,        0.5f, 2,  bh, CLR_WHITE);
    C2D_DrawRectSolid(bx+bw-2,  by,        0.5f, 2,  bh, CLR_WHITE);
    for (int i = 0; i < lineCount; i++) {
        if (lines[i] && lines[i][0])
            dt(bx + 12.0f, by + 12.0f + (float)i * 20.0f, 0.5f, 0.54f, CLR_WHITE, lines[i]);
    }
}

/* Internal: render selected-titles list on current scene (bottom).
   Caller must have called C2D_SceneBegin(bottom). */
static void _renderSelectedList(void) {
    int selCount = 0;
    for (int i = 0; i < titleCount; i++)
        if (titles[i].selected && titles[i].isValid) selCount++;
    char hdr[64];
    snprintf(hdr, sizeof(hdr), "SELECTED TITLES (%d)", selCount);
    C2D_DrawRectSolid(0, 0, 0.5f, 320, 18, CLR_RED);
    dt(4, 2, 0.5f, 0.54f, CLR_WHITE, hdr);
    int shown = 0;
    for (int i = 0; i < titleCount && shown < 10; i++) {
        if (!titles[i].selected || !titles[i].isValid) continue;
        u32 hi = (u32)(titles[i].titleID >> 32);
        const char *sym = (hi == 0x0004000E) ? "^" : (hi == 0x0004008C) ? "+" : " ";
        char shortName[30]; strncpy(shortName, titles[i].name, 28); shortName[28] = '\0';
        char line[48]; snprintf(line, sizeof(line), "%s %s", sym, shortName);
        dt(4, 22.0f + (float)shown * 20.0f, 0.5f, 0.52f, CLR_WHITE, line);
        shown++;
    }
    if (selCount > 10) {
        char more[32]; snprintf(more, sizeof(more), "...and %d more", selCount - 10);
        dt(4, 22.0f + 10.0f * 20.0f, 0.5f, 0.52f, CLR_GRAY, more);
    }
}

/* ---- Public draw functions ---- */

/* drawDialog: blocking context — manages its own C3D frame.
   Draws dialog box on top, clears bottom. */
void drawDialog(const char **lines, int lineCount) {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TextBufClear(dynamicBuf);
    C2D_TargetClear(top, CLR_BG);    C2D_SceneBegin(top);
    _renderDialogBox(lines, lineCount);
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
    C3D_FrameEnd(0);
}

/* drawSelectedTitlesList: standalone — manages its own C3D frame. */
void drawSelectedTitlesList(void) {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TextBufClear(dynamicBuf);
    C2D_TargetClear(top, CLR_BG);    C2D_SceneBegin(top);
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
    _renderSelectedList();
    C3D_FrameEnd(0);
}

/* drawDialogWithSelectedList: blocking context — manages its own C3D frame.
   Dialog on top, selected list on bottom. */
void drawDialogWithSelectedList(const char **lines, int lineCount) {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TextBufClear(dynamicBuf);
    C2D_TargetClear(top, CLR_BG);    C2D_SceneBegin(top);
    _renderDialogBox(lines, lineCount);
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
    _renderSelectedList();
    C3D_FrameEnd(0);
}

/* drawControlsOverlay: called from main loop frame (NO C3D_FrameBegin/End here).
   MUST be the only draw call for this frame — replaces drawUI entirely. */
void drawControlsOverlay(void) {
    C2D_TextBufClear(dynamicBuf);
    /* Top overlay */
    C2D_TargetClear(top, CLR_BG); C2D_SceneBegin(top);
    float bw = 300.0f, bh = 210.0f;
    float bx = (400.0f - bw) / 2.0f, by = (240.0f - bh) / 2.0f;
    C2D_DrawRectSolid(bx,      by,        0.5f, bw, bh, C2D_Color32(10,10,20,230));
    C2D_DrawRectSolid(bx,      by,        0.5f, bw, 2,  CLR_WHITE);
    C2D_DrawRectSolid(bx,      by+bh-2,   0.5f, bw, 2,  CLR_WHITE);
    C2D_DrawRectSolid(bx,      by,        0.5f, 2,  bh, CLR_WHITE);
    C2D_DrawRectSolid(bx+bw-2, by,        0.5f, 2,  bh, CLR_WHITE);
    dt(bx+8, by+6, 0.5f, 0.52f, CLR_CYAN, "=== CONTROLS ===");
    static const char *ctrl[] = {
        "Up/Down   : Navigate list",
        "L/R Pad   : Page -/+",
        "A         : Select title",
        "X         : Run uninstall",
        "L / R     : Change sort",
        "Y         : Change filter",
        "B         : Back to main menu",
        "SELECT    : Show/hide controls"
    };
    for (int i = 0; i < 8; i++)
        dt(bx+8, by+26.0f + (float)i*13.0f, 0.5f, 0.52f, CLR_WHITE, ctrl[i]);
    /* Bottom overlay */
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
    float bbw = 280.0f, bbh = 110.0f;
    float bbx = (320.0f - bbw) / 2.0f, bby = (240.0f - bbh) / 2.0f;
    C2D_DrawRectSolid(bbx,       bby,        0.5f, bbw, bbh, C2D_Color32(10,10,20,230));
    C2D_DrawRectSolid(bbx,       bby,        0.5f, bbw, 2,   CLR_WHITE);
    C2D_DrawRectSolid(bbx,       bby+bbh-2,  0.5f, bbw, 2,   CLR_WHITE);
    C2D_DrawRectSolid(bbx,       bby,        0.5f, 2,   bbh, CLR_WHITE);
    C2D_DrawRectSolid(bbx+bbw-2, bby,        0.5f, 2,   bbh, CLR_WHITE);
    dt(bbx+8, bby+8,  0.5f, 0.52f, CLR_CYAN,  "Touch screen");
    dt(bbx+8, bby+28, 0.5f, 0.52f, CLR_GRAY,  "Selected title details are");
    dt(bbx+8, bby+48, 0.5f, 0.52f, CLR_GRAY,  "visible on the bottom screen");
    dt(bbx+8, bby+68, 0.5f, 0.52f, CLR_GRAY,  "while navigating.");
    dt(bbx+8, bby+88, 0.5f, 0.52f, CLR_WHITE, "Release SELECT to return.");
}

/* drawTouchControls: draws title details on bottom screen.
   Called from drawUI (NO frame management, NO TextBufClear). */
void drawTouchControls(void) {
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
    if (filteredCount == 0) {
        dt(8, 110, 0.5f, 0.54f, CLR_GRAY, "No titles to show.");
        return;
    }
    int idx = filteredIndices[cursor];
    TitleInfo *ti = &titles[idx];
    C2D_DrawRectSolid(0, 0, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 2, 0.5f, 0.54f, CLR_WHITE, "TITLE DETAILS");
    /* Name */
    char nm[46]; strncpy(nm, ti->fullName[0] ? ti->fullName : ti->name, 44); nm[44] = '\0';
    dt(4, 22, 0.5f, 0.52f, CLR_WHITE, nm);
    /* TitleID */
    char tid[32]; snprintf(tid, sizeof(tid), "ID: %016llX", (unsigned long long)ti->titleID);
    dt(4, 42, 0.5f, 0.54f, CLR_GRAY, tid);
    /* Version + Size */
    char ver[16]; snprintf(ver, sizeof(ver), "v%u", ti->version);
    char sz[24];  formatSize(ti->size, sz, sizeof(sz));
    char vs[48];  snprintf(vs, sizeof(vs), "%s   %s", ver, sz);
    dt(4, 60, 0.5f, 0.54f, CLR_GRAY, vs);
    /* Type + Location */
    u32 hi = (u32)(ti->titleID >> 32);
    const char *type = (hi == 0x0004000E) ? "Update" : (hi == 0x0004008C) ? "DLC" : "Game";
    const char *loc  = (ti->mediaType == MEDIATYPE_SD) ? "SD" : "NAND";
    char tl[32]; snprintf(tl, sizeof(tl), "%s  |  %s", type, loc);
    dt(4, 78, 0.5f, 0.52f, CLR_CYAN, tl);
    /* Backup */
    if (ti->hasBackup) {
        char dateBuf[32]; getBackupLastDate(ti->titleID, dateBuf, sizeof(dateBuf));
        char bk[64]; snprintf(bk, sizeof(bk), "Backup: YES [%s]", dateBuf);
        dt(4, 96, 0.5f, 0.54f, CLR_GREEN, bk);
    } else {
        dt(4, 96, 0.5f, 0.54f, CLR_RED, "Backup: NO");
    }
    C2D_DrawRectSolid(0, 222, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_GRAY, "[SELECT = Controls]");
}

/* drawMainMenu: called from main loop — NO C3D frame management. */
void drawMainMenu(void) {
    C2D_TextBufClear(dynamicBuf);
    /* Top screen */
    C2D_TargetClear(top, CLR_BG); C2D_SceneBegin(top);
    C2D_DrawRectSolid(0, 0, 0.5f, 400, 22, CLR_HEADER);
    dt(8, 4, 0.5f, 0.54f, CLR_WHITE, "3DS Fast Uninstall  v2.0");
    static const char *items[] = {
        "[A] Install CIA",
        "[B] Backup Saves",
        "[U] Uninstall Titles",
        "[I] System Info",
        "[S] Settings"
    };
    for (int i = 0; i < 5; i++) {
        float y = 60.0f + (float)i * 30.0f;
        if (i == menuCursor)
            C2D_DrawRectSolid(0, y - 2, 0.5f, 400, 24, CLR_SELECTED);
        dt(30, y, 0.5f, 0.50f, (i == menuCursor) ? CLR_WHITE : CLR_GRAY, items[i]);
        if (i == menuCursor)
            dt(12, y, 0.5f, 0.54f, CLR_WHITE, ">");
    }
    C2D_DrawRectSolid(0, 222, 0.5f, 400, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_WHITE, "START = Exit");
    /* Bottom screen */
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
    static const char *descLine1[] = {
        "Install CIA files from SD card.",
        "Create save data backups",
        "Uninstall installed titles",
        "View info about installed titles",
        "Configure application options"
    };
    static const char *descLine2[] = {
        "Supports SD and NAND titles.",
        "for installed titles.",
        "with optional backup.",
        "(games, DLC, updates).",
        ""
    };
    C2D_DrawRectSolid(0, 0, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 2, 0.5f, 0.54f, CLR_WHITE, "Description");
    dt(8, 30, 0.5f, 0.54f, CLR_WHITE, descLine1[menuCursor]);
    dt(8, 52, 0.5f, 0.52f, CLR_GRAY,  descLine2[menuCursor]);
    C2D_DrawRectSolid(0, 222, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_WHITE, "A = Select   START = Exit");
}

/* drawUI: uninstall list — called from main loop, NO C3D frame management. */
void drawUI(void) {
    C2D_TextBufClear(dynamicBuf);
    /* Top screen */
    C2D_TargetClear(top, CLR_BG); C2D_SceneBegin(top);
    C2D_DrawRectSolid(0, 0, 0.5f, 400, 22, CLR_HEADER);
    dt(4, 4, 0.5f, 0.52f, CLR_WHITE, "Uninstall");
    /* Info bar */
    int selCount = 0;
    for (int i = 0; i < titleCount; i++)
        if (titles[i].selected && titles[i].isValid) selCount++;
    static const char *sortNames[]   = { "Name", "Size", "ID" };
    static const char *filterNames[] = { "All", "Upd", "DLC" };
    char info[80];
    snprintf(info, sizeof(info), "T:%d  Sel:%d  Sort:%s  [%s]",
             filteredCount, selCount,
             sortNames[currentSortMode], filterNames[currentFilterMode]);
    dt(4, 24, 0.5f, 0.52f, CLR_GRAY, info);
    /* Title list */
    for (int i = 0; i < MAX_VISIBLE_TITLES; i++) {
        int fi = scrollOffset + i;
        if (fi >= filteredCount) break;
        int idx = filteredIndices[fi];
        TitleInfo *ti = &titles[idx];
        float y = 38.0f + (float)i * 14.5f;
        bool isCursor = (fi == cursor);
        if (isCursor)
            C2D_DrawRectSolid(0, y - 1, 0.5f, 400, 15, CLR_SELECTED);
        /* Checkbox */
        u32 txtColor = ti->selected ? CLR_YELLOW : (isCursor ? CLR_WHITE : CLR_GRAY);
        dt(3, y, 0.5f, 0.38f, txtColor, ti->selected ? "[X]" : "[ ]");
        /* Name (truncated) */
        const char *srcName = ti->name[0] ? ti->name : "Unknown";
        char nm[32]; strncpy(nm, srcName, 28); nm[28] = '\0';
        if (strlen(srcName) > 28) { nm[25]='.'; nm[26]='.'; nm[27]='.'; nm[28]='\0'; }
        dt(28, y, 0.5f, 0.38f, txtColor, nm);
        /* Type symbol */
        u32 hi = (u32)(ti->titleID >> 32);
        if      (hi == 0x0004000E) dt(220, y, 0.5f, 0.44f, CLR_CYAN,  "^");
        else if (hi == 0x0004008C) dt(220, y, 0.5f, 0.44f, CLR_GREEN, "+");
        /* Full TitleID (16 hex chars) */
        char fullID[20];
        snprintf(fullID, sizeof(fullID), "%016llX", (unsigned long long)ti->titleID);
        dt(230, y, 0.5f, 0.40f, CLR_GRAY, fullID);
    }
    /* Scroll counter */
    if (filteredCount > MAX_VISIBLE_TITLES) {
        char sc[16]; snprintf(sc, sizeof(sc), "%d/%d", cursor + 1, filteredCount);
        dt(340, 225, 0.5f, 0.52f, CLR_GRAY, sc);
    }
    C2D_DrawRectSolid(0, 222, 0.5f, 400, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_WHITE, "A=Sel  X=Delete  L/R=Sort  Y=Filt  B=Menu  SEL=Help");
    /* Bottom screen via sub-function (no TextBufClear here) */
    drawTouchControls();
}

/* ---- SOON stubs ---- */
static void _drawSoon(const char *feature) {
    /* NO C3D frame mgmt — works both inside blocking loops and main loop frames */
    C2D_TextBufClear(dynamicBuf);
    C2D_TargetClear(top, CLR_BG); C2D_SceneBegin(top);
    C2D_DrawRectSolid(0, 0, 0.5f, 400, 22, CLR_HEADER);
    dt(8, 4, 0.5f, 0.54f, CLR_WHITE, feature);
    dt(110, 95, 0.5f, 0.65f, CLR_YELLOW, "COMING SOON");
    dt(80, 135, 0.5f, 0.54f, CLR_GRAY, "Feature under development (v2.0).");
    C2D_DrawRectSolid(0, 222, 0.5f, 400, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_WHITE, "B / START = Back to menu");
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
    dt(8, 108, 0.5f, 0.54f, CLR_GRAY, "Version 2.0 coming!");
}

/* drawFileBrowserScreen: file browser for CIA install.
   Called from runInstallFlow — NO C3D frame management here. */
void drawFileBrowserScreen(void) {
    C2D_TextBufClear(dynamicBuf);
    /* Top screen */
    C2D_TargetClear(top, CLR_BG); C2D_SceneBegin(top);
    C2D_DrawRectSolid(0, 0, 0.5f, 400, 22, CLR_HEADER);
    dt(4, 4, 0.5f, 0.54f, CLR_WHITE, "Install CIA");
    /* Path (show tail if long) */
    const char *dispPath = currentPath;
    char pathBuf[52];
    int plen = (int)strlen(currentPath);
    if (plen > 44) {
        snprintf(pathBuf, sizeof(pathBuf), "...%s", currentPath + plen - 40);
        dispPath = pathBuf;
    }
    dt(4, 24, 0.5f, 0.52f, CLR_GRAY, dispPath);
    /* File list */
    for (int i = 0; i < MAX_VISIBLE_TITLES; i++) {
        int fi = fileScrollOffset + i;
        if (fi >= fileCount) break;
        float y = 38.0f + (float)i * 14.5f;
        bool isCursor = (fi == fileCursor);
        if (isCursor)
            C2D_DrawRectSolid(0, y - 1, 0.5f, 400, 15, CLR_SELECTED);
        FileEntry *fe = &fileEntries[fi];
        if (fe->isDir) {
            char dirLine[52];
            snprintf(dirLine, sizeof(dirLine), "[DIR] %s/", fe->name);
            if (strlen(dirLine) > 46) { dirLine[43]='.'; dirLine[44]='.'; dirLine[45]='.'; dirLine[46]='\0'; }
            dt(4, y, 0.5f, 0.38f, isCursor ? CLR_WHITE : CLR_CYAN, dirLine);
        } else {
            char szBuf[16]; formatSize(fe->size, szBuf, sizeof(szBuf));
            char nm[28]; strncpy(nm, fe->name, 26); nm[26]='\0';
            if (strlen(fe->name) > 26) { nm[23]='.'; nm[24]='.'; nm[25]='.'; nm[26]='\0'; }
            char ciaLine[56]; snprintf(ciaLine, sizeof(ciaLine), "%-26s  [%s]", nm, szBuf);
            dt(4, y, 0.5f, 0.38f, isCursor ? CLR_WHITE : CLR_GRAY, ciaLine);
        }
    }
    /* Scroll counter */
    if (fileCount > MAX_VISIBLE_TITLES) {
        char sc[16]; snprintf(sc, sizeof(sc), "%d/%d", fileCursor+1, fileCount);
        dt(340, 225, 0.5f, 0.52f, CLR_GRAY, sc);
    }
    C2D_DrawRectSolid(0, 222, 0.5f, 400, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_WHITE, "A=Enter/Install  Y=All  B=Up/Menu  START=Cancel");
    /* Bottom screen: info file selezionato */
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
    C2D_DrawRectSolid(0, 0, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 2, 0.5f, 0.54f, CLR_WHITE, "Info");
    if (fileCount > 0 && fileCursor < fileCount) {
        FileEntry *fe = &fileEntries[fileCursor];
        if (fe->isDir) {
            if (strcmp(fe->name, "..") == 0) {
                dt(8, 26, 0.5f, 0.52f, CLR_CYAN, "Go to parent folder");
            } else {
                char dline[48]; snprintf(dline, sizeof(dline), "Folder: %.30s", fe->name);
                dt(8, 26, 0.5f, 0.52f, CLR_CYAN, dline);
            }
        } else {
            char fullCIAPath[512];
            snprintf(fullCIAPath, sizeof(fullCIAPath), "%s/%s", currentPath, fe->name);
            u64 tid = getCIATitleID(fullCIAPath);
            char tidStr[36];
            if (tid) snprintf(tidStr, sizeof(tidStr), "ID: %016llX", (unsigned long long)tid);
            else     snprintf(tidStr, sizeof(tidStr), "ID: N/A");
            dt(8, 26, 0.5f, 0.54f, CLR_WHITE, tidStr);
            char szBuf[24]; formatSize(fe->size, szBuf, sizeof(szBuf));
            char szLine[36]; snprintf(szLine, sizeof(szLine), "Size: %s", szBuf);
            dt(8, 46, 0.5f, 0.54f, CLR_GRAY, szLine);
        }
    } else {
        dt(8, 50, 0.5f, 0.52f, CLR_GRAY, "Empty folder");
    }
    C2D_DrawRectSolid(0, 222, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_GRAY, "All installs go to SD card");
}

/* drawBackupScreen: list of ALL titles with backup status.
   Called from runBackupFlow — NO C3D frame management. */
void drawBackupScreen(void) {
    C2D_TextBufClear(dynamicBuf);
    /* Top screen */
    C2D_TargetClear(top, CLR_BG); C2D_SceneBegin(top);
    C2D_DrawRectSolid(0, 0, 0.5f, 400, 22, CLR_HEADER);
    int selCount = 0;
    for (int i = 0; i < titleCount; i++)
        if (titles[i].selected) selCount++;
    char hdr[56]; snprintf(hdr, sizeof(hdr), "Save Backups   Sel:%d", selCount);
    dt(4, 4, 0.5f, 0.54f, CLR_WHITE, hdr);
    /* Title list — direct index into titles[], NOT filteredIndices[] */
    for (int i = 0; i < MAX_VISIBLE_TITLES; i++) {
        int ti = backupScrollOffset + i;
        if (ti >= titleCount) break;
        TitleInfo *t2 = &titles[ti];
        float y = 38.0f + (float)i * 14.5f;
        bool isCursor = (ti == backupCursor);
        if (isCursor)
            C2D_DrawRectSolid(0, y - 1, 0.5f, 400, 15, CLR_SELECTED);
        /* Checkbox — selection takes priority over backup indicator */
        const char *cb; u32 cbColor;
        if (t2->selected)        { cb = "[X]"; cbColor = CLR_YELLOW; }
        else if (t2->hasBackup)  { cb = "[*]"; cbColor = CLR_BACKUP_OK; }
        else                     { cb = "[ ]"; cbColor = isCursor ? CLR_WHITE : CLR_GRAY; }
        dt(3, y, 0.5f, 0.38f, cbColor, cb);
        /* Name */
        const char *srcName = t2->name[0] ? t2->name : "Unknown";
        char nm[36]; strncpy(nm, srcName, 34); nm[34] = '\0';
        if (strlen(srcName) > 34) { nm[31]='.'; nm[32]='.'; nm[33]='.'; nm[34]='\0'; }
        u32 nameColor = t2->selected ? CLR_YELLOW : (isCursor ? CLR_WHITE : CLR_GRAY);
        dt(28, y, 0.5f, 0.38f, nameColor, nm);
        /* Type tag */
        u32 hi = (u32)(t2->titleID >> 32);
        if      (hi == 0x0004000E) dt(340, y, 0.5f, 0.52f, CLR_CYAN,  "Upd");
        else if (hi == 0x0004008C) dt(340, y, 0.5f, 0.52f, CLR_GREEN, "DLC");
    }
    /* Scroll counter */
    if (titleCount > MAX_VISIBLE_TITLES) {
        char sc[16]; snprintf(sc, sizeof(sc), "%d/%d", backupCursor+1, titleCount);
        dt(352, 225, 0.5f, 0.52f, CLR_GRAY, sc);
    }
    C2D_DrawRectSolid(0, 222, 0.5f, 400, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_WHITE, "A=Sel  X=Backup Sel  Y=Backup All  B=Menu");
    /* Bottom screen: details for backupCursor title */
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
    C2D_DrawRectSolid(0, 0, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 2, 0.5f, 0.54f, CLR_WHITE, "Title Info");
    if (backupCursor < titleCount) {
        TitleInfo *ti = &titles[backupCursor];
        char nm2[46]; strncpy(nm2, ti->fullName[0] ? ti->fullName : ti->name, 44); nm2[44]='\0';
        dt(4, 22, 0.5f, 0.52f, CLR_WHITE, nm2);
        char tidStr[32]; snprintf(tidStr, sizeof(tidStr), "%016llX", (unsigned long long)ti->titleID);
        dt(4, 42, 0.5f, 0.52f, CLR_GRAY, tidStr);
        char szBuf[24]; formatSize(ti->size, szBuf, sizeof(szBuf));
        char locLine[36];
        snprintf(locLine, sizeof(locLine), "%s  |  %s", szBuf,
                 (ti->mediaType == MEDIATYPE_SD) ? "SD" : "NAND");
        dt(4, 60, 0.5f, 0.52f, CLR_GRAY, locLine);
        /* Backup status — only 1 file read per frame (cursor item only) */
        if (ti->hasBackup) {
            char dateBuf[32]; getBackupLastDate(ti->titleID, dateBuf, sizeof(dateBuf));
            char bkLine[52]; snprintf(bkLine, sizeof(bkLine), "Backup: %s", dateBuf);
            dt(4, 80, 0.5f, 0.54f, CLR_GREEN, bkLine);
        } else {
            dt(4, 80, 0.5f, 0.54f, CLR_RED, "No backup");
        }
    }
    C2D_DrawRectSolid(0, 222, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_GRAY, "X=Backup sel   Y=Backup all");
}
/* SysInfo detail view state — set by _runSysInfoDetailFlow before drawTitleDetails */
static int sysInfoDetailIdx    = 0;
static int sysInfoDetailCursor = 0;

/* drawSysInfoScreen: system info — NO C3D frame management (main loop). */
void drawSysInfoScreen(void) {
    C2D_TextBufClear(dynamicBuf);
    C2D_TargetClear(top, CLR_BG); C2D_SceneBegin(top);

    if (sysInfoMode == SYSINFO_OVERVIEW) {
        C2D_DrawRectSolid(0, 0, 0.5f, 400, 22, CLR_HEADER);
        dt(4, 4, 0.5f, 0.54f, CLR_WHITE, "SYSTEM INFORMATION");
        /* Calculate totals at runtime */
        int cntG=0, cntU=0, cntD=0;
        u64 szG=0, szU=0, szD=0;
        for (int i = 0; i < titleCount; i++) {
            if (!titles[i].isValid) continue;
            u32 hi = (u32)(titles[i].titleID >> 32);
            if      (hi == 0x00040000) { cntG++; szG += titles[i].size; }
            else if (hi == 0x0004000E) { cntU++; szU += titles[i].size; }
            else if (hi == 0x0004008C) { cntD++; szD += titles[i].size; }
        }
        char szBufG[24], szBufU[24], szBufD[24];
        formatSize(szG, szBufG, sizeof(szBufG));
        formatSize(szU, szBufU, sizeof(szBufU));
        formatSize(szD, szBufD, sizeof(szBufD));
        /* 3 navigable rows */
        const char *rowLabels[] = { "Games:   ", "Updates: ", "DLC:     " };
        int   rowCnt[] = { cntG, cntU, cntD };
        const char *rowSz[]  = { szBufG, szBufU, szBufD };
        for (int i = 0; i < 3; i++) {
            float y = 50.0f + (float)i * 36.0f;
            if (i == sysInfoCursor)
                C2D_DrawRectSolid(0, y - 2, 0.5f, 400, 30, CLR_SELECTED);
            char line[64];
            snprintf(line, sizeof(line), "  %-9s%3d    %s", rowLabels[i], rowCnt[i], rowSz[i]);
            dt(8, y + 6, 0.5f, 0.45f, (i == sysInfoCursor) ? CLR_WHITE : CLR_GRAY, line);
        }
        /* Separator + SD free space */
        C2D_DrawRectSolid(8, 162, 0.5f, 384, 1, CLR_GRAY);
        u64 freeB=0, totalB=0; getSDFreeSpace(&freeB, &totalB);
        char szFree[24], szTot[24];
        formatSize(freeB,  szFree, sizeof(szFree));
        formatSize(totalB, szTot,  sizeof(szTot));
        char sdLine[64]; snprintf(sdLine, sizeof(sdLine), "  SD Free: %s / %s", szFree, szTot);
        dt(8, 170, 0.5f, 0.52f, CLR_CYAN, sdLine);
        C2D_DrawRectSolid(0, 222, 0.5f, 400, 18, CLR_HEADER);
        dt(4, 224, 0.5f, 0.52f, CLR_WHITE, "A=Enter category   B=Menu");
        /* Bottom: hint */
        C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
        C2D_DrawRectSolid(0, 0, 0.5f, 320, 18, CLR_HEADER);
        dt(4, 2, 0.5f, 0.54f, CLR_WHITE, "Overview");
        dt(8, 26, 0.5f, 0.52f, CLR_GRAY, "Select a category with A");
        dt(8, 44, 0.5f, 0.52f, CLR_GRAY, "to see the title list,");
        dt(8, 62, 0.5f, 0.52f, CLR_GRAY, "backup, restore or delete.");
    } else {
        /* Sublist: GIOCHI / UPDATE / DLC */
        const char *hdrLabel =
            (sysInfoMode == SYSINFO_GAMES)   ? "GAMES"   :
            (sysInfoMode == SYSINFO_UPDATES) ? "UPDATES" : "DLC";
        char hdr[32]; snprintf(hdr, sizeof(hdr), "%s  (%d)", hdrLabel, sysInfoSubCount);
        C2D_DrawRectSolid(0, 0, 0.5f, 400, 22, CLR_HEADER);
        dt(4, 4, 0.5f, 0.52f, CLR_WHITE, hdr);
        for (int i = 0; i < MAX_VISIBLE_TITLES; i++) {
            int si = sysInfoSubScrollOffset + i;
            if (si >= sysInfoSubCount) break;
            int idx = sysInfoSubIndices[si];
            TitleInfo *ti = &titles[idx];
            float y = 28.0f + (float)i * 14.5f;
            bool isCursor = (si == sysInfoSubCursor);
            if (isCursor)
                C2D_DrawRectSolid(0, y - 1, 0.5f, 400, 15, CLR_SELECTED);
            char nm[34]; strncpy(nm, ti->name[0] ? ti->name : "Unknown", 32); nm[32]='\0';
            if (strlen(ti->name) > 32) { nm[29]='.'; nm[30]='.'; nm[31]='.'; nm[32]='\0'; }
            char szBuf[16]; formatSize(ti->size, szBuf, sizeof(szBuf));
            char szLine[20]; snprintf(szLine, sizeof(szLine), "[%s]", szBuf);
            u32 clr = isCursor ? CLR_WHITE : CLR_GRAY;
            dt(4, y, 0.5f, 0.36f, clr, nm);
            /* Right-align size: stima ~5.4px per char a scala 0.36f */
            float szX = 396.0f - (float)strlen(szLine) * 5.4f;
            dt(szX, y, 0.5f, 0.36f, CLR_GRAY, szLine);
        }
        if (sysInfoSubCount > MAX_VISIBLE_TITLES) {
            char sc[16]; snprintf(sc, sizeof(sc), "%d/%d", sysInfoSubCursor+1, sysInfoSubCount);
            dt(352, 225, 0.5f, 0.52f, CLR_GRAY, sc);
        }
        C2D_DrawRectSolid(0, 222, 0.5f, 400, 18, CLR_HEADER);
        dt(4, 224, 0.5f, 0.52f, CLR_WHITE, "A=Details & actions   B=Back");
        /* Bottom: simplified details for cursor item */
        C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
        C2D_DrawRectSolid(0, 0, 0.5f, 320, 18, CLR_HEADER);
        dt(4, 2, 0.5f, 0.54f, CLR_WHITE, "Info");
        if (sysInfoSubCount > 0 && sysInfoSubCursor < sysInfoSubCount) {
            int idx = sysInfoSubIndices[sysInfoSubCursor];
            TitleInfo *ti = &titles[idx];
            char nm2[46]; strncpy(nm2, ti->fullName[0] ? ti->fullName : ti->name, 44); nm2[44]='\0';
            dt(4, 22, 0.5f, 0.52f, CLR_WHITE, nm2);
            char tidStr[32]; snprintf(tidStr, sizeof(tidStr), "%016llX", (unsigned long long)ti->titleID);
            dt(4, 40, 0.5f, 0.52f, CLR_GRAY, tidStr);
            char szBuf[24]; formatSize(ti->size, szBuf, sizeof(szBuf));
            char loc[36]; snprintf(loc, sizeof(loc), "%s  |  %s", szBuf,
                                   (ti->mediaType == MEDIATYPE_SD) ? "SD" : "NAND");
            dt(4, 58, 0.5f, 0.52f, CLR_GRAY, loc);
            if (ti->hasBackup) {
                char dateBuf[32]; getBackupLastDate(ti->titleID, dateBuf, sizeof(dateBuf));
                char bkLine[48]; snprintf(bkLine, sizeof(bkLine), "Backup: %s", dateBuf);
                dt(4, 76, 0.5f, 0.52f, CLR_GREEN, bkLine);
            } else {
                dt(4, 76, 0.5f, 0.52f, CLR_RED, "No backup");
            }
            dt(4, 96, 0.5f, 0.52f, CLR_CYAN, "A = Details & actions");
        }
    }
}
/* drawSettingsScreen: settings UI — NO C3D frame management (main loop). */
void drawSettingsScreen(void) {
    C2D_TextBufClear(dynamicBuf);
    C2D_TargetClear(top, CLR_BG); C2D_SceneBegin(top);
    C2D_DrawRectSolid(0, 0, 0.5f, 400, 22, CLR_HEADER);
    dt(4, 4, 0.5f, 0.54f, CLR_WHITE, "SETTINGS");

    static const char *labels[] = {
        "Force Backup:",
        "Skip Uninstall Confirm:",
        "Force Restore:",
        "Skip Install Confirm:",
        "Backup Folder:"
    };
    /* Build value strings — booleans only; path shown on bottom screen */
    const char *values[5] = {
        config.forceBackup          ? "ON"   : "OFF",
        config.skipUninstallConfirm ? "ON"   : "OFF",
        config.forceRestore         ? "ON"   : "OFF",
        config.skipInstallConfirm   ? "ON"   : "OFF",
        "<see bottom>"
    };

    for (int i = 0; i < 5; i++) {
        float y = 38.0f + (float)i * 32.0f;
        if (i == settingsCursor)
            C2D_DrawRectSolid(0, y - 2, 0.5f, 400, 26, CLR_SELECTED);
        if (i == settingsCursor)
            dt(12, y + 2, 0.5f, 0.50f, CLR_WHITE, ">");
        dt(28, y + 2, 0.5f, 0.50f, (i == settingsCursor) ? CLR_WHITE : CLR_GRAY, labels[i]);
        if (i < 4)
            dt(260, y + 2, 0.5f, 0.50f, CLR_CYAN, values[i]);
    }
    C2D_DrawRectSolid(0, 222, 0.5f, 400, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_WHITE, "A/R/L=Change  DX/SX=Change  B/START=Salva & Esci");

    /* Bottom screen: contextual description */
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
    C2D_DrawRectSolid(0, 0, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 2, 0.5f, 0.54f, CLR_WHITE, "Description");

    static const char *desc[5][3] = {
        { "Backup automatico prima di ogni",   "uninstall (nessuna conferma).",      "Consigliato: ON per sicurezza." },
        { "Salta la conferma di cancellazione","Elimina subito alla pressione X.",   "Consigliato: OFF (conferma sempre)." },
        { "Ripristino save automatico",        "dopo ogni install CIA riuscita.",    "Usare solo con backup aggiornati." },
        { "Salta la conferma di installazione","Installa il CIA immediatamente.",    "Consigliato: OFF (conferma sempre)." },
        { "Folder for save backups.",          "Use Left/Right or L/R to cycle",    "through 5 available destinations." }
    };
    if (settingsCursor == 4) {
        /* Show full path on bottom screen */
        dt(8, 28, 0.5f, 0.52f, CLR_WHITE, "Current path:");
        dt(8, 46, 0.5f, 0.44f, CLR_CYAN,  config.backupPath);
        dt(8, 66, 0.5f, 0.50f, CLR_GRAY,  desc[4][1]);
        dt(8, 84, 0.5f, 0.50f, CLR_GRAY,  desc[4][2]);
    } else {
        dt(8, 28, 0.5f, 0.52f, CLR_WHITE, desc[settingsCursor][0]);
        dt(8, 48, 0.5f, 0.52f, CLR_GRAY,  desc[settingsCursor][1]);
        dt(8, 68, 0.5f, 0.54f, CLR_CYAN,  desc[settingsCursor][2]);
    }

    C2D_DrawRectSolid(0, 204, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 206, 0.5f, 0.54f, CLR_GREEN, "Changes saved in real time.");
    C2D_DrawRectSolid(0, 222, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_GRAY, "B/START = save & back to menu");
}
/* drawTitleDetails: detail view for a single title (SysInfo flow).
   Called from _runSysInfoDetailFlow within C3D_FrameBegin/End — NO frame mgmt. */
void drawTitleDetails(void) {
    C2D_TextBufClear(dynamicBuf);
    C2D_TargetClear(top, CLR_BG); C2D_SceneBegin(top);
    int idx = sysInfoDetailIdx;
    TitleInfo *ti = &titles[idx];
    C2D_DrawRectSolid(0, 0, 0.5f, 400, 22, CLR_HEADER);
    dt(4, 4, 0.5f, 0.54f, CLR_WHITE, "Title Details");
    /* Info */
    char nm[50]; strncpy(nm, ti->fullName[0] ? ti->fullName : ti->name, 48); nm[48]='\0';
    dt(4, 28, 0.5f, 0.52f, CLR_WHITE, nm);
    char tidLine[36]; snprintf(tidLine, sizeof(tidLine), "ID: %016llX", (unsigned long long)ti->titleID);
    dt(4, 48, 0.5f, 0.52f, CLR_GRAY, tidLine);
    char szBuf[24]; formatSize(ti->size, szBuf, sizeof(szBuf));
    char szLine[52]; snprintf(szLine, sizeof(szLine), "v%u   %s   %s",
                              ti->version, szBuf, (ti->mediaType == MEDIATYPE_SD) ? "SD" : "NAND");
    dt(4, 64, 0.5f, 0.52f, CLR_GRAY, szLine);
    /* Backup status */
    if (ti->hasBackup) {
        char dateBuf[32]; getBackupLastDate(ti->titleID, dateBuf, sizeof(dateBuf));
        char bkLine[52]; snprintf(bkLine, sizeof(bkLine), "Backup: %s", dateBuf);
        dt(4, 80, 0.5f, 0.52f, CLR_GREEN, bkLine);
    } else {
        dt(4, 80, 0.5f, 0.52f, CLR_RED, "No backup");
    }
    /* Related titles (only for base games) */
    u32 hi = (u32)(ti->titleID >> 32);
    int relIdx[20]; int relCount = 0;
    if (hi == 0x00040000) findRelatedTitles(ti->titleID, idx, relIdx, &relCount);
    float actY = 100.0f;
    if (relCount > 0) {
        dt(4, 96, 0.5f, 0.52f, CLR_CYAN, "Related:");
        for (int r = 0; r < relCount && r < 3; r++) {
            u32 rhi = (u32)(titles[relIdx[r]].titleID >> 32);
            char rl[42]; snprintf(rl, sizeof(rl), "  %s %.32s",
                                  (rhi == 0x0004000E) ? "[Upd]" : "[DLC]",
                                  titles[relIdx[r]].name);
            dt(4, 108.0f + (float)r * 13.0f, 0.5f, 0.52f, CLR_GRAY, rl);
        }
        actY = 148.0f;
    }
    /* 3 action options */
    static const char *actions[] = {
        "[A] Backup Save Data",
        "[Y] Restore Save Data",
        "[X] Delete Title (+ related)"
    };
    for (int a = 0; a < 3; a++) {
        float y = actY + (float)a * 19.0f;
        if (a == sysInfoDetailCursor)
            C2D_DrawRectSolid(0, y - 1, 0.5f, 400, 18, CLR_SELECTED);
        dt(8, y, 0.5f, 0.38f, (a == sysInfoDetailCursor) ? CLR_WHITE : CLR_GRAY, actions[a]);
    }
    C2D_DrawRectSolid(0, 222, 0.5f, 400, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.54f, CLR_WHITE, "Up/Down=Nav  A=Execute  B=Back");
    /* Bottom: description of selected action */
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
    C2D_DrawRectSolid(0, 0, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 2, 0.5f, 0.54f, CLR_WHITE, "Action");
    static const char *actDesc[3][2] = {
        { "Save the title data",              "to the configured backup folder." },
        { "Restore data from backup",         "previously created." },
        { "Delete the title from the system", "including DLC, Updates and data." }
    };
    dt(8, 28, 0.5f, 0.52f, CLR_WHITE, actDesc[sysInfoDetailCursor][0]);
    dt(8, 48, 0.5f, 0.52f, CLR_GRAY,  actDesc[sysInfoDetailCursor][1]);
    C2D_DrawRectSolid(0, 222, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_GRAY, "A = confirm action");
}

/* ============================================================
   SECTION 10: Input / Flow Handlers
   ============================================================ */

void handleMainMenuInput(void) {
    u32 keys = hidKeysDown();
    if (keys & KEY_DOWN) menuCursor = (menuCursor + 1) % 5;
    if (keys & KEY_UP)   menuCursor = (menuCursor + 4) % 5;
    if (keys & KEY_A) {
        switch (menuCursor) {
            case 0: /* Install CIA */
                appState = APP_INSTALL;
                break;
            case 1: /* Backup */
                /* loadTitles() is called in main() BEFORE C3D_FrameBegin */
                appState = APP_BACKUP;
                break;
            case 2: /* Uninstall */
                cursor = 0; scrollOffset = 0;
                /* Se i titoli sono già caricati, aggiorna subito la lista filtrata
                   (evita lista vuota quando si arriva da Backup senza reload) */
                if (titleCount > 0) updateFilteredList();
                appState = APP_UNINSTALL;
                break;
            case 3: /* SysInfo */
                sysInfoMode = SYSINFO_OVERVIEW;
                sysInfoCursor = 0;
                appState = APP_SYSINFO;
                break;
            case 4: /* Settings */
                settingsCursor = 0;
                appState = APP_SETTINGS;
                break;
        }
    }
}

/* handleUninstallInput: navigation only (KEY_X handled in main loop as blocking flow). */
void handleUninstallInput(void) {
    u32 keys = hidKeysDown();
    /* Navigation */
    if (keys & KEY_DOWN) {
        if (cursor < filteredCount - 1) {
            cursor++;
            if (cursor >= scrollOffset + MAX_VISIBLE_TITLES)
                scrollOffset = cursor - MAX_VISIBLE_TITLES + 1;
        }
    }
    if (keys & KEY_UP) {
        if (cursor > 0) {
            cursor--;
            if (cursor < scrollOffset) scrollOffset = cursor;
        }
    }
    if (keys & KEY_RIGHT) {
        cursor += MAX_VISIBLE_TITLES;
        if (cursor >= filteredCount) cursor = (filteredCount > 0) ? filteredCount - 1 : 0;
        if (cursor >= scrollOffset + MAX_VISIBLE_TITLES)
            scrollOffset = cursor - MAX_VISIBLE_TITLES + 1;
    }
    if (keys & KEY_LEFT) {
        cursor -= MAX_VISIBLE_TITLES;
        if (cursor < 0) cursor = 0;
        if (cursor < scrollOffset) scrollOffset = cursor;
    }
    /* Sort */
    if (keys & KEY_L) {
        currentSortMode = (SortMode)((currentSortMode + 2) % 3);
        sortTitles(); updateFilteredList();
    }
    if (keys & KEY_R) {
        currentSortMode = (SortMode)((currentSortMode + 1) % 3);
        sortTitles(); updateFilteredList();
    }
    /* Filter */
    if (keys & KEY_Y) {
        currentFilterMode = (FilterMode)((currentFilterMode + 1) % 3);
        updateFilteredList();
    }
    /* Select / deselect */
    if ((keys & KEY_A) && filteredCount > 0)
        titles[filteredIndices[cursor]].selected ^= true;
    /* Back to menu */
    if (keys & KEY_B) {
        for (int i = 0; i < titleCount; i++) titles[i].selected = false;
        appState = APP_MAIN_MENU;
    }
    /* NOTE: KEY_X is intercepted in main() before C3D_FrameBegin to avoid nested frames. */
}

/* _runUninstallDeleteFlow: blocking flow triggered by KEY_X in uninstall screen.
   Called from main loop BEFORE C3D_FrameBegin — manages own frames via
   drawDialogWithSelectedList / drawDialog / deleteTitle. */
static void _runUninstallDeleteFlow(void) {
    /* Step 1: count selected */
    int selectedCount = 0;
    for (int i = 0; i < titleCount; i++)
        if (titles[i].selected && titles[i].isValid) selectedCount++;

    if (selectedCount == 0) {
        const char *msg[] = { "", "  No title selected.", "",
                              "  Select at least one title with A.", "",
                              "  Press A to continue." };
        drawDialog(msg, 6);
        while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; }
        return;
    }

    /* Step 3: find related titles (DLC/Update) for selected base titles */
    int pendingRelated[MAX_TITLES];
    int pendingCount = 0;
    for (int i = 0; i < titleCount; i++) {
        if (!titles[i].selected || !titles[i].isValid) continue;
        if ((u32)(titles[i].titleID >> 32) != 0x00040000) continue;
        int relIdx[20]; int relCount = 0;
        findRelatedTitles(titles[i].titleID, i, relIdx, &relCount);
        for (int r = 0; r < relCount; r++) {
            int ri = relIdx[r];
            if (!titles[ri].selected && titles[ri].isValid) {
                bool dup = false;
                for (int p = 0; p < pendingCount; p++)
                    if (pendingRelated[p] == ri) { dup = true; break; }
                if (!dup && pendingCount < MAX_TITLES) pendingRelated[pendingCount++] = ri;
            }
        }
    }
    if (pendingCount > 0) {
        char relMsg[64];
        snprintf(relMsg, sizeof(relMsg), "  Found %d related titles (DLC/Update).", pendingCount);
        const char *dl[] = { "", relMsg, "", "  A=Include  B=Skip  START=Cancel" };
        bool addRelated = false, cancelAll = false;
        while (aptMainLoop()) {
            drawDialogWithSelectedList(dl, 4);
            hidScanInput(); u32 k = hidKeysDown();
            if (k & KEY_A)     { addRelated = true;  break; }
            if (k & KEY_B)     { break; }
            if (k & KEY_START) { cancelAll  = true;  break; }
        }
        if (cancelAll) {
            for (int i = 0; i < titleCount; i++) titles[i].selected = false;
            return;
        }
        if (addRelated) {
            for (int p = 0; p < pendingCount; p++) titles[pendingRelated[p]].selected = true;
            selectedCount += pendingCount;
        }
    }

    /* Step 4: backup? */
    bool doBackup = false;
    char chosenPath[256];
    snprintf(chosenPath, sizeof(chosenPath), "%s", config.backupPath);
    if (config.forceBackup) {
        doBackup = true;
    } else {
        const char *bkDl[] = { "", "  Backup save data?", "",
                               "  A=Yes   B=No   START=Cancel" };
        while (aptMainLoop()) {
            drawDialogWithSelectedList(bkDl, 4);
            hidScanInput(); u32 k = hidKeysDown();
            if (k & KEY_A)     { doBackup = true; break; }
            if (k & KEY_B)     { break; }
            if (k & KEY_START) {
                for (int i = 0; i < titleCount; i++) titles[i].selected = false;
                return;
            }
        }
    }

    /* Step 5: choose backup path */
    if (doBackup) {
        char pathLine[80];
        snprintf(pathLine, sizeof(pathLine), "  %.46s", config.backupPath);
        const char *pd[] = { "", pathLine, "", "  A=Use default   Y=Choose other" };
        bool choosePath = false;
        while (aptMainLoop()) {
            drawDialogWithSelectedList(pd, 4);
            hidScanInput(); u32 k = hidKeysDown();
            if (k & KEY_A) { break; }
            if (k & KEY_Y) { choosePath = true; break; }
        }
        if (choosePath) {
            int pidx = 0;
            for (int i = 0; i < (int)NUM_BACKUP_PATHS; i++)
                if (strcmp(config.backupPath, BACKUP_PATH_OPTIONS[i]) == 0) { pidx = i; break; }
            while (aptMainLoop()) {
                char pl[64]; snprintf(pl, sizeof(pl), "  %.52s", BACKUP_PATH_OPTIONS[pidx]);
                const char *ppd[] = { "  Select backup folder:", pl,
                                      "", "  Up/Down=Change   A=Confirm   B=Cancel" };
                drawDialogWithSelectedList(ppd, 4);
                hidScanInput(); u32 k = hidKeysDown();
                if (k & KEY_DOWN) pidx = (pidx + 1) % (int)NUM_BACKUP_PATHS;
                if (k & KEY_UP)   pidx = (pidx + (int)NUM_BACKUP_PATHS - 1) % (int)NUM_BACKUP_PATHS;
                if (k & KEY_A)  { snprintf(chosenPath, sizeof(chosenPath), "%s", BACKUP_PATH_OPTIONS[pidx]); break; }
                if (k & KEY_B)  { break; }
            }
        }
    }

    /* Step 6: confirm */
    if (!config.skipUninstallConfirm) {
        char cntMsg[64]; snprintf(cntMsg, sizeof(cntMsg), "  Delete %d titles?", selectedCount);
        char bkMsg[64];
        if (doBackup) snprintf(bkMsg, sizeof(bkMsg), "  Backup to: %.36s", chosenPath);
        else          snprintf(bkMsg, sizeof(bkMsg), "  No backup.");
        const char *cfDl[] = { "", cntMsg, bkMsg, "", "  A=Confirm   B=Cancel" };
        bool confirmed = false;
        while (aptMainLoop()) {
            drawDialogWithSelectedList(cfDl, 5);
            hidScanInput(); u32 k = hidKeysDown();
            if (k & KEY_A) { confirmed = true; break; }
            if (k & KEY_B) {
                for (int i = 0; i < titleCount; i++) titles[i].selected = false;
                return;
            }
        }
        if (!confirmed) {
            for (int i = 0; i < titleCount; i++) titles[i].selected = false;
            return;
        }
    }

    /* Step 7: execute backup + delete */
    for (int i = 0; i < titleCount; i++) {
        if (!titles[i].selected || !titles[i].isValid) continue;
        if (doBackup) backupSaveDataToPath(&titles[i], chosenPath);
        deleteTitle(&titles[i]); /* has its own dialog + wait loop */
    }

    /* Step 8: invalidate title list, return to menu */
    titleCount = 0;
    cursor = 0; scrollOffset = 0;
    for (int i = 0; i < MAX_TITLES; i++) titles[i].selected = false;
    appState = APP_MAIN_MENU;
}

/* _goUpDir: navigate to parent directory, updating globals. */
static void _goUpDir(void) {
    if (strcmp(currentPath, "sdmc:/") == 0) return;
    char parent[512];
    strncpy(parent, currentPath, sizeof(parent) - 1);
    parent[sizeof(parent) - 1] = '\0';
    char *lastSlash = strrchr(parent, '/');
    if (lastSlash != NULL) {
        *lastSlash = '\0';
        if (strlen(parent) <= 5) /* "sdmc:" or less → root */
            strncpy(parent, "sdmc:/", sizeof(parent) - 1);
    } else {
        strncpy(parent, "sdmc:/", sizeof(parent) - 1);
    }
    scanDirectory(parent);
    fileCursor = 0; fileScrollOffset = 0;
}

/* _installFolderCIAs: install all .cia files in folderPath.
   Shows progress bar via installCIA. After each successful install,
   checks for an existing backup and asks to restore (mirrors single-file behaviour).
   Returns number of successfully installed files. */
static int _installFolderCIAs(const char *folderPath) {
    /* Collect CIA paths with opendir to avoid clobbering fileEntries[] */
    char ciaPaths[64][512]; /* max 64 CIA per folder */
    int ciaCount = 0;
    DIR *d = opendir(folderPath);
    if (d) {
        struct dirent *ent;
        while ((ent = readdir(d)) != NULL && ciaCount < 64) {
            if (ent->d_type == DT_DIR) continue;
            const char *ext = strrchr(ent->d_name, '.');
            if (!ext || strcasecmp(ext, ".cia") != 0) continue;
            snprintf(ciaPaths[ciaCount], 512, "%s/%s", folderPath, ent->d_name);
            ciaCount++;
        }
        closedir(d);
    }
    if (ciaCount == 0) return 0;
    FS_MediaType dest = MEDIATYPE_SD; /* always SD */
    int installed = 0, failed = 0;
    for (int i = 0; i < ciaCount; i++) {
        bool ok = installCIA(ciaPaths[i], dest); /* installCIA manages its own C3D frames */
        if (ok) {
            installed++;
            /* Post-install backup check — same behaviour as single-file install */
            u64 tid = getCIATitleID(ciaPaths[i]);
            if (tid != 0) {
                char backupDir[512];
                if (findBackupDir(tid, backupDir, sizeof(backupDir))) {
                    TitleInfo tmp;
                    memset(&tmp, 0, sizeof(tmp));
                    tmp.titleID = tid; tmp.mediaType = dest;
                    getTitleName(tid, dest, tmp.name, sizeof(tmp.name));
                    if (config.forceRestore) {
                        restoreSaveData(&tmp);
                        const char *rDl[] = { "", "  Save data restored", "  automatically.", "", "  A=OK" };
                        drawDialog(rDl, 5);
                        while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; }
                    } else {
                        /* Extract filename for display */
                        const char *fname = strrchr(ciaPaths[i], '/');
                        fname = fname ? fname + 1 : ciaPaths[i];
                        char fn2[42]; strncpy(fn2, fname, 40); fn2[40] = '\0';
                        char fnMsg[48]; snprintf(fnMsg, sizeof(fnMsg), "  %.40s", fn2);
                        const char *askDl[] = { fnMsg, "  Found backup for this title.",
                                                "  Restore save data?", "", "  A=Yes   B=No" };
                        bool doRestore = false;
                        while (aptMainLoop()) {
                            drawDialog(askDl, 5);
                            hidScanInput(); u32 rk = hidKeysDown();
                            if (rk & KEY_A) { doRestore = true; break; }
                            if (rk & KEY_B) break;
                        }
                        if (doRestore) {
                            restoreSaveData(&tmp);
                            const char *rDl[] = { "", "  Save data restored.", "", "  A=OK" };
                            drawDialog(rDl, 4);
                            while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; }
                        }
                    }
                }
            }
        } else {
            failed++;
        }
    }
    char resMsg[64]; snprintf(resMsg, sizeof(resMsg), "  Installed: %d   Failed: %d", installed, failed);
    const char *resDl[] = { "", "  Batch install completed.", resMsg, "", "  A=Continue" };
    drawDialog(resDl, 5);
    while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; }
    if (installed > 0) titlesNeedRefresh = true;
    return installed;
}

/* runInstallFlow: blocking CIA install flow — manages own C3D frames. */
void runInstallFlow(void) {
    scanDirectory("sdmc:/");
    fileCursor = 0; fileScrollOffset = 0;

    while (aptMainLoop()) {
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        drawFileBrowserScreen();
        C3D_FrameEnd(0);
        hidScanInput();
        u32 k = hidKeysDown();

        if (k & KEY_START) return;

        /* Navigation */
        if (k & KEY_DOWN) {
            if (fileCursor < fileCount - 1) {
                fileCursor++;
                if (fileCursor >= fileScrollOffset + MAX_VISIBLE_TITLES)
                    fileScrollOffset = fileCursor - MAX_VISIBLE_TITLES + 1;
            }
        }
        if (k & KEY_UP) {
            if (fileCursor > 0) {
                fileCursor--;
                if (fileCursor < fileScrollOffset) fileScrollOffset = fileCursor;
            }
        }

        /* B: go up or exit */
        if (k & KEY_B) {
            if (strcmp(currentPath, "sdmc:/") == 0) return;
            _goUpDir();
            continue;
        }

        /* Y: install all CIA in current directory */
        if (k & KEY_Y) {
            int ciaInDir = 0;
            for (int i = 0; i < fileCount; i++)
                if (!fileEntries[i].isDir && fileEntries[i].isCIA) ciaInDir++;
            if (ciaInDir == 0) {
                const char *msg[] = { "", "  No CIA files in this folder.", "", "  A=Continue" };
                drawDialog(msg, 4);
                while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; }
                continue;
            }
            if (!config.skipInstallConfirm) {
                char cntMsg[52]; snprintf(cntMsg, sizeof(cntMsg), "  Install %d CIA files?", ciaInDir);
                const char *cfDl[] = { "", cntMsg, "  Destination: SD card", "", "  A=Yes   B=No" };
                bool confirmed = false;
                while (aptMainLoop()) {
                    drawDialog(cfDl, 5);
                    hidScanInput(); u32 ck = hidKeysDown();
                    if (ck & KEY_A) { confirmed = true; break; }
                    if (ck & KEY_B) break;
                }
                if (!confirmed) continue;
            }
            char savedPath[512]; strncpy(savedPath, currentPath, sizeof(savedPath)-1);
            _installFolderCIAs(savedPath);
            return;
        }

        /* A: enter dir or install CIA */
        if (k & KEY_A) {
            if (fileCount == 0) continue;
            FileEntry *fe = &fileEntries[fileCursor];

            if (fe->isDir) {
                if (strcmp(fe->name, "..") == 0) {
                    /* Same as B */
                    if (strcmp(currentPath, "sdmc:/") == 0) return;
                    _goUpDir();
                } else {
                    /* Enter directory directly — no confirmation dialog */
                    char newPath[512];
                    if (strcmp(currentPath, "sdmc:/") == 0)
                        snprintf(newPath, sizeof(newPath), "sdmc:/%s", fe->name);
                    else
                        snprintf(newPath, sizeof(newPath), "%s/%s", currentPath, fe->name);
                    scanDirectory(newPath);
                    fileCursor = 0; fileScrollOffset = 0;
                }
            } else if (fe->isCIA) {
                /* Install single CIA */
                char ciaPath[512];
                if (strcmp(currentPath, "sdmc:/") == 0)
                    snprintf(ciaPath, sizeof(ciaPath), "sdmc:/%s", fe->name);
                else
                    snprintf(ciaPath, sizeof(ciaPath), "%s/%s", currentPath, fe->name);

                u64 tid = getCIATitleID(ciaPath);
                FS_MediaType dest = MEDIATYPE_SD; /* always SD */

                if (!config.skipInstallConfirm) {
                    char tidStr[36];
                    if (tid) snprintf(tidStr, sizeof(tidStr), "  ID: %016llX", (unsigned long long)tid);
                    else     snprintf(tidStr, sizeof(tidStr), "  ID: N/A");
                    char fnShort[40]; strncpy(fnShort, fe->name, 38); fnShort[38]='\0';
                    char fileMsg[44]; snprintf(fileMsg, sizeof(fileMsg), "  %.38s", fnShort);
                    const char *cfDl[] = { "  Install?", fileMsg, tidStr, "  Destination: SD card", "", "  A=Yes   B=No" };
                    bool confirmed = false;
                    while (aptMainLoop()) {
                        drawDialog(cfDl, 6);
                        hidScanInput(); u32 ck = hidKeysDown();
                        if (ck & KEY_A) { confirmed = true; break; }
                        if (ck & KEY_B) break;
                    }
                    if (!confirmed) continue;
                }

                bool ok = installCIA(ciaPath, dest); /* installCIA manages its own C3D frames */

                if (ok) {
                    titlesNeedRefresh = true; /* lista titoli da ricaricare al prossimo Uninstall/SysInfo */
                    char fn2[42]; strncpy(fn2, fe->name, 40); fn2[40]='\0';
                    char okMsg[48]; snprintf(okMsg, sizeof(okMsg), "  %.40s", fn2);
                    const char *okDl[] = { "  Installation complete!", okMsg, "", "  A=Continue" };
                    drawDialog(okDl, 4);
                    while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; }

                    /* Check for existing backup to restore */
                    if (tid != 0) {
                        char backupDir[512];
                        if (findBackupDir(tid, backupDir, sizeof(backupDir))) {
                            TitleInfo tmp;
                            memset(&tmp, 0, sizeof(tmp));
                            tmp.titleID = tid; tmp.mediaType = dest;
                            getTitleName(tid, dest, tmp.name, sizeof(tmp.name));
                            if (config.forceRestore) {
                                restoreSaveData(&tmp);
                                const char *rDl[] = { "", "  Save data restored", "  automatically.", "", "  A=OK" };
                                drawDialog(rDl, 5);
                                while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; }
                            } else {
                                const char *askDl[] = { "", "  Found backup for this title.", "  Restore save data?", "", "  A=Yes   B=No" };
                                bool doRestore = false;
                                while (aptMainLoop()) {
                                    drawDialog(askDl, 5);
                                    hidScanInput(); u32 rk = hidKeysDown();
                                    if (rk & KEY_A) { doRestore = true; break; }
                                    if (rk & KEY_B) break;
                                }
                                if (doRestore) {
                                    restoreSaveData(&tmp);
                                    const char *rDl[] = { "", "  Save data restored.", "", "  A=OK" };
                                    drawDialog(rDl, 4);
                                    while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; }
                                }
                            }
                        }
                    }
                    /* Stay in browser after single install (user may install more) */
                } else {
                    const char *errDl[] = { "", "  Error during installation.", "  Check that the CIA file is valid.", "", "  A=Continue" };
                    drawDialog(errDl, 5);
                    while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; }
                }
            }
        }
    }
}

/* runBackupFlow: blocking backup flow — manages own C3D frames.
   Pre-condition: titleCount > 0. */
void runBackupFlow(void) {
    for (int i = 0; i < titleCount; i++) titles[i].selected = false;
    backupCursor = 0; backupScrollOffset = 0;

    while (aptMainLoop()) {
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        drawBackupScreen();
        C3D_FrameEnd(0);
        hidScanInput();
        u32 k = hidKeysDown();

        if ((k & KEY_B) || (k & KEY_START)) return;

        /* Navigation */
        if (k & KEY_DOWN) {
            if (backupCursor < titleCount - 1) {
                backupCursor++;
                if (backupCursor >= backupScrollOffset + MAX_VISIBLE_TITLES)
                    backupScrollOffset = backupCursor - MAX_VISIBLE_TITLES + 1;
            }
        }
        if (k & KEY_UP) {
            if (backupCursor > 0) {
                backupCursor--;
                if (backupCursor < backupScrollOffset) backupScrollOffset = backupCursor;
            }
        }
        if (k & KEY_RIGHT) {
            backupCursor += MAX_VISIBLE_TITLES;
            if (backupCursor >= titleCount) backupCursor = (titleCount > 0) ? titleCount - 1 : 0;
            if (backupCursor >= backupScrollOffset + MAX_VISIBLE_TITLES)
                backupScrollOffset = backupCursor - MAX_VISIBLE_TITLES + 1;
        }
        if (k & KEY_LEFT) {
            backupCursor -= MAX_VISIBLE_TITLES;
            if (backupCursor < 0) backupCursor = 0;
            if (backupCursor < backupScrollOffset) backupScrollOffset = backupCursor;
        }

        /* Select / deselect current title */
        if (k & KEY_A)
            titles[backupCursor].selected ^= true;

        /* KEY_X — backup selected titles */
        if (k & KEY_X) {
            int selCount = 0;
            for (int i = 0; i < titleCount; i++)
                if (titles[i].selected && titles[i].isValid) selCount++;
            if (selCount == 0) {
                const char *msg[] = { "", "  No title selected.", "  Select titles with A.", "", "  Press A to continue." };
                drawDialog(msg, 5);
                while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; }
                continue;
            }
            char cntMsg[56]; snprintf(cntMsg, sizeof(cntMsg), "  Backup %d selected titles?", selCount);
            const char *cfDl[] = { "", cntMsg, "", "  A=Yes   B=No" };
            bool confirmed = false;
            while (aptMainLoop()) {
                drawDialog(cfDl, 4);
                hidScanInput(); u32 ck = hidKeysDown();
                if (ck & KEY_A) { confirmed = true; break; }
                if (ck & KEY_B) break;
            }
            if (!confirmed) continue;
            int done = 0;
            for (int i = 0; i < titleCount; i++) {
                if (!titles[i].selected || !titles[i].isValid) continue;
                drawLoadingScreen(done, selCount, titles[i].name); /* manages own frame */
                backupSaveData(&titles[i]);
                titles[i].hasBackup = true;
                done++;
            }
            char doneMsg[56]; snprintf(doneMsg, sizeof(doneMsg), "  Backup completed for %d titles.", done);
            const char *doneDl[] = { "", doneMsg, "", "  A=Continue" };
            drawDialog(doneDl, 4);
            while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; }
            return;
        }

        /* KEY_Y — backup ALL valid titles */
        if (k & KEY_Y) {
            int validCount = 0;
            for (int i = 0; i < titleCount; i++)
                if (titles[i].isValid) validCount++;
            char cntMsg[56]; snprintf(cntMsg, sizeof(cntMsg), "  Backup ALL titles (%d)?", validCount);
            const char *cfDl[] = { "", cntMsg, "  This may take a while.", "", "  A=Yes   B=No" };
            bool confirmed = false;
            while (aptMainLoop()) {
                drawDialog(cfDl, 5);
                hidScanInput(); u32 ck = hidKeysDown();
                if (ck & KEY_A) { confirmed = true; break; }
                if (ck & KEY_B) break;
            }
            if (!confirmed) continue;
            int done = 0;
            for (int i = 0; i < titleCount; i++) {
                if (!titles[i].isValid) continue;
                drawLoadingScreen(done, validCount, titles[i].name); /* manages own frame */
                backupSaveData(&titles[i]);
                titles[i].hasBackup = true;
                done++;
            }
            char doneMsg[56]; snprintf(doneMsg, sizeof(doneMsg), "  Backup completed for %d titles.", done);
            const char *doneDl[] = { "", doneMsg, "", "  A=Continue" };
            drawDialog(doneDl, 4);
            while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; }
            return;
        }
    }
}

/* handleSysInfoInput: navigation only — KEY_A in sublist handled in main()
   as blocking flow to avoid nested C3D frames. */
void handleSysInfoInput(void) {
    u32 keys = hidKeysDown();
    if (sysInfoMode == SYSINFO_OVERVIEW) {
        if (keys & KEY_DOWN) sysInfoCursor = (sysInfoCursor + 1) % 3;
        if (keys & KEY_UP)   sysInfoCursor = (sysInfoCursor + 2) % 3;
        if (keys & KEY_B)    appState = APP_MAIN_MENU;
        if (keys & KEY_A) {
            u32 tHi = (sysInfoCursor == 0) ? 0x00040000u :
                      (sysInfoCursor == 1) ? 0x0004000Eu : 0x0004008Cu;
            sysInfoSubCount = 0;
            for (int i = 0; i < titleCount; i++) {
                if (titles[i].isValid && (u32)(titles[i].titleID >> 32) == tHi) {
                    sysInfoSubIndices[sysInfoSubCount++] = i;
                    if (sysInfoSubCount >= MAX_TITLES) break;
                }
            }
            sysInfoSubCursor = 0; sysInfoSubScrollOffset = 0;
            sysInfoMode = (sysInfoCursor == 0) ? SYSINFO_GAMES :
                          (sysInfoCursor == 1) ? SYSINFO_UPDATES : SYSINFO_DLC;
        }
    } else {
        /* Sublist — KEY_A intercepted in main() */
        if (keys & KEY_DOWN) {
            if (sysInfoSubCursor < sysInfoSubCount - 1) {
                sysInfoSubCursor++;
                if (sysInfoSubCursor >= sysInfoSubScrollOffset + MAX_VISIBLE_TITLES)
                    sysInfoSubScrollOffset = sysInfoSubCursor - MAX_VISIBLE_TITLES + 1;
            }
        }
        if (keys & KEY_UP) {
            if (sysInfoSubCursor > 0) {
                sysInfoSubCursor--;
                if (sysInfoSubCursor < sysInfoSubScrollOffset)
                    sysInfoSubScrollOffset = sysInfoSubCursor;
            }
        }
        if (keys & KEY_B) {
            /* Restore overview cursor to the category we came from */
            sysInfoCursor = (sysInfoMode == SYSINFO_GAMES)   ? 0 :
                            (sysInfoMode == SYSINFO_UPDATES) ? 1 : 2;
            sysInfoMode = SYSINFO_OVERVIEW;
        }
    }
}

/* _runSysInfoDetailFlow: blocking detail/action view for a single title.
   Called from main() BEFORE C3D_FrameBegin — manages own C3D frames.
   Uses sysInfoDetailIdx and sysInfoDetailCursor. */
static void _runSysInfoDetailFlow(void) {
    int idx = sysInfoDetailIdx;
    sysInfoDetailCursor = 0;

    while (aptMainLoop()) {
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        drawTitleDetails();
        C3D_FrameEnd(0);
        hidScanInput();
        u32 k = hidKeysDown();

        if (k & KEY_DOWN) sysInfoDetailCursor = (sysInfoDetailCursor + 1) % 3;
        if (k & KEY_UP)   sysInfoDetailCursor = (sysInfoDetailCursor + 2) % 3;
        if (k & KEY_B)    break;

        if (k & KEY_A) {
            if (sysInfoDetailCursor == 0) {
                /* Backup save */
                backupSaveData(&titles[idx]);
                titles[idx].hasBackup = true;
                const char *msg[] = { "", "  Backup completed.", "", "  A=OK" };
                drawDialog(msg, 4);
                while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; }

            } else if (sysInfoDetailCursor == 1) {
                /* Restore save */
                if (!titles[idx].hasBackup) {
                    const char *msg[] = { "", "  No backup available.", "", "  A=OK" };
                    drawDialog(msg, 4);
                    while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; }
                    continue;
                }
                bool ok = restoreSaveData(&titles[idx]);
                if (ok) {
                    const char *msg[] = { "", "  Save data restored.", "", "  A=OK" };
                    drawDialog(msg, 4);
                } else {
                    const char *msg[] = { "", "  Restore error.", "", "  A=OK" };
                    drawDialog(msg, 4);
                }
                while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; }

            } else {
                /* Delete — mark selected, reuse uninstall flow */
                titles[idx].selected = true;
                _runUninstallDeleteFlow(); /* handles related, backup, confirm, deleteTitle */
                if (titleCount == 0) {
                    /* Deletion done: _runUninstallDeleteFlow set appState=APP_MAIN_MENU */
                    sysInfoMode = SYSINFO_OVERVIEW;
                    break;
                }
                /* Cancelled: clean up selection */
                titles[idx].selected = false;
            }
        }
    }
}

/* handleSettingsInput: full settings handler with real-time config save. */
void handleSettingsInput(void) {
    u32 keys = hidKeysDown();

    if (keys & KEY_DOWN) settingsCursor = (settingsCursor + 1) % 5;
    if (keys & KEY_UP)   settingsCursor = (settingsCursor + 4) % 5;

    bool *boolPtrs[4] = {
        &config.forceBackup,
        &config.skipUninstallConfirm,
        &config.forceRestore,
        &config.skipInstallConfirm
    };

    /* Find current backup path index (for item 4 LEFT/RIGHT) */
    int pidx = 0;
    for (int i = 0; i < (int)NUM_BACKUP_PATHS; i++)
        if (strcmp(config.backupPath, BACKUP_PATH_OPTIONS[i]) == 0) { pidx = i; break; }

    bool changed = false;

    if ((keys & KEY_A) || (keys & KEY_RIGHT) || (keys & KEY_R)) {
        if      (settingsCursor <= 3) { *boolPtrs[settingsCursor] ^= true; changed = true; }
        else if (settingsCursor == 4) {
            pidx = (pidx + 1) % (int)NUM_BACKUP_PATHS;
            snprintf(config.backupPath, sizeof(config.backupPath), "%s", BACKUP_PATH_OPTIONS[pidx]);
            changed = true;
        }
    }
    if ((keys & KEY_LEFT) || (keys & KEY_L)) {
        if      (settingsCursor <= 3) { *boolPtrs[settingsCursor] ^= true; changed = true; }
        else if (settingsCursor == 4) {
            pidx = (pidx + (int)NUM_BACKUP_PATHS - 1) % (int)NUM_BACKUP_PATHS;
            snprintf(config.backupPath, sizeof(config.backupPath), "%s", BACKUP_PATH_OPTIONS[pidx]);
            changed = true;
        }
    }

    if (changed) saveConfig();
    if ((keys & KEY_B) || (keys & KEY_START)) { saveConfig(); appState = APP_MAIN_MENU; }
}

/* ============================================================
   SECTION 11: main()
   ============================================================ */
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
            /* No 'continue' — fall through so the first frame is rendered immediately */
        }

        /* --- Blocking flows: entered BEFORE opening a C3D frame --- */

        if (appState == APP_INSTALL) {
            runInstallFlow();
            appState = APP_MAIN_MENU;
            continue;
        }
        if (appState == APP_BACKUP) {
            runBackupFlow();
            appState = APP_MAIN_MENU;
            continue;
        }
        /* KEY_X uninstall flow: blocking, must run outside C3D frame */
        if (appState == APP_UNINSTALL && (keys & KEY_X)) {
            _runUninstallDeleteFlow();
            continue;
        }
        /* SysInfo title detail: blocking (Backup/Restore/Delete), outside C3D frame */
        if (appState == APP_SYSINFO && sysInfoMode != SYSINFO_OVERVIEW
                && sysInfoSubCount > 0 && (keys & KEY_A)) {
            sysInfoDetailIdx    = sysInfoSubIndices[sysInfoSubCursor];
            sysInfoDetailCursor = 0;
            _runSysInfoDetailFlow();
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
                    /* SELECT overlay replaces the list draw for this frame */
                    if (hidKeysHeld() & KEY_SELECT)
                        drawControlsOverlay();
                    else
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

