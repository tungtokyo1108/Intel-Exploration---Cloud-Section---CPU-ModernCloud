/*
 * kvm.h
 *
 *  Created on: Sep 17, 2018
 *      Student (coder): Tung Dang
 */

#ifndef SYSEMU_KVM_H_
#define SYSEMU_KVM_H_

#include "qemu/queue.h"
#include "qom/cpu.h"
#include "exec/memattrs.h"
#include "hw/irq.h"

#ifdef NEED_CPU_H
# ifdef CONFIG_KVM
#  include <linux/kvm.h>
#  define CONFIG_KVM_IS_POSSIBLE
# endif
#else
# define CONFIG_KVM_IS_POSSIBLE
#endif

#ifdef CONFIG_KVM_IS_POSSIBLE

extern bool kvm_allowed;
extern bool kvm_kernel_irqchip;
extern bool kvm_split_irqchip;
extern bool kvm_async_interrupts_allowed;
extern bool kvm_halt_in_kernel_allowed;
extern bool kvm_eventfds_allowed;
extern bool kvm_irqfds_allowed;
extern bool kvm_resamplefds_allowed;
extern bool kvm_msi_via_irqfd_allowed;
extern bool kvm_gsi_routing_allowed;
extern bool kvm_gsi_direct_mapping;
extern bool kvm_readonly_mem_allowed;
extern bool kvm_direct_msi_allowed;
extern bool kvm_ioeventfd_any_length_allowed;
extern bool kvm_msi_use_devid;

#define kvm_enabled() (kvm_allowed)
#define kvm_irqchip_in_kernel() (kvm_kernel_irqchip)

/**
 * Asked to split the irqchip implementation between user and kernel space.
 */

#define kvm_irqchip_is_split() (kvm_split_irqchip)

/**
 * Deliver interrupts to KVM asynchronously rather than having to do interrupt delivery synchronously
 */

#define kvm_async_interrupts_enabled() (kvm_async_interrupts_allowed)

/**
 * Halted cpus should still get a KVM_RUN ioctl to run inside of kernel space
 */

#define kvm_halt_in_kernel() (kvm_halt_in_kernel_allowed)

/**
 * Use eventfds to receive notifications from a KVM CPU
 */

#define kvm_eventfds_enabled() (kvm_eventfds_allowed)

// Use irqfds to inject interrupts into a KVM CPU
#define kvm_irqfds_enabled() (kvm_irqfds_allowed)
#define kvm_resamplefds_enabled() (kvm_resamplefds_allowed)

// Route a PCI MSI (Message Signaled Interrupt) to a KVM CPU via a irqfd

#define kvm_msi_via_irqfd_enabled() (kvm_msi_via_irqfd_allowed)
#define kvm_gsi_routing_enabled() (kvm_gsi_routing_allowed)
#define kvm_gsi_direct_mapping() (kvm_gsi_direct_mapping)

// KVM readonly memory is enabled
#define kvm_readonly_mem_enabled() (kvm_readonly_mem_allowed)
#define kvm_direct_msi_enabled() (kvm_direct_msi_allowed)
#define kvm_ioeventfd_any_length_enabled() (kvm_ioeventfd_any_length_allowed)
#define kvm_msi_devid_required() (kvm_msi_use_devid)

#else
#define kvm_enabled()           (0)
#define kvm_irqchip_in_kernel() (false)
#define kvm_irqchip_is_split() (false)
#define kvm_async_interrupts_enabled() (false)
#define kvm_halt_in_kernel() (false)
#define kvm_eventfds_enabled() (false)
#define kvm_irqfds_enabled() (false)
#define kvm_resamplefds_enabled() (false)
#define kvm_msi_via_irqfd_enabled() (false)
#define kvm_gsi_routing_allowed() (false)
#define kvm_gsi_direct_mapping() (false)
#define kvm_readonly_mem_enabled() (false)
#define kvm_direct_msi_enabled() (false)
#define kvm_ioeventfd_any_length_enabled() (false)
#define kvm_msi_devid_required() (false)
#endif

struct kvm_run;
struct kvm_lapic_state;
struct kvm_irq_routing_entry;

typedef struct KVMCapabilityInfo {
	const char *name;
	int value;
} KVMCapabilityInfo;

#define KVM_CAP_INFO(CAP) {"KVM_CAP_" stringify(CAP), KVM_CAP_##CAP}
#define KVM_CAP_LAST_INFO {NULL, 0}

struct KVMState;
typedef struct KVMState KVMState;
extern KVMState *kvm_state;

