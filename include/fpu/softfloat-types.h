/*
 * softfloat-types.h
 *
 *  Created on: Aug 14, 2018
 *      Author: tungdang
 */

#ifndef FPU_SOFTFLOAT_TYPES_H_
#define FPU_SOFTFLOAT_TYPES_H_

#include <stdint.h>

typedef uint8_t flag;
typedef uint16_t float16;
typedef uint32_t float32;
typedef uint64_t float64;

#define float16_val(x) (x)
#define float32_val(x) (x)
#define float64_val(x) (x)
#define make_float16(x) (x)
#define make_float32(x) (x)
#define make_float64(x) (x)
#define const_float16(x) (x)
#define const_float32(x) (x)
#define const_float64(x) (x)
typedef struct {
	uint64_t low;
	uint16_t high;
} floatx80;

#define make_floatx80(exp,mant) ((floatx80) {mant,exp})
#define make_floatx80_int(exp,mant) {.low = mant, .high = exp}

typedef struct {
	uint64_t low, high;
} float128;

#define make_float128(high_, low_) ((float128) {.high = high_, .low = low_})
#define make_float128_init(high_, low_) {.high = high_, .low = low_}

enum {
    float_tininess_after_rounding  = 0,
    float_tininess_before_rounding = 1
};

enum {
    float_round_nearest_even = 0,
    float_round_down         = 1,
    float_round_up           = 2,
    float_round_to_zero      = 3,
    float_round_ties_away    = 4,
    float_round_to_odd       = 5,
};

enum {
    float_flag_invalid   =  1,
    float_flag_divbyzero =  4,
    float_flag_overflow  =  8,
    float_flag_underflow = 16,
    float_flag_inexact   = 32,
    float_flag_input_denormal = 64,
    float_flag_output_denormal = 128
};

typedef struct float_status {
	signed char float_detect_tininess;
	signed char float_rounding_mode;
	uint8_t float_exception_flags;
	signed char floatx80_rounding_precision;
	flag flush_to_zero;
	flag flush_inputs_to_zero;
	flag default_nan_mode;
	flag snan_bit_is_one;

} float_status;

#endif /* FPU_SOFTFLOAT_TYPES_H_ */









































