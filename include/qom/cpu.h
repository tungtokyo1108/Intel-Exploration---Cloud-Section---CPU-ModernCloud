/*
 * cpu.h
 *
 *  Created on: Sep 7, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QOM_CPU_H_
#define QOM_CPU_H_

#include "hw/qdev-core.h"
#include "disas/bfd.h"
#include "exec/hwaddr.h"
#include "exec/memattrs.h"
#include "qemu/bitmap.h"
#include "qemu/queue.h"
#include "qemu/thread.h"
#include "qemu/typedefs.h"
#include "fprintf-fn.h"

typedef int (*WriteCoreDumpFunction)(const void *buf, size_t size, void *opaque);
typedef uint64_t vaddr;
#define VADDR_PRId PRId64
#define VADDR_PRIu PRIu64
#define VADDR_PRIo PRIo64
#define VADDR_PRIx PRIx64
#define VADDR_PRIX PRIX64
#define VADDR_MAX  UINT64_MAX

#define TYPE_CPU "cpu"
#define CPU(obj) ((CPUState *)(obj))
#define CPU_CLASS(class) OBJECT_CLASS_CHECK(CPUClass, (class), TYPE_CPU)
#define CPU_GET_CLASS(obj) OBJECT_GET_CLASS(CPUClass, (obj), TYPE_CPU)

typedef enum MMUAccessType {
	MMU_DATA_LOAD = 0,
	MUU_DATA_STORE = 1,
	MUU_INST_FETCH = 2
} MMUAccessType;

typedef struct CPUWatchpoint CPUWatchpoint;
typedef void (*CPUUnassignedAccess)(CPUState *cpu, hwaddr addr, bool is_write, bool is_exec, int opaque, unsigned size);
typedef struct TranslationBlock TranslationBlock;
typedef struct disassemble_info disassemble_info;
typedef struct GuestPanicInformation GuestPanicInformation;
typedef struct CPUClass CPUClass;
typedef struct CPUClass {
	DeviceClass parent_class;

	ObjectClass *(*class_by_name)(const char *cpu_model);
	void (*parse_features)(const char *type_name, char *str, Error **errp);
	void (*reset)(CPUState *cpu);
	int reset_dump_flags;
	bool (*has_work)(CPUState *cpu);
	void (*do_interrupt)(CPUState *cpu);
	CPUUnassignedAccess do_unassigned_access;
	void (*do_unaligned_access)(CPUState *cpu, vaddr addr, MMUAccessType access_type, int mmu_idx, uintptr_t readdr);
	void (*do_transaction_failed)(CPUState *cpu, hwaddr physaddr, vaddr addr, unsigned size, MMUAccessType access_type, int mmu_idx,
			                      MemTxAttrs attrs, MemTxResult response, uintptr_t retaddr);
	bool (*virtio_is_big_endian)(CPUState *cpu);
	int (*memory_rw_debug)(CPUState *cpu, vaddr addr, uint8_t *buf, int len, bool is_write);
	void (*dump_state)(CPUState *cpu, FILE *f, fprintf_function cpu_fprintf, int flags);
	GuestPanicInformation *(*get_crash_info)(CPUState *cpu);
	void (*dump_statistics)(CPUState *cpu, FILE *f, fprintf_function cpu_fprintf, int flags);
	int64_t (*get_arch_id)(CPUState *cpu);
	bool (*get_paging_enabled)(const CPUState *cpu);
	void (*get_memory_mapping)(CPUState *cpu, MemoryMappingList *list, Error *errp);
	void (*set_pc)(CPUState *cpu, vaddr value);
	void (*synchronize_from_tb)(CPUState *cpu, TranslationBlock *tb);
	int (*handle_mmu_fault)(CPUState *cpu, vaddr address, int size, int rw, int mmu_index);
	hwaddr (*get_phys_page_debug)(CPUState *cpu, vaddr addr);
	hwaddr (*get_phys_page_attrs_debug)(CPUState *cpu, vaddr addr, MemTxAttrs *attrs);
	int (*asidx_from_attrs)(CPUState *cpu, MemTxAttrs attrs);
	int (*gdb_read_register)(CPUState *cpu, uint8_t *buf, int reg);
	int (*gdb_write_register)(CPUState *cpu, uint8_t *buf, int reg);
	bool (*debug_check_watchpoint)(CPUState *cpu, CPUWatchpoint *wp);
	void (*debug_excp_handler)(CPUState *cpu);
	int (*write_elf64_note)(WriteCoreDumpFunction f, CPUState *cpu, int cpuid, void *opaque);
	int (*write_elf64_qemunote)(WriteCoreDumpFunction f, CPUState *cpu, void *opaque);
	int (*write_elf32_note)(WriteCoreDumpFunction f, CPUState *cpu, int cpuid, void *opaque);
    int (*write_elf32_qemunote)(WriteCoreDumpFunction f, CPUState *cpu, void *opaque);
    const struct VMStateDescription *vmsd;
    const char *gdb_core_xml_file;
    gchar *(*gdb_core_name)(CPUState *cpu);
    const char *(*gdb_get_dynamic_xml)(CPUState *cpu, const char *xmlname);
    void (*cpu_exec_enter)(CPUState *cpu);
    void (*cpu_exec_exit)(CPUState *cpu);
    bool (*cpu_exec_interrupt)(CPUState *cpu, const char *xmlname);
    void (*disas_set_info)(CPUState *cpu, disassemble_info *info);
    vaddr (*adjust_watchpoint_address)(CPUState *cpu, vaddr addr, int len);
    void (*tcg_initialize)(void);

    int bdb_num_core_regs;
    bool gdb_stop_before_watchpoint;
} CPUClass;

typedef struct icount_decr_u16 icount_decr_u16;
typedef struct icount_decr_u16 {
	uint16_t low;
	uint16_t high;
} icount_decr_u16;

typedef struct CPUWatchpoint {
	vaddr _vaddr;
	vaddr len;
	vaddr hitaddr;
	int flags;
	QTAILQ_ENTRY(CPUWatchpoint) entry;
} CPUWatchpoint;

struct KVMState;
struct kvm_run;
struct hax_vcpu_state;

#define TB_JMP_CACHE_BITS 12
#define TB_JMP_CACHE_SIZE (1 << TB_JMP_CACHE_BITS)

typedef union {
	int host_int;
	unsigned long host_ulong;
	void *host_ptr;
	vaddr target_ptr;
} run_on_cpu_data;

#define RUN_ON_CPU_HOST_PTR(p)    ((run_on_cpu_data){.host_ptr = (p)})
#define RUN_ON_CPU_HOST_INT(i)    ((run_on_cpu_data){.host_int = (i)})
#define RUN_ON_CPU_HOST_ULONG(ul) ((run_on_cpu_data){.host_ulong = (ul)})
#define RUN_ON_CPU_TARGET_PTR(v)  ((run_on_cpu_data){.target_ptr = (v)})
#define RUN_ON_CPU_NULL           RUN_ON_CPU_HOST_PTR(NULL)

typedef void (*run_on_cpu_func)(CPUState *cpu, run_on_cpu_data data);

struct qemu_work_item;
#define CPU_UNSET_NUMA_NODE_ID -1
#define CPU_TRACE_DSTATE_MAX_EVENTS 32

typedef struct GArray GArray;
typedef struct CPUState CPUState;
typedef struct CPUState {
	DeviceState parent_obj;

	int nr_cores; // Number of cores within this CPU package
	int nr_threads; // Number of threads within this CPU.
	struct QemuThread *thread;
#ifdef _WIN32
    HANDLE hThread;
#endif
    int thread_id;
    bool running, has_waiter; // CPU is currently running // a CPU is currently waiting for the cpu_exec_end
    struct QemuCond *halt_cond;
    bool thread_kicked;
    bool created; // Indicates whether the CPU thread has been successfully created
    bool stop; // Indicate a pending stop request
    bool stopped; // Indicates the CPU has been artificially stopped
    bool unplug; // Indicate a pending CPU unplug request.
    bool crash_occurred; // Indicate the OS reported a crash for this CPU
    bool exit_request;
    uint32_t cflags_next_tb;
    uint32_t interrupt_request; // Indicate a pending interrupt request
    int singlestep_enabled; // Flags for single stepping
    int64_t icount_budget;
    int64_t icount_extra; // Instruction until next timer event
    sigjmp_buf jmp_env; //
    // QemuMutex work_mutex;
    struct qemu_work_item *queued_work_first, *queued_work_last;
    CPUAddressSpace *cpu_ases; // Pointer to array of CPUAddressSpace
    int num_ases; // Number of CPUAddressSpace in @cpu_ases
    AddressSpace *as; // Pointer to the first AddressSpace, for the convenience of targets which only have a single AddressSpace
    MemoryRegion *memory;
    void *env_ptr; // Pointer to subclass-specific CPUArchState field
    struct TranslationBlock *tb_jmp_cache[TB_JMP_CACHE_SIZE]; // Additional GDB registers
    struct GDBRegisterState *gdb_regs;
    int gdb_num_regs; // Number of total registers accessible to GDB
    int gdb_num_g_regs; // Number of registers in GDB "g" packets
    QTAILQ_ENTRY(CPUState) node;
    QTAILQ_HEAD(breakpoints_head, CPUBreakpoint) breakpoints;
    CPUWatchpoint *watchpoint_hit;
    void *opaque; // User data
    uintptr_t mem_io_pc; // Host Program Counter at which the memory was accessed
    vaddr mem_io_vaddr; // Target virtual address at which the memory was accessd
    int kvm_fd; // vCPU file descriptor to KVM
    struct KVMState *kvm_state;
    struct kvm_run *kvm_run;
    DECLARE_BITMAP(trace_dstate_delayed, CPU_TRACE_DSTATE_MAX_EVENTS);
    DECLARE_BITMAP(trace_dstate, CPU_TRACE_DSTATE_MAX_EVENTS);

    int cpu_index;
    uint32_t halted;
    uint32_t can_do_io;
    int32_t exception_index;

    bool vcpu_dirty;
    bool throttle_thread_sheduled;
    bool ignore_memory_transaction_failtures;

    union {
    	uint32_t u32;
    	icount_decr_u16 u16;
    } icount_decr;

    struct hax_vcpu_state *hax_vcpu;
    uint16_t pending_tlb_flush;
    int hvf_fd;
    GArray *iommu_notifiers;

} CPUState;

QTAILQ_HEAD(CPUTailQ,CPUState);
extern struct CPUTailQ cpus;
#define CPU_NEXT(cpu) QTAILQ_NEXT(cpu, node)
#define CPU_FOREACH(cpu) QTAILQ_FOREACH(cpu, &cpu, node)
#define CPU_FOREACH_SAFE(cpu, next_cpu) \
	QTAILQ_FOREACH_SAFE(cpu, &cpu, node, next_cpu)
#define CPU_FOREACH_REVERSE(cpu) \
	QTAILQ_FOREACH_REVERSE(cpu, &cpu, CPUTailQ, node)
#define first_cpu QTAILQ_FIRST(&cpu)

extern __thread CPUState *current_cpu;
static inline void cpu_tb_jmp_cache_clear(CPUState *cpu) {
	unsigned int i;
	for (i=0; i < TB_JMP_CACHE_SIZE; i++) {
		atomic_set(&cpu->tb_jmp_cache[i], NULL);
	}
}

// Check whether we are running MultiThread TCG or not
extern bool mttcg_enabled;
#define qemu_tcg_mttcg_enabled() (mttcg_enabled)
bool cpu_paging_enabled(const CPUState *cpu);
/**
 * @cpu: The CPU whose memory mappping are to be obtained
 * @list: Where to write the memory mappings to
 */
