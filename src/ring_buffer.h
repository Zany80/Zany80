/// Simple ring buffer (see https://en.wikipedia.org/wiki/Circular_buffer)
/// Maximum size of 128MB.

#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef struct ring_buffer_t ring_buffer_t;

#ifdef __cplusplus
extern "C" {
#endif

ring_buffer_t *ring_buffer_new(size_t size);
void ring_buffer_free(ring_buffer_t *ring_buffer);
size_t ring_buffer_available(ring_buffer_t *ring_buffer);
bool ring_buffer_append(ring_buffer_t *ring_buffer, const char *c, size_t length);
void ring_buffer_prepend(ring_buffer_t *ring_buffer, char c);
void ring_buffer_prepend_buf(ring_buffer_t *ring_buffer, const char *buf, size_t length);
size_t ring_buffer_read_buf(ring_buffer_t *ring_buffer, char *buf, size_t length);
char ring_buffer_read(ring_buffer_t *ring_buffer);
char ring_buffer_peek(ring_buffer_t *buffer, int offset);

#ifdef __cplusplus
}
#endif
