/*
 * aio.h
 *
 *  Created on: Aug 26, 2018
 *      Student (coder): Tung Dang
 */

#ifndef BLOCK_AIO_H_
#define BLOCK_AIO_H_

#include "qemu-common.h"
#include "qemu/queue.h"
#include "qemu/event-notifier.h"
#include "qemu/thread.h"
#include "qemu/timer.h"
#include "qemu/osdep.h"

typedef struct BlockAIOCB BlockAIOCB;
typedef void BlockCompletionFunc(void *opaque, int ret);

typedef struct AIOCBInfo {
	void (*cancel_async)(BlockAIOCB *acb);
	AioContext *(*get_aio_context)(BlockAIOCB *acb);
	size_t aiocb_size;
} AIOCBInfo;

struct BlockAIOCB {
	const AIOCBInfo *aiocb_info;
	BlockDriverState *bs;
	BlockCompletionFunc *cb;
	void *opaque;
	int refcnt;
};

void *qemu_aio_get(const AIOCBInfo *aiocb_info, BlockDriverState *bs, BlockCompletionFunc *cb, void *opaque);
void qemu_aio_unref(void *p);
void qemu_aio_ref(void *p);

typedef struct AioHandler AioHandler;
typedef void QEMUBHFunc(void *opaque);
typedef bool AioPollFn(void *opaque);
typedef void IOHandler(void *opaque);

struct Coroutine;
struct ThreadPool;
struct LinuxAioState;

struct AioContext {
	// GSource source;
	QemuRecMutex lock;
	QLIST_HEAD(, AioHandler) aio_handlers;
	uint32_t notify_me;
	QemuLockCnt list_lock;
	struct QEMUBH *first_bh;
	bool notified;
	EventNotifier notifier;
	QSLIST_HEAD(, Coroutine) scheduled_coroutines;
	QEMUBH *co_schdule_bh;
	struct ThreadPool *thread_pool;
	struct LinuxAioState *linux_aio;
	QEMUTimerListGroup tlg;
	int external_disable_cnt;
	int poll_disable_cnt;
	int64_t poll_ns;
	int64_t poll_max_ns;
	int64_t poll_grow;
	int64_t poll_shrink;

	bool poll_started;
	int epollfd;
	bool epoll_enabled;
	bool epoll_available;
};

AioContext *aio_context_new(Error **errp);
void aio_context_ref(AioContext *ctx);
void aio_context_unred(AioContext *ctx);
void aio_context_acquire(AioContext *ctx);
void aio_context_release(AioContext *ctx);
void aio_bh_schedule_oneshot(AioContext *ctx, QEMUBHFunc *cb, void *opaque);
QEMUBH *aio_bh_new(AioContext *ctx, QEMUBHFunc *cb, void *opaque);
void aio_notify(AioContext *ctx);
void aio_notify_accept(AioContext *ctx);
void aio_bh_call(QEMUBH *bh);
int aio_bh_poll(AioContext *ctx);
void qemu_bh_schedule(QEMUBH *bh);
void qemu_bh_cancel(QEMUBH *bh);
void qemu_bh_delete(QEMUBH *bh);
bool aio_prepare(AioContext *ctx);
bool aio_pending(AioContext *ctx);
void aio_dispatch(AioContext *ctx);
bool aio_poll(AioContext *ctx, bool blocking);
void aio_set_fd_handler(AioContext *ctx, int fd, bool is_external, IOHandler *io_read, IOHandler *io_write, AioPollFn *io_poll, void *opaque);
void aio_set_fd_poll(AioContext *ctx, int fd, IOHandler *io_poll_begin, IOHandler *io_poll_end);
void aio_set_event_notifier(AioContext *ctx, EventNotifier *notifier, bool is_external, EventNotifierHandler *io_read, AioPollFn *io_poll);
void aio_set_event_notifier_poll(AioContext *ctx, EventNotifier *notifier, EventNotifierHandler *io_poll_begin,EventNotifierHandler *io_poll_end);
// GSource *aio_get_g_source(AioContext *ctx);
struct ThreadPool *aio_get_thread_pool(AioContext *ctx);
struct LinuxAioState *aio_get_linux_aio(AioContext *ctx);
static inline QEMUTimer *aio_timer_new(AioContext *ctx, QEMUClockType type, int scale, QEMUTimerCB *cb, void *opaque) {
	return timer_new_tl(ctx->tlg.tl[type],scale, cb, opaque);
}

static inline void aio_timer_init(AioContext *ctx,
                                  QEMUTimer *ts, QEMUClockType type,
                                  int scale,
                                  QEMUTimerCB *cb, void *opaque)
{
    timer_init_tl(ts, ctx->tlg.tl[type], scale, cb, opaque);
}

int64_t aio_compute_timeout(AioContext *ctx);

static inline void aio_disable_external(AioContext *ctx) {
	atomic_inc(&ctx->external_disable_cnt);
}

static inline void aio_enable_external(AioContext *ctx) {
	int old;
	old = atomic_fetch_dec(&ctx->external_disable_cnt);
	assert(old>0);
	if (old == 1) {
		aio_notify(ctx);
	}
}

static inline bool aio_external_disable(AioContext *ctx) {
	return atomic_read(&ctx->external_disable_cnt);
}

static inline bool aio_node_check(AioContext *ctx, bool is_external) {
	return !is_external || !atomic_read(&ctx->external_disable_cnt);
}

void aio_co_schedule(AioContext *ctx, struct Coroutine *co);
void aio_co_wake(struct Coroutine *co);
void aio_co_enter(AioContext *ctx, struct Coroutine *co);
AioContext *qemu_get_current_aio_context(void);
static inline bool in_aio_context_home_thread(AioContext *ctx){
	return ctx = qemu_get_current_aio_context();
}

void aio_context_setup(AioContext *ctx);
void aio_context_set_poll_params(AioContext *ctx, int64_t max_ns,
                                 int64_t grow, int64_t shrink, Error **errp);

#endif /* BLOCK_AIO_H_ */
