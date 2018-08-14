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

#define QSIMPLEQ_HEAD(name,type)     \
	struct name {                    \
	       struct type *sqh_first;   \
	       struct type **sqh_last;   \
}

#define QSIMPLEQ_HEAD_INITIALIZER(head) \
		{NULL, &(head).sqh_first}

#define QSIMPLEQ_ENTRY(type) \
		struct {            \
	           struct type *sqe_next; \
}

#define QSIMPLEQ_INIT(head) do {       \
	(head)->sqh_first = NULL;         \
	(head)->sqh_last = &(head)->sqh_first; \
} while(0)

#define QSIMPLEQ_INSERT_HEAD(head,elm,field) do {    \
	if (((elm)->field.sqe_next = (head)->sqe_first) == NULL)  \
	    (head)->sqh_last = &(elm)->field.sqe_next;            \
	(head)->sqh_first = (elm);                                \
} while(0)

#define QSIMPLEQ_INSERT_TAIL(head,elm,field) do {    \
	(elm)->field.sqe_next = NULL;                    \
	*(head)->sqh_last = (elm);                       \
	(head)->sqh_last = &(elm)->field.sqe_next;       \
} while(0)

#define QSIMPLEQ_INSERT_AFTER(head,listelm,elm,field) do {  \
	if (((elm)->field.sqe_next = (listelm)->field.sqe_next) == NULL)  \
	    (head)->sqh_last = &(elm)->field.sqe_next;                    \
	(listelm)->field.sqe_next = (elm);                                \
} while(0)

#define QSIMPLEQ_REMOVE_HEAD(head,field) do {                \
	if (((head)->sqh_first = (head)->sqh_first->field.sqe_next) == NULL) \
	    (head)->sqh_last = &(head)->sqh_first;                           \
} while(0)

#define QSIMPLEQ_SPLIT_AFTER(head,elm,field,removed) do {    \
	QSIMPLEQ_INIT(removed);                                  \
	if (((removed)->sqh_first = (head)->sqh_first) != NULL) {\
	    if(((head)->sqh_first = (elm)->field.sqe_next) == NULL) {  \
	    	(head)->sqh_last = &(head)->sqh_first;                 \
	    }                                                          \
		(removed)->sqh_last = &(elm)->field.sqe_next;              \
		(elm)->field.sqe_next = NULL;                              \
    }                                                              \
} while(0)

#define QSIMPLEQ_REMOVE(head,elm,type,field) do {            \
	if ((head)->sqh_first == (elm)) {                        \
		QSIMPLEQ_REMOVE_HEAD((head),field);                  \
	} else {                                                 \
		struct type *curelm = (head)->sqh_first;             \
		while (curelm->field.sqe_next != (elm))              \
		       curelm = curelm->field.sqe_next;              \
		if ((curelm->field.sqe_next = curelm->field.sqe_next->field.sqe_next) == NULL)  \
		     (head)->sqh_last = &(curelm)->field.sqe_next;   \
	}                                                        \
} while(0)

#define QSIMPLEQ_FOREACH(var,head,field)  \
	for ((var) = ((head)->sqh_first);     \
	    (var);                            \
        (var) = ((var)->field.sqe_next);  \
)

#define QSIMPLEQ_FOREACH_SAFE(var,head,field,next) \
	for ((var) = ((head)->sqh_first);              \
	    (var) && ((next == ((var)->field.sqe_next)),1); \
	    (var) = (next);                            \
)

#define QSIMPLEQ_CONTACT(head1,head2) do {         \
	if (!QSIMPLEQ_EMPTY((head2)))  {               \
		*(head1)->sqh_last = (head2)->sqh_first;   \
		(head1)->sqh_last = (head2)->sqh_last;     \
		QSIMPLEQ_INIT((head2));                    \
	}                                              \
} while(0)

