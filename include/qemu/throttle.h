/*
 * throttle.h
 *
 *  Created on: Aug 28, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_THROTTLE_H_
#define QEMU_THROTTLE_H_

#include "qemu-common.h"
#include "qemu/timer.h"
#include "qemu/osdep.h"

#define THROTTLE_VALUE_MAX 1000000000000000LL

typedef enum {
	    THROTTLE_BPS_TOTAL,
	    THROTTLE_BPS_READ,
	    THROTTLE_BPS_WRITE,
	    THROTTLE_OPS_TOTAL,
	    THROTTLE_OPS_READ,
	    THROTTLE_OPS_WRITE,
 	    BUCKETS_COUNT,
} BucketType;

typedef struct LeakyBucket {
	uint64_t avg; // average goal in units per second
	uint64_t max; // leaky bucket max burst in units
	double level; // bucket level in units
	double burst_level; // bucket level in uints
	uint64_t burst_length; // max length of the burst perid in seconds
} LeakyBucket;

typedef struct ThrottleConfig {
	LeakyBucket buckets[BUCKETS_COUNT]; // leaky buckets
	uint64_t op_size; // size of an operation in bytes
} ThrottleConfig;

typedef struct ThrottleState {
	ThrottleConfig cfg;
	int64_t previous_leak; // timestamp of the last leak done
} ThrottleState;

typedef struct ThrottleTimers {
	QEMUTimer *timers[2];
	QEMUClockType clock_type;
	QEMUTimerCB *read_timer_cb;
	QEMUTimerCB *write_timer_cb;
	void *timer_opaque;
} ThrottleTimers;

void throttle_leak_bucket(LeakyBucket *bkt, int64_t delta);

int64_t throttle_compute_wait(LeakyBucket *bkt);

void throttle_init(ThrottleState *ts);

void throttle_timers_init(ThrottleTimers *tt,
                          AioContext *aio_context,
                          QEMUClockType clock_type,
                          QEMUTimerCB *read_timer_cb,
                          QEMUTimerCB *write_timer_cb,
                          void *timer_opaque);

void throttle_timers_destroy(ThrottleTimers *tt);

void throttle_timers_detach_aio_context(ThrottleTimers *tt);

void throttle_timers_attach_aio_context(ThrottleTimers *tt,
                                        AioContext *new_context);

bool throttle_timers_are_initialized(ThrottleTimers *tt);

bool throttle_enabled(ThrottleConfig *cfg);

bool throttle_is_valid(ThrottleConfig *cfg, Error **errp);

void throttle_config(ThrottleState *ts,
                     QEMUClockType clock_type,
                     ThrottleConfig *cfg);

void throttle_get_config(ThrottleState *ts, ThrottleConfig *cfg);

void throttle_config_init(ThrottleConfig *cfg);

bool throttle_schedule_timer(ThrottleState *ts,
                             ThrottleTimers *tt,
                             bool is_write);

void throttle_account(ThrottleState *ts, bool is_write, uint64_t size);
void throttle_limits_to_config(ThrottleLimits *arg, ThrottleConfig *cfg,
                               Error **errp);
void throttle_config_to_limits(ThrottleConfig *cfg, ThrottleLimits *var);

#endif /* QEMU_THROTTLE_H_ */
