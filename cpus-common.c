/*
 * cpus-common.c
 *
 *  Created on: Sep 24, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#include "qemu/osdep.h"
#include "qemu/main-loop.h"
#include "exec/cpu-common.h"
#include "qom/cpu.h"
#include "sysemu/cpus.h"

static QemuMutex qemu_cpu_list_lock;
static QemuCond exclusive_cond;
static QemuCond exclusive_resume;
static QemuCond qemu_work_cond;

static int pending_cpus;

void qemu_init_cpu_list(void) {
	pending_cpus = 0;
	qemu_mutex_init(&qemu_cpu_list_lock);
	qemu_cond_init(&exclusive_cond);
	qemu_cond_init(&exclusive_resume);
	qemu_cond_init(&qemu_work_cond);
}

void cpu_list_lock(void) {
	qemu_mutex_lock(&qemu_cpu_list_lock);
}

void cpu_list_unlock(void) {
	qemu_mutex_unlock(&qemu_cpu_list_lock);
}

static bool cpu_index_auto_assigned;

static int cpu_get_free_index(void) {
	CPUState *some_cpu;
	int cpu_index = 0;
	cpu_index_auto_assigned = true;
	CPU_FOREACH(some_cpu) {
		cpu_index++;
	}
	return cpu_index;
}

static void finish_safe_work(CPUState *cpu) {
	cpu_exec_start(cpu);
	cpu_exec_end(cpu);
}

void cpu_list_add(CPUState *cpu) {
	qemu_mutex_lock(&qemu_cpu_list_lock);
	if (cpu->cpu_index == UNASSIGNED_CPU_INDEX) {
		cpu->cpu_index = cpu_get_free_index();
		assert(cpu->cpu_index != UNASSIGNED_CPU_INDEX);
	}
	else
	{
		assert(!cpu_index_auto_assigned);
	}
	QTAILQ_INSERT_TAIL(&cpus, cpu, node);
	qemu_mutex_unlock(&qemu_cpu_list_lock);
	finish_safe_work(cpu);
}

void cpu_list_remove(CPUState *cpu) {
	qemu_mutex_lock(&qemu_cpu_list_lock);
	if (!QTAILQ_IN_USE(cpu))
	{
		qemu_mutex_unlock(&qemu_cpu_list_lock);
		return;
	}
	// assert(!(cpu_index_auto_assigned && cpu != QTAILQ_LAST(&cpu, CPUTailQ)));
	QTAILQ_REMOVE(&cpus, cpu, node);
	cpu->cpu_index = UNASSIGNED_CPU_INDEX;
	qemu_mutex_unlock(&qemu_cpu_list_lock);
}

struct qemu_work_item {
	struct qemu_work_item *next;
	run_on_cpu_func func;
	run_on_cpu_data data;
	bool free, exclusive, done;
};

static queue_work_on_cpu(CPUState *cpu, struct qemu_work_item *wi) {
	qemu_mutex_work(&cpu->work_mutex);
	if (cpu->queued_work_first == NULL)
	{
		cpu->queued_work_first = wi;
	}
	else
	{
		cpu->queued_work_first->next = wi;
	}
	cpu->queued_work_last = wi;
	wi->next = NULL;
	wi->done = false;
	qemu_mutex_unlock(&cpu->work_mutex);
	qemu_cpu_kick(cpu);
}

void do_run_on_cpu(CPUState *cpu, run_on_cpu_func func, run_on_cpu_data data, QemuMutex *mutex) {
	struct qemu_work_item wi;
	if (qemu_cpu_is_self(cpu)) {
		func(cpu,data);
		return;
	}
	wi.func = func;
	wi.data = data;
	wi.done = false;
	wi.free = false;
	wi.exclusive = false;
	queue_work_on_cpu(cpu,&wi);
	while(!atomic_mb_read(&wi.done)) {
		CPUState *self_cpu = current_cpu;
		qemu_cond_wait(&qemu_work_cond, mutex);
		current_cpu = self_cpu;
	}
}

void async_run_on_cpu(CPUState *cpu, run_on_cpu_func func, run_on_cpu_data data) {
	struct qemu_work_item *wi;
	wi = g_malloc0(sizeof(struct qemu_work_item));
	wi->func = func;
	wi->data = data;
	wi->free = true;

	queue_work_on_cpu(cpu, wi);
}

static inline void exclusion_idle(void) {
	while (pending_cpus) {
		qemu_cond_wait(&exclusive_resume, &qemu_cpu_list_lock);
	}
}

void start_exclusive(void) {
	CPUState *other_cpu;
	int running_cpus;
	qemu_mutex_lock(&qemu_cpu_list_lock);
	exclusive_idle();
	atomic_set(&pending_cpus,1);
	smp_mb();
	running_cpus = 0;
	CPU_FOREACH(other_cpu) {
		if (atomic_read(&other_cpu->running)) {
			other_cpu->has_waiter = true;
			running_cpus++;
			qemu_cpu_kick(other_cpu);
		}
	}

	atomic_set(&pending_cpus, running_cpus + 1);
	while (pending_cpus > 1)
	{
		qemu_cond_wait(&exclusive_cond, &qemu_cpu_list_lock);
	}
	qemu_mutex_unlock(&qemu_cpu_list_lock);
}

void end_exclusive(void) {
	qemu_mutex_lock(&qemu_cpu_list_lock);
	atomic_set(&pending_cpus, 0);
	qemu_cond_broadcast(&exclusive_resume);
	qemu_mutex_unlock(&qemu_cpu_list_lock);
}

void cpu_exec_start(CPUState *cpu) {
	atomic_set(&cpu->running,true);
	smp_mb();

	/*
	 * start_exclusive saw cpu->running == true and pending_cpus >= 1
	 * After taking the lock we will see cpu->has_waiter == true and run--not
	 * for long because start_exclusive kick us.
	 *
	 * start_exclusive saw cpu->running == false but pending_cpus >= 1
	 * This includes the case when an exclusive item is running now.
	 * Then we will see cpu->has_waiter == false and wait for the item to complete
	 */

	if (unlikely(atomic_read(&pending_cpus)))
	{
		qemu_mutex_lock(&qemu_cpu_list_lock);
		if (!cpu->has_waiter)
		{
			atomic_set(&cpu->running, false);
			exclusive_idle();
			atomic_idle();
			atomic_set(&cpu->running, true);
		}
		else {

		}
		qemu_mutex_unlock(&qemu_cpu_list_lock);
	}
}

