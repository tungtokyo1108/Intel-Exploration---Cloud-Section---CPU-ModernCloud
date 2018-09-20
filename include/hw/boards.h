/*
 * boards.h
 *
 *  Created on: Sep 19, 2018
 *      Student (coder): Tung Dang
 */

#ifndef HW_BOARDS_H_
#define HW_BOARDS_H_

#include "sysemu/blockdev.h"
#include "sysemu/accel.h"
#include "hw/fw-build.h"
#include "hw/qdev.h"
#include "qom/object.h"
#include "qom/cpu.h"
#include "exec/memory.h"
#include "qemu/typedefs.h"
/**
 * Allocate a broad's main memory and initializes Memory Region appropriately.
 * Arrange for the memory to be migrated.
 *
 * Memory allocated via this function will be backed with the memory backend
 * the user provided using "-mem-path"
 *
 * For boards where the major RAM is split into two parts in the memory map,
 * Deal with this by calling memory_region_allocate_system_memory()
 * once to get a MemoryRegion with enough RAM for both parts,
 * creating alias MemoryRegions via memory_region_init_alias() which
 * alias into different parts of this RAM MemoryRegion and can be mapped
 * into the memory map in the appropriate places
 */

void memory_region_allocate_system_memory(MemoryRegion *mr, Object *owner, const char *name, uint64_t ram_size);
#define TYPE_MACHINE_SUFFIX "-machine"

#define MACHINE_TYPE_NAME(machinename) (machinename TYPE_MACHINE_SUFFIX)
#define TYPE_MACHINE "machine"
#undef MACHINE
#define MACHINE(obj) \
	OBJECT_CHECK(MachineState, (obj), TYPE_MACHINE)

#define MACHINE_GET_CLASS(obj) \
	OBJECT_GET_CLASS(MachineClass, (obj), TYPE_MACHINE)

#define MACHINE_CLASS(klass) \
	OBJECT_CLASS_CHECK(MachineClass, (klass), TYPE_MACHINE)

MachineClass *find_default_machine(void);
extern MachineState *current_machine;

typedef struct HotpluggableCPUList HotpluggableCPUList;
typedef struct CpuInstanceProperties CpuInstanceProperties;

void machine_run_board_init(MachineState *machine);
bool machine_usb(MachineState *machine);
bool machine_kernel_irqchip_allowed(MachineState *machine);
bool machine_kernel_irqchip_required(MachineState *machine);
bool machine_kernel_irqchip_split(MachineState *machine);
int machine_kvm_shadow_mem(MachineState *machine);
int machine_phandle_start(MachineState *machine);
bool machine_dump_guest_core(MachineState *machine);
bool machine_mem_merge(MachineState *machine);
void machine_register_compat_props(MachineState *machine);
HotpluggableCPUList *machine_query_hotpluggable_cpus(MachineState *machine);
void machine_set_cpu_numa_node(MachineState *machine,
                               const CpuInstanceProperties *props, Error **errp);
void machine_class_allow_dynamic_sysbus_dev(MachineClass *mc, const char *type);

typedef struct CPUArchId CPUArchId;
typedef struct CPUArchId {
	uint64_t arch_id;
	int64_t vcpus_count;
	CpuInstanceProperties props;
	Object *cpu;
	const char *tpye;
} CPUArchId;

typedef struct CPUArchIdList CPUArchIdlist;
typedef struct CPUArchIdList {
	int len;
	CPUArchId cpus[0];
} CPUArchIdList;

typedef struct strList strList;

