/*
 * qjson.h
 *
 *  Created on: Aug 31, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QAPI_QMP_QJSON_H_
#define QAPI_QMP_QJSON_H_

#include "qapi/qmp/qstring.h"
#include "qapi/qmp/qobject.h"

#define GCC_ATTR_ __attribute__((__unused__, format(printf,1,2)))
#define GCC_FMT_ATTR(n,m) __attribute__((format(printf,n,m)))

QObject *qobject_from_json(const char *string, Error **errp);
QObject *qobject_from_jsonf(const char *string, Error **errp);
QObject *qobject_from_jsonv(const char *string, va_list *ap, Error **errp) GCC_FMT_ATTR(1,0);
QDict *qdict_from_jsonf_nofall(const char *string, ...) GCC_FMT_ATTR(1,2);

QString *qobject_to_json(const QObject *obj);
QString *qobject_to_json_pretty(const QObject *obj);

#endif /* QAPI_QMP_QJSON_H_ */
