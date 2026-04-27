/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * resource.h — Resource IDs for embedded sprite PNGs
 *
 * Each animation prefix maps to a base ID. Individual frames are
 * base + frame_index (e.g., IDR_CRNR_CLMB + 2 = frame 02).
 *
 * ID spacing scheme: base IDs are 16 apart (MAX_ANIM_FRAMES) so that
 * frame N of an animation = base + N.  16 was chosen as the smallest
 * power-of-2 that comfortably fits all current animations (the longest
 * has 6 frames), leaving room for future expansion without collisions.
 */
#ifndef RESOURCE_H
#define RESOURCE_H

/* Base IDs for each animation (spaced by MAX_ANIM_FRAMES = 16) */
#define IDR_CRNR_CLMB       100
#define IDR_CRNR_GRB        116
#define IDR_CROUCH           132
#define IDR_FALL             148
#define IDR_IDLE             164
#define IDR_JUMP             180
#define IDR_RUN              196
#define IDR_SMRSLT           212
#define IDR_STAND            228
#define IDR_WALK             244
#define IDR_WALL_SLIDE       260
#define IDR_SLIDE            276

#endif /* RESOURCE_H */
