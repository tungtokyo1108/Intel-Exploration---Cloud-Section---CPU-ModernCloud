/*
 * memory.h
 *
 *  Created on: Sep 4, 2018
 *      Student (coder): Tung Dang
 */

#ifndef EXEC_MEMORY_H_
#define EXEC_MEMORY_H_

#include "exec/cpu-common.h"
#include "exec/hwaddr.h"
#include "exec/memattrs.h"
#include "exec/ramlist.h"
#include "qemu/queue.h"
#include "qemu/int128.h"
#include "qemu/notify.h"
#include "qom/object.h"
#include "qemu/rcu.h"
#include "hw/qdev-core.h"

#define RAM_ADDR_INVALID (~(ram_addr_t)0)

#define MAX_PHYS_ADDR_SPACE_BITS 62
#define MAX_PHYS_ADDR (((hwaddr)1 << MAX_PHYS_ADDR_SPACE_BITS) - 1)

#define TYPE_MEMORY_REGION "qemu:memory-region"
#define MEMORY_REGION(obj) \
	OBJECT_CHECK(MemoryRegion, (obj), TYPE_MEMORY_REGION)

#define TYPE_IOMMU_MEMORY_REGION "qemu:iommu-memory-region"
#define IOMMU_MEMORY_REGION(obj) \
	OBJECT_CHECK(IOMMUMemoryRegion, (obj), TYPE_IOMMU_MEMORY_REGION)
#define IOMMU_MEMORY_REGION_CLASS(klass) \
	OBJECT_CLASS_CHECK(IOMMUMemoryRegionClass, (klass), TYPE_IOMMU_MEMORY_REGION)
#define IOMMU_MEMORY_REGION_GET_CLASS(obj) \
	OBJECT_GET_CLASS(IOMMUMemoryRegionClass, (obj), TYPE_IOMMU_MEMORY_REGION)

typedef struct MemoryRegionOps MemoryRegionOps;
typedef struct MemoryRegionMmio MemoryRegionMmio;

typedef struct MemoryRegionMmio {
	CPUReadMemoryFunc *read[3];
	CPUWriteMemoryFunc *write[3];
} MemoryRegionMmio;

typedef struct IOMMUTLBEntry IOMMUTLBEntry;

typedef enum {
	IOMMU_NONE = 0,
    IOMMU_RO   = 1,
	IOMMU_WO   = 2,
	IOMMU_RW   = 3,
} IOMMUAccessFlags;

#define IOMMU_ACCESS_FLAG(r, w) (((r) ? IOMMU_RO : 0) | ((w) ? IOMMU_WO : 0))

typedef struct IOMMUTLBEntry {
	AddressSpace *target_as;
	hwaddr iova;
	hwaddr translated_addr;
	hwaddr addr_mask;
	IOMMUAccessFlags perm;
} IOMMUTLBEntry;

typedef enum {
    IOMMU_NOTIFIER_NONE = 0,
    /* Notify cache invalidations */
    IOMMU_NOTIFIER_UNMAP = 0x1,
    /* Notify entry changes */
    IOMMU_NOTIFIER_MAP = 0x2,
} IOMMUNotifierFlag;

#define IOMMU_NOTIFIER_ALL (IOMMU_NOTIFIER_MAP | IOMMU_NOTIFIER_UNMAP)

typedef struct IOMMUNotifier IOMMUNotifier;
typedef void (*IOMMUNotify)(IOMMUNotifier *notifier, IOMMUTLBEntry *data);

typedef struct IOMMUNotifier {
	IOMMUNotify notify;
	IOMMUNotifierFlag notifier_flags;
	// Notify for address space range
	hwaddr start;
	hwaddr end;
	int iommu_idx;
	QLIST_ENTRY(IOMMUNotifier) node;
} IOMMUNotifier;

static inline void iommu_notifier_init(IOMMUNotifier *n, IOMMUNotify fn, IOMMUNotifier flags, hwaddr start, hwaddr end, int iommu_idx) {
	n->notify = fn;
	n->notifier_flags = flags;
	n->start = start;
	n->end = end;
	n->iommu_idx = iommu_idx;
}

