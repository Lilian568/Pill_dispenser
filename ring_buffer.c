#include <stdio.h>
#include <stdlib.h>
#include "ring_buffer.h"

void rb_init(ring_buffer *rb, uint8_t *buffer, int size) {
    rb->buffer = buffer;
    rb->size = size;
    rb->head = 0;
    rb->tail = 0;
}

bool rb_full(ring_buffer *rb) {
    return (rb -> head +1) % rb->size == rb->tail;
}

bool rb_empty(ring_buffer *rb) {
    return rb->head == rb->tail;

}

bool rb_put(ring_buffer *rb, uint8_t data) {
    int nh =(rb-> head +1) % rb->size;
    if (nh == rb->tail) return false;

    rb -> buffer[rb->head] = data;
    rb -> head = nh;
    return true;
}

uint8_t rb_get(ring_buffer *rb) {
    uint8_t value = rb -> buffer[rb->tail];
    if(rb -> size == rb->tail) {
        rb->tail = (rb->tail + 1) % rb->size;
    }
    return value;
}
void rb_alloc(ring_buffer *rb, int size) {
    uint8_t *buffer = calloc(size, sizeof(uint8_t));
    rb_init(rb, buffer, size);
}
void rb_free(ring_buffer *rb) {
    free(rb->buffer);
}