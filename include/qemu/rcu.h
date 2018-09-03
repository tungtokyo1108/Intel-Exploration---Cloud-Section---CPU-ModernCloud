/*
 * rcu.h
 *
 *  Created on: Sep 3, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_RCU_H_
#define QEMU_RCU_H_

#include "qemu/thread.h"
#include "qemu/queue.h"
#include "qemu/atomic.h"
#include "qemu/sys_membarrier.h"
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifdef DEBUG_RCU
#define rcu_assert(args...)    assert(args)
#else
#define rcu_assert(args...)
#endif

extern unsigned long rcu_gp_ctr;
extern QemuEvent rcu_gp_event;
typedef struct rcu_reader_data rcu_reader_data;
typedef struct rcu_reader_data {
	// Data used by both reader and syschronize_rcu
	unsigned long ctr;
	bool waiting;

	unsigned depth; // data used by reader only
	QLIST_ENTRY(rcu_reader_data) node;
} rcu_reader_data;

extern __thread struct rcu_reader_data rcu_reader;
static inline void rcu_reader_lock(void) {
	struct rcu_reader_data *p_rcu_reader = &rcu_reader;
	unsigned ctr;
	if (p_rcu_reader->depth++ > 0)
	{
		return;
	}

	ctr = atomic_read(&rcu_gp_ctr);
	atomic_set(&p_rcu_reader->ctr,ctr);
	smp_mb_placeholder();
}

static inline void rcu_read_unlock(void) {
	struct rcu_reader_data *p_rcu_reader = &rcu_reader;
	assert(p_rcu_reader->depth != 0);
	if (--p_rcu_reader->depth > 0) {
		return;
	}
	// atomic_store_release(&p_rcu_reader->ctr,0);
	smp_mb_placeholder();
	if (unlikely(atomic_read(&p_rcu_reader->waiting))) {
	    atomic_set(&p_rcu_reader->waiting, false);
	    qemu_event_set(&rcu_gp_event);
	}
}

// Reader thread registration

extern void synchronize_rcu(void);
extern void rcu_register_thread(void);
extern void rcu_unregister_thread(void);

extern void rcu_enable_atfork(void);
extern void rcu_disable_atfork(void);

typedef void RCUCBFunc(struct rcu_head *head);
typedef struct rcu_head rcu_head;
typedef struct rcu_head {
	struct rcu_head *next;
	RCUCBFunc *func;
} rcu_head;

extern void call_rcu1(struct rcu_head *head, RCUCBFunc *func);

#define call_rcu(head,func,field) \
	call_rcu1(({                            \
	char __attribute__((unused))            \
	offset_must_be_zero[-offsetof(typeof(*(head)), field)], \
	func_type_invalid = (func) - (void(*)(typeof(head)))(func); \
	&(head)->field;                                             \
	}),                                                         \
	(RCUCBFunc *)(func))

#define g_free_rcu(obj, field) \
    call_rcu1(({                                                         \
        char __attribute__((unused))                                     \
            offset_must_be_zero[-offsetof(typeof(*(obj)), field)];       \
        &(obj)->field;                                                   \
      }),                                                                \
    (RCUCBFunc *)g_free);

#ifdef __cplusplus
}
#endif

#endif /* QEMU_RCU_H_ */
