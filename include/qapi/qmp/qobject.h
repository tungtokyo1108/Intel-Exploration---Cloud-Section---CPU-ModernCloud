/*
 * qobject.h
 *
 *  Created on: Aug 31, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QAPI_QMP_QOBJECT_H_
#define QAPI_QMP_QOBJECT_H_
#include "qemu/osdep.h"
#include "qemu/typedefs.h"

typedef struct QObjectBase_ QObjectBase_;
typedef struct QOject QObject;

typedef struct QObjectBase_ {
	QType type;
	size_t refcnt;
} QObjectBase_;

typedef struct QObject {
	QObjectBase_ base;
} QObject;



#endif /* QAPI_QMP_QOBJECT_H_ */
