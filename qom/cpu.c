/*
 * cpu.c
 * https://github.com/intel/nemu/blob/topic/virt-x86/qom/cpu.c
 *
 *  Created on: Dec 7, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qom/cpu.h"
#include "sysemu/hw_accel.h"
#include "qemu/notify.h"
#include "qemu/log.h"
#include "exec/log.h"
#include "exec/cpu-common.h"
#include "qemu/error-report.h"
#include "sysemu/sysemu.h"
#include "hw/boards.h"
#include "hw/qdev-properties.h"

CPUInterruptHandler cpu_interrupt_handler;
CPUState *cpu_by_arch_id (int64_t id) 
{
    CPUState *cpu;
    CPU_FOREACH(cpu) {
        CPUClass *cc = CPU_GET_CLASS(cpu);
        if (cc->get_arch_id(cpu) == id)
        {
            return cpu;
        }
    }
    return NULL;
}

bool cpu_exists(int64_t id)
{
    return !!cpu_by_arch_id(id);
}

CPUState *cpu_create(const char *typename) 
{
    Error *err = NULL;
    CPUState *cpu = CPU(object_new(typename));
    object_property_set_bool(OBJECT(cpu), true, "realized", &err);
    if (err != NULL)
    {
        error_report_err(err);
        object_unref(OBJECT(cpu));
        exit(EXIT_FAILURE);
    }
    return cpu;
}

bool cpu_paging_enabled(const CPUState *cpu)
{
    CPUClass *cc = CPU_GET_CLASS(cpu);
    return cc->get_paging_enabled(cpu);
}

static bool cpu_common_get_paging_enabled(const CPUState *cpu)
{
    return false;
}

void cpu_get_memory_mapping (CPUState *cpu, MemoryMappingList *list, Error **errp)
{
    CPUClass *cc = CPU_GET_CLASS(cpu);
    cc->get_memory_mapping(cpu, list, errp);
}

static void cpu_common_get_memory_mapping(CPUState *cpu, MemoryMappingList *list, Error **errp)
{
    error_setg(errp, "Obtaining memory mappings is unsupported on this CPU.");
}

void cpu_reset_interrupt(CPUState *cpu, int mask)
{
    bool need_lock = !qemu_mutex_iothread_locked();
    if (need_lock)
    {
        qemu_mutex_lock_iothread();
    }
    cpu->interrupt_request &= ~mask;
    if (need_lock)
    {
        qemu_mutex_unlock_iothread();
    }
}

void cpu_exit(CPUState *cpu) 
{
    atomic_set(&cpu->exit_request, i);
    smp_wmb();
    atomic_set(&cpu->icount_decr.u16.high, -1);
}

int cpu_write_elf32_qemunote(WriteCoreDumpFunction f, CPUState *cpu, void* opaque)
{
    CPUClass *cc = CPU_GET_CLASS(cpu);
    return (*cc->write_elf32_qemunote)(f, cpu, opaque);
}

static int cpu_common_write_elf32_qemunote(WriteCoreDumpFunction f, CPUState *cpu, 
                                           void *opaque)
{
    return 0;
}             

int cpu_write_elf32_note(WriteCoreDumpFunction f, CPUState *cpu, 
                         int cpuid, void *opaque)
{
    CPUClass *cc = CPU_GET_CLASS(cpu);
    return (*cc->write_elf32_note)(f, cpu, cpuid, opaque);
}

static int cpu_common_get_elf32_note(WriteCoreDumpFunction *f, CPUState *cpu,
                                     int cpuid, void *opaque)
{
    return -1;
}

int cpu_write_elf64_qemunote(WriteCoreDumpFunction f, CPUState *cpu, void* opaque)
{
    CPUClass *cc = CPU_GET_CLASS(cpu);
    return (*cc->write_elf64_qemunote)(f, cpu, opaque);
}

static int cpu_common_write_elf64_qemunote(WriteCoreDumpFunction f, CPUState *cpu, 
                                           void *opaque)
{
    return 0;
}             

int cpu_write_elf64_note(WriteCoreDumpFunction f, CPUState *cpu, 
                         int cpuid, void *opaque)
{
    CPUClass *cc = CPU_GET_CLASS(cpu);
    return (*cc->write_elf64_note)(f, cpu, cpuid, opaque);
}

static int cpu_common_get_elf64_note(WriteCoreDumpFunction *f, CPUState *cpu,
                                     int cpuid, void *opaque)
{
    return -1;
}

static int cpu_common_gdb_read_register(CPUState *cpu, uint8_t *buf, int reg)
{
    return 0;
}

static int cpu_common_gdb_write_register(CPUState *cpu, uint8_t *buf, int reg) 
{
    return 0;
}

static bool cpu_common_debug_check_watchpoint(CPUState *cpu, CPUWatchpoint *wp) 
{
    return true;
}

bool target_words_bigendian(void);
static bool cpu_common_virtio_is_big_endian(CPUState *cpu)
{
    return target_words_bigendian();
}

static void cpu_common_noop(CPUState *cpu)
{}

static bool cpu_common_exec_interrupt(CPUState *cpu, int int_rep)
{
    return false;
}

GuestPanicInformation *cpu_get_crash_info(CPUState *cpu)
{
    CPUClass *cc = CPU_GET_CLASS(cpu);
    GuestPanicInformation *res = NULL;

    if (cc->get_crash_info)
    {
        res = cc->get_crash_info(cpu);
    }
    return res;
}
