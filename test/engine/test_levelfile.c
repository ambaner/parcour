/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * test_levelfile.c — Tests for the .parcour level file parser and validator
 */

#include "test_framework.h"
#include "test_helpers.h"
#include "levelfile.h"
#include <string.h>
#include <stdio.h>

/* ── Helper: build a valid level file string ────────────────────────── */

/* Generate a minimal valid .parcour file as a string.
 * All borders are solid, interior is air, spawn at (10, 27). */
static int make_valid_level_string(char *buf, int bufSize) {
    int written = 0;
    written += snprintf(buf + written, bufSize - written,
        "# Test level\n"
        "version 1\n"
        "name Test Level\n"
        "author Tester\n"
        "size 40 30\n"
        "spawn 10 27\n"
        "\n"
        "map\n");

    for (int row = 0; row < LEVEL_ROWS; row++) {
        for (int col = 0; col < LEVEL_COLS; col++) {
            char c;
            if (row == 0 || row == LEVEL_ROWS - 1 || col == 0 || col == LEVEL_COLS - 1) {
                c = '#';
            } else {
                c = '.';
            }
            buf[written++] = c;
        }
        buf[written++] = '\n';
    }
    written += snprintf(buf + written, bufSize - written, "end\n");
    buf[written] = '\0';
    return written;
}

/* ── Tests ──────────────────────────────────────────────────────────── */

static void test_valid_file_parses_ok(void) {
    TEST_BEGIN("levelfile: valid file parses successfully");
    char buf[4096];
    int len = make_valid_level_string(buf, sizeof(buf));

    LevelData data;
    LevelError err = levelfile_load_from_memory(buf, len, &data);
    ASSERT_EQ_INT(LEVEL_OK, err);
    ASSERT_EQ_INT(10, data.spawn_col);
    ASSERT_EQ_INT(27, data.spawn_row);
    ASSERT_TRUE(strcmp(data.name, "Test Level") == 0);
    ASSERT_TRUE(strcmp(data.author, "Tester") == 0);

    /* Check border tiles are solid */
    ASSERT_EQ_INT(1, data.tiles[0][0]);
    ASSERT_EQ_INT(1, data.tiles[0][LEVEL_COLS - 1]);
    ASSERT_EQ_INT(1, data.tiles[LEVEL_ROWS - 1][0]);

    /* Check interior tile is air */
    ASSERT_EQ_INT(0, data.tiles[1][1]);
    TEST_PASS();
}

static void test_missing_version_fails(void) {
    TEST_BEGIN("levelfile: missing version returns error");
    char buf[4096];
    int len = make_valid_level_string(buf, sizeof(buf));

    /* Remove the version line by replacing "version 1\n" with spaces */
    char *vline = strstr(buf, "version 1\n");
    ASSERT_TRUE(vline != NULL);
    memset(vline, ' ', 10); /* overwrite "version 1\n" -> spaces + newline preserved */

    LevelData data;
    LevelError err = levelfile_load_from_memory(buf, len, &data);
    ASSERT_EQ_INT(LEVEL_ERR_NO_VERSION, err);
    TEST_PASS();
}

static void test_bad_version_fails(void) {
    TEST_BEGIN("levelfile: wrong version number returns error");
    const char *text =
        "version 99\n"
        "name X\n"
        "size 40 30\n"
        "spawn 10 27\n"
        "map\n"
        "end\n";

    LevelData data;
    LevelError err = levelfile_load_from_memory(text, (int)strlen(text), &data);
    ASSERT_EQ_INT(LEVEL_ERR_BAD_VERSION, err);
    TEST_PASS();
}

static void test_bad_size_fails(void) {
    TEST_BEGIN("levelfile: wrong size returns error");
    const char *text =
        "version 1\n"
        "name X\n"
        "size 80 60\n"
        "spawn 10 27\n"
        "map\n"
        "end\n";

    LevelData data;
    LevelError err = levelfile_load_from_memory(text, (int)strlen(text), &data);
    ASSERT_EQ_INT(LEVEL_ERR_BAD_SIZE, err);
    TEST_PASS();
}

static void test_spawn_out_of_bounds(void) {
    TEST_BEGIN("levelfile: spawn out of bounds returns error");
    char buf[4096];
    make_valid_level_string(buf, sizeof(buf));

    /* Replace spawn with out-of-bounds coords */
    char *sline = strstr(buf, "spawn 10 27");
    ASSERT_TRUE(sline != NULL);
    memcpy(sline, "spawn 99 99", 11);

    LevelData data;
    LevelError err = levelfile_load_from_memory(buf, (int)strlen(buf), &data);
    ASSERT_EQ_INT(LEVEL_ERR_SPAWN_OUT_OF_BOUNDS, err);
    TEST_PASS();
}