// Memory region callbacks

typedef struct MemoryRegionOps {
	// Read from memory region. @addr is relative to @mr; @size is in bytes
	uint64_t (*read)(void *opaque, hwaddr addr, unsigned size);
	// Write to the memory region.
	void (*write)(void *opaque, hwaddr addr, uint64_t data, unsigned size);
	MemTxResult(*read_with_attrs)(void *opaque, hwaddr addr, uint64_t *data, unsigned size, MemTxAttrs attrs);
	MemTxResult(*wrote_with_attrs)(void *opaque, hwaddr addr, uint64_t *data, unsigned size, MemTxAttrs attrs);

	// Return a pointer to a location with contains guest code
	void *(*request_ptr)(void *opaque, hwaddr addr, unsigned *size, unsigned *offset);
	enum device_endian endlianness;

	// Guest-visible constraints
	struct valid {
		// specify bounds on access sizes beyond which a machine check is thrown
		unsigned min_access_size;
		unsigned max_access_size;
		bool unaligned;
		bool (*accepts)(void *opaque, hwaddr addr, unsigned size, bool is_write, MemTxAttrs attrs);
	} valid;

	struct impl {
		// specifies the minimum size implemented. Smaller sizes will be rounded upwards and a partial result will be returned
		unsigned min_access_size;
		// specifies the maximum size implemented. Larger sizes will be done as a series of accesses with smaller size
		unsigned max_access_size;
		bool unaligned;
	} impl;

	const MemoryRegionMmio old_mmio;
} MemoryRegionOps;

enum IOMMUMemoryRegionAttr {
    IOMMU_ATTR_SPAPR_TCE_FD
};

/**
 * an IOMMU provides a mapping from input address to an output TLB entry.
 * If the IOMMU is aware of memory transaction attributes and the output TLB entry depends on transaction attributes. we represent this using IOMMU indexes.
 * @attr_to_index returns the IOMMU index for a set of transaction attributes
 * @translate takes an input address and an IOMMU index
 * and mapping return can only depend on the input address and IOMMU index
 */

typedef struct IOMMUMemoryRegionClass IOMMUMemoryRegionClass;
typedef struct IOMMUMemoryRegionClass {
	struct DeviceClass parent_class;
	/**
	 * The IOMMUAccessFlags indicated via @flag are optional and may be specified as IOMMU_NONE to indicate that caller needs the full translation information
	 * for both reads and write.
	 * If the access flags are specified then IOMMU implementation may use this as optimization, to stop doing a page table walk as soon as it knows
	 * that the requested permission are not allowed.
	 */
	IOMMUTLBEntry (*translate)(IOMMUMemoryRegion *iommu, hwaddr addr, IOMMUAccessFlags flag, int iommu_idx);
	// Return minimum supported page size in bytes
	uint64_t (*get_min_page_size)(IOMMUMemoryRegion *iommu);
	void (*notify_flag_changed)(IOMMUMemoryRegion *iommu, IOMMUNotifierFlag old_flags, IOMMUNotifierFlag new_flags);
	/**
	 * The default implementation of memory_region_iommu_replay() is to call the IOMMU translate method for
	 * every page in address space with flag == IOMMU_NONE and then call the notifier if translate return a valid mapping
	 */
	void (*replay)(IOMMUMemoryRegion *iommu, IOMMUNotifier *notifier);
	/**
	 * An optimal method can be used to allow users of the IOMMU to get implementation-specific information.
	 * The IOMMU implements this method to handle calls by IOMMU users to memory_region_iommu_get_attr() by filling
	 * in the arbitrary data pointer for any IOMMUMemoryRegionAttr values
	 */
	int (*get_attr)(IOMMUMemoryRegion *iommu, enum IOMMUMemoryRegionAttr attr, void *data);
	// the IOMMU index to use for a given set of transaction attributes
	int (*attrs_to_index)(IOMMUMemoryRegion *iommu, MemTxAttrs attrs);
	int (*num_indexes)(IOMMUMemoryRegion *iommu);
} IOMMUMemoryRegionClass;