typedef struct MachineClass MachineClass;
typedef struct MachineClass {
	ObjectClass parent_class;

	const char *family;
	char *name;
	const char *alias;
	const char *desc;
	const char *deprecation_reason;

	void (*init)(MachineState *state);
	void (*reset)(void);
	void (*hot_add_cpu)(const int64_t id, Error **errp);
	int (*kvm_type)(const char *arg);

	BlockInterfaceType block_default_type;
	int units_per_default_bus;
	int max_cpus;
	int min_cpus;
	int default_cpus;
	unsigned int no_serial:1,
	no_parallel:1,
	use_virtcon:1,
	no_floppy:1,
	no_cdrom:1,
	no_sdcard:1,
	pci_allow_0_address:1,
	legacy_fw_cfg_order:1;

	int is_default;
	const char *default_machine_opts;
	const char *default_boot_order;
	const char *default_display;
	GArray *compat_props;
	const char *hw_version;
	ram_addr_t default_ram_size;
	const char *default_cpu_type;
	bool option_rom_has_mr;
	bool rom_file_has_mr;
	int minimum_page_bits;
	bool has_hotpluggable_cpus;
	bool ignore_memory_transaction_failures;
	int numa_mem_align_shift;
	const char **valid_cpu_types;
	strList *allowed_dynamic_sysbus_devices;
	bool auto_anable_numa_with_memhp;
	void (*numa_auto_assign_ram)(MachineClass *mc, NodeInfo *nodes, int nb_nodes, ram_addr_t size);
	HotplugHandler *(*get_hotplug_handler)(MachineState *machine, DeviceState *dev);
	CpuInstanceProperties(*cpu_index_to_instance_props)(MachineState *machine, unsigned cpu_index);
	const CPUArchIdList *(*possible_cpu_arch_ids)(MachineState *machine);
	int64_t (*get_default_cpu_mode_id)(const MachineState *ms, int idx);

	FirmwareBuildMethods firmware_build_methods;

} MachineClass;

typedef struct DeviceMemoryState {
	hwaddr base;
	MemoryRegion mr;
} DeviceMemoryState;

typedef struct MachineState {
	Object parent_obj;
	Notifier sysbus_notifier;

	char *accel;
	bool kernel_irqchip_allowed;
	bool kernel_irqchip_required;
	bool kernel_irqchip_split;
	int kvm_shadow_mem;
	char *dtb;
	char *dumpdtb;
	int phandle_start;
	char *dt_compatible;
	bool dump_guest_core;
	bool mem_merge;
	bool usb;
	bool usb_disabled;
	bool igd_gfx_passthru;
	char *firmware;
	bool iommu;
	bool suppress_vmdesc;
	bool enfore_config_section;
	bool enable_graphics;
	char *memory_encryption;
	DeviceMemoryState *device_memory;

	ram_addr_t ram_size;
	ram_addr_t maxram_size;
	uint64_t ram_slots;
	const char *boot_order;
	char *kernel_filename;
	char *kernel_cmdline;
	char *initrd_filename;
	const char *cpu_type;
	AccelState *accelerator;
	CPUArchIdList *possible_cpus;

	FirmwareBuildState firmware_build_state;

} MachineState;

#define DEFINE_MACHINE(namestr, machine_initfn) \
	static void machine_initfn##_class_init(ObjectClass *oc, void *data) \
	{\
	MachineClass *mc = MACHINE_CLASS(oc); \
	machine_initfn(mc);                   \
	}\
	static const TypeInfo machine_initfn##_typeinfo = {                  \
			.name = MACHINE_TYPE_NAME(namestr),                          \
			.parent = TYPE_MACHINE,                                      \
			.class_init = machine_initfn##_class_init,                   \
	};                                                                   \
	static void machine_initfn##_register_types(void) {                  \
		type_register_static(&machine_initfn##_typeinfo);                \
	}\
	type_init(machine_initfn##_register_types)

#define SET_MACHINE_COMPAT(m, COMPAT) \
	do {                              \
		int i;                        \
		static GlobalProperty props[] = { \
				COMPAT                    \
		};                                \
		if (!m->compat_props) {           \
			m->compat_props = g_array_new(false, false, sizeof(void *)); \
		}\
		for (i = 0; props[i].driver != NULL; i++) { \
			GlobalProperty *prop = &props[i];       \
			g_array_append_val(m->compat_props,prop);\
		}\
	} while (0)

#endif /* HW_BOARDS_H_ */
