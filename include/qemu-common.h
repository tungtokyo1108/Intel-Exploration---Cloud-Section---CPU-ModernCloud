/*
 * qemu-common.h
 *
 *  Created on: Aug 16, 2018
 *      Author: tungdang
 */

#ifndef QEMU_COMMON_H_
#define QEMU_COMMON_H_

#include "qemu/fprintf-fn.h"
#include "qemu/module.h"
#include <stdint.h>

#define TFR(expr) do { if ((expr) != -1) break;} while (errno == EINTR)

void qemu_get_timedate(struct tm *tm, int offset);
int qemu_timedate_diff(struct tm *tm);

#define qemu_isalnum(c) isalnum((unsigned char)(c))
#define qemu_isalpha(c) isalpha((unsigned char)(c))
#define qemu_iscntrl(c) iscntrl((unsigned char)(c))

void *qemu_oom_check(void *ptr);

ssize_t qemu_write_full(int fd, const void *buf, size_t count);
int qemu_pipe(int pipefd[2]);
int qemu_openpty_raw(int *aslave, char *pty_name);
#define qemu_getsockopt(sockfd, level, optname, optval, optlen) \
	getsockopt(sockfd, level, optname, optval, optlen)
#define qemu_setsockopt(sockfd, level, optname, optval, optlen) \
	setsockopt(sockfd, level, optname, optval, optlen)
#define qemu_recv(sockfd, buf, len, flags) recv(sockfd, buf, len, flags)
#define qemu_sendto(sockfd, buf, len, flags, destaddr, addrlen) \
	sendto(sockfd, buf, len, flags, destaddr, addrlen)

void cpu_exec_init_all(void);
// void cpu_exec_step_atomic(CPUState *cpu);
bool set_preferred_target_page_bits(int bits);
#define QEMU_FILE_TYPE_BIOS 0
#define QEMU_FILE_TYPE_KEYMAP 1
char *qemu_find_file(int type, const char *name);

void os_setup_early_signal_handling(void);
char *os_find_datadir(void);
void os_parse_cmd_args(int index, const char *optarg);

void qemu_hexdump(const char *buf, FILE *fp, const char *prefix, size_t size);
//const char *qemu_ether_ntoa(const MACAddr *mac);
char *size_to_str(uint64_t val);
void page_size_init(void);
bool dump_in_progress(void);

#endif /* QEMU_COMMON_H_ */