#define QSIMPLEQ_PREPEND(head1,head2) do {         \
	if (!QSIMPLEQ_EMPTY((head2))) {                \
		*(head2)->sqh_last = (head1)->sqh_first;   \
		(head1)->sqh_first = (head2)->sqh_first;   \
		QSIMPLEQ_INIT((head2));                    \
	}                                              \
} while(0)

#define QSIMPLEQ_LAST(head,type,filed)             \
	(QSIMPLEQ_EMPTY((head))) ?                     \
			NULL : ((struct type*)(void*)((char*)((head)->sqh_last) - offset(struct type, field))) \

#define QSIMPLEQ_EMPTY(head) ((head)->sqh_first == NULL)
#define QSIMPLEQ_FIRST(head) ((head)->sqh_first)
#define QSIMPLEQ_NEXT(elm,field) ((elm)->field.sqe_next)

#define Q_TAILQ_HEAD(name,type,qual) \
	struct name {                    \
	       qual type *tqh_first;     \
	       qual type *qual *tqh_last; \
	}

#define QTAILQ_HEAD(name,type) Q_TAIL_HEAD(name, struct type)
#define QTAILQ_HEAD_INITIALIZER(head) \
        {NULL, &(head).tqh_first}

#define Q_TAILQ_ENTRY(type,qual)      \
		struct {                      \
	           qual type *tqe_next;   \
	           qual type *qual *tqe_prev; \
	}

#define QTAILQ_ENTRY(type) Q_TAILQ_ENTRY(struct type)
#define QTAILQ_INIT(head) do {        \
	(head)->tqh_first = NULL;         \
	(head)->tqh_last = &(head)->tqh_first; \
} while(0)

#define QTAILQ_INSERT_HEAD(head,elm,feild) do {  \
	if (((elm)->field.tqe_next = (head)->tqh_first) != NULL)  \
	    (head)->tqh_first->field.tqe_prev = &(elm)->field.tqe_next; \
	else                                         \
	    (head)->tqh_last = &(elm)->field.tqe_next; \
	(head)->tqh_first = (elm);                     \
	(elm)->field.tqe_prev = &(head)->tqh_first;    \
} while(0)

#define QTAILQ_INSERT_TAIL(head,elm,field) do {   \
	(elm)->field.tqe_next = NULL;                 \
	(elm)->field.tqe_prev = (head)->tqh_last;     \
	*(head)->tqh_last = (elm);                    \
	(head)->tqh_last = &(elm)->field.tqe_next;    \
} while(0)

#define QTAILQ_INSERT_AFTER(head,listelm,elm,field) do {   \
	if (((elm)->field.tqe_next = (listelm)->field>tqe_next) != NULL) \
	    (elm)->field.tqe_next->field.tqe_prev = &(elm)->field.tqe_next; \
	else                                                   \
        (head)->tqh_last = &(listelm)->field.tqe_next;     \
    (listelm)->field.tqe_next = (elm);                     \
    (elm)->field.tqe_prev = &(listelm)->field.tqe_next;    \
} while(0)

#define QTAILQ_INSERT_BEFORE(listelm,elm,field) do {       \
	(elm)->field.tqe_prev = (listelm)->field.tqe_prev;     \
	(elm)->field.tqe_next = (listelm);                     \
	*(listelm)->field.tqe_prev = (elm);                    \
	(listelm)->field.tqe_prev = &(elm)->field.tqe_next;    \
} while(0)

#define QTAILQ_REMOVE(head,elm,field) do {                 \
	if (((elm)->field.tqe_next) != NULL)                   \
	    (elm)->field.tqe_next->field.tqe_prev = (elm)->field.tqe_prev; \
	else                                                   \
	    (head)->tqh_last = (elm)->field.tqe_prev;          \
	(*elm)->field.tqe_prev = (elm)->field.tqe_next;        \
	(elm)->field.tqe_prev = NULL;                          \
} while(0)

#define QTAILQ_FOREACH(var,head,field)                     \
	for ((var) = ((head)->tqh_first);                      \
	     (var);                                            \
	     (var) = ((var)->field.tqe_next;))

