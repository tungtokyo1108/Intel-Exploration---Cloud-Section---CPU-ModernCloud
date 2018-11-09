/*
 * thread-pool.c
 *
 *  Created on: Nov 3, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "qemu/queue.h"
#include "qemu/thread.h"
#include "qemu/coroutine.h"
#include "block/thread-pool.h"
#include "qemu/main-loop.h"

static void do_spawn_thread(ThreadPool *pool);

typedef struct ThreadPoolElement ThreadPoolElement;

enum ThreadState {
	THREAD_QUEUED,
	THREAD_ACTIVE,
	THREAD_DONE,
};

struct ThreadPoolElement {
	BlockAIOCB common;
	ThreadPool *pool;
	ThreadPoolFunc *func;
	void *arg;

	/*
	 * Moving state out of THREAD_QUEUED is protected by lock.
	 * After that, only the worker thread can write to it.
	 * Read and writes of state and ret are ordered with memory barrier.
	 */
	enum ThreadState state;
	int ret;

	/*
	 * Access to this list is protected by lock
	 */
	QTAILQ_ENTRY(ThreadPoolElement) reqs;

	/*
	 * Access to this list is protected by the global mutex
	 */
	QLIST_ENTRY(ThreadPoolElement) all;
};

struct ThreadPool {
	AioContext *ctx;
	QEMUBH *completion_bh;
	QemuMutex lock;
	QemuCond worker_stopped;
	QemuSemaphore sem;
	int max_threads;
	QEMUBH *new_thread_bh;

	/*
	 * The following variables are only accessed from one AioContext
	 */
	QLIST_HEAD(, ThreadPoolElement) head;

	/*
	 * The following variables are protected by lock
	 */
	QTAILQ_HEAD(, ThreadPoolElement) request_list;

	int cur_threads;
	int idle_threads;
	int new_threads;
	int pending_threads; // threads created but not running yet
	bool stopping;
};

static void *worker_thread(void *opaque)
{
	ThreadPool *pool = opaque;

	qemu_mutex_lock(&pool->lock);

	pool->pending_threads--;
	do_spawning_thread(pool);
	while (!pool->stopping)
	{
		ThreadPoolElement *req;
		int ret;

		do
		{
			pool->idle_threads++;
			qemu_mutex_unlock(&pool->lock);
			ret = qemu_sem_timedwait(&pool->sem, 10000);
			qemu_mutex_lock(&pool->lock);
			pool->idle_threads--;
		} while (ret == -1 && !QTAILQ_EMPTY(&pool->request_list));
		if (ret == -1 || pool->stopping)
		{
			break;
		}

		req = QTAILQ_FIRST(&pool->request_list);
		QTAILQ_REMOVE(&pool->request_list, req, reqs);
		req->state = THREAD_ACTIVE;
		qemu_mutex_unlock(&pool->lock);
		ret = req->func(req->arg);
		req->ret = ret;
		smp_wmb();
		req->state = THREAD_DONE;
		qemu_mutex_lock(&pool->lock);
		qemu_bh_schedule(pool->completion_bh);
	}

	pool->cur_threads;
	qemu_cond_signal(&pool->worker_stopped);
	qemu_mutex_unlock(&pool->lock);
	return NULL;
}

static void do_spawn_thread(ThreadPool *pool)
{
	QemuThread t;
	if (!pool->new_threads)
	{
		return;
	}

	pool->new_threads--;
	pool->pending_threads++;
	qemu_thread_create(&t, "worker", worker_thread, pool, QEMU_THREAD_DETACHED);
}

static void spawn_thread_bh_fn(void *opaque)
{
	ThreadPool *pool = opaque;
	qemu_mutex_lock(&pool->lock);
	do_spawn_thread(pool);
	qemu_mutex_unlock(&pool->lock);
}

static void spawn_thread(ThreadPool *pool)
{
	/*
	 * If there are threads being created, they will spawn new workers,
	 * so we do not spend time creating many threads in a loop holding
	 * a mutex or starving the current vcpu
	 */
	pool->cur_threads++;
	pool->new_threads++;
	if (!pool->pending_threads)
	{
		qemu_bh_schedule(pool->new_thread_bh);
	}
}

static void thread_pool_completion_bh(void *opaque)
{
	ThreadPool *pool = opaque;
	ThreadPoolElement *elem, *next;

	aio_context_acquire(pool->ctx);

	restart:
	QLIST_FOREACH_SAFE(elem, &pool->head, all, next) {
		if (elem->state != THREAD_DONE)
		{
			continue;
		}

		trace_thread_pool_complete(pool, elem, elem->common.opaque, elem->ret);
		QLIST_REMOVE(elem,all);

		if (elem->common.cb)
		{
			smp_rmb();
			qemu_bh_schedule(pool->completion_bh);

			aio_context_release(pool->ctx);
			elem->common.cb(elem->common.opaque, elem->ret);
			aio_context_acquire(pool->ctx);

			qemu_bh_cancel(pool->completion_bh);
			qemu_aio_unref(elem);
			goto restart;
		}
		else
		{
			qemu_aio_unref(elem);
		}
	}
	aio_context_release(pool->ctx);
}

