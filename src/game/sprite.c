/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * sprite.c — PNG sprite loading and alpha-blended blitting
 *
 * Uses stb_image (public domain) for PNG decoding.
 * Converts RGBA → ARGB on load for direct framebuffer compatibility.
 */

#define STB_IMAGE_IMPLEMENTATION
#include <math.h>        /* stb_image uses pow() — must precede its include */
#pragma warning(push)
#pragma warning(disable: 4013)  /* 'pow' undefined — stb_image is 3rd-party */
#define STBI_ONLY_PNG
#define STBI_NO_STDIO   /* we'll use stbi_load for simplicity, re-enable */
#undef STBI_NO_STDIO
#include "stb_image.h"
#pragma warning(pop)

#include "sprite.h"
#include "renderer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Load frames from embedded Win32 RCDATA resources ──────────────── */

/*
 * decode_rgba_to_argb — Decode a PNG from memory and convert pixel layout.
 *
 * stb_image decodes PNGs into RGBA byte order (R at byte 0, A at byte 3),
 * but Win32 BITMAPINFO / StretchDIBits expects ARGB packed into UINT32
 * with alpha in the high byte: 0xAARRGGBB.  This function performs that
 * channel shuffle during load so blitting can work directly with the
 * framebuffer without per-pixel conversion at draw time.
 *
 * data    — raw PNG file bytes (from resource or disk)
 * dataLen — byte count of the PNG data
 * frame   — output: receives allocated ARGB pixel buffer + dimensions
 *
 * Returns 1 on success, 0 on decode or allocation failure.
 */
static int decode_rgba_to_argb(const unsigned char *data, int dataLen,
                               SpriteFrame *frame)
{
    int w, h, channels;
    unsigned char *rgba = stbi_load_from_memory(data, dataLen,
                                                &w, &h, &channels, 4);
    if (!rgba) return 0;

    UINT32 *argb = (UINT32 *)malloc(w * h * sizeof(UINT32));
    if (!argb) { stbi_image_free(rgba); return 0; }

    for (int p = 0; p < w * h; p++) {
        unsigned char r = rgba[p * 4 + 0];
        unsigned char g = rgba[p * 4 + 1];
        unsigned char b = rgba[p * 4 + 2];
        unsigned char a = rgba[p * 4 + 3];
        argb[p] = ((UINT32)a << 24) | ((UINT32)r << 16) |
                  ((UINT32)g << 8) | (UINT32)b;
    }

    stbi_image_free(rgba);
    frame->pixels = argb;
    frame->w = w;
    frame->h = h;
    return 1;
}

int sprite_anim_load_rc(SpriteAnim *anim, int baseId,
                        int speed, int loop)
{
    memset(anim, 0, sizeof(*anim));
    anim->speed = speed;
    anim->loop = loop;

    /* Win32 resource API chain:
     *   FindResource  → locates the resource by ID + type in the PE
     *   LoadResource  → maps the resource data into memory
     *   LockResource  → returns a direct pointer to the bytes (no copy)
     *   SizeofResource → byte count so stb_image knows how much to read
     * Resources stay valid for the process lifetime (no FreeResource needed). */
    HMODULE hModule = GetModuleHandle(NULL);

    for (int i = 0; i < MAX_ANIM_FRAMES; i++) {
        // Each frame's resource ID = baseId + frame index
        HRSRC hRes = FindResource(hModule, MAKEINTRESOURCE(baseId + i),
                                  RT_RCDATA);
        if (!hRes) break;   // no more frames for this animation

        HGLOBAL hData = LoadResource(hModule, hRes);
        if (!hData) break;

        const unsigned char *pngData =
            (const unsigned char *)LockResource(hData);
        DWORD pngSize = SizeofResource(hModule, hRes);
        if (!pngData || pngSize == 0) break;

        if (!decode_rgba_to_argb(pngData, (int)pngSize,
                                 &anim->frames[i]))
            break;

        anim->count = i + 1;
    }

    return anim->count;
}

