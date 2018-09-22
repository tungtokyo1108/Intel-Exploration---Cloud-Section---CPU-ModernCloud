/*
 * cpus.c
 *
 *  Created on: Sep 18, 2018
 *      Student (MIG Virtual Coder): Tung Dang
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

/**
 * Each guest (target) has been updated to support:
 * - atomic instructions
 * - memory ordering primitives (barriers)
 *
 * There are two limitations when a guest architecture has been converted to the new primitives
 * - The guest can not be oversized
 * - The host must have a stronger memory order than the guest
 */

static bool check_tcg_memory_orders_compatible(void) {
#if defined(TCG_GUEST_DEFAULT_MO) && defined(TCG_TARGET_DEFAULT_MO)
	return (TCG_GUEST_DEFAULT_MO & ~TCG_TARGET_DEFAULT_MO) == 0;
#else
	return false;
#endif
}

static bool default_mttcg_enabled(void) {
	if (use_icount || TCG_OVERSIZED_GUEST) {
		return false;
	}
	else
	{
#ifdef TARGET_SUPPORTS_MTTG
		return check_tcg_memory_orders_compatible();
#else
		return false;
#endif
	}
}

void qemu_tcg_configure(QemuOpts *opts, Error **errp) {
	const char *t = qemu_opt_get(opts, "thread");
	if (t) {
		if (strcmp(t, "multi") == 0)
		{
			if (TCG_OVERSIZED_GUEST)
			{
				error_setg(errp, "No MTTG when guest word size > hosts");
			}
			else if (use_icount)
			{
				error_setg(errp, "No MTTG when icount is enabled");
			}
			else
			{
#ifndef TARGET_SUPPORTS_MTTCG
				error_report("Guest not yet converted to MTTG and may get unexpected results");
#endif
				if (!check_tcg_memory_orders_compatible()) {
					error_report("Guest expects a stronger memory ordering than the host provides");
					error_printf("This may cause strange/hard to debug errors");
				}
				mttcg_enabled = true;
			}
		}
		else if (strcmp(t, "single") == 0)
		{
			mttcg_enabled = false;
		}
		else
		{
			error_setg(errp, "Invalid 'thread' setting %s", t);
		}
	}
	else
	{
		mttcg_enabled = default_mttcg_enabled();
	}
}

// The current number of executed instructions

static int64_t cpu_get_icount_executed(CPUState *cpu) {
	return cpu->icount_budget - (cpu->icount_decr.u16.low + cpu->icount_extra);
}

// Update the global shared timer_state.qemu_icount to take into account executed instructions

void cpu_update_icount(CPUState *cpu) {
	int64_t executed = cpu_get_icount_executed(cpu);
	cpu->icount_budget -= executed;

#ifdef CONFIG_ATOMIC64
	atomic_set__nocheck(&timers_state.qemu_icount,
			atomic_read__nocheck(&timers_state.qemu_icount) + executed);
#else
	timers_state.qemu_icount += executed;
#endif
}

int64_t cpu_get_icount_raw(void) {
	CPUState *cpu = current_cpu;
	if (cpu && cpu->running)
	{
		if (!cpu->can_do_io)
		{
			error_report("Bad icount read");
			exit(1);
		}
		cpu_update_icount(cpu);
	}
#ifdef CONFIG_ATOMIC64
	return atomic_read__nocheck(&timers_state.qemu_icount);
#else
	return timers_state.qemu_icount;
#endif
}

// Return the virtual CPU time, based on the instruction counter

static int64_t cpu_get_icount_locked(void) {
	int64_t icount = cpu_get_icount_raw();
	return timers_state.qemu_icount_bias + cpu_icount_to_ns(icount);
}

int64_t cpu_get_icount(void) {
	int64_t icount;
	unsigned start;
	do {
		start = seqlock_read_begin(&timers_state.vm_clock_seqlock);
		icount = cpu_get_icount_locked();
	} while (seqlock_read_retry(&timers_state.vm_clock_seqlock,start));
	return icount;
}

int64_t cpu_icount_to_ns(int64_t icount) {
	return icount << icount_time_shift;
}

/**
 * return the time elapsed in VM between vm_start and vm_stop.
 * cpu_get_ticks() uses units of the host CPU cycle counter
 */

int64_t cpu_get_ticks(void) {
	int64_t ticks;
	if (use_icount) {
		return cpu_get_icount();
	}

	ticks = timers_state.cpu_ticks_offset;
	if (timers_state.cpu_ticks_enabled) {
		ticks += cpu_get_host_ticks();
	}

	if (timers_state.cpu_ticks_prev > ticks)
	{
		timers_state.cpu_ticks_offset += timers_state.cpu_ticks_prev - ticks;
		ticks = timers_state.cpu_ticks_prev;
	}

	timers_state.cpu_ticks_prev = ticks;
	return ticks;
}

static int64_t cpu_get_clock_locked(void) {
	int64_t time;
	time = timers_state.cpu_clock_offset;
	if (timers_state.cpu_ticks_enabled)
	{
		time += get_clock();
	}
	return time;
}

/*
 * Return the monotonic time elapsed in VM
 * the time between vm_start and vm_stop
 */

int64_t cpu_get_clock(void) {
	int64_t ti;
	unsigned start;
	do {
		start = seqlock_read_begin(&timers_state.vm_clock_seqlock);
		ti = cpu_get_clock_locked();
	} while (seqlock_read_retry(&timers_state.vm_clock_seqlock, start));
	return ti;
}

// Caller must hold BQL which serves as mutex for vm_clock_seqlock

