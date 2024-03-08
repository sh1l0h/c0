#ifndef C0_CYCLICING_QUEUE_H
#define C0_CYCLICING_QUEUE_H

#include "../utils.h"

typedef struct CyclicQueue {
    unsigned char *data;

    size_t start;
    size_t allocated_elements;
    size_t element_size;

    size_t size;
} CyclicQueue;

void cyclic_queue_create(CyclicQueue *queue,
                         size_t element_size,
                         size_t initial_size);
void cyclic_queue_destroy(CyclicQueue *queue);

void *cyclic_queue_offset(CyclicQueue *queue, size_t index);
void cyclic_queue_copy(CyclicQueue *queue, size_t index, void *dest);
void cyclic_queue_resize(CyclicQueue *queue, size_t new_size);
void cyclic_queue_enqueue(CyclicQueue *queue, void *element);
void cyclic_queue_dequeue(CyclicQueue *queue, void (*free_elemnt)(void *));

void cyclic_queue_clear(CyclicQueue *queue);

#endif
