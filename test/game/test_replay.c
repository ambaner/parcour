/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * test_replay.c — Deterministic input replay tests
 */
#include "test_framework.h"
#include "test_helpers.h"
#include "types.h"
#include "character.h"

static void test_replay_run_jump_land(void) {
    TEST_BEGIN("replay: run \xe2\x86\x92 jump \xe2\x86\x92 land reproduces known outcome");
    Character p;
    character_init(&p, 15.0f * TILE_SIZE, 29.0f * TILE_SIZE - RENDER_H);
    p.onGround = 1;
    g_invariantFails = 0;

    InputFrame inputs[200];
    int ic = 0;
    REPEAT_INPUT(1, 0, 0, 40);
    REPEAT_INPUT(1, 1, 0, 1);
    REPEAT_INPUT(1, 0, 0, 5);
    REPEAT_INPUT(0, 0, 0, 60);

    replay(&p, inputs, ic);

    ASSERT_EQ_INT(p.onGround, 1);
    ASSERT_TRUE(p.x > 15.0f * TILE_SIZE + 100.0f);
    ASSERT_EQ_INT(g_invariantFails, 0);
    TEST_PASS();
}

static void test_replay_walk_turn_walk(void) {
    TEST_BEGIN("replay: walk right \xe2\x86\x92 turn \xe2\x86\x92 walk left reproduces symmetry");
    Character p;
    character_init(&p, 20.0f * TILE_SIZE, 29.0f * TILE_SIZE - RENDER_H);
    p.onGround = 1;
    g_invariantFails = 0;

    InputFrame inputs[200];
    int ic = 0;
    REPEAT_INPUT(1, 0, 0, 25);
    REPEAT_INPUT(-1, 0, 0, 25);
    REPEAT_INPUT(0, 0, 0, 30);

    float startX = p.x;
    replay(&p, inputs, ic);

    float drift = p.x - startX;
    ASSERT_TRUE(drift > -100.0f && drift < 100.0f);
    ASSERT_EQ_INT(p.state, STATE_IDLE);
    ASSERT_EQ_INT(g_invariantFails, 0);
    TEST_PASS();
}

static void test_replay_crouch_jump_sequence(void) {
    TEST_BEGIN("replay: crouch \xe2\x86\x92 jump \xe2\x86\x92 land is clean");
    Character p;
    character_init(&p, 20.0f * TILE_SIZE, 29.0f * TILE_SIZE - RENDER_H);
    p.onGround = 1;
    g_invariantFails = 0;

    InputFrame inputs[150];
    int ic = 0;
    REPEAT_INPUT(0, 1, 0, 1);
    REPEAT_INPUT(0, 1, 0, 5);
    REPEAT_INPUT(0, 0, 0, 60);
    REPEAT_INPUT(0, 0, 0, 20);

    replay(&p, inputs, ic);

    ASSERT_EQ_INT(p.onGround, 1);
    ASSERT_EQ_INT(p.state, STATE_IDLE);
    ASSERT_EQ_INT(g_invariantFails, 0);
    TEST_PASS();
}

void run_replay_tests(void) {
    printf("[Replay]\n");
    test_replay_run_jump_land();
    test_replay_walk_turn_walk();
    test_replay_crouch_jump_sequence();
}
