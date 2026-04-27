/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * log.h — Simple file logging for debugging
 *
 * Call game_log_init() at startup (recreates log file each run)
 * and game_log_close() at shutdown. Use game_log() anywhere.
 */
#ifndef LOG_H
#define LOG_H

void game_log_init(void);
void game_log_close(void);

/* printf-style logging — writes to parcour.log */
void game_log(const char *fmt, ...);

#endif /* LOG_H */