void cpu_enable_ticks(void) {
	seqlock_write_begin(&timers_state.vm_clock_seqlock);
	if (!timers_state.cpu_ticks_enabled)
	{
		timers_state.cpu_clock_offset -= cpu_get_host_ticks();
		timers_state.cpu_clock_offset -= get_clock();
		timers_state.cpu_ticks_enabled = 1;
	}
	seqlock_write_end(&timers_state.vm_clock_seqlock);
}

// The clock is stopped.

void cpu_disable_ticks(void) {
	seqlock_write_begin(&timers_state.vm_clock_seqlock);
	if (timers_state.cpu_ticks_enabled)
	{
		timers_state.cpu_clock_offset += cpu_get_host_ticks();
		timers_state.cpu_clock_offset = cpu_get_clock_locked();
		timers_state.cpu_ticks_enabled = 0;
	}
	seqlock_write_end(&timers_state.vm_clock_seqlock);
}

/*
 * Correlation between real and virtual time is always going to be
 * fairly approximate.
 * When the guest us idle real and virtual time will be aligned in
 * the IO wait loop
 */

#define ICOUNT_WOBBLE (NANOSECONDS_PER_SECOND / 10)

static void icount_adjust(void) {
	int64_t cur_time;
	int64_t cur_icount;
	int64_t delta;

	static int64_t last_delta;
	if (!runstate_is_running())
	{
		return;
	}

	seqlock_write_begin(&timers_state.vm_clock_seqlock);
	cur_time = cpu_get_clock_locked();
	cur_icount = cpu_get_icount_locked();
	delta = cur_icount - cur_time;
	if (delta > 0 && last_delta + ICOUNT_WOBBLE < delta * 2
			      && icount_time_shift > 0)
	{
		icount_time_shift--;
	}
	if (delta < 0 && last_delta - ICOUNT_WOBBLE > delta * 2
			      && icount_time_shift < MAX_ICOUNT_SHIFT)
	{
		icount_time_shift++;
	}
	last_delta = delta;
	timers_state.qemu_icount_bias = cur_icount
			- (timers_state.qemu_icount << icount_time_shift);
	seqlock_write_end(&timers_state.vm_clock_seqlock);
}

static void icount_adjust_rt(void *opaque) {
	timer_mod(timers_state.icount_rt_timer,
			  qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL_RT) + 1000);
	icount_adjust();
}

static void icount_adjust_vm(void *opaque) {
	timer_mod(timers_state.icount_vm_timer,
			  qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) +
			  NANOSECONDS_PER_SECOND / 10);
	icount_adjust();
}

static int64_t qemu_icount_round(int64_t count) {
	return (count + (1 << icount_time_shift) - 1) >> icount_time_shift;
}

static void icount_wrap_rt(void) {
	unsigned seq;
	int64_t warp_start;
	do {
		seq = seqlock_read_begin(&timers_state.vm_clock_seqlock);
		warp_start = timers_state.vm_clock_warp_start;
	} while (seqlock_read_entry(&timers_state.vm_clock_seqlock,seq));

	if (warp_start == -1) {
		return;
	}

	seqlock_write_begin(&timers_state.vm_clock_seqlock);
	if (runstate_is_runnig())
	{
		int64_t clock = REPLAY_CLOCK(REPLAY_CLOCK_VIRTUAL_RT,cpu_get_clock_locked());
		int64_t warp_delta;
		warp_delta = clock - timers_state.vm_clock_warp_start;
		if (use_icount == 2)
		{
			int64_t cur_icount = cpu_get_icount_locked();
			int64_t delta = clock - cur_icount;
			warp_delta = MIN(warp_delta, delta);
		}
		timers_state.qemu_icount_bias += warp_delta;
	}
	timers_state.vm_clock_warp_start = -1;
	seqlock_write_end(&timers_state.vm_clock_seqlock);

	if (qemu_clock_expired(QEMU_CLOCK_VIRTUAL))
	{
		qemu_clock_notify(QEMU_CLOCK_VIRTUAL);
	}
}

static void icount_timer_cb(void *opaque) {
	icount_warp_rt();
}

void qtest_clock_warp(int64_t dest) {
	int64_t clock = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
	AioContext *aio_context;
	assert(qtest_anabled());
	aio_context = qemu_get_aio_context();
	while (clock < dest)
	{
		int64_t deadline = qemu_clock_deadline_ns_all(QEMU_CLOCK_VIRTUAL);
		int64_t warp = qemu_soonest_timeout(dest - clock, deadline);

		seqlock_write_begin(&timers_state.vm_clock_seqlock);
		timers_state.qemu_icount_bias += warp;
		seqlock_write_end(&timers_state.vm_clock_seqlock);

		qemu_clock_run_times(QEMU_CLOCK_VIRTUAL);
		timerlist_run_timers(aio_context->tlg.tl[QEMU_CLOCK_VIRTUAL]);
		clock = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
	}
	qemu_clock_notify(QEMU_CLOCK_VIRTUAL);
}

