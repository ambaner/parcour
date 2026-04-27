/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * test_regression.c — Regression tests for previously fixed bugs
 */
#include "test_framework.h"
#include "test_helpers.h"
#include "types.h"
#include "character.h"
#include "input.h"
#include "level.h"

static void test_regression_edge_stuck(void) {
    TEST_BEGIN("regression: character must not freeze at platform edge");
    Character p;
    character_init(&p, 7.0f * TILE_SIZE - RENDER_W / 2,
                   26.0f * TILE_SIZE - RENDER_H);
    p.onGround = 1;

    float startX = p.x;
    g_invariantFails = 0;
    simulate_checked(&p, 60, 1, -1);

    ASSERT_TRUE(p.x > startX + 5.0f || p.y > 26.0f * TILE_SIZE - RENDER_H + 5.0f);
    ASSERT_EQ_INT(g_invariantFails, 0);
    TEST_PASS();
}

static void test_regression_head_platform_no_horizontal_block(void) {
    TEST_BEGIN("regression: overhead platform must not block horizontal movement");
    Character p;
    character_init(&p, 30.0f * TILE_SIZE, 26.0f * TILE_SIZE - RENDER_H);
    p.onGround = 1;

    float startX = p.x;
    g_invariantFails = 0;
    simulate_checked(&p, 80, 1, -1);

    ASSERT_TRUE(p.x > startX + 100.0f);
    ASSERT_EQ_INT(g_invariantFails, 0);
    TEST_PASS();
}

static void test_regression_grab_cooldown_prevents_oscillation(void) {
    TEST_BEGIN("regression: grab cooldown prevents re-grab after release");
    Character p;
    character_init(&p, 37.0f * TILE_SIZE, 5.0f * TILE_SIZE);
    p.onGround = 1;
    p.vy = 0;
    p.vx = 0;
    p.facing = 1;
    p.state = STATE_CORNER_GRAB;
    p.stateTimer = 5;
    p.grabCooldown = 0;

    input_clear();
    keyDown = 1;
    keyDownJustPressed = 1;
    character_update(&p);
    ASSERT_TRUE(p.grabCooldown >= 15);

    CharState history[30];
    int grabCount = 0;
    for (int i = 0; i < 30; i++) {
        input_clear();
        input_set_dir(1);
        character_update(&p);
        history[i] = p.state;
        if (p.state == STATE_CORNER_GRAB) grabCount++;
    }
    ASSERT_TRUE(grabCount <= 1);
    ASSERT_TRUE(!detect_oscillation(history, 30));
    TEST_PASS();
}

static void test_regression_auto_climb_sequence(void) {
    TEST_BEGIN("regression: auto-climb shows CORNER_GRAB before CORNER_CLIMB");
    Character p;
    character_init(&p, 32.0f * TILE_SIZE, 23.0f * TILE_SIZE - RENDER_H / 4);
    p.onGround = 1;
    p.vy = 0;
    p.vx = 0;
    p.facing = 1;
    p.state = STATE_CORNER_GRAB;
    p.stateTimer = 0;
    p.grabCooldown = 0;

    int sawGrab = 0, sawClimb = 0, grabBeforeClimb = 0;
    for (int i = 0; i < 40; i++) {
        input_clear();
        input_set_dir(1);
        character_update(&p);
        if (p.state == STATE_CORNER_GRAB) sawGrab = 1;
        if (p.state == STATE_CORNER_CLIMB) {
            sawClimb = 1;
            if (sawGrab) grabBeforeClimb = 1;
        }
    }
    ASSERT_TRUE(sawGrab);
    ASSERT_TRUE(sawClimb);
    ASSERT_TRUE(grabBeforeClimb);
    TEST_PASS();
}

static void test_regression_corner_climb_no_wall_trap(void) {
    TEST_BEGIN("regression: corner-climb must not trap character inside boundary wall");

    /* Reproduce the bug: climb the right-side ledge chain (cols 33-37)
     * three times.  The third climb's teleport overshoots into the air
     * gap at col 38; the old nudge loop pushed into col 39 (boundary
     * wall), permanently trapping the character. */
    Character p;
    /* Start near the bottom-right ledge (row 23, cols 33-37), facing right */
    character_init(&p, 31.0f * TILE_SIZE, 23.0f * TILE_SIZE - RENDER_H / 4);
    p.onGround = 1;
    p.facing = 1;

    /* Perform multiple corner-grab → corner-climb cycles */
    int climbs = 0;
    for (int frame = 0; frame < 600; frame++) {
        input_clear();
        input_set_dir(1);  /* hold right to auto-climb */

        /* Press UP periodically to trigger climb-ups faster */
        if (p.state == STATE_CORNER_GRAB && p.stateTimer >= 2) {
            keyUp = 1;
            keyUpJustPressed = 1;
        }

        character_update(&p);
        check_invariants(&p, frame);

        if (p.state == STATE_CORNER_CLIMB)
            climbs++;
    }

    /* After climbing, verify movement is possible */
    float posBeforeMove = p.x;
    simulate_checked(&p, 60, -1, -1);  /* walk left for 60 frames */

    ASSERT_TRUE(climbs > 0);
    /* Character must have moved — if trapped, x stays fixed */
    ASSERT_TRUE(p.x < posBeforeMove - 1.0f ||
                p.state == STATE_JUMP_FALL);  /* or fell off, also fine */
    ASSERT_EQ_INT(g_invariantFails, 0);
    TEST_PASS();
}

void run_regression_tests(void) {
    printf("[Regression]\n");
    test_regression_edge_stuck();
    test_regression_head_platform_no_horizontal_block();
    test_regression_grab_cooldown_prevents_oscillation();
    test_regression_auto_climb_sequence();
    test_regression_corner_climb_no_wall_trap();
}
