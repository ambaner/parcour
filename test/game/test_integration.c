/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * test_integration.c — End-to-end integration tests (multi-phase action cycles)
 */
#include "test_framework.h"
#include "test_helpers.h"
#include "types.h"
#include "character.h"

static void test_full_run_jump_land_cycle(void) {
    TEST_BEGIN("integration: run \xe2\x86\x92 jump \xe2\x86\x92 land cycle completes cleanly");
    Character p;
    character_init(&p, 15.0f * TILE_SIZE, 29.0f * TILE_SIZE - RENDER_H);
    p.onGround = 1;

    simulate(&p, 40, 1, -1);
    ASSERT_EQ_INT(p.state, STATE_RUNNING);
    float preJumpX = p.x;

    simulate(&p, 1, 1, 0);
    ASSERT_TRUE(p.state == STATE_CROUCH || p.state == STATE_SOMERSAULT);

    simulate(&p, 60, 1, -1);
    ASSERT_EQ_INT(p.onGround, 1);
    ASSERT_TRUE(p.x > preJumpX);
    TEST_PASS();
}

static void test_full_walk_stop_idle(void) {
    TEST_BEGIN("integration: walk \xe2\x86\x92 release \xe2\x86\x92 friction stop \xe2\x86\x92 idle");
    Character p;
    character_init(&p, 15.0f * TILE_SIZE, 29.0f * TILE_SIZE - RENDER_H);
    p.onGround = 1;

    simulate(&p, 15, 1, -1);
    simulate(&p, 60, 0, -1);
    ASSERT_EQ_INT(p.state, STATE_IDLE);
    ASSERT_EQ_FLOAT(p.vx, 0.0f, 0.05f);
    TEST_PASS();
}

static void test_full_direction_reversal(void) {
    TEST_BEGIN("integration: direction reversal via TURNING state");
    Character p;
    character_init(&p, 15.0f * TILE_SIZE, 29.0f * TILE_SIZE - RENDER_H);
    p.onGround = 1;

    simulate(&p, 10, 1, -1);
    ASSERT_EQ_INT(p.facing, 1);
    simulate(&p, 20, -1, -1);
    ASSERT_EQ_INT(p.facing, -1);
    TEST_PASS();
}

void run_integration_tests(void) {
    printf("[Integration]\n");
    test_full_run_jump_land_cycle();
    test_full_walk_stop_idle();
    test_full_direction_reversal();
}
