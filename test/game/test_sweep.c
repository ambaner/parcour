/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * test_sweep.c — Position sweep / property-based tests
 */
#include "test_framework.h"
#include "test_helpers.h"
#include "types.h"
#include "character.h"
#include "level.h"

#include <stdio.h>

static void test_sweep_freefall_always_lands(void) {
    TEST_BEGIN("sweep: free-fall from every column must land");
    int failures = 0;
    for (int col = 2; col < 38; col++) {
        Character p;
        character_init(&p, (float)(col * TILE_SIZE), 0.0f);
        p.state = STATE_JUMP_FALL;
        p.onGround = 0;
        p.vy = 0.0f;

        simulate(&p, 300, 0, -1);
        if (!p.onGround) {
            printf("    col %d: did not land (y=%.1f state=%d)\n",
                   col, p.y, p.state);
            failures++;
        }
    }
    ASSERT_EQ_INT(failures, 0);
    TEST_PASS();
}

static void test_sweep_walk_right_never_stuck(void) {
    TEST_BEGIN("sweep: walking right from every row must not get stuck");
    int rows[] = { 29, 26, 23, 17, 14, 10, 7, 3 };
    int numRows = sizeof(rows) / sizeof(rows[0]);
    int failures = 0;

    for (int r = 0; r < numRows; r++) {
        int row = rows[r];
        for (int col = 2; col < 38; col++) {
            if (!tile_solid(col * TILE_SIZE + TILE_SIZE / 2,
                            row * TILE_SIZE + 1))
                continue;
            Character p;
            float startX = (float)(col * TILE_SIZE);
            float startY = (float)(row * TILE_SIZE) - RENDER_H;
            character_init(&p, startX, startY);
            p.onGround = 1;

            simulate(&p, 120, 1, -1);

            if (p.x <= startX + 1.0f && p.y <= startY + 1.0f) {
                printf("    row=%d col=%d: stuck at (%.1f,%.1f)\n",
                       row, col, p.x, p.y);
                failures++;
            }
            break;
        }
    }
    ASSERT_EQ_INT(failures, 0);
    TEST_PASS();
}

static void test_sweep_invariants_during_freefall(void) {
    TEST_BEGIN("sweep: invariants hold during free-fall from every column");
    g_invariantFails = 0;
    for (int col = 3; col < 37; col++) {
        Character p;
        character_init(&p, (float)(col * TILE_SIZE), 0.0f);
        p.state = STATE_JUMP_FALL;
        p.onGround = 0;
        p.vy = 0.0f;

        simulate_checked(&p, 300, 0, -1);
    }
    ASSERT_EQ_INT(g_invariantFails, 0);
    TEST_PASS();
}

void run_sweep_tests(void) {
    printf("[Sweep]\n");
    test_sweep_freefall_always_lands();
    test_sweep_walk_right_never_stuck();
    test_sweep_invariants_during_freefall();
}
