/*
 * block_int.h
 *
 *  Created on: Aug 29, 2018
 *      Student (coder): Tung Dang
 */

#ifndef BLOCK_BLOCK_INT_H_
#define BLOCK_BLOCK_INT_H_

#include "block/accounting.h"
#include "block/block.h"
#include "block/aio-wait.h"
#include "qemu/queue.h"
#include "qemu/coroutine.h"
#include "qemu/stats64.h"
#include "qemu/timer.h"
#include "qemu/hbitmap.h"
#include "block/snapshot.h"
#include "qemu/main-loop.h"
#include "qemu/throttle.h"
#include "qemu/option.h"

#define BLOCK_FLAG_LAZY_REFCOUNTS   8
#define BLOCK_OPT_SIZE              "size"
#define BLOCK_OPT_ENCRYPT           "encryption"
#define BLOCK_OPT_ENCRYPT_FORMAT    "encrypt.format"
#define BLOCK_OPT_COMPAT6           "compat6"
#define BLOCK_OPT_HWVERSION         "hwversion"
#define BLOCK_OPT_BACKING_FILE      "backing_file"
#define BLOCK_OPT_BACKING_FMT       "backing_fmt"
#define BLOCK_OPT_CLUSTER_SIZE      "cluster_size"
#define BLOCK_OPT_TABLE_SIZE        "table_size"
#define BLOCK_OPT_PREALLOC          "preallocation"
#define BLOCK_OPT_SUBFMT            "subformat"
#define BLOCK_OPT_COMPAT_LEVEL      "compat"
#define BLOCK_OPT_LAZY_REFCOUNTS    "lazy_refcounts"
#define BLOCK_OPT_ADAPTER_TYPE      "adapter_type"
#define BLOCK_OPT_REDUNDANCY        "redundancy"
#define BLOCK_OPT_NOCOW             "nocow"
#define BLOCK_OPT_OBJECT_SIZE       "object_size"
#define BLOCK_OPT_REFCOUNT_BITS     "refcount_bits"
#define BLOCK_PROBE_BUF_SIZE 512

enum BdrvTrackedRequestType {
	BDRV_TRACKED_READ,
	BDRV_TRACKED_WRITE,
	BDRV_TRACKED_DISCARD,
	BDRV_TRACKED_TRUNCATE,
};

typedef struct BdrvTrackedRequest {
	BlockDriverState *bs;
	int64_t offset;
	uint64_t bytes;
	enum BdrvTrackedRequestType type;
	bool serialising;
	int64_t overlap_offset;
	uint64_t overlap_bytes;

	QLIST_ENTRY(BdrvTrackedRequest) list;
	Coroutine *co;
	CoQueue wait_queue;
	struct BdrvTrackedRequest *waiting_for;
} BdrvTrackedRequest;

struct BlockDriver {
	const char *format_name;
	int instance_size;
	bool is_filter;
	bool (*bdrv_recurse_is_first_non_filter)(BlockDriverState *bs, BlockDriverState *candidate);
	int (*bdrv_probe)(const uint8_t *buf, int buf_size, const char *filename);
	int (*bdrv_probe_device)(const char *filename);
	void (*bdrv_parse_filename)(const char *filename, QDict *option, Error **errp);
	bool bdrv_needs_filename;
	bool supports_backing;
	int (*bdrv_reopen_prepare)(BDRVReopenState *reopen_state, BlockReopenQueue *queue, Error **errp);
	void (*bdrv_reopen_commit)(BDRVReopenState *reopen_state);
	void (*bdrv_reopen_abort)(BDRVReopenState *reopen_state);
	void (*bdrv_join_options)(QDict *option, QDict *old_options);
	int (*bdrv_open)(BlockDriverState *bs, QDict *option, int flags, Error **errp);

