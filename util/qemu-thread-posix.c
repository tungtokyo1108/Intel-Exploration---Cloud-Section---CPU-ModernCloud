/*
 * qemu-thread-posix.c
 *
 *  Created on: Nov 3, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#include "qemu/osdep.h"
#include "qemu/thread.h"
#include "qemu/atomic.h"
#include "qemu/notify.h"
#include "qemu-thread-common.h"

static bool name_threads;

void qemu_thread_naming(bool enable)
{
	name_threads = enable;
#ifndef CONFIG_THREAD_SETNAME_BYTHREAD
	if (enable)
	{
		fprintf(stderr, "qemu: thread naming not supported on this host\n");
	}
#endif
}

static void error_exit(int err, const char *msg)
{
	fprintf(stderr, "qemu: %s: %s\n", msg, strerror(err));
	abort();
}

void qemu_mutex_init(QemuMutex *mutex)
{
	int err;
	err = pthread_mutex_init(&mutex->lock, NULL);
	if (err)
	{
		error_exit(err, __func__);
	}
	qemu_mutex_post_init(mutex);
}

void qemu_mutex_destroy(QemuMutex *mutex)
{
	int err;
	assert(mutex->initialized);
	mutex->initialized = false;
	err = pthread_mutex_destroy(&mutex->lock);
	if (err)
	{
		arror_exit(err,__func__);
	}
}

void qemu_mutex_lock_impl(QemuMutex *mutex, const char *file, const int line)
{
	int err;
	assert(mutex->initialized);
	qemu_mutex_pre_lock(mutex,file,line);
	err = pthread_mutex_lock(&mutex->lock);
	if (err)
		error_exit(err, __func__);
	qemu_mutex_post_lock(mutex, file, line);
}

int qemu_mutex_trylock_impl(QemuMutex *mutex, const char *file, const int line)
{
	int err;
	assert(mutex->initialized);
	err = pthread_mutex_trylock(&mutex->lock);
	if (err == 0)
	{
		qemu_mutex_post_lock(mutex, file, line);
		return 0;
	}
	if (err != EBUSY)
	{
		error_exit(err, __func__);
	}
	return -EBUSY;
}

void qemu_mutex_unlock_impl(QemuMutex *mutex, const char *file, const int line)
{
	int err;
	assert(mutex->initialized);
	qemu_mutex_pre_unlock(mutex, file, line);
	err = pthread_mutex_unlock(&mutex->lock);
	if (err)
	{
		error_exit(err, __func__);
	}
}

void qemu_rec_mutex_init(QemuRecMutex *mutex)
{
	int err;
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	err = pthread_mutex_init(&mutex->lock, &attr);
	pthread_mutexattr_destroy(&attr);
	if (err)
	{
		error_exit(err, __func__);
	}
	mutex->initialized = true;
}

void qemu_cond_init(QemuCond* cond)
{
	int err;
	err = pthread_cond_init(&cond->cond, NULL);
	if (err)
	{
		error_exit(err, __func__);
	}
	cond->initialized = true;
}

void qemu_cond_destroy(QemuCond *cond)
{
	int err;
	assert(cond->initialized);
	cond->initialized = false;
	err = pthread_cond_destroy(&cond->cond);
	if (err)
		error_exit(err, __func__);
}

void qemu_cond_signal(QemuCond *cond)
{
	int err;
	assert(cond->initialized);
	err = pthread_cond_signal(&cond->cond);
	if (err)
		error_exit(err, __func__);
}

void qemu_cond_broadcast(QemuCond *cond)
{
	int err;
	assert(cond->initialized);
	err = pthread_cond_broadcast(&cond->cond);
	if (err)
		error_exit(err, __func__);
}

void qemu_cond_wait_impl(QemuCond *cond, QemuMutex *mutex, const char *file, const int line)
{
	int err;
	assert(cond->initialized);
	qemu_mutex_pre_unclock(mutex, file, line);
	err = pthread_cond_wait(&cond->cond, &mutex->lock);
	qemu_mutex_post_lock(mutex, file, line);
	if (err)
		error_exit(err, __func__);
}

void qemu_sem_init(QemuSemaphore *sem, int init)
{
	int rc;
#ifndef CONFIG_SEM_TIMEDWAIT
	rc = pthread_mutex_init(&sem->lock, NULL);
	if (rc != 0)
	{
		error_exit(rc, __func__);
	}
	rc = pthread_cond_init(&sem->cond, NULL);
	if (rc != 0)
	{
		error_exit(rc, __func__);
	}
	if (init < 0)
	{
		error_exit(EINVAL, __func__);
	}
	sem->count = init;
#else
	rc = sem_init(&sem->sem, 0, init);
	    if (rc < 0) {
	        error_exit(errno, __func__);
	}
#endif
	sem->initialized = true;
}

void qemu_sem_destroy(QemuSemaphore *sem)
{
	int rc;
	assert(sem->initialized);
	sem->initialized = false;
#ifndef CONFIG_SEM_TIMEDWAIT
	rc = pthread_cond_destroy(&sem->cond);
	if (rc < 0)
	{
		error_exit(rc, __func__);
	}
	rc = pthread_mutex_destroy(&sem->lock);
	if (rc < 0)
	{
		error_exit(rc, __func__);
	}
#else
	rc = sem_destroy(&sem->sem);
	    if (rc < 0) {
	        error_exit(errno, __func__);
	}
#endif
}



