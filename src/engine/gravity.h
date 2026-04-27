/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * gravity.h — Realistic gravity simulation with progressive acceleration
 *
 * Models true free-fall: velocity increases each frame under constant
 * gravitational acceleration, with air drag that produces a natural
 * terminal velocity rather than a hard cap.
 */
#ifndef GRAVITY_H
#define GRAVITY_H

/* ── Gravity constants ──────────────────────────────────────────────── */
#define GRAV_ACCEL        0.48f   /* base gravitational acceleration per frame */
#define GRAV_TERMINAL    18.0f    /* absolute max fall speed (safety cap)      */
#define GRAV_DRAG         0.015f  /* air-resistance coefficient (quadratic)    */

/* ── Landing severity thresholds (based on vy at impact) ───────────── */
#define LANDING_SOFT_VY   4.0f    /* vy < this: instant recovery              */
#define LANDING_MED_VY    8.0f    /* vy < this: stagger landing               */
                                  /* vy >= MED: hard landing (ninja roll)      */

/* ── API ────────────────────────────────────────────────────────────── */

/*
 * gravity_apply — Advance vertical velocity by one frame of gravity.
 *
 * Applies gravitational acceleration with quadratic air drag so that
 * velocity increases realistically: fast at first, then asymptotically
 * approaches terminal velocity.  Short falls feel snappy; tall falls
 * visibly accelerate the whole way down.
 *
 *   vy_next = (vy + GRAV_ACCEL) * (1 - GRAV_DRAG * |vy|)
 *
 * Clamped to GRAV_TERMINAL as a safety net.
 */
void gravity_apply(float *vy);

/*
 * gravity_landing_tier — Classify a landing by impact velocity.
 *
 * Returns:
 *   0 = soft  (vy < LANDING_SOFT_VY)   — instant recovery
 *   1 = medium (SOFT..MED)              — brief stagger
 *   2 = hard  (vy >= LANDING_MED_VY)   — ninja roll / somersault
 */
int gravity_landing_tier(float landingVy);

/*
 * gravity_fall_height_estimate — Approximate pixel distance fallen
 * from rest to reach the given vy.  Useful for logging.
 */
float gravity_fall_height_estimate(float vy);

#endif /* GRAVITY_H */
