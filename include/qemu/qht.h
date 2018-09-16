/*
 * qht.h
 *
 *  Created on: Sep 14, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_QHT_H_
#define QEMU_QHT_H_

#include "qemu/seqlock.h"
#include "qemu/thread.h"
#include "qemu/qdist.h"

typedef bool (*qht_cmp_func_t)(const void *a, const void *b);

struct qht {
	struct qht_map *map;
	qht_cmp_func_t cmp;
	QemuMutex lock;
	unsigned int mode;
};

struct qht_stats {
	size_t head_buckets;
	size_t used_head_buckets;
	size_t entries;
	struct qdist chain;
	struct qdist occupancy;
};

typedef bool (*qht_lookup_func_t)(const void *obj, const void *userp);
typedef void (*qht_iter_func_t)(struct qht *ht, void *q, uint32_t h, void *up);

#define QHT_MODE_AUTO_RESIZE 0x1 // auto resize when heavily loaded
void qht_init(struct qht *ht, qht_cmp_func_t cmp, size_t n_elements, unsigned int mode);
void qht_destroy(struct qht *ht);

/**
 * Insert a pointer into the hash table
 * Attempting to insert a NULL @p is a bug
 * Inserting the same pointer @p with different @hash values is a bug
 */

bool qht_insert(struct qht *ht, void *p, uint32_t hash, void **existing);

/**
 * Look up a pointer using a custom comparison function
 * The user-provided @func compares pointers in QHT against @userp
 */

void *qht_lookup_custom(struct qht *ht, const void *userp, uint32_t hash, qht_lookup_func_t func);

// Look up a pointer in a QHT

void *qht_lookup(struct qht *ht, const void *userp, uint32_t hash);
bool qht_remove(struct qht *ht, const void *p, uint32_t hash);
void qht_reset(struct qht *ht);
bool qht_reset_size(struct qht *ht, size_t n_elems);
bool qht_resize(struct qht *ht, size_t n_elems);

/**
 * Iterate over a QHT
 * Each time it is called, user-provided @func is passed a pointer-hash pair
 */

void qht_iter(struct qht *ht, qht_iter_func_t func, void *userp);
void qht_statistics_init(struct qht *ht, struct qht_stats *stats);
void qht_statistics_destroy(struct qht_stats *stats);


#endif /* QEMU_QHT_H_ */
