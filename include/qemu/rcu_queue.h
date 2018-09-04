/*
 * rcu_queue.h
 *
 *  Created on: Sep 4, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_RCU_QUEUE_H_
#define QEMU_RCU_QUEUE_H_

#include "qemu/queue.h"
#include "qemu/atomic.h"

#ifdef __cplusplus
extern "C" {
#endif

// Access method

#define QLIST_EMPTY_RCU(head) (atomic_rcu_read(&(head)->lh_first) == NULL)
#define QLIST_FIRST_RCU(head) (atomic_rcu_read(&(head)->lh_first))
#define QLIST_NEXT_RCU(elm,field) (atomic_rcu_read(&(elm)->field.le_next))

/**
 * The including of a read/write memory barrier to the volatile access.
 */

#define QLIST_INSERT_AFTER_RCU(listelm,elm,field) do {           \
	(elm)->field.le_next = (listelm)->field.le_next;             \
	(elm)->field.le_prev = &(listelm)->field.le_next;            \
	atomic_rcu_set(&(listelm)->field.le_next,(elm));             \
	if ((elm)->field.le_next != NULL) {                          \
		(elm)->field.le_next->field.le_prev =                    \
		&(elm)->field.le_next;                                   \
	}                                                            \
} while (0)

#define QLIST_INSERT_BEFORE_RCU(listelm,elm,field) do {          \
	(elm)->field.le_prev = (listelm)->field.le_prev;             \
	(elm)->field.le_next = (listelm);                            \
	atomic_rcu_set((listelm)->field.le_prev,(elm));              \
	(listelm)->field.le_prev = &(elm)->field.le_next;            \
} while (0)

#define QLIST_INSERT_HEAD_RCU(head,elm,field) do {               \
	(elm)->field.le_prev = &(head)->lh_first;                    \
	(elm)->field.le_next = (head)->lh_first;                     \
	atomic_rcu_set((&(head)->lh_first),(elm));                   \
	if ((elm)->field.le_next != NULL) {                          \
		(elm)->field.le_next->field.le_prev =                    \
		&(elm)->field.le_next;                                   \
	}                                                            \
} while (0)

#define QLIST_REMOVE_RCU(elm,field) do {                         \
	if ((elm)->field.le_next != NULL)  {                         \
		(elm)->field.le_next->field.le_prev =                    \
		(elm)->field.le_prev;                                    \
	}                                                            \
	*(elm)->field.le_prev = (elm)->field.le_next;                \
} while (0)

/* List traversal must occur within an RCU critical section.  */
#define QLIST_FOREACH_RCU(var, head, field)                 \
        for ((var) = atomic_rcu_read(&(head)->lh_first);    \
                (var);                                      \
                (var) = atomic_rcu_read(&(var)->field.le_next))

#define QLIST_FOREACH_SAFE_RCU(var, head, field, next_var)           \
    for ((var) = (atomic_rcu_read(&(head)->lh_first));               \
      (var) &&                                                       \
          ((next_var) = atomic_rcu_read(&(var)->field.le_next), 1);  \
          (var) = (next_var))

#ifdef __cplusplus
}
#endif

#endif /* QEMU_RCU_QUEUE_H_ */
