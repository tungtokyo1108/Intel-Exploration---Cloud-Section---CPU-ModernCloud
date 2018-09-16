/*
 * cpus.h
 *
 *  Created on: Sep 16, 2018
 *      Student (coder): Tung Dang
 */

#ifndef SYSEMU_CPUS_H_
#define SYSEMU_CPUS_H_

#include "qemu/timer.h"

bool qemu_in_vcpu_thread(void);
void qemu_init_cpu_loop(void);
void resume_all_vcpus(void);
void pause_all_vcpus(void);
void cpu_stop_current(void);
void cpu_ticks_init(void);

void configure_icount(QemuOpts *opts, Error **errp);
extern int use_icount;
extern int icount_align_option;

extern int64_t max_delay;
extern int64_t max_advance;
void dump_drift_info(FILE *f, fprintf_function cpu_fprintf);
void qemu_cpu_kick_self(void);
void qemu_timer_notify_cb(void *opaque, QEMUClockType type);

void cpu_synchronize_all_states(void);
void cpu_synchronize_all_post_reset(void);
void cpu_synchronize_all_post_init(void);
void cpu_synchronize_all_pre_loadvm(void);

void qtest_clock_warp(int64_t dest);

#ifndef CONFIG_USER_ONLY
extern int smp_cores;
extern int smp_threads;
#endif

void list_cpus(FILE *f, fprintf_function cpu_fprintf, const char *optarg);
void qemu_tcg_configure(QemuOpts *opts, Error **errp);

#endif /* SYSEMU_CPUS_H_ */
