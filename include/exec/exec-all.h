/*
 * exec-all.h
 *
 *  Created on: Sep 16, 2018
 *      Student (coder): Tung Dang
 */

#ifndef EXEC_EXEC_ALL_H_
#define EXEC_EXEC_ALL_H_

#include "qemu-common.h"
#include "exec/tb-context.h"
#include "exec/ramlist.h"
#include "exec/memory.h"
#include "qom/cpu.h"
#include "sysemu/cpus.h"

#define DEBUG_DISAS

/**
 * Page tracking code used ram addresses in system mode, and virtual addresses in userspace mode
 */
#if defined(CONFIG_USER_ONLY)
typedef abi_ulong tb_page_addr_t;
#define TB_PAGE_ADDR_FMT TARGET_ABI_FMT_lx
#else
typedef ram_addr_t tb_page_addr_t;
#define TB_PAGE_ADDR_FMT RAM_ADDR_FMT
#endif

#include "qemu/log.h"

typedef struct CPUArchState CPUArchState;
typedef struct target_ulong target_ulong;
void gen_intermediate_code(CPUState *cpu, struct TranslationBlock *tb);
void restore_stable_to_opc(CPUArchState *env, struct TranslationBlock *tb, target_ulong *data);
void cpu_gen_init(void);

/**
 * Attempt to restore the state for a fault occurring in translated code.
 */

bool cpu_restore_state(CPUState *cpu, uintptr_t searched_pc, bool will_exit);
void QEMU_NORETURN cpu_loop_exit_noexc(CPUState *cpu);
void QEMU_NORETURN cpu_io_recompile(CPUState *cpu, uintptr_t retaddr);
TranslationBlock *tb_gen_code(CPUState *cpu,
                              target_ulong pc, target_ulong cs_base,
                              uint32_t flags, int cflags);
void QEMU_NORETURN cpu_loop_exit(CPUState *cpu);
void QEMU_NORETURN cpu_loop_exit_restore(CPUState *cpu, uintptr_t pc);
void QEMU_NORETURN cpu_loop_exit_atomic(CPUState *cpu, uintptr_t pc);

#if !defined(CONFIG_USER_ONLY)
void cpu_reloading_memory_map(void);

/**
 * Add the specific address space to the CPU's cpu_address list.
 * The address space added with @asidx 0 is the one used for the convenience pointer cpu->as
 * The target-specific code which register ASes is responsible for defining what semantics address space
 */

void cpu_address_space_init(CPUState *cpu, int asidx, const char *prefix, MemoryRegion *mr);
#endif

#if !defined(CONFIG_USER_ONLY) && defined(CONFIG_TCG)
/**
 * Flush one page from the TLB of the specific CPU for all MMU indexes
 */

void tlb_flush_page(CPUState *cpu, target_ulong addr);
void tlb_flush_page_all_cpus(CPUState *src, target_ulong addr);
void tlb_flush_page_all_cpus_synced(CPUState *src, target_ulong addr);

/**
 * Flush the entire TLB for specified CPU.
 * Most CPU architectures allow the implementation to drop entries for the TLB at any time
 * so this is generally safe
 */

void tlb_flush(CPUState *cpu);
void tlb_flush_all_cpus(CPUState *src_cpu);
void tlb_flush_all_cpus_synced(CPUState *src_cpu);

void tlb_flush_page_by_mmuidx(CPUState *cpu, target_ulong addr, uint16_t idxmap);
void tlb_flush_page_by_mmuidx_all_cpus(CPUState *cpu, target_ulong addr, uint16_t idxmap);
void tlb_flush_page_by_mmuidx_all_cpus_synced(CPUState *cpu, target_ulong addr, uint16_t idxmap);

void tlb_flush_by_mmuidx(CPUState *cpu, uint16_t idxmap);
void tlb_flush_by_mmuidx_all_cpus(CPUState *cpu, uint16_t idxmap);
void tlb_flush_by_mmuidx_all_cpus_synced(CPUState *cpu, uint16_t idxmap);

/**
 * Add an entry to this CPU's TLB with the specific memory transaction attributes.
 * This is called by the target CPU specific code after it has been called entry point
 * and performed a successful page table walk to find the physical address
 * and attributes for the virtual address which provoked the TLB miss
 */

void tlb_set_page_with_attrs(CPUState *cpu, target_ulong vaddr, hwaddr paddr, MemTxAttrs atttrs,
		                     int prot, int mmu_idx, target_ulong size);

void tlb_set_page(CPUState *cpu, target_ulong vaddr, hwaddr paddr, int prot,
		          int mmu_idx, target_ulong size);
#else
static inline void tlb_flush_page(CPUState *cpu, target_ulong addr)
{
}
static inline void tlb_flush_page_all_cpus(CPUState *src, target_ulong addr)
{
}
static inline void tlb_flush_page_all_cpus_synced(CPUState *src,
                                                  target_ulong addr)
{
}
static inline void tlb_flush(CPUState *cpu)
{
}
static inline void tlb_flush_all_cpus(CPUState *src_cpu)
{
}
static inline void tlb_flush_all_cpus_synced(CPUState *src_cpu)
{
}
static inline void tlb_flush_page_by_mmuidx(CPUState *cpu,
                                            target_ulong addr, uint16_t idxmap)
{
}

static inline void tlb_flush_by_mmuidx(CPUState *cpu, uint16_t idxmap)
{
}
static inline void tlb_flush_page_by_mmuidx_all_cpus(CPUState *cpu,
                                                     target_ulong addr,
                                                     uint16_t idxmap)
{
}
static inline void tlb_flush_page_by_mmuidx_all_cpus_synced(CPUState *cpu,
                                                            target_ulong addr,
                                                            uint16_t idxmap)
{
}
static inline void tlb_flush_by_mmuidx_all_cpus(CPUState *cpu, uint16_t idxmap)
{
}

