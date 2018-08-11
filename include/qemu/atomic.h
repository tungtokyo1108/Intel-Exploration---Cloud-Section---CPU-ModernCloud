/*
 * atomic.h
 *
 *  Created on: Aug 11, 2018
 *      Author: tungdang
 */

#ifndef QEMU_ATOMIC_H_
#define QEMU_ATOMIC_H_

#define barrier() ({asm volatile("" ::: "memory"); (void)0; })



#endif /* QEMU_ATOMIC_H_ */