	int (*bdrv_file_open)(BlockDriverState *bs, QDict *options, int flags, Error **errp);
	void (*bdrv_close)(BlockDriverState *bs);
	int coroutine_fn (*bdrv_co_create)(BlockdevCreateOptions *opts, Error **errp);
	int coroutine_fn (*bdrv_co_create_opts)(const char *filename, QemuOpts *opts, Error **errp);
	int (*bdrv_make_empty)(BlockDriverState *bs);
	void (*bdrv_refresh_filename)(BlockDriverState *bs, QDict *options);

	BlockAIOCB *(*bdrv_aio_preadv)(BlockDriverState *bs, uint64_t offset, uint64_t bytes, QEMUIOVector *qiov, int flags, BlockCompletionFunc *cb, void *opaque);
    BlockAIOCB *(*bdrv_aio_pwritev)(BlockDriverState *bs, uint64_t offset, uint64_t bytes, QEMUIOVector *qiov, int flags,BlockCompletionFunc *cb, void *opaque);
	BlockAIOCB *(*bdrv_aio_flush)(BlockDriverState *bs,BlockCompletionFunc *cb, void *opaque);
	BlockAIOCB *(*bdrv_aio_pdiscard)(BlockDriverState *bs, int64_t offset, int bytes, BlockCompletionFunc *cb, void *opaque);

	/**
	 * offset: position in bytes to write at
	 * bytes: number of bytes to write
	 * qiov: the buffers containing data to write
	 * flags: zero or more bits allowed by
	 *
	 * The buffer in @qiov may point directly to guest memory
	 */
	int coroutine_fn (*bdrv_co_readv)(BlockDriverState *bs,int64_t sector_num, int nb_sectors, QEMUIOVector *qiov);
	int coroutine_fn (*bdrv_co_preadv)(BlockDriverState *bs, uint64_t offset, uint64_t bytes, QEMUIOVector *qiov, int flags);
	int coroutine_fn (*bdrv_co_writev)(BlockDriverState *bs, int64_t sector_num, int nb_sectors, QEMUIOVector *qiov, int flags);
	int coroutine_fn (*bdrv_co_pwritev)(BlockDriverState *bs, uint64_t offset, uint64_t bytes, QEMUIOVector *qiov, int flags);

	int coroutine_fn (*bdrv_co_pwrite_zeros)(BlockDriverState *bs, int64_t offset, int bytes, BdrvRequestFlags flags);
	int coroutine_fn (*bdrv_co_pdiscard)(BlockDriverState *bs, int64_t offset, int bytes);


	/**
	 * Map range onto a child of @bs to copy data
	 * Perform the copy operation if @bs is the leaf and @src has the same BlockDriver
	 */
	int coroutine_fn (*bdrv_co_copy_range_from)(BlockDriverState *bs, BdrvChild *src, uint64_t offset, BdrvChild *dst, uint64_t dst_offset, uint64_t bytes,
			                                    BdrvRequestFlags read_flags, BdrvRequestFlags write_flags);
	int coroutine_fn (*bdrv_co_copy_range_to)(BlockDriverState *bs, BdrvChild *src, uint64_t src_offset, BdrvChild *dst, uint64_t dst_offset, uint64_t bytes,
				                              BdrvRequestFlags read_flags, BdrvRequestFlags write_flags);
	/**
	 * The driver should answer only according to the current layer
	 */
	int coroutine_fn (*bdrv_co_block_status)(BlockDriverState *bs, bool want_zero, int64_t offset, int64_t bytes, int64_t *pnum, int64_t *map, BlockDriverState **file);

	// Invalidate any cached meta-data
	void coroutine_fn (*brdv_co_invalidate_cache)(BlockDriverState *bs, Error **errp);
	int (*bdrv_inactivate)(BlockDriverState *bs);

	/**
	 * Flushes all data for all layers
	 * Flushes all data that was already written to the OS all the way down to the disk
	 * Flushed all internal data to the OS
	 */

	int coroutine_fn (*bdrv_co_flush_)(BlockDriverState *bs);
	int coroutine_fn (*bdrv_co_flush_to_dish)(BlockDriverState *bs);
	int coroutine_fn (*bdrv_co_flush_to_disk)(BlockDriverState *bs);