void qemu_start_warp_timer(void) {
	int64_t clock;
	int64_t deadline;

	if (!use_icount) {
		return;
	}

	if (!runstate_is_running()) {
		return;
	}

	// warp clock deterministically in record/replay mode
	if (!replay_checkpoint(CHECKPOINT_CLOCK_WARP_START)) {
		return;
	}

	if (!all_cpu_threads_idle()) {
		return;
	}

	if (qtest_enabled()) {
		return;
	}

	// Want to use the earliest deadline from ALL vm_clocks
	clock = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL_RT);
	deadline = qemu_clock_deadline_ns_all(QEMU_CLOCK_VIRTUAL);
	if (deadline < 0)
	{
		static bool notified;
		if (!icount_sleep && !notified)
		{
			warn_report("icount sleep disabled and no active timers");
			notified = true;
		}
		return;
	}

	if (deadline > 0)
	{
		/*
		 * Ensure QEMU_CLOCK_VIRTUAL proceeds even when the virtual CPU goes to sleep
		 */
		if (!icount_sleep)
		{
			/*
			 * Never let VCPU sleep in no sleep icount mode.
			 * If there is a pending QEMU_CLOCK_VIRTUAL timer we just advance
			 * to the next QEMU_CLOCK_VIRTUAL event and notify it.
			 */
			seqlock_write_begin(&timers_state.vm_clock_seqlock);
			timers_state.qemu_icount_bias += deadline;
			seqlock_write_end(&timers_state.vm_clock_seqlock);
			qemu_clock_notify(QEMU_CLOCK_VIRTUAL);
		}
		else
		{
			/*
			 * Do stop VCPU and only advance QEMU_CLOCK_VIRTUAL after some
			 * "real" time, has passed.
			 * This avoids that the warps are visible externally
			 * For example, not be sending network packets continuously
			 * instead of every 100ms
			 */

			seqlock_write_begin(&timers_state.vm_clock_seqlock);
		    if (timers_state.vm_clock_warp_start == -1
		        || timers_state.vm_clock_warp_start > clock)
		    {
		    	timers_state.vm_clock_warp_start = clock;
		    }
		    seqlock_write_end(&timers_state.vm_clock_seqlock);
		    timer_mod_anticipate(timers_state.icount_warp_timer, clock + deadline);
		}
	}
	else if (deadline == 0)
	{
		qemu_clock_notify(QEMU_CLOCK_VIRTUAL);
	}
}

static void qemu_account_warp_timer(void) {
	if (!use_icount || !icount_sleep) {
		return;
	}

	if (!runstate_is_running()) {
		return;
	}

	// warp clock deterministically in record/replay mode
    if (!replay_checkpoint(CHECKPOINT_CLOCK_WARP_START)) {
		return;
	}

    timer_del(timers_state.icount_warp_timer);
    icount_warp_rt();
}

static bool icount_state_needed(void *opaque) {
	return use_icount;
}

static bool warp_timer_state_needed(void *opaque) {
	TimersState *s = opaque;
	return s->icount_warp_timer != NULL;
}

static bool adjust_timers_state_needed(void *opaque) {
	TimersState *s = opaque;
	return s->icount_rt_timer != NULL;
}

static void cpu_throttle_thread(CPUState *cpu, run_on_cpu_data opaque) {
	double pct;
	double throttle_ratio;
	long sleeptime_ns;

	if (!cpu_throttle_get_percentage()) {
		return;
	}

	pct = (double)cpu_throttle_get_percentage()/100;
	throttle_ratio = pct/(1-pct);
	sleeptime_ns = (long)(throttle_ratio * CPU_THROTTLE_TIMESLICE_NS);

	qemu_mutex_unlock_iothread();
	g_usleep(sleeptime_ns/1000);
	qemu_mutex_lock_iothread();
	atomic_set(&cpu->throttle_thread_sheduled, 0);
}

static void cpu_throttle_timer_tick(void *opaque) {
	CPUState *cpu;
	double pct;

	// Stop timer if needed
	if (!cpu_throttle_get_percentage()) {
		return;
	}

	CPU_FOREACH(cpu) {
		if (!atomic_xchg(&cpu->throttle_thread_scheduled,1)) {
			async_run_on_cpu(cpu, cpu_throttle_thread, RUN_ON_CPU_NULL);
		}
	}

	pct = (double)cpu_throttle_get_percentage()/100;
	timer_mod(throttle_timer, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL_RT) +
			  CPU_THROTTLE_TIMESLICE_NS / (1-pct));
}

void cpu_throttle_set(int new_throttle_pct) {

	// Ensure throttle percentage is within valid range
	new_throttle_pct = MIN(new_throttle_pct,CPU_THROTTLE_PCT_MAX);
	new_throttle_pct = MAX(new_throttle_pct,CPU_THROTTLE_PCT_MIN);

	atomic_set(&throttle_percentage,new_throttle_pct);
	timer_mod(throttle_timer, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL_RT) +
			  CPU_THROTTLE_TIMESLICE_NS);
}

void cpu_throttle_stop(void) {
	atomic_set(&throttle_percentage,0);
}

bool cpu_throttle_active(void) {
	return (cpu_throttle_get_percentage() != 0);
}

int cpu_throttle_get_percentage(void) {
	return atomic_read(&throttle_percentage);
}

void cpu_ticks_init(void) {
	seqlock_init(&timers_state.vm_clock_seqlock);
	vmstate_register(NULL, 0, &vmstate_timers, &timers_state);
	throttle_timer = timer_new_ns(QEMU_CLOCK_VIRTUAL_RT,
			                      cpu_throttle_timer_tick, NULL);
}

void configure_icount(QemuOpts *opts, Error **errp) {
	const char *option;
	char *rem_str = NULL;

	option = qemu_opt_get(opts, "shift");
	if (!option) {
		if (qemu_opt_get(opts, "align") != NULL)
		{
			error_setg(errp, "Please specify shift option when using align");
		}
		return;
	}

	icount_sleep = qemu_opt_get_bool(opts, "sleep", true);
	if (icount_sleep)
	{
		timers_state.icount_warp_timer = timer_new_ns(QEMU_CLOCK_VIRTUAL_RT,icount_timer_cb,NULL);
	}

	icount_align_option = qemu_opt_get_bool(opts,"align",false);

	if (icount_align_option && !icount_sleep) {
		error_setg(errp, "align=on and sleep=off are incompatible");
	}

	if (strcmp(option,"auto") != 0) {
		errno = 0;
		icount_time_shift = strol(option, &rem_str,0);
		if (errno != 0 || *rem_str != '\0' || !strlen(option)) {
			error_setg(errp, "icount: Invalid shift value");
		}
		use_icount = 1;
		return;
	} else if (icount_align_option) {
		error_setg(errp, "shift=auto and align=on are incompatible");
	} else if (!icount_sleep) {
		error_setg(errp, "shift=auto and sleep=off are incompatible");
	}

	use_icount = 2;
	icount_time_shift = 3;

	/*
	 * Have both real time and virtual time triggers for speed adjustment
	 * The real time trigger catches emulated time passing too slowly,
	 * The virtual time trigger catches emulated time passing too fast
	 */

	timers_state.vm_clock_warp_start = -1;
	timers_state.icount_rt_timer = timer_new_ms(QEMU_CLOCK_VIRTUAL_RT,
			                                    icount_adjust_rt, NULL);
	timer_mod(timers_state.icount_rt_timer,
			  qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL_RT) + 1000);
	timers_state.icount_vm_timer = timer_new_ns(QEMU_CLOCK_VIRTUAL,
			                                    icount_adjust_vm,NULL);
	timer_mod(timers_state.icount_vm_timer,
			  qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) +
			  NANOSECONDS_PER_SECOND / 10);
}


