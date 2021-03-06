/*
 * fprintf-fn.h
 *
 *  Created on: Aug 16, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_FPRINTF_FN_H_
#define QEMU_FPRINTF_FN_H_
#include <stdio.h>

#define GCC_FMT_ATTR(n,m) __attribute__((format(gmu_printf,n,m)))
typedef int  (*fprintf_function)(FILE *f, const char *fmt, ...) GCC_FMT_ATTR(2,3);

#endif /* QEMU_FPRINTF_FN_H_ */