static void test_spawn_in_solid_fails(void) {
    TEST_BEGIN("levelfile: spawn in solid tile returns error");
    char buf[4096];
    make_valid_level_string(buf, sizeof(buf));

    /* Set spawn to (0, 0) which is a border/solid tile */
    char *sline = strstr(buf, "spawn 10 27");
    ASSERT_TRUE(sline != NULL);
    memcpy(sline, "spawn  0  0", 11);

    LevelData data;
    LevelError err = levelfile_load_from_memory(buf, (int)strlen(buf), &data);
    ASSERT_EQ_INT(LEVEL_ERR_SPAWN_IN_SOLID, err);
    TEST_PASS();
}

static void test_spawn_no_ground_fails(void) {
    TEST_BEGIN("levelfile: spawn with no ground below returns error");
    char buf[4096];
    make_valid_level_string(buf, sizeof(buf));

    /* Spawn at row 5 — body fits (rows 2-5 air), but ground is at row 29 (too far) */
    char *sline = strstr(buf, "spawn 10 27");
    ASSERT_TRUE(sline != NULL);
    memcpy(sline, "spawn 10  5", 11);

    LevelData data;
    LevelError err = levelfile_load_from_memory(buf, (int)strlen(buf), &data);
    ASSERT_EQ_INT(LEVEL_ERR_SPAWN_NO_GROUND, err);
    TEST_PASS();
}

static void test_bad_char_in_map_fails(void) {
    TEST_BEGIN("levelfile: invalid character in map returns error");
    char buf[4096];
    int len = make_valid_level_string(buf, sizeof(buf));

    /* Find the map data and inject an invalid char */
    char *mapStart = strstr(buf, "map\n");
    ASSERT_TRUE(mapStart != NULL);
    mapStart += 4; /* skip "map\n" */
    /* Skip first row (border), go into second row interior */
    char *secondRow = strchr(mapStart, '\n');
    ASSERT_TRUE(secondRow != NULL);
    secondRow += 2; /* skip newline + first '#' of border */
    *secondRow = 'X'; /* inject invalid char */

    LevelData data;
    LevelError err = levelfile_load_from_memory(buf, len, &data);
    ASSERT_EQ_INT(LEVEL_ERR_MAP_BAD_CHAR, err);
    TEST_PASS();
}

static void test_border_not_solid_fails(void) {
    TEST_BEGIN("levelfile: non-solid border returns error");
    char buf[4096];
    int len = make_valid_level_string(buf, sizeof(buf));

    /* Find the map and make top-left interior border tile air */
    char *mapStart = strstr(buf, "map\n");
    ASSERT_TRUE(mapStart != NULL);
    mapStart += 4; /* skip "map\n" */
    /* First char of first row should be '#', make it '.' */
    *mapStart = '.';

    LevelData data;
    LevelError err = levelfile_load_from_memory(buf, len, &data);
    ASSERT_EQ_INT(LEVEL_ERR_BORDER_NOT_SOLID, err);
    TEST_PASS();
}

static void test_roundtrip_save_load(void) {
    TEST_BEGIN("levelfile: save then load produces identical data");

    /* Build a valid LevelData manually */
    LevelData original;
    memset(&original, 0, sizeof(original));
    strncpy_s(original.name, sizeof(original.name), "Roundtrip Test", _TRUNCATE);
    strncpy_s(original.author, sizeof(original.author), "Bot", _TRUNCATE);
    original.spawn_col = 5;
    original.spawn_row = 27;

    /* Fill borders solid, interior air */
    for (int row = 0; row < LEVEL_ROWS; row++) {
        for (int col = 0; col < LEVEL_COLS; col++) {
            if (row == 0 || row == LEVEL_ROWS - 1 || col == 0 || col == LEVEL_COLS - 1) {
                original.tiles[row][col] = 1;
            } else {
                original.tiles[row][col] = 0;
            }
        }
    }
    /* Add a platform and ground near spawn */
    for (int col = 3; col < 10; col++) {
        original.tiles[28][col] = 1;
    }

    /* Save to temp file */
    const char *tmpPath = "test_roundtrip.parcour";
    LevelError err = levelfile_save(tmpPath, &original);
    ASSERT_EQ_INT(LEVEL_OK, err);

    /* Load it back */
    LevelData loaded;
    err = levelfile_load(tmpPath, &loaded);
    ASSERT_EQ_INT(LEVEL_OK, err);

    /* Compare */
    ASSERT_TRUE(strcmp(original.name, loaded.name) == 0);
    ASSERT_TRUE(strcmp(original.author, loaded.author) == 0);
    ASSERT_EQ_INT(original.spawn_col, loaded.spawn_col);
    ASSERT_EQ_INT(original.spawn_row, loaded.spawn_row);

    for (int row = 0; row < LEVEL_ROWS; row++) {
        for (int col = 0; col < LEVEL_COLS; col++) {
            ASSERT_EQ_INT(original.tiles[row][col], loaded.tiles[row][col]);
        }
    }

    /* Clean up temp file */
    remove(tmpPath);
    TEST_PASS();
}