static inline void tlb_flush_by_mmuidx_all_cpus_synced(CPUState *cpu,
                                                       uint16_t idxmap)
{
}
#endif

#define CODE_GEN_ALIGN 16
#if defined(CONFIG_SOFTMMU)
#define CODE_GEN_AVG_BLOCK_SIZE 400
#else
#define CODE_GEN_AVG_BLOCK_SIZE 150
#endif

// Translation Cache-related field of a TB

struct tb_tc {
	void *ptr;
	size_t size;
};

struct TranslationBlock {
	target_ulong pc;  // simulate PC corresponding to this block (EIP + CS base)
	target_ulong cs_base; // CS base for this block
	uint32_t flags;
	uint16_t size;
	uint16_t icount;
	uint32_t cflags;
#define CF_COUNT_MASK  0x00007fff
#define CF_LAST_IO     0x00008000 /* Last insn may be an IO access.  */
#define CF_NOCACHE     0x00010000 /* To be freed after execution */
#define CF_USE_ICOUNT  0x00020000
#define CF_INVALID     0x00040000 /* TB is stale. Set with @jmp_lock held */
#define CF_PARALLEL 0x00080000 /* Generate code for a parallel context */

#define CF_HASH_MASK \
		(CF_COUNT_MASK | CF_LAST_IO | CF_USE_ICOUNT | CF_PARALLEL)

uint32_t trace_vcpu_dstate; // Per-vCPU dynamic tracing state used to generate this TB
struct tb_tc tc;
struct TranslationBlock *orig_tb;
uintptr_t page_next[2];
tb_page_addr_t page_addr[2];
QemuSpin jmp_lock;

/**
 * The following data are used to directly call another TB from the code of this one.
 * Done by emitting direct or indirect native jump instructions.
 * The TB can be linked to another one by setting one of the jump targets
 */
uint16_t jmp_reset_offset[2];
#define TB_JMP_RESET_OFFSET_INVALID 0xffff
uintptr_t jmp_target_arg[2];

uintptr_t jmp_list_head;
uintptr_t jmp_list_next[2];
uintptr_t jmp_dest[2];

};

extern bool parallel_cpus;
static inline uint32_t tb_cflags(const TranslationBlock *tb) {
	return atomic_read(&tb->cflags);
}

static inline uint32_t curr_cflags(void)
{
    return (parallel_cpus ? CF_PARALLEL : 0)
         | (use_icount ? CF_USE_ICOUNT : 0);
}

#if defined(CONFIG_USER_ONLY)
void tb_invalidate_phys_addr(target_ulong addr);
void tb_invalidate_phys_range(target_ulong start, target_ulong end);
#else
void tb_invalidate_phys_addr(AddressSpace *as, hwaddr addr, MemTxAttrs attrs);
#endif
void tb_flush(CPUState *cpu);
void tb_phys_invalidate(TranslationBlock *tb, tb_page_addr_t page_addr);
TranslationBlock *tb_htable_lookup(CPUState *cpu, target_ulong pc,
                                   target_ulong cs_base, uint32_t flags, uint32_t cf_mask);
void tb_set_jmp_target(TranslationBlock *tb, int n, uintptr_t addr);
#if defined(CONFIG_TCG_INTERPRETER)
extern uintptr_t tci_tb_ptr;
# define GETPC() tci_tb_ptr
#else
# define GETPC() \
    ((uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)))
#endif

#define GETPC_ADJ   2

#if !defined(CONFIG_USER_ONLY) && defined(CONFIG_DEBUG_TCG)
void assert_no_pages_locked(void);
#else
static inline void assert_no_pages_locked(void)
{
}
#endif

#if !defined(CONFIG_USER_ONLY)
struct MemoryRegionSection *iotlb_to_section(CPUState *cpu,
                                             hwaddr index, MemTxAttrs attrs);

void tlb_fill(CPUState *cpu, target_ulong addr, int size,
              MMUAccessType access_type, int mmu_idx, uintptr_t retaddr);

#endif

#if defined(CONFIG_USER_ONLY)
void mmap_lock(void);
void mmap_unlock(void);
bool have_mmap_lock(void);

static inline tb_page_addr_t get_page_addr_code(CPUArchState *env1, target_ulong addr)
{
    return addr;
}
#else
static inline void mmap_lock(void) {}
static inline void mmap_unlock(void) {}

tb_page_addr_t get_page_addr_code (CPUArchState *env1, target_ulong addr);
void tlb_reset_dirty(CPUState *cpu, ram_addr_t start1, ram_addr_t length);
void tlb_set_dirty(CPUState *cpu, target_ulong vaddr);
void tb_flush_jmp_cache(CPUState *cpu, target_ulong addr);

MemoryRegionSection *
address_space_translate_for_iotlb(CPUState *cpu, int asidx, hwaddr addr,
                                  hwaddr *xlat, hwaddr *plen, MemTxAttrs attrs, int *prot);
hwaddr memory_region_section_get_iotlb(CPUState *cpu,
                                       MemoryRegionSection *section,
                                       target_ulong vaddr,
                                       hwaddr paddr, hwaddr xlat,
                                       int prot, target_ulong *address);
bool memory_region_is_unassigned(MemoryRegion *mr);
#endif
extern int singlestep;

#endif /* EXEC_EXEC_ALL_H_ */
