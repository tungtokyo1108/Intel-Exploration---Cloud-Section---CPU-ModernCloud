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
