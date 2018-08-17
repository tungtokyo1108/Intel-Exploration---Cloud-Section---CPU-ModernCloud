/*
 * bitmap.h
 *
 *  Created on: Aug 16, 2018
 *      Author: tungdang
 */

#ifndef QEMU_BITMAP_H_
#define QEMU_BITMAP_H_

#include "qemu/bitops.h"
#include <glib.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>

#define BITMAP_FIRST_WORD_MASK(start) (~OUL << ((start) & (BIT_PER_LONG - 1)))
#define BITMAP_LAST_WORD_MASK(nbits) (~OUL >> (-(nbits) & (BIT_PER_LONG - 1)))
#define DECLARE_BITMAP(name,bits) \
	unsigned long name[BITS_TO_LONGS(bits)]
#define small_nbits(nbits) \
	((nbits) <= BITS_PER_LONG)

int slow_bitmap_empty(const unsigned long *bitmap, long bits);
int slow_bitmap_full(const unsigned long *bitmap, long bits);
int slow_bitmap_equal(const unsigned long *bitmap1, const unsigned long *bitmap2, long bits);
void slow_bitmap_complement(unsigned long *dst, const unsigned long *src, long bits);
int slow_bitmap_and(unsigned long *dst, const unsigned long *bitmap1, const unsigned long *bitmap2, long bits);
void slow_bitmap_or(unsigned long *dst, const unsigned long *bitmap1, const unsigned long *bitmap2, long bits);
void slow_bitmap_xor(unsigned long *dst, const unsigned long *bitmap1, const unsigned long *bitmap2, long bits);
int slow_bitmap_andnot(unsigned long *dst, const unsigned long *bitmap1, const unsigned long* bitmap2, long bits);
int slow_bitmap_intersects(const unsigned long *bitmap1, const unsigned long *bitmap2, long bits);
long slow_bitmap_count_one(const unsigned long *bitmap, long nbits);

static inline unsigned long *bitmap_try_new(long nbits) {
	long len = BITS_TO_LONGS(nbits) * sizeof(unsigned long);
	return g_try_malloc0(len);
}

static inline unsigned long *bitmap_new(long nbits) {
	unsigned long *ptr = bitmap_try_new(nbits);
	if (ptr == NULL)
		abort();
	return ptr;
}

static inline void bitmap_zero(unsigned long *dst, long nbits) {
	if (small_nbits(nbits))
	{
		*dst = NULL;
	}
	else
	{
		long len = BITS_TO_LONGS(nbits) * sizeof(unsigned long);
		memset(dst,0,len);
	}
}

static inline void bitmap_fill(unsigned long *dst, long nbits) {
	size_t nlongs = BITS_TO_LONGS(nbits);
	if (!small_nbits(nbits))
	{
		long len = (nlongs - 1) * sizeof(unsigned long);
		memset(dst,0xff,len);
	}
	dst[nlongs - 1] = BITMAP_LAST_WORD_MASK(nbits);
}

static inline void bitmap_copy(unsigned long *dst, const unsigned long *src, long nbits) {
	if (small_nbits(nbits))
	{
		*dst = *src;
	}
	else
	{
		long len = BITS_TO_LONGS(nbits) * sizeof(unsigned long);
		memcpy(dst,src,len);
	}
}

static inline int bitmap_and(unsigned long *dst, const unsigned long *src1, const unsigned long *src2, long nbits) {
	if (small_nbits(nbits))
	{
		return (*dst = *src1 & *src2) != 0;
	}
	return slow_bitmap_and(dst, src1, src2, nbits);
}

static inline void bitmap_or(unsigned long *dst, const unsigned long *src1, const unsigned long *src2, long nbits) {
	if (small_nbits(nbits))
		*dst = *src1 | *src2;
	else
		slow_bitmap_or(dst,src1,src2,nbits);
}

static inline void bitmap_xor(unsigned long *dst, const unsigned long *src1, const unsigned long *src2, long nbits) {
	if (small_nbits(nbits))
		*dst = *src1 ^ *src2;
	else
		slow_bitmap_xor(dst,src1,src2,nbits);
}

static inline int bitmap_andnot(unsigned long *dst, const unsigned long *src1, const unsigned long * src2, long nbits) {
	if (small_nbits(nbits))
		return (*dst = *src1 & ~(*src2)) != 0;
	return slow_bitmap_andnot(dst, src1, src2, nbits);
}

static inline void bitmap_complement(unsigned long *dst, const unsigned long *src, long nbits) {
	if (small_nbits(nbits))
		*dst = ~(*src) & BITMAP_LAST_WORD_MASK(nbits);
	else
		slow_bitmap_complement(dst,src,nbits);
}

#endif /* QEMU_BITMAP_H_ */
