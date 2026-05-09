/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * test_state_transitions.c — Tests for specific FSM state transitions
 */
#include "test_framework.h"
#include "test_helpers.h"
#include "types.h"
#include "character.h"
#include "input.h"

static void test_character_turning_flips_facing(void) {
    TEST_BEGIN("character: TURNING state flips facing direction");
    Character p;
    character_init(&p, 15.0f * TILE_SIZE, 29.0f * TILE_SIZE - RENDER_H);
    p.onGround = 1;

    simulate(&p, 10, 1, -1);
    ASSERT_EQ_INT(p.facing, 1);

    simulate(&p, 1, -1, -1);
    ASSERT_TRUE(p.state == STATE_TURNING || p.state == STATE_WALK);

    simulate(&p, 10, -1, -1);
    ASSERT_EQ_INT(p.facing, -1);
    TEST_PASS();
}

static void test_character_slide_from_running(void) {
    TEST_BEGIN("character: SLIDE triggers from RUNNING + down press");
    Character p;
    character_init(&p, 10.0f * TILE_SIZE, 29.0f * TILE_SIZE - RENDER_H);
    p.onGround = 1;

    simulate(&p, 40, 1, -1);
    ASSERT_EQ_INT(p.state, STATE_RUNNING);

    input_clear();
    keyRight = 1;
    keyDown = 1;
    keyDownJustPressed = 1;
    character_update(&p);
    ASSERT_EQ_INT(p.state, STATE_SLIDE);

    simulate(&p, 30, 0, -1);
    ASSERT_EQ_INT(p.state, STATE_IDLE);
    TEST_PASS();
}

static void test_character_somersault_from_running_jump(void) {
    TEST_BEGIN("character: running jump produces SOMERSAULT");
    Character p;
    character_init(&p, 10.0f * TILE_SIZE, 29.0f * TILE_SIZE - RENDER_H);
    p.onGround = 1;

    simulate(&p, 40, 1, -1);
    ASSERT_EQ_INT(p.state, STATE_RUNNING);

    input_clear();
    keyRight = 1;
    keyUp = 1;
    keyUpJustPressed = 1;
    character_update(&p);
    ASSERT_EQ_INT(p.state, STATE_SOMERSAULT);
    ASSERT_TRUE(p.vy < 0);
    ASSERT_EQ_INT(p.onGround, 0);
    TEST_PASS();
}

static void test_character_crouch_stays_on_down(void) {
    TEST_BEGIN("character: down press from IDLE \xe2\x86\x92 CROUCH, stays crouched while held");
    Character p;
    character_init(&p, 15.0f * TILE_SIZE, 29.0f * TILE_SIZE - RENDER_H);
    p.onGround = 1;

    input_clear();
    keyDown = 1;
    keyDownJustPressed = 1;
    character_update(&p);
    ASSERT_EQ_INT(p.state, STATE_CROUCH);

    for (int i = 0; i < 10; i++) {
        input_clear();
        keyDown = 1;
        character_update(&p);
    }
    ASSERT_EQ_INT(p.state, STATE_CROUCH);

    simulate(&p, 10, 0, -1);
    ASSERT_EQ_INT(p.state, STATE_IDLE);
    TEST_PASS();
}

static void test_character_hard_landing_locks_input(void) {
    TEST_BEGIN("character: HARD_LANDING locks out input for 18 frames");
    Character p;
    character_init(&p, 15.0f * TILE_SIZE, 29.0f * TILE_SIZE - RENDER_H);
    p.onGround = 1;
    p.state = STATE_HARD_LANDING;
    p.stateTimer = 0;
    p.vx = 3.0f;

    for (int i = 0; i < 15; i++) {
        input_clear();
        keyRight = 1;
        character_update(&p);
        ASSERT_TRUE(p.state == STATE_HARD_LANDING || p.state == STATE_CROUCH);
    }

    simulate(&p, 15, 0, -1);
    ASSERT_TRUE(p.state == STATE_CROUCH || p.state == STATE_IDLE);
    TEST_PASS();
}

static void test_character_wall_slide_detach(void) {
    TEST_BEGIN("character: releasing direction during WALL_SLIDE \xe2\x86\x92 JUMP_FALL");
    Character p;
    character_init(&p, 37.5f * TILE_SIZE, 10.0f * TILE_SIZE);
    p.onGround = 0;
    p.vy = 5.0f;
    p.facing = 1;
    p.state = STATE_WALL_SLIDE;

    simulate(&p, 3, 0, -1);
    ASSERT_EQ_INT(p.state, STATE_JUMP_FALL);
    TEST_PASS();
}

static void test_character_wall_jump(void) {
    TEST_BEGIN("character: pressing Up during WALL_SLIDE \xe2\x86\x92 wall-jump");
    Character p;
    /* Position character wall-sliding against left border (col 0/1).
     * Use x just right of the left wall: bodyL = x+8 must clear col 1.
     * Col 1 is solid (border), so x must have bodyL at col 2 boundary.
     * x = 2*TILE_SIZE - 8 = 56.  After wall-jump vx>0, bodyR stays in air.
     * y=16*TILE_SIZE: rows 16-19 at cols 2-8 are all air. */
    character_init(&p, 2.0f * TILE_SIZE - 8.0f, 16.0f * TILE_SIZE);
    p.onGround = 0;
    p.vy = 2.0f;
    p.facing = -1;  /* facing left (against left wall) */
    p.state = STATE_WALL_SLIDE;

    input_clear();
    keyLeft = 1;
    keyUp = 1;
    keyUpJustPressed = 1;
    character_update(&p);
    ASSERT_EQ_INT(p.state, STATE_JUMP_RISE);
    ASSERT_EQ_INT(p.facing, 1);  /* flipped from -1 to +1 */
    ASSERT_TRUE(p.vx > 0);       /* pushed away from left wall */
    TEST_PASS();
}

static void test_character_corner_grab_release_down(void) {
    TEST_BEGIN("character: pressing Down during CORNER_GRAB \xe2\x86\x92 releases with cooldown");
    Character p;
    character_init(&p, 37.0f * TILE_SIZE, 5.0f * TILE_SIZE);
    p.onGround = 1;
    p.vy = 0;
    p.vx = 0;
    p.facing = 1;
    p.state = STATE_CORNER_GRAB;
    p.stateTimer = 0;
    p.grabCooldown = 0;

    input_clear();
    keyDown = 1;
    keyDownJustPressed = 1;
    character_update(&p);
    ASSERT_EQ_INT(p.onGround, 0);
    ASSERT_TRUE(p.grabCooldown > 0);
    ASSERT_TRUE(p.state == STATE_JUMP_FALL || p.state == STATE_LANDING);
    TEST_PASS();
}

void run_state_transition_tests(void) {
    printf("[State Transitions]\n");
    test_character_turning_flips_facing();
    test_character_slide_from_running();
    test_character_somersault_from_running_jump();
    test_character_crouch_stays_on_down();
    test_character_hard_landing_locks_input();
    test_character_wall_slide_detach();
    test_character_wall_jump();
    test_character_corner_grab_release_down();
}