/**
 * The kick timer is responsible for moving single thread vCPU
 * emulation on to the next vCPU.
 * If more than on vCPU is running a timer event with force a cpu->exit
 * so the next vCPU can get scheduled
 */

static QEMUTimer *tcg_kick_vcpu_timer;
static CPUState *tcg_current_rr_cpu;

#define TCG_KICK_PERID (NANOSECONDS_PER_SECOND / 10)

static inline int64_t qemu_tcg_next_kick(void) {
	return qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + TCG_KICK_PERIOD;
}

static void qemu_cpu_kick_rr_cpu(void) {
	CPUState *cpu;
	do {
		cpu = atomic_mb_read(&tcg_current_rr_cpu);
		if (cpu) {
			cpu_exit(cpu);
		}
	} while (cpu != atomic_mb_read(&tcg_current_rr_cpu));
}

static void do_nothing(CPUState *cpu, run_on_cpu_data unused)
{
}

void qemu_timer_notify_cb(void *opaque, QEMUClockType type) {
	if (!use_icount || type != QEMU_CLOCK_VIRTUAL)
	{
		qemu_notify_event();
		return;
	}

	if (qemu_in_vcpu_thread())
	{
		/*
		 * A CPU is currently running
		 * -> kick it back out to the tcg_cpu_exec() loop
		 * -> so it will recalculate its icount deadline immediately
		 */
		qemu_cpu_kick(current_cpu);
	} else if (first_cpu) {
		/*
		 * qemu_cpu_kick is not enough to kick a halted CPU out of qemu_tcg_wait_io_event
		 * -> aync_run_on_cpu cause cpu_thread_is_idle to return false
		 */
		async_run_on_cpu(first_cpu, do_nothing, RUN_ON_CPU_NULL);
	}
}

static void kick_tcg_thread(void *opaque) {
	timer_mod(tcg_kick_vcpu_timer, qemu_tcg_next_kick());
	qemu_cpu_kick_rr_cpu();
}

static void start_tcg_kick_timer(void) {
	assert(!mttcg_enabled);
	if (!tcg_kick_vcpu_timer && CPU_NEXT(first_cpu))
	{
		tcg_kick_vcpu_timer = timer_new_ns(QEMU_CLOCK_VIRTUAL,kick_tcg_thread,NULL);
		timer_mod(tcg_kick_vcpu_timer, qemu_tcg_next_kick());
	}
}

static void stop_tcg_kick_timer(void) {
	assert(!mttcg_enabled);
	if (tcg_kick_vcpu_timer)
	{
		timer_del(tcg_kick_vcpu_timer);
		tcg_kick_vcpu_timer = NULL;
	}
}


////////////////////////////////////////////////////////////////////////////////////////

void hw_error(const char *fmt, ...) {
	va_list ap;
	CPUState *cpu;

	va_start(ap,fmt);
	fprintf(stderr, "qemu: hardware error: ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	CPU_FOREACH(cpu) {
		fprintf(stderr, "CPU #%d:\n", cpu->cpu_index);
		cpu_dump_state(cpu, stderr, fprintf, CPU_DUMP_FPU);
	}

	va_end(ap);
	abort();
}

void cpu_sychronize_all_states(void) {
	CPUState *cpu;
	CPU_FOREACH(cpu) {
		cpu_synchronize_state(cpu);
		if (hvf_enabled())
		{
			hvf_cpu_synchronize_state(cpu);
		}
	}
}

void cpu_synchronize_all_post_reset(void) {
	CPUState *cpu;
	CPU_FOREACH(cpu) {
		cpu_synchronize_post_reset(cpu);
		if (hvf_enabled()) {
			hvf_cpu_synchronize_post_reset(cpu);
		}
	}
}

void cpu_synchronize_all_post_init(void) {
	CPUState *cpu;
	CPU_FOREACH(cpu) {
	    cpu_synchronize_post_init(cpu);
		if (hvf_enabled()) {
			hvf_cpu_synchronize_post_init(cpu);
		}
	}
}

void cpu_synchronize_all_pre_loadvm(void) {
	CPUState *cpu;
	CPU_FOREACH(cpu) {
		cpu_synchronize_pre_loadvm(cpu);
	}
}

static int do_vm_stop(RunState state, bool send_stop) {
	int ret = 0;
	if (runstate_is_running())
	{
		cpu_disable_ticks();
		pause_all_vcpus();
		runstate_set(state);
		vm_state_notify(0, state);
		if (send_stop) {
			qapi_event_send_stop(&error_abort);
		}
	}

	bdrv_drain_all();
	replay_diable_events();
	ret = bdrv_flush_all();
	return ret;
}

// Special vm_stop() variant for terminating the process

int vm_shutdown(void) {
	return do_vm_stop(RUN_STATE_SHUTDOWN, false);
}

