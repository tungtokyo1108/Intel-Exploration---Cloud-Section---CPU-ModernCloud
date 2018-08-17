/*
 * buffer.h
 *
 *  Created on: Aug 17, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_BUFFER_H_
#define QEMU_BUFFER_H_

#include "qemu-common.h"

#define GCC_ATTR_ __attribute__((__unused__, format(printf,1,2)))
#define GCC_FMT_ATTR(n,m) __attribute__((format(printf,n,m)))

typedef struct Buffer Buffer;

struct Buffer {
	char *name;
	size_t capacity;
	size_t offset;
	uint64_t avg_size;
	uint8_t *buffer;
};

void buffer_init(Buffer *buffer, const char *name, ...) {
	GCC_FMT_ATTR(2,3);
}

void buffer_shrink(Buffer *buffer);
void buffer_reserve(Buffer *buffer, size_t len);
void buffer_free(Buffer *buffer);
void buffer_append(Buffer *buffer, const void *data, size_t len);
void buffer_advance(Buffer *buffer, size_t len);

#endif /* QEMU_BUFFER_H_ */