void cpu_get_memory_mapping(CPUState *cpu, MemoryMappingList *list, Error **errp);

/**
 * @f: pointer to a function that writes memory to a file
 * @cpu: the CPU whose mapping is to be dumped
 * @cpuid: ID number of the CPU
 * @opaque: pointer to the CPUState struct
 */

int cpu_write_elf64_note(WriteCoreDumpFunction f, CPUState *cpu, int cpuid, void *opaque);
int cpu_write_elf64_qemunote(WriteCoreDumpFunction f, CPUState *cpu, void *opaque);
int cpu_write_elf32_note(WriteCoreDumpFunction f, CPUState *cpu, int cpuid, void *opaque);
int cpu_write_elf32_qemunote(WriteCoreDumpFunction f, CPUState *cpu, void *opaque);

GuestPanicInformation *cpu_get_crash_info(CPUState *cpu);
enum CPUDumpFlags {
    CPU_DUMP_CODE = 0x00010000,
    CPU_DUMP_FPU  = 0x00020000,
    CPU_DUMP_CCOP = 0x00040000,
};

void cpu_dump_statistics(CPUState *cpu, FILE *f, fprintf_function cpu_fprintf, int flags);

/**
 * Obtain the physical page corresponding to a virtual page
 * together with the coresponding memory transaction attributes to use for the access
 */

