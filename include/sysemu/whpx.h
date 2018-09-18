/*
 * whpx.h
 *
 *  Created on: Sep 17, 2018
 *      Student (coder): Tung Dang
 */

#ifndef SYSEMU_WHPX_H_
#define SYSEMU_WHPX_H_

#include "qemu-common.h"

// Window Hypervisor Platform accelerator support

int whpx_init_vcpu(CPUState *cpu);
int whpx_vcpu_exec(CPUState *cpu);
void whpx_destroy_vcpu(CPUState *cpu);
void whpx_vcpu_kick(CPUState *cpu);


void whpx_cpu_synchronize_state(CPUState *cpu);
void whpx_cpu_synchronize_post_reset(CPUState *cpu);
void whpx_cpu_synchronize_post_init(CPUState *cpu);
void whpx_cpu_synchronize_pre_loadvm(CPUState *cpu);

#ifdef CONFIG_WHPX

int whpx_enabled(void);

#else /* CONFIG_WHPX */

#define whpx_enabled() (0)

#endif

#endif /* SYSEMU_WHPX_H_ */
