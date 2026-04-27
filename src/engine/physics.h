/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * physics.h — Physics constants and movement helpers
 */
#ifndef PHYSICS_H
#define PHYSICS_H

/* ── Movement constants ─────────────────────────────────────────────── */
#define GROUND_ACCEL      0.55f   /* horizontal acceleration per frame on ground          */
#define GROUND_FRICTION   0.82f   /* velocity multiplier per frame on ground (lower=more)  */
#define TURN_DECEL        0.70f   /* extra drag when reversing direction (snappy turnaround) */
#define AIR_CONTROL       0.08f   /* horizontal acceleration while airborne (intentionally weak) */
#define AIR_FRICTION      0.98f   /* near-1.0 keeps air momentum feeling smooth and floaty */
#define MAX_RUN_SPEED     6.0f    /* hard cap on horizontal velocity in pixels per frame   */
#define STOP_THRESHOLD    0.05f   /* velocity below this snaps to zero (prevents sub-pixel creep) */

/* ── Gravity & jumping ──────────────────────────────────────────────── */
/* Gravity constants moved to gravity.h — use gravity_apply() instead  */
#define JUMP_IMPULSE    -10.0f    /* initial upward velocity on jump (negative = up)       */
#define JUMP_HOLD_BOOST   0.35f   /* additional upward force each frame while jump is held */
#define JUMP_HOLD_MAX     10      /* max frames the player can extend a jump by holding    */
#define MAX_FALL         18.0f    /* kept in sync with GRAV_TERMINAL                       */

/* ── Timing ─────────────────────────────────────────────────────────── */
#define LANDING_TICKS      8      /* frames of landing-recovery animation                  */
#define TURN_TICKS         5      /* frames of turn-around animation                       */

/* ── Helpers that operate on velocity pairs ─────────────────────────── */
/* Apply directional acceleration to horizontal velocity, clamped to MAX_RUN_SPEED. */
void physics_apply_accel(float *vx, float accel, int dir);

/* Multiply vx by friction factor; snap to zero if below STOP_THRESHOLD. */
void physics_apply_friction(float *vx, float friction);

/*
 * Collision resolution against the tile map.
 * Each function reads position + velocity, writes back corrected values.
 * Returns 1 if a collision occurred, 0 otherwise.
 */
int physics_collide_horizontal(float *x, float *vx, float y);
int physics_collide_vertical(float *y, float *vy, float x,
                             int *onGround, float *landingVy);
int physics_collide_ceiling(float *y, float *vy, float x);

#endif /* PHYSICS_H */
