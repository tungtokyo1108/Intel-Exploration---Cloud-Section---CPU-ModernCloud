/*
 * snapshot.h
 *
 *  Created on: Aug 29, 2018
 *      Student (coder): Tung Dang
 */

#ifndef BLOCK_SNAPSHOT_H_
#define BLOCK_SNAPSHOT_H_
#include "qemu/option.h"
#include "qemu/osdep.h"

#define SNAPSHOT_OPT_BASE       "snapshot."
#define SNAPSHOT_OPT_ID         "snapshot.id"
#define SNAPSHOT_OPT_NAME       "snapshot.name"

extern QemuOptsList internal_snapshot_opts;

typedef struct QEMUSnapshotInfo {
	char id_str[128];
	char name[256];
	uint64_t vm_state_size;
	uint32_t date_sec;
	uint32_t date_nsec;
	uint64_t vm_clock_nsec;
} QEMUSnapshotInfo;


int bdrv_snapshot_find(BlockDriverState *bs, QEMUSnapshotInfo *sn_info,
                       const char *name);
bool bdrv_snapshot_find_by_id_and_name(BlockDriverState *bs,
                                       const char *id,
                                       const char *name,
                                       QEMUSnapshotInfo *sn_info,
                                       Error **errp);
int bdrv_can_snapshot(BlockDriverState *bs);
int bdrv_snapshot_create(BlockDriverState *bs,
                         QEMUSnapshotInfo *sn_info);
int bdrv_snapshot_goto(BlockDriverState *bs,
                       const char *snapshot_id,
                       Error **errp);
int bdrv_snapshot_delete(BlockDriverState *bs,
                         const char *snapshot_id,
                         const char *name,
                         Error **errp);
int bdrv_snapshot_delete_by_id_or_name(BlockDriverState *bs,
                                       const char *id_or_name,
                                       Error **errp);
int bdrv_snapshot_list(BlockDriverState *bs,
                       QEMUSnapshotInfo **psn_info);
int bdrv_snapshot_load_tmp(BlockDriverState *bs,
                           const char *snapshot_id,
                           const char *name,
                           Error **errp);
int bdrv_snapshot_load_tmp_by_id_or_name(BlockDriverState *bs,
                                         const char *id_or_name, Error **errp);


int bdrv_snapshot_find(BlockDriverState *bs, QEMUSnapshotInfo *sn_info,
                       const char *name);
bool bdrv_snapshot_find_by_id_and_name(BlockDriverState *bs,
                                       const char *id,
                                       const char *name,
                                       QEMUSnapshotInfo *sn_info,
                                       Error **errp);
int bdrv_can_snapshot(BlockDriverState *bs);
int bdrv_snapshot_create(BlockDriverState *bs,
                         QEMUSnapshotInfo *sn_info);
int bdrv_snapshot_goto(BlockDriverState *bs,
                       const char *snapshot_id,
                       Error **errp);
int bdrv_snapshot_delete(BlockDriverState *bs,
                         const char *snapshot_id,
                         const char *name,
                         Error **errp);
int bdrv_snapshot_delete_by_id_or_name(BlockDriverState *bs,
                                       const char *id_or_name,
                                       Error **errp);
int bdrv_snapshot_list(BlockDriverState *bs,
                       QEMUSnapshotInfo **psn_info);
int bdrv_snapshot_load_tmp(BlockDriverState *bs,
                           const char *snapshot_id,
                           const char *name,
                           Error **errp);
int bdrv_snapshot_load_tmp_by_id_or_name(BlockDriverState *bs,
                                         const char *id_or_name, Error **errp);
BlockDriverState *bdrv_all_find_vmstate_bs(void);

#endif /* BLOCK_SNAPSHOT_H_ */
