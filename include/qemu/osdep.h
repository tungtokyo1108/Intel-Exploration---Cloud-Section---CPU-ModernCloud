/*
 * osdep.h
 *
 *  Created on: Aug 21, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_OSDEP_H_
#define QEMU_OSDEP_H_

#include "qemu/compiler.h"

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <inttypes.h>
#include <limits.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <assert.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/wait.h>
#include "qemu/typedefs.h"

#ifdef NDEBUG
#error building with NDEBUG is not supported
#endif
#ifdef G_DISABLE_ASSERT
#error building with G_DISABLE_ASSERT is not supported
#endif

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif
#ifndef O_BINARY
#define O_BINARY 0
#endif
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif
#ifndef ENOMEDIUM
#define ENOMEDIUM ENODEV
#endif
#if !defined(ENOTSUP)
#define ENOTSUP 4096
#endif
#if !defined(ECANCELED)
#define ECANCELED 4097
#endif
#if !defined(EMEDIUMTYPE)
#define EMEDIUMTYPE 4098
#endif
#if !defined(ESHUTDOWN)
#define ESHUTDOWN 4099
#endif

#define TYPE_SIGNED(t) (!((t)0 < (t)-1))
#define TYPE_WIDTH(t) (sizeof(t) * CHAR_BIT)
#define TYPE_MAXIMUM(t) ((t) (!TYPE_SIGNED(t) ? (t)-1 : ((((t)1 << (TYPE_WIDTH(t) - 2)) -1) * 2 + 1)))

#ifndef TIME_MAX
#define TIME_MAX TYPE_MAXIMUM(time_t)
#endif

#if UINTPTR_MAX == UINT32_MAX
#define HOST_LONG_BITS 32
#elif UINTPTR_MAX == UINT64_MAX
#define HOST_LONG_BITS 64
#else
#error Unknown pointer size
#endif

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN_NON_ZERO
#define MIN_NON_ZERO(a,b) ((a) == 0 ? (b) : ((b) == 0 ? (a) : (MIN(a,b))))
#endif

#define QEMU_ALIGN_DOWN(n,m) ((n) / (m) * (m))
#define QEMU_ALIGN_UP(n,m) QEMU_ALIGN_DOWN((n) + (m) - 1, (m))

#define QEMU_IS_ALIGNED(n,m) (((n) % (m)) == 0)
#define QEMU_ALIGN_PTR_DOWN(p,n) ((typeof(p))QEMU_ALIGN_DOWN((uintptr_t)(p), (n)))
#define QEMU_ALIGN_PTR_UP(p,n) ((typeof(p))QEMU_ALIGN_UP((uintptr_t)(p),(n)))
#define QEMU_PTR_IS_ALIGNED(p,n) QEMU_IS_ALIGNED((uintptr_t)(p),(n))

#ifndef ROUND_UP
#define ROUND_UP(n,p) (((n) + (d) -1) & -(0 ? (n) : (d)))
#endif

#ifndef DIV_ROUND_UP
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#endif

#define QEMU_IS_ARRAY(x) (!__builtin_types_compatible_p(typeof(x), typeof(&(x)[0])))
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) ((sizeof(x) / sizeof((x)[0])) + QEMU_BUILD_BUG_ON_ZERO(!QEMU_IS_ARRAY(x)))
#endif

void *qemu_try_memalign(size_t alignment, size_t size);
void *qemu_memalign(size_t alignment, size_t size);
void *qemu_anon_ram_alloc(size_t size, uint64_t *align, bool shared);
void qemu_vfree(void* ptr);
void qemu_anon_ram_free(void *ptr, size_t size);

#define QEMU_MADV_INVALID -1

#if defined(CONFIG_MADVISE)

#define QEMU_MADV_WILLNEED  MADV_WILLNEED
#define QEMU_MADV_DONTNEED  MADV_DONTNEED
#ifdef MADV_DONTFORK
#define QEMU_MADV_DONTFORK  MADV_DONTFORK
#else
#define QEMU_MADV_DONTFORK  QEMU_MADV_INVALID
#endif
#ifdef MADV_MERGEABLE
#define QEMU_MADV_MERGEABLE MADV_MERGEABLE
#else
#define QEMU_MADV_MERGEABLE QEMU_MADV_INVALID
#endif
#ifdef MADV_UNMERGEABLE
#define QEMU_MADV_UNMERGEABLE MADV_UNMERGEABLE
#else
#define QEMU_MADV_UNMERGEABLE QEMU_MADV_INVALID
#endif
#ifdef MADV_DODUMP
#define QEMU_MADV_DODUMP MADV_DODUMP
#else
#define QEMU_MADV_DODUMP QEMU_MADV_INVALID
#endif
#ifdef MADV_DONTDUMP
#define QEMU_MADV_DONTDUMP MADV_DONTDUMP
#else
#define QEMU_MADV_DONTDUMP QEMU_MADV_INVALID
#endif
#ifdef MADV_HUGEPAGE
#define QEMU_MADV_HUGEPAGE MADV_HUGEPAGE
#else
#define QEMU_MADV_HUGEPAGE QEMU_MADV_INVALID
#endif
#ifdef MADV_NOHUGEPAGE
#define QEMU_MADV_NOHUGEPAGE MADV_NOHUGEPAGE
#else
#define QEMU_MADV_NOHUGEPAGE QEMU_MADV_INVALID
#endif
#ifdef MADV_REMOVE
#define QEMU_MADV_REMOVE MADV_REMOVE
#else
#define QEMU_MADV_REMOVE QEMU_MADV_INVALID
#endif

#elif defined(CONFIG_POSIX_MADVISE)

#define QEMU_MADV_WILLNEED  POSIX_MADV_WILLNEED
#define QEMU_MADV_DONTNEED  POSIX_MADV_DONTNEED
#define QEMU_MADV_DONTFORK  QEMU_MADV_INVALID
#define QEMU_MADV_MERGEABLE QEMU_MADV_INVALID
#define QEMU_MADV_UNMERGEABLE QEMU_MADV_INVALID
#define QEMU_MADV_DODUMP QEMU_MADV_INVALID
#define QEMU_MADV_DONTDUMP QEMU_MADV_INVALID
#define QEMU_MADV_HUGEPAGE  QEMU_MADV_INVALID
#define QEMU_MADV_NOHUGEPAGE  QEMU_MADV_INVALID
#define QEMU_MADV_REMOVE QEMU_MADV_INVALID

#else /* no-op */