static bool cpu_can_run(CPUState *cpu) {
	if (cpu->stop) {
		return false;
	}

	if (cpu_is_stopped(cpu)) {
		return false;
	}

	return true;
}

static void cpu_handle_guest_debug(CPUState *cpu) {
	gdb_set_stop_cpu(cpu);
	qemu_system_debug_request();
	cpu->stopped = true;
}

#ifdef CONFIG_LINUX

static void sigbus_reraise(void) {
	sigset_t set;
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_handler = SIG_DFL;
	if (!sigaction(SIGBUS, &action, NULL)) {
		raise(SIGBUS);
		sigemptyset(&set);
		sigaddset(&set, SIGBUS);
		pthread_sigmask(SIG_UNBLOCK, &set, NULL);
	}
	perror("Failed to re-raise SIGBUS!\n");
	abort();
}

static void sigbus_handler(int n, siginfo_t *siginfo, void *ctx) {
	if (siginfo->si_code != BUS_MCEERR_AO && siginfo->si_code != BUS_MCEERR_AR) {
		sigbus_reraise();
	}

	if (current_cpu)
	{
		// Called asynchronously in VCPU thread
		if (kvm_on_sigbus_vcpus(current_cpu, siginfo->si_code, siginfo->si_addr))
		{
			sigbus_reraise();
		}
	}
	else
	{
		// Called synchronously via signaIfd in main thread
		if (kvm_on_sigbus(siginfo->si_code, siginfo->si_addr))
		{
			sigbus_reraise();
		}
	}
}

static void qemu_init_sigbus(void) {
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_flags = SA_SIGINFO;
	action.sa_sigaction = sigbus_handler;
	signation(SIGBUS, &action, NULL);
	prctl(PR_MCE_KILL, PR_MCE_KILL_SET, PR_MCE_KILL_EARLY, 0, 0);
}

#else
static void qemu_init_sigbus(void) {}
#endif

static QemuMutex qemu_global_mutex;
static QemuThread io_thread;

// CPU creation
static QemuMutex qemu_cpu_cond;
static QemuCond qemu_pause_cond;

void qemu_init_cpu_loop(void) {
	qemu_init_sigbus();
	qemu_cond_init(&qemu_cpu_cond);
	qemu_cond_init(&qemu_pause_cond);
	qemu_mutex_init(&qemu_global_mutex);
	qemu_thread_get_self(&io_thread);
}

void run_on_cpu(CPUState *cpu, run_on_cpu_func func, run_on_cpu_data data) {
	do_run_on_cpu(cpu, func, data, &qemu_global_mutex);
}

static void qemu_kvm_destroy_vcpu(CPUState *cpu) {
	if (kvm_destroy_vcpu(cpu) < 0)
	{
		error_report("kvm_destroy_vcpu failed");
		exit(EXIT_FAILURE);
	}
}

static void qemu_tcg_destroy_vcpu(CPUState *cpu) {}

static void qemu_cpu_stop(CPUState *cpu, bool exit) {
	g_assert(qemu_cpu_is_self(cpu));
	cpu->stop = false;
	cpu->stopped = true;
	if (exit)
	{
		cpu_exit(cpu);
	}
	qemu_cond_broadcast(&qemu_pause_cond);
}

static void qemu_wait_event_common(CPUState *cpu) {
	atomic_mb_set(&cpu->thread_kicked, false);
	if (cpu->stop)
	{
		qemu_cpu_stop(cpu,false);
	}
	process_queued_cpu_work(cpu);
}

static void qemu_tcg_rr_wait_io_event(CPUState *cpu) {
	while (all_cpu_threads_idle())
	{
		stop_tcg_kick_timer();
		qemu_cond_wait(cpu->halt_cond, &qemu_global_mutex);
	}
	start_tcg_kick_timer();
	qemu_wait_io_event_common(cpu);
}

static void qemu_wait_io_event(CPUState *cpu) {
	while (cpu_thread_is_idle(cpu))
	{
		qemu_cond_wait(cpu->halt_cond, &qemu_global_mutex);
	}
#ifdef _WIN32
	// Eat dummy APC queued by qemu_cpu_kick_thread
	if (!tcg_enabled())
	{
		SleepEx(0,true);
	}
#endif
	qemu_wait_io_event_common(cpu);

}

static void *qemu_kvm_cpu_thread_fn(void *arg) {
	CPUState *cpu = arg;
	int r;
	rcu_register_thread();
	qemu_mutex_lock_iothread();
	qemu_thread_get_self(cpu->thread);
	cpu->thread_id = qemu_get_thread_id();
	cpu->can_do_io = 1;
	current_cpu = cpu;

	r = kvm_init_vcpu(cpu);
	if (r < 0)
	{
		error_report("kvm_init_vcpu failed: %s", strerror(-r));
		exit(1);
	}

	kvm_init_cpu_signals(cpu);

	// signal CPU creation
	cpu->created = true;
	qemu_cond_signal(&qemu_cpu_cond);

	do {
		if (cpu_can_run(cpu))
		{
			r = kvm_cpu_exec(cpu);
			if (r == EXCP_DEBUG)
			{
				cpu_handle_guest_debug(cpu);
			}
		}
		qemu_wait_io_event(cpu);
	} while (!cpu->unplug || cpu_can_run(cpu));

	qemu_kvm_destroy_vcpu(cpu);
	cpu->created = false;
	qemu_cond_signal(&qemu_cpu_cond);
	qemu_mutex_unlock_iothread();
	rcu_unregister_thread();
	return NULL;
}