	const char *protocol_name;
	int coroutine_fn (*bdrv_co_truncate)(BlockDriverState *bs, int64_t offset,
	                                         PreallocMode prealloc, Error **errp);
	int64_t (*bdrv_getlength)(BlockDriverState *bs);
    bool has_variable_length;
	int64_t (*bdrv_get_allocated_file_size)(BlockDriverState *bs);
	BlockMeasureInfo *(*bdrv_measure)(QemuOpts *opts, BlockDriverState *in_bs, Error **errp);
	int coroutine_fn (*bdrv_co_pwritev_compressed)(BlockDriverState *bs,uint64_t offset, uint64_t bytes, QEMUIOVector *qiov);
	int (*bdrv_snapshot_create)(BlockDriverState *bs, QEMUSnapshotInfo *sn_info);
	    int (*bdrv_snapshot_goto)(BlockDriverState *bs,
	                              const char *snapshot_id);
	    int (*bdrv_snapshot_delete)(BlockDriverState *bs,
	                                const char *snapshot_id,
	                                const char *name,
	                                Error **errp);
	    int (*bdrv_snapshot_list)(BlockDriverState *bs,
	                              QEMUSnapshotInfo **psn_info);
	    int (*bdrv_snapshot_load_tmp)(BlockDriverState *bs,
	                                  const char *snapshot_id,
	                                  const char *name,
	                                  Error **errp);
	    int (*bdrv_get_info)(BlockDriverState *bs, BlockDriverInfo *bdi);
	    ImageInfoSpecific *(*bdrv_get_specific_info)(BlockDriverState *bs);

	    int coroutine_fn (*bdrv_save_vmstate)(BlockDriverState *bs,
	                                          QEMUIOVector *qiov,
	                                          int64_t pos);
	    int coroutine_fn (*bdrv_load_vmstate)(BlockDriverState *bs,
	                                          QEMUIOVector *qiov,
	                                          int64_t pos);

	    int (*bdrv_change_backing_file)(BlockDriverState *bs,
	const char *backing_file, const char *backing_fmt);

	bool (*bdrv_is_inserted)(BlockDriverState *bs);
	void (*bdrv_eject)(BlockDriverState *bs, bool eject_flags);
	void (*bdrv_lock_medium)(BlockDriverState *bs, bool locked);
	BlockAIOCB *(*bdrv_aio_ioctl)(BlockDriverState *bs, unsigned long int req, void *buf, BlockCompletionFunc *cb, void *opaque);
	int coroutine_fn (*bdrv_co_ioctl)(BlockDriverState *bs, unsigned long int req, void *buf);
	QemuOptsList *create_opts;
	int coroutine_fn (*bdrv_co_check)(BlockDriverState *bs,
	                                      BdrvCheckResult *result,
	                                      BdrvCheckMode fix);
	    int (*bdrv_amend_options)(BlockDriverState *bs, QemuOpts *opts,
	                              BlockDriverAmendStatusCB *status_cb,
	                              void *cb_opaque,
	                              Error **errp);
	    void (*bdrv_debug_event)(BlockDriverState *bs, BlkdebugEvent event);
	    int (*bdrv_debug_breakpoint)(BlockDriverState *bs, const char *event,
	        const char *tag);
	    int (*bdrv_debug_remove_breakpoint)(BlockDriverState *bs,
	        const char *tag);
	    int (*bdrv_debug_resume)(BlockDriverState *bs, const char *tag);
	    bool (*bdrv_debug_is_suspended)(BlockDriverState *bs, const char *tag);
	void (*bdrv_refresh_limits)(BlockDriverState *bs, Error **errp);
	int (*bdrv_has_zero_init)(BlockDriverState *bs);

	void (*bdrv_detach_aio_context)(BlockDriverState *bs);
	void (*bdrv_attach_aio_context)(BlockDriverState *bs, AioContext *new_context);

