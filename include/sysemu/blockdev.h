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


#endif /* SYSEMU_BLOCKDEV_H_ */