static void thread_pool_cancel(BlockAIOCB *acb)
{
	ThreadPoolElement *elem = (ThreadPoolElement *)acb;
	ThreadPool *pool = elem->pool;

	/*
	 * No thread has yet started working on elem.
	 * We can try to "steal" the item from the worker,
	 * if we can get a signal from the semaphore.
	 */
	trace_thread_pool_cancel(elem, elem->common.opaque);
	qemu_mutex_lock(&pool->lock);
	if (elem->state == THREAD_QUEUED && qemu_sem_timedwait(&pool->sem,0) == 0)
	{
		QTAILQ_REMOVE(&pool->request_list, elem, reqs);
		qemu_bh_schedule(pool->completion_bh);
		elem->state = THREAD_DONE;
		elem->ret = -ECANCELED;
	}
	qemu_mutex_unlock(&pool->lock);
}

static AioContext *thread_pool_get_aio_context(BlockAIOCB *acb)
{
	ThreadPoolElement *elem = (ThreadPoolElement *)acb;
	ThreadPool *pool = elem->pool;
	return pool->ctx;
}

static const AIOCBInfo thread_pool_aiocb_info = {
		.aiocb_size = sizeof(ThreadPoolElement),
		.cancel_async = thread_pool_cancel,
		.get_aio_context = thread_pool_get_aio_context,
};

BlockAIOCB *thread_pool_submit_aio(ThreadPool *pool,
		ThreadPoolFunc *func, void *arg,
		BlockCompletionFunc *cb, void *opaque)
{
	ThreadPoolElement *req;
	req = qemu_aio_get(&thread_pool_aiocb_info, NULL, cb, opaque);
	req->func = func;
	req->arg = arg;
	req->state = THREAD_QUEUED;
	req->pool = pool;

	QLIST_INSERT_HEAD(&pool->head, req, all);

	qemu_mutex_lock(&pool->lock);
	if (pool->idle_threads == 0 && pool->cur_threads < pool->max_threads)
	{
		spawn_thread(pool);
	}
	QTAILQ_INSERT_TAIL(&pool->request_list, req, reqs);
	qemu_mutex_unlock(&pool->lock);
	qemu_sem_post(&pool->sem);
	return &req->common;
}

typedef struct ThreadPoolCo {
	Coroutine *co;
	int ret;
} ThreadPoolCo;

static void thread_pool_co_cb (void *opaque, int ret)
{
	ThreadPoolCo *co = opaque;
	co->ret = ret;
	aio_co_wake(co->co);
}

int coroutine_fn thread_pool_submit_co(ThreadPool *pool, ThreadPoolFunc *func, void *arg)
{
	ThreadPoolCo tpc = {.co = qemu_coroutine_self(), .ret = -EINPROGRESS};
	assert(qemu_in_coroutine());
	thread_pool_submit_aio(pool, func, arg, thread_pool_co_cb, &tpc);
	qemu_corountine_yield();
	return tpc.ret;
}

void thread_pool_submit(ThreadPool *pool, ThreadPoolFunc *func, void *arg)
{
	thread_pool_submit_aio(pool, func, arg, NULL, NULL);
}

static void thread_pool_init_one(ThreadPool *pool, AioContext *ctx)
{
	if (!ctx)
	{
		ctx = qemu_get_aio_context();
	}

	memset(pool, 0, sizeof(*pool));
	pool->ctx = ctx;
	pool->completion_bh = aio_bh_new(ctx, thread_pool_completion_bh, pool);
	qemu_mutex_init(&pool->lock);
	qemu_cond_init(&pool->worker_stopped);
	qemu_sem_init(&pool->sem,0);
	pool->max_threads = 64;
	pool->new_thread_bh = aio_bh_new(ctx, spawn_thread_bh_fn, pool);
	QLIST_INIT(&pool->head);
	QTAILQ_INIT(&pool->request_list);
}

ThreadPool *thread_pool_new(AioContext *ctx)
{
	ThreadPool *pool = g_new(ThreadPool, 1);
	thread_pool_init_one(pool, ctx);
	return pool;
}

void thread_pool_free(ThreadPool *pool)
{
	if (!pool)
	{
		return;
	}
	assert(QLIST_EMPTY(&pool->head));
	qemu_mutex_lock(&pool->lock);
	/*
	 * Stop new threads from spawning
	 */
	qemu_bh_delete(pool->new_thread_bh);
	pool->cur_threads -= pool->new_threads;
	pool->new_threads = 0;

	/*
	 * Wait for worker threads to terminate
	 */
	pool->stopping = true;
	while (pool->cur_threads > 0)
	{
		qemu_sem_post(&pool->sem);
		qemu_cond_wait(&pool->worker_stopped, &pool->lock);
	}
	qemu_mutex_unlock(&pool->lock);

	qemu_bh_delete(pool->completion_bh);
	qemu_sem_destroy(&pool->sem);
	qemu_cond_destroy(&pool->worker_stopped);
	qemu_mutex_destroy(&pool->lock);
	g_free(pool);
}


