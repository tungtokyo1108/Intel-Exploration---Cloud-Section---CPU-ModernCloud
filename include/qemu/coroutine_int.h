/*
 * coroutine_int.h
 *
 *  Created on: Aug 20, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_COROUTINE_INT_H_
#define QEMU_COROUTINE_INT_H_

#include "qemu/queue.h"
#include "qemu/coroutine.h"

#define COROUTINE_STACK_SIZE (1 << 20)
typedef struct Coroutine Coroutine;

typedef enum {
	COROUTINE_YIELD = 1,
	COROUTINE_TERMINATE = 2,
	COROUTINE_ENTER = 3,
} CoroutineAction;

typedef struct Coroutine {
	CoroutineEntry *entry;
	void *entry_arg;
	Coroutine *caller;
	QSLIST_ENTRY(Coroutine) pool_next;
	size_t locks_held;

	const char *scheduled;
	QSIMPLEQ_ENTRY(Coroutine) co_queue_next;
	QSIMPLEQ_HEAD(, Coroutine) co_queue_wakeup;
	QLIST_ENTRY(Coroutine) co_scheduled_next;
} Coroutine;

Coroutine *qemu_coroutine_new(void);
void qemu_coroutine_delete(Coroutine *co);
CoroutineAction qemu_coroutine_switch(Coroutine *from, Coroutine *to, CoroutineAction action);

#endif /* QEMU_COROUTINE_INT_H_ */
