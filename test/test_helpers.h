/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * test_helpers.h — Shared helper functions for all test modules
 */
#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include "character.h"

/* ── Input helpers ── */
void input_clear(void);
void input_set_dir(int dir);

/* ── Simulation helpers ── */
void simulate(Character *p, int frames, int dir, int jumpOnFrame);
void simulate_record(Character *p, int frames, int dir, CharState *history);
void simulate_checked(Character *p, int frames, int dir, int jumpOnFrame);

/* ── Oscillation detector ── */
int detect_oscillation(CharState *history, int count);

/* ── Invariant checker ── */
extern int g_invariantFails;
void check_invariants(const Character *p, int frameNum);

/* ── Deterministic replay ── */
typedef struct {
    int dir;     /* -1=left, 0=none, 1=right */
    int jump;    /* 1=press Up this frame */
    int down;    /* 1=press Down this frame */
} InputFrame;

void replay(Character *p, const InputFrame *inputs, int count);

#define REPEAT_INPUT(d, j, dn, n) \
    for (int _ri = 0; _ri < (n); _ri++) { inputs[ic].dir=(d); inputs[ic].jump=(j); inputs[ic].down=(dn); ic++; }

/* ── Per-module test entry points ── */
void run_math_tests(void);
void run_gravity_tests(void);
void run_level_tests(void);
void run_physics_tests(void);
void run_character_tests(void);
void run_state_transition_tests(void);
void run_regression_tests(void);
void run_sweep_tests(void);
void run_replay_tests(void);
void run_integration_tests(void);

#endif /* TEST_HELPERS_H */
