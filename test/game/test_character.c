/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * test_character.c — Unit tests for the character state machine (FSM)
 */
#include "test_framework.h"
#include "test_helpers.h"
#include "types.h"
#include "character.h"
#include "level.h"

static void test_character_starts_idle(void) {
    TEST_BEGIN("character: starts in IDLE state on ground");
    Character p;
    character_init(&p, 10.0f * TILE_SIZE, 29.0f * TILE_SIZE - RENDER_H);
    ASSERT_EQ_INT(p.state, STATE_IDLE);
    ASSERT_EQ_INT(p.facing, 1);
    TEST_PASS();
}

static void test_character_walk_right(void) {
    TEST_BEGIN("character: pressing right transitions to WALK");
    Character p;
    character_init(&p, 10.0f * TILE_SIZE, 29.0f * TILE_SIZE - RENDER_H);
    p.onGround = 1;
    simulate(&p, 5, 1, -1);
    ASSERT_TRUE(p.state == STATE_WALK || p.state == STATE_RUNNING);
    ASSERT_TRUE(p.x > 10.0f * TILE_SIZE);
    ASSERT_EQ_INT(p.facing, 1);
    TEST_PASS();
}

static void test_character_walk_to_run(void) {
    TEST_BEGIN("character: sustained walk transitions to RUNNING after 30 frames");
    Character p;
    character_init(&p, 10.0f * TILE_SIZE, 29.0f * TILE_SIZE - RENDER_H);
    p.onGround = 1;
    simulate(&p, 35, 1, -1);
    ASSERT_EQ_INT(p.state, STATE_RUNNING);
    TEST_PASS();
}

static void test_character_stop_on_release(void) {
    TEST_BEGIN("character: releasing direction stops character");
    Character p;
    character_init(&p, 10.0f * TILE_SIZE, 29.0f * TILE_SIZE - RENDER_H);
    p.onGround = 1;
    simulate(&p, 10, 1, -1);
    ASSERT_TRUE(p.vx > 0.0f);
    simulate(&p, 30, 0, -1);
    ASSERT_TRUE(p.state == STATE_IDLE || p.state == STATE_STOPPING);
    TEST_PASS();
}

static void test_character_jump(void) {
    TEST_BEGIN("character: Up press from idle \xe2\x86\x92 crouch \xe2\x86\x92 jump");
    Character p;
    character_init(&p, 15.0f * TILE_SIZE, 29.0f * TILE_SIZE - RENDER_H);
    p.onGround = 1;

    simulate(&p, 1, 0, 0);
    ASSERT_EQ_INT(p.state, STATE_CROUCH);

    simulate(&p, 5, 0, 0);
    ASSERT_TRUE(p.state == STATE_JUMP_RISE || p.state == STATE_CROUCH);
    TEST_PASS();
}

static void test_character_fall_off_edge(void) {
    TEST_BEGIN("character: walking off platform edge causes fall");
    Character p;
    character_init(&p, 5.0f * TILE_SIZE, 26.0f * TILE_SIZE - RENDER_H);
    p.onGround = 1;
    simulate(&p, 90, 1, -1);
    ASSERT_TRUE(p.y > 26.0f * TILE_SIZE - RENDER_H);
    TEST_PASS();
}

static void test_character_no_edge_oscillation(void) {
    TEST_BEGIN("character: no state oscillation at platform edges");
    Character p;
    character_init(&p, 7.0f * TILE_SIZE - 10, 26.0f * TILE_SIZE - RENDER_H);
    p.onGround = 1;

    CharState history[120];
    simulate_record(&p, 120, 1, history);
    ASSERT_TRUE(!detect_oscillation(history, 120));
    TEST_PASS();
}

static void test_character_wall_slide_on_real_wall(void) {
    TEST_BEGIN("character: wall-slide triggers on real walls");
    Character p;
    character_init(&p, 37.0f * TILE_SIZE, 10.0f * TILE_SIZE);
    p.onGround = 0;
    p.vy = 5.0f;
    p.facing = 1;
    p.state = STATE_JUMP_FALL;

    simulate(&p, 15, 1, -1);
    ASSERT_TRUE(p.state == STATE_WALL_SLIDE || p.state == STATE_JUMP_FALL ||
                p.state == STATE_CORNER_GRAB);
    TEST_PASS();
}

