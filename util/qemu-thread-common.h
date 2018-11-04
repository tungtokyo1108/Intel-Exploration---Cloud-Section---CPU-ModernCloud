/*
 * qemu-thread-common.h
 *
 *  Created on: Nov 4, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#ifndef UTIL_QEMU_THREAD_COMMON_H_
#define UTIL_QEMU_THREAD_COMMON_H_

#include "qemu/typedefs.h"
#include "qemu/thread.h"
#include "qemu/thread-posix.h"

static inline void qemu_mutex_post_init(QemuMutex *mutex) {
#ifdef CONFIG_DEBUG_MUTEX
	mutex->file = NULL;
	mutex->line = 0;
#endif
	mutex->initialized = true;
}

static inline void qemu_mutex_pre_lock(QemuMutex *mutex, const char *file, int line ) {
	trace_qemu_mutex_lock(mutex, file, line);
}

static inline void qemu_mutex_post_lock(QemuMutex *mutex, const char *file, int line) {
#ifdef CONFIG_DEBUG_MUTEX
    mutex->file = file;
    mutex->line = line;
#endif

    trace_qemu_mutex_locked(mutex, file, line);
}

static inline void qemu_mutex_pre_unlock(QemuMutex *mutex, const char *file, int line) {
#ifdef CONFIG_DEBUG_MUTEX
    mutex->file = NULL;
    mutex->line = 0;
#endif

    trace_qemu_mutex_unlock(mutex, file, line);
}

#endif /* UTIL_QEMU_THREAD_COMMON_H_ */
