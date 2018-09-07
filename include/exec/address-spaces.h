/*
 * address-spaces.h
 *
 *  Created on: Sep 4, 2018
 *      Student (coder): Tung Dang
 */

#ifndef EXEC_ADDRESS_SPACES_H_
#define EXEC_ADDRESS_SPACES_H_

#include "exec/memory.h"

#ifndef CONFIG_USER_ONLY

MemoryRegion *get_system_memory(void);
MemoryRegion *get_system_io(void);

extern AddressSpace address_space_memory;
extern AddressSpace address_space_io;

#endif

#endif /* EXEC_ADDRESS_SPACES_H_ */
