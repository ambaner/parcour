/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * levelgen.h — Random level (maze) generator
 *
 * Generates valid .parcour levels with random internal structure.
 * All generated levels pass levelfile_validate() by construction.
 *
 * Part of the engine library. Used by editor and tests.
 */
#ifndef LEVELGEN_H
#define LEVELGEN_H

#include "levelfile.h"

/* ── Generation parameters ──────────────────────────────────────────── */
typedef struct {
    unsigned int seed;       /* RNG seed (0 = use time-based seed) */
    int density;             /* Wall density: 0-100 (higher = more walls) */
    int platformCount;       /* Target number of platforms to place */
    int verticalWalls;       /* Number of vertical wall segments */
} LevelGenParams;

/* Default parameters for a playable maze */
#define LEVELGEN_DEFAULT_PARAMS { 0, 35, 8, 4 }

/* ── API ────────────────────────────────────────────────────────────── */

/* Generate a random level. Result is guaranteed to pass validation.
 * Returns 1 on success, 0 if generation failed after retries. */
int levelgen_generate(const LevelGenParams *params, LevelData *out);

/* Generate with just a seed (uses default params otherwise). */
int levelgen_generate_seeded(unsigned int seed, LevelData *out);

#endif /* LEVELGEN_H */
