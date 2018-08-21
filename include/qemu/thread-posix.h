/*
 * thread-posix.h
 *
 *  Created on: Aug 21, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_THREAD_POSIX_H_
#define QEMU_THREAD_POSIX_H_

#include <pthread.h>
#include <semaphore.h>

typedef struct QemuMutex QemuRecMutex;
#define qemu_rec_mutex_destroy qemu_mutex_destroy
#define qemu_rec_mutex_lock qemu_mutex_lock
#define qemu_rec_mutex_trylock qemu_mutex_trylock
#define qemu_rec_mutex_unlock qemu_mutex_unlock

struct QemuMutex {
	pthread_mutex_t lock;
	bool initialized;
};

struct QemuCond {
	pthread_cond_t cond;
	bool initialized;
};

struct QemuSemaphore {
#ifndef CONFIG_SEM_TIMEDWAIT
	pthread_mutex_t lock;
	pthread_cond_t cond;
	unsigned int count;
#else
	sem_t sem;
#endif
	bool initialized;
};

struct QemuEvent {
	unsigned value;
	bool initialized;
};

struct QemuThread {
	pthread_t thread;
};

#endif /* QEMU_THREAD_POSIX_H_ */
