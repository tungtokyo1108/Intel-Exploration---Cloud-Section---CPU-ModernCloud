/*
 * sys_membarrier.h
 *
 *  Created on: Sep 3, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_SYS_MEMBARRIER_H_
#define QEMU_SYS_MEMBARRIER_H_

#ifdef CONFIG_MEMBARRIER
/* Only block reordering at the compiler level in the performance-critical
 * side.  The slow side forces processor-level ordering on all other cores
 * through a system call.
 */
extern void smp_mb_global_init(void);
extern void smp_mb_global(void);
#define smp_mb_placeholder()       barrier()
#else
/* Keep it simple, execute a real memory barrier on both sides.  */
static inline void smp_mb_global_init(void) {}
#define smp_mb_global()            smp_mb()
#define smp_mb_placeholder()       smp_mb()
#endif

#endif /* QEMU_SYS_MEMBARRIER_H_ */