/* ── Load a numbered sequence of PNGs from disk ────────────────────── */
int sprite_anim_load(SpriteAnim *anim, const char *dir,
                     const char *prefix, int speed, int loop)
{
    memset(anim, 0, sizeof(*anim));
    anim->speed = speed;
    anim->loop = loop;

    for (int i = 0; i < MAX_ANIM_FRAMES; i++) {
        /* Filename convention: <prefix>-<NN>.png where NN is zero-padded.
         * e.g. "adventurer-idle-00.png", "adventurer-idle-01.png", ...
         * Loading stops at the first missing file (no gaps allowed). */
        char path[512];
        snprintf(path, sizeof(path), "%s\\%s-%02d.png", dir, prefix, i);

        int w, h, channels;
        unsigned char *rgba = stbi_load(path, &w, &h, &channels, 4);
        if (!rgba) break;   /* no more frames */

        /* Convert RGBA → ARGB (Windows BITMAPINFO format) */
        UINT32 *argb = (UINT32 *)malloc(w * h * sizeof(UINT32));
        if (!argb) { stbi_image_free(rgba); break; }

        for (int p = 0; p < w * h; p++) {
            unsigned char r = rgba[p * 4 + 0];
            unsigned char g = rgba[p * 4 + 1];
            unsigned char b = rgba[p * 4 + 2];
            unsigned char a = rgba[p * 4 + 3];
            argb[p] = ((UINT32)a << 24) | ((UINT32)r << 16) |
                      ((UINT32)g << 8) | (UINT32)b;
        }

        stbi_image_free(rgba);

        anim->frames[i].pixels = argb;
        anim->frames[i].w = w;
        anim->frames[i].h = h;
        anim->count = i + 1;
    }

    return anim->count;
}

/* ── Free animation frames ─────────────────────────────────────────── */
void sprite_anim_free(SpriteAnim *anim)
{
    for (int i = 0; i < anim->count; i++) {
        free(anim->frames[i].pixels);
        anim->frames[i].pixels = NULL;
    }
    anim->count = 0;
}

/* ── Alpha-blended, scaled blit with optional horizontal flip ──────── */
void sprite_blit(const SpriteFrame *frame, int x, int y,
                 int scale, int flipH)
{
    if (!frame || !frame->pixels) return;

    int fw = frame->w;
    int fh = frame->h;
    int dw = fw * scale;
    int dh = fh * scale;

    /* Nearest-neighbor scaling: each destination pixel (dx, dy) maps back
     * to a source pixel at (dx/scale, dy/scale) via integer division.
     * This gives the characteristic blocky pixel-art look. */
    for (int dy = 0; dy < dh; dy++) {
        int sy = dy / scale;          // source row for this dest row
        if (sy >= fh) continue;

        int screenY = y + dy;
        if (screenY < 0 || screenY >= SCREEN_H) continue;

        for (int dx = 0; dx < dw; dx++) {
            int sx = dx / scale;      // source column for this dest column
            if (sx >= fw) continue;

            // Horizontal flip: mirror the source X for left-facing sprites
            if (flipH) sx = fw - 1 - sx;

            UINT32 src = frame->pixels[sy * fw + sx];
            unsigned int a = (src >> 24) & 0xFF;
            if (a == 0) continue;   /* fully transparent */

            int screenX = x + dx;
            if (screenX < 0 || screenX >= SCREEN_W) continue;

            if (a >= 250) {
                /* Nearly or fully opaque — fast path: skip the blend math
                 * for pixels that are >= 98% opaque.  The visual difference
                 * between alpha 250 and 255 is imperceptible, and this
                 * avoids an expensive read-modify-write for the vast
                 * majority of sprite pixels. */
                renderer_put_pixel(screenX, screenY, src | 0xFF000000);
            } else {
                /* Standard alpha blend: out = src * alpha + dst * (1-alpha)
                 * Done per-channel in [0..255] integer math to avoid floats.
                 * Division by 255 (not 256) keeps white·255 = white. */
                UINT32 dst = renderer_get_pixel(screenX, screenY);
                unsigned int invA = 255 - a;
                unsigned int rr = (((src >> 16) & 0xFF) * a +
                                   ((dst >> 16) & 0xFF) * invA) / 255;
                unsigned int gg = (((src >> 8) & 0xFF) * a +
                                   ((dst >> 8) & 0xFF) * invA) / 255;
                unsigned int bb = ((src & 0xFF) * a +
                                   (dst & 0xFF) * invA) / 255;
                renderer_put_pixel(screenX, screenY,
                                   0xFF000000 | (rr << 16) | (gg << 8) | bb);
            }
        }
    }
}
