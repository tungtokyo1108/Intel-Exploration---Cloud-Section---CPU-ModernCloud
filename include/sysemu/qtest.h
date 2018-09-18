/*
 * qtest.h
 *
 *  Created on: Sep 18, 2018
 *      Student (coder): Tung Dang
 */

#ifndef SYSEMU_QTEST_H_
#define SYSEMU_QTEST_H_

#include "qemu-common.h"

extern bool qtest_allowed;
static inline bool qtest_enabled(void) {
	return qtest_allowed;
}

bool qtest_driver(void);
void qtest_init(const char *qtest_chrdev, const char *qtest_log, Error **errp);

static inline int qtest_available(void) {
#ifdef CONFIG_POSIX
	return 1;
#else
	return 0;
#endif
}

#endif /* SYSEMU_QTEST_H_ */
