/*
 * blockdev.h
 *
 *  Created on: Sep 19, 2018
 *      Student (coder): Tung Dang
 */

#ifndef SYSEMU_BLOCKDEV_H_
#define SYSEMU_BLOCKDEV_H_

#include "block/block.h"
#include "qemu/queue.h"

typedef enum {
	IF_DEFAULT = -1,
	IF_NONE = 0,
	IF_IDE, IF_SCSI, IF_FLOPPY, IF_PFLASH, IF_MTD, IF_SD, IF_VIRTIO, IF_XEN,
	IF_COUNT
} BlockInterfaceType;

typedef struct DriveInfo DriveInfo;
typedef struct DriveInfo {
	const char *devaddr;
	BlockInterfaceType type;
	int bus;
	int unit;
	int auto_del;
	bool is_default;
	int media_cd;
	int cyls, heads, secs, trans;
	QemuOpts *opts;
	char *serial;
	QTAILQ_ENTRY(DriveInfo) next;
} DriveInfo;

DriveInfo *blk_legacy_dinfo(BlockBackend *blk);
DriveInfo *blk_set_lagacy_dinfo(BlockBackend *blk, DriveInfo *dinfo);
BlockBackend *blk_by_legacy_dinfo(DriveInfo *dinfo);

void override_max_devs(BlockInterfaceType type, int max_devs);

DriveInfo *drive_get(BlockInterfaceType type, int bus, int unit);
void drive_check_orphaned(void);
DriveInfo *drive_get_by_index(BlockInterfaceType type, int index);
int drive_get_max_bus(BlockInterfaceType type);
int drive_get_max_devs(BlockInterfaceType type);
DriveInfo *drive_get_next(BlockInterfaceType type);

QemuOpts *drive_def(const char *optstr);
QemuOpts *drive_add(BlockInterfaceType type, int index, const char *file, const char *optstr);

DriveInfo *drive_new(QemuOpts *arg, BlockInterfaceType block_default_type);
void hmp_commit(Monitor *mon, const QDict *qdict);
void hmp_drive_del(Monitor *mon, const QDict *qdict);

#endif /* SYSEMU_BLOCKDEV_H_ */
