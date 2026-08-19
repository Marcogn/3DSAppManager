/*
 * shims/3ds.h — minimal libctru type stubs for host-side unit tests.
 */
#ifndef SHIMS_3DS_H
#define SHIMS_3DS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>  /* ssize_t */
#include <string.h>
#include <strings.h>    /* strcasecmp */
#include <stdlib.h>
#include <dirent.h>     /* DT_DIR (some libc builds need this before it's visible) */

#ifndef DT_DIR
#define DT_DIR 4        /* matches glibc's real value; only used as a comparison constant here */
#endif

/* ---- Basic types ---- */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef u32      Handle;
typedef s32      Result;

/* ---- FS types ---- */
typedef enum { MEDIATYPE_SD = 0, MEDIATYPE_NAND = 1, MEDIATYPE_GAME_CARD = 2 } FS_MediaType;
typedef enum { PATH_ASCII = 1, PATH_BINARY = 2, PATH_UTF16 = 3, PATH_EMPTY = 4 } FS_PathType;

typedef struct { FS_PathType type; u32 size; const void *data; } FS_Path;
typedef struct { u32 id; } FS_Archive;
typedef struct {
    FS_MediaType mediaType;
    u8           unknown;
    u16          reserved1;
    u64          saveId;
    u32          reserved2;
} FS_ExtSaveDataInfo;
typedef struct {
    u16 name[256];
    u32 attributes;
    u64 fileSize;
} FS_DirectoryEntry;

#define FS_ATTRIBUTE_DIRECTORY  (1u << 0)
#define FS_OPEN_READ            (1u << 0)
#define FS_OPEN_WRITE           (1u << 1)
#define FS_OPEN_CREATE          (1u << 2)
#define FS_WRITE_FLUSH          (1u << 0)

/* ---- Archive IDs ---- */
#define ARCHIVE_SAVEDATA_AND_CONTENT  0x00000007
#define ARCHIVE_USER_SAVEDATA         0x00000008
#define ARCHIVE_EXTDATA               0x00000006
#define ARCHIVE_BOSS_EXTDATA          0x00000038

/* ---- AM types ---- */
typedef struct { u64 size; u64 titleID; u16 version; u16 flags; u32 tid_high; } AM_TitleEntry;

/* ---- CFG ---- */
#define CFG_LANGUAGE_EN 1
typedef u8 CFG_Language;

/* ---- HID ---- */
#define KEY_A       (1u << 0)
#define KEY_B       (1u << 1)
#define KEY_SELECT  (1u << 2)
#define KEY_START   (1u << 3)
#define KEY_RIGHT   (1u << 4)
#define KEY_LEFT    (1u << 5)
#define KEY_UP      (1u << 6)
#define KEY_DOWN    (1u << 7)
#define KEY_R       (1u << 8)
#define KEY_L       (1u << 9)
#define KEY_X       (1u << 10)
#define KEY_Y       (1u << 11)

/* ---- R_SUCCEEDED / R_FAILED ---- */
#define R_SUCCEEDED(res) ((res) == 0)
#define R_FAILED(res)    ((res) != 0)

/* ---- utf16_to_utf8 stub ---- */
static inline ssize_t utf16_to_utf8(uint8_t *dst, const uint16_t *src, size_t size) {
    if (!src || !dst || size == 0) return -1;
    size_t i = 0;
    while (i < size - 1) {  /* leave room for null terminator */
        uint16_t c = src[i];
        if (c == 0) break;
        if (c < 0x80) {
            dst[i] = (uint8_t)c;
        } else {
            dst[i] = '?';
        }
        i++;
    }
    dst[i] = '\0';
    return (ssize_t)i;
}

/* ---- fsMakePath stub ---- */
static inline FS_Path fsMakePath(FS_PathType type, const char *path) {
    FS_Path p; p.type = type; p.size = (u32)(strlen(path) + 1); p.data = path;
    return p;
}

/* ---- GFX stubs ---- */
typedef enum { GFX_TOP = 0, GFX_BOTTOM = 1 } gfxScreen_t;
typedef enum { GFX_LEFT = 0, GFX_RIGHT = 1 } gfx3dSide_t;
static inline void gfxInitDefault(void) {}
static inline void gfxExit(void) {}

/* ---- APT stub ---- */
static inline bool aptMainLoop(void) { return false; }

/* ---- HID stubs ---- */
static inline void hidScanInput(void) {}
static inline u32  hidKeysDown(void)  { return 0; }
static inline u32  hidKeysHeld(void)  { return 0; }

/* ---- CFGU stub ---- */
static inline Result cfguInit(void)  { return 0; }
static inline void   cfguExit(void)  {}
static inline Result CFGU_GetSystemLanguage(u8 *lang) { if (lang) *lang = CFG_LANGUAGE_EN; return 0; }

