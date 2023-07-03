#include <stdio.h>
#include <stdbool.h>
#include <SDL_timer.h>
#include <SDL_mutex.h>

#include "logging.h"

static bool log_header(int level, const char *tag);

static bool check_level(int level, const char *tag);

static SDL_mutex *lock = NULL;

void commons_logging_init(const char *context_name) {
    (void) context_name;
    lock = SDL_CreateMutex();
}

void commons_logging_deinit() {
    SDL_DestroyMutex(lock);
    lock = NULL;
}

void commons_log_vprintf(commons_log_level level, const char *tag, const char *fmt, va_list arg) {
    if (lock == NULL) {
        return;
    }
    SDL_LockMutex(lock);
    if (!log_header(level, tag)) {
        SDL_UnlockMutex(lock);
        return;
    }
    vfprintf(stderr, fmt, arg);
    int last_idx = (int) strlen(fmt) - 1;
    if (last_idx >= 0 && fmt[last_idx] == '\n') {
        fprintf(stderr, "\x1b[0m");
    } else {
        fprintf(stderr, "\x1b[0m\n");
    }
    SDL_UnlockMutex(lock);
}

static bool log_header(int level, const char *tag) {
    if (!check_level(level, tag)) {
        return false;
    }
    switch (level) {
        case COMMONS_LOG_LEVEL_INFO:
            fprintf(stderr, "[%.03f][%s]\x1b[36m ", (float) SDL_GetTicks() / 1000.0f, tag);
            break;
        case COMMONS_LOG_LEVEL_WARN:
            fprintf(stderr, "[%.03f][%s]\x1b[33m ", (float) SDL_GetTicks() / 1000.0f, tag);
            break;
        case COMMONS_LOG_LEVEL_ERROR:
            fprintf(stderr, "[%.03f][%s]\x1b[31m ", (float) SDL_GetTicks() / 1000.0f, tag);
            break;
        case COMMONS_LOG_LEVEL_FATAL:
            fprintf(stderr, "[%.03f][%s]\x1b[41m ", (float) SDL_GetTicks() / 1000.0f, tag);
            break;
        case COMMONS_LOG_LEVEL_VERBOSE:
            fprintf(stderr, "[%.03f][%s]\x1b[34m ", (float) SDL_GetTicks() / 1000.0f, tag);
            break;
        default:
            fprintf(stderr, "[%.03f][%s] ", (float) SDL_GetTicks() / 1000.0f, tag);
            break;
    }
    return true;
}

static bool check_level(int level, const char *tag) {
    return level < COMMONS_LOG_LEVEL_VERBOSE;
}