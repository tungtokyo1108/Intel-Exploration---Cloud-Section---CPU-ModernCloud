/*
 * hw_accel.h
 *
 *  Created on: Sep 17, 2018
 *      Student (coder): Tung Dang
 */

#ifndef SYSEMU_HW_ACCEL_H_
#define SYSEMU_HW_ACCEL_H_

#include "qom/cpu.h"
#include "sysemu/hax.h"
#include "sysemu/kvm.h"
#include "sysemu/whpx.h"

static inline void cpu_synchronsize_state(CPUState *cpu) {
	if (kvm_enabled()) {
		kvm_cpu_synchronize_state(cpu);
	}
	if (hax_enabled()) {
		hax_cpu_synchronize_state(cpu);
	}
	if (whpx_enabled()) {
		whpx_cpu_synchronize_state(cpu);
	}
}

static inline void cpu_synchronsize_post_state(CPUState *cpu) {
	if (kvm_enabled()) {
		kvm_cpu_synchronize_post_init(cpu);
	}
	if (hax_enabled()) {
		hax_cpu_synchronize_post_init(cpu);
	}
	if (whpx_enabled()) {
		whpx_cpu_synchronize_post_init(cpu);
	}
}

static inline void cpu_synchronsize_pre_loadvm(CPUState *cpu) {
	if (kvm_enabled()) {
		kvm_cpu_synchronize_pre_loadvm(cpu);
	}
	if (hax_enabled()) {
		hax_cpu_synchronize_pre_loadvm(cpu);
	}
	if (whpx_enabled()) {
		whpx_cpu_synchronize_pre_loadvm(cpu);
	}
}

#endif /* SYSEMU_HW_ACCEL_H_ */