/* ---- AM stubs ---- */
static inline Result amInit(void)  { return 0; }
static inline void   amExit(void)  {}
static inline Result AM_GetTitleCount(FS_MediaType mt, u32 *cnt) { (void)mt; *cnt = 0; return 0; }
static inline Result AM_GetTitleList(u32 *rc, FS_MediaType mt, u32 cnt, u64 *ids) { (void)mt;(void)cnt;(void)ids; *rc=0; return 0; }
static inline Result AM_GetTitleInfo(FS_MediaType mt, u32 cnt, u64 *ids, AM_TitleEntry *out) { (void)mt;(void)cnt;(void)ids;(void)out; return -1; }
static inline Result AM_DeleteTitle(FS_MediaType mt, u64 tid) { (void)mt;(void)tid; return 0; }
static inline Result AM_StartCiaInstall(FS_MediaType mt, Handle *h) { (void)mt;(void)h; return -1; }
static inline Result AM_CancelCIAInstall(Handle h) { (void)h; return 0; }
static inline Result AM_FinishCiaInstall(Handle h) { (void)h; return -1; }

/* fake_fs.h needs the types/constants above and provides the mutable
   registry the FS/AM stubs below consult — opt-in, defaults to "nothing
   registered" so unconfigured behavior below is unchanged (always fails). */
#include "fake_fs.h"

static inline Result AM_GetTitleExtDataId(u64 *id, FS_MediaType mt, u64 tid) {
    (void)mt;
    for (int i = 0; i < FAKE_FS_MAX_EXTIDS; i++)
        if (g_fakeExtIds[i].used && g_fakeExtIds[i].titleID == tid) { *id = g_fakeExtIds[i].extId; return 0; }
    *id = 0; return -1;
}

/* ---- FS stubs ---- */
static inline Result fsInit(void)  { return 0; }
static inline void   fsExit(void)  {}

static inline Result FSUSER_OpenArchive(FS_Archive *a, u32 id, FS_Path p) {
    (void)p;
    FakeFsArchive *fa = fakeFsFindArchive(id);
    if (!fa || fa->openFails) return -1;
    a->id = id;
    return 0;
}
static inline Result FSUSER_CloseArchive(FS_Archive a) { (void)a; return 0; }
static inline Result FSUSER_OpenFileDirectly(Handle *h, u32 arch, FS_Path ap, FS_Path fp, u32 flags, u32 attr) { (void)h;(void)arch;(void)ap;(void)fp;(void)flags;(void)attr; return -1; }

static inline Result FSUSER_OpenFile(Handle *h, FS_Archive a, FS_Path p, u32 flags, u32 attr) {
    (void)attr;
    FakeFsArchive *fa = fakeFsFindArchive(a.id);
    if (!fa) return -1;
    char norm[256]; fakeFsNormalize(norm, sizeof(norm), (const char*)p.data);
    FakeFsNode *n = fakeFsFindNode(fa, norm);
    if (n && (n->openFails || n->isDir)) return -1;
    if (!n) {
        if (!(flags & FS_OPEN_CREATE)) return -1;
        char parent[256], name[64];
        fakeFsSplitPath(norm, parent, sizeof(parent), name, sizeof(name));
        n = fakeFsAddFile(fa, parent, name, NULL, 0);
        if (!n) return -1;
    }
    for (int i = 0; i < FAKE_FS_MAX_HANDLES; i++) {
        if (!g_fakeHandles[i].used) {
            g_fakeHandles[i].used = true;
            g_fakeHandles[i].kind = FAKE_FH_FILE;
            g_fakeHandles[i].archive = fa;
            g_fakeHandles[i].nodeIndex = (int)(n - fa->nodes);
            *h = (Handle)(i + 1);
            return 0;
        }
    }
    return -1;
}

static inline Result FSUSER_OpenDirectory(Handle *h, FS_Archive a, FS_Path p) {
    FakeFsArchive *fa = fakeFsFindArchive(a.id);
    if (!fa) return -1;
    char norm[256]; fakeFsNormalize(norm, sizeof(norm), (const char*)p.data);
    FakeFsNode *n = fakeFsFindNode(fa, norm);
    if (!n || !n->isDir || n->openFails) return -1;
    for (int i = 0; i < FAKE_FS_MAX_HANDLES; i++) {
        if (!g_fakeHandles[i].used) {
            g_fakeHandles[i].used = true;
            g_fakeHandles[i].kind = FAKE_FH_DIR;
            g_fakeHandles[i].archive = fa;
            g_fakeHandles[i].dirListPos = 0;
            snprintf(g_fakeHandles[i].dirPath, sizeof(g_fakeHandles[i].dirPath), "%s", norm);
            *h = (Handle)(i + 1);
            return 0;
        }
    }
    return -1;
}

