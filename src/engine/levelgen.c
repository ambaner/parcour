/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * levelgen.c — Random level (maze) generator
 *
 * Algorithm overview:
 *   1. Fill all borders solid, interior air
 *   2. Place random horizontal platforms (ensuring 4-tile clearance above)
 *   3. Place random vertical wall segments (ensuring 2-tile-wide gaps)
 *   4. Find a valid spawn point (air tile with ground below)
 *   5. Validate the result; retry with next seed if invalid
 *
 * Uses a simple LCG (linear congruential generator) for portability.
 * The same seed always produces the same level.
 */

#include "levelgen.h"
#include <string.h>
#include <stdio.h>

/* ── Simple LCG random number generator ─────────────────────────────── */
typedef struct {
    unsigned int state;
} Rng;

static void rng_init(Rng *rng, unsigned int seed) {
    rng->state = seed ? seed : 12345u;
}

static unsigned int rng_next(Rng *rng) {
    rng->state = rng->state * 1103515245u + 12345u;
    return (rng->state >> 16) & 0x7FFF;
}

/* Random int in [lo, hi] inclusive */
static int rng_range(Rng *rng, int lo, int hi) {
    if (lo >= hi) return lo;
    return lo + (int)(rng_next(rng) % (unsigned int)(hi - lo + 1));
}

/* ── Character dimensions in tiles ──────────────────────────────────── */
#define CHAR_W  (RENDER_W / TILE_SIZE)   /* 2 tiles wide */
#define CHAR_H  (RENDER_H / TILE_SIZE)   /* 4 tiles tall */

/* ── Internal: place platforms ──────────────────────────────────────── */
static void place_platforms(Rng *rng, LevelData *data, int count) {
    for (int i = 0; i < count; i++) {
        int len = rng_range(rng, 3, 10);
        int col = rng_range(rng, 2, LEVEL_COLS - len - 2);
        int row = rng_range(rng, CHAR_H + 1, LEVEL_ROWS - 3);

        /* Ensure character clearance above: 4 rows of air above platform */
        int blocked = 0;
        for (int dy = 1; dy <= CHAR_H && !blocked; dy++) {
            if (row - dy < 1) { blocked = 1; break; }
            for (int dx = 0; dx < len && !blocked; dx++) {
                if (data->tiles[row - dy][col + dx] != 0)
                    blocked = 1;
            }
        }
        if (blocked) continue;

        /* Place the platform */
        for (int dx = 0; dx < len; dx++) {
            data->tiles[row][col + dx] = 1;
        }
    }
}

/* ── Internal: place vertical walls ─────────────────────────────────── */
static void place_vertical_walls(Rng *rng, LevelData *data, int count) {
    for (int i = 0; i < count; i++) {
        int height = rng_range(rng, 4, LEVEL_ROWS / 2);
        int col = rng_range(rng, 3, LEVEL_COLS - 4);
        int startRow = rng_range(rng, 1, LEVEL_ROWS - height - 1);

        /* Ensure at least CHAR_W (2) tiles of gap on each side */
        int tooClose = 0;
        for (int dy = 0; dy < height && !tooClose; dy++) {
            int r = startRow + dy;
            /* Check left neighbor */
            if (col >= 2 && data->tiles[r][col - 1] == 1)
                tooClose = 1;
            /* Check right neighbor */
            if (col + 1 < LEVEL_COLS - 1 && data->tiles[r][col + 1] == 1)
                tooClose = 1;
        }
        if (tooClose) continue;

        /* Place the wall */
        for (int dy = 0; dy < height; dy++) {
            data->tiles[startRow + dy][col] = 1;
        }

        /* Cut a gap for the character to pass through (CHAR_H tall) */
        int gapStart = rng_range(rng, startRow + 1,
                                 startRow + height - CHAR_H - 1);
        if (gapStart < startRow + 1) gapStart = startRow + 1;
        for (int dy = 0; dy < CHAR_H; dy++) {
            int r = gapStart + dy;
            if (r > 0 && r < LEVEL_ROWS - 1)
                data->tiles[r][col] = 0;
        }
    }
}

