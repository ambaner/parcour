/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * physics.c — Physics helpers and tile-map collision resolution
 *
 * Depends on: types.h, math.h, level.h (for tile_solid)
 */

#include "types.h"
#include "math.h"
#include "physics.h"
#include "level.h"

/* ── Velocity helpers ───────────────────────────────────────────────── */

/*
 * physics_apply_accel — Add directional acceleration to horizontal velocity.
 *   vx:    pointer to current horizontal velocity
 *   accel: acceleration magnitude (e.g. GROUND_ACCEL or AIR_CONTROL)
 *   dir:   -1 (left) or +1 (right)
 * Result is clamped to [-MAX_RUN_SPEED, MAX_RUN_SPEED].
 */
void physics_apply_accel(float *vx, float accel, int dir) {
    *vx += accel * dir;
    *vx = clampf(*vx, -MAX_RUN_SPEED, MAX_RUN_SPEED);
}

/*
 * physics_apply_friction — Dampen horizontal velocity by a friction multiplier.
 *   vx:       pointer to current horizontal velocity
 *   friction: multiplier per frame (e.g. 0.82 for ground, 0.98 for air)
 * Snaps to zero when speed drops below STOP_THRESHOLD to avoid sub-pixel drift.
 */
void physics_apply_friction(float *vx, float friction) {
    *vx *= friction;
    if (absf(*vx) < STOP_THRESHOLD)
        *vx = 0;
}

/* ── Horizontal collision ───────────────────────────────────────────── */
/*
 * Check left/right collision across the character's height.
 * ALL heights use tile_solid — the character cannot pass through any
 * solid tile at any body height.
 *
 * Only the LEADING edge is checked (the side toward movement direction).
 * This prevents "stuck" scenarios where the character lands from a jump
 * with its body partially overlapping a barrier — it can still walk
 * away from the overlap because the trailing edge isn't checked.
 */
int physics_collide_horizontal(float *x, float *vx, float y) {
    if (*vx == 0.0f) return 0;

    float nx = *x + *vx;
    int heights[5];
    heights[0] = (int)y + 4;            /* head */
    heights[1] = (int)y + RENDER_H / 4; /* up */
    heights[2] = (int)y + RENDER_H / 2; /* mid */
    heights[3] = (int)y + RENDER_H * 3 / 4; /* low */
    heights[4] = (int)y + RENDER_H - 2; /* foot */

    if (*vx > 0.0f) {
        /* Moving right — check right edge of new position */
        int newR = (int)nx + RENDER_W - 9;
        int curR = (int)*x + RENDER_W - 9;
        for (int i = 0; i < 5; i++) {
            if (tile_solid(newR, heights[i])) {
                /* Only block if this is a NEW solid tile we'd enter.
                 * If our current position already overlaps solid at this
                 * height (e.g. ceiling above us), don't block horizontal. */
                if (!tile_solid(curR, heights[i])) {
                    *vx = 0;
                    return 1;
                }
            }
        }
    } else {
        /* Moving left — check left edge of new position */
        int newL = (int)nx + 8;
        int curL = (int)*x + 8;
        for (int i = 0; i < 5; i++) {
            if (tile_solid(newL, heights[i])) {
                /* Only block if entering a new solid column at this height */
                if (!tile_solid(curL, heights[i])) {
                    *vx = 0;
                    return 1;
                }
            }
        }
    }

    *x = nx;
    return 0;
}

/* ── Vertical collision (falling / landing) ─────────────────────────── */
int physics_collide_vertical(float *y, float *vy, float x,
                             int *onGround, float *landingVy) {
    if (*vy < 0) {
        return physics_collide_ceiling(y, vy, x);
    }

    float ny = *y + *vy;
    int footC = (int)x + RENDER_W / 2;        /* center foot probe */
    int footY = (int)ny + RENDER_H;

    /* Landing: require center foot on solid ground.  This prevents
     * the character from "standing" on an edge with only one foot
     * on the platform — if the center of mass is past the edge,
     * they fall off.  Consistent with the edge-overhang rule. */
    if (tile_solid(footC, footY)) {
        int tileRow = footY / TILE_SIZE;
        *y = (float)(tileRow * TILE_SIZE - RENDER_H);
        *landingVy = *vy;
        *vy = 0;
        *onGround = 1;
        return 1;
    }

    *y = ny;
    *onGround = 0;
    return 0;
}

/* ── Ceiling collision (rising) ─────────────────────────────────────── */
/*
 * When rising (vy < 0), check for solid tiles the character's head would
 * enter. Uses 3 probe points across the width and sweeps tile-by-tile
 * from the current head position to the destination to prevent tunneling.
 */
int physics_collide_ceiling(float *y, float *vy, float x) {
    float ny = *y + *vy;

    /* Probe points across the character's width */
    int probeX[3];
    probeX[0] = (int)x + 10;               /* left edge inset */
    probeX[1] = (int)x + RENDER_W / 2;     /* center */
    probeX[2] = (int)x + RENDER_W - 11;    /* right edge inset */

    /* Which tile row is the head currently in vs. where it will be?
     * Head pixel is at y (top of bounding box). We want the tile row
     * the head will enter at the new position. */
    int curHeadRow = (int)(*y) / TILE_SIZE;     /* current head tile row */
    int newHeadRow = (int)ny / TILE_SIZE;       /* destination head tile row */

    /* Sweep upward from the row above current into the destination row.
     * We skip curHeadRow itself because the character is already there. */
    for (int row = curHeadRow - 1; row >= newHeadRow; row--) {
        /* Check if any probe hits a solid tile in this row.
         * Probe at the middle of the tile row for reliable detection. */
        int py = row * TILE_SIZE + TILE_SIZE / 2;
        for (int i = 0; i < 3; i++) {
            if (tile_solid(probeX[i], py)) {
                /* Stop: place character so head is just below this tile */
                *y = (float)((row + 1) * TILE_SIZE);
                *vy = 0;
                return 1;
            }
        }
    }
    *y = ny;
    return 0;
}
