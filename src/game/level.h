/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * level.h — Tile map data, collision queries, and tile rendering
 */
#ifndef LEVEL_H
#define LEVEL_H

#include "types.h"

/* ── Tile map (0=air, 1=solid) ──────────────────────────────────────── */
extern int level[LEVEL_ROWS][LEVEL_COLS];

/* ── Queries ────────────────────────────────────────────────────────── */
int tile_solid(int px, int py);

/* True only for multi-tile-high walls, not thin 1-tile platforms.
 * Used to distinguish structures you can wall-slide on from thin
 * floors that only block vertically. */
int tile_wall(int px, int py);

/* Check if there's a wall next to character at facing direction */
int tile_wall_beside(float x, float y, int facing);

/* Check if there's a grabbable ledge (solid below, air above) near the
 * character's top while airborne.  Scans a vertical window around the
 * "hand zone" and returns 1 with the snap Y position in *grabY. */
int tile_ledge_nearby(float x, float y, int facing, float *grabY);

/* Check if there's a head-height ledge the character can grab while grounded.
 * Returns 1 and sets *grabY if a platform edge is at head level ahead. */
int tile_head_ledge(float x, float y, int facing, float *grabY);

/* ── Drawing ────────────────────────────────────────────────────────── */
void level_draw(void);

#endif /* LEVEL_H */