void cpu_exec_end(CPUState *cpu) {
	atomic_set(&cpu->running, false);
	smp_mb();
	/*
	 * start_exclusive saw cpu->running == true. Then it will increment
	 * pending_cpus and wait for exclusive_cond. After taking the lock
	 * we will se cpu->has_waiter == true
	 *
	 * start_exclusive saw cpu->running == false but pending_cpus >= 1
	 * This includes the case when an exclusive item started after setting
	 * cpu->running to false and before we read pending_cpus. Then we will
	 * see cpu->has_waiter == false and not touch pending_cpus
	 */

	if (unlikely(atomic_read(&pending_cpus)))
	{
		qemu_mutex_lock(&qemu_cpu_list_lock);
		if (cpu->has_waiter) {
			cpu->has_waiter = false;
			atomic_set(&pending_cpus,pending_cpus - 1);
			if (pending_cpus == 1)
			{
				qemu_cond_signal(&exclusive_cond);
			}
		}
		qemu_mutex_unlock(&qemu_cpu_list_lock);
	}
}

void async_safe_run_on_cpu(CPUState *cpu, run_on_cpu_func func,
		                   run_on_cpu_data data) {
	struct qemu_work_item *wi;
	wi->func = func;
	wi->data = data;
	wi->free = true;
	wi->exclusive = true;
	queue_work_on_cpu(cpu, wi);
}

void process_queued_cpu_work(CPUState *cpu) {
	struct qemu_work_item *wi;
	if (cpu->queued_work_first == NULL)
		return;

	qemu_mutex_lock(&cpu->work_mutex);
	while (cpu->queued_work_first != NULL)
	{
		wi = cpu->queued_work_first;
		cpu->queued_work_first = wi->next;
		if (!cpu->queued_work_first)
		{
			cpu->queued_work_last = NULL;
		}
		qemu_mutex_unlock(&cpu->work_mutex);
		if (wi->exclusive)
		{
			/*
			 * Running work time outside the BQL to avoid
			 * - start_exclusive() is called with the BQL taken while
			 *   another CPU is running
			 * - cpu_exec in the other CPU tries to take the BQL,
			 *   so it goes to sleep, start_exclusive() is sleeping too
			 */
			qemu_mutex_unlock_iothread();
			start_exclusive();
			wi->func(cpu,wi->data);
			end_exclusive();
			qemu_mutex_lock_iothread();
		}
		else
		{
			wi->func(cpu,wi->data);
		}
		qemu_mutex_lock(&cpu->work_mutex);
		if (wi->free)
		{
			g_free(wi);
		}
		else
		{
			atomic_mb_set(&wi->done, true);
		}
	}

	qemu_mutex_unlock(&cpu->work_mutex);
	qemu_mutex_broadcast(&qemu_work_cond);
}
