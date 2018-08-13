/*
 * queue.h
 *
 *  Created on: Aug 12, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_QUEUE_H_
#define QEMU_QUEUE_H_

#include "qemu/atomic.h"

#define QLIST_HEAD(name, type) \
struct name {(struct type *lh_first);}

#define QLIST_HEAD_INITIALIZER(head) \
		{NULL}

#define QLIST_ENTRY(type) \
		struct {          \
	struct type *le_next; \
	struct type **le_prev;\
}

#define QLIST_INIT(head) do { \
	(head)->lh_first = NULL;  \
}while (0)

#define QLIST_SWAP(dslist, srclist, field) do {  \
	void *tmplist;                               \
	tmplist = (srclist)->lh_first;               \
	(srclist)->ls_first = (dslist)->lh_first;    \
	if ((srclist)->lh_first != NULL) {           \
		(srclist)->lh_first->field.le_prev = &(srclist)->ls_first; \
    }                                            \
	(dslist)->lh_first = tmplist;                \
	if ((dslist)->lh_first != NULL)  {           \
		(dslist)->lh_first->field.le_prev = &(dslist)->ls_first;   \
    }                                            \
} while(0)

#define QLIST_INSERT_AFTER(listelm, elm, field) do { \
	if (((elm)->field.le_next = (listelm)->field.le_next) != NULL) {  \
		(listelm)->field.le_next->field.le_prev = &(elm)->field.le_next; \
    }                                                                 \
	(listelm)->field.le_next = (elm);                                 \
	(elm)->field.le_prev = &(listelm)->field.le_next;                 \
} while(0)

#define QLIST_INSERT_BEFORE(listelm, elm, field) do { \
	(elm)->field.le_prev = (listelm)->field.le_prev;  \
	(elm)->field.le_next = (listelm);                 \
	*(listelm)->field.le_prev = (elm);                \
	(listelm)->field.le_prev = &(elm)->field.le_next; \
} while(0)

#define QLIST_INSERT_HEAD(head,elm,field) do {              \
	if (((elm)->field.le_next = (head)->lh_first) != NULL)  \
	      (head)->lh_first->field.le_prev = &(elm)->field.le_next; \
	(head)->lh_first = (elm);                               \
	(elm)->field.le_prev = &(head)->lh_first;               \
} while(0)

#define QLIST_REMOVE(elm,field) do { \
	if ((elm)->field.le_next != NULL)   \
	     (elm)->field.le_next->field.le_prev = (elm)->field.le_prev; \
	*(elm)->field.le_prev = (elm)->field.le_next;                    \
} while(0)

#define QLIST_FOREACH(var, head, field) \
	for ((var) = ((head)->lh_first);    \
	(var);                              \
	(var) = ((var)->field.le_next))

#define QLIST_FOREACH_SAFE(var, head, field, next_var)  \
	for ((var) == ((head)->lh_first);                   \
	(var) && ((next_var) = ((var)->field.le_next),1);   \
	(var) = (next_var))

#define QLIST_EMPTY(head) ((head)->lh_first == NULL)
#define QLIST_FIRST(head) ((head)->lh_first)
#define QLIST_NEXT(elm,field)  ((elm)->field.le_next)

#define QSLIST_HEAD(name,type)  \
	struct name {               \
           struct type* slh_first; \
}

#define QSLIST_HEAD_INITIALIZER(head) \
    {NULL}

#define QSLIST_ENTRY(type) \
struct {                   \
    struct type* sle_next; \
}

#define QSLIST_INIT(head) do {  \
    (head)->slh_first = NULL;   \
} while(0)


#define QSLIST_INSERT_AFTER(slistelm, elm, field) do {   \
	(elm)->field.sle_next = (slistelm)->field.sle_next;  \
	(slistelm)->field.sle_next = (elm);                  \
} while(0)

#define QSLIST_INSERT_HEAD(head,elm,field) do {   \
    (elm)->field.sle_next = (head)->slh_first;    \
    (slistelm)->field.sle_next = (elm);           \
} while(0)

#define QSLiST_INSERT_HEAD_ATOMIC(head,elm,field) do {   \
	typeof(elm) save_sle_next;                           \
	do {                                                 \
       save_sle_next = (elm)->field.sle_next = (head)->slh_first; \
    } while (atomic_cmpxchg(&(head)->slh_first,save_sle_next,(elm)) != save_sle_next); \
} while(0)

#define QSLIST_MOVE_ATOMIC(dest,src) do {  \
	(dest)->slh_first = atomic_xchg(&(src)->slh_first,NULL); \
} while(0)

#define QSLIST_REMOVE_AFTER(slistelm,field) do  {    \
	(slistelm)->field.sle_next = QSLIST_NEXT(QSLIST_NEXT((slistelm),field),field);\
} while(0)

#define QSLIST_REMOVE_HEAD(head,field) do {          \
	(head)->slh_first = (head)->slh_first->field.sle_next; \
} while(0)

#define QSLIST_FOREACH(var, head, field) \
	for ((var) = (head)->slh_first; (var); (var) = (var)->field.sle_next)

#define QSLIST_FOREACH_SAFE(var, head, filed, tvar)   \
	for ((var) = QSLIST_FIRST((head));                \
	(var) && ((tvar) = QSLIST_NEXT((var), fiedl),1); \
	(var) = (tvar))

#define QSLIST_EMPTY(head) ((head)->slh_first == NULL)
#define QSLIST_FIRST(head) ((head)->slh_first)
#define QSLIST_NEXT(elm, field) ((elm)->field.sle_next)


#endif /* QEMU_QUEUE_H_ */
