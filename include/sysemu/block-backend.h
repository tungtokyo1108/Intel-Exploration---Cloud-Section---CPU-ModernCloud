/*
 * block-backend.h
 *
 *  Created on: Aug 31, 2018
 *      Student (coder): Tung Dang
 */

#ifndef SYSEMU_BLOCK_BACKEND_H_
#define SYSEMU_BLOCK_BACKEND_H_

#include "qemu/iov.h"
#include "block/throttle-groups.h"
#include "block/block.h"
#include "qemu/typedefs.h"

typedef struct BlockDevOps BlockDevOps;
typedef struct BlockBackendPublic BlockBackendPublic;

typedef struct BlockDevOps {
	void (*change_media_cb)(void *opaque, bool load, Error **errp); // runs when virtual media changed
	void (*eject_request_cb)(void *opaque, bool force); // an eject request is issued from the monitor, the tray is closed, and the medium is locked
	bool (*is_tray_open)(void *opaque); // Is the virtual tray open?
	bool (*is_medium_locked)(void *opaque); // Is the virtual medium locked into the device?
	bool (*resize_cb)(void *opaque); // the size changed
	void (*drained_begin)(void *opaque); // the backend received a drain request
	void (*drained_end)(void *opaque); // the backend's last drain request ends
} BlockDevOps;

typedef struct BlockBackendPublic {
	ThrottleGroupMember throttle_group_member;
} BlockBackendPublic;

BlockBackend *blk_new(uint64_t perm, uint64_t shared_perm);
BlockBackend *blk_new_open(const char *filename, const char *reference, QDict *options, int flags, Error **errp);
int blk_get_refcnt(BlockBackend *blk);
void blk_ref(BlockBackend *blk);
void blk_unref(BlockBackend *blk);
void blk_remove_all_bs(void);
const char *blk_name(const BlockBackend *blk);
BlockBackend *blk_by_name(const char *name);
BlockBackend *blk_next(BlockBackend *blk);
BlockBackend *blk_all_next(BlockBackend *blk);
bool monitor_add_blk(BlockBackend *blk, const char *name, Error **errp);
void monitor_remove_blk(BlockBackend *blk);

BlockBackendPublic *blk_get_public(BlockBackend *blk);
BlockBackend *blk_by_public(BlockBackendPublic *publics);

BlockDriverState *blk_bs(BlockBackend *blk);
void blk_remove_bs(BlockBackend *blk);
int blk_insert_bs(BlockBackend *blk, BlockDriverState *bs, Error **errp);
bool bdrv_has_blk(BlockDriverState *bs);
bool bdrv_is_root_node(BlockDriverState *bs);
int blk_set_perm(BlockBackend *blk, uint64_t perm, uint64_t shared_perm,Error **errp);
void blk_get_perm(BlockBackend *blk, uint64_t *perm, uint64_t *shared_perm);

void blk_set_allow_write_beyond_eof(BlockBackend *blk, bool allow);
void blk_iostatus_enable(BlockBackend *blk);
bool blk_iostatus_is_enabled(const BlockBackend *blk);
BlockDeviceIoStatus blk_iostatus(const BlockBackend *blk);
void blk_iostatus_disable(BlockBackend *blk);
void blk_iostatus_reset(BlockBackend *blk);
void blk_iostatus_set_err(BlockBackend *blk, int error);
int blk_attach_dev(BlockBackend *blk, DeviceState *dev);
void blk_attach_dev_legacy(BlockBackend *blk, void *dev);
void blk_detach_dev(BlockBackend *blk, void *dev);
void *blk_get_attached_dev(BlockBackend *blk);
char *blk_get_attached_dev_id(BlockBackend *blk);
BlockBackend *blk_by_dev(void *dev);
BlockBackend *blk_by_qdev_id(const char *id, Error **errp);
void blk_set_dev_ops(BlockBackend *blk, const BlockDevOps *ops, void *opaque);
int blk_pread_unthrottled(BlockBackend *blk, int64_t offset, uint8_t *buf,
                          int bytes);
