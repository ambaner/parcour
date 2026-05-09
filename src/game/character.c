/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * character.c — Sprite-based character: loading, state machine, drawing
 *
 * Uses rvros "Animated Pixel Adventurer" sprite frames (50x37 px each)
 * loaded from PNG files via stb_image. Each CharState maps to a
 * SpriteAnim with its own frame sequence and timing.
 */

#include "character.h"
#include "input.h"
#include "physics.h"
#include "gravity.h"
#include "renderer.h"
#include "level.h"
#include "math.h"
#include "log.h"
#include "resource.h"
#include <string.h>

/* ════════════════════════════════════════════════════════════════════════
 * SPRITE ANIMATION DATA
 * ════════════════════════════════════════════════════════════════════════ */

static SpriteAnim anims[STATE_COUNT];

/*
 * AnimDef — Table-driven animation configuration.
 *
 * Instead of hard-coding sprite loading logic per state, each CharState
 * indexes into anim_defs[] to find its sprite prefix (for disk loading),
 * RCDATA base ID (for embedded resources), playback speed (ticks per
 * frame), and loop flag.  This makes adding new states trivial: just
 * append one row to the table and add the enum value.
 */
typedef struct {
    const char *prefix;
    int baseId;     /* RCDATA resource base ID (0 = reuses another) */
    int speed;
    int loop;
} AnimDef;

/*
 * Animation definition table: maps each CharState (by index) to its
 * sprite file prefix, embedded resource base ID, playback speed (ticks
 * per frame — lower = faster), and loop behavior.
 *
 * Several states share the same sprite prefix/resource (e.g. LANDING,
 * SOMERSAULT, SLIDE, and HARD_LANDING all use "smrslt") but differ in
 * speed and looping — the state machine selects the right feel.
 */
static const AnimDef anim_defs[STATE_COUNT] = {
    /* STATE_IDLE         */ { "adventurer-idle",       IDR_IDLE,       8,  1 },
    /* STATE_WALK         */ { "adventurer-walk",       IDR_WALK,       9,  1 },
    /* STATE_RUNNING      */ { "adventurer-run",        IDR_RUN,        3,  1 },
    /* STATE_STOPPING     */ { "adventurer-stand",      IDR_STAND,      4,  0 },
    /* STATE_TURNING      */ { "adventurer-stand",      IDR_STAND,      3,  0 },
    /* STATE_CROUCH       */ { "adventurer-crouch",     IDR_CROUCH,     3,  0 },
    /* STATE_JUMP_RISE    */ { "adventurer-jump",       IDR_JUMP,       5,  0 },
    /* STATE_JUMP_FALL    */ { "adventurer-fall",       IDR_FALL,       6,  1 },
    /* STATE_LANDING      */ { "adventurer-smrslt",     IDR_SMRSLT,     3,  0 },
    /* STATE_CORNER_GRAB  */ { "adventurer-crnr-grb",   IDR_CRNR_GRB,  6,  0 },
    /* STATE_CORNER_CLIMB */ { "adventurer-crnr-clmb",  IDR_CRNR_CLMB, 4,  0 },
    /* STATE_WALL_SLIDE   */ { "adventurer-wall-slide", IDR_WALL_SLIDE, 6,  1 },
    /* STATE_SOMERSAULT   */ { "adventurer-smrslt",     IDR_SMRSLT,     3,  1 },
    /* STATE_SLIDE        */ { "adventurer-slide",      IDR_SMRSLT,     4,  0 },
    /* STATE_HARD_LANDING */ { "adventurer-smrslt",     IDR_SMRSLT,     3,  0 },
};

/* ════════════════════════════════════════════════════════════════════════
 * SPRITE LOADING / FREEING
 * ════════════════════════════════════════════════════════════════════════ */

/*
 * character_load_sprites — Load all animation frames for every CharState.
 *
 * Dual-mode loading strategy:
 *   1. Try embedded Win32 resources first (RCDATA in the .exe) — this
 *      enables single-file deployment with no external dependencies.
 *   2. If zero frames were found in resources (exe built without .rc),
 *      fall back to loading loose PNGs from spriteDir on disk — this
 *      is the normal development workflow.
 *
 * Returns total number of frames loaded across all animations.
 */
