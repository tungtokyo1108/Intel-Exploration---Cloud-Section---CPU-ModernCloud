/*
 * qobject-output-visitor.h
 *
 *  Created on: Aug 31, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QAPI_QOBJECT_OUTPUT_VISITOR_H_
#define QAPI_QOBJECT_OUTPUT_VISITOR_H_

#include "qapi/visitor.h"

typedef struct QObjectOutputVisitor QObjectOutputVisitor;

Visitor *qobject_output_visitor_new(QObject **result);

#endif /* QAPI_QOBJECT_OUTPUT_VISITOR_H_ */
