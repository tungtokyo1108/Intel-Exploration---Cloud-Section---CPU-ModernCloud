/*
 * thread-pool.h
 *
 *  Created on: Nov 3, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#ifndef BLOCK_THREAD_POOL_H_
#define BLOCK_THREAD_POOL_H_

#include "block/block.h"
#include "block/aio.h"

typedef int ThreadPoolFunc(void *opaque);
typedef struct ThreadPool ThreadPool;
ThreadPool *thread_pool_new(struct AioContext *ctx);
void thread_pool_free(ThreadPool *pool);

BlockAIOCB *thread_pool_submit_aio(ThreadPool *pool, ThreadPoolFunc *func, void* arg,
		                           BlockCompletionFunc *cb, void *opaque);
int coroutine_fn thread_pool_submit_co(ThreadPool *pool, ThreadPoolFunc *func, void *arg);
void thread_pool_submit(ThreadPool *pool, ThreadPoolFunc *func, void *arg);

#endif /* BLOCK_THREAD_POOL_H_ */
