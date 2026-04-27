/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * character.h — Sprite-based character: states, animation, and public API
 *
 * Uses pre-rendered PNG sprite frames (rvros Animated Pixel Adventurer)
 * loaded at startup. Each state maps to a SpriteAnim.
 */
#ifndef CHARACTER_H
#define CHARACTER_H

#include "types.h"
#include "sprite.h"

/* ── Sprite display scale (3× = 150×111 px on screen) ──────────────── */
#define SPRITE_DRAW_SCALE  3

/* ── Character states ───────────────────────────────────────────────── */
typedef enum {
    STATE_IDLE,
    STATE_WALK,
    STATE_RUNNING,
    STATE_STOPPING,
    STATE_TURNING,
    STATE_CROUCH,
    STATE_JUMP_RISE,
    STATE_JUMP_FALL,
    STATE_LANDING,
    STATE_CORNER_GRAB,
    STATE_CORNER_CLIMB,
    STATE_WALL_SLIDE,
    STATE_SOMERSAULT,
    STATE_SLIDE,
    STATE_HARD_LANDING,
    STATE_COUNT
} CharState;

/* ── Character struct ───────────────────────────────────────────────── */
typedef struct {
    float x, y;            /* top-left of bounding box */
    float vx, vy;
    CharState state;
    int facing;            /* +1 = right, -1 = left */
    int animFrame;
    int animTick;
    int onGround;
    int stateTimer;        /* frames elapsed since entering current state;
                            * used for timed transitions (e.g. walk→run
                            * promotion at 30 frames, crouch wind-up) */
    int jumpHoldFrames;    /* how many frames UP has been held mid-jump;
                            * capped at JUMP_HOLD_MAX to implement
                            * variable jump height (tap = short hop,
                            * hold = full height) */
    float landingVy;       /* vertical speed at the moment of landing;
                            * used to classify impact tier: soft (instant
                            * recovery), medium (stagger), hard (ninja roll) */
    int grabCooldown;      /* frames before corner-grab can re-trigger;
                            * prevents oscillation when walking off a ledge
                            * or releasing a grab (would instantly re-grab
                            * the same ledge without this delay) */
} Character;

/* ── Public API ─────────────────────────────────────────────────────── */
int  character_load_sprites(const char *spriteDir);
void character_free_sprites(void);
void character_init(Character *p, float startX, float startY);
void character_update(Character *p);
void character_draw(const Character *p);

/* ── Dimensions of the on-screen sprite (for collision) ─────────────── */
int  character_draw_width(void);
int  character_draw_height(void);

#endif /* CHARACTER_H */
