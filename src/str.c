#include "../include/str.h"
#include <stdlib.h>

String *strings[STRING_BUCKETS_SIZE] = {NULL};

char *str_get(char *str, size_t len)
{
    size_t index = str_hash(str) & (STRING_BUCKETS_SIZE - 1);

    String *curr = strings[index];
    while (curr != NULL) {
        if (strncmp(curr->str, str, len) == 0)
            return curr->str;
        curr = curr->next;
    }

    size_t len_in_bytes = len * sizeof *str;
    char *s = malloc(len_in_bytes);
    memcpy(s, str, len_in_bytes);

    curr = malloc(sizeof *curr);
    curr->str = s;
    curr->len = len;
    curr->next = strings[index];
    strings[index] = curr;

    return s;
}

char *str_get_null_term(char *str)
{
    return str_get(str, strlen(str));
}

// Source: http://www.cse.yorku.ca/~oz/hash.html
size_t str_hash(char *str)
{
    size_t hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}
