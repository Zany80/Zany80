#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "SIMPLE/ring_buffer.h"

struct ring_buffer_t {
	size_t capacity;
	char *buf;
	char *begin;
	size_t size;
};

static char *end(ring_buffer_t *buffer) {
	char *e = buffer->begin + buffer->size;
	if (e >= buffer->buf + buffer->capacity) {
		e -= buffer->capacity;
	}
	return e;
}

ring_buffer_t *ring_buffer_new(size_t capacity) {
	if (capacity == 0 || capacity > 128 * 1024 * 1024) {
		return NULL;
	}
	ring_buffer_t *buffer = malloc(sizeof(ring_buffer_t));
	buffer->buf = malloc(capacity);
	buffer->begin = buffer->buf;
	buffer->size = 0;
	buffer->capacity = capacity;
	return buffer;
}

void ring_buffer_free(ring_buffer_t *buffer) {
	free(buffer->buf);
	free(buffer);
}

size_t ring_buffer_available(ring_buffer_t *buffer) {
	return buffer->size;
}

void ring_buffer_resize(ring_buffer_t *buf) {
	char *new_buf = realloc(buf->buf, buf->capacity *= 2);
	if (new_buf == NULL) {
		fputs("[RingBuffer] Out of memory!\n", stderr);
		exit(1);
	}
	buf->begin = buf->begin - buf->buf + new_buf;
	buf->buf = new_buf;
}

void ring_buffer_prepend(ring_buffer_t *buf, char c) {
	if (buf->size + 1 >= buf->capacity) {
		ring_buffer_resize(buf);
	}
	buf->size++;
	buf->begin--;
	if (buf->begin < buf->buf) {
		buf->begin += buf->capacity;
	}
	*buf->begin = c;
}

void ring_buffer_prepend_buf(ring_buffer_t *ringbuf, const char *buf, size_t length) {
	size_t old_size;
	char *old_contents = malloc(old_size = ring_buffer_available(ringbuf));
	ring_buffer_read_buf(ringbuf, old_contents, old_size);
	ring_buffer_append(ringbuf, buf, length);
	ring_buffer_append(ringbuf, old_contents, old_size);
	free(old_contents);
}

bool ring_buffer_append(ring_buffer_t *buf, const char *c, size_t length) {
	if (length + buf->size >= buf->capacity) {
		ring_buffer_resize(buf);
	}
	while (buf->size < buf->capacity && length > 0) {
		*end(buf) = *c++;
		buf->size++;
		length--;
	}
	return length == 0;
}

char ring_buffer_read(ring_buffer_t *buf) {
	// Returning -1 and doing nothing when empty, bug found by test case
	if (buf->size == 0)
		return -1;
	char c = *buf->begin++;
	if (buf->begin == buf->buf + buf->capacity) {
		buf->begin = buf->buf;
	}
	buf->size--;
	return c;
}

// TODO: implement a faster version of this using e.g. memcpy
size_t ring_buffer_read_buf(ring_buffer_t *buffer, char *out, size_t length) {
	size_t r = 0;
	while (length > 0 && buffer->size > 0) {
		*out++ = ring_buffer_read(buffer);
		length--;
		r++;
	}
	return r;
}

char ring_buffer_peek(ring_buffer_t *buffer, int offset) {
	return *(buffer->begin + offset);
}
