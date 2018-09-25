/*
 * vl.c
 *
 *  Created on: Sep 25, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qapi/error.h"
#include "qemu-version.h"
#include "qemu/cutils.h"
#include "qemu/help_option.h"
#include "qemu/uuid.h"
#include "sysemu/secomp.h"

#ifdef CONFIG_SDL
#if defined(__APPLE__) || defined(main)
#include <SDL.h>
int qemu_main(int argc, char **argv, char **envp);
int main (int argc, char **argv) {
	return qemu_main(argc, argv, NULL);
}
#undef main
#define main qemu_main
#endif
#endif

#ifdef CONFIG_COCOA
#undef main
#define main qemu_main
#endif

#include "qemu/error-report.h"
#include "qemu/sockets.h"
#include "hw/hw.h"
#include "hw/boards.h"
#include "sysemu/accel.h"
#include "hw/usb.h"
#include "hw/isa/isa.h"
#include "hw/scsi/scsi.h"
#include "hw/display/vga.h"
#include "hw/bt.h"
#include "sysemu/watchdog.h"
#include "hw/smbios/smbios.h"
#include "hw/acpi/acpi.h"
#include "hw/xen/xen.h"
#include "hw/qdev.h"
#include "hw/loader.h"
#include "monitor/qdev.h"
#include "sysemu/bt.h"
#include "net/net.h"
#include "net/slirp.h"
#include "monitor/monitor.h"
#include "ui/console.h"
#include "ui/input.h"
#include "sysemu/sysemu.h"
#include "sysemu/numa.h"
#include "exec/gdbstub.h"
#include "qemu/timer.h"
#include "chardev/char.h"
#include "qemu/bitmap.h"
#include "qemu/log.h"
#include "sysemu/blockdev.h"
#include "hw/block/block.h"
#include "migration/misc.h"
#include "migration/snapshot.h"
#include "migration/global_state.h"
#include "sysemu/tpm.h"
#include "sysemu/dma.h"
#include "hw/audio/soundhw.h"
#include "audio/audio.h"
#include "sysemu/cpus.h"
#include "migration/colo.h"
#include "migration/postcopy-ram.h"
#include "sysemu/kvm.h"
#include "sysemu/hax.h"
#include "qemu/option.h"
#include "qemu/config-file.h"
#include "qemu-options.h"
#include "qemu/main-loop.h"
#ifdef CONFIG_VIRTFS
#include "fsdev/qemu-fsdev.h"
#endif
#include "sysemu/qtest.h"

#include "disas/disas.h"


#include "slirp/libslirp.h"

#include "trace-root.h"
#include "trace/control.h"
#include "qemu/queue.h"
#include "sysemu/arch_init.h"

#include "ui/qemu-spice.h"
#include "qapi/string-input-visitor.h"
#include "qapi/opts-visitor.h"
#include "qapi/clone-visitor.h"
#include "qom/object_interfaces.h"
#include "exec/semihost.h"
#include "crypto/init.h"
#include "sysemu/replay.h"
#include "qapi/qmp/qerror.h"
#include "sysemu/iothread.h"