#define QEMU_MADV_WILLNEED  QEMU_MADV_INVALID
#define QEMU_MADV_DONTNEED  QEMU_MADV_INVALID
#define QEMU_MADV_DONTFORK  QEMU_MADV_INVALID
#define QEMU_MADV_MERGEABLE QEMU_MADV_INVALID
#define QEMU_MADV_UNMERGEABLE QEMU_MADV_INVALID
#define QEMU_MADV_DODUMP QEMU_MADV_INVALID
#define QEMU_MADV_DONTDUMP QEMU_MADV_INVALID
#define QEMU_MADV_HUGEPAGE  QEMU_MADV_INVALID
#define QEMU_MADV_NOHUGEPAGE  QEMU_MADV_INVALID
#define QEMU_MADV_REMOVE QEMU_MADV_INVALID

#endif

#define HAVE_CHARDEV_SERIAL 1
#define HAVE_CHARDEV_PARPORT 1

#ifndef BUS_MCEERR_AR
#define BUS_MCEERR_AR 4
#endif

#ifndef BUS_MCEERR_AD
#define BUS_MCEERR_AD 5
#endif

#if defined(__linux__) && (defined(__x86_64__) || defined(__arm__) || defined(__aarch64__))
#define QEMU_VMALLOC_ALIGN (512 * 4096)
#else
#define QEMU_VMALLOC_ALIGN getpagesize()
#endif

struct qemu_signalfd_siginfo {
    uint32_t ssi_signo;   /* Signal number */
    int32_t  ssi_errno;   /* Error number (unused) */
    int32_t  ssi_code;    /* Signal code */
    uint32_t ssi_pid;     /* PID of sender */
    uint32_t ssi_uid;     /* Real UID of sender */
    int32_t  ssi_fd;      /* File descriptor (SIGIO) */
    uint32_t ssi_tid;     /* Kernel timer ID (POSIX timers) */
    uint32_t ssi_band;    /* Band event (SIGIO) */
    uint32_t ssi_overrun; /* POSIX timer overrun count */
    uint32_t ssi_trapno;  /* Trap number that caused signal */
    int32_t  ssi_status;  /* Exit status or signal (SIGCHLD) */
    int32_t  ssi_int;     /* Integer sent by sigqueue(2) */
    uint64_t ssi_ptr;     /* Pointer sent by sigqueue(2) */
    uint64_t ssi_utime;   /* User CPU time consumed (SIGCHLD) */
    uint64_t ssi_stime;   /* System CPU time consumed (SIGCHLD) */
    uint64_t ssi_addr;    /* Address that generated signal
                             (for hardware-generated signals) */
    uint8_t  pad[48];     /* Pad size to 128 bytes (allow for
                             additional fields in the future) */
};

int qemu_signalfd(const sigset_t *mask);
void sigaction_invoke(struct sigaction *action, struct qemu_signalfd_siginfo *info);
int qemu_madise(void *addr, size_t len, int advice);
int qemu_open(const char *name, int flags, ...);
int qemu_close(int fd);
int qemu_dup(int fd);
int qemu_lock_fd(int fd, int64_t start, int64_t len, bool exclusive);
int qemu_unlock_fd(int fd, int64_t start, int64_t len);
int qemu_lock_fd_test(int fd, int64_t start, int64_t len, bool exclusive);
bool qemu_has_ofd_lock(void);


#if defined(__HAIKU__) && defined(__i386__)
#define FMT_pid "%ld"
#elif defined(WIN64)
#define FMT_pid "%" PRId64
#else
#define FMT_pid "%d"
#endif

int qemu_create_pidfile(const char *filename);
int qemu_get_thread_id(void);

#ifndef CONFIG_IOVEC
struct iovec {
	void *iov_base;
	size_t iov_len;
};
#define IOV_MAX 1024
ssize_t readv(int fd, const struct iovec *iov, int iov_cnt);
ssize_t writev(int fd, const struct iovec *iov, int iov_cnt);
#else
#include <sys/uio.h>
#endif

#define qemu_timersub timersub
void qemu_set_cloexec(int fd);
#define QEMU_HW_VERSION "2.5+"
void qemu_set_hw_version(const char *);
const char *qemu_hw_version(void);
void figs_set_state(bool requested);
char *qemu_get_local_state_pathname(const char *relative_pathname);
void qemu_init_exec_dir(const char *argv0);
char *qemu_get_exec_dir(void);
void os_mem_prealloc(int fd, char *area, size_t sz, int smp_cpus, Error **errp);
char *qemu_get_pid_name(pid_t pid);
pid_t qemu_fork(Error **errp);
extern uintptr_t qemu_real_host_page_size;
extern intptr_t qemu_real_host_page_mask;

extern int qemu_icache_linesize;
extern int qemu_dcache_linesize;

#endif /* QEMU_OSDEP_H_ */

























































