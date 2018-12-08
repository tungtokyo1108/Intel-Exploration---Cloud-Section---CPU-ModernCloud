/*
 * cpu.c
 * https://github.com/intel/nemu/blob/topic/virt-x86/qom/cpu.c
 *
 *  Created on: Dec 7, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qom/cpu.h"
#include "sysemu/hw_accel.h"
#include "qemu/notify.h"
#include "qemu/log.h"
#include "exec/log.h"
#include "exec/cpu-common.h"
#include "qemu/error-report.h"
#include "sysemu/sysemu.h"
#include "hw/boards.h"
#include "hw/qdev-properties.h"
