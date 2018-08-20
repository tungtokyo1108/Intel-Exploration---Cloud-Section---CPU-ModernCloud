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


#endif /* QEMU_OSDEP_H_ */

























































