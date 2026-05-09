/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * levelfile.c — Parser, validator, and writer for .parcour level files
 *
 * Format specification:
 *   - Lines starting with '#' are comments (ignored)
 *   - Required fields: version, size, spawn, map...end
 *   - Optional fields: name, author
 *   - Tile chars: '.' = air (0), '#' = solid (1)
 *   - Size is fixed at 40x30 (LEVEL_COLS x LEVEL_ROWS)
 *
 * Depends on: types.h (for LEVEL_COLS, LEVEL_ROWS, TILE_SIZE)
 */

#include "levelfile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Helpers ────────────────────────────────────────────────────────── */

/* Skip leading whitespace, return pointer to first non-space char */
static const char *skip_spaces(const char *s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

/* Check if line starts with a given prefix (after skipping spaces) */
static int starts_with(const char *line, const char *prefix) {
    line = skip_spaces(line);
    return strncmp(line, prefix, strlen(prefix)) == 0;
}

/* Copy a string value from "key value" line into dst (up to maxLen-1 chars) */
static void parse_string_value(const char *line, const char *key, char *dst, int maxLen) {
    const char *p = skip_spaces(line) + strlen(key);
    p = skip_spaces(p);
    int i = 0;
    while (*p && *p != '\n' && *p != '\r' && i < maxLen - 1) {
        dst[i++] = *p++;
    }
    dst[i] = '\0';
}

/* ── Core parser (works on in-memory text) ──────────────────────────── */

LevelError levelfile_load_from_memory(const char *text, int textLen, LevelData *out) {
    if (!text || !out) return LEVEL_ERR_FILE_READ;

    /* Zero-initialize output */
    memset(out, 0, sizeof(LevelData));
    strncpy_s(out->name, sizeof(out->name), "Untitled", _TRUNCATE);
    strncpy_s(out->author, sizeof(out->author), "Unknown", _TRUNCATE);
    out->spawn_col = -1;
    out->spawn_row = -1;

    int gotVersion = 0;
    int gotSize = 0;
    int gotSpawn = 0;
    int gotMap = 0;
    int mapRow = 0;
    int inMap = 0;
    int sizeW = 0, sizeH = 0;

    /* Process line by line */
    const char *pos = text;
    const char *end = text + textLen;
    char line[256];

    while (pos < end) {
        /* Extract one line */
        int lineLen = 0;
        while (pos + lineLen < end && pos[lineLen] != '\n' && pos[lineLen] != '\r') {
            lineLen++;
        }
        if (lineLen >= (int)sizeof(line)) lineLen = (int)sizeof(line) - 1;
        memcpy(line, pos, lineLen);
        line[lineLen] = '\0';

        /* Advance past line + newline chars */
        pos += lineLen;
        if (pos < end && *pos == '\r') pos++;
        if (pos < end && *pos == '\n') pos++;

        /* Skip empty lines and comments (but NOT inside map block) */
        const char *trimmed = skip_spaces(line);
        if (!inMap && (*trimmed == '\0' || *trimmed == '#')) continue;
        if (inMap && *trimmed == '\0') continue;

        /* Inside map block */
        if (inMap) {
            if (starts_with(line, "end")) {
                inMap = 0;
                gotMap = 1;
                continue;
            }
            if (mapRow >= LEVEL_ROWS) {
                return LEVEL_ERR_MAP_ROW_COUNT;
            }
            /* Parse tile row — must be exactly LEVEL_COLS chars */
            int col = 0;
            const char *p = line;
            while (*p && *p != '\n' && *p != '\r' && col < LEVEL_COLS) {
                if (*p == TILE_CHAR_AIR) {
                    out->tiles[mapRow][col] = 0;
                } else if (*p == TILE_CHAR_SOLID) {
                    out->tiles[mapRow][col] = 1;
                } else {
                    return LEVEL_ERR_MAP_BAD_CHAR;
                }
                col++;
                p++;
            }
            if (col != LEVEL_COLS) {
                return LEVEL_ERR_MAP_COL_COUNT;
            }
            mapRow++;
            continue;
        }

        /* Header fields */
        if (starts_with(line, "version ")) {
            int ver = 0;
            if (sscanf_s(trimmed, "version %d", &ver) == 1) {
                if (ver != LEVELFILE_VERSION) return LEVEL_ERR_BAD_VERSION;
                gotVersion = 1;
            }
        } else if (starts_with(line, "name ")) {
            parse_string_value(line, "name", out->name, LEVELFILE_MAX_NAME);
        } else if (starts_with(line, "author ")) {
            parse_string_value(line, "author", out->author, LEVELFILE_MAX_AUTHOR);
        } else if (starts_with(line, "size ")) {
            if (sscanf_s(trimmed, "size %d %d", &sizeW, &sizeH) == 2) {
                if (sizeW != LEVEL_COLS || sizeH != LEVEL_ROWS) {
                    return LEVEL_ERR_BAD_SIZE;
                }
                gotSize = 1;
            }
        } else if (starts_with(line, "spawn ")) {
            int sx = 0, sy = 0;
            if (sscanf_s(trimmed, "spawn %d %d", &sx, &sy) == 2) {
                out->spawn_col = sx;
                out->spawn_row = sy;
                gotSpawn = 1;
            }
        } else if (starts_with(line, "map")) {
            inMap = 1;
            mapRow = 0;
        }
    }

    /* Check required fields */
    if (!gotVersion) return LEVEL_ERR_NO_VERSION;
    if (!gotSize) return LEVEL_ERR_NO_SIZE;
    if (!gotSpawn) return LEVEL_ERR_NO_SPAWN;
    if (!gotMap) return LEVEL_ERR_NO_MAP;
    if (mapRow != LEVEL_ROWS) return LEVEL_ERR_MAP_ROW_COUNT;

    /* Run validation */
    return levelfile_validate(out);
}

/* ── File-based loader ──────────────────────────────────────────────── */

LevelError levelfile_load(const char *path, LevelData *out) {
    if (!path || !out) return LEVEL_ERR_FILE_NOT_FOUND;

    FILE *f = NULL;
    if (fopen_s(&f, path, "rb") != 0 || !f) {
        return LEVEL_ERR_FILE_NOT_FOUND;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0 || size > 1024 * 1024) {
        fclose(f);
        return LEVEL_ERR_FILE_READ;
    }

    char *buf = (char *)malloc(size + 1);
    if (!buf) {
        fclose(f);
        return LEVEL_ERR_FILE_READ;
    }

    size_t bytesRead = fread(buf, 1, size, f);
    fclose(f);
    buf[bytesRead] = '\0';

    LevelError err = levelfile_load_from_memory(buf, (int)bytesRead, out);
    free(buf);
    return err;
}

/* ── Validator ──────────────────────────────────────────────────────── */

LevelError levelfile_validate(const LevelData *data) {
    if (!data) return LEVEL_ERR_FILE_READ;

    /* Check spawn is within bounds */
    if (data->spawn_col < 0 || data->spawn_col >= LEVEL_COLS ||
        data->spawn_row < 0 || data->spawn_row >= LEVEL_ROWS) {
        return LEVEL_ERR_SPAWN_OUT_OF_BOUNDS;
    }

    /* Check spawn tile is air (not inside a wall) — checks the full
     * character bounding box: 2 tiles wide × 4 tiles tall above spawn.
     * The character's feet are on spawn_row, body extends upward. */
    {
        int charW = RENDER_W / TILE_SIZE;  /* 2 */
        int charH = RENDER_H / TILE_SIZE;  /* 4 */
        for (int dy = 0; dy < charH; dy++) {
            int r = data->spawn_row - dy;
            if (r < 0) return LEVEL_ERR_SPAWN_OUT_OF_BOUNDS;
            for (int dx = 0; dx < charW; dx++) {
                int c = data->spawn_col + dx;
                if (c >= LEVEL_COLS) return LEVEL_ERR_SPAWN_OUT_OF_BOUNDS;
                if (data->tiles[r][c] != 0)
                    return LEVEL_ERR_SPAWN_IN_SOLID;
            }
        }
    }

    /* Check spawn has solid ground below (within 3 tiles) */
    {
        int hasGround = 0;
        for (int r = data->spawn_row + 1; r < LEVEL_ROWS && r <= data->spawn_row + 3; r++) {
            if (data->tiles[r][data->spawn_col] == 1) {
                hasGround = 1;
                break;
            }
        }
        if (!hasGround) return LEVEL_ERR_SPAWN_NO_GROUND;
    }

    /* Check border is solid */
    for (int col = 0; col < LEVEL_COLS; col++) {
        if (data->tiles[0][col] != 1) return LEVEL_ERR_BORDER_NOT_SOLID;
        if (data->tiles[LEVEL_ROWS - 1][col] != 1) return LEVEL_ERR_BORDER_NOT_SOLID;
    }
    for (int row = 0; row < LEVEL_ROWS; row++) {
        if (data->tiles[row][0] != 1) return LEVEL_ERR_BORDER_NOT_SOLID;
        if (data->tiles[row][LEVEL_COLS - 1] != 1) return LEVEL_ERR_BORDER_NOT_SOLID;
    }

    return LEVEL_OK;
}

/* ── Writer ─────────────────────────────────────────────────────────── */

LevelError levelfile_save(const char *path, const LevelData *data) {
    if (!path || !data) return LEVEL_ERR_WRITE_FAILED;

    /* Validate before saving */
    LevelError err = levelfile_validate(data);
    if (err != LEVEL_OK) return err;

    FILE *f = NULL;
    if (fopen_s(&f, path, "w") != 0 || !f) {
        return LEVEL_ERR_WRITE_FAILED;
    }

    fprintf(f, "# Parcour Level File\n");
    fprintf(f, "version %d\n", LEVELFILE_VERSION);
    fprintf(f, "name %s\n", data->name);
    fprintf(f, "author %s\n", data->author);
    fprintf(f, "size %d %d\n", LEVEL_COLS, LEVEL_ROWS);
    fprintf(f, "spawn %d %d\n", data->spawn_col, data->spawn_row);
    fprintf(f, "\nmap\n");

    for (int row = 0; row < LEVEL_ROWS; row++) {
        for (int col = 0; col < LEVEL_COLS; col++) {
            fputc(data->tiles[row][col] == 1 ? TILE_CHAR_SOLID : TILE_CHAR_AIR, f);
        }
        fputc('\n', f);
    }

    fprintf(f, "end\n");
    fclose(f);
    return LEVEL_OK;
}

/* ── Clearance check ────────────────────────────────────────────────── */

/* Character footprint in tiles */
#define CHAR_TILES_W  (RENDER_W / TILE_SIZE)   /* 2 */
#define CHAR_TILES_H  (RENDER_H / TILE_SIZE)   /* 4 */

int levelfile_check_clearance(const LevelData *data, int warnings[LEVEL_ROWS][LEVEL_COLS]) {
    if (!data || !warnings) return 0;

    /* Determine which tiles are part of a valid character-sized rectangle.
     * A character can stand with bottom-left at (row, col) if tiles
     * [row-(H-1) .. row][col .. col+(W-1)] are all air. */
    int reachable[LEVEL_ROWS][LEVEL_COLS];
    memset(reachable, 0, sizeof(reachable));

    for (int row = CHAR_TILES_H - 1; row < LEVEL_ROWS; row++) {
        for (int col = 0; col <= LEVEL_COLS - CHAR_TILES_W; col++) {
            int fits = 1;
            for (int dy = 0; dy < CHAR_TILES_H && fits; dy++) {
                for (int dx = 0; dx < CHAR_TILES_W && fits; dx++) {
                    if (data->tiles[row - dy][col + dx] != 0) {
                        fits = 0;
                    }
                }
            }
            if (fits) {
                for (int dy = 0; dy < CHAR_TILES_H; dy++) {
                    for (int dx = 0; dx < CHAR_TILES_W; dx++) {
                        reachable[row - dy][col + dx] = 1;
                    }
                }
            }
        }
    }

    /* Flag unreachable air tiles */
    int count = 0;
    for (int row = 0; row < LEVEL_ROWS; row++) {
        for (int col = 0; col < LEVEL_COLS; col++) {
            if (data->tiles[row][col] == 0 && !reachable[row][col]) {
                warnings[row][col] = 1;
                count++;
            } else {
                warnings[row][col] = 0;
            }
        }
    }
    return count;
}

/* ── Enclosure check ────────────────────────────────────────────────── */

int levelfile_check_enclosure(const LevelData *data) {
    if (!data) return 0;

    /* Flood-fill from the spawn tile through 4-connected air tiles.
     * If the fill reaches row 1 (interior top), spawn is NOT enclosed. */
    int visited[LEVEL_ROWS][LEVEL_COLS];
    memset(visited, 0, sizeof(visited));

    /* BFS queue — max possible entries is all tiles */
    typedef struct { int r, c; } Pos;
    Pos queue[LEVEL_ROWS * LEVEL_COLS];
    int head = 0, tail = 0;

    int sr = data->spawn_row;
    int sc = data->spawn_col;
    if (data->tiles[sr][sc] != 0) return 0;  /* spawn in solid — validator catches this */

    visited[sr][sc] = 1;
    queue[tail++] = (Pos){sr, sc};

    static const int dr[] = {-1, 1, 0, 0};
    static const int dc[] = {0, 0, -1, 1};

    while (head < tail) {
        Pos cur = queue[head++];

        /* If we reached top interior row, spawn is open */
        if (cur.r == 1)
            return 0;

        for (int i = 0; i < 4; i++) {
            int nr = cur.r + dr[i];
            int nc = cur.c + dc[i];
            if (nr < 0 || nr >= LEVEL_ROWS || nc < 0 || nc >= LEVEL_COLS)
                continue;
            if (visited[nr][nc] || data->tiles[nr][nc] != 0)
                continue;
            visited[nr][nc] = 1;
            queue[tail++] = (Pos){nr, nc};
        }
    }

    /* Fill never reached row 1 — spawn area is enclosed */
    return 1;
}

/* ── Error strings ──────────────────────────────────────────────────── */

const char *levelfile_error_string(LevelError err) {
    switch (err) {
    case LEVEL_OK:                   return "OK";
    case LEVEL_ERR_FILE_NOT_FOUND:   return "File not found";
    case LEVEL_ERR_FILE_READ:        return "Could not read file";
    case LEVEL_ERR_NO_VERSION:       return "Missing 'version' field";
    case LEVEL_ERR_BAD_VERSION:      return "Unsupported version (expected 1)";
    case LEVEL_ERR_NO_SIZE:          return "Missing 'size' field";
    case LEVEL_ERR_BAD_SIZE:         return "Invalid size (must be 40 30)";
    case LEVEL_ERR_NO_SPAWN:         return "Missing 'spawn' field";
    case LEVEL_ERR_SPAWN_OUT_OF_BOUNDS: return "Spawn position out of bounds";
    case LEVEL_ERR_SPAWN_IN_SOLID:   return "Spawn position is inside a solid tile";
    case LEVEL_ERR_SPAWN_NO_GROUND:  return "Spawn has no ground below (within 3 tiles)";
    case LEVEL_ERR_NO_MAP:           return "Missing 'map' block";
    case LEVEL_ERR_MAP_ROW_COUNT:    return "Map has wrong number of rows (expected 30)";
    case LEVEL_ERR_MAP_COL_COUNT:    return "Map row has wrong number of columns (expected 40)";
    case LEVEL_ERR_MAP_BAD_CHAR:     return "Map contains invalid character (use '.' or '#')";
    case LEVEL_ERR_BORDER_NOT_SOLID: return "Level border must be solid (all edges '#')";
    case LEVEL_ERR_WRITE_FAILED:     return "Could not write file";
    }
    return "Unknown error";
}
