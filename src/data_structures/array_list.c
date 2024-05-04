#include "../../include/data_structures/array_list.h"

void array_list_create(ArrayList *list, size_t element_size, size_t initial_size)
{
    list->element_size = element_size;
    list->allocated_bytes = element_size * initial_size;
    list->data = malloc(list->allocated_bytes);
    list->size = 0;
}

void array_list_destroy(ArrayList *list, void (*free_element)(void *))
{
    if (free_element != NULL) {
        for (size_t i = 0; i < list->size; i++) 
            free_element(array_list_offset(list, i));
    }

    free(list->data);
}

void array_list_set(ArrayList *list, size_t index, const void *element)
{
    void *indexed_element = list->data + index * list->element_size;

    if (element != NULL)
        memcpy(indexed_element, element, list->element_size);
    else
        memset(indexed_element, 0, list->element_size);
}

void *array_list_offset(ArrayList *list, size_t index)
{
    return list->data + index * list->element_size;
}

void array_list_remove(ArrayList *list,
                       size_t index,
                       void (*free_element)(void *))
{
    list->size--;

    if (list->size == index)
        return;

    void *element = array_list_offset(list, index);

    if (free_element)
        free_element(element);

    void *next_element = array_list_offset(list, index + 1);
    memmove(element, next_element, list->element_size * (list->size - index));
}

void array_list_unordered_remove(ArrayList *list, size_t index,
                                 void (*free_element)(void *))
{
    if (index == list->size - 1) {
        list->size--;
        return;
    }

    void *element_to_remove = array_list_offset(list, index);

    if (free_element)
        free_element(element_to_remove);

    void *last_element = array_list_offset(list, list->size - 1);
    memcpy(element_to_remove, last_element, list->element_size);
    list->size--;
}

void *array_list_append(ArrayList *list, const void *element)
{
    void *new_element = list->data + list->size * list->element_size;
    list->size++;

    if (element != NULL)
        memcpy(new_element, element, list->element_size);
    else
        memset(new_element, 0, list->element_size);

    if (list->size >= list->allocated_bytes / list->element_size)
        list->data = realloc(list->data, list->allocated_bytes *= 2);

    return new_element;
}

void array_list_sort(ArrayList *list, int (*cmp)(const void *, const void *))
{
    // The time complexity of qsort is not defined by the standard
    // TODO: write sort
    qsort(list->data, list->size, list->element_size, cmp);
}
