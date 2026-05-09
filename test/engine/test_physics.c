/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * test_physics.c — Unit tests for collision / physics functions
 */
#include "test_framework.h"
#include "test_helpers.h"
#include "types.h"
#include "physics.h"
#include "level.h"

static void test_vertical_collision_landing(void) {
    TEST_BEGIN("physics: vertical collision lands on floor");
    float y = 29.0f * TILE_SIZE - RENDER_H - 5.0f;
    float vy = 8.0f;
    float x = 15.0f * TILE_SIZE;
    int onGround = 0;
    float landingVy = 0;

    physics_collide_vertical(&y, &vy, x, &onGround, &landingVy);
    ASSERT_EQ_INT(onGround, 1);
    ASSERT_EQ_FLOAT(vy, 0.0f, 0.001f);
    ASSERT_TRUE(landingVy > 0.0f);
    TEST_PASS();
}

static void test_vertical_collision_center_foot(void) {
    TEST_BEGIN("physics: landing requires center foot on solid");
    float x = 7.5f * TILE_SIZE;
    float y = 26.0f * TILE_SIZE - RENDER_H - 2.0f;
    float vy = 3.0f;
    int onGround = 0;
    float landingVy = 0;

    physics_collide_vertical(&y, &vy, x, &onGround, &landingVy);
    ASSERT_EQ_INT(onGround, 0);
    TEST_PASS();
}

static void test_horizontal_collision_wall_blocks(void) {
    TEST_BEGIN("physics: horizontal collision blocks at walls");
    /* Character at y=20*TILE_SIZE has head at row 20, feet at row 23.
     * All those rows at col 38 are air; col 39 is the boundary wall.
     * Moving right should be blocked when right edge enters col 39. */
    float x = 1190.0f;
    float vx = 6.0f;
    float y = 20.0f * TILE_SIZE;

    int hit = physics_collide_horizontal(&x, &vx, y);
    ASSERT_EQ_INT(hit, 1);
    ASSERT_EQ_FLOAT(vx, 0.0f, 0.001f);
    TEST_PASS();
}

static void test_horizontal_collision_thin_platform_at_foot_blocks(void) {
    TEST_BEGIN("physics: solid tiles at foot level block horizontal movement");
    /* Character at y=8*TILE_SIZE has feet at row 11.  Default level
     * has a thin platform at row 11 cols 33-37.  Moving right from
     * col 31 should be blocked when right edge enters col 33. */
    float x = 31.0f * TILE_SIZE;
    float vx = 10.0f;
    float y = 8.0f * TILE_SIZE;

    int hit = physics_collide_horizontal(&x, &vx, y);
    ASSERT_EQ_INT(hit, 1);
    ASSERT_EQ_FLOAT(vx, 0.0f, 0.001f);
    TEST_PASS();
}

static void test_horizontal_collision_open_air_passable(void) {
    TEST_BEGIN("physics: movement in open air is not blocked");
    /* Character at y=5*TILE_SIZE: head at row 5, feet at row 8.
     * All tiles in that region around col 15 are air in the default
     * level, so horizontal movement should proceed freely. */
    float x = 15.0f * TILE_SIZE;
    float vx = 4.0f;
    float y = 5.0f * TILE_SIZE;

    int hit = physics_collide_horizontal(&x, &vx, y);
    ASSERT_EQ_INT(hit, 0);
    ASSERT_TRUE(vx > 0.0f);
    TEST_PASS();
}

static void test_ceiling_collision(void) {
    TEST_BEGIN("physics: ceiling collision stops upward movement");
    float y = 1.0f * TILE_SIZE;
    float vy = -10.0f;
    float x = 15.0f * TILE_SIZE;

    int hit = physics_collide_ceiling(&y, &vy, x);
    ASSERT_EQ_INT(hit, 1);
    ASSERT_EQ_FLOAT(vy, 0.0f, 0.001f);
    TEST_PASS();
}

void run_physics_tests(void) {
    printf("[Physics]\n");
    test_vertical_collision_landing();
    test_vertical_collision_center_foot();
    test_horizontal_collision_wall_blocks();
    test_horizontal_collision_thin_platform_at_foot_blocks();
    test_horizontal_collision_open_air_passable();
    test_ceiling_collision();
}
