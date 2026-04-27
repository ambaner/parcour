/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * test_gravity.c — Unit tests for gravity / falling physics
 */
#include "test_framework.h"
#include "test_helpers.h"
#include "gravity.h"

static void test_gravity_accelerates(void) {
    TEST_BEGIN("gravity: velocity increases each frame while falling");
    float vy = 0.0f;
    float prev = vy;
    for (int i = 0; i < 10; i++) {
        gravity_apply(&vy);
        ASSERT_TRUE(vy > prev);
        prev = vy;
    }
    TEST_PASS();
}

static void test_gravity_terminal_velocity(void) {
    TEST_BEGIN("gravity: vy stays below terminal velocity");
    float vy = 0.0f;
    for (int i = 0; i < 300; i++) {
        gravity_apply(&vy);
    }
    ASSERT_TRUE(vy <= GRAV_TERMINAL);
    ASSERT_TRUE(vy > 0.0f);
    TEST_PASS();
}

static void test_gravity_drag_slows_acceleration(void) {
    TEST_BEGIN("gravity: air drag makes later frames accelerate less");
    float vy = 0.0f;
    gravity_apply(&vy);
    float delta1 = vy;

    for (int i = 0; i < 30; i++) gravity_apply(&vy);
    float before = vy;
    gravity_apply(&vy);
    float deltaN = vy - before;

    ASSERT_TRUE(deltaN < delta1);
    TEST_PASS();
}

static void test_gravity_landing_tiers(void) {
    TEST_BEGIN("gravity: landing tier classification");
    ASSERT_EQ_INT(gravity_landing_tier(2.0f), 0);
    ASSERT_EQ_INT(gravity_landing_tier(5.0f), 1);
    ASSERT_EQ_INT(gravity_landing_tier(10.0f), 2);
    TEST_PASS();
}

void run_gravity_tests(void) {
    printf("[Gravity]\n");
    test_gravity_accelerates();
    test_gravity_terminal_velocity();
    test_gravity_drag_slows_acceleration();
    test_gravity_landing_tiers();
}