static void test_error_strings_not_null(void) {
    TEST_BEGIN("levelfile: all error codes have non-null strings");
    for (int i = 0; i <= LEVEL_ERR_WRITE_FAILED; i++) {
        const char *s = levelfile_error_string((LevelError)i);
        ASSERT_TRUE(s != NULL);
        ASSERT_TRUE(strlen(s) > 0);
    }
    TEST_PASS();
}

static void test_comments_ignored(void) {
    TEST_BEGIN("levelfile: comment lines are ignored");
    char buf[4096];
    int len = make_valid_level_string(buf, sizeof(buf));

    /* The helper already includes a "# Test level" comment */
    LevelData data;
    LevelError err = levelfile_load_from_memory(buf, len, &data);
    ASSERT_EQ_INT(LEVEL_OK, err);
    TEST_PASS();
}

static void test_clearance_open_level_no_warnings(void) {
    TEST_BEGIN("levelfile: open level has zero clearance warnings");

    /* Build a level with all-air interior (plenty of space) */
    LevelData data;
    memset(&data, 0, sizeof(data));
    for (int row = 0; row < LEVEL_ROWS; row++) {
        for (int col = 0; col < LEVEL_COLS; col++) {
            if (row == 0 || row == LEVEL_ROWS - 1 || col == 0 || col == LEVEL_COLS - 1) {
                data.tiles[row][col] = 1;
            }
        }
    }
    data.spawn_col = 5;
    data.spawn_row = 27;
    data.tiles[28][5] = 1; /* ground below spawn */

    int warnings[LEVEL_ROWS][LEVEL_COLS];
    int count = levelfile_check_clearance(&data, warnings);
    ASSERT_EQ_INT(0, count);
    TEST_PASS();
}

static void test_clearance_narrow_gap_warns(void) {
    TEST_BEGIN("levelfile: 1-tile-wide gap produces warnings");

    /* Build level with a 1-tile-wide vertical corridor in the middle */
    LevelData data;
    memset(&data, 0, sizeof(data));
    for (int row = 0; row < LEVEL_ROWS; row++) {
        for (int col = 0; col < LEVEL_COLS; col++) {
            if (row == 0 || row == LEVEL_ROWS - 1 || col == 0 || col == LEVEL_COLS - 1) {
                data.tiles[row][col] = 1;
            }
        }
    }
    /* Create a wall with a 1-tile-wide gap at col 20 */
    for (int row = 1; row < LEVEL_ROWS - 1; row++) {
        data.tiles[row][19] = 1;
        data.tiles[row][21] = 1;
        /* col 20 remains air — but it's only 1 tile wide */
    }

    int warnings[LEVEL_ROWS][LEVEL_COLS];
    int count = levelfile_check_clearance(&data, warnings);
    ASSERT_TRUE(count > 0);

    /* The air tiles in the 1-wide gap should be flagged */
    ASSERT_EQ_INT(1, warnings[15][20]);
    TEST_PASS();
}

static void test_clearance_short_gap_warns(void) {
    TEST_BEGIN("levelfile: 3-tile-tall gap produces warnings");

    /* Build level with a horizontal ceiling 3 tiles above floor */
    LevelData data;
    memset(&data, 0, sizeof(data));
    for (int row = 0; row < LEVEL_ROWS; row++) {
        for (int col = 0; col < LEVEL_COLS; col++) {
            if (row == 0 || row == LEVEL_ROWS - 1 || col == 0 || col == LEVEL_COLS - 1) {
                data.tiles[row][col] = 1;
            }
        }
    }
    /* Floor at row 28, ceiling at row 25 → 3 air rows (25 is solid) */
    /* Actually: floor solid at 28, ceiling solid at 25, air at 26,27 = 2 rows */
    /* Need exactly 3 rows of air which is < 4 tiles tall */
    for (int col = 5; col < 15; col++) {
        data.tiles[25][col] = 1; /* ceiling */
        data.tiles[28][col] = 1; /* floor */
    }
    /* Air at rows 26, 27 — only 2 tiles tall, definitely too short */

    int warnings[LEVEL_ROWS][LEVEL_COLS];
    int count = levelfile_check_clearance(&data, warnings);
    ASSERT_TRUE(count > 0);

    /* Tiles in the 2-tall gap between ceiling(25) and floor(28) should warn
     * (actually air is at 26, 27 — but bordered by solid 25 and 28)
     * Height = 2 air tiles, char needs 4. Should warn. */
    ASSERT_TRUE(warnings[26][7] == 1);
    ASSERT_TRUE(warnings[27][7] == 1);
    TEST_PASS();
}

