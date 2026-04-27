/*
 * Copyright (c) 2026 ambaner. Licensed under the MIT License.
 * log.c — Simple file logging for debugging
 *
 * Log file is recreated each run (previous contents deleted).
 * Logs are flushed after every write for crash safety.
 */

#include "log.h"
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>

#pragma warning(disable: 4996) /* fopen is fine for a debug log */

#define LOG_FILENAME "parcour.log"

static FILE *s_logFile = NULL;
static LARGE_INTEGER s_startTime;
static LARGE_INTEGER s_freq;

void game_log_init(void)
{
    /* Delete old log and create fresh */
    s_logFile = fopen(LOG_FILENAME, "w");
    if (s_logFile) {
        QueryPerformanceFrequency(&s_freq);
        QueryPerformanceCounter(&s_startTime);
        fprintf(s_logFile, "=== Parcour Debug Log ===\n\n");
        fflush(s_logFile);
    }
}

void game_log_close(void)
{
    if (s_logFile) {
        fprintf(s_logFile, "\n=== Log closed ===\n");
        fclose(s_logFile);
        s_logFile = NULL;
    }
}

void game_log(const char *fmt, ...)
{
    if (!s_logFile) return;

    /* Timestamp in seconds since start */
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    double elapsed = (double)(now.QuadPart - s_startTime.QuadPart)
                   / (double)s_freq.QuadPart;

    fprintf(s_logFile, "[%8.3f] ", elapsed);

    va_list args;
    va_start(args, fmt);
    vfprintf(s_logFile, fmt, args);
    va_end(args);

    fprintf(s_logFile, "\n");
    fflush(s_logFile);
}
