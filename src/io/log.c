#include "../../include/io/log.h"
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

static const char *type_strings[] = {
    "info", "warning", "error", "fatal error"
};

static char *type_colors[] = {
    "\e[1;34m", "\e[1;35m", "\e[1;31m", "\e[0;31m"
};

static char *clear_color = "\e[0m";

void log_init(bool no_colors)
{
    if (!no_colors && isatty(fileno(stderr)))
        return;

    clear_color = "";
    for (int i = 0; i < LOG_TYPE_COUNT; i++)
        type_colors[i] = clear_color;
}

void log_print(LogType type, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    fprintf(stderr, "c0: %s%s:%s ", type_colors[type],
            type_strings[type], clear_color);
    vfprintf(stderr, format, args);
    fputc('\n', stderr);
    fflush(stderr);

    va_end(args);
}

void log_print_with_location(LogType type, Location *location,
                             const char *format, ...)
{
    va_list args;
    va_start(args, format);

    if (location->column_start != location->column_end) {
        fprintf(stderr, "%s:%ld:%ld-%ld: %s%s:%s ", location->file_path,
                location->line, location->column_start, location->column_end,
                type_colors[type], type_strings[type], clear_color);
    }
    else {
        fprintf(stderr, "%s:%ld:%ld: %s%s:%s ", location->file_path,
                location->line, location->column_start, type_colors[type],
                type_strings[type], clear_color);
    }

    vfprintf(stderr, format, args);
    fputc('\n', stderr);
    va_end(args);

    FILE *file = fopen(location->file_path, "r");

    char *line = NULL;
    size_t n = 0, line_count = 1;
    ssize_t chars_read;

    while (((chars_read = getline(&line, &n, file)) != -1 &&
            line_count++ != location->line));

    fclose(file);

    if (--line_count != location->line) {
        log_error("Unreachable line number %ld in file %s",
                  location->line, location->file_path);
        return;
    }

    fprintf(stderr, " %ld |%s", line_count, line);
    size_t line_char_number = floor(log10(line_count)) + 1;

    for (size_t i = 0; i < line_char_number + 2; i++)
        fputc(' ', stderr);

    fputc('|', stderr);

    for (size_t i = 0; i < location->column_start - 1; i++) 
        fputc(line[i] == '\t' ? '\t' : ' ', stderr);

    fputs(type_colors[type], stderr);

    for (size_t i = location->column_start; i <= location->column_end; i++)
        fputc('^', stderr);

    fputs(clear_color, stderr);

    fputc('\n', stderr);

    fflush(stderr);

    free(line);
}