static void *qemu_dummy_cpu_thread_fn(void *arg) {
	CPUState *cpu = arg;
	sigset_t waitset;
	int r;

	rcu_register_thread();
	qemu_mutex_lock_iothread();
	qemu_thread_get_seft(cpu->thread);

	cpu->thread_id = qemu_get_thread_id();
	cpu->can_do_io = 1;
	current_cpu = cpu;

	simemptyset(&waitset);
	sigaddset(&waitset, SIG_IPI);

	cpu->created = true;
	qemu_cond_signal(&qemu_cpu_cond);

	do {
		qemu_mutex_unlock_iothread();
		do {
			int sig;
			r = sigwait(&waitset, &sig);
		} while (r == -1 && (errno == EAGAIN || errno == EINTR));
		if (r == -1) {
			perror("sigwait");
			exit(1);
		}
		qemu_mutex_lock_iothread();
		qemu_wait_io_event(cpu);
	} while (!cpu->unplug);

	rcu_unregister_thread();
	return NULL;
}

static int64_t tcg_get_icount_limit(void) {
	int64_t deadline;

	if (replay_mode != REPLAY_MODE_PLAY)
	{
		deadline = qemu_clock_deadline_ns_all(QEMU_CLOCK_VIRTUAL);
		if ((deadline < 0) || (deadline > INT32_MAX)) {
			deadline = INT32_MAX;
		}
		return qemu_icount_round(deadline);
	}
	else
	{
		return replay_get_instructions();
	}
}

static void handle_icount_deadline(void) {
	assert(qemu_in_vcpu_thread());
	if (use_icount) {
		int64_t deadline = qemu_clock_deadline_ns_all(QEMU_CLOCK_VIRTUAL);
		if (deadline == 0)
		{
			// Wake up other AioContext
			qemu_clock_notify(QEMU_CLOCK_VIRTUAL);
			qemu_clock_run_timers(QEMU_CLOCK_VIRTUAL);
		}
	}
}

static void prepare_icount_for_run(CPUState *cpu) {
	if (use_icount)
	{
		int insns_left;
		g_assert(cpu->icount_decr.u16.low == 0);
		g_assert(cpu->icount_extra == 0);

		cpu->icount_budget = tcg_get_icount_limit();
		insns_left = MIN(0Xffff, cpu->icount_budget);
		cpu->icount_decr.u16.low = insns_left;
		cpu->icount_extra = cpu->icount_budget - insns_left;

		replay_mutex_lock();
	}
}

static void process_icount_data(CPUState *cpu) {
	if (use_icount)
	{
		// Account for executed instructions
		cpu_update_icount(cpu);

		// Reset the counters
		cpu->icount_decr.u16.low = 0;
		cpu->icount_extra = 0;
		cpu->icount_budget = 0;

		replay_account_executed_instructions();
		replay_mutex_unlock();
	}
}

static int tcg_cpu_exec(CPUState *cpu) {
	int ret;
#ifdef CONFIG_PROFILER
	int64_t ti;
#endif

	assert(tcg_enabled());
#ifdef CONFIG_PROFILER
	ti = profile_getclock();
#endif

	cpu_exec_start(cpu);
	ret = cpu_exec(cpu);
	cpu_exec_end(cpu);

#ifdef CONFIG_PROFILER
	tcg_timer += profile_getclock() - ti;
#endif

	return ret;
}

/*
 * Destroy any remaining vCPUs which have been unplugged and have finished running
 */

static void deal_with_unplugged_cpus(void) {
	CPUState *cpu;
	CPU_FOREARCH(cpu)
	{
		if (cpu->unplug && !cpu_can_run(cpu))
		{
			qemu_tcg_destroy_vcpu(cpu);
			cpu->created = false;
			qemu_cond_signal(&qemu_cpu_cond);
			break;
		}
	}
}

/*
 * In the single-threaded case each vCPU is simulated in turn.
 * If there is more than a single vCPU, a simple timer will be created
 * to kick the vCPU and do not stuck in a tight loop in on vCPU
 */

static void *qemu_tcg_rr_cpu_thread_fn(void *arg) {
	CPUState *cpu = arg;

	assert(tcg_enabled);
	rcu_register_thread();
	tcg_register_thread();

	qemu_mutex_lock_iothread();
	qemu_thread_get_self(cpu->thread);

	cpu->thread_id = qemu_get_thread_id();
	cpu->created = true;
	cpu->can_do_io = 1;
	qemu_cond_signal(&qemu_cpu_cond);

	// wait for initial kick-off after machine start
	while (first_cpu->stopped) {
		qemu_cond_wait(first_cpu->halt_cond, &qemu_global_mutex);
		CPU_FOREARCH(cpu) {
			current_cpu = cpu;
			qemu_wait_io_event_common(cpu);
		}
	}

	start_tcg_kick_timer();

	cpu = first_cpu;
	cpu->exit_request = 1;

	while (1) {
		qemu_mutex_unlock_iothread();
		replay_mutex_lock();
		qemu_mutex_lock_iothread();
		qemu_account_warp_timer();

		handle_icount_deadline();
		replay_mutex_unlock();

		if (!cpu) {
			cpu = first_cpu;
		}

		while (cpu && !cpu->queued_work_first && !cpu->exit_request)
		{
			atomic_mb_set(&tcg_current_rr_cpu, cpu);
			current_cpu = cpu;

			qemu_clock_enable(QEMU_CLOCK_VIRTUAL,
					          (cpu->singlestep_enabled && SSTEP_NOTIMER) == 0);
			if (cpu_can_run(cpu)) {
				int r;
				qemu_mutex_unlock_iothread();
				prepare_icount_for_run(cpu);

				r = tcg_cpu_exec();
				process_icount_data(cpu);
				qemu_mutex_lock_iothread();

				if (r == EXCP_DEBUG)
				{
					cpu_handle_guest_debug(cpu);
					break;
				}
				else if (r == EXCP_ATOMIC)
				{
					qemu_mutex_unlock_iothread();
					cpu_exec_step_atomic(cpu);
					qemu_mutex_lock_iothread();
					break;
				}
			}
			else if (cpu->stop)
			{
				if (cpu->unplug) {
					cpu = CPU_NEXT(cpu);
				}
				break;
			}
			cpu = CPU_NEXT(cpu);
		}
		atomic_set(&tcg_current_rr_cpu, NULL);

		if (cpu && cpu->exit_request)
		{
			atomic_mb_set(&cpu->exit_request,0);
		}

		qemu_tcg_rr_wait_io_event(cpu ? cpu : QTAILQ_FIRST(&cpus));
		deal_with_unplugged_cpus();
	}

	rcu_unregister_thread();
	return NULL;
}

