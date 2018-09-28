/*
 * xen.h
 *
 *  Created on: Sep 28, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#ifndef HW_XEN_XEN_H_
#define HW_XEN_XEN_H_

#include "qemu-common.h"
#include "exec/cpu-common.h"
#include "hw/irq.h"

// stuff needed outside xen-.c interface to qemu

enum xen_node {
	XEN_EMULATE = 0,
	XEN_CREATE,
	XEN_ATTACH
};

extern uint32_t xen_domid;
extern enum xen_node xen_node;
extern bool xen_domid_restrict;
extern bool xen_allowed;

static inline bool xen_enabled(void) {
	return xen_allowed;
}

int xen_pci_slot_get_pirq(PCIDevice *pci_dev, int irq_num);
void xen_piix3_set_irq(void *opaque, int irq_num, int level);
void xen_piix_pci_write_config_client(uint32_t address, uint32_t val, int len);
void xen_hvm_inject_msi(uint64_t addr, uint32_t data);
int xen_is_pirq_msi(uint32_t msi_data);

qemu_irq *xen_interrupt_controller_init(void);
void xenstore_store_pv_console_info(int i, struct Chardev *chr);
void xen_hvm_init(PCMachineState *pcms, MemoryRegion **ram_memory);
void xen_ram_alloc(ram_addr_t ram_addr, ram_addr_t size,
		           struct MemoryRegion *mr, Error **errp);
void xen_hvm_modifier_memory(ram_addr_t start, ram_addr_t length);
void xen_register_framebuffer(struct MemoryRegion *mr);

#endif /* HW_XEN_XEN_H_ */