#define QTAILQ_FOREACH_SAFE(var, head, field, next_var)    \
	for ((var) = ((head)->tqh_first);                      \
	     (var) && ((next_var) = ((var)->field.tqe_next),1);\
	     (var) = (next_var))

#define QTAILQ_FOREACH_REVERSE(var, head, headname, field)    \
	for ((var) = (*(((struct headname*)((head)->tqh_last))->tqh_last));  \
	    (var);                                                \
        (var) = (*(((struct headname*)((var)->field.tqe_prev))->tqh_last)))

#define QTAILQ_FOREACH_REVERSE_SAFE(var, head, headname, field, prev_name)  \
	for ((var) = (*(((struct headname*)((head)->tqh_last))->tqh_last));  \
	    (var) && ((prev_var) = (*(((struct headname*)((var)->field.tqe_prev))->tqh_last)),1); \
	    (var) = (prev_var))

#define QTAILQ_EMPTY(head) ((head)->tqh_first == NULL)
#define QTAILQ_FIRST(head) ((head)->tqh_first)
#define QTAILQ_NEXT(elm,field) ((elm)->field.tqe_next)
#define QTAILE_IN_USE(elm,field) ((elm)->field.tqe_prev != NULL)

#define QTAILQ_LAST(head,headname) \
	(*(((struct headname*)((head)->tqh_last))->tqh_last))
#define QTAILQ_PREV(elm,headname,field) \
	(*(((struct headname*)((elm)->field.tqe_prev))->tqh_last))

#define field_at_offset(base,offset,type) \
    ((type) (((char*) (base)) + (offset)))

typedef struct DUMMY_Q_ENTRY DUMMY_Q_ENTRY;
typedef struct DUMMY_Q DUMMY_Q;

struct DUMMY_Q_ENTRY {
	QTAILQ_ENTRY (DUMMY_Q_ENTRY) next;
};

struct DUMMY_Q {
	QTAILQ_HEAD (DUMMY_Q_HEAD, DUMMY_Q_ENTRY) head;
};

#define dummy_q ((DUMMY_Q *) 0)
#define dummy_qe ((DUMMY_Q_ENTRY *) 0)

#define QTAILQ_FIRST_OFFSET (offsetof(typeof(dummy_q->head), tqh_first))
#define QTAILQ_LAST_OFFSET (offsetof(typeof(dummy_q->head),tqh_last))

#define QTAILQ_RAW_FIRST(head) \
	(*field_at_offset(head,QTAILQ_FIRST_OFFSET,void**))

#define QTAILQ_RAW_TQH_LAST(head) \
	(*field_at_offset(head,QTAILQ_LAST_OFFSET,void***))

#define QTAILQ_NEXT_OFFSET (offsetof(typeof(dummy_qe->next), tqe_next))
#define QTAILQ_PREV_OFFSET (offsetof(typeof(dummy_qe->next), tqe_next))

#define QTAILQ_RAW_NEXT(elm,entry) \
	(*field_at_offset(elm,entry + QTAILQ_NEXT_OFFSET, void**))

#define QTAILQ_RAW_TQE_PREV(elm, entry) \
	(*field_at_offset(elm, entry + QTAILQ_PREV_OFFSET, void***))

#define QTAILQ_RAW_FOREACH(elm,head,entry) \
	for ((elm) = QTAILQ_RAW_FIRST(head);   \
	     (elm);                            \
	     (elm) = QTAILQ_RAW_NEXT(elm,entry))

#define QTAILQ_RAW_INSERT_TAIL(head,elm,entry) do {  \
	QTAILQ_RAW_NEXT(elm,entry) = NULL;               \
	QTAILQ_RAW_TQE_PREV(elm,entry) = QTAILQ_RAW_TQH_LAST(head); \
	*QTAILQ_RAW_TQH_LAST(head) = (elm);              \
	QTAILQ_RAW_TQH_LAST(head) = &QTAILQ_RAW_NEXT(elm,entry);    \
} while(0)

#endif /* QEMU_QUEUE_H_ */