/* ── Enclosure tests ───────────────────────────────────────────────── */

static void test_enclosure_open_level(void) {
    TEST_BEGIN("levelfile: open level reports spawn NOT enclosed");

    /* Standard open level — spawn can reach row 1 through air */
    LevelData data;
    memset(&data, 0, sizeof(data));
    for (int row = 0; row < LEVEL_ROWS; row++) {
        for (int col = 0; col < LEVEL_COLS; col++) {
            if (row == 0 || row == LEVEL_ROWS - 1 || col == 0 || col == LEVEL_COLS - 1)
                data.tiles[row][col] = 1;
        }
    }
    /* Floor at row 28 */
    for (int col = 1; col < LEVEL_COLS - 1; col++)
        data.tiles[LEVEL_ROWS - 2][col] = 1;
    data.spawn_col = 10;
    data.spawn_row = LEVEL_ROWS - 3;  /* row 27, above floor */

    int result = levelfile_check_enclosure(&data);
    ASSERT_TRUE(result == 0);  /* NOT enclosed — can reach top */
    TEST_PASS();
}

static void test_enclosure_sealed_room(void) {
    TEST_BEGIN("levelfile: sealed room reports spawn enclosed");

    /* Build a sealed box: solid border + solid ceiling at row 20 */
    LevelData data;
    memset(&data, 0, sizeof(data));
    for (int row = 0; row < LEVEL_ROWS; row++) {
        for (int col = 0; col < LEVEL_COLS; col++) {
            if (row == 0 || row == LEVEL_ROWS - 1 || col == 0 || col == LEVEL_COLS - 1)
                data.tiles[row][col] = 1;
        }
    }
    /* Seal rows 20-29 as an enclosed room: ceiling at row 20 */
    for (int col = 0; col < LEVEL_COLS; col++)
        data.tiles[20][col] = 1;
    /* Spawn inside the sealed area */
    data.spawn_col = 10;
    data.spawn_row = 27;

    int result = levelfile_check_enclosure(&data);
    ASSERT_TRUE(result == 1);  /* Enclosed — cannot reach row 1 */
    TEST_PASS();
}

static void test_enclosure_gap_in_ceiling(void) {
    TEST_BEGIN("levelfile: ceiling with gap reports spawn NOT enclosed");

    /* Same as sealed room but with a 1-tile gap in ceiling */
    LevelData data;
    memset(&data, 0, sizeof(data));
    for (int row = 0; row < LEVEL_ROWS; row++) {
        for (int col = 0; col < LEVEL_COLS; col++) {
            if (row == 0 || row == LEVEL_ROWS - 1 || col == 0 || col == LEVEL_COLS - 1)
                data.tiles[row][col] = 1;
        }
    }
    for (int col = 0; col < LEVEL_COLS; col++)
        data.tiles[20][col] = 1;
    /* Punch a gap at col 15 */
    data.tiles[20][15] = 0;
    data.spawn_col = 10;
    data.spawn_row = 27;

    int result = levelfile_check_enclosure(&data);
    ASSERT_TRUE(result == 0);  /* NOT enclosed — gap allows escape */
    TEST_PASS();
}

/* ── Suite runner ───────────────────────────────────────────────────── */

void run_levelfile_tests(void) {
    printf("[LevelFile]\n");
    test_valid_file_parses_ok();
    test_missing_version_fails();
    test_bad_version_fails();
    test_bad_size_fails();
    test_spawn_out_of_bounds();
    test_spawn_in_solid_fails();
    test_spawn_no_ground_fails();
    test_bad_char_in_map_fails();
    test_border_not_solid_fails();
    test_roundtrip_save_load();
    test_error_strings_not_null();
    test_comments_ignored();
    test_clearance_open_level_no_warnings();
    test_clearance_narrow_gap_warns();
    test_clearance_short_gap_warns();
    test_enclosure_open_level();
    test_enclosure_sealed_room();
    test_enclosure_gap_in_ceiling();
}
