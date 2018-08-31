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
typedef struct QType QType;

typedef struct QObjectBase_ {
	QType type;
	size_t refcnt;
} QObjectBase_;

typedef struct QObject {
	QObjectBase_ base;
} QObject;

#define QOBJECT(obj) ({               \
	typeof(obj) _obj = (obj);         \
	_obj ? container_of(&(_obj)->base, QObject, base) : NULL; \
})

#define QTYPE_CAST_TO_QNull     QTYPE_QNULL
#define QTYPE_CAST_TO_QNum      QTYPE_QNUM
#define QTYPE_CAST_TO_QString   QTYPE_QSTRING
#define QTYPE_CAST_TO_QDict     QTYPE_QDICT
#define QTYPE_CAST_TO_QList     QTYPE_QLIST
#define QTYPE_CAST_TO_QBool     QTYPE_QBOOL

QEMU_BUILD_BUG_MSG(QTYPE__MAX != 7, "The QTYPE_CAST_TO_* list needs to be extended");
#define qobject_to(type,obj)      \
	(((type*)qobject_check_type(obj,glue(QTYPE_CAST_TO_, type))))

// Initialize an oject to default values
static inline void qobject_init(QObject *obj, QType type) {
	// assert(QTYPE_NONE < type && type < QTYPE__MAX);
	obj->base.refcnt = 1;
	obj->base.type = type;
}

static inline void qobject_ref_impl(QObject *obj) {
	if (obj) {
		obj->base.refcnt++;
	}
}

bool qobject_is_equal(const QObject *x, const QObject *y);
void qobject_destroy(QObject *obj);
static inline void qobject_unref_impl(QObject *obj) {
	assert(!obj || obj->base.refcnt);
	if (obj && --obj->base.refcnt == 0) {
		qobject_destroy(obj);
	}
}

#define qobject_ref(obj) ({           \
	typeof(obj) _o = (obj);           \
	qobject_ref_impl(QOBJECT(_o));    \
	_o;                               \
})

#define qobject_unref(obj) qobject_unref_impl(QOBJECT(obj))

static inline QType qobject_type(const QObject *obj) {
	// assert(QTYPE_NONE < obj->base.type && obj->base.type < QTYPE__MAX);
	return obj->base.type;
}

static inline QObject *qobject_check_type(const QObject *obj, QType *type) {
	if (obj && qobject_type(obj) == type) {
		return (QObject *)obj;
	} else {
		return NULL;
	}
}

#endif /* QAPI_QMP_QOBJECT_H_ */
