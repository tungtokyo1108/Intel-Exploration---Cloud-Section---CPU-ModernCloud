/*
 * qstring.h
 *
 *  Created on: Aug 31, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QAPI_QMP_QSTRING_H_
#define QAPI_QMP_QSTRING_H_

#include "qapi/qmp/qobject.h"

typedef struct QString QString;

typedef struct QString {
	QObjectBase_ base;
	char *string;
	size_t length;
	size_t capacity;
} QString;

QString *qstring_new(void);
QString *qstring_from_str(const char *str);
QString *qstring_from_substr(const char *str, size_t start, size_t end);
size_t qstring_get_length(const QString *qstring);
const char *qstring_get_str(const QString *qstring);
const char *qstring_get_try_str(const QString *qstring);
const char *qobject_get_try_str(const QObject *qstring);
void qstring_append_int(QString *qstring, int64_t value);
void qstring_append(QString *qstring, const char *str);
void qstring_append_chr(QString *qstring, int c);
bool qstring_is_equal(const QObject *x, const QObject *y);
void qstring_destroy_obj(QObject *obj);

#endif /* QAPI_QMP_QSTRING_H_ */