int coroutine_fn blk_co_preadv(BlockBackend *blk, int64_t offset,
                               unsigned int bytes, QEMUIOVector *qiov,
                               BdrvRequestFlags flags);
int coroutine_fn blk_co_pwritev(BlockBackend *blk, int64_t offset,
                               unsigned int bytes, QEMUIOVector *qiov,
                               BdrvRequestFlags flags);
int blk_pwrite_zeroes(BlockBackend *blk, int64_t offset,
                      int bytes, BdrvRequestFlags flags);
BlockAIOCB *blk_aio_pwrite_zeroes(BlockBackend *blk, int64_t offset,
                                  int bytes, BdrvRequestFlags flags,
                                  BlockCompletionFunc *cb, void *opaque);
int blk_make_zero(BlockBackend *blk, BdrvRequestFlags flags);
int blk_pread(BlockBackend *blk, int64_t offset, void *buf, int bytes);
int blk_pwrite(BlockBackend *blk, int64_t offset, const void *buf, int bytes,
               BdrvRequestFlags flags);
int64_t blk_getlength(BlockBackend *blk);
void blk_get_geometry(BlockBackend *blk, uint64_t *nb_sectors_ptr);
int64_t blk_nb_sectors(BlockBackend *blk);
BlockAIOCB *blk_aio_preadv(BlockBackend *blk, int64_t offset,
                           QEMUIOVector *qiov, BdrvRequestFlags flags,
                           BlockCompletionFunc *cb, void *opaque);
BlockAIOCB *blk_aio_pwritev(BlockBackend *blk, int64_t offset,
                            QEMUIOVector *qiov, BdrvRequestFlags flags,
                            BlockCompletionFunc *cb, void *opaque);
BlockAIOCB *blk_aio_flush(BlockBackend *blk,
                          BlockCompletionFunc *cb, void *opaque);
BlockAIOCB *blk_aio_pdiscard(BlockBackend *blk, int64_t offset, int bytes,
                             BlockCompletionFunc *cb, void *opaque);
void blk_aio_cancel(BlockAIOCB *acb);
void blk_aio_cancel_async(BlockAIOCB *acb);
int blk_co_ioctl(BlockBackend *blk, unsigned long int req, void *buf);
int blk_ioctl(BlockBackend *blk, unsigned long int req, void *buf);
BlockAIOCB *blk_aio_ioctl(BlockBackend *blk, unsigned long int req, void *buf,
                          BlockCompletionFunc *cb, void *opaque);
int blk_co_pdiscard(BlockBackend *blk, int64_t offset, int bytes);
int blk_co_flush(BlockBackend *blk);
int blk_flush(BlockBackend *blk);
int blk_commit_all(void);
void blk_drain(BlockBackend *blk);
void blk_drain_all(void);
void blk_set_on_error(BlockBackend *blk, BlockdevOnError on_read_error,
                      BlockdevOnError on_write_error);
BlockdevOnError blk_get_on_error(BlockBackend *blk, bool is_read);
BlockErrorAction blk_get_error_action(BlockBackend *blk, bool is_read,
                                      int error);
void blk_error_action(BlockBackend *blk, BlockErrorAction action,
                      bool is_read, int error);