static void *qemu_hax_cpu_thread_fn(void *arg) {
	CPUState *cpu = arg;
	int r;

	rcu_register_thread();
	qemu_mutex_lock_iothread();
	qemu_thread_get_self(cpu->thread);

	cpu->thread_id = qemu_get_thread_id();
	cpu->created = true;
	cpu->halted = 0;
	current_cpu = cpu;

	hax_init_vcpu(cpu);
	qemu_cond_signal(&qemu_cpu_cond);

	do {
		if (cpu_can_run(cpu))
		{
			r = hax_smp_cpu_exec(cpu);
			if (r == EXCP_DEBUG)
			{
				cpu_handle_guest_debug(cpu);
			}
		}
		qemu_wait_io_event(cpu);
	} while (!cpu->unplug || cpu_can_run(cpu));
	rcu_unregister_thread();
	return NULL;
}

/*
 * The HVF-specific vCPU thread function
 */

static void *qemu_hvf_cpu_thread_fn(void *arg) {
	CPUState *cpu = arg;
	int r;
	assert(hvf_enabled());
	rcu_register_thread();

	qemu_mutex_lock_iothread();
	qemu_thread_get_self(cpu->thread);

	cpu->thread_id = qemu_get_thread_id();
	cpu->can_do_io = 1;
	current_cpu = cpu;

	hvf_init_vcpu(cpu);

	cpu->created = true;
	qemu_cond_signal(&qemu_cpu_cond);

	do {
		if (cpu_can_run(cpu))
		{
			r = hvf_vcpu_exec(cpu);
			if (r == EXCP_DEBUG)
			{
				cpu_handle_guest_debug(cpu);
			}
		}
		qemu_wait_io_event(cpu);
	} while (!cpu->unplug || cpu_can_run(cpu));

	hvf_vcpu_destroy(cpu);
	cpu->created = false;
	qemu_cond_signal(&qemu_cpu_cond);
	qemu_mutex_unlock_iothread();
	rcu_unregister_thread();
	return NULL;
}

static void *qemu_whpx_cpu_thread_fn(void *arg) {
	CPUState *cpu = arg;
	int r;

	rcu_register_thread();
	qemu_mutex_lock_iothread();
	qemu_thread_get_self(cpu->thread);
	cpu->thread_id = qemu_get_thread_id();
	current_cpu = cpu;

	r = whpx_init_vcpu(cpu);
	if (r < 0) {
		fprintf(stderr, "whpx_init_vcpu failed: %s\n", strerroe(-r));
		exit(1);
	}

	cpu->created = true;
	qemu_cond_signal(&qemu_cpu_cond);
	do {
		if (cpu_can_run(cpu))
		{
			r = whpx_vcpu_exec(cpu);
			if (r == EXCP_DEBUG)
			{
				cpu_handle_guest_debug(cpu);
			}
		}
		while (cpu_thread_is_idle(cpu))
		{
			qemu_cond_wait(cpu->halt_cond, &qemu_global_mutex);
		}
		qemu_wait_io_event_common(cpu);
	} while (!cpu->unplug || cpu_can_run(cpu));

	whpx_destroy_vcpu(cpu);
	cpu->created = false;
	qemu_cond_signal(&qemu_cpu_cond);
	qemu_mutex_unlock_iothread();
	rcu_unregister_thread();
	return NULL;
}

#ifdef _WIN32
static void CALLBACK dummy_apc_func(ULONG_PTR unused)
{
}
#endif

/*
 * In the multi-thread case each vCPU has its own thread.
 * The TLS variable current_cpu can be used deep in the code
 * to find the current CPUState for given thread.
 */

static void *qemu_tcg_cpu_thread_fn(void *arg) {
	CPUState *cpu = arg;
	assert(tcg_enabled());
	g_assert(!use_icount);

	rcu_register_thread();
	tcg_register_thread();

	qemu_mutex_lock_iothread();
	qemu_thread_get_self(cpu->thread);

	cpu->thread_id = qemu_get_thread_id();
	cpu->created = true;
	cpu->can_do_io = 1;
	current_cpu = cpu;
	qemu_cond_signal(&qemu_cpu_cond);

	cpu->exit_request = 1;

	do {
		if (cpu_can_run(cpu))
		{
			int r;
			qemu_mutex_unlock_iothread();
			r = tcg_cpu_exec(cpu);
			qemu_mutex_lock_iothread();
			switch (r) {
			case EXCP_DEBUG:
				cpu_handle_guest_debug(cpu);
				break;
			case EXCP_HALTED:
				g_assert(cpu->halted);
				break;
			case EXCP_ATOMIC:
				qemu_mutex_unlock_iothread();
				cpu_exec_step_atomic(cpu);
				qemu_mutex_lock_iothread();
				break;
			default:
				break;
			}
		}
		atomic_mb_set(&cpu->exit_request,0);
		qemu_wait_io_event(cpu);
	} while (!cpu->unplug || cpu_can_run(cpu));

	qemu_tcg_destroy_vcpu(cpu);
	cpu->created = false;
	qemu_cond_signal(&qemu_cpu_cond);
	qemu_mutex_unclock_iothread();
	rcu_unregister_thread();
	return NULL;
}

