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
bool timerlistgroup_run_timers(QEMUTimerListGroup *tlg);

int64_t timerlistgroup_deadline_ns(QEMUTimerListGroup *tlg);
void timer_init_tl(QEMUTimer *ts, QEMUTimerList *timer_list, int scale, QEMUTimerCB *cb, void *opaque);
static inline void timer_init(QEMUTimer *ts, QEMUClockType type, int scale, QEMUTimerCB *cb, void *opaque) {
	timer_init_tl(ts,main_loop_tlg.tl[type],scale,cb,opaque);
}

static inline void timer_init_ns(QEMUTimer *ts, QEMUClockType type, QEMUTimerCB *cb, void *opaque) {
	timer_init(ts,type,SCALE_NS,cb,opaque);
}

static inline void timer_init_us(QEMUTimer *ts, QEMUClockType type, QEMUTimerCB *cb, void *opaque) {
	timer_init(ts,type,SCALE_US,cb,opaque);
}

static inline void timer_init_ms(QEMUTimer *ts, QEMUClockType type, QEMUTimerCB *cb, void *opaque) {
	timer_init(ts,type,SCALE_MS,cb,opaque);
}

static inline QEMUTimer *timer_new_tl(QEMUTimerList *timer_list, int scale, QEMUTimerCB *cb, void *opaque){
	QEMUTimer *ts = g_malloc0(sizeof(QEMUTimer));
	timer_init_tl(ts, timer_list, scale, cb, opaque);
	return ts;
}

static inline QEMUTimer *timer_new(QEMUClockType type, int scale, QEMUTimerCB *cb, void *opaque) {
	return timer_new_tl(main_loop_tlg.tl[type],scale,cb,opaque);
}

static inline QEMUTimer *timer_new_ns(QEMUClockType type, QEMUTimerCB *cb, void *opaque) {
	return timer_new(type,SCALE_NS,cb,opaque);
}

static inline QEMUTimer *timer_new_us(QEMUClockType type, QEMUTimerCB *cb,
                                      void *opaque)
{
    return timer_new(type, SCALE_US, cb, opaque);
}

static inline QEMUTimer *timer_new_ms(QEMUClockType type, QEMUTimerCB *cb,
                                      void *opaque)
{
    return timer_new(type, SCALE_MS, cb, opaque);
}

void timer_deinit(QEMUTimer *ts);
static inline void timer_free(QEMUTimer *ts) {
	g_free(ts);
}

void timer_del(QEMUTimer *ts);
void timer_mod_ns(QEMUTimer *ts, int64_t expire_time);
void timer_mod_anticipate_ns(QEMUTimer *ts, int64_t expire_time);
void timer_mod(QEMUTimer *ts, int64_t expire_timer);
void timer_mod_anticipate(QEMUTimer *ts, int64_t expire_time);
bool timer_pending(QEMUTimer *ts);
uint64_t timer_expire_time_ns(QEMUTimer *ts);
void timer_get(QEMUFile *f, QEMUTimer *ts);
void timer_put(QEMUFile *f, QEMUTimer *ts);
int qemu_timeout_ns_to_ms(int64_t ns);
int qemu_poll_ns(GPollFD *fds, guint nfds, int64_t timeout);
static inline int64_t qemu_soonest_timeout(int64_t timeout1, int64_t timeout2) {
	return ((uint64_t) timeout1 < (uint64_t) timeout2) ? timeout1 : timeout2;
}

void init_clocks(QEMUTimerListNotifyCB *notify_cb);
int64_t cpu_get_ticks(void);
void cpu_enable_ticks(void);
void cpu_disable_ticks(void);
static inline int64_t get_max_clock_jump(void) {
	return 60 * NANOSECONDS_PER_SECOND;
}

static inline int64_t get_clock_realtime(void) {
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return tv.tv_sec * 1000000000LL + (tv.tv_usec * 1000);
}

extern int use_rt_clock;
static inline int64_t get_clock(void)
{
#ifdef CLOCK_MONOTONIC
    if (use_rt_clock) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return ts.tv_sec * 1000000000LL + ts.tv_nsec;
    } else
#endif
    {
        return get_clock_realtime();
    }
}

int64_t cpu_get_icount_raw(void);
int64_t cpu_get_icount(void);
int64_t cpu_get_clock(void);
int64_t cpu_icount_to_ns(int64_t icount);
void cpu_update_icount(CPUState *cpu);
#if defined(__i386__)

static inline int64_t cpu_get_host_ticks(void)
{
    int64_t val;
    asm volatile ("rdtsc" : "=A" (val));
    return val;
}

#elif defined(__x86_64__)

static inline int64_t cpu_get_host_ticks(void)
{
    uint32_t low,high;
    int64_t val;
    asm volatile("rdtsc" : "=a" (low), "=d" (high));
    val = high;
    val <<= 32;
    val |= low;
    return val;
}
#else
static inline int64_t cpu_get_host_ticks(void)
{
    return get_clock();
}
#endif

#ifdef CONFIG_PROFILER
static inline int64_t profile_getclock(void)
{
    return get_clock();
}

extern int64_t tcg_time;
extern int64_t dev_time;
#endif

#endif /* QEMU_TIMER_H_ */
