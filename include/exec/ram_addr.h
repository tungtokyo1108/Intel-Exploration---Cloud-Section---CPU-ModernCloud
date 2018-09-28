/*
 * ram_addr.h
 *
 *  Created on: Sep 28, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#ifndef EXEC_RAM_ADDR_H_
#define EXEC_RAM_ADDR_H_

#include "hw/xen/xen.h"
#include "exec/ramlist.h"

#define TARGET_PAGE_BITS 1

struct RAMBlock {
	struct rcu_head rcu;
	struct MemoryRegion *mr;
	uint8_t *host;
	ram_addr_t offset;
	ram_addr_t used_length;
	ram_addr_t max_length;
	void (*resized)(const char*, uint64_t length, void *host);
	uint32_t flags;
	char idstr[256];
	QLIST_ENTRY(RAMBlock) next;
	QLIST_HEAD(, RAMBlockNotifier) ramblock_notifier;
	int fd;
	size_t page_size;
	unsigned long *bmap;
	unsigned long *unsentmap;
	unsigned long *receivedmap;
};

static inline bool offset_in_ramblock(RAMBlock *b, ram_addr_t offset) {
	return (b && b->host && offset < b->used_length) ? true : false;
}

static inline void *ramblock_ptr(RAMBlock *block, ram_addr_t offset) {
	assert(offset_in_ramblock(block,offset));
	return (char *)block->host + offset;
}

static inline unsigned long int ramblock_recv_bitmap_offset(void *host_addr, RAMBlock *rb) {
	uint64_t host_addr_offset = (uint64_t)(uintptr_t)(host_addr - (void *)rb->host);
	return host_addr_offset >> TARGET_PAGE_BITS;
}

long qemu_getrampagesize(void);
RAMBlock *qemu_ram_alloc_from_file(ram_addr_t size, MemoryRegion *mr, bool share,
		                           const char *mem_path, Error **errp);
RAMBlock *qemu_ram_alloc_from_fd(ram_addr_t size, MemoryRegion *mr, bool share,
		                         int fd, Error **errp);
RAMBlock *qemu_ram_alloc_from_ptr(ram_addr_t size, void *host, MemoryRegion *mr,
		                          Error **errp);
RAMBlock *qemu_ram_alloc(ram_addr_t size, bool share, MemoryRegion *mr,
		                          Error **errp);
RAMBlock *qemu_ram_alloc_resizeable(ram_addr_t size, ram_addr_t max_size,
		                            void (*resized)(const char*, uint64_t length, void *host),
		                            MemoryRegion *mr, Error **errp);
void qemu_ram_free(RAMBlock *block);
int qemu_ram_resize(RAMBlock *block, ram_addr_t newsize, Error **errp);

#define DIRTY_CLIENTS_ALL ((1 << DIRTY_MEMORY_NUM) - 1)
#define DIRTY_CLIENTS_NOCODE (DIRTY_CLIENTS_ALL & ~(1 << DIRTY_MEMORY_CODE))

void tb_invalidate_phys_range(ram_addr_t start, ram_addr_t end);
static inline bool cpu_physical_memory_get_dirty(ram_addr_t start, ram_addr_t length, unsigned client) {
	DirtyMemoryBlocks *blocks;
	unsigned long end, page;
	unsigned long idx, offset, base;
	bool dirty = false;
	assert(client < DIRTY_MEMORY_NUM);
	end = TARGET_PAGE_ALIGN(start + length) >> TARGET_PAGE_BITS;
	page = start >> TARGET_PAGE_BITS;

	rcu_read_lock();

	blocks = atomic_rcu_read(&ram_list.dirty_memory[client]);
	idx = page/DIRTY_MEMORY_BLOCK_SIZE;
	offset = page % DIRTY_MEMORY_BLOCK_SIZE;
	base = page - offset;
	while (page < end)
	{
		unsigned long next = MIN(end, base + DIRTY_MEMORY_BLOCK_SIZE);
		unsigned long num = next - base;
		unsigned long found = find_next_bit(blocks->blocks[idx], num, offset);
		if (found < num) {
			dirty = true;
			break;
		}

		page = next;
		idx++;
		offset = 0;
		base += DIRTY_MEMORY_BLOCK_SIZE;
	}

	rcu_read_unlock();
	return dirty;
}

static inline bool cpu_physical_memory_all_dirty(ram_addr_t start, ram_addr_t length, unsigned client) {
	DirtyMemoryBlocks *blocks;
	unsigned long end, page;
	unsigned long idx, offset, base;
	bool dirty = true;

	assert(client < DIRTY_MEMORY_NUM);
	end = TARGET_PAGE_ALIGN(start + length) >> TARGET_PAGE_BITS;
	page = start >> TARGET_PAGE_BITS;

	rcu_read_lock();
	blocks = atomic_rcu_read(&ram_list.dirty_memory[client]);

	idx = page / DIRTY_MEMORY_BLOCK_SIZE;
	offset = page % DIRTY_MEMORY_BLOCK_SIZE;
	base = page - offset;

	while (page < end)
	{
		unsigned long next = MIN(end, base + DIRTY_MEMORY_BLOCK_SIZE);
		unsigned long num = next - base;
		unsigned long found = find_next_zero_bit(blocks->blocks[idx], num, offset);
		if (found < num)
		{
			dirty = false;
			break;
		}

		page = next;
		idx++;
		offset = 0;
		base += DIRTY_MEMORY_BLOCK_SIZE;
	}

	rcu_read_unlock();
	return dirty;
}

static inline bool cpu_physical_memory_get_dirty_flag(ram_addr_t addr, unsigned client) {
	return cpu_physical_memory_get_dirty(addr, 1, client);
}

static inline bool cpu_physical_memory_is_clean(ram_addr_t addr) {
	bool vga = cpu_physical_memory_get_dirty_flag(addr, DIRTY_MEMORY_VGA);
	bool code = cpu_physical_memory_get_dirty_flag(addr, DIRTY_MEMORY_CODE);
	bool migration = cpu_physical_memory_get_dirty_flag(addr, DIRTY_MEMORY_MIGRATION);

	return !(vga && code && migration);
}

static inline uint8_t cpu_physical_memory_range_includes_clean(ram_addr_t start, ram_addr_t length,
		                                                       uint8_t mask) {
	uint8_t ret = 0;
	if (mask & (1 << DIRTY_MEMORY_VGA) &&
	    !cpu_physical_memory_all_dirty(start,length,DIRTY_MEMORY_VGA)) {
		ret |= (1 << DIRTY_MEMORY_VGA);
	}
	if (mask & (1 << DIRTY_MEMORY_CODE) &&
		!cpu_physical_memory_all_dirty(start,length,DIRTY_MEMORY_CODE)) {
		ret |= (1 << DIRTY_MEMORY_CODE);
	}
	if (mask & (1 << DIRTY_MEMORY_MIGRATION) &&
		!cpu_physical_memory_all_dirty(start,length,DIRTY_MEMORY_MIGRATION)) {
	    ret |= (1 << DIRTY_MEMORY_MIGRATION);
	}
	return ret;
}

static inline void cpu_physical_memory_set_dirty_flag(ram_addr_t addr, unsigned client) {
	unsigned long page, idx, offset;
	DirtyMemoryBlocks *blocks;

	assert(client < DIRTY_MEMORY_NUM);

	page = addr >> TARGET_PAGE_BITS;
	idx = page / DIRTY_MEMORY_BLOCK_SIZE;
	offset = page % DIRTY_MEMORY_BLOCK_SIZE;

	rcu_read_lock();
	blocks = atomic_rcu_read(&ram_list.dirty_memory[client]);
	set_bit_atomic(offset, blocks->blocks[idx]);
	rcu_read_unlock();
}

static inline void cpu_physical_memory_set_dirty_range(ram_addr_t start, ram_addr_t length,
		                                               uint8_t mask) {
	DirtyMemoryBlocks *blocks[DIRTY_MEMORY_NUM];
	unsigned long end, page;
	unsigned long idx, offset, base;
	int i;

	if (!mask && !xen_enabled()) {
		return;
	}

	end = TARGET_PAGE_ALIGN(start + length) >> TARGET_PAGE_BITS;
	page = start >> TARGET_PAGE_BITS;

	rcu_read_lock();
	for (i=0; i < DIRTY_MEMORY_NUM; i++)
	{
		blocks[i] = atomic_rcu_read(&ram_list.dirty_memory[i]);
	}

	idx = page / DIRTY_MEMORY_BLOCK_SIZE;
	offset = page % DIRTY_MEMORY_BLOCK_SIZE;
	base = page - offset;
	while (page < end)
	{
		unsigned long next = MIN(end, base + DIRTY_MEMORY_BLOCK_SIZE);
		if (likely(mask & (1 << DIRTY_MEMORY_MIGRATION)))
		{
			bitmap_set_atomic(blocks[DIRTY_MEMORY_MIGRATION]->blocks[idx], offset, next - page);
		}
		if (unlikely(mask & (1 << DIRTY_MEMORY_VGA)))
		{
			bitmap_set_atomic(blocks[DIRTY_MEMORY_VGA]->blocks[idx],offset,next - page);
		}
		if (unlikely(mask & (1 << DIRTY_MEMORY_CODE)))
		{
			bitmap_set_atomic(blocks[DIRTY_MEMORY_CODE]->blocks[idx],offset,next - page);
		}

		page = next;
		idx++;
		offset =0 ;
		base += DIRTY_MEMORY_BLOCK_SIZE;
	}

	rcu_read_unlock();
	xen_hvm_modifier_memory(start,length);
}

#endif /* EXEC_RAM_ADDR_H_ */























