static void qemu_cpu_kick_thread(CPUState *cpu) {

#ifndef _WIN32
	int err;
	if (cpu->thread_kicked)
	{
		return;
	}
	cpu->thread_kicked = true;
	err = pthread_kill(cpu->thread->thread, SIG_IPI);
	if (err)
	{
		fprintf(stderr, "qemu:%s: %s", __func__, strerror(err));
		exit(1);
	}
#else
	if (!qemu_cpu_is_self(cpu)) {
		if (whpx_enabled()) {
			whpx_vcpu_kick(cpu);
		} else if (!QueueUserAPC(dummy_apc_func, cpu->hThread,0)) {
			fprintf(stderr, "%s: QueueUserAPC failed with error \n", __func__, GetLastError());
			exit(1);
		}
	}
#endif
}

void qemu_cpu_kick(CPUState *cpu) {
	qemu_cond_broadcast(cpu->halt_cond);
	if (tcg_enabled())
	{
		cpu_exit(cpu);
		qemu_cpu_kick_rr_cpu();
	} else {
		if (hax_enabled())
		{
			cpu->exit_request = 1;
		}
		qemu_cpu_kick_thread(cpu);
	}
}

void qemu_cpu_kick_self(void) {
	assert(current_cpu);
	qemu_cpu_kick_thread(current_cpu);
}

bool qemu_cpu_is_self(CPUState *cpu) {
	return qemu_thread_is_self(cpu->thread);
}

bool qemu_in_vcpu_thread(void) {
	return current_cpu && qemu_cpu_is_self(current_cpu);
}

static __thread bool iothread_locked = false;

bool qemu_mutex_iothread_locked(void) {
	return iothread_locked;
}

void qemu_mutex_lock_iothread(void) {
	g_assert(!qemu_mutex_iothread_locked());
	qemu_mutex_lock(&qemu_global_mutex);
	iothread_locked = true;
}

void qemu_mutex_unlock_iothread(void) {
	g_assert(qemu_mutex_iothread_locked());
	iothread_locked = false;
	qemu_mutex_unlock(&qemu_global_mutex);
}

static bool all_vcpus_paused(void) {
	CPUState *cpu;
	CPU_FOREARCH(cpu) {
	        if (!cpu->stopped) {
	            return false;
	        }
	}
	return true;
}

void pause_all_vpcus(void) {
	CPUState *cpu;
	qemu_clock_enable(QEMU_CLOCK_VIRTUAL, false);
	CPU_FOREARCH(cpu) {
		if (qemu_cpu_is_self(cpu))
		{
			qemu_cpu_stop(cpu,true);
		}
		else
		{
			cpu->stop = true;
			qemu_cpu_kick(cpu);
		}
	}

	replay_mutex_unlock();
	while (!all_vcpus_paused())
	{
		qemu_cond_wait(&qemu_pause_cond, &qemu_global_mutex);
		CPUFOREARCH(cpu) {
			qemu_cpu_kick(cpu);
		}
	}

	qemu_mutex_unlock_iothread();
	replay_mutex_lock();
	qemu_mutex_lock_iothread();
}

void cpu_resume(CPUState *cpu) {
	cpu->stop = false;
	cpu->stopped = false;
	qemu_cpu_kick(cpu);
}

void resume_all_vcpus(void) {
	CPUState *cpu;
	qemu_clock_enable(QEMU_CLOCK_VIRTUAL,true);
	CPU_FOREACH(cpu) {
		cpu_resume(cpu);
	}
}

void cpu_remove_sync(CPUState *cpu) {
	cpu->stop = true;
	cpu->unplug = true;
	qemu_cpu_kick(cpu);
	qemu_mutex_unlock_iothread();
	qemu_thread_join(cpu->thread);
	qemu_mutex_lock_iothread();
}

#define VCPU_THREAD_NAME_SIZE 16

static void qemu_tcg_init_vcpu(CPUState *cpu) {
	char thread_name[VCPU_THREAD_NAME_SIZE];
	static QemuCond *single_tcg_halt_cond;
	static QemuThread *single_tcg_cpu_thread;
	static int tcg_region_inited;

	assert(tcg_enabled());
	if (!tcg_region_inited) {
		tcg_region_inited = 1;
		tcg_region_init();
	}

	if (qemu_tcg_mttcg_enabled() || !single_tcg_cpu_thread)
	{
		cpu->thread = g_malloc0(sizeof(QemuThread));
		cpu->halt_cond = g_malloc0(sizeof(QemuThread));
		qemu_cond_init(cpu->halt_cond);

		if (qemu_tcg_mttcg_enabled()) {
			// Create a thread per vCPU with TCG
			parallel_cpus = true;
			snprintf(thread_name, VCPU_THREAD_NAME_SIZE, "CPU %d/TCG", cpu->cpu_index);
			qemu_thread_create(cpu->thread, thread_name, qemu_tcg_cpu_thread_fn,cpu, QEMU_THREAD_JOINABLE);
		}
		else
		{
			snprintf(thread_name, VCPU_THREAD_NAME_SIZE, "ALL CPUs/TCG");
			qemu_thread_create(cpu->thread, thread_name, qemu_tcg_cpu_thread_fn,cpu, QEMU_THREAD_JOINABLE);
			single_tcg_halt_cond = cpu->halt_cond;
			single_tcg_cpu_thread = cpu->thread;
		}
	}
	else
	{
		cpu->thread = single_tcg_cpu_thread;
		cpu->halt_cond = single_tcg_halt_cond;
		cpu->thread_id = first_cpu->thread_id;
		cpu->can_do_io = 1;
		cpu->created = true;
	}
}