int character_load_sprites(const char *spriteDir)
{
    int total = 0;

    /* Try embedded resources first (single-exe mode) */
    for (int i = 0; i < STATE_COUNT; i++) {
        int n = sprite_anim_load_rc(&anims[i],
                                    anim_defs[i].baseId,
                                    anim_defs[i].speed,
                                    anim_defs[i].loop);
        total += n;
    }

    if (total > 0)
        return total;

    /* Fallback: load from loose PNG files on disk */
    for (int i = 0; i < STATE_COUNT; i++) {
        int n = sprite_anim_load(&anims[i], spriteDir,
                                 anim_defs[i].prefix,
                                 anim_defs[i].speed,
                                 anim_defs[i].loop);
        total += n;
    }
    return total;
}

void character_free_sprites(void)
{
    for (int i = 0; i < STATE_COUNT; i++)
        sprite_anim_free(&anims[i]);
}

int character_draw_width(void)
{
    if (anims[0].count > 0)
        return anims[0].frames[0].w * SPRITE_DRAW_SCALE;
    return RENDER_W;
}

int character_draw_height(void)
{
    if (anims[0].count > 0)
        return anims[0].frames[0].h * SPRITE_DRAW_SCALE;
    return RENDER_H;
}

/* state_name — Debug helper: returns a human-readable name for a CharState. */
static const char *state_name(CharState s) {
    static const char *names[] = {
        "IDLE","WALK","RUNNING","STOPPING","TURNING","CROUCH",
        "JUMP_RISE","JUMP_FALL","LANDING","CORNER_GRAB","CORNER_CLIMB",
        "WALL_SLIDE","SOMERSAULT","SLIDE","HARD_LANDING"
    };
    if (s >= 0 && s < STATE_COUNT) return names[s];
    return "???";
}

/* set_state — Transition to a new state, resetting animation and timer.
 * No-ops if already in the target state (prevents animation restarts). */
static void set_state(Character *p, CharState s) {
    if (p->state != s) {
        game_log("STATE: %s -> %s  (pos=%.1f,%.1f vel=%.2f,%.2f ground=%d)",
                 state_name(p->state), state_name(s),
                 p->x, p->y, p->vx, p->vy, p->onGround);
        p->state = s;
        p->stateTimer = 0;
        p->animTick = 0;
        p->animFrame = 0;
    }
}

/* ════════════════════════════════════════════════════════════════════════
 * PUBLIC API
 * ════════════════════════════════════════════════════════════════════════ */

/* character_init — Set up initial character state at the given spawn point. */
void character_init(Character *p, float startX, float startY) {
    memset(p, 0, sizeof(*p));
    p->x = startX;
    p->y = startY;
    p->facing = 1;
    p->state = STATE_IDLE;
}

/*
 * character_update — Per-frame state machine, physics, and animation.
 *
 * Overall structure:
 *   1. Ground states switch — handle walking, running, crouching, turning,
 *      sliding, landing recovery, corner grab/climb while grounded
 *   2. Air states — variable jump height, air control, rise→fall transition,
 *      wall-slide detection (pre-collision), corner-grab detection
 *   3. Gravity — applied unless hanging from a ledge
 *   4. Horizontal collision — wall pushback, post-collision wall-slide trigger
 *   5. Head-ledge grab — PoP-style reach-up-and-grab while walking
 *   6. Vertical collision — ground landing + landing tier classification
 *   7. "Fell off ledge" check — catches cases where ground disappears
 *   8. Animation advance — tick→frame system with loop vs hold-last
 */