	// io queue for linux-aio
	void (*bdrv_io_plug)(BlockDriverState *bs);
	void (*bdrv_io_unplug)(BlockDriverState *bs);

	// try to get bs's logical and physical block size
	int (*bdrv_probe_blocksizes)(BlockDriverState *bs, BlockSizes *bsz);
	int (*bdrv_probe_geometry)(BlockDriverState *bs, HDGeometry *geo); // try to get bs's geometry

	// Implemented in the begining of a drain operation to drain and stop any internal sources of requests in the driver
	void coroutine_fn (*bdrv_co_drain_begin)(BlockDriverState *bs);
	void coroutine_fn (*bdrv_co_drain_end)(BlockDriverState *bs);
	void (*bdrv_add_child)(BlockDriverState *parent, BlockDriverState *child, Error **errp);
	void (*bdrv_del_child)(BlockDriverState *parent, BdrvChild *child, Error **errp);

	int (*bdrv_check_perm)(BlockDriverState *bs, uint64_t perm, uint64_t shared, Error **errp); // informs the block driver that a permission change is intended

	/**
	 * Called to inform the driver that set of cumulative set of used permission for @bs has change to @perm
	 * set of sharable permission to @shared
	 */
	void (*bdrv_set_perm)(BlockDriverState *bs, uint64_t perm, uint64_t shared);

	/**
	 * Called to inform the driver that after previous bdrv_check_perm() call, the permission update is not performed
	 * any preparation made for it need to be done
	 */
	void (*bdrv_abort_perm_update)(BlockDriverState *bs);

	/**
	 * return in @nperm and @nshared the permission that the driver for @bs need for its child.
	 * based on the cumulative permission requested by the parents
0	 */

	void (*bdrv_child_perm)(BlockDriverState *bs, BdrvChild *c, const BdrvChildRole *role, BlockReopenQueue *reopen_queue,
			                uint64_t parent_perm, uint64_t parent_shared, uint64_t *nperm, uint64_t *nshared);

	/**
	 * Bitmaps should be marked as "IN_USE" in the image on reopening image
	 */
	    int (*bdrv_reopen_bitmaps_rw)(BlockDriverState *bs, Error **errp);
	    bool (*bdrv_can_store_new_dirty_bitmap)(BlockDriverState *bs,
	                                            const char *name,
	                                            uint32_t granularity,
	                                            Error **errp);
	    void (*bdrv_remove_persistent_dirty_bitmap)(BlockDriverState *bs,
	                                                const char *name, Error **errp);

	void (*bdrv_register_buf)(BlockDriverState *bs, void *host, size_t size);
	void (*bdrv_unregister_buf)(BlockDriverState *bs, void *host);
	QLIST_ENTRY(BlockDriver) list;
};

typedef struct BlockLimits {
	uint32_t request_aligment;
	int32_t max_pdiscard; // maximum number of bytes that can be discarded at once
	uint32_t pdiscard_aligment; // optimal aligment for discard request in bytes
	int32_t max_pwrite_zeroes; // maximum number of bytes that can zeroized at once
	uint32_t pwrite_zeroes_aligment; // optimal aligment for write zeroes request in bytes
	uint32_t opt_transfer; // optimal transfer length in bytes
	uint32_t max_transfer; // maximul transfer length ib bytes
	size_t min_mem_aligment;
	size_t opt_mem_aligment;
	int max_iov; // maximum number of iovec elements
} BlockLimits;

typedef struct BdrvOpBlocker BdrvOpBlocker;

typedef struct BdrvAioNotifier {
	void (*attached_aio_context)(AioContext *new_context, void *opaque);
	void (*detach_aio_context)(void *opaque);
	void *opaque;
	bool deleted;
	QLIST_ENTRY(BdrvAioNotifier) list;
} BdrvAioNotifier;

