/* ============================================================
   install.c — CIA file scanning and installation
   ============================================================ */
#include "install.h"
#include "globals.h"
#include "utils.h"
#include "draw.h"

u64 getCIATitleID(const char *ciaPath) {
    FILE *f = fopen(ciaPath, "rb"); if (!f) return 0;
    u32 hdrSz = 0; fread(&hdrSz, 4, 1, f);
    fseek(f, 8, SEEK_SET);
    u32 certSz = 0, tikSz = 0, tmdSz = 0;
    fread(&certSz, 4, 1, f); fread(&tikSz, 4, 1, f); fread(&tmdSz, 4, 1, f);
    if (tmdSz < 0x200) { fclose(f); return 0; }
    u32 tmdOff = ALIGN64(hdrSz) + ALIGN64(certSz) + ALIGN64(tikSz);
    fseek(f, tmdOff, SEEK_SET);
    u32 sigType = 0; fread(&sigType, 4, 1, f);
    u32 sigSz = 0x100, padSz = 0x3C;
    if      (sigType == 0x00010003) { sigSz = 0x200; }
    else if (sigType == 0x00010005) { sigSz = 0x3C; padSz = 0x40; }
    u32 bodyOff = tmdOff + 4 + sigSz + padSz;
    fseek(f, bodyOff + 0x4C, SEEK_SET);
    u8 tb[8] = {0}; fread(tb, 8, 1, f); fclose(f);
    u64 tid = 0; for (int i = 0; i < 8; i++) tid = (tid << 8) | tb[i];
    return tid;
}

int scanDirectory(const char *path) {
    fileCount = 0;
    snprintf(currentPath, sizeof(currentPath), "%s", path);

#define MKFP(fp, base, name) \
    do { int _l = (int)strlen(base); \
         if (_l > 0 && (base)[_l-1] == '/') snprintf(fp, sizeof(fp), "%s%s", base, name); \
         else snprintf(fp, sizeof(fp), "%s/%s", base, name); } while(0)

    /* Add ".." entry if not root */
    if (strcmp(path, "sdmc:/") != 0) {
        strcpy(fileEntries[0].name, ".."); fileEntries[0].isDir = true;
        fileEntries[0].isCIA = false; fileEntries[0].size = 0; fileCount = 1;
    }

    /* Pass 1: directories */
    DIR *d = opendir(path);
    if (d) {
        struct dirent *ent;
        while ((ent = readdir(d)) != NULL && fileCount < MAX_FILES) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
            bool isDir = (ent->d_type == DT_DIR);
            if (!isDir) {
                char fp2[512]; MKFP(fp2, path, ent->d_name);
                struct stat st; if (stat(fp2, &st) == 0 && S_ISDIR(st.st_mode)) isDir = true;
            }
            if (!isDir) continue;
            strncpy(fileEntries[fileCount].name, ent->d_name, 255); fileEntries[fileCount].name[255] = '\0';
            fileEntries[fileCount].isDir = true; fileEntries[fileCount].isCIA = false;
            fileEntries[fileCount].size = 0; fileCount++;
        }
        closedir(d);
    }

    /* Pass 2: .cia files (new opendir — avoids rewinddir unreliability on CTRU) */
    d = opendir(path);
    if (d) {
        struct dirent *ent;
        while ((ent = readdir(d)) != NULL && fileCount < MAX_FILES) {
            char fp2[512]; MKFP(fp2, path, ent->d_name);
            bool isDir = (ent->d_type == DT_DIR);
            if (!isDir) { struct stat st; if (stat(fp2, &st) == 0 && S_ISDIR(st.st_mode)) isDir = true; }
            if (isDir) continue;
            const char *ext = strrchr(ent->d_name, '.');
            if (!ext || strcasecmp(ext, ".cia") != 0) continue;
            strncpy(fileEntries[fileCount].name, ent->d_name, 255); fileEntries[fileCount].name[255] = '\0';
            fileEntries[fileCount].isDir = false; fileEntries[fileCount].isCIA = true;
            struct stat st;
            fileEntries[fileCount].size = (stat(fp2, &st) == 0) ? (u64)st.st_size : 0;
            fileEntries[fileCount].titleID = getCIATitleID(fp2);
            fileCount++;
        }
        closedir(d);
    }
#undef MKFP
    return fileCount;
}

bool installCIA(const char *ciaPath, FS_MediaType dest) {
    FILE *f = fopen(ciaPath, "rb"); if (!f) return false;
    fseek(f, 0, SEEK_END); long fsize = ftell(f); fseek(f, 0, SEEK_SET);
    if (fsize <= 0) { fclose(f); return false; }
    Handle cia;
    if (R_FAILED(AM_StartCiaInstall(dest, &cia))) { fclose(f); return false; }
    u8 *buf = (u8*)malloc(CHUNK_SIZE);
    if (!buf) { AM_CancelCIAInstall(cia); fclose(f); return false; }
    long done = 0; bool ok = true;
    while (done < fsize) {
        int toRead = (int)((fsize - done > CHUNK_SIZE) ? CHUNK_SIZE : fsize - done);
        int br = (int)fread(buf, 1, toRead, f);
        if (br <= 0) { ok = false; break; }
        u32 bw = 0;
        if (R_FAILED(FSFILE_Write(cia, &bw, done, buf, br, FS_WRITE_FLUSH))) { ok = false; break; }
        done += br;
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        drawInstallProgressScreen(ciaPath, done, fsize);
        C3D_FrameEnd(0);
    }
    free(buf); fclose(f);
    if (!ok) { AM_CancelCIAInstall(cia); return false; }
    return R_SUCCEEDED(AM_FinishCiaInstall(cia));
}

