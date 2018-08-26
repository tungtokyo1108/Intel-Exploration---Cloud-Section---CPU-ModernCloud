/*
 * aio-wait.h
 *
 *  Created on: Aug 26, 2018
 *      Student (coder): Tung Dang
 */

#ifndef BLOCK_AIO_WAIT_H_
#define BLOCK_AIO_WAIT_H_
#include "block/aio.h"

typedef struct {
	unsigned num_waiters;
} AioWait;

#define AIO_WAIT_WHILE(wait, ctx, cond) ({ \
	bool waited_ = false; \
	bool busy_ = true;    \
	AioWait *wait_ = (wait); \
	AioWait *ctx_ = (ctx);   \
	if (in_aio_context_home_thread(ctx_)) { \
		while ((cond) || busy_) {           \
			busy = aio_poll(ctx_,(cond));   \
			waited_ |= !!(cond) | busy_;    \
		}\
	}\ else {                               \
		assert(qemu_get_current_aio_context() == qemu_get_aio_context()); \
		atomic_inc(&wait_->num_waiters);    \
		while (busy_) {                     \
			if ((cond)) {                   \
				waited_ = busy_ = true;     \
				aio_context_release(ctx_);  \
				aio_poll(qemu_get_aio_context(), true);                   \
				aio_context_acquire(ctx_);  \
			}\ else {                       \
				busy_ = aio_poll(ctx_, false);                            \
				waiter_ |= busy_;           \
			}\
		}\
		atomic_dec(&wait_->num_waiters);    \
	}\
	waited_;\
})

void aio_wait_kick(AioWait *wait);
void aio_wait_bh_oneshot(AioContext *ctx, QEMUBHFunc *cb, void *opaque);

#endif /* BLOCK_AIO_WAIT_H_ */
