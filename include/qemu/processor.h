/*
 * processor.h
 *
 *  Created on: Aug 21, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_PROCESSOR_H_
#define QEMU_PROCESSOR_H_

#include "qemu/atomic.h"

#if defined(__i386__) || defined(__x86_64__)
#define cpu_relax() asm volatile("rep; nop" ::: "memory")
#elif defined(__aarch64__)
#define cpu_relax() asm volatile("yield" ::: "memory")
#else
#define cpu_relax() barrier()
#endif


#endif /* QEMU_PROCESSOR_H_ */
