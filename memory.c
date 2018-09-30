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

static MemTxResult access_with_adjusted_size
(hwaddr addr, uint64_t *value, unsigned size, unsigned access_size_min, unsigned access_size_max,
 MemTxResult (*access_fn)(MemoryRegion *mr, hwaddr addr, uint64_t *value, unsigned size, unsigned shift,
		 uint64_t mask, MemTxAttrs attrs), MemoryRegion *mr, MemTxAttrs attrs) {
	uint64_t access_mask;
	unsigned access_size;
	unsigned i;
	MemTxResult r = MEMTX_OK;

	if (!access_size_min)
	{
		access_size_min = 1;
	}
	if (!access_size_max)
	{
		access_size_max = 4;
	}
	access_size = MAX(MIN(size, access_size_max), access_size_min);
	access_mask = -1ULL >> (64 - access_size * 8);
	if (memory_region_big_endian(mr))
	{
		for (i=0; i < size; i += access_size)
		{
			r |= access_fn(mr, addr + 1, value, access_size,
					(size - access_size - 1) * 8, access_mask, attrs);
		}
	}
	else
	{
		for (i=0; i < size; i += access_size)
		{
			r |= access_fn(mr, addr + 1, value, access_size, i*8, access_mask, attrs);
		}
	}
	return r;
}

static AddressSpace *memory_region_to_address_space(MemoryRegion *mr) {
	AddressSpace *as;
	while (mr->container)
	{
		mr = mr->container;
	}
	QTAILQ_FOREACH(as, &address_space, address_spaces_link) {
		if (mr == as->root) {
			return as;
		}
	}
	return NULL;
}

static void render_memory_region(FlatView *view, MemoryRegion *mr, Int128 base,
		                         AddrRange clip, bool readonly) {
	MemoryRegion *subregion;
	unsigned i;
	hwaddr offset_in_region;
	Int128 remain;
	Int128 now;
	FlatRange fr;
	AddrRange tmp;

	if (!mr->enabled) {
		return;
	}

	int128_addto(&base, int128_make64(mr->addr));
	readonly |= mr->readonly;
	tmp = addrrange_make(base, mr->size);
	if (!addrrange_intersects(tmp,clip))
	{
		return;
	}
	clip = addrrange_intersection(tmp,clip);
	if (mr->alias)
	{
		int128_subfrom(&base, int128_make64(mr->alias->addr));
		int128_subfrom(&base, int128_make64(mr->alias_offset));
		render_memory_region(view, mr->alias, base, clip, readonly);
		return;
	}
	QTAILQ_FOREACH(subregion, &mr->subregions, subregion_link) {
		render_memory_region(view, subregion, base, clip, readonly);
	}

	if (!mr->terminates)
	{
		return;
	}

	offset_in_region = int128_get64(int128_sub(clip.start,base));
	base = clip.start;
	remain = clip.size;

	fr.mr = mr;
	fr.dirty_log_mask = memory_region_get_dirty_log_mask(mr);
	fr.romd_mode = mr->romd_mode;
	fr.readonly = readonly;

	for (i=0; i < view->nr && int128_nz(remain); ++i)
	{
		if (int128_ge(base, addrrange_end(view->ranges[i].addr))) {
			continue;
		}
		if (int128_lt(base, view->ranges[i].addr.start))
		{
			now = int128_min(remain, int128_sub(view->ranges[i].addr.start, base));
			fr.offset_in_region = offset_in_region;
			fr.addr = addrrange_make(base,now);
			flatview_insert(view,i,&fr);
			++i;
			int128_addto(&base, now);
			offset_in_region += int128_get64(now);
			int128_subfrom(&remain,now);
		}
		now = int128_sub(int128_min(int128_add(base,remain), addrrange_end(view->ranges[i].addr)),base);
		int128_addto(&base,now);
		offset_in_region += int128_get64(now);
		int128_subfrom(&remain,now);
	}
	if (int128_nz(remain))
	{
		fr.offset_in_region = offset_in_region;
		fr.addr = addrrange_make(base,remain);
		flatview_insert(view,i,&fr);
	}
}

static MemoryRegion *memory_region_get_flatview_root(MemoryRegion *mr) {
	while (mr->enabled)
	{
		if (mr->alias)
		{
			if (!mr->alias_offset && int128_ge(mr->size, mr->alias->size))
			{
				// Use it as the "real" root, so that we can share more Flatview
				mr = mr->alias;
				continue;
			}
		}
		else if (!mr->terminates)
		{
			unsigned int found = 0;
			MemoryRegion *child, *next = NULL;
			QTAILQ_FOREACH(child,&mr->subregions,subregions_link) {
				if (child->enabled) {
					if (++found > 1) {
						next = NULL;
						break;
					}
					if (!child->addr && int128_ge(mr->size, child->size)) {
						next = child;
					}
				}
			}
			if (found == 0) {
				return NULL;
			}
			if (next) {
				mr = next;
				continue;
			}
		}
		return mr;
	}
	return NULL;
}

/* Render a memory topology into a list of disjoint absolute ranges*/
static FlatView *generate_memory_topology(MemoryRegion *mr) {
	int i;
	FlatView *view;
	view = flatview_new(mr);

	if (mr)
	{
		render_memory_region(view,mr,int128_zero(),
				addrrange_make(int128_zero(), int128_2_64()), false);
	}
	flatview_simplify(view);
	view->dispatch = address_space_dispatch_new(view);
	for (i=0; i < view->nr; i++)
	{
		MemoryRegionSection mrs = section_from_flat_range(&view->ranges[i],view);
		flatview_add_to_dispatch(view, &mrs);
	}
	address_space_dispatch_compact(view->dispatch);
	g_hash_table_replace(flat_views,mr,view);
	return view;
}

