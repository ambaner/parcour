/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * math.h — Game math utilities
 */
#ifndef GAME_MATH_H
#define GAME_MATH_H

/* Return the absolute value of v. */
float absf(float v);

/* Clamp v to the range [lo, hi]. */
float clampf(float v, float lo, float hi);

/* Return +1, -1, or 0 depending on the sign of v. */
float signf(float v);

/* Linearly interpolate between a and b by factor t (0..1). */
float lerpf(float a, float b, float t);

/* Return the smaller of a and b. */
float minf(float a, float b);

/* Return the larger of a and b. */
float maxf(float a, float b);

#endif /* GAME_MATH_H */
