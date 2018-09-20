/*
 * cpus.c
 *
 *  Created on: Sep 18, 2018
 *      Student (coder): Tung Dang
 */

#include "qemu/osdep.h"
#include "qemu/config-file.h"
#include "qom/cpu.h"
#include "monitor/monitor.h"
#include "qapi/error.h"
#include "qapi/qmp/qerror.h"
#include "qemu/error-report.h"
#include "sysemu/sysemu.h"
#include "sysemu/block-backend.h"
#include "exec/gdbstub.h"
#include "sysemu/dma.h"
#include "sysemu/hw_accel.h"
#include "sysemu/kvm.h"
#include "sysemu/hax.h"
#include "sysemu/hvf.h"
#include "sysemu/whpx.h"
#include "exec/exec-all.h"

#include "qemu/thread.h"
#include "sysemu/cpus.h"
#include "sysemu/qtest.h"
#include "qemu/main-loop.h"
#include "qemu/option.h"
#include "qemu/bitmap.h"
#include "qemu/seqlock.h"
#include "exec/tcg-wrapper.h"
#include "hw/nmi.h"
#include "sysemu/replay.h"
#include "hw/boards.h"


