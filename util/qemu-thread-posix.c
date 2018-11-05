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

void qemu_sem_post(QemuSemaphore *sem)
{
	int rc;
	assert(sem->initialized);
#ifndef CONFIG_SEM_TIMEDWAIT
	pthread_mutex_lock(&sem->lock);
	if (sem->count == UINT_MAX)
	{
		rc = EINVAL;
	}
	else
	{
		sem->count++;
		rc = pthread_cond_signal(&sem->cond);
	}
	pthread_mutex_unlock(&sem->lock);
	if (rc != 0)
	{
		error_exit(rc, __func__);
	}
#else
	rc = sem_post(&sem->sem);
	    if (rc < 0) {
	        error_exit(errno, __func__);
	}
#endif
}

static void compute_abs_deadline(struct timespec *ts, int ms)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	ts->tv_nsec = tv.tv_usec * 1000 + (ms % 1000) * 1000000;
	ts->tv_sec = tv.tv_sec + ms/1000;
	if (ts->tv_nsec >= 1000000000)
	{
		ts->tv_sec++;
		ts->tv_nsec -= 1000000000;
	}
}

int qemu_sem_timedwait(QemuSemaphore *sem, int ms)
{
	int rc;
	struct timespec ts;
	assert(sem->initialized);
#ifndef CONFIG_SEM_TIMEDWAIT
	rc = 0;
	compute_abs_deadline(&ts, ms);
	pthread_mutex_lock(&sem->lock);
	while (sem->count == 0)
	{
		rc = pthread_cond_timedwait(&sem->cond, &sem->lock, &ts);
		if (rc == ETIMEDOUT)
		{
			break;
		}
		if (rc != 0)
		{
			error_exit(rc, __func__);
		}
	}
	if (rc != ETIMEDOUT)
	{
		--sem->count;
	}
	pthread_mutex_unlock(&sem->lock);
	return (rc == ETIMEDOUT ? -1 : 0);
#else
	if (ms <= 0) {
	        /* This is cheaper than sem_timedwait.  */
	        do {
	            rc = sem_trywait(&sem->sem);
	        } while (rc == -1 && errno == EINTR);
	        if (rc == -1 && errno == EAGAIN) {
	            return -1;
	        }
	    } else {
	        compute_abs_deadline(&ts, ms);
	        do {
	            rc = sem_timedwait(&sem->sem, &ts);
	        } while (rc == -1 && errno == EINTR);
	        if (rc == -1 && errno == ETIMEDOUT) {
	            return -1;
	        }
	    }
	    if (rc < 0) {
	        error_exit(errno, __func__);
	    }
	return 0;
#endif
}

void qemu_sem_wait(QemuSemaphore *sem)
{
	int rc;
	assert(sem->initialized);
#ifndef CONFIG_SEM_TIMEDWAIT
	pthread_mutex_lock(&sem->lock);
	while (sem->count == 0)
	{
		rc = pthread_cond_wait(&sem->cond, &sem->lock);
		if (rc != 0)
		{
			error_exit(rc, __func__);
		}
	}
	--sem->count;
	pthread_mutex_unlock(&sem->lock);
#else
	do {
	        rc = sem_wait(&sem->sem);
	    } while (rc == -1 && errno == EINTR);
	    if (rc < 0) {
	        error_exit(errno, __func__);
	}
#endif
}

#ifdef __linux__
#include "qemu/futex.h"
#else
static inline void qemu_futex_wake(QemuEvent *ev, int n)
{
	assert(ev->initialized);
	pthread_mutex_lock(&ev->lock);
	if (n == 1)
	{
		pthread_cond_signal(&ev->cond);
	}
	else
	{
		pthread_cond_broadcast(&ev->cond);
	}
	pthread_mutex_unlock(&ev->lock);
}

static inline void qemu_futex_wait(QemuEvent *ev, unsigned val)
{
	assert(ev->initialized);
	pthread_mutex_lock(&ev->lock);
	if (ev->value == val)
	{
		pthread_cond_wait(&ev->cond, &ev->lock);
	}
	pthread_mutex_unlock(&ev->lock);
}
#endif

#define EV_SET         0
#define EV_FREE        1
#define EV_BUSY       -1

void qemu_event_init(QemuEvent *ev, bool init)
{
#ifndef __linux__
	pthread_mutex_init(&ev->lock, NULL);
	pthread_cond_init(&ev->cond, NULL);
#endif

	ev->value = (init ? EV_SET : EV_FREE);
	ev->initialized = true;
}

void qemu_event_destroy(QemuEvent *ev)
{
	assert(ev->initialized);
	ev->initialized = false;
#ifndef __linux__
    pthread_mutex_destroy(&ev->lock);
    pthread_cond_destroy(&ev->cond);
#endif
}

