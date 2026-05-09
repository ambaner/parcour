/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * test_levelgen.c — Tests for the random level generator
 *
 * Strategy: generate many levels with different seeds and verify that:
 *   1. Every generated level passes validation
 *   2. Every generated level has valid spawn placement
 *   3. Clearance warnings are reasonable (not 100% of air tiles)
 *   4. Same seed always produces identical output (determinism)
 *   5. Different seeds produce different levels (variety)
 */

#include "test_framework.h"
#include "levelgen.h"
#include "levelfile.h"
#include <string.h>
#include <stdio.h>

/* ── Test: basic generation succeeds ────────────────────────────────── */
static void test_basic_generation(void) {
    TEST_BEGIN("levelgen: basic generation with default params succeeds");

    LevelData data;
    int ok = levelgen_generate_seeded(1, &data);
    ASSERT_EQ_INT(1, ok);

    /* Must pass validation */
    LevelError err = levelfile_validate(&data);
    ASSERT_EQ_INT(LEVEL_OK, err);
    TEST_PASS();
}

/* ── Test: determinism (same seed = same level) ─────────────────────── */
static void test_determinism(void) {
    TEST_BEGIN("levelgen: same seed produces identical level");

    LevelData a, b;
    levelgen_generate_seeded(42, &a);
    levelgen_generate_seeded(42, &b);

    ASSERT_EQ_INT(a.spawn_col, b.spawn_col);
    ASSERT_EQ_INT(a.spawn_row, b.spawn_row);
    ASSERT_TRUE(memcmp(a.tiles, b.tiles, sizeof(a.tiles)) == 0);
    TEST_PASS();
}

/* ── Test: different seeds produce different levels ──────────────────── */
static void test_variety(void) {
    TEST_BEGIN("levelgen: different seeds produce different levels");

    LevelData a, b;
    levelgen_generate_seeded(100, &a);
    levelgen_generate_seeded(200, &b);

    /* At least the tiles should differ */
    ASSERT_TRUE(memcmp(a.tiles, b.tiles, sizeof(a.tiles)) != 0);
    TEST_PASS();
}

/* ── Test: batch validation (generate 50 levels, all must be valid) ── */
static void test_batch_validation(void) {
    TEST_BEGIN("levelgen: 50 random levels all pass validation");

    int failures = 0;
    for (unsigned int seed = 1; seed <= 50; seed++) {
        LevelData data;
        int ok = levelgen_generate_seeded(seed, &data);
        if (!ok) { failures++; continue; }

        LevelError err = levelfile_validate(&data);
        if (err != LEVEL_OK) {
            printf("    seed %u failed validation: %s\n",
                   seed, levelfile_error_string(err));
            failures++;
        }
    }
    ASSERT_EQ_INT(0, failures);
    TEST_PASS();
}

/* ── Test: spawn is always in a valid position ──────────────────────── */
static void test_spawn_validity(void) {
    TEST_BEGIN("levelgen: spawn is always on air with ground below");

    int failures = 0;
    for (unsigned int seed = 1; seed <= 30; seed++) {
        LevelData data;
        if (!levelgen_generate_seeded(seed, &data)) { failures++; continue; }

        /* Spawn tile must be air */
        if (data.tiles[data.spawn_row][data.spawn_col] != 0) {
            printf("    seed %u: spawn at (%d,%d) is solid\n",
                   seed, data.spawn_col, data.spawn_row);
            failures++;
            continue;
        }

        /* Must have ground within 3 tiles below */
        int hasGround = 0;
        for (int below = 1; below <= 3; below++) {
            int r = data.spawn_row + below;
            if (r < LEVEL_ROWS && data.tiles[r][data.spawn_col] == 1) {
                hasGround = 1;
                break;
            }
        }
        if (!hasGround) {
            printf("    seed %u: no ground below spawn (%d,%d)\n",
                   seed, data.spawn_col, data.spawn_row);
            failures++;
        }
    }
    ASSERT_EQ_INT(0, failures);
    TEST_PASS();
}