static inline hwaddr cpu_get_phys_page_attrs_debug(CPUState *cpu, vaddr addr, MemTxAttrs *attrs) {
	CPUClass *cc = CPU_GET_CLASS(cpu);
	if (cc->get_phys_page_attrs_debug) {
		return cc->get_phys_page_attrs_debug(cpu, addr, attrs);
	}
	*attrs = MEMTXATTRS_UNSPECIFIED;
	return cc->get_phys_page_debug(cpu,addr);
}

/**
 * @cpu: The CPU to obtain the physical page address for
 * Obtain the physical page corresponding to a virtual one
 */

static inline hwaddr cpu_get_phys_page_debug(CPUState *cpu, vaddr addr) {
	MemTxAttrs attrs = {};
	return cpu_get_phys_page_attrs_debug(cpu,addr,&attrs);
}

/**
 * Return the address space index specifying the CPU AddressSpace
 * to use for memory access with the given transaction attributes.
 */

static inline int cpu_asidx_from_attrs(CPUState *cpu, MemTxAttrs attrs) {
	CPUClass *cc = CPU_GET_CLASS(cpu);
	int ret = 0;
	if (cc->asidx_from_attrs) {
		ret = cc->asidx_from_attrs(cpu,attrs);
		assert(ret < cpu->num_ases && ret >= 0);
	}
	return ret;
}

