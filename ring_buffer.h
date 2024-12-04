//defines a ring buffer implementation. provides a circular data structure for storing and processing data

#ifndef UART_IRQ_RING_BUFFER_H
#define UART_IRQ_RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct  {
    int head; //manage the start and end positions of data in the buffer.
    int tail;
    int size;
    uint8_t *buffer;
} ring_buffer;

void rb_init(ring_buffer *rb, uint8_t *buffer, int size); //Initializes the ring buffer with a given buffer and size.
bool rb_empty(ring_buffer *rb); //Checks if the buffer is empty
bool rb_full(ring_buffer *rb); //Checks if the buffer is full
bool rb_put(ring_buffer *rb, uint8_t data); // Adds data to the buffer (if not full).
uint8_t rb_get(ring_buffer *rb); // Retrieves data from the buffer (if not empty).

void rb_alloc(ring_buffer *rb, int size); //Dynamically allocates memory for the buffer.
void rb_free(ring_buffer *rb); //Frees the allocated memory for the buffer.

#endif //UART_IRQ_RING_BUFFER_H
