/*
 * shims/fake_fs.h — opt-in in-memory fake FS_Archive backend for host tests.
 *
 * By default NOTHING is registered, so every FSUSER_/FSFILE_/FSDIR_ call
 * in 3ds.h behaves exactly as before this file existed (always fails) —
 * existing tests that never touch this API are completely unaffected.
 *
 * A test that wants to exercise real archive-copying logic (backup_restore.c)
 * calls fakeFsReset() then fakeFsRegisterArchive()/fakeFsAddFile()/etc. to
 * build a small virtual directory tree before invoking the function under
 * test. The FS_* stub bodies in 3ds.h consult this state instead of always
 * returning failure.
 */
#ifndef SHIMS_FAKE_FS_H
#define SHIMS_FAKE_FS_H

#include <stdio.h>

#define FAKE_FS_MAX_ARCHIVES 4
#define FAKE_FS_MAX_NODES    32
#define FAKE_FS_MAX_HANDLES  16
#define FAKE_FS_MAX_DATA     4096  /* per-node content buffer; plenty for test fixtures */
#define FAKE_FS_MAX_EXTIDS   8

typedef struct {
    bool used;
    char path[256];        /* this node's own normalized full path */
    char parentPath[256];  /* normalized path of the directory this node lives directly under */
    char name[64];          /* basename, used to fill FS_DirectoryEntry.name */
    bool isDir;
    bool openFails;         /* force FSUSER_OpenDirectory/FSUSER_OpenFile to fail on this node */
    bool readFails;          /* force FSFILE_Read to fail on this node (files only) */
    u64  size;
    u8   data[FAKE_FS_MAX_DATA];
} FakeFsNode;

typedef struct {
    bool used;
    u32  archiveId;
    bool openFails;         /* force FSUSER_OpenArchive to fail for this archive id */
    FakeFsNode nodes[FAKE_FS_MAX_NODES];
    int  nodeCount;
} FakeFsArchive;

typedef enum { FAKE_FH_NONE, FAKE_FH_DIR, FAKE_FH_FILE } FakeFsHandleKind;

typedef struct {
    bool used;
    FakeFsHandleKind kind;
    FakeFsArchive *archive;
    int  nodeIndex;   /* FAKE_FH_FILE: index into archive->nodes */
    int  dirListPos;  /* FAKE_FH_DIR: iteration cursor */
    char dirPath[256]; /* FAKE_FH_DIR: normalized directory path being iterated */
} FakeFsHandleEntry;

typedef struct { bool used; u64 titleID; u64 extId; } FakeAmExtEntry;

static FakeFsArchive      g_fakeArchives[FAKE_FS_MAX_ARCHIVES];
static FakeFsHandleEntry  g_fakeHandles[FAKE_FS_MAX_HANDLES];
static FakeAmExtEntry     g_fakeExtIds[FAKE_FS_MAX_EXTIDS];

static inline void fakeFsNormalize(char *out, size_t outSize, const char *in) {
    size_t o = 0;
    bool lastWasSlash = false;
    for (size_t i = 0; in[i] != '\0' && o + 1 < outSize; i++) {
        if (in[i] == '/') {
            if (lastWasSlash) continue;
            lastWasSlash = true;
        } else {
            lastWasSlash = false;
        }
        out[o++] = in[i];
    }
    /* strip a trailing slash unless the whole path IS "/" */
    if (o > 1 && out[o - 1] == '/') o--;
    out[o] = '\0';
}

static inline void fakeFsReset(void) {
    memset(g_fakeArchives, 0, sizeof(g_fakeArchives));
    memset(g_fakeHandles, 0, sizeof(g_fakeHandles));
    memset(g_fakeExtIds, 0, sizeof(g_fakeExtIds));
}

static inline FakeFsArchive *fakeFsFindArchive(u32 archiveId) {
    for (int i = 0; i < FAKE_FS_MAX_ARCHIVES; i++)
        if (g_fakeArchives[i].used && g_fakeArchives[i].archiveId == archiveId) return &g_fakeArchives[i];
    return NULL;
}

/* Registers a fake archive for `archiveId` with an implicit root "/" dir and
   returns it so the test can add children. If openFails is true, the
   archive itself never opens (FSUSER_OpenArchive fails) — used to simulate
   e.g. a title with no extdata at all. */
