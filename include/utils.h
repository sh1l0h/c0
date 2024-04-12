#ifndef C0_UTILS_H
#define C0_UTILS_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

#include "./io/log.h"

size_t str_hash(char *str);
size_t ptr_hash(void *ptr);
char *str_dup(char *str);

#endif
