/*
 * error-report.h
 *
 *  Created on: Aug 17, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_ERROR_REPORT_H_
#define QEMU_ERROR_REPORT_H_

#include <stdarg.h>

//#define GCC_ATTR_ __attribute__((__unused__, format(gnu_printf,1,2)))
// #define GCC_FMT_ATTR(n,m) __attribute__((format(gmu_printf,n,m)))

#define GCC_ATTR
#define GCC_FMT_ATTR(n,m)

typedef struct Location {
	enum {LOC_NAME, LOC_CMDLINE, LOC_FILE} kind;
	int num;
	const void *ptr;
	struct Location *prev;
} Location;

Location *loc_push_restore(Location *loc);
Location *loc_push_none(Location *loc);
Location *loc_pop(Location *loc);
Location *loc_save(Location *loc);

void loc_restore(Location *loc);
void loc_set_none(void);
void loc_set_cmdline(char **argv, int idx, int cnt);
void loc_set_file(const char *fname, int lno);

void error_vprintf(const char *fmt, va_list ap) {GCC_FMT_ATTR(1,0);}
void error_printf(const char *fmt, ...) {GCC_FMT_ATTR(1,2);}
void error_vprintf_nless_qmp(const char *fmt, va_list ap) {GCC_FMT_ATTR(1,0);}
void error_pintf_unless_qmp(const char *fmt, ...) {GCC_FMT_ATTR(1,2);}
void error_set_programme(const char *argv0);

void error_vreport(const char *fmt, va_list ap) {GCC_FMT_ATTR(1,0);}
void warn_vreprt(const char *fmt, va_list ap) {GCC_FMT_ATTR(1,0);}
void info_vroprt(const char *fmt, va_list ap) {GCC_FMT_ATTR(1,0);}

void error_report(const char *fmt, ...) GCC_FMT_ATTR(1, 2);
void warn_report(const char *fmt, ...) GCC_FMT_ATTR(1, 2);
void info_report(const char *fmt, ...) GCC_FMT_ATTR(1, 2);

const char *error_get_progname(void);
extern bool enable_timestamp_msg;

#endif /* QEMU_ERROR_REPORT_H_ */
