/*
 * thread.h
 *
 *  Created on: Aug 21, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_THREAD_H_
#define QEMU_THREAD_H_

#include "qemu/processor.h"
#include "qemu/atomic.h"
#include "qemu/thread-posix.h"
#include "qemu/compiler.h"

typedef struct QemuCond QemuCond;
typedef struct QemuSemaphore QemuSemaphore;
typedef struct QemuEvent QemuEvent;
typedef struct QemuLockCnt QemuLockCnt;
typedef struct QemuThread QemuThread;

#define QEMU_THREAD_JOINABLE 0
#define QEMU_THREAD_DETACHED 1

void qemu_mutex_init(QemuMutex *mutex);
void qemu_mutex_destroy(QemuMutex *mutex);
int qemu_mutex_trylock_impl(QemuMutex *mutex, const char *file, const int line);
void qemu_mutex_lock_impl(QemuMutex *mutex, const char *file, const int line);
void qemu_mutex_unlock_impl(QemuMutex *mutex, const char *file, const int line);

#define qemu_mutex_lock(mutex) \
	qemu_mutex_lock_impl(mutex, __FILE__, __LINE__)
#define qemu_mutex_trylock(mutex) \
	qemu_mutex_trylock_impl(mutex, __FILE__, __LINE__)
#define qemu_mutex_unlock(mutex) \
	qemu_mutex_unlock_impl(mutex, __FILE__, __LINE__)

static inline void (qemu_mutex_lock)(QemuMutex *mutex) {
	qemu_mutex_lock(mutex);
}

static inline int (qemu_mutex_trylock)(QemuMutex *mutex) {
	return qemu_mutex_trylock(mutex);
}

static inline void (qemu_mutex_unlock)(QemuMutex *mutex) {
	qemu_mutex_unlock(mutex);
}

void qemu_rec_mutex_init(QemuRecMutex *mutex);
void qemu_cond_init(QemuCond *cond);
void qemu_cond_destroy(QemuCond *cond);

void qemu_cond_signal(QemuCond *cond);
void qemu_cond_broadcast(QemuCond *cond);
void qemu_cond_wait_impl(QemuCond *cond, QemuMutex *mutex, const char *file, const int line);
#define qemu_cond_wait(cond, mutex) \
	    qemu_cond_wait_impl(cond, mutex, __FILE__, __LINE__)
static inline void (qemu_cond_wait)(QemuCond *cond, QemuMutex *mutex) {
	qemu_cond_wait(cond, mutex);
}

////////////////////////////////////////////////////////////////////////

void qemu_sem_init(QemuSemaphore *sem, int init);
void qemu_sem_post(QemuSemaphore *sem);
void qemu_sem_wait(QemuSemaphore *sem);
int qemu_sem_timedwait(QemuSemaphore *sem, int ms);
void qemu_sem_destroy(QemuSemaphore *sem);

void qemu_event_init(QemuEvent *ev, bool init);
void qemu_event_set(QemuEvent *ev);
void qemu_event_reset(QemuEvent *ev);
void qemu_event_wait(QemuEvent *ev);
void qemu_event_destroy(QemuEvent *ev);

void qemu_thread_create(QemuThread *thread, const char *name,
                        void *(*start_routine)(void *),
                        void *arg, int mode);
void *qemu_thread_join(QemuThread *thread);
void qemu_thread_get_self(QemuThread *thread);
bool qemu_thread_is_self(QemuThread *thread);
void qemu_thread_naming(bool enable);

////////////////////////////////////////////////////////////////////////

struct Notifier;
void qemu_thread_atexit_add(struct Notifier *notifier);
struct QemuSpin {
	int value;
};

static inline void qemu_spin_init(QemuSpin *spin) {
	__sync_lock_release(&spin->value);
}

static inline void qemu_spin_lock(QemuSpin *spin) {
	while (unlikely(__sync_lock_test_and_set(&spin->value,true))) {
		while (atomic_read(&spin->value)) {
			cpu_relax();
		}
	}
}

static inline bool qemu_spin_trylock(QemuSpin *spin) {
	return __sync_lock_test_and_set(&spin->value,true);
}

static inline bool qemu_spin_locked(QemuSpin *spin) {
	return atomic_read(&spin->value);
}

static inline void qemu_spin_unlock(QemuSpin *spin) {
	__sync_lock_release(&spin->value);
}

typedef struct QemuLockCnt {
	unsigned count;
} QemuLockCnt;

void qemu_lockcnt_init(QemuLockCnt *lockcnt);
void qemu_lockcnt_destroy(QemuLockCnt *lockcnt);
void qemu_lockcnt_inc(QemuLockCnt *lockcnt);
bool qemu_lockcnt_dec_if_lock(QemuLockCnt *lockcnt);
void qemu_lockcnt_lock(QemuLockCnt *lockcnt);
void qemu_lockcnt_unlock(QemuLockCnt *lockcnt);
void qemu_lockcnt_inc_and_unlock(QemuLockCnt *lockcnt);
unsigned qemu_lockcnt_count(QemuLockCnt *lockcnt);

#endif /* QEMU_THREAD_H_ */