bool kvm_has_free_slot(MachineState *ms);
bool kvm_has_sync_mmu(void);
int kvm_has_vcpu_events(void);
int kvm_has_robust_singlestep(void);
int kvm_has_debugregs(void);
int kvm_has_pit_state2(void);
int kvm_has_many_ioeventfds(void);
int kvm_has_gsi_routing(void);
int kvm_has_intx_set_mask(void);
int kvm_init_vcpu(CPUState *cpu);
int kvm_cpu_exec(CPUState *cpu);
int kvm_destroy_vcpu(CPUState *cpu);

// KVM supports using kernel generated IRQs from user space
bool kvm_arm_supports_user_irq(void);

// Whether memory encryption is enabled
bool kvm_memcrypt_enabled(void);

// Encrypt the memory range
int kvm_memcrypt_encrypt_data(uint8_t *ptr, uint64_t len);


/////////////////////////////////////////////////////////////

#ifdef NEED_CPU_H

void kvm_flush_coalesced_mmio_buffer(void);
int kvm_insert_breakpoint(CPUState *cpu, target_ulong addr, target_ulong len, int type);
int kvm_remove_breakpoint(CPUState *cpu, target_ulong addr, target_ulong len, int type);
void kvm_remove_all_breakpoints(CPUState *cpu);
int kvm_update_guest_debug(CPUState *cpu, int code, void *addr);
int kvm_on_sigbus_vcpu(CPUState *cpu, int code, void *addr);
int kvm_on_sigbus(int code, void *addr);

void phys_mem_set_alloc(void *(*alloc)(size_t, uint64_t *align, bool shared));

int kvm_ioctl(KVMState *s, int type, ...);
int kvm_vm_ioctl(KVMState *s, int type, ...);
int kvm_vcpu_iolctl(CPUState *cpu, int type, ...);

// Call an ioctl on a kvm device
int kvm_device_ioctl(int fd, int type, ...);

// Check for existence of a specific vm/device attribute
int kvm_vm_check_attr(KVMState *s, uint32_t group, uint64_t attr);
int kvm_device_check_attr(KVMState *s, uint32_t group, uint64_t attr);

// set or get value of a specific vm attribute
int kvm_device_access(int fd, int group, uint64_t attr, void *val, bool write, Error **errp);

// Create a KVM device for the device control API
int kvm_create_device(KVMState *s, uint64_t type, bool test);
bool kvm_device_spported(int vmfd, uint64_t type);
extern const KVMCapabilityInfo kvm_arch_required_capabilities[];
void kvm_arch_pre_run(CPUState *cpu, struct kvm_run *run);
MemTxAttrs kvm_arch_post_run(CPUState *cpu, struct kvm_run *run);
int kvm_arch_handle_exit(CPUState *cpu, struct kvm_run *run);
int kvm_arch_process_async_events(CPUState *cpu);
int kvm_arch_get_registers(CPUState *cpu);

#define KVM_PUT_RUNTIME_STATE 1
#define KVM_PUT_RESET_STATE   2
#define KVM_PUT_FULL_STATE    3

int kvm_arch_put_registers(CPUState *cpu, int level);
int kvm_arch_init(MachineState *ms, KVMState *s);
int kvm_arch_init_vcpu(CPUState *cpu);
bool kvm_vcpu_id_is_valid(int vcpu_id);

unsigned long kvm_arch_vcpu_id(CPUState *cpu);
#ifdef TARGET_I386
#define KVM_HAVE_MCE_INJECTION 1
void kvm_arch_on_sigbus_vcpu(CPUState *cpu, int code, void *addr);
#endif

void kvm_arch_init_irq_routing(KVMState *s);
int kvm_arch_fixup_msi_route(struct kvm_irq_routing_entry *route,
                             uint64_t address, uint32_t data, PCIDevice *dev);
int kvm_arch_add_msi_route_post(struct kvm_irq_routing_entry *route, int vector, PCIDevice *dev);

// Notify arch about released MSI routes
int kvm_arch_release_virq_post(int virq);
int kvm_arch_msi_data_to_gsi(uint32_t data);
int kvm_set_irq(KVMState *s, int irq, int level);
int kvm_irqchip_send_msi(KVMState *s, MSIMessage msg);
void kvm_irqchip_add_irq_route(KVMState *s, int gsi, int irqchip, int pin);
void kvm_get_apic_state(DeviceState *d, struct kvm_lapic_state *kapic);

struct kvm_guest_debug;
struct kvm_debug_exit_arch;

struct kvm_sw_breakpoint {
	target_ulong pc;
	target_ulong saved_insn;
	int use_count;
	QTAILQ_ENTRY(kvm_sw_breakpoint) entry;
};

