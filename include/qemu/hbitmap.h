/*
 * hbitmap.h
 *
 *  Created on: Aug 20, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_HBITMAP_H_
#define QEMU_HBITMAP_H_

#include "bitops.h"
#include "host-utils.h"

typedef struct HBitmap HBitmap;
typedef struct HBitmapIter HBitmapIter;

#define BITS_PER_LEVEL (BITS_PER_LONG == 32 ? 5 : 6)
#define HBITMAP_LOG_MAX_SIZE (BITS_PER_LONG == 32 ? 34 : 41)
#define HBITMAP_LEVELS ((HBITMAP_LOG_MAX_SIZE / BITS_PER_LEVEL) + 1)

typedef struct HBitmapIter {
	const HBitmap *hb;
	int granularity;
	size_t pos;
	unsigned long cur[HBITMAP_LEVELS];
} HBitmapIter;

HBitmap *hbitmap_alloc(uint64_t size, int granularity);
void hbitmap_truncate(HBitmap *hb, uint64_t size);
bool hbitmap_merge(HBitmap *a, const HBitmap *b);
int hbitmap_granularity(const HBitmap *hb);
uint64_t hbitmap_count(const HBitmap *hb);
void hbitmap_set(HBitmap *hb, uint64_t start, uint64_t count);
void hbitmap_reset(HBitmap *hb, uint64_t start, uint64_t count);
void hbitmap_reset_all(HBitmap *hb);
bool hbitmap_get(const HBitmap *hb, uint64_t item);
bool hbitmap_is_serializable(const HBitmap *hb);
uint64_t hbitmap_serialization_align(const HBitmap *hb);
uint64_t hbitmap_serialization_size(const HBitmap *hb, uint64_t start, uint64_t count);
void hbitmap_serialize_part(const HBitmap *nb, uint8_t *buf, uint64_t start, uint64_t count);
void hbitmap_deserialize_part(HBitmap *hb, uint8_t *buf, uint64_t start, uint64_t count, bool finish);
void hbitmap_deserialize_zeroes(HBitmap *hb, uint64_t start, uint64_t count, bool finish);
void hbitmap_deserialize_ones(HBitmap *hb, uint64_t start, uint64_t count, bool finish);
void hbitmap_deserialize_finish(HBitmap *hb);
char *hbitmap_sha256(const HBitmap *bitmap, Error **errp);
void hbitmap_free(HBitmap *hb);
void hbitmap_iter_init(HBitmapIter *hbi, const HBitmap *hb, uint64_t first);
unsigned long hbitmap_iter_skip_words(HBitmapIter *hbi);
uint64_t hbitmap_next_zero(const HBitmap *hb, uint64_t start);
HBitmap *hbitmap_create_meta(HBitmap *hb, int chunk_size);
void hbitmap_free_meta(HBitmap *hb);
int64_t hbitmap_iter_next(HBitmapIter *hbi);
static inline size_t hbitmap_iter_next_word(HBitmapIter *hbi, unsigned long *p_cur)
{
    unsigned long cur = hbi->cur[HBITMAP_LEVELS - 1];

    if (cur == 0) {
        cur = hbitmap_iter_skip_words(hbi);
        if (cur == 0) {
            *p_cur = 0;
            return -1;
        }
    }

    hbi->cur[HBITMAP_LEVELS - 1] = 0;
    *p_cur = cur;
    return hbi->pos;
}

#endif /* QEMU_HBITMAP_H_ */
