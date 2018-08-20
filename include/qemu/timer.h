/*
 * timer.h
 *
 *  Created on: Aug 20, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_TIMER_H_
#define QEMU_TIMER_H_

#include "qemu-common.h"
#include "qemu/notify.h"
#include "qemu/host-utils.h"

#define NANOSECONDS_PER_SECOND 1000000000LL

#define SCALE_MS 1000000
#define SCALE_US 1000
#define SCALE_NS 1

typedef enum {
	QEMU_CLOCK_REALTIME = 0,
    QEMU_CLOCK_VIRTUAL = 1,
	QEMU_CLOCK_HOST = 2,
    QEMU_CLOCK_VIRTUAL_RT = 3,
	QEMU_CLOCK_MAX
} QEMUClockType;

typedef struct QEMUTimerList QEMUTimerList;
struct QEMUTimerListGroup {
	QEMUTimerList *tl[QEMU_CLOCK_MAX];
};

typedef void QEMUTimerCB(void *opaque);
typedef void QEMUTimerListNotifyCB(void *opaque, QEMUClockType *type);

struct QEMUTimer {
	int64_t expire_time;
	QEMUTimerList *timer_list;
	QEMUTimerCB *cb;
	void *opaque;
	QEMUTimer *next;
	int scale;
};

extern QEMUTimerListGroup main_loop_tlg;

int64_t qemu_clock_get_ns(QEMUClockType type);
static inline int64_t qemu_clock_get_ms(QEMUClockType type) {
	return qemu_clock_get_ns(type) / SCALE_MS;
}

static inline int64_t qemu_clock_get_us(QEMUClockType type) {
	return qemu_clock_get_ms(type) / SCALE_US;
}

bool qemu_clock_expired(QEMUClockType type);
bool qemu_clock_use_for_deadline(QEMUClockType type);
int64_t qemu_clock_deadline_ns_all(QEMUClockType type);
void qemu_clock_notify(QEMUClockType type);
void qemu_clock_enable(QEMUClockType type, bool enabled);
void qemu_start_warp_timer(void);
void qemu_clock_register_reset_notifier(QEMUClockType type, Notifier *notifier);
bool qemu_clock_run_timers(QEMUClockType type);
bool qemu_clock_run_all_timers(void);

QEMUTimerList *timerlist_new(QEMUClockType type, QEMUTimerListNotifyCB *cb, void *opaque);
void timerlist_free(QEMUTimerList *timer_list);
bool timerlist_has_timers(QEMUTimerList *timer_list);
bool timerlist_expired(QEMUTimerList *timer_list);
int64_t timerlist_deadline_ns(QEMUTimerList *timer_list);
bool timerlist_run_timers(QEMUTimerList *timer_list);
void timerlist_notify(QEMUTimerList *timer_list);
void timerlistgroup_init(QEMUTimerListGroup *tlg, QEMUTimerListNotifyCB *cb, void *opaque);


#endif /* QEMU_TIMER_H_ */






















































