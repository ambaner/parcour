/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * game.c — Entry point: window creation, game loop, module wiring
 *
 * Build: cl /O2 /W4 game.c character.c sprite.c input.c renderer.c
 *            level.c physics.c math.c
 *            /link user32.lib gdi32.lib winmm.lib /out:parcour.exe
 */

#include "types.h"
#include "input.h"
#include "renderer.h"
#include "level.h"
#include "levelfile.h"
#include "character.h"
#include "log.h"
#include <mmsystem.h>
#include <string.h>
#include <stdio.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")

static BITMAPINFO bmi;
static Character player;

/* ── Dynamic window size (updated on WM_SIZE) ── */
static int winClientW = WINDOW_W;
static int winClientH = WINDOW_H;

/* ── Render one frame ───────────────────────────────────────────────── */
static void render_frame(void) {
    renderer_clear(0xFF1A1A2E);
    level_draw();
    character_draw(&player);
}

/* ── Window procedure ───────────────────────────────────────────────── */
static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_KEYDOWN:
        input_key_down((int)wp);
        if (wp == VK_ESCAPE) PostQuitMessage(0);
        return 0;
    case WM_KEYUP:
        input_key_up((int)wp);
        return 0;
    case WM_SIZE:
        winClientW = LOWORD(lp);
        winClientH = HIWORD(lp);
        if (winClientW < 1) winClientW = 1;
        if (winClientH < 1) winClientH = 1;
        return 0;
    case WM_GETMINMAXINFO: {
        /* Enforce a minimum window size so the game stays usable.
         * AdjustWindowRect converts client-area dimensions (640×480) to
         * full window dimensions (including title bar, borders, etc.)
         * so the minimum applies to the drawable area, not the frame. */
        MINMAXINFO *mmi = (MINMAXINFO *)lp;
        RECT rc = {0, 0, 640, 480};
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
        mmi->ptMinTrackSize.x = rc.right - rc.left;
        mmi->ptMinTrackSize.y = rc.bottom - rc.top;
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

/* ── Entry point ────────────────────────────────────────────────────── */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmdLine, int cmdShow) {
    (void)hPrev; (void)cmdShow;

    /* ── Init logging (recreates log file each run) ── */
    game_log_init();
    game_log("Game starting up");

    /* ── Load custom level from command line (if provided) ── */
    {
        /* cmdLine contains everything after the exe name.
         * Strip leading/trailing whitespace and quotes. */
        char levelPath[512] = {0};
        if (cmdLine && cmdLine[0] != '\0') {
            const char *p = cmdLine;
            while (*p == ' ' || *p == '\t') p++;
            if (*p == '"') {
                p++;
                int i = 0;
                while (*p && *p != '"' && i < (int)sizeof(levelPath) - 1)
                    levelPath[i++] = *p++;
                levelPath[i] = '\0';
            } else {
                strncpy_s(levelPath, sizeof(levelPath), p, _TRUNCATE);
                /* Trim trailing whitespace */
                int len = (int)strlen(levelPath);
                while (len > 0 && (levelPath[len-1] == ' ' || levelPath[len-1] == '\r' || levelPath[len-1] == '\n'))
                    levelPath[--len] = '\0';
            }
        }

        if (levelPath[0] != '\0') {
            game_log("Loading level file: %s", levelPath);
            LevelData data;
            LevelError err = levelfile_load(levelPath, &data);
            if (err != LEVEL_OK) {
                char errMsg[512];
                snprintf(errMsg, sizeof(errMsg),
                    "Failed to load level file:\n%s\n\nError: %s",
                    levelPath, levelfile_error_string(err));
                MessageBoxA(NULL, errMsg, "Level Load Error", MB_OK | MB_ICONERROR);
                game_log("Level load failed: %s", levelfile_error_string(err));
                game_log_close();
                return 1;
            }
            level_load_from_data(&data);
            game_log("Level loaded: \"%s\" by %s (spawn %d,%d)",
                     data.name, data.author, data.spawn_col, data.spawn_row);
        } else {
            game_log("No level file specified, using built-in level (spawn %d,%d)",
                     level_spawn_col, level_spawn_row);
        }
    }

    /* ── Load sprite frames ── */
    {
        /* Find sprite directory: walk up from exe directory until we
         * find a 'sprites' folder.  This handles the exe living in
         * build/amd64fre/ while sprites stay at project root.
         *
         * Algorithm: starting from the exe's own directory, try
         * <dir>/sprites, then <parent>/sprites, up to 4 parent levels.
         * This avoids hard-coding relative paths that break when the
         * build output directory depth changes. */
        char exePath[512] = {0};
        GetModuleFileNameA(NULL, exePath, sizeof(exePath));
        char *lastSlash = exePath;
        for (char *p = exePath; *p; p++) {
            if (*p == '\\' || *p == '/') lastSlash = p;
        }
        *lastSlash = '\0';

        char spriteDir[512];
        char tryDir[512];
        int found = 0;

        /* Try up to 4 parent levels */
        strncpy_s(tryDir, sizeof(tryDir), exePath, _TRUNCATE);
        for (int up = 0; up < 5; up++) {
            snprintf(spriteDir, sizeof(spriteDir), "%s\\sprites", tryDir);
            if (GetFileAttributesA(spriteDir) != INVALID_FILE_ATTRIBUTES) {
                found = 1;
                break;
            }
            /* Go up one directory */
            char *sl = tryDir;
            for (char *p = tryDir; *p; p++) {
                if (*p == '\\' || *p == '/') sl = p;
            }
            if (sl == tryDir) break;
            *sl = '\0';
        }

        if (!found) {
            snprintf(spriteDir, sizeof(spriteDir), "%s\\sprites", exePath);
        }

        int loaded = character_load_sprites(spriteDir);
        game_log("Sprites loaded: %d frames from %s", loaded, spriteDir);
        if (loaded == 0) {
            MessageBoxA(NULL,
                "Could not load sprites!\n\n"
                "Make sure the 'sprites' folder exists at the\n"
                "project root (next to src/, build/, etc.).",
                "Sprite Loading Error", MB_OK | MB_ICONERROR);
            return 1;
        }
    }

    /* ── Init player — start at level spawn point ── */
    {
        float spawnX = (float)level_spawn_col * TILE_SIZE;
        float spawnY = (float)(level_spawn_row + 1) * TILE_SIZE - RENDER_H;
        character_init(&player, spawnX, spawnY);
        game_log("Player init at (%.0f, %.0f) [spawn tile %d,%d]",
                 spawnX, spawnY, level_spawn_col, level_spawn_row);
    }

    /* ── Create window ── */
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"ParcourGame";
    RegisterClassW(&wc);

    RECT wr = {0, 0, WINDOW_W, WINDOW_H};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowW(L"ParcourGame",
        L"Parcour — PoP-inspired Platformer  [Arrows=Move  Up=Jump  Esc=Quit]",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        wr.right - wr.left, wr.bottom - wr.top,
        NULL, NULL, hInst, NULL);

    /* ── Set up DIB for blitting ── */
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = SCREEN_W;
    bmi.bmiHeader.biHeight = -SCREEN_H;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    /* ── Game loop (fixed timestep) ──
     * Poll for Windows messages, then check if FRAME_MS has elapsed.
     * If yes: run one simulation tick + render.  If no: Sleep(1) to
     * yield the CPU and avoid busy-spinning at 100% usage.
     * This gives a fixed ~60 FPS (assuming FRAME_MS ≈ 16). */
    HDC hdc = GetDC(hwnd);
    MSG msg;
    DWORD lastTime = timeGetTime();

    for (;;) {
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) goto done;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        DWORD now = timeGetTime();
        if (now - lastTime >= FRAME_MS) {
            lastTime = now;
            input_update();
            character_update(&player);
            render_frame();
            /* Blit with aspect-ratio-preserving letterbox.
             * Compute uniform scale = min(scaleX, scaleY) so the game
             * image fills the window without stretching.  The remainder
             * on the longer axis becomes black letterbox/pillarbox bars. */
            {
                float scaleX = (float)winClientW / SCREEN_W;
                float scaleY = (float)winClientH / SCREEN_H;
                float scale  = (scaleX < scaleY) ? scaleX : scaleY;
                int dstW = (int)(SCREEN_W * scale);
                int dstH = (int)(SCREEN_H * scale);
                int offX = (winClientW - dstW) / 2;
                int offY = (winClientH - dstH) / 2;

                /* Clear letterbox bars */
                HBRUSH black = (HBRUSH)GetStockObject(BLACK_BRUSH);
                if (offX > 0) {
                    RECT left  = {0, 0, offX, winClientH};
                    RECT right = {offX + dstW, 0, winClientW, winClientH};
                    FillRect(hdc, &left, black);
                    FillRect(hdc, &right, black);
                }
                if (offY > 0) {
                    RECT top    = {0, 0, winClientW, offY};
                    RECT bottom = {0, offY + dstH, winClientW, winClientH};
                    FillRect(hdc, &top, black);
                    FillRect(hdc, &bottom, black);
                }

                StretchDIBits(hdc, offX, offY, dstW, dstH,
                              0, 0, SCREEN_W, SCREEN_H,
                              framebuf, &bmi, DIB_RGB_COLORS, SRCCOPY);
            }
        } else {
            Sleep(1);
        }
    }

done:
    game_log("Game shutting down");
    game_log_close();
    character_free_sprites();
    ReleaseDC(hwnd, hdc);
    return 0;
}
