/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * editor.c — Parcour Level Editor (Win32 GUI)
 *
 * A simple tile-based level editor that produces .parcour files.
 * Click to place solid tiles, right-click to erase (set to air).
 * Middle-click or Ctrl+click to set the spawn point.
 *
 * Controls:
 *   Left-click       Paint solid tile
 *   Right-click      Erase (set to air)
 *   Ctrl+click       Set spawn point
 *   Ctrl+S           Save level
 *   Ctrl+O           Open level
 *   Ctrl+N           New (blank) level
 *   Ctrl+V           Validate current level
 *
 * Build: cl editor.c /I..\engine /I..\game /link engine.lib user32.lib
 *        gdi32.lib comdlg32.lib /out:editor.exe
 */

#include "types.h"
#include "levelfile.h"
#include <stdio.h>
#include <string.h>
#include <commdlg.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comdlg32.lib")

/* ── Editor state ───────────────────────────────────────────────────── */
static LevelData g_level;
static char g_filePath[512] = {0};
static int g_modified = 0;

/* ── Drawing constants ──────────────────────────────────────────────── */
#define EDITOR_TILE  20       /* tile size in pixels for the editor grid */
#define GRID_W       (LEVEL_COLS * EDITOR_TILE)   /* 800 */
#define GRID_H       (LEVEL_ROWS * EDITOR_TILE)   /* 600 */
#define TOOLBAR_H    40       /* toolbar height above the grid */
#define STATUS_H     24       /* status bar height below grid */
#define CLIENT_W     GRID_W
#define CLIENT_H     (TOOLBAR_H + GRID_H + STATUS_H)

/* Colors */
#define COL_AIR       RGB(30, 30, 46)
#define COL_SOLID     RGB(100, 100, 100)
#define COL_BORDER    RGB(140, 80, 80)
#define COL_GRID      RGB(50, 50, 60)
#define COL_SPAWN     RGB(50, 200, 80)
#define COL_WARN      RGB(200, 150, 30)
#define COL_TOOLBAR   RGB(40, 40, 55)
#define COL_STATUS    RGB(35, 35, 50)

static HWND g_hwnd;
static char g_statusText[256] = "Ready — Ctrl+N: New | Ctrl+O: Open | Ctrl+S: Save";

/* Clearance warning map: 1 = this air tile is in a space too tight for the character */
static int g_warnings[LEVEL_ROWS][LEVEL_COLS];
static int g_warningCount = 0;
static int g_spawnEnclosed = 0;  /* 1 if spawn is in a sealed enclosure */

/* ── Clearance analysis (delegates to engine levelfile API) ─────────── */
static void check_clearance(void) {
    g_warningCount = levelfile_check_clearance(&g_level, g_warnings);
    g_spawnEnclosed = levelfile_check_enclosure(&g_level);
}


/* ── Initialize a blank level (solid border, air interior) ──────────── */
static void editor_new_level(void) {
    memset(&g_level, 0, sizeof(g_level));
    strncpy_s(g_level.name, sizeof(g_level.name), "Untitled", _TRUNCATE);
    strncpy_s(g_level.author, sizeof(g_level.author), "Unknown", _TRUNCATE);
    g_level.spawn_col = LEVEL_COLS / 2;
    g_level.spawn_row = LEVEL_ROWS - 2;

    for (int row = 0; row < LEVEL_ROWS; row++) {
        for (int col = 0; col < LEVEL_COLS; col++) {
            if (row == 0 || row == LEVEL_ROWS - 1 ||
                col == 0 || col == LEVEL_COLS - 1) {
                g_level.tiles[row][col] = 1;
            } else {
                g_level.tiles[row][col] = 0;
            }
        }
    }
    g_filePath[0] = '\0';
    g_modified = 0;
    check_clearance();
    snprintf(g_statusText, sizeof(g_statusText), "New level created (%dx%d)", LEVEL_COLS, LEVEL_ROWS);
}

/* ── File dialogs ───────────────────────────────────────────────────── */
static int editor_open_dialog(char *path, int pathSize) {
    OPENFILENAMEA ofn = {0};
    path[0] = '\0';
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwnd;
    ofn.lpstrFilter = "Parcour Level (*.parcour)\0*.parcour\0All Files\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = pathSize;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = "parcour";
    return GetOpenFileNameA(&ofn);
}

