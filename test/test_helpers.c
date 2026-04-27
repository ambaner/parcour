/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * test_helpers.c — Shared helper function implementations
 */
#include "test_framework.h"
#include "test_helpers.h"
#include "types.h"
#include "character.h"
#include "input.h"
#include "physics.h"
#include "level.h"

#include <stdio.h>

/* ── Invariant fail counter ── */
int g_invariantFails = 0;

/* ── Input helpers ── */

void input_clear(void) {
    keyLeft = keyRight = keyUp = keyDown = 0;
    keyUpJustPressed = keyDownJustPressed = 0;
    keyUpPrev = keyDownPrev = 0;
}

void input_set_dir(int dir) {
    keyLeft = (dir < 0) ? 1 : 0;
    keyRight = (dir > 0) ? 1 : 0;
}

/* ── Simulation helpers ── */

void simulate(Character *p, int frames, int dir, int jumpOnFrame) {
    for (int i = 0; i < frames; i++) {
        input_clear();
        input_set_dir(dir);

        if (jumpOnFrame >= 0 && i == jumpOnFrame) {
            keyUp = 1;
            keyUpJustPressed = 1;
        } else if (jumpOnFrame >= 0 && i > jumpOnFrame && i <= jumpOnFrame + 5) {
            keyUp = 1;
            keyUpJustPressed = 0;
        }

        character_update(p);
    }
}

void simulate_record(Character *p, int frames, int dir, CharState *history) {
    for (int i = 0; i < frames; i++) {
        input_clear();
        input_set_dir(dir);
        character_update(p);
        history[i] = p->state;
    }
}

void simulate_checked(Character *p, int frames, int dir, int jumpOnFrame) {
    for (int i = 0; i < frames; i++) {
        input_clear();
        input_set_dir(dir);

        if (jumpOnFrame >= 0 && i == jumpOnFrame) {
            keyUp = 1;
            keyUpJustPressed = 1;
        } else if (jumpOnFrame >= 0 && i > jumpOnFrame && i <= jumpOnFrame + 5) {
            keyUp = 1;
            keyUpJustPressed = 0;
        }

        character_update(p);
        check_invariants(p, i);
    }
}

/* ── Oscillation detector ── */

int detect_oscillation(CharState *history, int count) {
    if (count < 6) return 0;
    for (int i = 0; i < count - 5; i++) {
        if (history[i] != history[i+1] &&
            history[i] == history[i+2] &&
            history[i+1] == history[i+3] &&
            history[i] == history[i+4] &&
            history[i+1] == history[i+5]) {
            return 1;
        }
    }
    return 0;
}

/* ── Invariant checker ── */

void check_invariants(const Character *p, int frameNum) {
    if (p->x < -RENDER_W || p->x > SCREEN_W + RENDER_W) {
        printf("    INVARIANT FAIL [frame %d]: x=%.1f out of bounds\n",
               frameNum, p->x);
        g_invariantFails++;
    }
    if (p->y < -RENDER_H * 2 || p->y > SCREEN_H + RENDER_H) {
        printf("    INVARIANT FAIL [frame %d]: y=%.1f fell through world\n",
               frameNum, p->y);
        g_invariantFails++;
    }

    if (p->state < 0 || p->state >= STATE_COUNT) {
        printf("    INVARIANT FAIL [frame %d]: invalid state %d\n",
               frameNum, p->state);
        g_invariantFails++;
    }
    if (p->facing != 1 && p->facing != -1) {
        printf("    INVARIANT FAIL [frame %d]: invalid facing %d\n",
               frameNum, p->facing);
        g_invariantFails++;
    }

    if (p->onGround && p->state != STATE_CORNER_GRAB &&
        p->state != STATE_CORNER_CLIMB) {
        int cx = (int)p->x + RENDER_W / 2;
        int fy = (int)p->y + RENDER_H + 1;
        if (cx >= 0 && cx < SCREEN_W && fy >= 0 && fy < SCREEN_H) {
            if (!tile_solid(cx, fy)) {
                printf("    INVARIANT FAIL [frame %d]: onGround=1 but no solid under foot "
                       "(cx=%d fy=%d pos=%.1f,%.1f state=%d)\n",
                       frameNum, cx, fy, p->x, p->y, p->state);
                g_invariantFails++;
            }
        }
    }
}

/* ── Deterministic replay ── */

void replay(Character *p, const InputFrame *inputs, int count) {
    for (int i = 0; i < count; i++) {
        input_clear();
        input_set_dir(inputs[i].dir);
        if (inputs[i].jump) {
            keyUp = 1;
            keyUpJustPressed = 1;
        }
        if (inputs[i].down) {
            keyDown = 1;
            keyDownJustPressed = 1;
        }
        character_update(p);
        check_invariants(p, i);
    }
}
