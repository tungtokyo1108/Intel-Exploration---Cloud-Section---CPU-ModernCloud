/*
 * cpus.c
 *
 *  Created on: Sep 18, 2018
 *      Student (coder): Tung Dang
 */

#include "qemu/osdep.h"
#include "qemu/config-file.h"
#include "qom/cpu.h"
#include "monitor/monitor.h"
#include "qapi/error.h"
#include "qapi/qmp/qerror.h"
#include "qemu/error-report.h"
#include "sysemu/sysemu.h"
#include "sysemu/block-backend.h"
#include "exec/gdbstub.h"
#include "sysemu/dma.h"
#include "sysemu/hw_accel.h"
#include "sysemu/kvm.h"
#include "sysemu/hax.h"
#include "sysemu/hvf.h"
#include "sysemu/whpx.h"
#include "exec/exec-all.h"

#include "qemu/thread.h"
#include "sysemu/cpus.h"
#include "sysemu/qtest.h"
#include "qemu/main-loop.h"
#include "qemu/option.h"
#include "qemu/bitmap.h"
#include "qemu/seqlock.h"
#include "exec/tcg-wrapper.h"
#include "hw/nmi.h"
#include "sysemu/replay.h"
#include "hw/boards.h"

#ifdef CONFIG_LINUX

#include <sys/prctl.h>

#ifndef PR_MCE_KILL
#define PR_MCE_KILL 33
#endif

#ifndef PR_MCE_KILL_SET
#define PR_MCE_KILL_SET 1
#endif

#ifndef PR_MCE_KILL_EARLY
#define PR_MCE_KILL_EARLY 1
#endif

#endif

int64_t max_deplay;
int64_t max_advance;

static QEMUTimer *throttle_timer;
static unsigned int throttle_percentage;

#define CPU_THROTTLE_PCT_MIN 1
#define CPU_THROTTLE_PCT_MAX 99
#define CPU_THROTTLE_TIMESLICE_NS 1000000

bool cpu_is_stopped(CPUState *cpu) {
	return cpu->stopped || !runstate_is_running();
}

static bool cpu_thread_is_idle(CPUState *cpu) {
	if (cpu->stop || cpu->queued_work_first)
	{
		return false;
	}
	if (cpu_is_stopped(cpu))
	{
		return true;
	}
	if (!cpu->halted || cpu_has_work(cpu) || kvm_halt_in_kernel())
	{
		return false;
	}
	return true;
}

static bool all_cpu_threads_idle(void) {
	CPUState *cpu;
	CPU_FOREACH(cpu) {
		if (!cpu_thread_is_idle(cpu))
		{
			return false;
		}
	}
	return true;
}

static bool icount_sleep = true;
static int icount_time_shift;
#define MAX_ICOUNT_SHIFT 10

typedef struct TimersState {
	int64_t cpu_ticks_prev;
	int64_t cpu_ticks_offset;

	QemuSeqLock vm_clock_seqlock;
	int64_t cpu_clock_offset;
	int64_t cpu_ticks_enabled;
	int64_t dummy;

	int64_t qemu_icount_bias; // Compensate for varying guest execution speed
	int64_t qemu_icount;      // Only written by TCG thread
	int64_t vm_clock_warp_start;
	QEMUTimer *icount_rt_timer;
	QEMUTimer *icount_vm_timer;
	QEMUTimer *icount_warp_timer;

} TimersState;

static TimersState timers_state;
bool mttcg_enabled;