static int editor_save_dialog(char *path, int pathSize) {
    OPENFILENAMEA ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwnd;
    ofn.lpstrFilter = "Parcour Level (*.parcour)\0*.parcour\0All Files\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = pathSize;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = "parcour";
    return GetSaveFileNameA(&ofn);
}

/* ── Actions ────────────────────────────────────────────────────────── */
static void editor_open(void) {
    char path[512] = {0};
    if (!editor_open_dialog(path, sizeof(path))) return;

    LevelData data;
    LevelError err = levelfile_load(path, &data);
    if (err != LEVEL_OK) {
        char msg[512];
        snprintf(msg, sizeof(msg), "Failed to load:\n%s\n\n%s", path, levelfile_error_string(err));
        MessageBoxA(g_hwnd, msg, "Load Error", MB_OK | MB_ICONERROR);
        return;
    }
    g_level = data;
    strncpy_s(g_filePath, sizeof(g_filePath), path, _TRUNCATE);
    g_modified = 0;
    check_clearance();
    {
        const char *warn = "";
        if (g_warningCount > 0 && !g_spawnEnclosed)
            warn = "  [!] Tight spaces + spawn NOT sealed";
        else if (g_warningCount > 0)
            warn = "  [!] Tight spaces detected";
        else if (!g_spawnEnclosed)
            warn = "";  /* open spawn is normal */
        snprintf(g_statusText, sizeof(g_statusText), "Loaded: %s%s", g_level.name, warn);
    }
    InvalidateRect(g_hwnd, NULL, FALSE);
}

static void editor_save(void) {
    if (g_filePath[0] == '\0') {
        if (!editor_save_dialog(g_filePath, sizeof(g_filePath))) return;
    }

    LevelError err = levelfile_save(g_filePath, &g_level);
    if (err != LEVEL_OK) {
        char msg[512];
        snprintf(msg, sizeof(msg), "Failed to save:\n%s\n\n%s",
                 g_filePath, levelfile_error_string(err));
        MessageBoxA(g_hwnd, msg, "Save Error", MB_OK | MB_ICONERROR);
        snprintf(g_statusText, sizeof(g_statusText), "Save failed: %s", levelfile_error_string(err));
    } else {
        g_modified = 0;
        snprintf(g_statusText, sizeof(g_statusText), "Saved: %s", g_filePath);
    }
    InvalidateRect(g_hwnd, NULL, FALSE);
}

static void editor_validate(void) {
    LevelError err = levelfile_validate(&g_level);
    if (err == LEVEL_OK) {
        snprintf(g_statusText, sizeof(g_statusText), "Validation passed! Level is valid.");
        MessageBoxA(g_hwnd, "Level is valid!\n\nAll checks passed.", "Validation", MB_OK | MB_ICONINFORMATION);
    } else {
        snprintf(g_statusText, sizeof(g_statusText), "Validation failed: %s", levelfile_error_string(err));
        char msg[512];
        snprintf(msg, sizeof(msg), "Level is NOT valid:\n\n%s", levelfile_error_string(err));
        MessageBoxA(g_hwnd, msg, "Validation Failed", MB_OK | MB_ICONWARNING);
    }
    InvalidateRect(g_hwnd, NULL, FALSE);
}

/* ── Coordinate conversion ──────────────────────────────────────────── */
static int pixel_to_col(int px) { return px / EDITOR_TILE; }
static int pixel_to_row(int py) { return (py - TOOLBAR_H) / EDITOR_TILE; }

