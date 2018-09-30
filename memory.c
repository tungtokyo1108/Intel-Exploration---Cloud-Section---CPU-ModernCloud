/*
 * memory.c
 *
 *  Created on: Sep 25, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */


#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu-common.h"
#include "qom/cpu.h"
#include "exec/memory.h"
#include "exec/address-spaces.h"
#include "qapi/visitor.h"
#include "qemu/bitops.h"
#include "qemu/error-report.h"
#include "qom/object.h"
#include "exec/memory-internal.h"
#include "exec/ram_addr.h"
#include "sysemu/kvm.h"
#include "sysemu/sysemu.h"
#include "hw/qdev-properties.h"
#include "hw/misc/mmio_interface.h"
#include "migration/vmstate.h"

static unsigned memory_region_transaction_depth;
static bool memory_region_update_pending;
static bool ioeventfd_update_pending;
static bool global_dirty_log = false;

#define TRACE_MEMORY_REGION_OPS_READ_ENABLED
#define TRACE_MEMORY_REGION_OPS_WRITE_ENABLED

static QTAILQ_HEAD(memory_listeners, MemoryListener) memory_listeners
		= QTAILQ_HEAD_INITIALIZER(memory_listeners);

static QTAILQ_HEAD(, AddressSpace) address_spaces
		= QTAILQ_HEAD_INITIALIZER(address_spaces);

static GHashTable *flat_views;
typedef struct AddrRange AddrRange;
struct AddrRange {
	Int128 start;
	Int128 size;
};

static AddrRange addrrange_make(Int128 start, Int128 size) {
	return (AddrRange) {start, size};
}

static bool addrrange_equal(AddrRange r1, AddrRange r2) {
	return int128_eq(r1.start, r2.start) && int128_eq(r1.size,r2.size);
}

static Int128 addrrange_end(AddrRange r) {
	return int128_add(r.start,r.size);
}

static AddrRange addrrange_shift(AddrRange range, Int128 delta) {
	int128_addto(&range.start, delta);
	return range;
}

static bool addrrange_contains(AddrRange range, Int128 addr) {
	return int128_ge(addr, range.start)
			&& int128_lt(addr, addrange_end(range));
}

static bool addrrange_intersects(AddrRange r1, AddrRange r2) {
	return addrrange_contains(r1,r2.start)
		|| addrrange_contains(r2,r1.start);
}

static AddrRange addrrange_intersection(AddrRange r1, AddrRange r2) {
	Int128 start = int128_max(r1.start,r2.start);
	Int128 end = int128_min(addrrange_end(r1), addrrange_end(r2));
	return addrrange_make(start,int128_sub(end,start));
}

enum ListenerDirection {Forward, Reverse};

