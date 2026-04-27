/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * types.h — Core constants and shared types for Parcour
 *
 * This is the root header — no dependencies except <windows.h>.
 * Every other module includes this.
 */
#ifndef TYPES_H
#define TYPES_H

#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include <windows.h>

/* ── Display ────────────────────────────────────────────────────────── */
#define SCALE        1            /* window-to-framebuffer scale (1 = native resolution)  */
#define SCREEN_W  1280            /* framebuffer width in pixels                          */
#define SCREEN_H   960            /* framebuffer height in pixels                         */
#define WINDOW_W   (SCREEN_W * SCALE)
#define WINDOW_H   (SCREEN_H * SCALE)
#define FPS         60            /* target frame rate                                    */
#define FRAME_MS    (1000 / FPS)  /* milliseconds per frame (~16 ms at 60 FPS)            */

/* ── Tiles ──────────────────────────────────────────────────────────── */
#define TILE_SIZE   32            /* square tile edge length in pixels                    */
#define LEVEL_COLS  (SCREEN_W / TILE_SIZE)   /* 40  — columns that fit the screen width  */
#define LEVEL_ROWS  (SCREEN_H / TILE_SIZE)   /* 30  — rows that fit the screen height    */

/* ── Grid-sprite dimensions (used by renderer_draw_sprite) ─────────── */
#define SPRITE_GRID_W      24    /* character-art grid width (cells)                      */
#define SPRITE_GRID_H      32    /* character-art grid height (cells)                     */
#define SPRITE_SCALE   3         /* each grid cell maps to 3×3 pixels on screen           */

/* ── Character bounding box (used by physics + drawing) ────────────── */
#define RENDER_W      64         /* bounding box width for collision and rendering        */
#define RENDER_H     128         /* bounding box height (≈4 tiles tall)                   */
#define HIP_OFFSET_Y  64         /* hip joint Y relative to bounding-box top — the pivot
                                  * point for the skeletal system's upper/lower body split */

#endif /* TYPES_H */
