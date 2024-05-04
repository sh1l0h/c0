#ifndef C0_ARRAY_LIST_H
#define C0_ARRAY_LIST_H

#include "../utils.h"

typedef struct ArrayList {
    char *data;
    size_t allocated_bytes;

    size_t element_size;
    size_t size;
} ArrayList;

void array_list_create(ArrayList *list, size_t element_size, size_t initial_size);
void array_list_destroy(ArrayList *list, void (*free_element)(void *));

void array_list_set(ArrayList *list, size_t index, const void *element);

void *array_list_offset(ArrayList *list, size_t index);

void array_list_remove(ArrayList *list, size_t index,
                       void (*free_element)(void *));
void array_list_unordered_remove(ArrayList *list, size_t index,
                                 void (*free_element)(void *));

void *array_list_append(ArrayList *list, const void *element);

void array_list_sort(ArrayList *list, int (*cmp)(const void *, const void *));

#endif
