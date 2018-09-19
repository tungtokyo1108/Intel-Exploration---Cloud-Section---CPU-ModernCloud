/*
 * boards.h
 *
 *  Created on: Sep 19, 2018
 *      Student (coder): Tung Dang
 */

#ifndef HW_BOARDS_H_
#define HW_BOARDS_H_

#include "sysemu/blockdev.h"
#include "sysemu/accel.h"
#include "hw/fw-build.h"
#include "hw/qdev.h"
#include "qom/object.h"
#include "qom/cpu.h"
#include "exec/memory.h"
#include "qemu/typedefs.h"
/**
 * Allocate a broad's main memory and initializes Memory Region appropriately.
 * Arrange for the memory to be migrated.
 *
 * Memory allocated via this function will be backed with the memory backend
 * the user provided using "-mem-path"
 *
 * For boards where the major RAM is split into two parts in the memory map,
 * Deal with this by calling memory_region_allocate_system_memory()
 * once to get a MemoryRegion with enough RAM for both parts,
 * creating alias MemoryRegions via memory_region_init_alias() which
 * alias into different parts of this RAM MemoryRegion and can be mapped
 * into the memory map in the appropriate places
 */

void memory_region_allocate_system_memory(MemoryRegion *mr, Object *owner, const char *name, uint64_t ram_size);
#define TYPE_MACHINE_SUFFIX "-machine"

#define MACHINE_TYPE_NAME(machinename) (machinename TYPE_MACHINE_SUFFIX)
#define TYPE_MACHINE "machine"
#undef MACHINE
#define MACHINE(obj) \
	OBJECT_CHECK(MachineState, (obj), TYPE_MACHINE)

#define MACHINE_GET_CLASS(obj) \
	OBJECT_GET_CLASS(MachineClass, (obj), TYPE_MACHINE)

#define MACHINE_CLASS(klass) \
	OBJECT_CLASS_CHECK(MachineClass, (klass), TYPE_MACHINE)

MachineClass *find_default_machine(void);
extern MachineState *current_machine;



#endif /* HW_BOARDS_H_ */
