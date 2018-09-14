/*
 * seqlock.h
 *
 *  Created on: Sep 14, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_SEQLOCK_H_
#define QEMU_SEQLOCK_H_

#include "qemu/atomic.h"
#include "qemu/thread.h"

typedef struct QemuSeqLock QemuSeqLock;
struct QemuSeqLock {
	unsigned sequence;
};

static inline void seqlock_init(QemuSeqLock *sl) {
	sl->sequence = 0;
}

static inline void seqlock_write_begin(QemuSeqLock *sl) {
	atomic_set(&sl->sequence, sl->sequence + 1);
	smp_wmb();
}

static inline void seqlock_write_end(QemuSeqLock *sl) {
	smp_wmb();
	atomic_set(&sl->sequence, sl->sequence + 1);
}

static inline unsigned seqlock_read_begin(QemuSeqLock *sl) {
	unsigned ret = atomic_read(&sl->sequence);
    smp_rmb();
	return ret & ~1;
}

static inline int seqlock_read_retry(const QemuSeqLock *sl, unsigned start)
{
    smp_rmb();
    return unlikely(atomic_read(&sl->sequence) != start);
}

#endif /* QEMU_SEQLOCK_H_ */
