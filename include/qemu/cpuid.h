/*
 * cpuid.h
 *
 *  Created on: Aug 20, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_CPUID_H_
#define QEMU_CPUID_H_

#include <cpuid.h>

#ifndef bit_CMOV
#define bit_CMOV (1 << 15)
#endif

#ifndef bit_SSE2
#define bit_SSE2 (1 << 26)
#endif

#ifndef bit_SSE4_1
#define bit_SSE4_1      (1 << 19)
#endif
#ifndef bit_MOVBE
#define bit_MOVBE       (1 << 22)
#endif
#ifndef bit_OSXSAVE
#define bit_OSXSAVE     (1 << 27)
#endif
#ifndef bit_AVX
#define bit_AVX         (1 << 28)
#endif

#ifndef bit_BMI
#define bit_BMI         (1 << 3)
#endif
#ifndef bit_AVX2
#define bit_AVX2        (1 << 5)
#endif
#ifndef bit_BMI2
#define bit_BMI2        (1 << 8)
#endif

#ifndef bit_LZCNT
#define bit_LZCNT       (1 << 5)
#endif

#endif /* QEMU_CPUID_H_ */
