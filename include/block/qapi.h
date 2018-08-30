/*
 * qapi.h
 *
 *  Created on: Aug 30, 2018
 *      Author: tungdang
 */

#ifndef BLOCK_QAPI_H_
#define BLOCK_QAPI_H_

#include "block/block.h"
#include "block/snapshot.h"

BlockDriverInfo *bdrv_block_device_info(BlockBackend *blk, BlockDriverState *bs, Error **errp);
int bdrv_query_snapshot_info_list(BlockDriverState *bs, QEMUSnapshotInfo **p_list, Error **errp);
void bdrv_query_image_info(BlockDriverState *bs, ImageInfo **p_info, Error **errp);
void bdrv_snapshot_dump(fprintf_function func_fprintf, void *f, QEMUSnapshotInfo *sn);
void bdrv_image_info_specific_dump(fprintf_function func_fprintf, void *f, ImageInfoSpecific *info_spec);
void bdrv_image_info_dump(fprintf_function func_fprintf, void *f, ImageInfo *info);

#endif /* BLOCK_QAPI_H_ */
