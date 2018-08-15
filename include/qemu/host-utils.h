/*
 * host-utils.h
 *
 *  Created on: Aug 15, 2018
 *      Author: tungdang
 */

#ifndef QEMU_HOST_UTILS_H_
#define QEMU_HOST_UTILS_H_

#include "qemu/bswap.h"

static inline void mulu64(uint64_t *plow, uint64_t *phigh, uint64_t a, uint64_t b) {
	__uint128_t r = (__uint128_t)a * b;
	*plow = r;
	*phigh = r >> 64;
}

static inline void muls64(uint64_t *plow, uint64_t *phigh, int64_t a, int64_t b) {
	__int128_t r = (__int128_t)a * b;
	*plow = r;
    *phigh = r >> 64;
}

static inline uint64_t muldiv64(uint64_t a, uint32_t b, uint32_t c) {
	return (__int128_t)a * b/c;
}

static inline int divu128(uint64_t *plow, uint64_t *phigh, uint64_t divisor) {
	if (divisor == 0)
	{ return 1;
	} else {
		__uint128_t divident = ((__uint128_t)*phigh << 64) | *plow;
		__uint128_t result = divident / divisor;
		*plow = result;
		*phigh = divident % divisor;
		return result > UINT64_MAX;
	}
}

static inline int divs128(int64_t *plow, int64_t *phigh, int64_t divisor) {
	if (divisor == 0) {
		return 1;
	} else {
		__int128_t dividend = ((__int128_t)*phigh << 64) | *plow;
		__int128_t result = dividend / divisor;
		*plow = result;
		*phigh = dividend % divisor;
		return result != *plow;
	}
}

static inline uint64_t muldiv_64(uint64_t a, uint32_t b, uint32_t c) {
	union {
		uint64_t ll;
		struct {
			uint32_t low, high;
		} l;
	} u, res;
	uint64_t rl, rh;

	u.ll = a;
	rl = (uint64_t)u.l.low * (uint64_t)b;
	rh = (uint64_t)u.l.high * (uint64_t)b;
	rh += (rl >> 32);
	res.l.high = rh / c;
	res.l.low = (((rh % c) << 32) + (rl & 0xffffffff)) / c;
	return res.ll;
}

static inline int clz32(uint32_t val) {
	return val ? __builtin_clz(val) : 32;
}

static inline int clo32(uint32_t val) {
	return clz32(~val);
}

static inline int clz64(uint64_t val) {
	return val ? __builtin_clzll(val) : 64;
}

static inline int clo64(uint64_t val) {
	return clz64(~val);
}

static inline int ctz32(uint32_t val) {
	return val ? __builtin_ctz(val) : 32;
}

static inline int cto32(uint32_t val) {
	return ctz32(~val);
}

static inline int ctz64(uint64_t val)
{
    return val ? __builtin_ctzll(val) : 64;
}

static inline int cto64(uint64_t val)
{
    return ctz64(~val);
}

static inline int clrsb32(uint32_t val)
{
#if QEMU_GNUC_PREREQ(4, 7)
    return __builtin_clrsb(val);
#else
    return clz32(val ^ ((int32_t)val >> 1)) - 1;
#endif
}

static inline int clrsb64(uint64_t val) {
#if QEMU_GNUC_PREREQ(4,7)
	return __builtin_clrsbll(val);
#else
	return clz64(val ^ ((int64_t)val >> 1)) - 1;
#endif
}

/////////////////////////////////////////////////////////////////

static inline int ctpop8(uint8_t val) {
	return __builtin_popcount(val);
}

static inline int ctpop16(uint16_t val) {
	return __builtin_popcount(val);
}

static inline int ctpop32(uint32_t val)
{
    return __builtin_popcount(val);
}

static inline int ctpop64(uint64_t val)
{
    return __builtin_popcountll(val);
}

///////////////////////////////////////////////////////////////

static inline uint8_t revbit8(uint8_t x) {
	x = ((x & 0xf0) >> 4)
	  | ((x & 0x0f) << 4);

	x = ((x & 0x88) >> 3)
	  | ((x & 0x44) >> 1)
	  | ((x & 0x22) << 1)
	  | ((x & 0x11) << 3);

	return x;
}

static inline uint16_t revbit16(uint16_t x) {
	x = bswap16(x);
	x = ((x & 0xf0f0) >> 4)
	  | ((x & 0x0f0f) << 4);
	x = ((x & 0x8888) >> 3)
	  | ((x & 0x4444) >> 1)
	  | ((x & 0x2222) << 1)
	  | ((x & 0x1111) << 3);

	return x;
}

static inline uint32_t revbit32(uint32_t x)
{
    x = bswap32(x);
    x = ((x & 0xf0f0f0f0u) >> 4)
      | ((x & 0x0f0f0f0fu) << 4);
    x = ((x & 0x88888888u) >> 3)
      | ((x & 0x44444444u) >> 1)
      | ((x & 0x22222222u) << 1)
      | ((x & 0x11111111u) << 3);
    return x;
}

static inline uint64_t revbit64(uint64_t x)
{
    x = bswap64(x);
    x = ((x & 0xf0f0f0f0f0f0f0f0ull) >> 4)
      | ((x & 0x0f0f0f0f0f0f0f0full) << 4);
    x = ((x & 0x8888888888888888ull) >> 3)
      | ((x & 0x4444444444444444ull) >> 1)
      | ((x & 0x2222222222222222ull) << 1)
      | ((x & 0x1111111111111111ull) << 3);
    return x;
}

//////////////////////////////////////////////////////////////

static inline bool is_power_of_2 (uint64_t value) {
	if (!value)
	{
		return false;
	}
	return !(value & (value - 1));
}

static inline uint32_t ppw2roundup32(uint32_t x) {
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);

	return x;
}

void urshift(uint64_t *plow, uint64_t *phigh, int32_t shift);
void ulshift(uint64_t *plow, uint64_t *phigh, int32_t shift, bool *overflow);

#endif /* QEMU_HOST_UTILS_H_ */
