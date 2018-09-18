/*
 * dma.h
 *
 *  Created on: Sep 17, 2018
 *      Student (coder): Tung Dang
 */

#ifndef SYSEMU_DMA_H_
#define SYSEMU_DMA_H_

#include "exec/memory.h"
#include "exec/address-spaces.h"
#include "hw/hw.h"
#include "block/block.h"
#include "block/accounting.h"

typedef struct ScatterGatherEntry ScatterGatherEntry;
typedef enum {
	DMA_DIRECTION_TO_DEVICE = 0,
	DMA_DIRECTION_FROM_DEVICE = 1,
} DMADirection;

typedef struct QEMUSGList QEMUSGList;
typedef struct QEMUSGList {
	ScatterGatherEntry *sg;
	int nsg;
	int nalloc;
	size_t size;
	DeviceState *dev;
	AddressSpace *as;
} QEMUSGList;

/**
 * An IOMMU is present, bus address become district from CPU/Memory physical addresses
 * and may be a different size.
 * Because the IOVA size depends more on the bus than on the platform,
 * we have to treat these as 64-bit always to cover all cases.
 */

#ifndef CONFIG_USER_ONLY

typedef uint64_t dma_addr_t;
#define DMA_ADDR_BITS 64
#define DMA_ADDR_FMT "%" PRIx64

static inline void dma_barrier(AddressSpace *as, DMADirection dir) {
	smp_mb();
}

// Check that given range of addresses is valid for DMA
static inline bool dma_memory_valid(AddressSpace *as, dma_addr_t addr, dma_addr_t len, DMADirection dir) {
	return address_space_access_valid(as, addr, len, dir == DMA_DIRECTION_FROM_DEVICE, MEMTXATTRS_UNSPECIFIED);
}

static inline int dma_memory_rw_relaxed(AddressSpace *as, dma_addr_t addr, void *buf, dma_addr_t len, DMADirection dir) {
	return (bool)address_space_rw(as, addr, MEMTXATTRS_UNSPECIFIED, buf, len, dir == DMA_DIRECTION_FROM_DEVICE);
}

static inline int dma_memory_read_relaxed(AddressSpace *as, dma_addr_t addr, void *buf, dma_addr_t len) {
	return dma_memory_rw_relaxed(as, addr, buf, len, DMA_DIRECTION_TO_DEVICE);
}

static inline int dma_memory_write_relaxed(AddressSpace *as, dma_addr_t addr, const void *buf, dma_addr_t len) {
	return dma_memory_rw_relaxed(as, addr, (void *)buf, len, DMA_DIRECTION_TO_DEVICE);
}

static inline int dma_memory_rw(AddressSpace *as, dma_addr_t addr, void *buf, dma_addr_t len, DMADirection dir) {
	dma_barrier(as,dir);
	return dma_memory_rw_relaxed(as, addr, buf, len, dir);
}

static inline int dma_memory_read(AddressSpace *as, dma_addr_t addr, void *buf, dma_addr_t len) {
	return dma_memory_rw(as, addr, buf, len, DMA_DIRECTION_TO_DEVICE);
}

static inline int dma_memory_write(AddressSpace *as, dma_addr_t addr, const void *buf, dma_addr_t len) {
	return dma_memory_rw(as, addr, (void *)buf, len, DMA_DIRECTION_TO_DEVICE);
}

int dma_memory_set(AddressSpace *as, dma_addr_t addr, uint8_t c, dma_addr_t len);
static inline void *dma_memory_map(AddressSpace *as, dma_addr_t addr, dma_addr_t len, DMADirection dir) {
	hwaddr xlen = *len;
	void *p;
	p = address_space_map(as, addr, &xlen, dir == DMA_DIRECTION_FROM_DEVICE, MEMTXATTRS_UNSPECIFIED);
	*len = xlen;
	return p;
}

static inline void dma_memory_unmap(AddressSpace *as, void *buffer, dma_addr_t len, DMADirection dir, dma_addr_t access_len) {
	address_space_unmap(as, buffer, (hwaddr)len, dir == DMA_DIRECTION_FROM_DEVICE, access_len);
}

#define DEFINE_LDST_DMA(_lname, _sname, _bits, _end) \
	static inline uint##_bits##_t ld##_lname##_##_end##_dma(AddressSpace *as, dma_addr_t addr) \
	{                                                                                           \
	uint##_bits##_t val;                                                                        \
	dma_memory_read(as, addr, &val, (_bits) / 8);                                               \
	}                                                                                           \
	static inline void st##_sname##_##_end##_dma(AddressSpace *as, dma_addr_t addr,             \
			                                     uint##_bits##_t val)                           \
    {                                                                                           \
		val = cpu_to_##_end##_bits(val);                                                        \
		dma_memory_write(as, addr, &val, (_bits)/8)                                             \
    }

static inline uint8_t ldub_dma(AddressSpace *as, dma_addr_t addr) {
	uint8_t val;
	dma_memory_read(as, addr, &val, 1);
	return val;
}

static inline void stb_dma(AddressSpace *as, dma_addr_t addr, uint8_t val) {
	dma_memory_write(as, addr, &val, 1);
}

DEFINE_LDST_DMA(uw, w, 16, le);
DEFINE_LDST_DMA(l, l, 32, le);
DEFINE_LDST_DMA(q, q, 64, le);
DEFINE_LDST_DMA(uw, w, 16, be);
DEFINE_LDST_DMA(l, l, 32, be);
DEFINE_LDST_DMA(q, q, 64, be);

#undef DEFINE_LDST_DMA

typedef struct ScatterGatherEntry {
	dma_addr_t base;
	dma_addr_t len;
} ScatterGatherEntry;

void qemu_sglist_init(QEMUSGList *qsg, DeviceState *dev, int alloc_hint, AddressSpace *as);
void qemu_sglist_add(QEMUSGList *qsg, dma_addr_t base, dma_addr_t len);
void qemu_sglist_destroy(QEMUSGList *qsg);

#endif

typedef BlockAIOCB *DMAIOFunc(int64_t offset, QEMUIOVector *iov,
                              BlockCompletionFunc *cb, void *cb_opaque,
                              void *opaque);

BlockAIOCB *dma_blk_io(AioContext *ctx,
                       QEMUSGList *sg, uint64_t offset, uint32_t align,
                       DMAIOFunc *io_func, void *io_func_opaque,
                       BlockCompletionFunc *cb, void *opaque, DMADirection dir);
BlockAIOCB *dma_blk_read(BlockBackend *blk,
                         QEMUSGList *sg, uint64_t offset, uint32_t align,
                         BlockCompletionFunc *cb, void *opaque);
BlockAIOCB *dma_blk_write(BlockBackend *blk,
                          QEMUSGList *sg, uint64_t offset, uint32_t align,
                          BlockCompletionFunc *cb, void *opaque);
uint64_t dma_buf_read(uint8_t *ptr, int32_t len, QEMUSGList *sg);
uint64_t dma_buf_write(uint8_t *ptr, int32_t len, QEMUSGList *sg);

void dma_acct_start(BlockBackend *blk, BlockAcctCookie *cookie, QEMUSGList *sg, enum BlockAcctType type);

#endif /* SYSEMU_DMA_H_ */
