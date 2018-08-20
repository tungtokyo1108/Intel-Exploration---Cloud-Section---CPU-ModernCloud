/*
 * compiler.h
 *
 *  Created on: Aug 20, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_COMPILER_H_
#define QEMU_COMPILER_H_

#if defined __clang_analyzer__ || defined __COVERITY__
#define QEMU_STATIC_ANALYSIS 1
#endif

#if defined(__GNUC__) && defined(__GNUC_MINOR__)
# define QEMU_GNUC_PREREQ(maj, min) \
         ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
# define QEMU_GNUC_PREREQ(maj, min) 0
#endif

#define QEMU_NORETURN __attribute__ ((__noreturn__))

#define QEMU_WARN_UNUSED_RESULT __attribute__((warn_unused_result))

#define QEMU_SENTINEL __attribute__((sentinel))

#if QEMU_GNUC_PREREQ(4, 3)
#define QEMU_ARTIFICIAL __attribute__((always_inline, artificial))
#else
#define QEMU_ARTIFICIAL
#endif

#define QEMU_PACKED __attribute__((packed))
#define QEMU_ALIGNED(X) __attribute__((aligned(X)))

#ifndef glue
#define xglue(x,y) x ## y
#define glue(x,y) xglue(x,y)
#define stringify(s) tostring(s)
#define tostring(s) #s
#endif

#ifndef likely
#if __GNUC__ < 3
#define __builtin_expect(x,n) (x)
#endif

#define likely(x) __builtin_expect(!!(x),1)
#define unlikely(x) __builtin_expect(!!(x),0)
#endif



#endif /* QEMU_COMPILER_H_ */
