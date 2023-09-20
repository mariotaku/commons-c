#include "logging.h"

#include <stdio.h>
#include <string.h>
#include <PmLogLib.h>

static PmLogContext context;
static struct timespec ts_init;

void commons_logging_init(const char *context_name) {
    PmLogGetContext(context_name != NULL ? context_name : "commons_logging", &context);
    clock_gettime(CLOCK_MONOTONIC, &ts_init);
}

void commons_logging_deinit() {
    // No-op
}

void commons_log_vprintf(commons_log_level level, const char *tag, const char *fmt, va_list arg) {
    char msg[1024];
    vsnprintf(msg, 1023, fmt, arg);
    char *brk = strrchr(msg, '\n');
    if (brk != NULL) {
        *brk = '\0';
    }
    FILE *output = stdout;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    float timestamp = (float) (ts.tv_sec - ts_init.tv_sec) + (float) (ts.tv_nsec - ts_init.tv_nsec) / 1000000000.0f;
    switch (level) {
        case COMMONS_LOG_LEVEL_FATAL:
            output = stderr;
            PmLogCritical(context, tag, 0, "[%.03f] %s", timestamp, msg);
            break;
        case COMMONS_LOG_LEVEL_ERROR:
            output = stderr;
            PmLogError(context, tag, 0, "[%.03f] %s", timestamp, msg);
            break;
        case COMMONS_LOG_LEVEL_WARN:
            output = stderr;
            PmLogWarning(context, tag, 0, "[%.03f] %s", timestamp, msg);
            break;
        case COMMONS_LOG_LEVEL_INFO:
            PmLogInfo(context, tag, 0, "[%.03f] %s", timestamp, msg);
            break;
        case COMMONS_LOG_LEVEL_DEBUG:
        case COMMONS_LOG_LEVEL_VERBOSE:
            PmLogDebug(context, "[%.03f] %s", timestamp, msg);
            break;
    }
    fprintf(output, "[%.03f][%s] %s\n", timestamp, tag, msg);
}