typedef struct CoalescedMemoryRange CoalescedMemoryRange;
typedef struct MemoryRegionIoeventfd MemoryRegionIoeventfd;
typedef struct MemoryRegion MemoryRegion;

typedef struct MemoryRegion {
	Object parent_obj;

	bool romd_mode;
	bool ram;
	bool subpage;
	bool readonly;
	bool rom_device;
	bool flush_coalesced_mmio;
	bool global_locking;
	uint8_t dirty_log_mask;
	bool is_iommu;
	RAMBlock *ram_block;
	Object *owner;

	const MemoryRegionOps *ops;
	void *opaque;
	MemoryRegion *container;
	Int128 size;
	hwaddr addr;
	void (*destructor)(MemoryRegion *mr);
	uint64_t align;
	bool terminates;
	bool ram_device;
	bool enabled;
	bool warning_printed;
	uint8_t vga_logging_count;
	MemoryRegion *alias;
	hwaddr alias_offset;
	int32_t priority;
	QTAILQ_HEAD(subregions, MemoryRegion) subregions;
	QTAILQ_ENTRY(MemoryRegion) subregions_link;
	QTAILQ_HEAD(coalesced_ranges, CoalescedMemoryRange) colesced;
	const char *name;
	unsigned ioventfd_nb;
	MemoryRegionIoeventfd *ioeventfds;
} MemoryRegion;

typedef struct IOMMUMemoryRegion IOMMUMemoryRegion;
typedef struct IOMMUMemoryRegion {
	MemoryRegion parent_obj;
	QLIST_HEAD(, IOMMUNotifier) iommu_notify;
	IOMMUNotifierFlag iommu_notify_flags;
} IOMMUMemoryRegion;

#define IOMMU_NOTIFIER_FOREACH(n,mr) \
	QLIST_FOREACH((n), &(mr)->iommu_notify, node)

/**
 * Callbacks structure for updates to the physical memory map
 * Allows a component to adjust to changes in the guest-visible memory map
 */

typedef struct MemoryListener MemoryListener;
typedef struct MemoryListener {
	void (*begin)(MemoryListener *listener);
	void (*commit)(MemoryListener *listener);
	void (*region_add)(MemoryListener *listener, MemoryRegionSection *section);
	void (*region_del)(MemoryListener *listener, MemoryRegionSection *section);
	void (*region_nop)(MemoryListener *listener, MemoryRegionSection *section);
	void (*log_start)(MemoryListener *listener, MemoryRegionSection *section, int old, int _new);
	void (*log_stop)(MemoryListener *listener, MemoryRegionSection *section, int old, int _new);
	void (*log_sync)(MemoryListener *listener, MemoryRegionSection *section);
	void (*log_global_start)(MemoryListener *listener);
	void (*log_global_stop)(MemoryListener *listener);
	void (*eventfd_add)(MemoryListener *listener, MemoryRegionSection *section, bool match_data, uint64_t data, EventNotifier *e);
	void (*eventfd_del)(MemoryListener *listener, MemoryRegionSection *section, bool match_data, uint64_t data, EventNotifier *e);
	void (*coalesced_mmio_add)(MemoryListener *listener, MemoryRegionSection *section, hwaddr addr, hwaddr len);
	void (*coalesced_mmio_del)(MemoryListener *listener, MemoryRegionSection *section, hwaddr addr, hwaddr len);
	unsigned priority;
	AddressSpace *address_space;
	QTAILQ_ENTRY(MemoryListener) link;
	QTAILQ_ENTRY(MemoryListener) link_as;
} MemoryListener;

typedef struct AddressSpace AddressSpace;
typedef struct AddressSpace {
	struct rcu_head rcu;
	char *name;
	MemoryRegion *root;
	FlatView *current_map;
	int ioeventfd_nb;
	MemoryRegionIoeventfd *ioeventfds;
	QTAILQ_HEAD(memory_listener_as, MemoryListener) listeners;
	QTAILQ_ENTRY(AddressSpace) address_spaces_link;
} AddressSpace;

