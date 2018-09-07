/*
 * memory-internal.h
 *
 *  Created on: Sep 7, 2018
 *      Student (coder): Tung Dang
 */

#ifndef EXEC_MEMORY_INTERNAL_H_
#define EXEC_MEMORY_INTERNAL_H_

#include "exec/memory.h"

#ifndef CONFIG_USER_ONLY

static inline AddressSpaceDispatch *flatview_to_dispatch(FlatView *fv) {
	return fv->dispatch;
}

static inline AddressSpaceDispatch *address_space_to_dispatch(AddressSpace *as) {
	return flatview_to_dispatch(address_space_to_flatview(as));
}

FlatView *address_space_get_flatview(AddressSpace *as);
void flatview_unref(FlatView *view);
extern const MemoryRegionOps unassigned_mem_ops;
bool memory_region_access_valid(MemoryRegion *mr, hwaddr addr, unsigned size, bool is_write, MemTxAttrs attrs);
void flatview_add_to_dispatch(FlatView *fv, MemoryRegionSection *section);
AddressSpaceDispatch *address_space_dispatch_new(FlatView *fv);
void address_space_dispatch_compact(AddressSpaceDispatch *d);
void address_space_dispatch_free(AddressSpaceDispatch *d);

void mtree_print_dispatch(fprintf_function mon, void *f, struct AddressSpaceDispatch *d, MemoryRegion *root);
struct page_collection;
typedef struct vaddr vaddr;
typedef struct NotDirtyInfo NotDirtyInfo;
typedef struct NotDirtyInfo {
	CPUState *cpu;
	struct page_collection *pages;
	ram_addr_t ram_addr;
	vaddr mem_vaddr;
	unsigned size;
	bool active;
} NotDirtyInfo;

void memory_notdirty_write_prepare(NotDirtyInfo *ndi, CPUState *cpu, vaddr mem_vaddr,
		                           ram_addr_t ram_addr, unsigned size);
void memory_notdirty_write_complete(NotDirtyInfo *ndi);

#endif

#endif /* EXEC_MEMORY_INTERNAL_H_ */