static inline FakeFsArchive *fakeFsRegisterArchive(u32 archiveId, bool openFails) {
    FakeFsArchive *a = NULL;
    for (int i = 0; i < FAKE_FS_MAX_ARCHIVES; i++) {
        if (!g_fakeArchives[i].used) { a = &g_fakeArchives[i]; break; }
    }
    if (!a) return NULL;
    memset(a, 0, sizeof(*a));
    a->used = true;
    a->archiveId = archiveId;
    a->openFails = openFails;
    FakeFsNode *root = &a->nodes[a->nodeCount++];
    memset(root, 0, sizeof(*root));
    root->used = true;
    root->isDir = true;
    snprintf(root->path, sizeof(root->path), "/");
    root->parentPath[0] = '\0';
    return a;
}

static inline FakeFsNode *fakeFsFindNode(FakeFsArchive *a, const char *normalizedPath) {
    for (int i = 0; i < a->nodeCount; i++)
        if (a->nodes[i].used && strcmp(a->nodes[i].path, normalizedPath) == 0) return &a->nodes[i];
    return NULL;
}

static inline FakeFsNode *fakeFsAddNode(FakeFsArchive *a, const char *parentPath, const char *name, bool isDir) {
    if (a->nodeCount >= FAKE_FS_MAX_NODES) return NULL;
    FakeFsNode *n = &a->nodes[a->nodeCount++];
    memset(n, 0, sizeof(*n));
    n->used = true;
    n->isDir = isDir;
    char normParent[256];
    fakeFsNormalize(normParent, sizeof(normParent), parentPath);
    snprintf(n->parentPath, sizeof(n->parentPath), "%s", normParent);
    snprintf(n->name, sizeof(n->name), "%s", name);
    if (strcmp(normParent, "/") == 0) snprintf(n->path, sizeof(n->path), "/%s", name);
    else                              snprintf(n->path, sizeof(n->path), "%s/%s", normParent, name);
    return n;
}

static inline FakeFsNode *fakeFsAddDir(FakeFsArchive *a, const char *parentPath, const char *name) {
    return fakeFsAddNode(a, parentPath, name, true);
}

static inline FakeFsNode *fakeFsAddFile(FakeFsArchive *a, const char *parentPath, const char *name,
                                         const void *data, u64 size) {
    FakeFsNode *n = fakeFsAddNode(a, parentPath, name, false);
    if (!n) return NULL;
    if (size > FAKE_FS_MAX_DATA) size = FAKE_FS_MAX_DATA;
    if (data && size) memcpy(n->data, data, size);
    n->size = size;
    return n;
}

/* Splits a normalized path ("/a/b/c") into its parent ("/a/b") and basename
   ("c"). The root itself ("/") splits into parent "" and name "". */
static inline void fakeFsSplitPath(const char *normalizedPath, char *parentOut, size_t parentSize,
                                    char *nameOut, size_t nameSize) {
    if (strcmp(normalizedPath, "/") == 0) {
        if (parentSize) parentOut[0] = '\0';
        if (nameSize) nameOut[0] = '\0';
        return;
    }
    const char *lastSlash = strrchr(normalizedPath, '/');
    size_t plen = (size_t)(lastSlash - normalizedPath);
    if (plen == 0) plen = 1; /* parent is root "/" */
    if (plen >= parentSize) plen = parentSize - 1;
    memcpy(parentOut, normalizedPath, plen);
    parentOut[plen] = '\0';
    snprintf(nameOut, nameSize, "%s", lastSlash + 1);
}

/* Registers a file node that only reports a large size (e.g. to exercise
   the MAX_FILE_SIZE skip-without-failing path) without needing a real
   FAKE_FS_MAX_DATA-sized backing buffer. Production code never actually
   reads such a file's content (it skips by size before ever calling
   FSFILE_Read), so no backing data is needed. */
static inline FakeFsNode *fakeFsAddOversizedFile(FakeFsArchive *a, const char *parentPath,
                                                  const char *name, u64 reportedSize) {
    FakeFsNode *n = fakeFsAddNode(a, parentPath, name, false);
    if (n) n->size = reportedSize;
    return n;
}

static inline void fakeAmSetExtDataId(u64 titleID, u64 extId) {
    for (int i = 0; i < FAKE_FS_MAX_EXTIDS; i++) {
        if (!g_fakeExtIds[i].used) {
            g_fakeExtIds[i].used = true;
            g_fakeExtIds[i].titleID = titleID;
            g_fakeExtIds[i].extId = extId;
            return;
        }
    }
}

#endif /* SHIMS_FAKE_FS_H */