typedef struct BdrvChildRole {
	bool stay_at_node;
	bool parent_is_bds;
	void (*inherit_options)(int *child_flags, QDict *child_options, int parent_flags, QDict *parent_options);
	void (*change_media)(BdrvChild *child, bool load);
	void (*resize)(BdrvChild *child);
	const char *(*get_name)(BdrvChild *child); // a name is more useful for human users
	char *(*get_parent_desc)(BdrvChild *child); // a malloced string that described the parent of the child for a human reader
	void (*drained_begin)(BdrvChild *child);
	void (*drained_end)(BdrvChild *child);
	bool (*drained_poll)(BdrvChild *child);
	void (*activate)(BdrvChild *child, Error **errp);
	int (*inactivate)(BdrvChild *child);
	void (*attach)(BdrvChild *child);
	void (*detach)(BdrvChild *child);
	// Notifier the parent that filename of its child has changed
	int (*update_filename)(BdrvChild *child, BlockDriverState *new_base, const char *filename, Error **errp);
} BdrvChildRole;

extern const BdrvChildRole child_file;
extern const BdrvChildRole child_format;
extern const BdrvChildRole child_backing;

typedef struct BdrvChild {
	BlockDriverState *bs;
	char *name;
	const BdrvChildRole *role;
	void *opaque;
	uint64_t perm;
	uint64_t shared_perm;
	QLIST_ENTRY(BdrvChild) next;
	QLIST_ENTRY(BdrvChild) next_parent;
} BdrvChild;

typedef struct BlockDriverState {
	int open_flags;
	bool read_only;
	bool encrypted;
	bool sg; // if true, the device is a dev/sg
	bool probed; // if true, format was probed rather than specified
	bool force_share; // if true, always allow all shared permission
	bool implicit; // if true, this filter node was automatically inserted
	BlockDriver *drv;
	void *opaque;
	AioContext *aio_context;
	QLIST_HEAD(, BdrvAioNotifier) aio_notifiers;
	bool walking_aio_notifier; // to make removal during iteration safe
	char filename[PATH_MAX];
	char backing_file[PATH_MAX];
	char backing_format[16];
	QDict *full_open_options;
	char exact_filename[PATH_MAX];
	BdrvChild *backing;
	BdrvChild *file;
	BlockLimits bl;
	unsigned int supported_write_flags;
	unsigned int supported_zero_flags;

	char node_name[32];
	QTAILQ_ENTRY(BlockDriverState) node_list;
	QTAILQ_ENTRY(BlockDriverState) bs_list; // element of list of all BlockDriverStates
	QTAILQ_ENTRY(BlockDriverState) monitor_list;

	QLIST_HEAD(, BdrvOpBlocker) op_blockers[BLOCK_OP_TYPE_MAX];
	BlockJob *job;
	BlockDriverState *inherits_from;
	QLIST_HEAD(, BdrvChild) children;
	QLIST_HEAD(, BdrvChild) parents;

	QDict *options;
	QDict *explicit_options;
	BlockdevDetectZerosOptions detect_zeros;
	Error *backing_blocker;
	int64_t total_sectors;
	NotifierWithReturnList before_write_notifiers;

	// threshold limit for writes in bytes
	uint64_t write_threshold_offset;
	NotifierWithReturn write_threadhold_notifier;

	QemuMutex dirty_bitmap_mutex;
	QLIST_HEAD(, BdrvDirtyBitmap) dirty_bitmaps;
	Stat64 wr_highest_offset;
	int copy_on_read;
	unsigned int in_flight;
	unsigned int serialising_in_flight;

	AioWait wait;
	unsigned io_plugged;
	int enable_write_cache;
	int quiesce_counter;
	int recursive_quiesce_counter;
	unsigned int write_gen;

	CoMutex reqs_lock;
	QLIST_HEAD(, BdrvTrackedRequest) tracked_requests;
	CoQueue flush_queue;
	bool active_flush_req;
	unsigned int flushed_gen;

} BlockDriverState;

