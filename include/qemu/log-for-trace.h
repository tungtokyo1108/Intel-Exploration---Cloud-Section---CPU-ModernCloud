/*
 * log-for-trace.h
 *
 *  Created on: Sep 16, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_LOG_FOR_TRACE_H_
#define QEMU_LOG_FOR_TRACE_H_

extern int qemu_loglevel;
#define LOG_TRACE (1 << 15)

static inline bool qemu_loglevel_mask(int mask) {
	return (qemu_loglevel & mask) != 0;
}

int GCC_FMT_ATTR(1, 2) qemu_log(const char *fmt, ...);

#endif /* QEMU_LOG_FOR_TRACE_H_ */