/* ── Internal: scatter random solid tiles based on density ──────────── */
static void scatter_fill(Rng *rng, LevelData *data, int density) {
    /* density is 0-100, representing percentage chance per eligible cell */
    /* Only fill cells that have enough clearance around them */
    for (int row = 2; row < LEVEL_ROWS - 2; row++) {
        for (int col = 2; col < LEVEL_COLS - 2; col++) {
            if (data->tiles[row][col] != 0) continue;
            if (rng_range(rng, 0, 99) >= density) continue;

            /* Don't fill if it would create a corridor < CHAR_W wide */
            /* Check: would both left AND right neighbors be solid? */
            if (data->tiles[row][col - 1] == 1 || data->tiles[row][col + 1] == 1)
                continue;

            data->tiles[row][col] = 1;
        }
    }
}

/* ── Internal: find a valid spawn position ──────────────────────────── */
static int find_spawn(const LevelData *data, int *outCol, int *outRow) {
    /* Scan bottom-up for an air tile with solid below (within 3 tiles) */
    for (int row = LEVEL_ROWS - 3; row >= CHAR_H; row--) {
        for (int col = 1; col < LEVEL_COLS - CHAR_W; col++) {
            if (data->tiles[row][col] != 0) continue;

            /* Check clearance: CHAR_W × CHAR_H block of air above */
            int fits = 1;
            for (int dy = 0; dy < CHAR_H && fits; dy++) {
                for (int dx = 0; dx < CHAR_W && fits; dx++) {
                    if (data->tiles[row - dy][col + dx] != 0)
                        fits = 0;
                }
            }
            if (!fits) continue;

            /* Check ground within 3 tiles below */
            int hasGround = 0;
            for (int below = 1; below <= 3; below++) {
                if (row + below >= LEVEL_ROWS) break;
                if (data->tiles[row + below][col] == 1) {
                    hasGround = 1;
                    break;
                }
            }
            if (hasGround) {
                *outCol = col;
                *outRow = row;
                return 1;
            }
        }
    }
    return 0;
}

/* ── Public API ─────────────────────────────────────────────────────── */

int levelgen_generate(const LevelGenParams *params, LevelData *out) {
    if (!params || !out) return 0;

    /* Try up to 10 seeds if generation doesn't produce a valid level */
    unsigned int seed = params->seed ? params->seed : 42u;

    for (int attempt = 0; attempt < 10; attempt++) {
        Rng rng;
        rng_init(&rng, seed + (unsigned int)attempt);

        /* Clear and set metadata */
        memset(out, 0, sizeof(*out));
        snprintf(out->name, LEVELFILE_MAX_NAME, "Generated_%u", seed + (unsigned int)attempt);
        snprintf(out->author, LEVELFILE_MAX_AUTHOR, "levelgen");

        /* Step 1: Fill borders solid, interior air */
        for (int row = 0; row < LEVEL_ROWS; row++) {
            for (int col = 0; col < LEVEL_COLS; col++) {
                if (row == 0 || row == LEVEL_ROWS - 1 ||
                    col == 0 || col == LEVEL_COLS - 1) {
                    out->tiles[row][col] = 1;
                } else {
                    out->tiles[row][col] = 0;
                }
            }
        }

        /* Step 2: Add a ground floor (with some gaps) */
        for (int col = 1; col < LEVEL_COLS - 1; col++) {
            out->tiles[LEVEL_ROWS - 2][col] = 1;
        }
        /* Cut 1-2 gaps in the floor for pits */
        int gapCount = rng_range(&rng, 1, 2);
        for (int g = 0; g < gapCount; g++) {
            int gapCol = rng_range(&rng, 4, LEVEL_COLS - 6);
            int gapLen = rng_range(&rng, 2, 4);
            for (int dx = 0; dx < gapLen && gapCol + dx < LEVEL_COLS - 1; dx++) {
                out->tiles[LEVEL_ROWS - 2][gapCol + dx] = 0;
            }
        }

        /* Step 3: Place platforms */
        place_platforms(&rng, out, params->platformCount);

        /* Step 4: Place vertical walls */
        place_vertical_walls(&rng, out, params->verticalWalls);

        /* Step 5: Scatter additional solids based on density */
        if (params->density > 0) {
            scatter_fill(&rng, out, params->density / 5);
        }

        /* Step 6: Find valid spawn */
        if (!find_spawn(out, &out->spawn_col, &out->spawn_row))
            continue;

        /* Step 7: Validate */
        if (levelfile_validate(out) == LEVEL_OK)
            return 1;
    }

    return 0; /* All attempts failed */
}

int levelgen_generate_seeded(unsigned int seed, LevelData *out) {
    LevelGenParams params = LEVELGEN_DEFAULT_PARAMS;
    params.seed = seed;
    return levelgen_generate(&params, out);
}