typedef struct BlockBackendRootState {
	int open_flags;
	bool read_only;
	BlockdevDetectZeroesOptions detect_zeroes;
} BlockBackendRootState;

typedef enum BlockMirrorBackingMode {
	MIRROR_SOURCE_BACKING_CHAIN,
	MIRROR_OPEN_BACKING_CHAIN,
	MIRROR_LEAVE_BACKING_CHAIN,
} BlockMirrorBackingMode;

static inline BlockDriverState *backing_bs(BlockDriverState *bs) {
	return bs->backing ? bs->backing->bs : NULL;
}

extern BlockDriver bdrv_file;
extern BlockDriver bdrv_raw;
extern BlockDriver bdrv_qcow2;

int coroutine_fn bdrv_co_preadv(BdrvChild *child,
    int64_t offset, unsigned int bytes, QEMUIOVector *qiov,
    BdrvRequestFlags flags);
int coroutine_fn bdrv_co_pwritev(BdrvChild *child,
    int64_t offset, unsigned int bytes, QEMUIOVector *qiov,
    BdrvRequestFlags flags);

extern unsigned int bdrv_drain_all_count;
void bdrv_apply_subtree_drain(BdrvChild *child, BlockDriverState *new_parent);
void bdrv_unapply_subtree_drain(BdrvChild *child, BlockDriverState *old_parent);

int get_tmp_filename(char *filename, int size);
BlockDriver *bdrv_probe_all(const uint8_t *buf, int buf_size,
                            const char *filename);
void bdrv_parse_filename_strip_prefix(const char *filename, const char *prefix, QDict *options);

void bdrv_add_before_write_notifier(BlockDriverState *bs, NotifierWithReturn *notifier);
void bdrv_detach_aio_context(BlockDriverState *bs); // to detach children from the current AioContext
void bdrv_attach_aio_context(BlockDriverState *bs, AioContext *new_context);

/**
 * If a long-running job intends to be always run in the same AioContext as a certain BDS, it may use this function to be notifier of changes
 */
void bdrv_add_aio_context_notifier(BlockDriverState *bs,
        void (*attached_aio_context)(AioContext *new_context, void *opaque), void (*detach_aio_context)(void *opaque), void *opaque);
void bdrv_remove_aio_context_notifier(BlockDriverState *bs,
        void (*aio_context_attached)(AioContext *, void *), void (*aio_context_detached)(void *), void *opaque);
void bdrv_wakeup(BlockDriverState *bs);

/**
 * Clusters that are unallocated in @bs, but allocated any image between @base and @bs will be written to @bs
 * At the end of successful streaming job, the backing file of @bs will be changed to @backing_file_str in the written image
 * and to @base in the live BlockDriveState
 */

void stream_start(const char *job_id, BlockDriverState *bs, BlockDriverState *base, const char *backing_file_str,
		          int64_t speed, BlockdevOnError on_error, Error **errp);
void commit_start(const char *job_id, BlockDriverState *bs,
                  BlockDriverState *base, BlockDriverState *top, int64_t speed,
                  BlockdevOnError on_error, const char *backing_file_str, const char *filter_node_name, Error **errp);
void commit_active_start(const char *job_id, BlockDriverState *bs,
                         BlockDriverState *base, int creation_flags,
                         int64_t speed, BlockdevOnError on_error,
                         const char *filter_node_name,
                         BlockCompletionFunc *cb, void *opaque, bool auto_complete, Error **errp);
/**
 * Start a mirroring operation on @bs.
 * Clusters that are allocated in @bs will be written to @target until job is cancelled or completed
 * At the end of a successful mirroring job, @bs will be switching to read from @target
 */

void mirror_start(const char *job_id, BlockDriverState *bs,
                  BlockDriverState *target, const char *replaces,
                  int64_t speed, uint32_t granularity, int64_t buf_size,
                  MirrorSyncMode mode, BlockMirrorBackingMode backing_mode,
                  BlockdevOnError on_source_error,
                  BlockdevOnError on_target_error,
                  bool unmap, const char *filter_node_name, MirrorCopyMode copy_mode, Error **errp);

