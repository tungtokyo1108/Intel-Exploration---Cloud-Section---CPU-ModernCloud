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


