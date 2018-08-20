/*
 * cutils.h
 *
 *  Created on: Aug 20, 2018
 *      Author: tungdang
 */

#ifndef QEMU_CUTILS_H_
#define QEMU_CUTILS_H_

#include "qemu/fprintf-fn.h"
#include <stdint.h>

/**
 * Copy @str into @buffer
 * @buf: buffer containing existing string
 * @buf_size: size of @buf
 * @str: string to copy
 */

void pstrcpy(char *buf, int buf_size, const char *str);
void strpadcpy(char *buf, int buf_size, const char *str, char pad);
char *pstrcat(char *buf, int buf_size, const char *s);
int strstart(const char *str, const char *val, const char **ptr);
int qemu_strnlen(const char *s, int max_len);

char *qemu_strsep(char **input, const char *delim);
// time_t mktimegm(struct tm *tm);
int qemu_fdatasync(int fd);
int fcnt1_setfl(int fd, int flag);
int qemu_parse_fd(const char *param);
int qemu_strtoi(const char *nptr, const char **endptr, int base, int *result);
int qemu_strtoui(const char *nptr, const char **endptr, int base, unsigned int *result);
int qemu_strtol(const char *nptr, const char **endptr, int base, long *result);
int qemu_strtoul(const char *nptr, const char **endptr, int base, unsigned long *result);
int qemu_strtoi64(const char *nptr, const char **endptr, int base, int64_t *result);
int qemu_strtou64(const char *nptr, const char **endptr, int base, uint64_t *result);

int parse_uint(const char *s, unsigned long long *value, char **endptr, int base);
int parse_uint_full(const char *s, unsigned long long *value, int base);

int qemu_strtosz(const char *nptr, char **end, uint64_t *result);
int qemu_strtosz_MiB(const char *nptr, char **end, uint64_t *result);
int qemu_strtosz_metric(const char *nptr, char **end, uint64_t *result);

#define K_BYTE     (1ULL << 10)
#define M_BYTE     (1ULL << 20)
#define G_BYTE     (1ULL << 30)
#define T_BYTE     (1ULL << 40)
#define P_BYTE     (1ULL << 50)
#define E_BYTE     (1ULL << 60)
#define STR_OR_NULL(str) ((str) ? (str) : "null")

bool buffer_is_zero(const void *buf, size_t len);
bool test_buffer_is_zero_next_accel(void);

int uleb128_encode_small(uint8_t *out, uint32_t n);
int uleb128_decode_small(const uint8_t *in, uint32_t *n);


#endif /* QEMU_CUTILS_H_ */
