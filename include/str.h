#ifndef C0_STR_H
#define C0_STR_H

#include <string.h>

#define STRING_BUCKETS_SIZE 1024

#define str_len(_str) (((String*)(_str))->size)

typedef struct String String;

struct String {
    char *str;
    size_t len;
    String *next;
};

extern String *strings[STRING_BUCKETS_SIZE];

char *str_get(char *str, size_t len);
char *str_get_null_term(char *str);

size_t str_hash(char *str);

#endif
