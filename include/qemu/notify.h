/*
 * notify.h
 *
 *  Created on: Aug 17, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_NOTIFY_H_
#define QEMU_NOTIFY_H_

#include "qemu/queue.h"

typedef struct Notifier Notifier;

struct Notifier {
	void (*notify)(Notifier *notifier, void *data);
	QLIST_ENTRY(Notifier) node;
};

typedef struct NotifierList {
	QLIST_HEAD(, Notifier) notifier;
} NotifierList;

#define NOTIFIER_LIST_INTIALIZER(head) \
{QLIST_HEAD_INITIALIZER((head).notifiers)}

void notifier_list_init(NotifierList *list);
void notifier_list_add(NotifierList *list, Notifier *notifier);
void notifier_remove(Notifier *notifier);
void notifier_list_notify(NotifierList *list, void *data);

typedef struct NotifierWithReturn NotifierWithReturn;
struct NotifierWithReturn {
	int (*notify)(NotifierWithReturn *notifier, void *data);
	QLIST_ENTRY(NotifierWithReturn) node;
};

typedef struct NotifierWithReturnList {
	QLIST_HEAD(, NotifierWithReturn) notifier;
} NotifierWithReturnList;

#define NOTIFIER_WITH_RETURN_LIST_INITIALIZER(head) \
		{QLIST_HEAD_INITIALIZER((head).notifiers)}

void notifier_with_return_list_init(NotifierWithReturnList *list);
void notifier_with_return_list_add(NotifierWithReturnList *list, NotifierWithReturn *notifier);
void notifier_with_return_remove(NotifierWithReturn *notifier);
int notifier_with_return_list_notify(NotifierWithReturnList *list, void *data);

#endif /* QEMU_NOTIFY_H_ */
