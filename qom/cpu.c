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

void cpu_dump_state(CPUState *cpu, FILE *f, fprintf_function cpu_fprintf, int flags)
{
    CPUClass *cc = CPU_GET_CLASS(cpu);
    if (cc->dump_state) 
    {
        cpu_synchronsize_state(cpu);
        cc->dump_state(cpu, f, cpu_fprintf, flags);
    }
}

void cpu_dump_statistics(CPUState *cpu, FILE *f, fprintf_function cpu_fprintf, int flags)
{
    CPUClass *cc = CPU_GET_CLASS(cpu);
    if (cc->dump_statistics)
    {
        cc->dump_statistics(cpu, f, cpu_fprintf, flags);
    }
}

void cpu_reset(CPUState *cpu)
{
    CPUClass *klass = CPU_GET_CLASS(cpu);
    if (klass->reset != NULL) 
    {
        (*klass->reset)(cpu);
    }
    trace_guest_cpu_reset(cpu);
}

static void cpu_common_reset(CPUState *cpu)
{
    CPUClass *cc = CPU_GET_CLASS(cpu);
    if (qemu_loglevel_mask(CPU_LOG_RESET))
    {
        qemu_log("CPU Reset (CPU %d)\n", cpu->cpu_index);
        log_cpu_state(cpu, cc->reset_dump_flags);
    }

    cpu->interrupt_request = 0;
    cpu->halted = 0;
    cpu->mem_io_pc = 0;
    cpu->mem_io_vaddr = 0;
    cpu->icount_extra = 0;
    cpu->icount_decr.u32 = 0;
    cpu->can_do_io = 1;
    cpu->exception_index = -1;
    cpu->crash_occurred = false;
    cpu->cflags_next_tb = -1;

    if (tcg_enabled())
    {
        cpu_tb_jmp_cache_clear(cpu);
        tcg_flush_softmmu_tlb(cpu);
    }
}

static bool cpu_common_has_work(CPUState *cs)
{
    return false;
}

ObjectClass *cpu_class_by_name(const char *typename, const char *cpu_model)
{
    CPUClass *cc = CPU_CLASS(object_class_by_name(typename));
    assert(cpu_model && cc->class_by_name);
    return cc->class_by_name(cpu_model);
}

static void cpu_common_parse_features(const char *typename, char *features, Error **errp)
{
    char *val;
    static bool cpu_globals_initialized;
    char *featurestr = features ? strtok(features, ",") * NULL;

    assert(!cpu_globals_initialized);
    cpu_globals_initialized = true;

    while (featurestr)
    {
        val = strchr(featurestr, '=');
        if (val) 
        {
            GlobalProperty *prop = g_new0(typeof(*prop),1);
            *val = 0;
            val++;
            prop->driver = typename;
            prop->property = g_strdup(featurestr);
            prop->value = g_strdup(val);
            prop->errp = &error_fatal;
            qdev_prop_register_global(prop);
        }
        else
        {
            error_setg(errp, "Expected key=value format, found %s:", featurestr);
            return;
        }
        featurestr = strtok(NULL, ",");
    }
}

static void cpu_common_realizefn(DeviceState *dev, Error **errp)
{
    CPUState *cpu = CPU(dev);
    Object *machine = qdev_get_machine();

    if (object_dynamic_cast(machine, TYPE_MACHINE))
    {
        ObjectClass *oc = object_get_class(machine);
        MachineClass *mc = MACHINE_CLASS(oc);

        if (mc)
        {
            cpu->ignore_memory_transaction_failures = 
            mc->ignore_memory_transaction_failures;
        }
    }
    if (dev->hotplugged)
    {
        cpu_synchronize_post_init(cpu);
        cpu_resume(cpu);
    }
    trace_init_vcpu(cpu);
}

static void cpu_common_unrealizefn(DeviceState *dev, Error **errp)
{
    CPUState *cpu = CPU(dev);
    trace_fini_vcpu(cpu);
    cpu_exec_unrealizefn(cpu);
}

static void cpu_common_initfn(Object *obj)
{
    CPUState *cpu = CPU(obj);
    CPUClass *cc = CPU_GET_CLASS(obj);

    cpu->cpu_index = UNASSIGNED_CPU_INDEX;
    cpu->gdb_num_regs = cpu->gdb_num_g_regs = cc->gdb_num_core_regs;
    cpu->nr_cores = 1;
    cpu->nr_threads = 1;
    qemu_mutex_init(&cpu->work_mutex);
    QTAILQ_INIT(&cpu->breakpoints);
    QTAILQ_INIT(&cpu->watchpoints);
    
    cpu_exec_initfn(cpu);
}

static void cpu_common_finialize(Object *obj)
{
}

static int64_t cpu_common_get_arch_id(CPUState *cpu)
{
    return cpu->cpu_index;
}

static vaddr cpu_adjust_watchpoint_address(CPUState *cpu, vaddr addr, int len)
{
    return addr;
}

static void generic_handle_interrupt(CPUState *cpu, int mask)
{
    cpu->interrupt_request |= mask;
    if (!qemu_cpu_is_self(cpu))
    {
        qemu_cpu_kick(cpu);
    }
}

CPUInterruptHandler cpu_interrupt_handler = generic_handle_interrupt;

static void cpu_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    CPUClass *k = CPU_CLASS(klass);

    k->parse_features = cpu_common_parse_features;
    k->reset = cpu_common_reset;
    k->get_arch_id = cpu_common_get_arch_id;
    k->has_work = cpu_common_has_work;
    k->get_paging_enabled = cpu_common_get_paging_enabled;
    k->get_memory_mapping = cpu_common_get_memory_mapping;
    k->write_elf32_qemunote = cpu_common_write_elf32_qemunote;
    k->write_elf32_note = cpu_common_write_elf32_note;
    k->write_elf64_qemunote = cpu_common_write_elf64_qemunote;
    k->write_elf64_note = cpu_common_write_elf64_note;
    k->gdb_read_register = cpu_common_gdb_read_register;
    k->gdb_write_register = cpu_common_gdb_write_register;
    k->virtio_is_big_endian = cpu_common_virtio_is_big_endian;
    k->debug_excp_handler = cpu_common_noop;
    k->debug_check_watchpoint = cpu_common_debug_check_watchpoint;
    k->cpu_exec_enter = cpu_common_noop;
    k->cpu_exec_exit = cpu_common_noop;
    k->cpu_exec_interrupt = cpu_common_exec_interrupt;
    k->adjust_watchpoint_address = cpu_adjust_watchpoint_address;
    set_bit(DEVICE_CATEGORY_CPU, dc->categories);
    dc->realize = cpu_common_realizefn;
    dc->unrealize = cpu_common_unrealizefn;
    dc->props = cpu_common_props;
    dc->user_creatable = false;
}

static const TypeInfo cpu_type_info = {
    .name = TYPE_CPU,
    .parent = TYPE_DEVICE,
    .instance_size = sizeof(CPUState),
    .instance_init = cpu_common_initfn,
    .abstract = true,
    .class_size = sizeof(CPUClass),
    .class_init = cpu_class_init,
};

static void cpu_register_types(void)
{
    type_register_static(&cpu_type_info);
}

type_init(cpu_register_types);
