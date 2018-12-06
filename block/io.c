/*
 * io.c
 * https://github.com/intel/nemu/blob/topic/virt-x86/block/io.c
 *
 *  Created on: Nov 27, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#include "qemu/osdep.h"
#include "sysemu/block-backend.h"
#include "block/aio-wait.h"
#include "block/blockjob.h"
#include "block/blockjob_int.h"
#include "block/block_int.h"
#include "qemu/cutils.h"
#include "qapi/error.h"
#include "qemu/error-report.h"

#define NOT_DONE 0x7fffffff

/*Maximum bounce buffer for copy-on-read and write zeroes, in bytes*/
#define MAX_BOUNCE_BUFFER (32768 << BDRV_SECTOR_BITS)
static AioWait drain_all_aio_wait;

static void bdrv_parent_cb_resize(BlockDriverState *bs);
static int coroutine_fn bdrv_co_do_pwrite_zeroes(BlockDriverState *bs, int offset, int bytes,
                                                 BdrvRequestFlags flags);

void bdrv_parent_drained_begin(BlockDriverState* bs, BdrvChild *ignore, bool ignore_bds_parents) 
{
    BdrvChild *c;
    BdrvChild *next;

    QLIST_FOREACH_SAFE(c,&bs->parent, next_parent, next) {
        if (c == ignore || (ignore_bds_parents && c->role->parent_is_bds)) 
        {
            continue;
        }
        bdrv_parent_drained_begin_single(c, false);
    } 
} 

void bdrv_parent_drained_end(BlockDriverState *bs, BdrvChild *ignore, bool ignore_bds_parents)
{
    BdrvChild *c;
    BdrvChild *next;

    QLIST_FOREACH_SAFE(c, &bs->parent, next_parent, next) {
        if (c == ignore || (ignore_bds_parents && c->role->parent_is_bds)) {
            continue;
        }
        if (c->role->drained_end)
        {
            c->role->drained_end(c);
        }
    }
}

static bool bdrv_parent_drained_poll_single(BdrvChild *c) 
{
    if (c->role->drained_poll)
    {
        return c->role->drained_poll(c);
    }
    return false;
}

static bool bdrv_parent_drained_poll(BlockDriverState *bs, BdrvChild *ignore, 
                                     bool ignore_bds_parents)
{
    BdrvChild *c;
    BdrvChild *next;
    bool busy = false;

    QLIST_FOREACH_SAFE(c, &bs->parents, next_parent, next) {
        if (c == ignore || (ignore_bds_parents && c->role->parent_is_bds)) {
            continue;
        }
        busy |= bdrv_parent_drained_poll_single(c);
    }
    return busy;
}                                    

void bdrv_parent_drained_begin_single(BdrvChild *c, bool poll)
{
    if (c->role->drained_begin)
    {
        c->role->drained_begin(c);
    }
    if (poll)
    {
        BDRV_POLL_WHILE(c->bs, bdrv_parent_drained_poll_single(c));
    }
}

static void bdrv_merge_limits(BlockLimits *dst, const BlockLimits *src) 
{
    dst->opt_transfer = MAX(dst->opt_transfer, src->opt_transfer);
    dst->max_transfer = MIN_NON_ZERO(dst->max_transfer, src->max_transfer);
    dst->opt_mem_aligment = MAX(dst->opt_mem_aligment, src->alignment);
    dst->min_mem_aligment = MAX(dst->min_mem_aligment, src->min_mem_aligment);
    dst->max_iov = MIN_NON_ZERO(dst->max_iov, src->max_iov);
}

void bdrv_refresh_limits(BlockDriverState *bs, Error **errp) 
{
    BlockDriver *drv = bs->drv;
    Error *local_err = NULL;
    memset(&bs->bl,0,sizeof(bs->bl));
    if (!drv)
    {
        return;
    }
    bs->bl.request_aligment = (drv->bdrv_co_preadv || 
                               drv->bdrv_aio_preadv) ? 1 : 512;

    if (bs->file)
    {
        bdrv_refresh_limits (bs->file->bs, &local_err);
        if (local_err)
        {
            error_propagate(errp, local_err);
            return;
        }
        bdrv_merge_limits(&bs->bl, &bs->file->bs->bl);
    }                           
    else
    {
        bs->bl.min_mem_aligment = 512;
        bs->bl.opt_mem_aligment = getpagesize();
        bs->bl.max_iov = IOV_MAX;
    }

    if (bs->backing)
    {
        bdrv_refresh_limits(bs->backing_bs, &local_err);
        if (local_err)
        {
            error_propagate(errp, local_err);
            return;
        }
        bdrv_merge_limits(&bs->bl, &bs->backing->bs->bl);
    }
    if (drv->bdrv_refresh_limits)
    {
        drv->bdrv_refresh_limits(bs, errp);
    }
}

/**
 * The copy-on-read flag is actually a reference count so multiple users may
 * use the feature without worrying about clobbering its previous state.
 * Copy-on-read stays enabled until all users have called to disable it.
 */
void bdrv_enable_copy_on_read(BlockDriverState *bs)
{
    atomic_inc(&bs->copy_on_read);
}

void bdrv_disable_copy_on_read(BlockDriverState *bs)
{
    int old = atomic_fetch_dec(&bs->copy_on_read);
    assert(old >= 1);
}

typedef struct {
    Coroutine *co;
    BlockDriverState *bs;
    bool done;
    bool begin;
    bool recursive;
    bool poll;
    BdrvChild *parent;
    bool ignore_bds_parents;
} BdrvCoDrainData;

static void coroutine_fn bdrv_drain_invoke_entry(void *opaque) 
{
    BdrvCoDrainData *data = opaque;
    BlockDriverState *bs = data->bs;

    if (data->begin)
    {
        bs->drv->bdrv_co_drain_begin(bs);
    }
    else
    {
        bs->drv->bdrv_co_drain_end(bs);
    }

    atomic_mb_set(&data->done, true);
    bdrv_dec_in_flight(bs);

    if (data->begin)
    {
        g_free(data);
    }
}

static void bdrv_drain_invoke(BlockDriverState *bs, bool begin) 
{
    BdrvCoDrainData *data;
    if (!bs->drv || (begin && !bs->drv->bdrv_co_drain_begin) || 
         (!begin && !bs->bdrv_co_drain_end)) 
    {
        return;
    }

    data = g_new(BdrvCoDrainData, 1);
    *data = (BdrvCoDrainData) {
        .bs = bs,
        .done = false,
        .begin = begin
    };

    bdrv_inc_in_flight(bs);
    data->co = qemu_coroutine_create(bdrv_drain_invoke_entry, data);
    aio_co_schedule(bdrv_get_aio_context(bs), data->co);

    if (!begin)
    {
        BDRV_POLL_WHILE(bs, !data->done);
        g_free(data);
    }
}
