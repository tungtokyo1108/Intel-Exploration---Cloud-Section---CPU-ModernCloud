/*
 * qnull.h
 *
 *  Created on: Aug 31, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QAPI_QMP_QNULL_H_
#define QAPI_QMP_QNULL_H_

#include "qapi/qmp/qobject.h"

typedef struct QNull QNull;

typedef struct QNull {
	QObjectBase_ base;
} QNull;

static inline QNull *qnull(void) {
	return qobject_ref(&qnull_);
}

bool qnull_is_equal(const QObject *x, const QObject *y);

#endif /* QAPI_QMP_QNULL_H_ */
