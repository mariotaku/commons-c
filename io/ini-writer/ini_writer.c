#include "ini_writer.h"

#include <stdarg.h>

int ini_write_section(FILE *fp, const char *name) {
    return fprintf(fp, "[%s]\r\n", name);
}

int ini_write_string(FILE *fp, const char *name, const char *value) {
    return fprintf(fp, "%s = %s\r\n", name, value);
}

int ini_write_stringf(FILE *fp, const char *name, const char *fmt, ...) {
    fprintf(fp, "%s = ", name);
    va_list args;
    int ret;
    va_start(args, fmt);
    ret = vfprintf(fp, fmt, args);
    va_end(args);
    fputs("\r\n", fp);
    return ret;
}

int ini_write_int(FILE *fp, const char *name, int value) {
    return fprintf(fp, "%s = %d\r\n", name, value);
}

int ini_write_bool(FILE *fp, const char *name, bool value) {
    return fprintf(fp, "%s = %s\r\n", name, value ? "true" : "false");
}

int ini_write_comment(FILE *fp, const char *fmt, ...) {
    fputs(";", fp);
    va_list args;
    int ret;
    va_start(args, fmt);
    ret = vfprintf(fp, fmt, args);
    va_end(args);
    fputs("\r\n", fp);
    return ret;
}