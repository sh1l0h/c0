#include "../include/utils.h"

// Source: http://www.cse.yorku.ca/~oz/hash.html
size_t str_hash(char *str)
{
    size_t hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

char *str_dup(char *str)
{
    char *result = malloc((strlen(str) + 1) * sizeof *result);
    strcpy(result, str);
    return result;
}
