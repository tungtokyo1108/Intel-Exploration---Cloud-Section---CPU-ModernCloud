/*
 * mmio_interface.h
 *
 *  Created on: Sep 28, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#ifndef HW_MISC_MMIO_INTERFACE_H_
#define HW_MISC_MMIO_INTERFACE_H_

#include "exec/memory.h"

#define TYPE_MMIO_INTERFACE "mmio_interface"
#define MMIO_INTERFACE(obj) OBJECT_CHECK(MMIOInterface, (obj), TYPE_MMIO_INTERFACE)

typedef struct MMIOInterface {
	DeviceState parent_obj;
	MemoryRegion *subregion;
	MemoryRegion ram_mem;
	uint64_t start;
	uint64_t end;
	bool ro;
	uint64_t id;
	void *host_ptr;
} MMIOInterface;

void mmio_interface_map(MMIOInterface *s);
void mmio_interface_unmap(MMIOInterface *s);

#endif /* HW_MISC_MMIO_INTERFACE_H_ */