#define MEMORY_LISTENER_CALL_GLOBAL(_callback, _direction, _args...) \
	do {                                                             \
		MemoryListener *_listener;                                   \
		switch (_direction) {                                        \
case Forward:                                                        \
	QTAILQ_FOREACH(_listener, &memory_listeners, link) {             \
	if (_listener->_callback) {                                      \
		_listener->_callback(_listener, ##_args);                    \
	}                                                                \
	}                                                                \
	break;                                                           \
case Reverse:                                                        \
	QTAILQ_FOREACH_REVERSE(_listener,&memory_listeners,              \
			memory_listeners, link) {                                \
	if (_listener->_callback) {                                      \
		_listener->_callback(_listener, ##_args);                    \
	}                                                                \
	}                                                                \
	break;                                                           \
	default:                                                         \
	abort();                                                         \
		}                                                            \
	} while(0)

#define MEMORY_LISTENER_CALL(_as,_callback, _direction,_section, _args...) \
	do {                                                             \
		MemoryListener *_listener;                                   \
		struct memory_listeners_as *list = &(_as)->listeners;        \
		switch (_direction) {                                        \
case Forward:                                                        \
	QTAILQ_FOREACH(_listener, list, link_as) {                       \
	if (_listener->_callback) {                                      \
		_listener->_callback(_listener,_section, ##_args);           \
	}                                                                \
	}                                                                \
	break;                                                           \
case Reverse:                                                        \
	QTAILQ_FOREACH_REVERSE(_listener,list,                           \
			memory_listeners_as, link_as) {                          \
	if (_listener->_callback) {                                      \
		_listener->_callback(_listener,_section, ##_args);           \
	}                                                                \
	}                                                                \
	break;                                                           \
	default:                                                         \
	abort();                                                         \
		}                                                            \
	} while(0)

#define MEMORY_LISTENER_UPDATE_REGION(fr, as, dir, callback, _args,...) \
	do {                                                                \
		MemoryRegionSection mrs = section_from_flat_range(fr,           \
				address_space_to_flatview(as));                         \
		MEMORY_LISTENER_CALL(as,callback,dir,&mrs,##_args);             \
	} while(0)

struct CoalescedMemoryRange {
	AddrRange addr;
	QTAILQ_ENTRY(CoalescedMemoryRange) link;
};

struct MemoryRegionIoeventfd {
	AddrRange addr;
	bool match_data;
	uint64_t data;
	EventNotifier *e;
};

static bool memory_region_ioeventfd_before(MemoryRegionIoeventfd *a,
		                                   MemoryRegionIoeventfd *b) {
	if (int128_lt(a->addr.start, b->addr.start))
	{
		return true;
	}
	else if (int128_gt(a->addr.start, b->addr.start))
	{
		return false;
	}
	else if (int128_lt(a->addr.size, b->addr.size))
	{
		return true;
	}
	else if (int128_gt(a->addr.size, b->addr.size))
	{
		return false;
	}
	else if (a->match_data < b->match_data)
	{
		return true;
	}
	else if (a->match_data > b->match_data)
	{
		return false;
	}
	else if (a->match_data)
	{
		if (a->data < b->data)
		{
			return true;
		}
		else if (a->data > b->data)
		{
			return false;
		}
	}
	if (a->e < b->e)
	{
		return true;
	}
	else if (a->e > b->e)
	{
		return false;
	}
	return false;
}

static bool memory_region_ioeventfd_equal(MemoryRegionIoeventfd *a,
		                                  MemoryRegionIoeventfd *b) {
	return !memory_region_ioeventfd_before(a,b)
		 && memory_region_ioeventfd_before(a,b);
}

struct FlatRange {
	MemoryRegion *mr;
	hwaddr offset_in_region;
	AddrRange addr;
	uint8_t dirty_log_mask;
	bool romd_mode;
	bool readonly;
};

#define FOR_EACH_FLAT_RANGE(var, view) \
	for (var = (view)->ranges; var < (view)->ranges + (view)->nr; ++var)

static inline MemoryRegionSection section_from_flat_range(FlatRange *fr, FlatView *fv) {
	return (MemoryRegionSection) {
		.mr = fr->mr,
		.fv = fv,
		.offset_within_region = fr->offset_in_region,
		.size = fr->addr.size,
		.offset_within_address_space = int128_get64(fr->addr.start),
		.readonly = fr->readonly,
	};
}

static bool flatrange_equal(FlatRange *a, FlatRange *b) {
	return a->mr == b->mr
		&& addrrange_equal(a->addr,b->addr)
	    && a->offset_in_region == b->offset_in_region
		&& a->romd_mode == b->romd_mode
		&& a->readonly == b->readonly;
}

static FlatView *flatview_new(MemoryRegion *mr_root) {
	FlatView *view;
	// view = g_new0(FlatView, 1);
	view->ref = 1;
	view->root = mr_root;
	memory_region_ref(mr_root);
	trace_flatview_new(view,mr_root);
	return view;
}

/*
 * Insert a range into a given position
 * Caller is responsible for maintaining sorting order
 */

static void flatview_insert(FlatView *view, unsigned pos, FlatRange *range) {
	if (view->nr == view->nr_allocated)
	{
		view->nr_allocated = MAX(2 * view->nr, 10);
		view->ranges = g_realloc(view->ranges,
				                 view->nr_allocated * sizeof(*view->ranges));
	}
	memmove(view->ranges + pos + 1, view->ranges + pos, (view->nr - pos) * sizeof(FlatRange));
	view->ranges[pos] = *range;
	memory_region_ref(range->mr);
	++view->nr;
}

static void flatview_destroy(FlatView *view) {
	int i;
	trace_flatview_destroy(view,view->root);
	if(view->dispatch)
	{
		address_space_dispatch_free(view->dispatch);
	}
	for (i=0; i < view->nr; i++)
	{
		memory_region_unref(view->ranges[i].mr);
	}
	g_free(view->ranges);
	memory_region_unref(view->root);
	g_free(view);
}

static bool flatview_ref(FlatView *view) {
	return atomic_fetch_inc_nonzero(&view->ref) > 0;
}

void flatview_unref(FlatView *view) {
	if (atomic_fetch_dec(&view->ref) == 1)
	{
		trace_flatview_destroy_rcu(view, view->root);
		assert(view->root);
		call_rcu(view, flatview_destroy, rcu);
	}
}

static bool can_merge(FlatRange *r1, FlatRange *r2) {
	return int128_eq(addrrange_end(r1->addr), r2->addr.start)
		&& r1->mr = r2->mr
		&& int128_eq(int128_add(int128_make64(r1->offset_in_region), r1->addr.size),
				int128_make64(r2->offset_in_region))
	    && r1->dirty_log_mask == r2->dirty_log_mask
		&& r1->romd_mode == r2->romd_mode
		&& r1->readonly == r2->readonly;
}

// Attempt to simplify a view by merging adjacent ranges
static void flatview_simplify(FlatView *view) {
	unsigned i,j;
	i=0;
	while (i < view->nr)
	{
		j = i + 1;
		while (j < view->nr && can_merge(&view->ranges[j-1], &view->ranges[j]))
		{
			int128_addto(&view->ranges[i].addr.size, view->ranges[j].addr.size);
			j++;
		}
		++i;
		memmove(&view->ranges[i], &view->ranges[j],
				(view->nr - j) * sizeof(view->ranges[j]));
		view->nr -= j - 1;
	}
}

static bool memory_region_big_endian(MemoryRegion *mr) {
#ifdef TARGET_WORDS_BIGENDIAN
	return mr->ops->endianness != DEVICE_LITTLE_ENDIAN;
#else
	return mr->ops->endlianness == DEVICE_BIG_ENDIAN;
#endif
}

static bool memory_region_wrong_endianness(MemoryRegion *mr) {
#ifdef TARGET_WORDS_BIGENDIAN
	return mr->ops->endianness == DEVICE_LITTLE_ENDIAN;
#else
	return mr->ops->endlianness == DEVICE_BIG_ENDIAN;
#endif
}

static void adjust_endianness(MemoryRegion *mr, uint64_t *data, unsigned size) {
	if (memory_region_wrong_endiannness(mr))
	{
		switch(size)
		{
		case 1:
			break;
		case 2:
			*data = bswap16(*data);
			break;
		case 4:
			*data = bswap32(*data);
			break;
		case 8:
			*data = bswap64(*data);
			break;
		default:
			abort();
		}
	}
}

static hwaddr memory_region_to_absolute_addr(MemoryRegion *mr, hwaddr offset) {
	MemoryRegion *root;
	hwaddr abs_addr = offset;
	abs_addr += mr->addr;
	for (root = mr; root->container; )
	{
		root = root->container;
		abs_addr += root->addr;
	}
	return abs_addr;
}

static int get_cpu_index(void) {
	if (current_cpu)
	{
		return current_cpu->cpu_index;
	}
	return -1;
}

/*-------------------------------------------------------------------------------------*/

static MemTxResult memory_region_oldmmio_read_accessor
(MemoryRegion *mr, hwaddr addr, uint64_t *value, unsigned size,
		unsigned shift, uint64_t mask, MemTxAttrs attrs) {
	uint64_t tmp;
	tmp = mr->ops->old_mmio.read[ctz32(size)](mr->opaque,addr);
	if (mr->subpage)
	{
		trace_memory_region_subpage_read(get_cpu_index(),mr,addr,tmp,size);
	}
	else if (mr == &io_mem_notdirty)
	{
		/*
		 * Accesses to code which has previously been translated into a TB show up
		 * in the MMIO path, as accesses to the io_mem_notdirty
		 */
		trace_memory_region_tb_read(get_cpu_index(),addr,tmp,size);
	}
	else if (TRACE_MEMORY_REGION_OPS_READ_ENABLED)
	{
		hwaddr abs_addr = memory_region_to_absolute_addr(mr, addr);
		trace_memory_region_ops_read(get_cpu_index(),mr,abs_addr,tmp,size);
	}
	*value |= (tmp & mask) << shift;
	return MEMTX_OK;
}

static MemTxResult memory_region_read_accessor
(MemoryRegion *mr, hwaddr addr, uint64_t *value, unsigned size,
		unsigned shift, uint64_t mask, MemTxAttrs attrs) {
	uint64_t tmp;
	tmp = mr->ops->read(mr->opaque,addr,size);
	if (mr->subpage)
	{
		trace_memory_region_subpage_read(get_cpu_index(),mr,addr,tmp,size);
	}
	else if (mr == &io_mem_notdirty)
	{
			/*
			 * Accesses to code which has previously been translated into a TB show up
			 * in the MMIO path, as accesses to the io_mem_notdirty
			 */
		trace_memory_region_tb_read(get_cpu_index(),addr,tmp,size);
	}
    else if (TRACE_MEMORY_REGION_OPS_READ_ENABLED)
	{
		hwaddr abs_addr = memory_region_to_absolute_addr(mr, addr);
		trace_memory_region_ops_read(get_cpu_index(),mr,abs_addr,tmp,size);
	}
	*value |= (tmp & mask) << shift;
	return MEMTX_OK;
}

static MemTxResult memory_region_read_with_attrs_accessor
(MemoryRegion *mr, hwaddr addr, uint64_t *value, unsigned size,
		unsigned shift, uint64_t mask, MemTxAttrs attrs) {
	uint64_t tmp = 0;
	MemTxResult r;
	r = mr->ops->read_with_attrs(mr->opaque, addr, &tmp, size, attrs);
	if (mr->subpage)
	{
		trace_memory_region_subpage_read(get_cpu_index(), mr, addr, tmp, size);
	}
	else if (mr == &io_mem_notdirty)
	{
		trace_memory_region_tb_read(get_cpu_index(),addr, tmp, size);
	}
	else if (TRACE_MEMORY_REGION_OPS_READ_ENABLED)
	{
		hwaddr abs_addr = memory_region_to_absolute_addr(mr, addr);
		trace_memory_region_ops_read(get_cpu_index(), mr, abs_addr, tmp,size);
	}
	*value |= (tmp & mask) << shift;
	return r;
}

/*-------------------------------------------------------------------------------------*/

static MemTxResult memory_region_oldmmio_write_accessor
(MemoryRegion *mr, hwaddr addr, uint64_t *value, unsigned size,
		unsigned shift, uint64_t mask, MemTxAttrs attrs) {
	uint64_t tmp;
	tmp = (*value >> shift) & mask;
	if (mr->subpage)
	{
		trace_memory_region_subpage_write(get_cpu_index(),mr,addr,tmp,size);
	}
	else if (mr == &io_mem_notdirty)
	{
		trace_memory_region_tb_write(get_cpu_index(), addr, tmp, size);
	}
	else if (TRACE_MEMORY_REGION_OPS_WRITE_ENABLED)
	{
		hwaddr abs_addr = memory_region_to_absolute_addr(mr,addr);
		trace_memory_region_ops_write(get_cpu_index(),mr,abs_addr,tmp,size);
	}
	mr->ops->old_mmio.write[ctz32(size)](mr->opaque,addr,tmp);
	return MEMTX_OK;
}

static MemTxResult memory_region_write_accessor
(MemoryRegion *mr, hwaddr addr, uint64_t *value, unsigned size,
		unsigned shift, uint64_t mask, MemTxAttrs attrs) {
	uint64_t tmp;
	tmp = (*value >> shift) & mask;
	if (mr->subpage)
	{
		trace_memory_region_subpage_write(get_cpu_index(),mr,addr,tmp,size);
	}
	else if (mr == &io_mem_notdirty)
	{
		trace_memory_region_tb_write(get_cpu_index(), addr, tmp, size);
	}
	else if (TRACE_MEMORY_REGION_OPS_WRITE_ENABLED)
	{
		hwaddr abs_addr = memory_region_to_absolute_addr(mr,addr);
		trace_memory_region_ops_write(get_cpu_index(),mr,abs_addr,tmp,size);
	}
	mr->ops->write(mr->opaque,addr,tmp,size);
	return MEMTX_OK;
}

static MemTxResult memory_region_write_with_attrs_accessor
(MemoryRegion *mr, hwaddr addr, uint64_t *value, unsigned size,
		unsigned shift, uint64_t mask, MemTxAttrs attrs) {
	uint64_t tmp;
	tmp = (*value >> shift) & mask;
	if (mr->subpage)
	{
		trace_memory_region_subpage_write(get_cpu_index(),mr,addr,tmp,size);
	}
	else if (mr == &io_mem_notdirty)
	{
		trace_memory_region_tb_write(get_cpu_index(), addr, tmp, size);
	}
	else if (TRACE_MEMORY_REGION_OPS_WRITE_ENABLED)
	{
		hwaddr abs_addr = memory_region_to_absolute_addr(mr,addr);
		trace_memory_region_ops_write(get_cpu_index(),mr,abs_addr,tmp,size);
	}
	return mr->ops->write_with_attrs(mr->opaque, addr, tmp, size, attrs);
}

