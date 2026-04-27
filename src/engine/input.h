/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * input.h — Keyboard state tracking
 */
#ifndef INPUT_H
#define INPUT_H

/* Raw key states (1 = held, 0 = released) */
extern int keyLeft, keyRight, keyUp, keyDown;

/* Edge-detected: true only on the frame the key is first pressed */
extern int keyUpJustPressed;
extern int keyDownJustPressed;

/* Previous-frame states (for edge detection) */
extern int keyUpPrev;
extern int keyDownPrev;

/* Call once per frame, before anything reads keyUpJustPressed */
void input_update(void);

/* Call from WM_KEYDOWN / WM_KEYUP */
void input_key_down(int vk);
void input_key_up(int vk);

#endif /* INPUT_H */
