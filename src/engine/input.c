/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * input.c — Keyboard state tracking
 *
 * Owns the raw key flags and the per-frame edge detection.
 * No dependencies beyond <windows.h> for VK_ constants.
 */

#include "types.h"
#include "input.h"

int keyLeft, keyRight, keyUp, keyDown;
int keyUpJustPressed;
int keyDownJustPressed;
int keyUpPrev;
int keyDownPrev;

/*
 * input_update — Compute per-frame edge-detected "just pressed" flags.
 *
 * Must be called once at the start of each frame, before any gameplay
 * code reads keyUpJustPressed / keyDownJustPressed.  Edge detection
 * works by comparing the current key state against the previous frame:
 * a key is "just pressed" when it is held NOW but was NOT held last frame.
 * After sampling, the previous-frame state is updated for the next frame.
 */
void input_update(void) {
    keyUpJustPressed   = (keyUp   && !keyUpPrev);
    keyDownJustPressed = (keyDown && !keyDownPrev);
    keyUpPrev   = keyUp;
    keyDownPrev = keyDown;
}

/*
 * input_key_down — Set the held-flag for a key.
 * Called from the Win32 WM_KEYDOWN handler.  Multiple calls for the
 * same key while held are harmless (flag stays 1).
 */
void input_key_down(int vk) {
    if (vk == VK_LEFT)   keyLeft  = 1;
    if (vk == VK_RIGHT)  keyRight = 1;
    if (vk == VK_UP)     keyUp    = 1;
    if (vk == VK_DOWN)   keyDown  = 1;
}

/*
 * input_key_up — Clear the held-flag for a key.
 * Called from the Win32 WM_KEYUP handler.
 */
void input_key_up(int vk) {
    if (vk == VK_LEFT)   keyLeft  = 0;
    if (vk == VK_RIGHT)  keyRight = 0;
    if (vk == VK_UP)     keyUp    = 0;
    if (vk == VK_DOWN)   keyDown  = 0;
}
