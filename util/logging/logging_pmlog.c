#include <stdio.h>

#include "logging.h"

#include <SDL.h>
#include <PmLogLib.h>

static SDL_mutex *mutex = NULL;
static PmLogContext context;

void commons_logging_init() {
    PmLogGetContext("ihsplay", &context);
}

void commons_logging_deinit() {
    // No-op
}

void commons_log_vprintf(commons_log_level level, const char *tag, const char *fmt, va_list arg) {
    char msg[1024];
    vsnprintf(msg, 1023, fmt, arg);
    msg[1023] = '\0';
    FILE *output = stdout;
    switch (level) {
        case COMMONS_LOG_LEVEL_FATAL:
            output = stderr;
            PmLogCritical(context, tag, 0, "[%.03f] %s", ((float) SDL_GetTicks() / 1000.0f), msg);
            break;
        case COMMONS_LOG_LEVEL_ERROR:
            output = stderr;
            PmLogError(context, tag, 0, "[%.03f] %s", ((float) SDL_GetTicks() / 1000.0f), msg);
            break;
        case COMMONS_LOG_LEVEL_WARN:
            output = stderr;
            PmLogWarning(context, tag, 0, "[%.03f] %s", ((float) SDL_GetTicks() / 1000.0f), msg);
            break;
        case COMMONS_LOG_LEVEL_INFO:
            PmLogInfo(context, tag, 0, "[%.03f] %s", ((float) SDL_GetTicks() / 1000.0f), msg);
            break;
        case COMMONS_LOG_LEVEL_DEBUG:
        case COMMONS_LOG_LEVEL_VERBOSE:
            PmLogDebug(context, tag, 0, "[%.03f] %s", ((float) SDL_GetTicks() / 1000.0f), msg);
            break;
    }
    fprintf(output, "[%.03f][%s] %s\n", ((float) SDL_GetTicks() / 1000.0f), tag, msg);
}