/* ── Drawing ────────────────────────────────────────────────────────── */
static void editor_paint(HDC hdc) {
    /* Toolbar background */
    RECT rcToolbar = {0, 0, CLIENT_W, TOOLBAR_H};
    HBRUSH brToolbar = CreateSolidBrush(COL_TOOLBAR);
    FillRect(hdc, &rcToolbar, brToolbar);
    DeleteObject(brToolbar);

    /* Toolbar text */
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(200, 200, 220));
    RECT rcText = {8, 8, CLIENT_W - 8, TOOLBAR_H - 4};
    DrawTextA(hdc, "LMB: Solid | RMB: Air | Ctrl+Click: Spawn | Ctrl+S: Save | Ctrl+O: Open | Ctrl+V: Validate", -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    /* Draw tiles */
    for (int row = 0; row < LEVEL_ROWS; row++) {
        for (int col = 0; col < LEVEL_COLS; col++) {
            int x = col * EDITOR_TILE;
            int y = TOOLBAR_H + row * EDITOR_TILE;
            RECT rc = {x, y, x + EDITOR_TILE, y + EDITOR_TILE};

            COLORREF color;
            int isBorder = (row == 0 || row == LEVEL_ROWS - 1 ||
                            col == 0 || col == LEVEL_COLS - 1);

            if (g_level.tiles[row][col] == 1) {
                color = isBorder ? COL_BORDER : COL_SOLID;
            } else if (g_warnings[row][col]) {
                color = COL_WARN;
            } else {
                color = COL_AIR;
            }

            HBRUSH br = CreateSolidBrush(color);
            FillRect(hdc, &rc, br);
            DeleteObject(br);

            /* Grid lines */
            HPEN pen = CreatePen(PS_SOLID, 1, COL_GRID);
            HPEN oldPen = (HPEN)SelectObject(hdc, pen);
            MoveToEx(hdc, x, y, NULL);
            LineTo(hdc, x + EDITOR_TILE, y);
            MoveToEx(hdc, x, y, NULL);
            LineTo(hdc, x, y + EDITOR_TILE);
            SelectObject(hdc, oldPen);
            DeleteObject(pen);
        }
    }

    /* Draw spawn marker */
    {
        int sx = g_level.spawn_col * EDITOR_TILE + EDITOR_TILE / 2;
        int sy = TOOLBAR_H + g_level.spawn_row * EDITOR_TILE + EDITOR_TILE / 2;
        HBRUSH brSpawn = CreateSolidBrush(COL_SPAWN);
        HBRUSH oldBr = (HBRUSH)SelectObject(hdc, brSpawn);
        Ellipse(hdc, sx - 6, sy - 6, sx + 6, sy + 6);
        SelectObject(hdc, oldBr);
        DeleteObject(brSpawn);
    }

    /* Status bar */
    RECT rcStatus = {0, TOOLBAR_H + GRID_H, CLIENT_W, CLIENT_H};
    HBRUSH brStatus = CreateSolidBrush(COL_STATUS);
    FillRect(hdc, &rcStatus, brStatus);
    DeleteObject(brStatus);

    SetTextColor(hdc, RGB(180, 180, 200));
    RECT rcStat = {8, TOOLBAR_H + GRID_H + 4, CLIENT_W - 8, CLIENT_H - 2};
    DrawTextA(hdc, g_statusText, -1, &rcStat, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

/* ── Mouse handling ─────────────────────────────────────────────────── */
static void editor_handle_click(int px, int py, int button, int ctrl) {
    int col = pixel_to_col(px);
    int row = pixel_to_row(py);

    if (col < 0 || col >= LEVEL_COLS || row < 0 || row >= LEVEL_ROWS) return;

    /* Don't allow editing border tiles */
    if (row == 0 || row == LEVEL_ROWS - 1 || col == 0 || col == LEVEL_COLS - 1) {
        snprintf(g_statusText, sizeof(g_statusText), "Border tiles cannot be modified");
        InvalidateRect(g_hwnd, NULL, FALSE);
        return;
    }

    if (ctrl) {
        /* Set spawn */
        g_level.spawn_col = col;
        g_level.spawn_row = row;
        g_level.tiles[row][col] = 0; /* spawn must be air */
        g_modified = 1;
        check_clearance();
        {
            const char *warn = "";
            if (g_warningCount > 0 && !g_spawnEnclosed)
                warn = "  [!] Tight spaces + spawn NOT sealed";
            else if (g_warningCount > 0)
                warn = "  [!] Tight spaces (spawn sealed)";
            snprintf(g_statusText, sizeof(g_statusText), "Spawn set at (%d, %d)%s", col, row, warn);
        }
    } else if (button == 1) {
        /* Left click: solid */
        /* Don't place solid on spawn */
        if (col == g_level.spawn_col && row == g_level.spawn_row) {
            snprintf(g_statusText, sizeof(g_statusText), "Cannot place solid on spawn point");
        } else {
            g_level.tiles[row][col] = 1;
            g_modified = 1;
            check_clearance();
            if (g_warningCount > 0) {
                snprintf(g_statusText, sizeof(g_statusText),
                         "Solid at (%d, %d) — Warning: %d tile(s) too tight for character",
                         col, row, g_warningCount);
            } else {
                snprintf(g_statusText, sizeof(g_statusText), "Solid at (%d, %d)", col, row);
            }
        }
    } else if (button == 2) {
        /* Right click: air */
        g_level.tiles[row][col] = 0;
        g_modified = 1;
        check_clearance();
        snprintf(g_statusText, sizeof(g_statusText), "Air at (%d, %d)%s",
                 col, row, g_warningCount > 0 ? "  [!] Tight spaces detected" : "");
    }

    InvalidateRect(g_hwnd, NULL, FALSE);
}

/* ── Window procedure ───────────────────────────────────────────────── */
static LRESULT CALLBACK editor_wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        /* Double-buffer to avoid flicker */
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(hdc, CLIENT_W, CLIENT_H);
        HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

        editor_paint(memDC);

        BitBlt(hdc, 0, 0, CLIENT_W, CLIENT_H, memDC, 0, 0, SRCCOPY);
        SelectObject(memDC, oldBmp);
        DeleteObject(memBmp);
        DeleteDC(memDC);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN: {
        int px = LOWORD(lp);
        int py = HIWORD(lp);
        int ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        int button = (msg == WM_LBUTTONDOWN) ? 1 : 2;
        editor_handle_click(px, py, button, ctrl);
        return 0;
    }

    case WM_MOUSEMOVE: {
        /* Paint while dragging */
        if (wp & MK_LBUTTON || wp & MK_RBUTTON) {
            int px = LOWORD(lp);
            int py = HIWORD(lp);
            int ctrl = (wp & MK_CONTROL) != 0;
            int button = (wp & MK_LBUTTON) ? 1 : 2;
            editor_handle_click(px, py, button, ctrl);
        }
        return 0;
    }

    case WM_KEYDOWN:
        if (GetKeyState(VK_CONTROL) & 0x8000) {
            switch (wp) {
            case 'S': editor_save(); return 0;
            case 'O': editor_open(); return 0;
            case 'N': editor_new_level(); InvalidateRect(hwnd, NULL, FALSE); return 0;
            case 'V': editor_validate(); return 0;
            }
        }
        if (wp == VK_ESCAPE) {
            if (g_modified) {
                int r = MessageBoxA(hwnd, "Level has unsaved changes.\nQuit anyway?",
                                    "Confirm Quit", MB_YESNO | MB_ICONQUESTION);
                if (r != IDYES) return 0;
            }
            PostQuitMessage(0);
        }
        return 0;

    case WM_CLOSE:
        if (g_modified) {
            int r = MessageBoxA(hwnd, "Level has unsaved changes.\nQuit anyway?",
                                "Confirm Quit", MB_YESNO | MB_ICONQUESTION);
            if (r != IDYES) return 0;
        }
        PostQuitMessage(0);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

/* ── Entry point ────────────────────────────────────────────────────── */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmdLine, int cmdShow) {
    (void)hPrev; (void)cmdShow;

    /* Start with a blank level */
    editor_new_level();

    /* If a file was passed on command line, load it */
    if (cmdLine && cmdLine[0] != '\0') {
        char path[512] = {0};
        const char *p = cmdLine;
        while (*p == ' ') p++;
        if (*p == '"') {
            p++;
            int i = 0;
            while (*p && *p != '"' && i < (int)sizeof(path) - 1)
                path[i++] = *p++;
            path[i] = '\0';
        } else {
            strncpy_s(path, sizeof(path), p, _TRUNCATE);
        }

        if (path[0] != '\0') {
            LevelData data;
            LevelError err = levelfile_load(path, &data);
            if (err == LEVEL_OK) {
                g_level = data;
                strncpy_s(g_filePath, sizeof(g_filePath), path, _TRUNCATE);
                snprintf(g_statusText, sizeof(g_statusText), "Loaded: %s", g_level.name);
            }
        }
    }

    /* Register window class */
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = editor_wnd_proc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_CROSS);
    wc.lpszClassName = L"ParcourEditor";
    RegisterClassW(&wc);

    /* Calculate window size */
    RECT wr = {0, 0, CLIENT_W, CLIENT_H};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME, FALSE);

    g_hwnd = CreateWindowW(L"ParcourEditor",
        L"Parcour Level Editor  [LMB=Solid  RMB=Air  Ctrl+Click=Spawn  Esc=Quit]",
        (WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX) | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        wr.right - wr.left, wr.bottom - wr.top,
        NULL, NULL, hInst, NULL);

    /* Message loop */
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}
