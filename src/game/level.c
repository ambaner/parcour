/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * level.c — Tile map data, collision, and tile rendering
 *
 * Depends on: types.h, renderer.h
 */

#include "level.h"
#include "renderer.h"

/* ── Tile map ───────────────────────────────────────────────────────── */
/*
 * Large playground level (40 cols × 30 rows).
 *
 * Level design layout:
 *   LEFT SIDE:    Tall tower with cliff drop (tests hard landing / ninja roll)
 *   CENTER:       Staircase of floating platforms (tests precise jumping)
 *   RIGHT SIDE:   Ledge chain (4 ledges spaced every 4 rows — tests
 *                 corner-grab, climb-up, and wall-slide sequences)
 *   BOTTOM:       Split ground with a gap at center (tests ledge walk-off)
 *
 * Legend:
 *   W = boundary wall (col 0 & 39)   T = tower (cols 0–5, full height)
 *   P = floating platform             C = right-side ledge column
 *
 * Row 0 and row 29 are solid to form ceiling and floor.
 * Rows 27–28 are open below the ground platforms to allow sub-level space.
 */
int level[LEVEL_ROWS][LEVEL_COLS] = {
/*          0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 */
/* row 0 */ {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
/* row 1 */ {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
/* row 2 */ {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
/* row 3 */ {1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,1},  /* tower top + right high */
/* row 4 */ {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1},
/* row 5 */ {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1},
/* row 6 */ {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1},
/* row 7 */ {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,1},  /* right mid-high */
/* row 8 */ {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
/* row 9 */ {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
/* row10 */ {1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},  /* center high platform */
/* row11 */ {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,1},  /* right mid */
/* row12 */ {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
/* row13 */ {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
/* row14 */ {1,1,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},  /* center mid platform */
/* row15 */ {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,1},  /* right mid-low */
/* row16 */ {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
/* row17 */ {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
/* row18 */ {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},  /* center low platform */
/* row19 */ {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,1},  /* right low */
/* row20 */ {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
/* row21 */ {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
/* row22 */ {1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},  /* lower center platform */
/* row23 */ {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,1},  /* right ground-level */
/* row24 */ {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
/* row25 */ {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
/* row26 */ {1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1},  /* ground platforms */
/* row27 */ {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
/* row28 */ {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
/* row29 */ {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},  /* bottom ground */
};

/*
 * tile_solid — Is the tile at pixel position (px, py) solid?
 *
 * Returns 1 if the tile is solid OR if the position is out of bounds.
 * Treating out-of-bounds as solid makes the level edges act as invisible
 * walls, so the character can never escape the grid without explicit
 * boundary checks at every call site.
 */
int tile_solid(int px, int py) {
    int col = px / TILE_SIZE;
    int row = py / TILE_SIZE;
    if (col < 0 || col >= LEVEL_COLS || row < 0 || row >= LEVEL_ROWS)
        return 1;
    return level[row][col] == 1;
}

/*
 * tile_wall — Is this a "wall" tile?
 *
 * A wall is a solid tile that is part of a vertically continuous
 * structure (at least 2 tiles tall).  Single-row platforms (thin
 * floors) return 0 — they only block vertically (landing), not
 * horizontally (walking / wall-slide).
 */
int tile_wall(int px, int py) {
    if (!tile_solid(px, py)) return 0;
    return tile_solid(px, py - TILE_SIZE) || tile_solid(px, py + TILE_SIZE);
}

/*
 * tile_wall_beside — Is there a wall adjacent to the character in the
 * facing direction?
 *
 * Checks at mid-body and lower-body heights to catch walls that only
 * partially overlap the character vertically.
 */
int tile_wall_beside(float x, float y, int facing) {
    /* Probe offsets: +4 pixels past the right edge, or -5 pixels before
     * the left edge.  The asymmetry accounts for the character's bounding
     * box origin being at the left edge (x is the left side). */
    int probeX = (facing > 0)
        ? (int)x + RENDER_W + 4
        : (int)x - 5;
    int midY  = (int)y + RENDER_H / 2;
    int lowY  = (int)y + RENDER_H * 3 / 4;
    return tile_wall(probeX, midY) || tile_wall(probeX, lowY);
}

/*
 * tile_ledge_nearby — Is there a grabbable ledge near the character's
 * top while airborne?
 *
 * A ledge is defined as a solid tile with air directly above it — the
 * top surface of a wall or platform.  The scan window covers the
 * character's top quarter to top third (the "hand" zone), stepping
 * in half-tile increments for sufficient resolution.
 *
 * Returns 1 and writes the snap Y position to *grabY.
 */
int tile_ledge_nearby(float x, float y, int facing, float *grabY) {
    /* Probe 2–3 pixels past the bounding box edge in the facing direction */
    int probeX = (facing > 0)
        ? (int)x + RENDER_W + 2
        : (int)x - 3;

    /* Scan a window from the character's top to 1/3 of the way down,
     * checking every half-tile for a solid-below + air-above transition */
    int topY = (int)y;
    int scanEnd = topY + RENDER_H / 3;
    for (int py = topY; py <= scanEnd; py += TILE_SIZE / 2) {
        if (tile_solid(probeX, py) && !tile_solid(probeX, py - TILE_SIZE)) {
            /* Found a ledge — snap character so hands are at ledge top */
            int ledgeRow = py / TILE_SIZE;
            *grabY = (float)(ledgeRow * TILE_SIZE - RENDER_H / 4);
            return 1;
        }
    }
    return 0;
}

/*
 * tile_head_ledge — Is there a platform edge at head height that a
 * grounded character can reach up and grab?
 *
 * Unlike tile_ledge_nearby (used in air), this checks a single point
 * at the character's head level for a grab-while-walking interaction.
 * The ledge must be solid at head level with air above (a top edge).
 *
 * Returns 1 and sets *grabY for the snap position.
 */
int tile_head_ledge(float x, float y, int facing, float *grabY) {
    /* Probe just past the bounding box in the facing direction */
    int probeX = (facing > 0)
        ? (int)x + RENDER_W + 2
        : (int)x - 3;

    /* Check at head height — top quarter of the character bounding box */
    int headY = (int)y + RENDER_H / 4;

    if (tile_solid(probeX, headY) && !tile_solid(probeX, headY - TILE_SIZE)) {
        int ledgeRow = headY / TILE_SIZE;
        *grabY = (float)(ledgeRow * TILE_SIZE - RENDER_H / 4);
        return 1;
    }
    return 0;
}

/*
 * draw_tile — Render a single tile with a pseudo-3D stone effect.
 *
 * Visual approach:
 *   - Top and left edges: lighter gray (0x808080) → simulates light
 *   - Bottom and right edges: darker gray (0x404040) → simulates shadow
 *   - Interior: mid gray (0x606060) with an XOR dither pattern
 *
 * The dither pattern ((x ^ y) & 3) == 0 creates a subtle stone texture
 * by brightening every 4th pixel in a diagonal pattern (adds 0x080808
 * per channel).  XOR ensures the pattern doesn't align in boring rows.
 */
static void draw_tile(int col, int row, int type) {
    int x0 = col * TILE_SIZE;
    int y0 = row * TILE_SIZE;
    UINT32 color;

    if (type == 1) {
        for (int y = 0; y < TILE_SIZE; y++) {
            for (int x = 0; x < TILE_SIZE; x++) {
                int sy = y0 + y, sx = x0 + x;
                if (sy >= SCREEN_H || sx >= SCREEN_W) continue;
                if (y == 0 || x == 0)
                    color = 0xFF808080;
                else if (y == TILE_SIZE - 1 || x == TILE_SIZE - 1)
                    color = 0xFF404040;
                else
                    color = 0xFF606060;
                if (((x ^ y) & 3) == 0)
                    color += 0x00080808;
                framebuf[sy * SCREEN_W + sx] = color;
            }
        }
    }
}

/* level_draw — Render all non-air tiles in the level grid. */
void level_draw(void) {
    for (int row = 0; row < LEVEL_ROWS; row++)
        for (int col = 0; col < LEVEL_COLS; col++)
            if (level[row][col] != 0)
                draw_tile(col, row, level[row][col]);
}
