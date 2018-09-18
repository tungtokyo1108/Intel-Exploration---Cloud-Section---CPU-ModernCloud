/*
 * accel.h
 *
 *  Created on: Sep 17, 2018
 *      Student (coder): Tung Dang
 */

#ifndef SYSEMU_ACCEL_H_
#define SYSEMU_ACCEL_H_

#include "qom/object.h"
#include "hw/qdev-properties.h"

typedef struct AccelState {
	Object parent_obj;
} AccelState;

typedef struct AccelClass {
	ObjectClass parent_class;

	const char *opt_name;
	const char *name;
	int (*available)(void);
	int (*int_machine)(MachineState *ms);
	void (*setup_post)(MachineState *ms, AccelState *accel);
	bool *allowed;
	GlobalProperty *global_props;
} AccelClass;

#define TYPE_ACCEL "accel"
#define ACCEL_CLASS_SUFFIX "-" TYPE_ACCEL
#define ACCEL_CLASS_NAME(a) (a ACCEL_CLASS_SUFFIX)

#define ACCEL_CLASS(klass) \
	OBJECT_CLASS_CHECK(AccelClass, (klass), TYPE_ACCEL)
#define ACCEL(obj) \
	OBJECT_CHECK(AccelState, (obj), TYPE_ACCEL)
#define ACCEL_GET_CLASS(obj) \
	OBJECT_GET_CLASS(AccelClass, (obj), TYPE_ACCEL)

extern unsigned long tcg_tb_size;
void configure_accelerator(MachineState *ms);
void accel_register_compat_props(AccelState *accel);
void accel_setup_post(MachineState *ms);

#endif /* SYSEMU_ACCEL_H_ */