static void address_space_add_del_ioeventfds(AddressSpace *as, MemoryRegionIoeventfd *fds_new,
		unsigned fds_new_nb, MemoryRegionIoeventfd *fds_old, unsigned fds_old_nb) {
	unsigned iold, inew;
	MemoryRegionIoeventfd *fd;
	MemoryRegionSection section;

	/*
	 * Generate a symmetric difference of the old and new fd sets,
	 * adding and deleting as necessary
	 */

	iold = inew = 0;
	while (iold < fds_old_nb || inew < fds_new_nb)
	{
		if (iold < fds_old_nb && (inew == fds_new_nb
				|| memory_region_ioeventfd_before(&fds_old[iold], &fds_new[inew]))) {
			fd = &fds_old[iold];
			section = (MemoryRegionSection) {
				.fv = address_space_to_flatview(as),
				.offset_within_address_space = int128_get64(fd->addr.start),
				.size = fd->addr.size,
			};
			MEMORY_LISTENER_CALL(as, eventfd_del, Forward, &section,
					             fd->match_data, fd->data, fd->e);
			++iold;
		}
		else if (inew < fds_new_nb
				&& (iold == fds_old_nb
					|| memory_region_ioeventfd_before(&fds_new[inew],&fds_old[iold])))
		{
			fd = &fds_new[inew];
			section = (MemoryRegionSection) {
				.fv = address_space_to_flatview(as),
				.offset_within_address_space = int128_get64(fd->addr.start),
				.size = fd->addr.size,
			};
			MEMORY_LISTENER_CALL(as, eventfd_add, Reverse, &section,
					fd->match_data, fd->data, fd->e);
			++inew;
		}
		else
		{
			++iold;
			++inew;
		}
	}
}

FlatView *address_space_get_flatview(AddressSpace *as) {
	FlatView *view;
	rcu_read_lock();
	do {
		view = address_space_to_flatview(as);
	} while (!flatview_ref(view));
	rcu_read_unlock();
	return view;
}

static void address_space_update_ioeventfds(AddressSpace *as) {
	FlatView *view;
	FlatRange *fr;
	unsigned ioeventfd_nb = 0;
	MemoryRegionIoeventfd *ioeventfds = NULL;
	AddrRange tmp;
	unsigned i;
	view = address_space_get_flatview(as);
	FOR_EACH_FLAT_RANGE(fr, view) {
		for (i=0; i < fr->mr->ioventfd_nb; ++i) {
			tmp = addrrange_shift(fr->mr->ioeventfds[i].addr,
					int128_sub(fr->addr.start,
						       int128_make64(fr->offset_in_region)));
			if (addrrange_intersects(fr->addr,tmp)) {
				++ioeventfd_nb;
				ioeventfds = g_realloc(ioeventfds,
						ioeventfd_nb * sizeof(*ioeventfds));
				ioeventfds[ioeventfd_nb - 1] = fr->mr->ioeventfds[i];
				ioeventfds[ioeventfd_nb - 1].addr = tmp;
			}
		}
	}

	address_space_add_del_ioeventfds(as, ioeventfds, ioeventfd_nb,
			                         as->ioeventfds, as->ioeventfd_nb);
	g_free(as->ioeventfds);
	as->ioeventfds = ioeventfds;
	as->ioeventfd_nb = ioeventfd_nb;
	flatview_unref(view);
}

static void address_space_update_topology_pass(AddressSpace *as, const FlatView *old_view,
		                                       const FlatView *new_view, bool adding) {
	unsigned iold, inew;
	FlatRange *frold, *frnew;

	/*
	 * Generate a symmetric different of the old and new memory map
	 * Kill ranges in the old map and instantiate in the new map
	 */

	iold = inew = 0;
	while (iold < old_view->nr || inew < new_view->nr)
	{
		if (iold < old_view->nr)
		{
			frold = &old_view->ranges[iold];
		}
		else
		{
			frold = NULL;
		}
		if (inew < new_view->nr)
		{
			frnew = &new_view->ranges[inew];
		}
		else
		{
			frnew = NULL;
		}

		if (frold
			&& (!frnew || int128_lt(frold->addr.start, frnew->addr.start)
			           || (int128_eq(frold->addr.start, frnew->addr.start)
				           && !flatrange_equal(frold,frnew)))) {
			if (!adding)
			{
				MEMORY_LISTENER_UPDATE_REGION(frold,as,Reverse,region_del);
			}
			++iold;
		}
		else if (frold && frnew && flatrrange_equal(frold,frnew)) {
			if (adding)
			{
				MEMORY_LISTENER_UPDATE_REGION(frnew,as,Forward,region_nop);
				if (frnew->dirty_log_mask & ~frold->dirty_log_mask) {
					MEMORY_LISTENER_UPDATE_REGION(frnew,as,Forward,log_start,
							                      frold->dirty_log_mask,
												  frnew->dirty_log_mask);
				}
				if (frold->dirty_log_mask & ~frnew->dirty_log_mask) {
					MEMORY_LISTENER_UPDATE_REGION(frnew,as,Reverse,log_stop,
											      frold->dirty_log_mask,
												  frnew->dirty_log_mask);
				}
			}
			++iold;
			++inew;
		}
		else
		{
			if (adding)
			{
				MEMORY_LISTENER_UPDATE_REGION(frnew,as,Forward,region_add);
			}
			++inew;
		}
	}
}
