/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * levelfile.h — Parse, validate, and save .parcour level files
 *
 * Part of the engine library. Used by both the game and the editor.
 */
#ifndef LEVELFILE_H
#define LEVELFILE_H

#include "types.h"

/* ── Constants ──────────────────────────────────────────────────────── */
#define LEVELFILE_VERSION       1
#define LEVELFILE_MAX_NAME     64
#define LEVELFILE_MAX_AUTHOR   64

/* Tile characters in the file format */
#define TILE_CHAR_AIR    '.'
#define TILE_CHAR_SOLID  '#'

/* ── Error codes ────────────────────────────────────────────────────── */
typedef enum {
    LEVEL_OK = 0,
    LEVEL_ERR_FILE_NOT_FOUND,
    LEVEL_ERR_FILE_READ,
    LEVEL_ERR_NO_VERSION,
    LEVEL_ERR_BAD_VERSION,
    LEVEL_ERR_NO_SIZE,
    LEVEL_ERR_BAD_SIZE,
    LEVEL_ERR_NO_SPAWN,
    LEVEL_ERR_SPAWN_OUT_OF_BOUNDS,
    LEVEL_ERR_SPAWN_IN_SOLID,
    LEVEL_ERR_SPAWN_NO_GROUND,
    LEVEL_ERR_NO_MAP,
    LEVEL_ERR_MAP_ROW_COUNT,
    LEVEL_ERR_MAP_COL_COUNT,
    LEVEL_ERR_MAP_BAD_CHAR,
    LEVEL_ERR_BORDER_NOT_SOLID,
    LEVEL_ERR_WRITE_FAILED
} LevelError;

/* ── Parsed level data ──────────────────────────────────────────────── */
typedef struct {
    char name[LEVELFILE_MAX_NAME];
    char author[LEVELFILE_MAX_AUTHOR];
    int  spawn_col;
    int  spawn_row;
    int  tiles[LEVEL_ROWS][LEVEL_COLS];   /* 0=air, 1=solid */
} LevelData;

/* ── API ────────────────────────────────────────────────────────────── */

/* Parse a .parcour file from disk. Returns LEVEL_OK on success. */
LevelError levelfile_load(const char *path, LevelData *out);

/* Parse a .parcour file from an in-memory string buffer. */
LevelError levelfile_load_from_memory(const char *text, int textLen, LevelData *out);

/* Validate a LevelData struct (checks border, spawn, etc.).
 * Called automatically by levelfile_load, but exposed for editor use. */
LevelError levelfile_validate(const LevelData *data);

/* Save a LevelData struct to a .parcour file on disk. */
LevelError levelfile_save(const char *path, const LevelData *data);

/* Check for air tiles too tight for the character (2 tiles wide × 4 tall).
 * Writes 1 into warnings[row][col] for each unreachable air tile.
 * Returns the total number of warning tiles. */
int levelfile_check_clearance(const LevelData *data, int warnings[LEVEL_ROWS][LEVEL_COLS]);

/* Check if spawn is fully enclosed. Flood-fills air tiles from the spawn
 * position — if the fill reaches row 1 (top interior border), the spawn
 * area is open to the rest of the level (not sealed).
 * Returns 1 if spawn is enclosed, 0 if spawn area connects to the top. */
int levelfile_check_enclosure(const LevelData *data);

/* Get a human-readable error string for a LevelError code. */
const char *levelfile_error_string(LevelError err);

#endif /* LEVELFILE_H */
