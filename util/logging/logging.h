#pragma once

#include <stddef.h>
#include <stdarg.h>

typedef enum commons_log_level {
    COMMONS_LOG_LEVEL_FATAL,
    COMMONS_LOG_LEVEL_ERROR,
    COMMONS_LOG_LEVEL_WARN,
    COMMONS_LOG_LEVEL_INFO,
    COMMONS_LOG_LEVEL_DEBUG,
    COMMONS_LOG_LEVEL_VERBOSE,
} commons_log_level;

void commons_logging_init(const char *context_name);

void commons_logging_deinit();

void commons_log_printf(commons_log_level level, const char *tag, const char *fmt, ...) __attribute__ ((format (printf, 3, 4)));

void commons_log_hexdump(commons_log_level level, const char *tag, const unsigned char *data, size_t len);

void commons_log_vprintf(commons_log_level level, const char *tag, const char *fmt, va_list arg);

#define commons_log_fatal(tag, ...) commons_log_printf(COMMONS_LOG_LEVEL_FATAL, (tag), __VA_ARGS__)

#define commons_log_error(tag, ...) commons_log_printf(COMMONS_LOG_LEVEL_ERROR, (tag), __VA_ARGS__)

#define commons_log_warn(tag, ...) commons_log_printf(COMMONS_LOG_LEVEL_WARN, (tag), __VA_ARGS__)

#define commons_log_info(tag, ...) commons_log_printf(COMMONS_LOG_LEVEL_INFO, (tag), __VA_ARGS__)

#define commons_log_debug(tag, ...) commons_log_printf(COMMONS_LOG_LEVEL_DEBUG, (tag), __VA_ARGS__)

#define commons_log_verbose(tag, ...) commons_log_printf(COMMONS_LOG_LEVEL_VERBOSE, (tag), __VA_ARGS__)

#ifdef COMMONS_LOGGING_SS4S
#include "ss4s/logging.h"
void commons_ss4s_logf(SS4S_LogLevel level, const char *tag, const char *fmt, ...) __attribute__ ((format (printf, 3, 4)));
#endif

#ifdef COMMONS_LOGGING_LVGL
void commons_lv_log(const char *message);
#endif

#ifdef COMMONS_LOGGING_SDL
#include <SDL_log.h>
void commons_sdl_log(void *userdata, int category, SDL_LogPriority priority, const char *message);
#endif