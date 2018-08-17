/*
 * option.h
 *
 *  Created on: Aug 17, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_OPTION_H_
#define QEMU_OPTION_H_

#include "qemu/queue.h"
#include "qemu/error-report.h"
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

const char *get_opt_name(char *buf, int buf_size, const char *p, char delim);
const char *get_opt_value(char *buf, int buf_size, const char *p);

void parse_option_size(const char *name, const char *value, uint64_t *ret);

enum QemuOptType {
	QEMU_OPT_STRING = 0,
	QEMU_OPT_BOOL,
	QEMU_OPT_NUMBER,
	QEMU_OPT_SIZE,
};

typedef struct QemuOptDesc {
	const char *name;
	enum QemuOptType type;
	const char *help;
	const char *def_value_str;
} QemuOptDesc;

typedef struct QemuOptsList {
	const char *name;
	const char *implied_opt_name;
	bool merge_lists;
	QTAILQ_HEAD(, QemuOpts) head;
	QemuOptDesc desc[];
} QemuOptsList;

typedef struct QemuOpt {
	char *name;
	char *str;
	const QemuOptDesc *desc;
	union {
		bool boolean;
		uint64_t uint;
	} value;
	QemuOpts *opts;
	QTAILQ_ENTRY(QemuOpt) next;
} QemuOpt;

typedef struct QemuOpts{
	char *id;
	QemuOptsList *list;
	Location loc;
	QTAILQ_HEAD(QemuOptHead, QemuOpt) head;
	QTAILQ_ENTRY(QemuOpts) next;
} QemuOpts;

const char *qemu_opt_get(QemuOpts *opts, const char *name);
char *qemu_opt_get_del(QemuOpts *opts, const char *name);

bool qemu_opt_has_help_opt(QemuOpts *opts);
QemuOpt *qemu_opt_find(QemuOpts *opts, const char *name);
bool qemu_opt_get_bool(QemuOpts *opts, const char *name, bool defval);
uint64_t qemu_opt_get_number(QemuOpts *opts, const char *name, uint64_t defval);
uint64_t qemu_opt_get_size(QemuOpts *opt, const char *name, uint64_t defval);
uint64_t qemu_opt_get_number_del(QemuOpts *opts, const char *name, uint64_t defval);
uint64_t qemu_opt_get_size_del(QemuOpts *opts, const char *name, uint64_t defval);

int qemu_opt_unset(QemuOpts* opts, const char *name);
void qemu_opt_set(QemuOpts* opts, const char *name, const char *value);
void qemu_opt_set_bool(QemuOpts* opts, const char *name, bool val);
void qemu_opt_set_number(QemuOpts* opts, const char *name, int64_t val);
typedef int (*qemu_opt_loopfunc)(void *opaque, const char *name, const char *value);
int qemu_opt_foreach(QemuOpts *opts, qemu_opt_loopfunc func, void *opaque);

typedef struct {
	QemuOpts *opts;
	QemuOpt *opt;
	const char *name;
} QemuOptsIter;

QemuOpts *qemu_opts_find(QemuOptsList *list, const char *id);
QemuOpts *qemu_opts_create(QemuOptsList *list, const char *id, int fail_if_exists);
void qemu_opts_reset(QemuOptsList *list);
void qemu_opts_loc_restore(QemuOpts *opts);
void qemu_opts_set(QemuOptsList *list, const char *id, const char *name, const char *value);
const char *qemu_opts_id(QemuOpts *opts);
void qemu_opts_set_id(QemuOpts *opts, char *id);
void qemu_opts_del(QemuOpts *opts, const QemuOptDesc *desc);
void qemu_opts_validate(QemuOpts *opts, const QemuOptDesc *desc);
void qemu_opts_do_parse(QemuOpts *opts, const char *params, const char *firstname);

#endif /* QEMU_OPTION_H_ */
