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
    float x = 38.0f * TILE_SIZE - RENDER_W + 5;
    float vx = 6.0f;
    float y = 28.0f * TILE_SIZE;

    int hit = physics_collide_horizontal(&x, &vx, y);
    ASSERT_EQ_INT(hit, 1);
    ASSERT_EQ_FLOAT(vx, 0.0f, 0.001f);
    TEST_PASS();
}

static void test_horizontal_collision_thin_platform_passable(void) {
    TEST_BEGIN("physics: thin platforms don't block horizontal movement");
    float x = 32.0f * TILE_SIZE;
    float vx = 4.0f;
    float y = 8.0f * TILE_SIZE;

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
    test_horizontal_collision_thin_platform_passable();
    test_ceiling_collision();
}