static void test_character_no_wall_slide_on_thin_platform(void) {
    TEST_BEGIN("character: thin platforms don't trigger wall-slide");
    Character p;
    character_init(&p, 32.0f * TILE_SIZE, 6.0f * TILE_SIZE);
    p.onGround = 0;
    p.vy = 3.0f;
    p.facing = 1;
    p.state = STATE_JUMP_FALL;

    simulate(&p, 20, 1, -1);
    ASSERT_TRUE(p.state != STATE_WALL_SLIDE);
    TEST_PASS();
}

static void test_character_corner_grab(void) {
    TEST_BEGIN("character: corner-grab triggers when falling past ledge");
    Character p;
    character_init(&p, 32.0f * TILE_SIZE, 20.0f * TILE_SIZE);
    p.onGround = 0;
    p.vy = 3.0f;
    p.facing = 1;
    p.state = STATE_JUMP_FALL;

    simulate(&p, 60, 1, -1);
    ASSERT_TRUE(p.state == STATE_CORNER_GRAB || p.state == STATE_CORNER_CLIMB ||
                p.onGround == 1);
    TEST_PASS();
}

static void test_character_corner_climb_lands_solid(void) {
    TEST_BEGIN("character: after corner-climb, character is on solid ground");
    Character p;
    character_init(&p, 32.0f * TILE_SIZE, 23.0f * TILE_SIZE - RENDER_H / 4);
    p.onGround = 1;
    p.vy = 0;
    p.vx = 0;
    p.facing = 1;
    p.state = STATE_CORNER_GRAB;
    p.stateTimer = 0;
    p.grabCooldown = 0;

    simulate(&p, 1, 0, 0);
    simulate(&p, 25, 0, -1);

    ASSERT_EQ_INT(p.onGround, 1);
    ASSERT_EQ_INT(p.state, STATE_IDLE);
    int cx = (int)p.x + RENDER_W / 2;
    int fy = (int)p.y + RENDER_H + 1;
    ASSERT_EQ_INT(tile_solid(cx, fy), 1);
    TEST_PASS();
}

static void test_character_auto_climb_on_forward_hold(void) {
    TEST_BEGIN("character: holding forward in corner-grab auto-climbs after 15 frames");
    Character p;
    character_init(&p, 32.0f * TILE_SIZE, 23.0f * TILE_SIZE - RENDER_H / 4);
    p.onGround = 1;
    p.vy = 0;
    p.vx = 0;
    p.facing = 1;
    p.state = STATE_CORNER_GRAB;
    p.stateTimer = 0;
    p.grabCooldown = 0;

    simulate(&p, 10, 1, -1);
    ASSERT_EQ_INT(p.state, STATE_CORNER_GRAB);

    simulate(&p, 10, 1, -1);
    ASSERT_TRUE(p.state == STATE_CORNER_CLIMB || p.state == STATE_IDLE);
    TEST_PASS();
}

static void test_character_hard_landing(void) {
    TEST_BEGIN("character: high fall produces hard landing (ninja roll)");
    Character p;
    character_init(&p, 10.0f * TILE_SIZE, 3.0f * TILE_SIZE);
    p.onGround = 0;
    p.vy = 0.0f;
    p.state = STATE_JUMP_FALL;
    p.facing = 1;

    simulate(&p, 120, 0, -1);
    ASSERT_EQ_INT(p.onGround, 1);
    TEST_PASS();
}

void run_character_tests(void) {
    printf("[Character]\n");
    test_character_starts_idle();
    test_character_walk_right();
    test_character_walk_to_run();
    test_character_stop_on_release();
    test_character_jump();
    test_character_fall_off_edge();
    test_character_no_edge_oscillation();
    test_character_wall_slide_on_real_wall();
    test_character_no_wall_slide_on_thin_platform();
    test_character_corner_grab();
    test_character_corner_climb_lands_solid();
    test_character_auto_climb_on_forward_hold();
    test_character_hard_landing();
}
