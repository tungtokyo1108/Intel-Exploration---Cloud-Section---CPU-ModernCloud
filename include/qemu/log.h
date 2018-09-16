/*
 * log.h
 *
 *  Created on: Sep 16, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_LOG_H_
#define QEMU_LOG_H_

#include "qemu/log-for-trace.h"
#include "qemu/osdep.h"

extern FILE *qemu_logfile;
static inline bool qemu_log_enabled(void) {
	return qemu_logfile != NULL;
}

static inline bool qemu_log_separate(void) {
	return qemu_logfile != NULL && qemu_logfile != stderr;
}

#define CPU_LOG_TB_OUT_ASM (1 << 0)
#define CPU_LOG_TB_IN_ASM  (1 << 1)
#define CPU_LOG_TB_OP      (1 << 2)
#define CPU_LOG_TB_OP_OPT  (1 << 3)
#define CPU_LOG_INT        (1 << 4)
#define CPU_LOG_EXEC       (1 << 5)
#define CPU_LOG_PCALL      (1 << 6)
#define CPU_LOG_TB_CPU     (1 << 8)
#define CPU_LOG_RESET      (1 << 9)
#define LOG_UNIMP          (1 << 10)
#define LOG_GUEST_ERROR    (1 << 11)
#define CPU_LOG_MMU        (1 << 12)
#define CPU_LOG_TB_NOCHAIN (1 << 13)
#define CPU_LOG_PAGE       (1 << 14)
#define CPU_LOG_TB_OP_IND  (1 << 16)
#define CPU_LOG_TB_FPU (1 << 17)

// Lock output for a series of related logs

static inline void qemu_log_lock(void) {
	qemu_flockfile(qemu_logfile);
}

static inline void qemu_log_unlock(void)
{
    qemu_funlockfile(qemu_logfile);
}

static inline void GCC_FMT_ATTR(1,0)
qemu_log_vprintf(const char *fmt, va_list va) {
	if (qemu_logfile) {
		vfprintf(qemu_logfile,fmt,va);
	}
}

/**
 * Log only if a bit is set on the current loglevel mask
 */

#define qemu_log_mask(MASK,FMT,...) \
	do {                            \
		if (unlikely(qemu_loglevel_mask(MASK))) { \
			qemu_log(FMT, ## __VA_ARGS__);        \
		}\
	} while (0)

/**
 * Log only if a bit is set on the current loglevel mask
 * and we are in the address range we care about
 */

#define qemu_log_mask_and_addr(MASK, ADDR, FMT, ...) \
	do {                                             \
		if (unlikely(qemu_loglevel_mask(MASK)) &&    \
				     qemu_log_in_addr_range(ADDR)) { \
			qemu_log(FMT, ## __VA_ARGS__);           \
		}\
	}

typedef struct QEMULogItem {
	int mask;
	const char *name;
	const char *help;
} QEMULogItem;

extern const QEMULogItem qemu_log_items[];

void qemu_set_log(int log_flags);
void qemu_log_needs_buffers(void);
void qemu_set_log_filename(const char *filename, Error **errp);
void qemu_set_dfilter_ranges(const char *ranges, Error **errp);
bool qemu_log_in_addr_range(uint64_t addr);
int qemu_str_to_log_mask(const char *str);
void qemu_print_log_usage(FILE *f);
void qemu_log_flash(void);
void qemu_log_close(void);

#endif /* QEMU_LOG_H_ */
