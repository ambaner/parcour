/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * test_math.c — Unit tests for math utility functions
 */
#include "test_framework.h"
#include "test_helpers.h"
#include "math.h"

static void test_absf(void) {
    TEST_BEGIN("math: absf");
    ASSERT_EQ_FLOAT(absf(-5.0f), 5.0f, 0.001f);
    ASSERT_EQ_FLOAT(absf(3.0f), 3.0f, 0.001f);
    ASSERT_EQ_FLOAT(absf(0.0f), 0.0f, 0.001f);
    TEST_PASS();
}

static void test_clampf(void) {
    TEST_BEGIN("math: clampf");
    ASSERT_EQ_FLOAT(clampf(5.0f, 0.0f, 3.0f), 3.0f, 0.001f);
    ASSERT_EQ_FLOAT(clampf(-2.0f, 0.0f, 3.0f), 0.0f, 0.001f);
    ASSERT_EQ_FLOAT(clampf(1.5f, 0.0f, 3.0f), 1.5f, 0.001f);
    TEST_PASS();
}

static void test_signf(void) {
    TEST_BEGIN("math: signf");
    ASSERT_EQ_FLOAT(signf(10.0f), 1.0f, 0.001f);
    ASSERT_EQ_FLOAT(signf(-3.0f), -1.0f, 0.001f);
    ASSERT_EQ_FLOAT(signf(0.0f), 0.0f, 0.001f);
    TEST_PASS();
}

static void test_lerpf(void) {
    TEST_BEGIN("math: lerpf");
    ASSERT_EQ_FLOAT(lerpf(0.0f, 10.0f, 0.5f), 5.0f, 0.001f);
    ASSERT_EQ_FLOAT(lerpf(0.0f, 10.0f, 0.0f), 0.0f, 0.001f);
    ASSERT_EQ_FLOAT(lerpf(0.0f, 10.0f, 1.0f), 10.0f, 0.001f);
    TEST_PASS();
}

void run_math_tests(void) {
    printf("[Math]\n");
    test_absf();
    test_clampf();
    test_signf();
    test_lerpf();
}
