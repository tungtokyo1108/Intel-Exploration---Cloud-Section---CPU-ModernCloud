/*
 * bswap.h
 *
 *  Created on: Aug 14, 2018
 *      Author: tungdang
 */

#ifndef QEMU_BSWAP_H_
#define QEMU_BSWAP_H_

#include "fpu/softfloat-types.h"
#ifndef CONFIG_MACHINE_BSWAP_H
# include <endian.h>
# include <bswap.h>
#endif
# include <endian.h>
# include <byteswap.h>
#include <stdio.h>
#include <string.h>

static inline uint16_t bswap16(uint16_t x) {
	return bswap_16(x);
}

static inline uint32_t bswap32(uint32_t x) {
	return bswap_32(x);
}

static inline uint64_t bswap64(uint64_t x) {
	return bswap_64(x);
}

static inline void bswap16s(uint16_t *s) {
	*s = bswap16(*s);
}

static inline void bswap32s(uint32_t *s) {
	*s = bswap32(*s);
}

static inline void bswap64s(uint64_t *s) {
	*s = bswap64(*s);
}

#define le_bswap(v,size) (v)
#define be_bswap(v,size) glue(bswap,size)(v)
#define le_bswaps(v,size)
#define be_bswaps(p,size) do {*p = glue(bswap,size)(*p);} while(0)

/*
#define CPU_CONVERT(endian,size,type) \
static inline type endian ## size ## _to_cpu(type v) \
{\
	return glue(endian, _bswap)(v,size);\
}\
\
static inline type cpu_to_## endian ## size(type v) \
{\
	return glue(endian, _bswap)(v, size);\
}\
\
static inline void endian ## size ## _to_cpus(type *p)\
{\
	glue(endian, _bswaps)(p,size);\
}\
\
static inline void cpu_to_## endian ## size ## s(type *p)\
{\
	glue(endian, _bswaps)(p,size);\
}

CPU_CONVERT(be, 16, uint16_t)
CPU_CONVERT(be, 32, uint32_t)
CPU_CONVERT(be, 64, uint64_t)
CPU_CONVERT(le, 16, uint16_t)
CPU_CONVERT(le, 32, uint32_t)
CPU_CONVERT(le, 64, uint64_t)
*/

static inline uint32_t qemu_bswap_len(uint32_t value, int len) {
	return bswap32(value) >> (32 - 8 * len);
}

typedef union {
	float32 f;
	uint32_t l;
} CPU_FloatU;

typedef union {
	float64 d;
#if defined(HOST_WORDS_BIGENDIAN)
	struct {
		uint32_t upper;
		uint32_t lower;
	} l;
#else
	struct {
		uint32_t lower;
		uint32_t upper;
	} l;
#endif
	uint64_t ll;
} CPU_DoubleU;

typedef union {
	floatx80 d;
	struct {
		uint64_t lower;
		uint16_t upper;
	} l;
} CPU_LDoubleU;

typedef union {
	float128 q;
#if defined(HOST_WORDS_BIGENDIAN)
	struct {
		uint32_t upmost;
		uint32_t upper;
		uint32_t lower;
		uint32_t lowest;
	} l;
	struct {
		uint64_t upper;
		uint64_t lower;
	} ll;
#else
	struct {
		uint32_t lowest;
		uint32_t lower;
		uint32_t upper;
		uint32_t upmost;
	} l;
	struct {
		uint64_t lower;
		uint64_t upper;
	} ll;
#endif;
} CPU_QuadU;

static inline int ldub_p(const void *ptr) {
	return *(uint8_t *)ptr;
}

static inline int ldsb_p(const void *ptr) {
	return *(int8_t *)ptr;
}

static inline void stb_p(void *ptr, uint8_t v) {
	*(uint8_t *)ptr = v;
}

static inline int lduw_he_p(const void *ptr) {
	uint16_t r;
	memcpy(&r, ptr, sizeof(r));
	return r;
}

static inline int ldsw_he_p(const void *ptr) {
	int16_t r;
	memcpy(&r,ptr,sizeof(r));
	return r;
}

static inline void stw_he_p(void *ptr, uint16_t v) {
	memcpy(ptr, &v, sizeof(v));
}

static inline int ldl_he_p(const void *ptr) {
	int32_t r;
	memcpy(&r,ptr,sizeof(r));
	return r;
}

static inline void stl_he_p(void *ptr, uint32_t v) {
	memcpy(ptr,&v,sizeof(v));
}

static inline uint64_t ldq_he_p(const void *ptr) {
	uint64_t r;
	memcpy(&r,ptr,sizeof(r));
	return r;
}

static inline void stq_he_p(void *ptr, uint64_t v) {
	memcpy(ptr,&v,sizeof(v));
}

static inline int lduw_le_p(const void *ptr) {
	return (uint16_t)le_bswap(lduw_he_p(ptr),16);
}

static inline int ldsw_le_p(const void *ptr) {
	return (int16_t)le_bswap(lduw_he_p(ptr),16);
}

static inline int ldl_le_p(const void *ptr) {
	return le_bswap(ldl_he_p(ptr),32);
}

static inline uint64_t ldq_le_p(const void *ptr) {
	return le_bswap(ldq_he_p(ptr),64);
}

static inline void stw_le_p(void *ptr, uint16_t v) {
	stw_he_p(ptr, le_bswap(v,16));
}

static inline void stl_le_p(void* ptr, uint32_t v) {
	stl_he_p(ptr, le_bswap(v,32));
}

static inline void stq_le_p(void *ptr, uint64_t v) {
	stq_he_p(ptr, le_bswap(v,64));
}

static inline float32 ldfl_le_p(const void *ptr) {
	CPU_DoubleU u;
	u.ll = ldq_le_p(ptr);
	return u.d;
}

static inline void stfq_le_p(void *ptr, float64 v) {
	CPU_DoubleU u;
	u.d = v;
	stq_le_p(ptr,u.ll);
}

static inline int lduw_be_p(const void *ptr) {
	return (uint16_t)be_bswap(lduw_he_p(ptr),16);
}

static inline int ldsw_be_p(const void *ptr) {
	return (int16_t)be_bswap(lduw_he_p(ptr),16);
}

static inline int ldl_be_p(const void *ptr) {
	return be_bswap(ldl_he_p(ptr),32);
}

static inline uint64_t ldq_be_p(const void *ptr) {
	return be_bswap(ldq_he_p(ptr),64);
}

static inline void stw_be_p(void *ptr, uint16_t v)
{
    stw_he_p(ptr, be_bswap(v, 16));
}

static inline void stl_be_p(void *ptr, uint32_t v)
{
    stl_he_p(ptr, be_bswap(v, 32));
}

static inline void stq_be_p(void *ptr, uint64_t v)
{
    stq_he_p(ptr, be_bswap(v, 64));
}

static inline float32 ldfl_be_p(const void *ptr) {
	CPU_FloatU u;
	u.l = ldl_be_p(ptr);
	return u.f;
}

static inline void stfl_be_p(void *ptr, float32 v) {
	CPU_FloatU u;
	u.f = v;
	stl_be_p(ptr,u.l);
}

static inline float64 ldfq_be_p(const void *ptr)
{
    CPU_DoubleU u;
    u.ll = ldq_be_p(ptr);
    return u.d;
}

static inline void stfq_be_p(void *ptr, float64 v)
{
    CPU_DoubleU u;
    u.d = v;
    stq_be_p(ptr, u.ll);
}

#endif /* QEMU_BSWAP_H_ */
















































