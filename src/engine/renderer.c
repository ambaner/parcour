/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * renderer.c — Framebuffer ownership and drawing primitives
 *
 * Depends on: types.h only.
 */

#include "renderer.h"

UINT32 framebuf[SCREEN_W * SCREEN_H];

/* Fill the entire framebuffer with a uniform ARGB color. */
void renderer_clear(UINT32 color) {
    for (int i = 0; i < SCREEN_W * SCREEN_H; i++)
        framebuf[i] = color;
}

/* Write a single pixel into the framebuffer (no-op if out of bounds). */
void renderer_put_pixel(int x, int y, UINT32 color) {
    if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H)
        framebuf[y * SCREEN_W + x] = color;
}

/* Read one pixel from the framebuffer; returns 0 for out-of-bounds. */
UINT32 renderer_get_pixel(int x, int y) {
    if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H)
        return framebuf[y * SCREEN_W + x];
    return 0;
}

/* Fill a rectangle, clipping each pixel individually to screen bounds. */
void renderer_fill_rect(int x, int y, int w, int h, UINT32 color) {
    for (int row = 0; row < h; row++) {
        int sy = y + row;
        if (sy < 0 || sy >= SCREEN_H) continue;
        for (int col = 0; col < w; col++) {
            int sx = x + col;
            if (sx < 0 || sx >= SCREEN_W) continue;
            framebuf[sy * SCREEN_W + sx] = color;
        }
    }
}

/*
 * renderer_draw_sprite — Render a character-grid sprite scaled up.
 *
 * Each cell of the SPRITE_GRID_W x SPRITE_GRID_H character array maps to
 * a SPRITE_SCALE x SPRITE_SCALE block of pixels on screen.  The color_fn
 * callback translates each character to an ARGB color; returning 0 means
 * "transparent" (the pixel is skipped).
 *
 * When flip is set, columns are read in reverse order so the sprite
 * appears mirrored horizontally — used for facing-left vs. facing-right.
 */
void renderer_draw_sprite(const char *data, int px, int py, int flip,
                          UINT32 (*color_fn)(char)) {
    for (int row = 0; row < SPRITE_GRID_H; row++) {
        for (int col = 0; col < SPRITE_GRID_W; col++) {
            char c = data[row * SPRITE_GRID_W + col];
            UINT32 color = color_fn(c);
            if (color == 0) continue;               // transparent cell — skip
            int srcCol = flip ? (SPRITE_GRID_W - 1 - col) : col;  // mirror column if flipped
            int bx = px + srcCol * SPRITE_SCALE;     // top-left x of scaled block
            int by = py + row * SPRITE_SCALE;        // top-left y of scaled block
            // Fill the SPRITE_SCALE x SPRITE_SCALE block for this cell
            for (int dy = 0; dy < SPRITE_SCALE; dy++) {
                int sy = by + dy;
                if (sy < 0 || sy >= SCREEN_H) continue;
                for (int dx = 0; dx < SPRITE_SCALE; dx++) {
                    int sx = bx + dx;
                    if (sx < 0 || sx >= SCREEN_W) continue;
                    framebuf[sy * SCREEN_W + sx] = color;
                }
            }
        }
    }
}

/* ── Skeletal drawing primitives ────────────────────────────────────── */

/*
 * renderer_fill_circle — Rasterize a filled circle using the midpoint distance test.
 *
 * Iterates over a bounding square from -r to +r in both axes.
 * A pixel is inside the circle when dx*dx + dy*dy <= r*r.
 * Simple brute-force approach; sufficient for the small radii used by the
 * skeletal limb system (typically 2-6 pixels).
 */
void renderer_fill_circle(int cx, int cy, int r, UINT32 color) {
    int r2 = r * r;  // precompute radius squared for the distance test
    for (int dy = -r; dy <= r; dy++) {
        int sy = cy + dy;
        if (sy < 0 || sy >= SCREEN_H) continue;
        for (int dx = -r; dx <= r; dx++) {
            if (dx * dx + dy * dy <= r2) {
                int sx = cx + dx;
                if (sx >= 0 && sx < SCREEN_W)
                    framebuf[sy * SCREEN_W + sx] = color;
            }
        }
    }
}

/*
 * renderer_fill_ellipse — Rasterize a filled axis-aligned ellipse.
 *
 * Uses the implicit ellipse equation: (dx/rx)^2 + (dy/ry)^2 <= 1,
 * rearranged to integer form (dx*dx*ry2 + dy*dy*rx2 <= rx2*ry2)
 * to avoid floating-point math.  Used for the character's head/torso.
 */
void renderer_fill_ellipse(int cx, int cy, int rx, int ry, UINT32 color) {
    int rx2 = rx * rx;       // precompute squared radii
    int ry2 = ry * ry;
    int rxry = rx2 * ry2;    // combined threshold for the integer distance test
    for (int dy = -ry; dy <= ry; dy++) {
        int sy = cy + dy;
        if (sy < 0 || sy >= SCREEN_H) continue;
        for (int dx = -rx; dx <= rx; dx++) {
            if (dx * dx * ry2 + dy * dy * rx2 <= rxry) {
                int sx = cx + dx;
                if (sx >= 0 && sx < SCREEN_W)
                    framebuf[sy * SCREEN_W + sx] = color;
            }
        }
    }
}

/*
 * renderer_draw_limb — Draw a tapered limb by stamping circles along a line.
 *
 * Walks from (x1,y1) to (x2,y2) in sub-pixel steps (2× the Manhattan
 * distance to avoid gaps), linearly interpolating the circle radius from
 * w1 to w2.  If the endpoints coincide, draws a single circle instead.
 * This "brush stroke" approach naturally produces rounded endpoints and
 * smooth tapering for the skeletal character's arms and legs.
 */
void renderer_draw_limb(int x1, int y1, int x2, int y2,
                        int w1, int w2, UINT32 color) {
    int dx = x2 - x1, dy = y2 - y1;
    int adx = dx < 0 ? -dx : dx;
    int ady = dy < 0 ? -dy : dy;
    int steps = (adx > ady ? adx : ady) * 2;  // 2× oversampling prevents gaps
    if (steps == 0) {
        renderer_fill_circle(x1, y1, w1, color);
        return;
    }
    for (int i = 0; i <= steps; i++) {
        int cx = x1 + dx * i / steps;           // interpolated center x
        int cy = y1 + dy * i / steps;           // interpolated center y
        int rr = w1 + (w2 - w1) * i / steps;   // interpolated radius
        renderer_fill_circle(cx, cy, rr, color);
    }
}

/*
 * renderer_draw_limb_shaded — Draw a limb with a 1-pixel dark outline.
 *
 * Achieves the outline effect by drawing the limb twice: first at radius+1
 * in the outline color, then at the original radius in the fill color.
 * The fill overdraw covers the interior, leaving a 1px border visible.
 */
void renderer_draw_limb_shaded(int x1, int y1, int x2, int y2,
                               int w1, int w2,
                               UINT32 fill, UINT32 outline) {
    renderer_draw_limb(x1, y1, x2, y2, w1 + 1, w2 + 1, outline);
    renderer_draw_limb(x1, y1, x2, y2, w1, w2, fill);
}
