/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * renderer.h — Framebuffer and drawing primitives
 */
#ifndef RENDERER_H
#define RENDERER_H

#include "types.h"

/* ── The shared framebuffer (SCREEN_W x SCREEN_H, ARGB) ────────────── */
extern UINT32 framebuf[SCREEN_W * SCREEN_H];

/* ── Primitives ─────────────────────────────────────────────────────── */

/* Fill the entire framebuffer with a solid color. */
void renderer_clear(UINT32 color);

/* Set a single pixel (bounds-checked; out-of-bounds writes are ignored). */
void renderer_put_pixel(int x, int y, UINT32 color);

/* Read a single pixel; returns 0 (transparent black) for out-of-bounds. */
UINT32 renderer_get_pixel(int x, int y);

/* Fill an axis-aligned rectangle, clipped to screen bounds. */
void renderer_fill_rect(int x, int y, int w, int h, UINT32 color);

/* ── Sprite drawing (scaled by SPRITE_SCALE) ────────────────────────── */

/*
 * renderer_draw_sprite — Draw a character-art sprite scaled up by SPRITE_SCALE.
 *   data:     SPRITE_GRID_W * SPRITE_GRID_H character array defining the sprite
 *   px, py:   top-left pixel position on screen
 *   flip:     if non-zero, mirror the sprite horizontally
 *   color_fn: maps each character in data to an ARGB color (0 = transparent/skip)
 */
void renderer_draw_sprite(const char *data, int px, int py, int flip,
                          UINT32 (*color_fn)(char));

/* ── Skeletal body-part drawing ─────────────────────────────────────── */

/* Fill a circle centered at (cx,cy) with radius r using the midpoint test. */
void renderer_fill_circle(int cx, int cy, int r, UINT32 color);

/* Fill an axis-aligned ellipse centered at (cx,cy) with radii rx, ry. */
void renderer_fill_ellipse(int cx, int cy, int rx, int ry, UINT32 color);

/*
 * renderer_draw_limb — Draw a tapered limb (line with varying thickness).
 *   (x1,y1)→(x2,y2): limb endpoints
 *   w1, w2:           circle radii at each endpoint (linearly interpolated)
 * Implemented by stamping filled circles along the line.
 */
void renderer_draw_limb(int x1, int y1, int x2, int y2,
                        int w1, int w2, UINT32 color);

/*
 * renderer_draw_limb_shaded — Draw a limb with a 1-pixel outline.
 * Draws the outline limb first (radius+1), then the fill limb on top.
 */
void renderer_draw_limb_shaded(int x1, int y1, int x2, int y2,
                               int w1, int w2,
                               UINT32 fill, UINT32 outline);

#endif /* RENDERER_H */