int blk_is_read_only(BlockBackend *blk);
int blk_is_sg(BlockBackend *blk);
int blk_enable_write_cache(BlockBackend *blk);
void blk_set_enable_write_cache(BlockBackend *blk, bool wce);
void blk_invalidate_cache(BlockBackend *blk, Error **errp);
bool blk_is_inserted(BlockBackend *blk);
bool blk_is_available(BlockBackend *blk);
void blk_lock_medium(BlockBackend *blk, bool locked);
void blk_eject(BlockBackend *blk, bool eject_flag);
int blk_get_flags(BlockBackend *blk);
uint32_t blk_get_max_transfer(BlockBackend *blk);
int blk_get_max_iov(BlockBackend *blk);
void blk_set_guest_block_size(BlockBackend *blk, int align);
void *blk_try_blockalign(BlockBackend *blk, size_t size);
void *blk_blockalign(BlockBackend *blk, size_t size);
bool blk_op_is_blocked(BlockBackend *blk, BlockOpType op, Error **errp);
void blk_op_unblock(BlockBackend *blk, BlockOpType op, Error *reason);
void blk_op_block_all(BlockBackend *blk, Error *reason);
void blk_op_unblock_all(BlockBackend *blk, Error *reason);
AioContext *blk_get_aio_context(BlockBackend *blk);
void blk_set_aio_context(BlockBackend *blk, AioContext *new_context);
void blk_add_aio_context_notifier(BlockBackend *blk,
        void (*attached_aio_context)(AioContext *new_context, void *opaque),
        void (*detach_aio_context)(void *opaque), void *opaque);
void blk_remove_aio_context_notifier(BlockBackend *blk,
                                     void (*attached_aio_context)(AioContext *,
                                                                  void *),
                                     void (*detach_aio_context)(void *),
                                     void *opaque);
void blk_add_remove_bs_notifier(BlockBackend *blk, Notifier *notify);
void blk_add_insert_bs_notifier(BlockBackend *blk, Notifier *notify);
void blk_io_plug(BlockBackend *blk);
void blk_io_unplug(BlockBackend *blk);
BlockAcctStats *blk_get_stats(BlockBackend *blk);
BlockBackendRootState *blk_get_root_state(BlockBackend *blk);
void blk_update_root_state(BlockBackend *blk);
bool blk_get_detect_zeroes_from_root_state(BlockBackend *blk);
int blk_get_open_flags_from_root_state(BlockBackend *blk);

void *blk_aio_get(const AIOCBInfo *aiocb_info, BlockBackend *blk,
                  BlockCompletionFunc *cb, void *opaque);
int coroutine_fn blk_co_pwrite_zeroes(BlockBackend *blk, int64_t offset,
                                      int bytes, BdrvRequestFlags flags);
int blk_pwrite_compressed(BlockBackend *blk, int64_t offset, const void *buf,
                          int bytes);
int blk_truncate(BlockBackend *blk, int64_t offset, PreallocMode prealloc,
                 Error **errp);
int blk_pdiscard(BlockBackend *blk, int64_t offset, int bytes);
int blk_save_vmstate(BlockBackend *blk, const uint8_t *buf,
                     int64_t pos, int size);
int blk_load_vmstate(BlockBackend *blk, uint8_t *buf, int64_t pos, int size);
int blk_probe_blocksizes(BlockBackend *blk, BlockSizes *bsz);
int blk_probe_geometry(BlockBackend *blk, HDGeometry *geo);
BlockAIOCB *blk_abort_aio_request(BlockBackend *blk,
                                  BlockCompletionFunc *cb,
                                  void *opaque, int ret);

void blk_set_io_limits(BlockBackend *blk, ThrottleConfig *cfg);
void blk_io_limits_disable(BlockBackend *blk);
void blk_io_limits_enable(BlockBackend *blk, const char *group);
void blk_io_limits_update_group(BlockBackend *blk, const char *group);
void blk_set_force_allow_inactivate(BlockBackend *blk);

void blk_register_buf(BlockBackend *blk, void *host, size_t size);
void blk_unregister_buf(BlockBackend *blk, void *host);

int coroutine_fn blk_co_copy_range(BlockBackend *blk_in, int64_t off_in,
                                   BlockBackend *blk_out, int64_t off_out,
                                   int bytes, BdrvRequestFlags read_flags, BdrvRequestFlags write_flags);


#endif /* SYSEMU_BLOCK_BACKEND_H_ */
