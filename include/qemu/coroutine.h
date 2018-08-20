/*
 * coroutine.h
 *
 *  Created on: Aug 20, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_COROUTINE_H_
#define QEMU_COROUTINE_H_

#include "qemu/queue.h"
#include "qemu/timer.h"

#define coroutine_fn

typedef struct Coroutine Coroutine;
typedef void coroutine_fn CoroutineEntry(void *opaque);
Coroutine *qemu_coroutine_create(CoroutineEntry *entry, void *opaque);
void qemu_coroutine_enter(Coroutine *coroutine);
// void qemu_aio_coroutine_enter(AioContext *ctx, Coroutine *co);
void coroutine_fn qemu_coroutine_yield(void);
Coroutine *coroutine_fn qemu_coroutine_self(void);
bool qemu_in_coroutine(void);
bool qemu_coroutine_entered(Coroutine *co);
struct CoWaitRecord;
struct CoMutex {
	unsigned locked;
	QLIST_HEAD(, CoWaitRecord) from_push, to_pop;
	unsigned handoff, sequence;
	Coroutine *holder;
};

void qemu_co_mutex_init(CoMutex *mutex);
void coroutine_fn qemu_co_mutex_lock(CoMutex *mutex);
void coroutine_fn qemu_co_mutex_unlock(CoMutex *mutex);
typedef struct CoQueue {
	QSIMPLEQ_HEAD(, Coroutine) entries;
} CoQueue;

void qemu_co_queue_init(CoQueue *queue);
#define qemu_co_queue_wait(queue, lock) \
	qemu_co_queue_wait_impl(queue,QEMU_MAKE_LOCKABLE(lock))
// void coroutine_fn qemu_co_queue_wait_impl(CoQueue *queue, QemuLockable *lock);]
bool coroutine_fn qemu_co_queue_next(CoQueue *queue);
void coroutine_fn qemu_co_queue_restart_all(CoQueue *queue);

#define qemu_co_enter_next(queue,lock) \
	qemu_co_enter_next_impl(queue,QEMU_MAKE_LOCKABLE(lock))

bool qemu_co_queue_empty(CoQueue *queue);

typedef struct CoRwlock {
	int pending_writer;
	int reader;
	CoMutex mutex;
	CoMutex queue;
} CoRwlock;

void qemu_co_rwlock_init(CoRwlock *lock);
void qemu_co_rwlock_rdlock(CoRwlock *lock);
void qemu_co_rwlock_wrlock(CoRwlock *lock);
void qemu_co_rwlock_unlock(CoRwlock *lock);
void coroutine_fn qemu_co_sleep_ns(QEMUClockType type, int64_t ns);

#endif /* QEMU_COROUTINE_H_ */
