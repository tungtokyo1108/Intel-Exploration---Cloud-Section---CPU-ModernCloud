/*
 * hax.h
 *
 *  Created on: Sep 17, 2018
 *      Student (coder): Tung Dang
 */

#ifndef SYSEMU_HAX_H_
#define SYSEMU_HAX_H_

#include "qemu-common.h"
#include "hw/hw.h"
#include "qemu/bitops.h"
#include "exec/memory.h"

int hax_sync_vcpus(void);
int hax_init_vcpu(CPUState *cpu);
int hax_smp_cpu_exec(CPUState *cpu);
int hax_populate_ram(uint64_t va, uint64_t size);

void hax_cpu_synchronize_state(CPUState *cpu);
void hax_cpu_synchronize_post_reset(CPUState *cpu);
void hax_cpu_synchronize_post_init(CPUState *cpu);
void hax_cpu_synchronize_pre_loadvm(CPUState *cpu);

#ifdef CONFIG_HAX

int hax_enabled(void);

int hax_vcpu_destroy(CPUState *cpu);
void hax_raise_event(CPUState *cpu);
void hax_reset_vcpu_state(void *opaque);

#else /* CONFIG_HAX */

#define hax_enabled() (0)

#endif

#endif /* SYSEMU_HAX_H_ */
