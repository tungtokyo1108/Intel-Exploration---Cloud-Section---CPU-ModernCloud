/*
 * event-notifier.h
 *
 *  Created on: Aug 26, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_EVENT_NOTIFIER_H_
#define QEMU_EVENT_NOTIFIER_H_

#include "qemu-common.h"

struct EventNotifier {
	int rfd;
	int wfd;
};

typedef void EventNotifierHandler(EventNotifier *);
int event_notifier_init(EventNotifier *, int active);
void event_notifier_cleanup(EventNotifier *);
int event_notifier_set(EventNotifier *);
int event_notifier_test_and_clear(EventNotifier *);
int event_notifier_get_fd(const EventNotifier *);

#endif /* QEMU_EVENT_NOTIFIER_H_ */
