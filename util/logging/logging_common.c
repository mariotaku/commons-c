#include "logging.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

static void app_lv_log_line(const char *line, size_t len);

void commons_log_printf(commons_log_level level, const char *tag, const char *fmt, ...) {
    va_list arg;
    va_start(arg, fmt);
    commons_log_vprintf(level, tag, fmt, arg);
    va_end(arg);
}

void commons_log_hexdump(commons_log_level level, const char *tag, const void *data, size_t len) {
    char line[80];
    static const char hex_table[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    for (int i = 0; i < len; i += 16) {
        int offset = 0;
        snprintf(line, 80, "%04x  ", i);
        offset += 6;
        for (int col = i, col_end = i + 16; col < col_end; ++col) {
            if (col >= len) {
                line[offset++] = ' ';
                line[offset++] = ' ';
            } else {
                line[offset++] = hex_table[(((const unsigned char *) data)[col] >> 4) & 0xF];
                line[offset++] = hex_table[(((const unsigned char *) data)[col]) & 0xF];
            }
            line[offset++] = ' ';
            if (col - i == 7) {
                line[offset++] = ' ';
            }
        }
        line[offset++] = ' ';
        line[offset++] = '|';
        for (int col = i, col_end = i + 16; col < col_end && col < len; ++col) {
            if (col >= len) {
                line[offset++] = ' ';
            } else {
                unsigned char ch = ((const unsigned char *) data)[col];
                line[offset++] = ch >= ' ' && ch <= '~' ? ch : '.';
            }
        }
        line[offset++] = '|';
        line[offset] = '\0';
        commons_log_printf(level, tag, "%s", line);
    }
}

#ifdef COMMONS_LOGGING_SS4S
#endif


#ifdef COMMONS_LOGGING_LVGL
#endif