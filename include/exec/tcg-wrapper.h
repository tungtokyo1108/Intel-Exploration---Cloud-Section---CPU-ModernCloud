/*
 * tcg-wrapper.h
 *
 *  Created on: Sep 19, 2018
 *      Student (coder): Tung Dang
 */

#ifndef EXEC_TCG_WRAPPER_H_
#define EXEC_TCG_WRAPPER_H_

#ifdef CONFIG_TCG

#include "tcg.h"
#include "translate-all.h"

#else

#include "exec/exec-all.h"

#ifdef CONFIG_USER_ONLY
int page_unprotect(target_ulong address, uintptr_t pc);
#endif

/* Default target word size to pointer size.  */
#ifndef TCG_TARGET_REG_BITS
# if UINTPTR_MAX == UINT32_MAX
#  define TCG_TARGET_REG_BITS 32
# elif UINTPTR_MAX == UINT64_MAX
#  define TCG_TARGET_REG_BITS 64
# else
#  error Unknown pointer size for tcg target
# endif
#endif

#if TARGET_LONG_BITS > TCG_TARGET_REG_BITS
#define TCG_OVERSIZED_GUEST 1
#else
#define TCG_OVERSIZED_GUEST 0
#endif

struct page_collection {
};

inline struct page_collection * page_collection_lock(tb_page_addr_t start, tb_page_addr_t end)
{
    return NULL;
}

inline void page_collection_unlock(struct page_collection *set)
{ }

static inline void tb_invalidate_phys_page_fast(struct page_collection *pages,
                                                tb_page_addr_t start, int len)
{
}


static inline void tb_invalidate_phys_page_range(tb_page_addr_t start, tb_page_addr_t end,
                                                 int is_cpu_write_access)
{
}

static inline void tb_check_watchpoint(CPUState *cpu)
{
}

static inline void flush_icache_range(uintptr_t start, uintptr_t stop)
{
}

inline void tb_flush(CPUState *cpu)
{
}

inline void tlb_set_dirty(CPUState *cpu, target_ulong vaddr)
{
}

static inline void tb_lock(void)
{
}

static inline void tb_unlock(void)
{
}

inline void tcg_register_thread(void)
{
}

inline void tcg_region_init(void)
{
}

#endif

#endif /* EXEC_TCG_WRAPPER_H_ */
