#ifndef C0_LOG_H
#define C0_LOG_H

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

#define log_info(...)  log_print(LOG_INFO, __VA_ARGS__)
#define log_warn(...)  log_print(LOG_WARN, __VA_ARGS__)
#define log_error(...) log_print(LOG_ERROR, __VA_ARGS__)
#define log_fatal(...) log_print(LOG_FATAL, __VA_ARGS__)

#define log_info_with_loc(...)  log_print_with_location(LOG_INFO, __VA_ARGS__)
#define log_warn_with_loc(...)  log_print_with_location(LOG_WARN, __VA_ARGS__)
#define log_error_with_loc(...) log_print_with_location(LOG_ERROR, __VA_ARGS__)
#define log_fatal_with_loc(...) log_print_with_location(LOG_FATAL, __VA_ARGS__)

typedef struct Location {
    char *file_path;
    size_t line, column_start, column_end;
} Location;

typedef enum LogType {
    LOG_INFO = 0,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL,

    LOG_TYPE_COUNT // Always keep this as the last entry
} LogType;

void log_init();

void log_print(LogType type, const char *format, ...);
void log_print_with_location(LogType type, Location *location,
                             const char *format, ...);

#endif
