/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * gravity.c — Realistic gravity with air drag and landing classification
 *
 * Depends on: gravity.h, log.h, math.h
 */

#include "gravity.h"
#include "math.h"
#include "log.h"

/* ── Apply one frame of gravity ─────────────────────────────────────── */
void gravity_apply(float *vy)
{
    /* Accelerate */
    *vy += GRAV_ACCEL;

    /* Quadratic air drag — only when FALLING (vy > 0).
     * Rising (vy < 0) uses pure gravity so jump height is preserved.
     * drag_factor = 1 - GRAV_DRAG * vy, clamped to [0.80, 1.0]      */
    if (*vy > 0.0f) {
        float drag = 1.0f - GRAV_DRAG * (*vy);
        if (drag < 0.80f) drag = 0.80f;
        *vy *= drag;
    }

    /* Safety cap — should rarely trigger due to drag */
    if (*vy > GRAV_TERMINAL) {
        *vy = GRAV_TERMINAL;
    }
}

/* ── Classify landing severity ──────────────────────────────────────── */
int gravity_landing_tier(float landingVy)
{
    if (landingVy < LANDING_SOFT_VY)  return 0;   /* soft  */
    if (landingVy < LANDING_MED_VY)   return 1;   /* medium */
    return 2;                                      /* hard  */
}

/* ── Estimate fall height from rest to reach given vy ───────────────── */
/*
 * Uses kinematic approximation (ignoring drag for simplicity):
 *   h ≈ vy² / (2 * g)
 * This gives a rough pixel distance for logging purposes.
 */
float gravity_fall_height_estimate(float vy)
{
    if (vy <= 0.0f) return 0.0f;
    return (vy * vy) / (2.0f * GRAV_ACCEL);
}