/* ── Test: clearance is reasonable ──────────────────────────────────── */
static void test_clearance_reasonable(void) {
    TEST_BEGIN("levelgen: clearance warnings under 50%% of air tiles");

    int failures = 0;
    for (unsigned int seed = 1; seed <= 20; seed++) {
        LevelData data;
        if (!levelgen_generate_seeded(seed, &data)) continue;

        int warnings[LEVEL_ROWS][LEVEL_COLS];
        int warnCount = levelfile_check_clearance(&data, warnings);

        /* Count air tiles */
        int airCount = 0;
        for (int r = 0; r < LEVEL_ROWS; r++)
            for (int c = 0; c < LEVEL_COLS; c++)
                if (data.tiles[r][c] == 0) airCount++;

        /* Warnings shouldn't exceed 50% of air — that would mean
         * most of the level is unusable */
        if (airCount > 0 && warnCount > airCount / 2) {
            printf("    seed %u: %d warnings / %d air (%.0f%%)\n",
                   seed, warnCount, airCount,
                   100.0 * warnCount / airCount);
            failures++;
        }
    }
    ASSERT_EQ_INT(0, failures);
    TEST_PASS();
}

/* ── Test: borders are always solid ─────────────────────────────────── */
static void test_borders_solid(void) {
    TEST_BEGIN("levelgen: all borders solid in generated levels");

    int failures = 0;
    for (unsigned int seed = 1; seed <= 30; seed++) {
        LevelData data;
        if (!levelgen_generate_seeded(seed, &data)) { failures++; continue; }

        for (int col = 0; col < LEVEL_COLS; col++) {
            if (data.tiles[0][col] != 1 || data.tiles[LEVEL_ROWS - 1][col] != 1) {
                failures++;
                break;
            }
        }
        for (int row = 0; row < LEVEL_ROWS; row++) {
            if (data.tiles[row][0] != 1 || data.tiles[row][LEVEL_COLS - 1] != 1) {
                failures++;
                break;
            }
        }
    }
    ASSERT_EQ_INT(0, failures);
    TEST_PASS();
}

/* ── Test: custom params (high density) ─────────────────────────────── */
static void test_high_density(void) {
    TEST_BEGIN("levelgen: high density levels still valid");

    LevelGenParams params = { 999, 80, 12, 6 };
    LevelData data;
    int ok = levelgen_generate(&params, &data);
    ASSERT_EQ_INT(1, ok);
    ASSERT_EQ_INT(LEVEL_OK, levelfile_validate(&data));
    TEST_PASS();
}

/* ── Test: save/load roundtrip of generated level ───────────────────── */
static void test_roundtrip(void) {
    TEST_BEGIN("levelgen: generated level survives save/load roundtrip");

    LevelData original;
    levelgen_generate_seeded(777, &original);

    const char *path = "test_gen_roundtrip.parcour";
    LevelError err = levelfile_save(path, &original);
    ASSERT_EQ_INT(LEVEL_OK, err);

    LevelData loaded;
    err = levelfile_load(path, &loaded);
    ASSERT_EQ_INT(LEVEL_OK, err);

    ASSERT_EQ_INT(original.spawn_col, loaded.spawn_col);
    ASSERT_EQ_INT(original.spawn_row, loaded.spawn_row);
    ASSERT_TRUE(memcmp(original.tiles, loaded.tiles, sizeof(original.tiles)) == 0);

    remove(path);
    TEST_PASS();
}

/* ── Suite runner ───────────────────────────────────────────────────── */

void run_levelgen_tests(void) {
    printf("[LevelGen]\n");
    test_basic_generation();
    test_determinism();
    test_variety();
    test_batch_validation();
    test_spawn_validity();
    test_clearance_reasonable();
    test_borders_solid();
    test_high_density();
    test_roundtrip();
}