void cpu_list_add(CPUState *cpu);
void cpu_list_remove(CPUState *cpu);
void cpu_reset(CPUState *cpu);

ObjectClass *cpu_class_by_name(const char *type_name, const char *cpu_moodel);
CPUState *cpu_create(const char *type_name);

// processes optimal parameters and registers them as global properties
const char *parse_cpu_model(const char *cpu_model);

// check whether the CPU has work to do
static inline bool cpu_has_work(CPUState *cpu) {
	CPUClass *cc = CPU_GET_CLASS(cpu);
	g_assert(cc->has_work);
	return cc->has_work(cpu);
}

// check whether the caller is executing on the vCPU thread
bool qemu_cpu_is_self(CPUState *cpu);
void qemu_cpu_kick(CPUState *cpu);
bool cpu_is_stopped(CPUState *cpu);
void do_run_on_cpu(CPUState *cpu, run_on_cpu_func func, run_on_cpu_data data, QemuMutex *mutex);
void run_on_cpu(CPUState *cpu, run_on_cpu_func func, run_on_cpu_data data);

// Schedules the function for execution on the vCPU @cpu asynchronously.
void async_run_on_cpu(CPUState *cpu, run_on_cpu_func func, run_on_cpu_data data);

// Schedules the function for execution on the vCPU @cpu asynchronously.
// while all other vCPU are sleeping

void async_safe_run_on_cpu(CPUState *cpu, run_on_cpu_func func, run_on_cpu_data data);

// Get a CPU matching @index
CPUState *qemu_get_cpu(int index);
bool cpu_exists(int64_t id);
CPUState *cpu_by_arch_id(int64_t id);

/**
 * Throttle all vcpus by forcing them to sleep for the given percentage of time.
 * Can be called as needed to adjust new_throttle_pct
 */

void cpu_throttle_set(int new_throttle_pct);
void cpu_throttle_stop(void);
bool cpu_throttle_active(void);
int cpu_throttle_get_percentage(void);
typedef void (*CPUInterruptHandler)(CPUState *,int);
extern CPUInterruptHandler cpu_interrupt_handler;
static inline void cpu_interrupt(CPUState *cpu, int mask) {
	cpu_interrupt_handler(cpu,mask);
}
void cpu_interrupt(CPUState *cpu, int mask);
static inline void cpu_unassigned_access(CPUState *cpu, hwaddr addr, bool is_write, bool is_exec, int opaque, unsigned size) {
	CPUClass *cc = CPU_GET_CLASS(cpu);
	if (cc->do_unassigned_access) {
		cc->do_unassigned_access(cpu, addr, is_write, is_exec, opaque, size);
	}
}

static inline void cpu_transaction_failed(CPUState *cpu, hwaddr physaddr, vaddr addr, unsigned size, MMUAccessType access_type,
		                                  int mmu_idx, MemTxAttrs attrs, MemTxResult response, uintptr_t retaddr) {
	CPUClass *cc = CPU_GET_CLASS(cpu);
	if (!cpu->ignore_memory_transaction_failtures && cc->do_transaction_failed) {
		cc->do_transaction_failed(cpu, physaddr,addr,size,access_type,mmu_idx,attrs,response,retaddr);
	}
}

// Sets the program counter for a CPU
static inline void cpu_set_pc(CPUState *cpu, vaddr addr) {
	CPUClass *cc = CPU_GET_CLASS(cpu);
	cc->set_pc(cpu,addr);
}

