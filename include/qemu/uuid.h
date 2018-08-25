/*
 * uuid.h
 *
 *  Created on: Aug 25, 2018
 *      Author: tungdang
 */

#ifndef QEMU_UUID_H_
#define QEMU_UUID_H_

#include "qemu-common.h"

typedef struct {
	union {
		unsigned char data[16];
		struct {
			uint32_t time_slow;
			uint16_t time_mid;
			uint16_t time_hight_and_version;
			uint8_t clock_seq_and_reverved;
			uint8_t clock_seq_low;
			uint8_t node[6];
		};
	};
} QemuUUID;

#define UUID_FMT "%02hhx%02hhx%02hhx%02hhx-" \
                 "%02hhx%02hhx-%02hhx%02hhx-" \
                 "%02hhx%02hhx-" \
                 "%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx"

#define UUID_FMT_LEN 36

#define UUID_NONE "00000000-0000-0000-0000-000000000000"
void qemu_uuid_generate(QemuUUID *out);
int qemu_uuid_is_null(const QemuUUID *uu);
int qemu_uuid_is_equal(const QemuUUID *lhv, const QemuUUID *rhv);
void qemu_uuid_unparse(const QemuUUID *uuid, char *out);
char *qemu_uuid_unparse_strdup(const QemuUUID *uuid);
int qemu_uuid_parse(const char *str, QemuUUID *uuid);
void qemu_uuid_bswap(QemuUUID *uuid);

#endif /* QEMU_UUID_H_ */
