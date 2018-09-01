/*
 * qerror.h
 *
 *  Created on: Sep 1, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QAPI_QMP_QERROR_H_
#define QAPI_QMP_QERROR_H_

#define QERR_BASE_NOT_FOUND \
    "Base '%s' not found"

#define QERR_BUS_NO_HOTPLUG \
    "Bus '%s' does not support hotplugging"

#define QERR_DEVICE_HAS_NO_MEDIUM \
    "Device '%s' has no medium"

#define QERR_DEVICE_INIT_FAILED \
    "Device '%s' could not be initialized"

#define QERR_DEVICE_IN_USE \
    "Device '%s' is in use"

#define QERR_DEVICE_NO_HOTPLUG \
    "Device '%s' does not support hotplugging"

#define QERR_FD_NOT_FOUND \
    "File descriptor named '%s' not found"

#define QERR_FD_NOT_SUPPLIED \
    "No file descriptor supplied via SCM_RIGHTS"

#define QERR_FEATURE_DISABLED \
    "The feature '%s' is not enabled"

#define QERR_INVALID_BLOCK_FORMAT \
    "Invalid block format '%s'"

#define QERR_INVALID_PARAMETER \
    "Invalid parameter '%s'"

#define QERR_INVALID_PARAMETER_TYPE \
    "Invalid parameter type for '%s', expected: %s"

#define QERR_INVALID_PARAMETER_VALUE \
    "Parameter '%s' expects %s"

#define QERR_INVALID_PASSWORD \
    "Password incorrect"

#define QERR_IO_ERROR \
    "An IO error has occurred"

#define QERR_JSON_PARSING \
    "Invalid JSON syntax"

#define QERR_MIGRATION_ACTIVE \
    "There's a migration process in progress"

#define QERR_MISSING_PARAMETER \
    "Parameter '%s' is missing"

#define QERR_PERMISSION_DENIED \
    "Insufficient permission to perform this operation"

#define QERR_PROPERTY_VALUE_BAD \
    "Property '%s.%s' doesn't take value '%s'"

#define QERR_PROPERTY_VALUE_OUT_OF_RANGE \
    "Property %s.%s doesn't take value %" PRId64 " (minimum: %" PRId64 ", maximum: %" PRId64 ")"

#define QERR_QGA_COMMAND_FAILED \
    "Guest agent command failed, error was '%s'"

#define QERR_SET_PASSWD_FAILED \
    "Could not set password"

#define QERR_UNDEFINED_ERROR \
    "An undefined error has occurred"

#define QERR_UNSUPPORTED \
    "this feature or command is not currently supported"

#define QERR_REPLAY_NOT_SUPPORTED \
"Record/replay feature is not supported for '%s'"

#endif /* QAPI_QMP_QERROR_H_ */
