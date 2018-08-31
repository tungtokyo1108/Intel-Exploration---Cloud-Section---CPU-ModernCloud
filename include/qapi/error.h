/*
 * error.h
 *
 *  Created on: Aug 30, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QAPI_ERROR_H_
#define QAPI_ERROR_H_

#include "qemu/osdep.h"

// Overall category of an error
/*
typedef enum ErrorClass {
	ERROR_CLASS_GENERIC_ERROR = QAPI_ERROR_CLASS_GENERICERROR,
	ERROR_CLASS_COMMAND_NOT_FOUND = QAPI_ERROR_CLASS_COMMANDNOTFOUND,
	ERROR_CLASS_DEVICE_NOT_ACTIVE = QAPI_ERROR_CLASS_DEVICENOTACTIVE,
	ERROR_CLASS_DEVICE_NOT_FOUND = QAPI_ERROR_CLASS_DEVICENOTFOUND,
	ERROR_CLASS_KVM_MISSING_CAP = QAPI_ERROR_CLASS_KVMMISSINGCAP,
} ErrorClass;
*/

#define GCC_ATTR_ __attribute__((__unused__, format(printf,1,2)))
#define GCC_FMT_ATTR(n,m) __attribute__((format(printf,n,m)))

typedef enum ErrorClass {
	ERROR_CLASS_GENERIC_ERROR,
	ERROR_CLASS_COMMAND_NOT_FOUND,
	ERROR_CLASS_DEVICE_NOT_ACTIVE,
	ERROR_CLASS_DEVICE_NOT_FOUND,
	ERROR_CLASS_KVM_MISSING_CAP,
} ErrorClass;

const char *error_get_pretty(const Error *err);
ErrorClass error_get_class(const Error *err);
#define error_setg(errp, fmt, ...)              \
	error_setg_internal((errp), __FILE__, __LINE__, __func__, (fmt), ## __VA_ARGS__)
void error_setg_internal(Error **errp, const char *src, int line, const char *func, const char *fmt, ...) GCC_FMT_ATTR(5,6);

#define error_setg_errno(errp, os_error, fmt, ...)                      \
    error_setg_errno_internal((errp), __FILE__, __LINE__, __func__,     \
                              (os_error), (fmt), ## __VA_ARGS__)
void error_setg_errno_internal(Error **errp,
                               const char *fname, int line, const char *func,
                               int os_error, const char *fmt, ...) GCC_FMT_ATTR(6, 7);

#ifdef _WIN32
#define error_setg_win32(errp, win32_err, fmt, ...)                     \
    error_setg_win32_internal((errp), __FILE__, __LINE__, __func__,     \
                              (win32_err), (fmt), ## __VA_ARGS__)
void error_setg_win32_internal(Error **errp,
                               const char *src, int line, const char *func,
                               int win32_err, const char *fmt, ...)
    GCC_FMT_ATTR(6, 7);
#endif

void error_propagate(Error **dst_errp, Error *local_err);
void error_vprepend(Error **errp, const char *fmt, va_list ap); // Prepend some text to errp's human-readable error message
void error_prepend(Error **errp, const char *fmt, ... ) GCC_FMT_ATTR(2,3);
void error_append_hint(Error **errp, const char *fmt, ...) GCC_FMT_ATTR(2,3);

// Convenience function to report open() failure

#define error_setg_file_open(errp, os_errno, filename) \
	error_setg_file_open_internal((errp), __FILE__, __LINE__, __func__, (os_errno), (filename))
void error_setg_file_open_internal(Error **errp, const char *src, int line, const char *func, int os_errno, const char *filename);

Error *error_copy(const Error *err);
void error_free(Error *err);
void error_free_or_abort(Error **errp);
void warn_report_err(Error *err);
void error_report_err(Error *err);
void warn_reportf_err(Error *err, const char *fmt, ...)GCC_FMT_ATTR(2, 3);
void error_reportf_err(Error *err, const char *fmt, ...)GCC_FMT_ATTR(2, 3);
#define error_set(errp, err_class, fmt, ...)            \
	error_set_internal((errp), __FILE__, __LINE__, __func__, (err_class), (fmt), ## __VA_ARGS__)

void error_set_internal(Error **errp, const char *src, int line, const char *func, ErrorClass err_class, const char *fmt, ...) GCC_FMT_ATTR(6,7);

// Special error destination to abort/exit on error
extern Error *error_abort;
extern Error *error_fatal;

#endif /* QAPI_ERROR_H_ */