BlockJob *backup_job_create(const char *job_id, BlockDriverState *bs,
                            BlockDriverState *target, int64_t speed,
                            MirrorSyncMode sync_mode,
                            BdrvDirtyBitmap *sync_bitmap,
                            bool compress,
                            BlockdevOnError on_source_error,
                            BlockdevOnError on_target_error,
                            int creation_flags,
                            BlockCompletionFunc *cb, void *opaque, JobTxn *txn, Error **errp);

void hmp_drive_add_node(Monitor *mon, const char *optstr);
BdrvChild *bdrv_root_attach_child(BlockDriverState *child_bs,
                                  const char *child_name,
                                  const BdrvChildRole *child_role,
                                  uint64_t perm, uint64_t shared_perm, void *opaque, Error **errp);
void bdrv_root_unref_child(BdrvChild *child);
int bdrv_child_try_set_perm(BdrvChild *c, uint64_t perm, uint64_t shared, Error **errp);

/**
 * Default implementation
 */
void bdrv_filter_default_perms(BlockDriverState *bs, BdrvChild *c,
                               const BdrvChildRole *role,
                               BlockReopenQueue *reopen_queue,
                               uint64_t perm, uint64_t shared,uint64_t *nperm, uint64_t *nshared);
void bdrv_format_default_perms(BlockDriverState *bs, BdrvChild *c,
                               const BdrvChildRole *role,
                               BlockReopenQueue *reopen_queue,
                               uint64_t perm, uint64_t shared, uint64_t *nperm, uint64_t *nshared);
int coroutine_fn bdrv_co_block_status_from_file(BlockDriverState *bs,
                                                bool want_zero,
                                                int64_t offset,
                                                int64_t bytes,
                                                int64_t *pnum,
                                                int64_t *map, BlockDriverState **file);
int coroutine_fn bdrv_co_block_status_from_backing(BlockDriverState *bs,
                                                   bool want_zero,
                                                   int64_t offset,
                                                   int64_t bytes,
                                                   int64_t *pnum,
                                                   int64_t *map, BlockDriverState **file);
const char *bdrv_get_parent_name(const BlockDriverState *bs);
void blk_dev_change_media_cb(BlockBackend *blk, bool load, Error **errp);
bool blk_dev_has_removable_media(BlockBackend *blk);
bool blk_dev_has_tray(BlockBackend *blk);
void blk_dev_eject_request(BlockBackend *blk, bool force);
bool blk_dev_is_tray_open(BlockBackend *blk);
bool blk_dev_is_medium_locked(BlockBackend *blk);
void bdrv_set_dirty(BlockDriverState *bs, int64_t offset, int64_t bytes);
void bdrv_clear_dirty_bitmap(BdrvDirtyBitmap *bitmap, HBitmap **out);
void bdrv_undo_clear_dirty_bitmap(BdrvDirtyBitmap *bitmap, HBitmap *in);
void bdrv_inc_in_flight(BlockDriverState *bs);
void bdrv_dec_in_flight(BlockDriverState *bs);
void blockdev_close_all_bdrv_states(void);

int coroutine_fn bdrv_co_copy_range_from(BdrvChild *src, uint64_t src_offset,
                                         BdrvChild *dst, uint64_t dst_offset,
                                         uint64_t bytes,
                                         BdrvRequestFlags read_flags, BdrvRequestFlags write_flags);
int coroutine_fn bdrv_co_copy_range_to(BdrvChild *src, uint64_t src_offset,
                                       BdrvChild *dst, uint64_t dst_offset,
                                       uint64_t bytes,
                                       BdrvRequestFlags read_flags, BdrvRequestFlags write_flags);
int refresh_total_sectors(BlockDriverState *bs, int64_t hint);

#endif /* BLOCK_BLOCK_INT_H_ */