static inline void cpu_unligned_access(CPUState *cpu, vaddr addr, MMUAccessType access_type, int mmu_idx, uintptr_t retaddr) {
	CPUClass *cc = CPU_GET_CLASS(cpu);
	cc->do_unaligned_access(cpu,addr,access_type,mmu_idx,retaddr);
}

// Reset interrupts on the vCPU
void cpu_set_reset_interrupt(CPUState *cpu, int mask);
void cpu_exit(CPUState *cpu);
void cpu_resume(CPUState *cpu);
void cpu_remove(CPUState *cpu);
void cpu_remove_sync(CPUState *cpu);
void process_queued_cpu_work(CPUState *cpu);
void cpu_exec_start(CPUState *cpu);
void cpu_exec_end(CPUState *cpu);

/**
 * Wait for a concurrent exclusive section to end, and the start
 * a section of work that is run while other CPUs are not running
 * CPUs that call cpu_exec_start during the exclusive section go to
 * sleep until this CPU calls and exclusive
 */

void start_exclusive(void);
void end_exclusive(void);
void qemu_init_vcpu(CPUState *cpu);

#define SSTEP_ENABLE  0x1  /* Enable simulated HW single stepping */
#define SSTEP_NOIRQ   0x2  /* Do not use IRQ while single stepping */
#define SSTEP_NOTIMER 0x4 /* Do not Timers while single stepping */

void cpu_single_step(CPUState *cpu, int enabled);
#define BP_MEM_READ           0x01
#define BP_MEM_WRITE          0x02
#define BP_MEM_ACCESS         (BP_MEM_READ | BP_MEM_WRITE)
#define BP_STOP_BEFORE_ACCESS 0x04
/* 0x08 currently unused */
#define BP_GDB                0x10
#define BP_CPU                0x20
#define BP_ANY                (BP_GDB | BP_CPU)
#define BP_WATCHPOINT_HIT_READ 0x40
#define BP_WATCHPOINT_HIT_WRITE 0x80
#define BP_WATCHPOINT_HIT (BP_WATCHPOINT_HIT_READ | BP_WATCHPOINT_HIT_WRITE)

int cpu_breakpoint_insert(CPUState *cpu, vaddr pc, int flags,CPUBreakpoint **breakpoint);
int cpu_breakpoint_remove(CPUState *cpu, vaddr pc, int flags);
void cpu_breakpoint_remove_by_ref(CPUState *cpu, CPUBreakpoint *breakpoint);
void cpu_breakpoint_remove_all(CPUState *cpu, int mask);

static inline bool cpu_breakpoint_test(CPUState *cpu, vaddr pc, int mask) {
	CPUBreakpoint *bp;
	if (unlikely(!QTAILQ_EMPTY(&cpu->breakpoints))) {
		QTAILQ_FOREACH(bp, &cpu->breakpoints,entry)  {
			if (bp->pc == pc && (bp->flags & mask)) {
				return true;
			}
		}
	}
	return false;
}

int cpu_watchpoint_insert(CPUState *cpu, vaddr addr, vaddr len, int flags, CPUWatchpoint **watchpoint);
int cpu_watchpoint_remove(CPUState *cpu, vaddr addr, vaddr len, int flags);
void cpu_watchpoint_remove_by_ref(CPUState *cpu, CPUWatchpoint *watchpoint);
void cpu_watchpoint_remove_all(CPUState *cpu, int mask);

AddressSpace *cpu_get_address_space(CPUState *cpu, const char *fmt, ...) GCC_FMT_ATTR(2,3);
extern Property cpu_common_props[];
void cpu_exec_initfn(CPUState *cpu);
void cpu_exec_realizefn(CPUState *cpu, Error **errp);
void cpu_exec_unrealizefn(CPUState *cpu);

#ifdef NEED_CPU_H

#ifdef CONFIG_SOFTMMU
extern const struct VMStateDescription vmstate_cpu_common;
#else
#define vmstate_cpu_common vmstate_dummy
#endif

#define VMSTATE_CPU() {                                                     \
    .name = "parent_obj",                                                   \
    .size = sizeof(CPUState),                                               \
    .vmsd = &vmstate_cpu_common,                                            \
    .flags = VMS_STRUCT,                                                    \
    .offset = 0,                                                            \
}

#endif /* NEED_CPU_H */

#define UNASSIGNED_CPU_INDEX -1
#endif /* QOM_CPU_H_ */
