/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * math.c — Game math utilities
 *
 * Pure functions with no dependencies — the foundation layer.
 */

#include "math.h"

/* Absolute value of a float. */
float absf(float v) {
    return v < 0 ? -v : v;
}

/* Clamp v to [lo, hi]. */
float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* Sign of v: +1, -1, or 0. */
float signf(float v) {
    if (v > 0) return 1.0f;
    if (v < 0) return -1.0f;
    return 0.0f;
}

/* Linear interpolation: returns a when t=0, b when t=1. */
float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

/* Minimum of two floats. */
float minf(float a, float b) {
    return a < b ? a : b;
}

/* Maximum of two floats. */
float maxf(float a, float b) {
    return a > b ? a : b;
}