typedef struct AddressSpaceDispatch AddressSpaceDispatch;
typedef struct FlatRange FlatRange;
typedef struct FlatView FlatView;
typedef struct FlatView {
	struct rcu_head rcu;
	unsigned ref;
	FlatRange *ranges;
	unsigned nr;
	unsigned nr_allocated;
	struct AddressSpaceDispatch *dispatch;
	MemoryRegion *root;
} FlatView;

static inline FlatView *address_space_to_flatview(AddressSpace *as) {
	return atomic_rcu_read(&as->current_map);
}

typedef struct MemoryRegionSection MemoryRegionSection;
typedef struct MemoryRegionSection {
	MemoryRegion *mr;
	FlatView *fv;
	hwaddr offset_within_region;
	Int128 size;
	hwaddr offset_within_address_space;
	bool readonly;
} MemoryRegionSection;

// the region typically acts as a container for other memory regions

void memory_region_init(MemoryRegion *mr, Object *owner, const char *name, uint64_t size);

/**
 * Whenever memory regions are accessed outside the BQL, they need to be preserved against hot-unplug,
 * All MemoryRegions must have an owner if they can disappear, even if the device they belong to operates under the BQL.
 */

void memory_region_ref(MemoryRegion *mr);
void memory_region_unref(MemoryRegion *mr);

// Access into the region will cause the callbacks in @op to be called. If @size is nonzero, subregions will be clipped to @size

void memory_region_init_io(MemoryRegion *mr, Object *owner, const MemoryRegionOps *ops, void *opaque, const char *name, uint64_t size);

// Initialize RAM memory region. Accesses into the region will modify memory directly.

void memory_region_init_ram_nomigrate(MemoryRegion *mr, Oject *owner, const char *name, uint64_t size, Error **errp);
void memory_region_init_ram_shared_nomigrate(MemoryRegion *mr, Oject *owner, const char *name, uint64_t size, bool share, Error **errp);

// Initialize memory region with resizeable RAM. Only an initial portion of this RAM is actually used.
// The used size can change across reboots

void memory_region_init_resizeable_ram(MemoryRegion *mr, Object *owner, const char *name, uint64_t size, uint64_t max_size,
		                               void (*resized)(const char *, uint64_t length, void *host), Error **errp);

// Initialize memory region with a mmap-end backend
void memory_region_init_ram_from_file(MemoryRegion *mr, Object *owner, const char *name, uint64_t size, uint64_t align,
		                              bool share, const char *path, Error **errp);
void memory_region_init_ram_from_fd(MemoryRegion *mr, Object *owner, const char *name, uint64_t size, uint64_t align,
		                              bool share, int fd, Error **errp);

// Initialize RAM memory region from a user-provided pointer.
void memory_region_init_ram_ptr(MemoryRegion *mr, Oject *owner, const char *name, uint64_t size, void *ptr);

/**
 * A RAM device represents a mapping to a physical device.
 * The memory region may be mapped into the VM address space and access to the region will modify directly
 * The memory region should not be included in a memory dump, device may not be enabled/mapped at the time of the dump
 */
void memory_region_init_ram_device_ptr(MemoryRegion *mr, Oject *owner, const char *name, uint64_t size, void *ptr);

// Initialize a memory region that aliase all or a part of another memory region.
void memory_region_init_alias(MemoryRegion *mr, Object *owner, const char *name, MemoryRegion *orig, hwaddr offset, uint64_t size);

// Initialize a ROM memory region.
void memory_region_init_rom_nomigrate(MemoryRegion *mr, Oject *owner, const char *name, uint64_t size, Error **errp);

// Initialize a ROM memory region, write are handled via callbacks
void memory_region_init_rom_device_nomigrate(MemoryRegion *mr, Oject *owner,const MemoryRegionOps *ops, void *opaque,
		                                     const char *name, uint64_t size, Error **errp);

