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
 * Check left/right collision at foot-level and mid-body.  Thin platforms
 * at head height are intentionally passable from the side — you jump
 * on top of them.  Full-height walls still block at both probes.
 */
int physics_collide_horizontal(float *x, float *vx, float y) {
    float nx = *x + *vx;
    int bodyL = (int)nx + 8;
    int bodyR = (int)nx + RENDER_W - 9;
    int headY = (int)y + 4;                    /* top of bounding box    */
    int upY   = (int)y + RENDER_H / 4;        /* upper quarter          */
    int midY  = (int)y + RENDER_H / 2;        /* middle                 */
    int lowY  = (int)y + RENDER_H * 3 / 4;    /* lower quarter          */
    int footY = (int)y + RENDER_H - 2;        /* foot level             */

    /* Only check mid-body and below for horizontal wall collision.
     * Use tile_wall (not tile_solid) so thin 1-tile platforms don't
     * block horizontal movement — they only block from above (landing).
     * Head-level platform interaction is handled in character.c. */
    (void)headY;
    (void)upY;

    if (tile_wall(bodyL, footY) || tile_wall(bodyR, footY) ||
        tile_wall(bodyL, lowY)  || tile_wall(bodyR, lowY)  ||
        tile_wall(bodyL, midY)  || tile_wall(bodyR, midY)) {
        *vx = 0;
        return 1;
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
int physics_collide_ceiling(float *y, float *vy, float x) {
    float ny = *y + *vy;
    int headL = (int)x + 10;
    int headR = (int)x + RENDER_W - 11;
    int headY = (int)ny;

    if (tile_solid(headL, headY) || tile_solid(headR, headY)) {
        *vy = 0;
        return 1;
    }
    *y = ny;
    return 0;
}
