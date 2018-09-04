/*
 * nmi.h
 *
 *  Created on: Sep 4, 2018
 *      Student (coder): Tung Dang
 */

#ifndef HW_NMI_H_
#define HW_NMI_H_

#include "qemu-common.h"
#include "qom/object.h"

#define TYPE_NMI "nmi"

#define NMI_CLASS(klass) \
	OBJECT_CLASS_CHECK(NMIClass,(klass),TYPE_NMI)
#define NMI_GET_CLASS(obj) \
	OBJECT_GET_CLASS(NMIClass,(obj),TYPE_NMI)
#define NMI(obj) \
	INTERFACE_CHECK(NMI,(obj),TYPE_NMI)

typedef struct NMIState NMIState;
typedef struct NMIState {
	Object parent_obj;
} NMIState;

typedef struct NMIClass NMIClass;
typedef struct NMIClass {
	InterfaceClass parent_class;
	void (*nmi_monitor_handler)(NMIState *n, int cpu_index, Error **errp);
} NMIClass;

void nmi_monitor_handle(int cpu_index, Error **errp);

#endif /* HW_NMI_H_ */