/**
 * Initialize a memory region of a custom type that translate addresses
 * An IOMMU region translate addresses and forwards accesses to a target memory region.
 * IOMMU implementation must define a subclass of TYPE_IOMMU_MEMORY_REGION.
 * @_iommu_mr should be a pointer to enough memory for an instance of that subclass
 * @instance_size is  the size of the subclass
 * will be called to handle accesses to memory region
 */

void memory_region_init_iommu(void *_iommu_mr, size_t instance_size, const char *mrtypename, Object *owner, const char *name, uint64_t size);

/**
 * Initialize RAM memory region.
 * This function allocates RAM for a board model or device,
 * arranges for it to be migrated
 */

void memory_region_init_ram(MemoryRegion *mr, Object *owner, const char *name, uint64_t size, Error **errp);

/**
 * Initialize a ROM memory region
 * This has the same effect as calling memory_region_init_ram()
 * marking region read-only with memory_region_set_readonly().
 * arranging for the contents to be migrated
 */

void memory_region_init_rom(MemoryRegion *mr, Object *owner, const char *name, uint64_t size, Error **errp);

/**
 * Initialize a ROM memory region. Writes are handled via callbacks
 * This function initializes a memory region backed by RAM or reads and callbacks for writes
 * arranges for the RAM backing to be migrated
 */

void memory_region_init_rom_device(MemoryRegion *mr, Object *owner,const MemoryRegionOps *ops, void *opaque, const char *name, uint64_t size, Error **errp);

// get a memory region's owner
struct Object *memory_region_owner(MemoryRegion *mr);
uint64_t memory_region_size(MemoryRegion *mr);

// check whether a memory region is random access
static inline bool memory_region_is_ram(MemoryRegion *mr) {
	return mr->ram;
}

// check whether a memory region is a random device
bool memory_region_is_ram_device(MemoryRegion *mr);

// check whether a memory region is in ROMD mode
static inline bool memory_region_is_romd(MemoryRegion *mr) {
	return mr->rom_device && mr->romd_mode;
}

// check whether a memory region is an iommu
static inline IOMMUMemoryRegion *memory_region_get_iommu(MemoryRegion *mr) {
	if (mr->alias)
	{
		return memory_region_get_iommu(mr->alias);
	}
	if (mr->is_iommu)
	{
		return (IOMMUMemoryRegion *) mr;
	}
	return NULL;
}

static inline IOMMUMemoryRegionClass *memory_region_get_iommu_class_nocheck(IOMMUMemoryRegion *iommu_mr) {
	return (IOMMUMemoryRegionClass *) (((Object *)iommu_mr));
}

#define memory_region_is_iommu(mr) (memory_region_get_iommu(mr) != NULL)

// get minimum supported page size for an iommu

uint64_t memory_region_iommu_get_min_page_size(IOMMUMemoryRegion *iommu_mr);

/**
 * Notify a change in an IOMMU translation entry
 * Notification type will be decided by entry.perm bits
 * @UNMAP (cache invalidation) notifier: set entry.perm to IOMMU_NONE
 * @MAP (newly added entry) notifier: set entry.perm to the permission to the page
 */

void memory_region_notify_iommu(IOMMUMemoryRegion *iommu_mr, int iommu_idx, IOMMUTLBEntry entry);

// notify a change in an IOMMU translation entry to a single notifier

void memory_region_notify_one(IOMMUNotifier *notifier, IOMMUTLBEntry *entry);

// register a notifier for changes to IOMMU translation entries
void memory_region_register_iommu_notifier(MemoryRegion *mr, IOMMUNotifier *n);

// replay existing IOMMU translations to a notifier with the minimum page
void memory_region_iommu_replay(IOMMUMemoryRegion *iommu_mr, IOMMUNotifier *n);
void memory_region_iommu_replay_all(IOMMUMemoryRegion *iommu_mr);

// unregister a notifier for changes to IOMMU translation entries
void memory_region_unregister_iommu_notifier(MemoryRegion *mr, IOMMUNotifier *n);

#endif /* EXEC_MEMORY_H_ */
