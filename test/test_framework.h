/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * test_framework.h — Minimal test framework macros and shared state
 */
#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>

/* Shared counters — defined in test_main.c */
extern int g_testsPassed;
extern int g_testsFailed;
extern const char *g_currentTest;

#define TEST_BEGIN(name) \
    do { \
        g_currentTest = name; \
    } while(0)

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            printf("  FAIL: %s\n", g_currentTest); \
            printf("        %s:%d: ASSERT_TRUE(%s)\n", __FILE__, __LINE__, #cond); \
            g_testsFailed++; \
            return; \
        } \
    } while(0)

#define ASSERT_EQ_FLOAT(a, b, eps) \
    do { \
        float _a = (a), _b = (b), _e = (eps); \
        if ((_a - _b) > _e || (_b - _a) > _e) { \
            printf("  FAIL: %s\n", g_currentTest); \
            printf("        %s:%d: ASSERT_EQ_FLOAT(%.4f, %.4f, eps=%.4f)\n", \
                   __FILE__, __LINE__, _a, _b, _e); \
            g_testsFailed++; \
            return; \
        } \
    } while(0)

#define ASSERT_EQ_INT(a, b) \
    do { \
        int _a = (a), _b = (b); \
        if (_a != _b) { \
            printf("  FAIL: %s\n", g_currentTest); \
            printf("        %s:%d: ASSERT_EQ_INT(%d, %d)\n", \
                   __FILE__, __LINE__, _a, _b); \
            g_testsFailed++; \
            return; \
        } \
    } while(0)

#define TEST_PASS() \
    do { \
        printf("  PASS: %s\n", g_currentTest); \
        g_testsPassed++; \
    } while(0)

#endif /* TEST_FRAMEWORK_H */