void character_update(Character *p) {
    int inputDir = 0;
    if (keyLeft)  inputDir = -1;
    if (keyRight) inputDir =  1;

    p->stateTimer++;
    if (p->grabCooldown > 0) p->grabCooldown--;
    if (p->stepCooldown > 0) p->stepCooldown--;

    /* ── Ground states ────────────────────────────────────────────── */
    if (p->onGround) {
        switch (p->state) {

        case STATE_IDLE:
            physics_apply_friction(&p->vx, GROUND_FRICTION);
            if (keyUpJustPressed) {
                set_state(p, STATE_CROUCH);   /* crouch → jump */
            } else if (keyDownJustPressed) {
                set_state(p, STATE_CROUCH);   /* crouch and stay */
            } else if (inputDir != 0) {
                p->facing = inputDir;
                set_state(p, STATE_WALK);
            }
            break;

        case STATE_WALK:
            if (inputDir == p->facing) {
                /* Walk speed is capped at 33% of max run speed */
                float walkMax = MAX_RUN_SPEED * 0.33f;
                physics_apply_accel(&p->vx, GROUND_ACCEL * 0.5f, p->facing);
                if (absf(p->vx) > walkMax)
                    p->vx = walkMax * (float)p->facing;
                physics_apply_friction(&p->vx, GROUND_FRICTION);
                // Walk→run promotion: after 30 frames of continuous walking,
                // transition to full run speed (PoP-style momentum build-up)
                if (p->stateTimer >= 30)
                    set_state(p, STATE_RUNNING);
            } else if (inputDir == -p->facing) {
                set_state(p, STATE_TURNING);
            } else {
                p->vx = 0;
                set_state(p, STATE_IDLE);
            }
            if (keyUpJustPressed)
                set_state(p, STATE_CROUCH);
            else if (keyDownJustPressed)
                set_state(p, STATE_CROUCH);
            break;

        case STATE_RUNNING:
            if (inputDir == p->facing) {
                physics_apply_accel(&p->vx, GROUND_ACCEL, p->facing);
                physics_apply_friction(&p->vx, GROUND_FRICTION);
            } else if (inputDir == -p->facing) {
                set_state(p, STATE_TURNING);
                break;
            } else {
                set_state(p, STATE_STOPPING);
                break;
            }
            /* Running jump → somersault (1.1× normal jump impulse for a
             * visually distinct, more athletic leap when at full sprint) */
            if (keyUpJustPressed) {
                p->vy = JUMP_IMPULSE * 1.1f;
                p->onGround = 0;
                p->jumpHoldFrames = 0;
                set_state(p, STATE_SOMERSAULT);
            } else if (keyDownJustPressed) {
                set_state(p, STATE_SLIDE);
            }
            break;

        case STATE_STOPPING:
            physics_apply_friction(&p->vx, GROUND_FRICTION);
            if (absf(p->vx) < STOP_THRESHOLD) {
                p->vx = 0;
                set_state(p, STATE_IDLE);
            } else if (inputDir == p->facing) {
                set_state(p, STATE_WALK);
            } else if (inputDir != 0 && inputDir == -p->facing) {
                set_state(p, STATE_TURNING);
            }
            if (keyUpJustPressed)
                set_state(p, STATE_CROUCH);
            break;

        case STATE_TURNING:
            /* Decelerate during the turn with slightly reduced friction
             * (0.85×) so the slide-to-stop feels weighty, not instant */
            physics_apply_friction(&p->vx, GROUND_FRICTION * 0.85f);
            /* After TURN_TICKS (or if velocity drops below threshold),
             * flip facing direction and resume walking or go idle */
            if (p->stateTimer >= TURN_TICKS || absf(p->vx) < STOP_THRESHOLD) {
                p->facing = -p->facing;
                p->vx = 0;
                if (inputDir == p->facing)
                    set_state(p, STATE_WALK);
                else
                    set_state(p, STATE_IDLE);
            }
            if (keyUpJustPressed) {
                p->facing = -p->facing;
                set_state(p, STATE_CROUCH);
            }
            break;

        case STATE_CROUCH:
            /* Crouch is the "wind-up" for a jump (PoP sequence: crouch →
             * spring up).  Also serves as a standalone crouch if DOWN is
             * held without UP.  The 4-frame delay prevents accidental
             * jumps from a quick tap of DOWN then UP in the same beat. */
            physics_apply_friction(&p->vx, GROUND_FRICTION);
            if (keyUpJustPressed || (keyUp && p->stateTimer >= 4)) {
                /* Jump! */
                p->vy = JUMP_IMPULSE;
                p->onGround = 0;
                p->jumpHoldFrames = 0;
                if (inputDir != 0) {
                    float speed = absf(p->vx);
                    if (speed < MAX_RUN_SPEED * 0.4f)
                        p->vx = MAX_RUN_SPEED * 0.5f * inputDir;
                }
                set_state(p, STATE_JUMP_RISE);
            } else if (!keyDown && !keyUp && p->stateTimer >= 4) {
                /* Released down without pressing up → stand up */
                set_state(p, STATE_IDLE);
            }
            break;

        case STATE_SLIDE:
            /* Running crouch-slide: momentum carry with extra-low friction
             * (0.92×) for a satisfying slide distance.  Ends after 20
             * frames or when speed drops below threshold. */
            physics_apply_friction(&p->vx, GROUND_FRICTION * 0.92f);
            if (p->stateTimer >= 20 || absf(p->vx) < STOP_THRESHOLD) {
                p->vx = 0;
                set_state(p, STATE_IDLE);
            }
            break;

        case STATE_LANDING:
            physics_apply_friction(&p->vx, GROUND_FRICTION * 0.9f);
            if (p->stateTimer >= LANDING_TICKS) {
                if (inputDir != 0) {
                    p->facing = inputDir;
                    set_state(p, STATE_WALK);
                } else {
                    set_state(p, STATE_IDLE);
                }
            }
            if (keyUpJustPressed && p->stateTimer >= 2)
                set_state(p, STATE_CROUCH);
            break;

        case STATE_HARD_LANDING:
            /* Ninja roll — character tumbles forward to absorb impact.
             * Uses somersault animation + forward momentum, then
             * transitions to crouch as recovery. */
            physics_apply_friction(&p->vx, GROUND_FRICTION * 0.95f);
            if (p->stateTimer >= 18) {
                /* Roll finished → brief crouch recovery */
                p->vx = 0;
                set_state(p, STATE_CROUCH);
            }
            break;

        case STATE_CORNER_GRAB:
            /* Hanging on a ledge — velocity zeroed to "freeze" on the wall.
             *   - UP (instant) or hold forward for ~15 frames → climb up
             *   - DOWN or pressing away → release with a small downward
             *     push and 20-frame grab cooldown to prevent immediately
             *     re-grabbing the same ledge */
            p->vx = 0;
            p->vy = 0;
            if (keyUpJustPressed ||
                (inputDir == p->facing && p->stateTimer >= 15)) {
                set_state(p, STATE_CORNER_CLIMB);
            } else if (keyDownJustPressed || inputDir == -p->facing) {
                /* Let go — push away from ledge so we don't re-grab */
                p->vy = 2.0f;
                p->onGround = 0;
                p->grabCooldown = 20;  // ignore ledges for 20 frames to prevent oscillation
                set_state(p, STATE_JUMP_FALL);
            }
            break;

        case STATE_CORNER_CLIMB:
            /* Climbing animation plays for 20 frames with velocity frozen.
             * At completion, teleport the character onto the ledge surface:
             *   - Shift horizontally by RENDER_W (≈2 tiles) in facing dir
             *   - Shift upward by 3/4 of RENDER_H to place feet on top */
            p->vx = 0;
            p->vy = 0;
            if (p->stateTimer >= 20) {
                /* Finished climbing — move character on top of the ledge.
                 * Shift 2 tiles horizontally (= RENDER_W) so the center
                 * of mass lands solidly on the platform. */
                float destX = p->x + (float)(p->facing * RENDER_W);
                float destY = p->y - (float)(RENDER_H * 3 / 4);

                /* Validate destination isn't fully blocked — check mid/foot
                 * body probes. Skip head check since the ledge surface we're
                 * climbing onto may have tiles directly above. */
                int destBodyL = (int)destX + 8;
                int destBodyR = (int)destX + RENDER_W - 9;
                int destFoot  = (int)destY + RENDER_H - 2;
                int destMid   = (int)destY + RENDER_H / 2;
                if (tile_solid(destBodyL, destFoot) && tile_solid(destBodyR, destFoot) &&
                    tile_solid(destBodyL, destMid)  && tile_solid(destBodyR, destMid)) {
                    /* Destination fully blocked — abort climb, fall back */
                    set_state(p, STATE_CORNER_GRAB);
                    break;
                }

                p->x = destX;
                p->y = destY;

                /* Safety nudge: if center-foot is over air after teleport,
                 * nudge BACKWARD (toward the wall we climbed) first, since
                 * the platform surface is behind us.  Nudging forward would
                 * push past narrow platforms into boundary walls, trapping
                 * the character with bodyR inside a wall column. */
                {
                    int cx = (int)p->x + RENDER_W / 2;
                    int fy = (int)p->y + RENDER_H + 1;
                    if (!tile_solid(cx, fy)) {
                        /* Try nudging backward (toward the wall/platform) */
                        float savedX = p->x;
                        int found = 0;
                        for (int nudge = 0; nudge < TILE_SIZE; nudge++) {
                            p->x -= (float)p->facing;
                            cx = (int)p->x + RENDER_W / 2;
                            if (tile_solid(cx, fy)) { found = 1; break; }
                        }
                        if (!found) {
                            /* Backward failed — try forward as last resort */
                            p->x = savedX;
                            for (int nudge = 0; nudge < TILE_SIZE; nudge++) {
                                p->x += (float)p->facing;
                                cx = (int)p->x + RENDER_W / 2;
                                if (tile_solid(cx, fy)) break;
                            }
                        }
                    }
                }
                p->onGround = 1;
                set_state(p, STATE_IDLE);
            }
            break;

        default:
            set_state(p, STATE_IDLE);
            break;
        }
    }

    /* ── Air states ─────────────────────────────────────────────────
     * Handles variable-height jumping, air control, state transitions
     * (rise→fall), wall-slide detection, and corner-grab while falling. */
    if (!p->onGround) {
        /* Variable jump height: holding UP applies a small upward boost
         * each frame, up to JUMP_HOLD_MAX frames.  This lets players
         * tap for a short hop or hold for a full-height jump. */
        if ((p->state == STATE_JUMP_RISE || p->state == STATE_SOMERSAULT) &&
            keyUp && p->jumpHoldFrames < JUMP_HOLD_MAX) {
            p->vy -= JUMP_HOLD_BOOST;
            p->jumpHoldFrames++;
        }

        /* Air control */
        if (inputDir != 0)
            physics_apply_accel(&p->vx, AIR_CONTROL, inputDir);
        physics_apply_friction(&p->vx, AIR_FRICTION);

        /* Transition from rise to fall when vertical velocity becomes
         * non-negative (apex of jump arc reached) */
        if (p->state == STATE_JUMP_RISE && p->vy >= 0)
            set_state(p, STATE_JUMP_FALL);
        if (p->state == STATE_SOMERSAULT && p->vy >= 0)
            set_state(p, STATE_JUMP_FALL);

        /* Wall-slide detection (pre-collision probe):
         * Two triggers exist for wall-slide — this is the FIRST.
         * It catches cases where the character is already adjacent to a
         * wall from a previous frame (no fresh collision event needed).
         * The SECOND trigger (post-collision, below) catches the frame
         * where we first collide with a wall. */
        /* NOTE: Primary wall-slide trigger is AFTER physics_collide_horizontal
         * (see below). This pre-check catches cases where the character is
         * already adjacent to a wall from a previous frame. */
        if (p->state == STATE_JUMP_FALL && inputDir == p->facing &&
            p->vy > 1.0f && tile_wall_beside(p->x, p->y, p->facing)) {
            game_log("WALL-SLIDE triggered (pre-collision probe)");
            set_state(p, STATE_WALL_SLIDE);
        }

        /* Wall-slide mechanics */
        if (p->state == STATE_WALL_SLIDE) {
            p->vy *= 0.6f;  /* slow descent */
            if (p->vy > 2.0f) p->vy = 2.0f;  /* cap slide speed */
            if (inputDir != p->facing) {
                /* Released wall → fall */
                set_state(p, STATE_JUMP_FALL);
            }
            if (keyUpJustPressed) {
                /* Wall-jump: push away from wall */
                p->facing = -p->facing;
                p->vx = MAX_RUN_SPEED * 0.6f * (float)p->facing;
                p->vy = JUMP_IMPULSE * 0.85f;
                p->jumpHoldFrames = 0;
                set_state(p, STATE_JUMP_RISE);
            }
        }

        /* Corner-grab: detect a ledge while falling past it.
         * grabCooldown prevents oscillation: without it, releasing a grab
         * would immediately re-detect the same ledge on the next frame. */
        if ((p->state == STATE_JUMP_FALL || p->state == STATE_WALL_SLIDE) &&
            p->vy > 0 && p->grabCooldown == 0) {
            float grabY;
            if (tile_ledge_nearby(p->x, p->y, p->facing, &grabY)) {
                game_log("CORNER-GRAB at grabY=%.1f (was y=%.1f vy=%.2f)",
                         grabY, p->y, p->vy);
                p->y = grabY;
                p->vy = 0;
                p->vx = 0;
                p->onGround = 1;  /* treat as grounded for state machine */
                set_state(p, STATE_CORNER_GRAB);
            }
        }
    }

    /* ── Gravity (skip if hanging) ── */
    if (p->state != STATE_CORNER_GRAB && p->state != STATE_CORNER_CLIMB)
        gravity_apply(&p->vy);

    /* ── Collision resolution ── */
    int hitWall = physics_collide_horizontal(&p->x, &p->vx, p->y);

    /* ── Auto-step-up: foot-level barrier (1 tile high) ──
     * When grounded and walking into a single-tile barrier, automatically
     * step up onto it without requiring a jump. Checks:
     * 1. The barrier is only 1 tile tall (solid at foot row, air one row up)
     * 2. Enough vertical clearance above the step for the full body */
    if (hitWall && p->onGround && inputDir == p->facing &&
        p->stepCooldown == 0 &&
        (p->state == STATE_WALK || p->state == STATE_RUNNING)) {
        /* Probe one tile ahead of the leading body edge into the wall */
        int probeX;
        if (inputDir > 0)
            probeX = (int)p->x + RENDER_W - 9 + TILE_SIZE / 2;
        else
            probeX = (int)p->x + 8 - TILE_SIZE / 2;

        int footRow = ((int)p->y + RENDER_H - 2) / TILE_SIZE;
        int aboveRow = footRow - 1;

        /* Foot row is solid but the row above is air → 1-tile step */
        if (tile_solid(probeX, footRow * TILE_SIZE + TILE_SIZE / 2) &&
            !tile_solid(probeX, aboveRow * TILE_SIZE + TILE_SIZE / 2)) {
            /* Verify clearance: full body height above the step */
            float stepY = (float)(footRow * TILE_SIZE) - RENDER_H;
            int clearance = 1;
            for (int h = 4; h < RENDER_H - 2; h += 16) {
                if (tile_solid(probeX, (int)stepY + h)) {
                    clearance = 0;
                    break;
                }
            }
            if (clearance) {
                /* Position character centered on the step tile */
                int stepCol = probeX / TILE_SIZE;
                p->y = stepY;
                p->x = (float)(stepCol * TILE_SIZE) - RENDER_W / 2.0f + TILE_SIZE / 2.0f;
                p->vy = 0;
                p->vx = 0;
                p->onGround = 1;
                p->stepCooldown = 20;  /* pause before next step-up */
                set_state(p, STATE_IDLE);
                game_log("AUTO-STEP-UP at y=%.1f x=%.1f (step row=%d col=%d)",
                         p->y, p->x, footRow, stepCol);
            }
        }
    }

    /* Hard clamp to level bounds — prevents any escape regardless of
     * velocity or collision probe gaps at the border */
    if (p->x < (float)TILE_SIZE) {
        p->x = (float)TILE_SIZE;
        p->vx = 0;
    }
    if (p->x > (float)((LEVEL_COLS - 1) * TILE_SIZE - RENDER_W)) {
        p->x = (float)((LEVEL_COLS - 1) * TILE_SIZE - RENDER_W);
        p->vx = 0;
    }
    if (p->y < (float)TILE_SIZE) {
        p->y = (float)TILE_SIZE;
        p->vy = 0;
    }

    /* Post-collision wall-slide: if we hit a wall while airborne & falling,
     * and the player is pressing into the wall, trigger wall-slide.
     * This is more reliable than the pre-collision probe because the
     * character is now pressed right against the wall. */
    if (hitWall && !p->onGround && p->state == STATE_JUMP_FALL &&
        inputDir != 0 && p->vy > 0.5f) {
        p->facing = inputDir;  /* face the wall */
        game_log("WALL-SLIDE triggered (post-collision, hitWall=%d inputDir=%d vy=%.2f)",
                 hitWall, inputDir, p->vy);
        set_state(p, STATE_WALL_SLIDE);
    }

    /* PoP-style ground ledge grab: walking/running into a structure at
     * head height → reach up and grab its top edge. The grab target must
     * be a wall that also blocks at mid-body (prevents jailbreaking from
     * enclosed spaces by grabbing thin ceiling tiles). */
    if (p->onGround && inputDir == p->facing &&
        p->grabCooldown == 0 &&
        (p->state == STATE_WALK || p->state == STATE_RUNNING ||
         p->state == STATE_STOPPING)) {
        float grabY;
        if (tile_head_ledge(p->x, p->y, p->facing, &grabY)) {
            game_log("HEAD-LEDGE GRAB at grabY=%.1f (was y=%.1f, grounded, walking into wall)",
                     grabY, p->y);
            p->y = grabY;
            p->vy = 0;
            p->vx = 0;
            p->onGround = 1;
            set_state(p, STATE_CORNER_GRAB);
        }
    }

    if (p->state != STATE_CORNER_GRAB && p->state != STATE_CORNER_CLIMB) {
        int wasOnGround = p->onGround;
        physics_collide_vertical(&p->y, &p->vy, p->x,
                                 &p->onGround, &p->landingVy);

        if (!wasOnGround && p->onGround) {
            /* Landing tier system — classifies impact by fall velocity:
             *   tier 0 (soft):   instant recovery → idle or running
             *   tier 1 (medium): brief stagger (LANDING_TICKS) → idle
             *   tier 2 (hard):   ninja roll with forward momentum → crouch */
            int tier = gravity_landing_tier(p->landingVy);
            float estHeight = gravity_fall_height_estimate(p->landingVy);
            game_log("LANDED: landingVy=%.2f tier=%d estFallPx=%.0f vx=%.2f",
                     p->landingVy, tier, estHeight, p->vx);
            if (tier == 2) {
                /* Big fall — ninja roll to break the fall */
                p->vx = (float)p->facing * MAX_RUN_SPEED * 0.5f;
                set_state(p, STATE_HARD_LANDING);
            } else if (tier == 1) {
                set_state(p, STATE_LANDING);
            } else {
                set_state(p, absf(p->vx) > STOP_THRESHOLD ?
                          STATE_RUNNING : STATE_IDLE);
            }
        }
    }

    /* Edge overhang is now enforced by the vertical collision itself
     * (landing requires center foot on solid ground), so no separate
     * edge-fall check is needed here.  Removed to prevent oscillation
     * between landing and edge-fall on every frame. */

    /* ── Fell off a ledge ──
     * If the character is airborne but in a ground-only state (e.g.
     * walked off an edge), force transition to JUMP_FALL.  Many states
     * are excluded: air states (already handled), crouch (might be
     * mid-jump-startup), and grab/climb (deliberately hanging). */
    if (!p->onGround && p->state != STATE_JUMP_RISE &&
        p->state != STATE_JUMP_FALL && p->state != STATE_CROUCH &&
        p->state != STATE_SOMERSAULT && p->state != STATE_WALL_SLIDE &&
        p->state != STATE_CORNER_GRAB && p->state != STATE_CORNER_CLIMB) {
        /* Walked/ran off edge — brief grab cooldown so we don't
         * instantly re-grab the ledge we just left */
        p->grabCooldown = 15;
        set_state(p, STATE_JUMP_FALL);
    }

    /* ── Advance animation ──
     * Tick-based system: animTick counts up each frame.  When it reaches
     * the animation's 'speed' value, the frame advances.  On the last
     * frame, looping anims restart at 0; non-looping anims hold the
     * final frame (used for one-shot sequences like landing). */
    {
        const SpriteAnim *anim = &anims[p->state];
        if (anim->count > 0) {
            p->animTick++;
            if (p->animTick >= anim->speed) {
                p->animTick = 0;
                if (p->animFrame < anim->count - 1) {
                    p->animFrame++;
                } else if (anim->loop) {
                    p->animFrame = 0;
                }
            }
            if (p->animFrame >= anim->count)
                p->animFrame = anim->count - 1;
        }
    }
}

/*
 * character_draw — Render the character's current animation frame.
 *
 * The sprite is centered horizontally on the collision bounding box and
 * bottom-aligned (feet touch the bottom of the bbox).  This is because
 * the sprite art (50×37 px) is larger than the collision box (RENDER_W ×
 * RENDER_H), so centering prevents visual jitter during state changes
 * while bottom-alignment keeps feet planted on the ground.
 */
void character_draw(const Character *p) {
    const SpriteAnim *anim = &anims[p->state];
    if (anim->count == 0) return;

    int frame = p->animFrame;
    if (frame >= anim->count) frame = anim->count - 1;

    const SpriteFrame *sf = &anim->frames[frame];

    int drawW = sf->w * SPRITE_DRAW_SCALE;
    int drawH = sf->h * SPRITE_DRAW_SCALE;

    /* Center sprite horizontally on character position */
    int drawX = (int)p->x + (RENDER_W - drawW) / 2;
    /* Align bottom of sprite with bottom of bounding box */
    int drawY = (int)p->y + RENDER_H - drawH;

    int flipH = (p->facing < 0) ? 1 : 0;

    sprite_blit(sf, drawX, drawY, SPRITE_DRAW_SCALE, flipH);
}
