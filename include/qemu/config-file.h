/*
 * config-file.h
 *
 *  Created on: Aug 17, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_CONFIG_FILE_H_
#define QEMU_CONFIG_FILE_H_

#include "qemu/option.h"

QemuOptsList *qemu_find_opts(const char *group);
QemuOpts *qemu_find_opts_singleton(const char *group);

void qemu_add_opts(QemuOptsList *list);
void qemu_add_drive_opts(QemuOptsList *list);
int qemu_set_option(const char *str);
int qemu_global_option(const char *str);

void qemu_config_write(FILE *fp);
int qemu_config_parse(FILE *fp, QemuOptsList **lists, const char *fname);
int qemu_read_config_file(const char *filename);

// void qemu_config_parse_qdict(QDict *options, QemuOptsList **lists, Error **errp);

#endif /* QEMU_CONFIG_FILE_H_ */
