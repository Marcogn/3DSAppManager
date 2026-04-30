/*
 * shims/citro2d.h — minimal C2D stubs for host-side unit tests.
 */
#ifndef SHIMS_CITRO2D_H
#define SHIMS_CITRO2D_H

#include "3ds.h"

typedef void *C2D_TextBuf;
typedef struct { int dummy; } C2D_Text;
typedef struct { int dummy; } C2D_Image;

#define C2D_DEFAULT_MAX_OBJECTS 4096
#define C2D_WithColor           0

static inline u32 C2D_Color32(u8 r, u8 g, u8 b, u8 a) {
    return ((u32)a << 24) | ((u32)b << 16) | ((u32)g << 8) | r;
}

static inline void   C2D_Init(size_t s)                           { (void)s; }
static inline void   C2D_Fini(void)                               {}
static inline void   C2D_Prepare(void)                            {}
static inline C3D_RenderTarget *C2D_CreateScreenTarget(gfxScreen_t s, gfx3dSide_t d) { (void)s;(void)d; return NULL; }
static inline C2D_TextBuf C2D_TextBufNew(size_t n)                { (void)n; return NULL; }
static inline void   C2D_TextBufDelete(C2D_TextBuf b)             { (void)b; }
static inline void   C2D_TextBufClear(C2D_TextBuf b)              { (void)b; }
static inline void   C2D_TextParse(C2D_Text *t, C2D_TextBuf b, const char *s) { (void)t;(void)b;(void)s; }
static inline void   C2D_TextOptimize(C2D_Text *t)                { (void)t; }
static inline void   C2D_DrawText(const C2D_Text *t, u32 f, float x, float y, float z, float sw, float sh, u32 c) { (void)t;(void)f;(void)x;(void)y;(void)z;(void)sw;(void)sh;(void)c; }
static inline void   C2D_DrawRectSolid(float x, float y, float z, float w, float h, u32 c) { (void)x;(void)y;(void)z;(void)w;(void)h;(void)c; }
static inline void   C2D_TargetClear(C3D_RenderTarget *t, u32 c)  { (void)t;(void)c; }
static inline void   C2D_SceneBegin(C3D_RenderTarget *t)           { (void)t; }

#endif /* SHIMS_CITRO2D_H */