static inline Result FSUSER_CreateDirectory(FS_Archive a, FS_Path p, u32 attr) {
    (void)attr;
    FakeFsArchive *fa = fakeFsFindArchive(a.id);
    if (!fa) return 0; /* real code ignores this Result — stay lenient */
    char norm[256]; fakeFsNormalize(norm, sizeof(norm), (const char*)p.data);
    if (!fakeFsFindNode(fa, norm)) {
        char parent[256], name[64];
        fakeFsSplitPath(norm, parent, sizeof(parent), name, sizeof(name));
        fakeFsAddDir(fa, parent, name);
    }
    return 0;
}

static inline Result FSUSER_DeleteExtSaveData(FS_ExtSaveDataInfo i) { (void)i; return 0; }

static inline Result FSFILE_Read(Handle h, u32 *br, u64 off, void *buf, u32 size) {
    *br = 0;
    int idx = (int)h - 1;
    if (idx < 0 || idx >= FAKE_FS_MAX_HANDLES || !g_fakeHandles[idx].used || g_fakeHandles[idx].kind != FAKE_FH_FILE) return -1;
    FakeFsNode *n = &g_fakeHandles[idx].archive->nodes[g_fakeHandles[idx].nodeIndex];
    if (n->readFails) return -1;
    if (off >= n->size) return 0;
    u64 avail = n->size - off;
    u32 toCopy = (avail < size) ? (u32)avail : size;
    memcpy(buf, n->data + off, toCopy);
    *br = toCopy;
    return 0;
}
static inline Result FSFILE_Write(Handle h, u32 *bw, u64 off, const void *buf, u32 size, u32 flags) {
    (void)flags;
    *bw = 0;
    int idx = (int)h - 1;
    if (idx < 0 || idx >= FAKE_FS_MAX_HANDLES || !g_fakeHandles[idx].used || g_fakeHandles[idx].kind != FAKE_FH_FILE) return -1;
    FakeFsNode *n = &g_fakeHandles[idx].archive->nodes[g_fakeHandles[idx].nodeIndex];
    u64 end = off + size;
    if (end > FAKE_FS_MAX_DATA) end = FAKE_FS_MAX_DATA;
    if (end > off) memcpy(n->data + off, buf, (size_t)(end - off));
    if (end > n->size) n->size = end;
    *bw = (u32)(end - off);
    return 0;
}
static inline Result FSFILE_GetSize(Handle h, u64 *sz) {
    *sz = 0;
    int idx = (int)h - 1;
    if (idx < 0 || idx >= FAKE_FS_MAX_HANDLES || !g_fakeHandles[idx].used || g_fakeHandles[idx].kind != FAKE_FH_FILE) return -1;
    *sz = g_fakeHandles[idx].archive->nodes[g_fakeHandles[idx].nodeIndex].size;
    return 0;
}
static inline Result FSFILE_Close(Handle h) {
    int idx = (int)h - 1;
    if (idx >= 0 && idx < FAKE_FS_MAX_HANDLES) g_fakeHandles[idx].used = false;
    return 0;
}

static inline Result FSDIR_Read(Handle h, u32 *er, u32 cnt, FS_DirectoryEntry *e) {
    *er = 0;
    int idx = (int)h - 1;
    if (idx < 0 || idx >= FAKE_FS_MAX_HANDLES || !g_fakeHandles[idx].used || g_fakeHandles[idx].kind != FAKE_FH_DIR) return -1;
    FakeFsHandleEntry *hd = &g_fakeHandles[idx];
    FakeFsArchive *fa = hd->archive;
    u32 filled = 0;
    while (filled < cnt) {
        FakeFsNode *match = NULL;
        while (hd->dirListPos < fa->nodeCount) {
            FakeFsNode *cand = &fa->nodes[hd->dirListPos++];
            if (cand->used && strcmp(cand->parentPath, hd->dirPath) == 0) { match = cand; break; }
        }
        if (!match) break;
        FS_DirectoryEntry *out = &e[filled];
        memset(out, 0, sizeof(*out));
        size_t nlen = strlen(match->name);
        for (size_t k = 0; k < nlen && k < 255; k++) out->name[k] = (u16)(unsigned char)match->name[k];
        out->attributes = match->isDir ? FS_ATTRIBUTE_DIRECTORY : 0;
        out->fileSize = match->size;
        filled++;
    }
    *er = filled;
    return 0;
}
static inline Result FSDIR_Close(Handle h) {
    int idx = (int)h - 1;
    if (idx >= 0 && idx < FAKE_FS_MAX_HANDLES) g_fakeHandles[idx].used = false;
    return 0;
}

/* ---- C3D stubs ---- */
#define C3D_DEFAULT_CMDBUF_SIZE 0x40000
#define C3D_FRAME_SYNCDRAW      0
typedef struct { int dummy; } C3D_RenderTarget;
static inline bool   C3D_Init(size_t s)       { (void)s; return true; }
static inline void   C3D_Fini(void)           {}
static inline bool   C3D_FrameBegin(u8 f)     { (void)f; return true; }
static inline void   C3D_FrameEnd(u8 f)       { (void)f; }

#endif /* SHIMS_3DS_H */
