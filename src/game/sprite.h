/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * sprite.h — PNG sprite loading and blitting
 *
 * Supports two loading modes:
 *   1. From embedded Win32 resources (RCDATA) — single-exe deployment
 *   2. From loose PNG files on disk — development fallback
 *
 * Uses stb_image for PNG decoding in both modes.
 */
#ifndef SPRITE_H
#define SPRITE_H

#include "types.h"

/* Maximum frames per animation */
#define MAX_ANIM_FRAMES 16

/* A single sprite frame (loaded from PNG) */
typedef struct {
    UINT32 *pixels;    /* ARGB pixel data (pre-converted from RGBA) */
    int w, h;          /* original dimensions */
} SpriteFrame;

/* An animation: a named sequence of frames */
typedef struct {
    SpriteFrame frames[MAX_ANIM_FRAMES];
    int count;         /* number of frames loaded */
    int speed;         /* ticks per frame */
    int loop;          /* 1 = loop, 0 = hold last frame */
} SpriteAnim;

/* Load a sequence of frames from embedded RCDATA resources.
 * baseId = first resource ID; loads baseId+0, baseId+1, ... until
 * FindResource fails.  Returns number of frames loaded. */
int sprite_anim_load_rc(SpriteAnim *anim, int baseId,
                        int speed, int loop);

/* Load a sequence of PNGs from disk: dir/prefix-00.png, prefix-01.png, ...
 * Returns number of frames loaded (0 on failure). */
int sprite_anim_load(SpriteAnim *anim, const char *dir,
                     const char *prefix, int speed, int loop);

/* Free all frames in an animation */
void sprite_anim_free(SpriteAnim *anim);

/* Blit a sprite frame to the framebuffer with scaling and optional flip.
 * x,y = top-left in framebuffer coords. scale = integer multiplier.
 * flipH = 1 to mirror horizontally (for facing left). */
void sprite_blit(const SpriteFrame *frame, int x, int y,
                 int scale, int flipH);

#endif /* SPRITE_H */
