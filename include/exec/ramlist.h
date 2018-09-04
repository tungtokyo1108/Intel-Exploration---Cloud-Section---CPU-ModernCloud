/*
 * ramlist.h
 *
 *  Created on: Sep 4, 2018
 *      Student (coder): Tung Dang
 */

#ifndef EXEC_RAMLIST_H_
#define EXEC_RAMLIST_H_
#include "qemu/typedefs.h"
#include "qemu/osdep.h"

typedef struct RAMBlockNotifier RAMBlockNotifier;

#define DIRTY_MEMORY_VGA       0
#define DIRTY_MEMORY_CODE      1
#define DIRTY_MEMORY_MIGRATION 2
#define DIRTY_MEMORY_NUM       3

/**
 * The dirty memory bitmap is split into fixed-size blocks to allow growth under RCU
 * When adding new RAMBlocks requires the dirty memory to grow, a new DirtyMemoryBlocks array is allocated with pointer to existing blocks
 * Other threads can safety access existing blocks while dirty memory is being grown.
 *
 */

#define DIRTY_MEMORY_BLOCK_SIZE ((ram_addr_t)256 * 1024 * 8)

typedef struct DirtyMemoryBlocks DirtyMemoryBlocks;
typedef struct DirtyMemoryBlocks {
	struct rcu_head rcu;
	unsigned long *blocks[];
} DirtyMemoryBlocks;

typedef struct RAMList RAMList;
typedef struct RAMList {
	QemuMutex muxtex;
	RAMBlock *mru_block;
	QLIST_HEAD(, RAMBlock) blocks;
	DirtyMemoryBlocks *dirty_memory[DIRTY_MEMORY_NUM];
	uint32_t version;
	QLIST_HEAD(, RAMBlockNotifier) ramblock_notifiers;
} RAMList;

extern RAMList ram_list;
#define INTERNAL_RAMBLOCK_FOREACH(block) \
	QLIST_FOREACH_RCU(block,&ram_list.blocks,next)
#define RAMBLOCK_FOREACH(block) INTERNAL_RAMBLOCK_FOREACH(block)
void qemu_mutex_lock_ramlist(void);
void qemu_mutex_unlock_ramlist(void);

typedef struct RAMBlockNotifier {
	void (*ram_block_added)(RAMBlockNotifier *n, void *host, size_t size);
	void (*ram_block_removed)(RAMBlockNotifier *n, void *host, size_t size);
	QLIST_ENTRY(RAMBlockNotifier) next;
} RAMBlockNotifier;

void ram_block_notifier_add(RAMBlockNotifier *n);
void ram_block_notifier_remove(RAMBlockNotifier *n);
void ram_block_notify_add(void *host, size_t size);
void ram_block_notify_remove(void *host, size_t size);
void ram_block_dump(Monitor *mon);

#endif /* EXEC_RAMLIST_H_ */






























