/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * test_level.c — Unit tests for level / tile query functions
 */
#include "test_framework.h"
#include "test_helpers.h"
#include "types.h"
#include "level.h"

static void test_tile_solid_boundaries(void) {
    TEST_BEGIN("level: out-of-bounds returns solid");
    ASSERT_EQ_INT(tile_solid(-1, 100), 1);
    ASSERT_EQ_INT(tile_solid(SCREEN_W + 10, 100), 1);
    ASSERT_EQ_INT(tile_solid(100, -1), 1);
    ASSERT_EQ_INT(tile_solid(100, SCREEN_H + 10), 1);
    TEST_PASS();
}

static void test_tile_solid_known_tiles(void) {
    TEST_BEGIN("level: known solid and air tiles");
    ASSERT_EQ_INT(tile_solid(5 * TILE_SIZE, 29 * TILE_SIZE), 1);
    ASSERT_EQ_INT(tile_solid(20 * TILE_SIZE, 29 * TILE_SIZE), 1);
    ASSERT_EQ_INT(tile_solid(10 * TILE_SIZE, 1 * TILE_SIZE), 0);
    ASSERT_EQ_INT(tile_solid(20 * TILE_SIZE, 1 * TILE_SIZE), 0);
    TEST_PASS();
}

static void test_tile_wall_vs_platform(void) {
    TEST_BEGIN("level: tile_wall distinguishes walls from thin platforms");
    ASSERT_EQ_INT(tile_wall(37 * TILE_SIZE, 5 * TILE_SIZE), 1);
    ASSERT_EQ_INT(tile_wall(0 * TILE_SIZE + 5, 15 * TILE_SIZE), 1);
    ASSERT_EQ_INT(tile_solid(34 * TILE_SIZE, 7 * TILE_SIZE), 1);
    ASSERT_EQ_INT(tile_wall(34 * TILE_SIZE, 7 * TILE_SIZE), 0);
    ASSERT_EQ_INT(tile_solid(15 * TILE_SIZE, 10 * TILE_SIZE), 1);
    ASSERT_EQ_INT(tile_wall(15 * TILE_SIZE, 10 * TILE_SIZE), 0);
    TEST_PASS();
}

static void test_tile_wall_beside(void) {
    TEST_BEGIN("level: tile_wall_beside detects real walls");
    float xNearWall = 2.0f * TILE_SIZE;
    float yMid = 15.0f * TILE_SIZE;
    ASSERT_EQ_INT(tile_wall_beside(xNearWall, yMid, -1), 1);

    float xOpen = 15.0f * TILE_SIZE;
    float yOpen = 15.0f * TILE_SIZE;
    ASSERT_EQ_INT(tile_wall_beside(xOpen, yOpen, 1), 0);
    TEST_PASS();
}

static void test_head_ledge_detection(void) {
    TEST_BEGIN("level: tile_head_ledge detects platform at head height");
    float grabY;
    float x = 33.0f * TILE_SIZE - RENDER_W - 3;
    float y = 26.0f * TILE_SIZE - RENDER_H;

    int headY = (int)y + RENDER_H / 4;
    int headRow = headY / TILE_SIZE;
    int result = tile_head_ledge(x, y, 1, &grabY);
    (void)result;
    (void)headRow;
    TEST_PASS();
}

void run_level_tests(void) {
    printf("[Level]\n");
    test_tile_solid_boundaries();
    test_tile_solid_known_tiles();
    test_tile_wall_vs_platform();
    test_tile_wall_beside();
    test_head_ledge_detection();
}
