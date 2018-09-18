/*
 * hvf.h
 *
 *  Created on: Sep 17, 2018
 *      Student (coder): Tung Dang
 */

#ifndef SYSEMU_HVF_H_
#define SYSEMU_HVF_H_

#include "qemu-common.h"
#include "qemu/bitops.h"
#include "exec/memory.h"
#include "sysemu/accel.h"

extern int hvf_disabled;

#define HVF_SLOT_LOG (1 << 0)

typedef struct hvf_slot {
	uint64_t start;
	uint64_t size;
	uint8_t *mem;
	int slot_id;
	uint32_t flags;
	MemoryRegion *region;
} hvf_slot;

typedef struct hvf_vcpu_caps {
	uint64_t vmx_cap_pinbased;
	uint64_t vmx_cap_procbased;
	uint64_t vmx_cap_procbased2;
	uint64_t vmx_cap_entry;
	uint64_t vmx_cap_exit;
	uint64_t vmx_cap_preemption_timer;
} hvf_vcpu_caps;

typedef struct HVFState {
	AccelState parent;
	hvf_slot slots[32];
	int num_slots;
	hvf_vcpu_caps *hvf_caps;
} HVFState;

extern HVFState *hvf_state;
void hvf_set_phys_mem(MemoryRegionSection *, bool);
void hvf_handle_io(CPUArchState *, uint16_t, void *, int, int, int);
hvf_slot *hvf_find_overlap_slot(uint64_t, uint64_t);
void hvf_disable(int disable);
int hvf_sync_vcpus(void);

int hvf_sync_vcpus(void);

int hvf_init_vcpu(CPUState *);
int hvf_vcpu_exec(CPUState *);
int hvf_smp_cpu_exec(CPUState *);
void hvf_cpu_synchronize_state(CPUState *);
void hvf_cpu_synchronize_post_reset(CPUState *);
void hvf_cpu_synchronize_post_init(CPUState *);
void _hvf_cpu_synchronize_post_init(CPUState *, run_on_cpu_data);

void hvf_vcpu_destroy(CPUState *);
void hvf_raise_event(CPUState *);
void hvf_reset_vcpu(CPUState *);
void vmx_update_tpr(CPUState *);
void update_apic_tpr(CPUState *);
int hvf_put_registers(CPUState *);
void vmx_clear_int_window_exiting(CPUState *cpu);

#define TYPE_HVF_ACCEL ACCEL_CLASS_NAME("hvf")

#define HVF_STATE(obj) \
OBJECT_CHECK(HVFState, (obj), TYPE_HVF_ACCEL)

#endif /* SYSEMU_HVF_H_ */
