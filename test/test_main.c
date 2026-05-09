/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * test_main.c — Test runner entry point
 *
 * Calls per-module test functions defined in separate source files.
 * Each module (math, gravity, level, physics, character, etc.) has its
 * own test_*.c file for clarity.
 *
 * Build:
 *   cl /O2 /W4 /WX /I..\src test_main.c test_helpers.c test_math.c
 *      test_gravity.c test_level.c test_physics.c test_character.c
 *      test_state_transitions.c test_regression.c test_sweep.c
 *      test_replay.c test_integration.c
 *      ..\src\character.c ..\src\sprite.c ..\src\input.c ..\src\renderer.c
 *      ..\src\level.c ..\src\physics.c ..\src\math.c ..\src\log.c
 *      ..\src\gravity.c
 *      /link user32.lib gdi32.lib winmm.lib
 */

#include "test_framework.h"
#include "test_helpers.h"
#include "log.h"

#include <stdio.h>
#include <string.h>

/* Global test counters (owned by test_main.c) */
int g_testsPassed = 0;
int g_testsFailed = 0;
const char *g_currentTest = "";

int main(int argc, char *argv[]) {
    game_log_init();
    game_log("=== TEST RUNNER STARTED ===");

    /* Parse --quick / --full flags (default: full) */
    int runFull = 1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--quick") == 0) { runFull = 0; }
        if (strcmp(argv[i], "--full") == 0)  { runFull = 1; }
    }

    printf("\n");
    printf("========================================\n");
    printf("  Parcour Test Suite (%s)\n", runFull ? "full" : "quick");
    printf("========================================\n\n");

    /* ── Quick tests (fast, no multi-frame simulation) ── */
    run_math_tests();
    printf("\n"); run_gravity_tests();
    printf("\n"); run_level_tests();
    printf("\n"); run_physics_tests();
    printf("\n"); run_levelfile_tests();
    printf("\n"); run_levelgen_tests();

    if (!runFull) goto SUMMARY;

    /* ── Full tests (multi-frame simulation, sweeps, replays) ── */
    printf("\n"); run_character_tests();
    printf("\n"); run_state_transition_tests();
    printf("\n"); run_regression_tests();
    printf("\n"); run_sweep_tests();
    printf("\n"); run_replay_tests();
    printf("\n"); run_integration_tests();

SUMMARY:
    printf("\n========================================\n");
    printf("  Results: %d passed, %d failed, %d total\n",
           g_testsPassed, g_testsFailed, g_testsPassed + g_testsFailed);
    printf("========================================\n\n");

    game_log("=== TEST RUNNER FINISHED: %d passed, %d failed ===",
             g_testsPassed, g_testsFailed);
    game_log_close();

    return g_testsFailed > 0 ? 1 : 0;
}