QTAILQ_HEAD(kvm_sw_breakpoint_head, kvm_sw_breakpoint);
struct kvm_sw_breakpoint *kvm_find_sw_breakpoint(CPUState *cpu, target_ulong pc);
int kvm_sw_breakpoints_active(CPUState *cpu);
int kvm_arch_insert_sw_breakpoint(CPUState *cpu, struct kvm_sw_breakpoint *bp);
int kvm_arch_remove_sw_breakpoint(CPUState *cpu, struct kvm_sw_breakpoint *bp);

int kvm_arch_insert_hw_breakpoint(target_ulong addr, target_ulong len, int type);
int kvm_arch_remove_hw_breakpoint(target_ulong addr, target_ulong len, int type);

void kvm_arch_remove_all_hw_breakpoints(void);
void kvm_arch_update_guest_debug(CPUState *cpu, struct kvm_guest_debug *dbg);
bool kvm_arch_stop_on_emulation_error(CPUState *cpu);
int kvm_check_extension(KVMState *s, unsigned int extension);
int kvm_vm_check_extension(KVMState *s, unsigned int extension);

#define kvm_vm_enable_cap(s, capability, cap_flags, ...) ({      \
	struct kvm_enable_cap cap = {                                \
			.cap = capability,                                   \
			.flags = cap_flags,                                  \
	};                                                           \
	uint64_t args_tmp[] = {__VA_ARGS__};                         \
	size_t n = MIN(ARRAY_SIZE(args_tmp), ARRAY_SIZE(cap.args));  \
	memcpy(cap.args, args_tmp, n * sizeof(cap.args[0]));         \
	kvm_vm_ioctl(s,KVM_ENABLE_CAP, &cap);                        \
	})

#define kvm_vcpu_enable_cap(cpu, capability, cap_flags, ...) ({  \
	struct kvm_enable_cap cap = {                                \
			.cap = capability,                                   \
			.flags = cap_flags,                                  \
	};                                                           \
	uint64_t args_tmp[] = {__VA_ARGS__};                         \
    size_t n = MIN(ARRAY_SIZE(args_tmp), ARRAY_SIZE(cap.args));  \
    memcpy(cap.args, args_tmp, n * sizeof(cap.args[0]));         \
    kvm_vm_ioctl(s,KVM_ENABLE_CAP, &cap);                        \
	})

uint32_t kvm_arch_get_supported_cpuid(KVMState *env, uint32_t function,
                                      uint32_t index, int reg);
#if !defined(CONFIG_USER_ONLY)
int kvm_physical_memory_addr_from_host(KVMState *s, void *ram_addr,
                                       hwaddr *phys_addr);
#endif

#endif
////////////////////////////////////////////////////////////

void kvm_cpu_synchronize_state(CPUState *cpu);
void kvm_cpu_synchronize_post_reset(CPUState *cpu);
void kvm_cpu_synchronize_post_init(CPUState *cpu);
void kvm_cpu_synchronize_pre_loadvm(CPUState *cpu);
void kvm_init_cpu_signals(CPUState *cpu);

int kvm_irqchip_add_msi_route(KVMState *s, int vector, PCIDevice *dev);
int kvm_irqchip_update_msi_route(KVMState *s, int virq, MSIMessage msg,PCIDevice *dev);
void kvm_irqchip_commit_routes(KVMState *s);
void kvm_irqchip_release_virq(KVMState *s, int virq);

int kvm_irqchip_add_adapter_route(KVMState *s, AdapterInfo *adapter);
int kvm_irqchip_add_hv_sint_route(KVMState *s, uint32_t vcpu, uint32_t sint);

int kvm_irqchip_add_irqfd_notifier_gsi(KVMState *s, EventNotifier *n, EventNotifier *rn, int virq);
int kvm_irqchip_remove_irqfd_notifier_gsi(KVMState *s, EventNotifier *n, int virq);
int kvm_irqchip_add_irqfd_notifier(KVMState *s, EventNotifier *n, EventNotifier *rn, qemu_irq irq);
int kvm_irqchip_remove_irqfd_notifier(KVMState *s, EventNotifier *n, qemu_irq irq);

void kvm_irqchip_set_qemuirq_gsi(KVMState *s, qemu_irq irq, int gsi);
void kvm_pc_gsi_handler(void *opaque, int n, int level);
void kvm_pc_setup_irq_routing(bool pci_enabled);
void kvm_init_irq_routing(KVMState *s);

// Allow architecture to create an in-kernel irq chip
int kvm_arch_irqchip_create(MachineState *ms, KVMState *s);

int kvm_set_one_reg(CPUState *cs, uint64_t id, void *source);

int kvm_get_one_reg(CPUState *cs, uint64_t id, void *target);
struct ppc_radix_page_info *kvm_get_radix_page_info(void);
int kvm_get_max_memslots(void);

#endif /* SYSEMU_KVM_H_ */
