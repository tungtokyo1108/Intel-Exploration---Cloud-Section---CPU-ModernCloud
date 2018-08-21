/*
 * timed-average.h
 *
 *  Created on: Aug 21, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_TIMED_AVERAGE_H_
#define QEMU_TIMED_AVERAGE_H_

#include "qemu/timer.h"
#include <stdint.h>

typedef struct TimedAverageWindow TimedAverageWindow;
typedef struct TimedAverage TimedAverage;

struct TimedAverageWindow {
	uint64_t min;
	uint64_t max;
	uint64_t sum;
	uint64_t count;
	int64_t expiration;
};

struct TimedAverage {
	uint64_t period;
	TimedAverageWindow windows[2];
	unsigned current;
	QEMUClockType clock_type;
};

void timed_average_init(TimedAverage *ta, QEMUClockType clock_type, uint64_t period);
void timed_average_account(TimedAverage *ta, uint64_t value);
uint64_t timed_average_min(TimedAverage *ta);
uint64_t timed_average_avg(TimedAverage *ta);
uint64_t timed_average_max(TimedAverage *ta);
uint64_t timed_average_sum(TimedAverage *ta, uint64_t *elapsed);

#endif /* QEMU_TIMED_AVERAGE_H_ */
