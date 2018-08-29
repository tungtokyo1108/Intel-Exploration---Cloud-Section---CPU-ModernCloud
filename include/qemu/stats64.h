/*
 * stats64.h
 *
 *  Created on: Aug 28, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_STATS64_H_
#define QEMU_STATS64_H_

#include "qemu/atomic.h"
#include <stdint.h>

typedef struct Stat64 {
#ifdef CONFIG_ATOMIC64
	uint64_t value;
#else
	uint32_t low, high;
	uint32_t lock;
#endif
} Stat64;


#ifdef CONFIG_ATOMIC64
static inline void stat64_init(Stat64 *s, uint64_t value) {
	*s = (Stat64){value};
}

static inline uint64_t stat64_get(const Stat64 *s) {
	return atomic_read__nocheck(&s->value);
}

static inline void stat64_add(Stat64 *s, uint64_t value) {
	atomic_add(&s->value,value);
}

static inline void stat64_min(Stat64 *s, uint64_t value) {
	uint64_t orig = atomic_read__nocheck(&s->value);
	while (orig > value) {
		orig = atomic_cmpxchg__nocheck(&s->value,orig,value);
	}
}

static inline void stat64_max(Stat64* s, uint64_t value) {
	uint64_t orig = atomic_read__nocheck(&s->value);
	while (orig < value) {
		orig = atomic_cmpxchg__nocheck(&s->value,orig,value);
	}
}
#else
uint64_t stat64_get(const Stat64 *s);
bool stat64_min_slow(Stat64 *s, uint64_t value);
bool stat64_max_slow(Stat64 *s, uint64_t value);
bool stat64_add32_carry(Stat64 *s, uint32_t low, uint32_t high);

static inline void stat64_init(Stat64 *s, uint64_t value) {
	*s = (Stat64) {.low = value, .high = value >> 32, .lock = 0};
}

static inline void stat64_add(Stat64 *s, uint64_t value) {
	uint32_t low, high;
	high = value >> 32;
	low = (uint32_t) value;
	if (!low) {
		if (high) {
			atomic_add(&s->high,high);
		}
		return;
	}

	for (;;) {
		uint32_t orig = s->low;
		uint32_t result = orig + low;
		uint32_t old;

		if (result < low || high) {
			if (stat64_add32_carry(s,low,high)) {
				return;
			}
			continue;
		}
		old = atomic_cmpxchg(&s->low,orig,result);
		if (orig == old){
			return;
		}
	}
}

static inline void stat64_min(Stat64 *s, uint64_t value) {
	uint32_t low, high;
	uint32_t orig_low, orig_high;
	high = value >> 32;
	low = (uint32_t) value;
	do {
		orig_high = atomic_read(&s->high);
		if (orig_high < high) {
			return;
		}

		if (orig_high == high) {
			//smp_rmb();
			orig_low = atomic_read(&s->low);
			if (orig_low <= low) {
				return;
			}
			// smp_rmb();
			orig_high = atomic_read(&s->high);
			if (orig_high < high) {
				return;
			}
		}
	} while (!stat64_min_slow(s,value));

}

static inline void stat64_max(Stat64 *s, uint64_t value) {
	uint32_t low, high;
	uint32_t orig_low, orig_high;
	high = value >> 32;
	low = (uint32_t) value;
	do {
		orig_high = atomic_read(&s->high);
		if (orig_high > high) {
			return;
		}

		if (orig_high == high) {
			smp_rmb();
			orig_low = atomic_read(&s->low);
			if (orig_low >= low) {
				return;
			}

			smp_rmb();
			orig_high = atomic_read(&s->high);
			if (orig_high > high) {
				return;
			}
		}
	} while (!stat64_max_slow(s,value));
}

#endif

#endif /* QEMU_STATS64_H_ */
