/*
 * module.h
 *
 *  Created on: Aug 16, 2018
 *      Author: tungdang
 */

#ifndef QEMU_MODULE_H_
#define QEMU_MODULE_H_

#define DSO_STAMP_FUN glue(qemu_stamp,CONFIG_STAMP)
#define DSO_STAMP_FUN_STR stringify(DSO_STAMP_FUN)

void qemu_module_dummy(void);
#define module_init(function,type) \
static void __attribute__((constructor)) do_qemu_init_## function(void) \
{\
	register_dso_module_init(function,type);                            \
}

typedef enum {
	MODULE_INIT_BLOCK,
    MODULE_INIT_OPTS,
	MODULE_INIT_QOM,
	MODULE_INIT_TRACE,
	MODULE_INIT_MAX
} module_init_type;

#define block_init(function) module_init(function, MODULE_INIT_BLOCK)
#define opts_init(function) module_init(function, MODULE_INIT_OPTS)
#define type_init(function) module_init(function, MODULE_INIT_QOM)
#define trace_init(function) module_init(function, MODULE_INIT_TRACE)

#define block_module_load_one(lib) module_load_one("block-",lib)
#define ui_module_load_one(lib) module_load_one("ui-",lib)
#define audio_module_load_one(lib) module_load_one("audio-",lib)

void register_module_init(void (*fn)(void), module_init_type type);
void register_dso_module_init(void (*fn)(void), module_init_type type);

void module_call_init(module_init_type type);
void module_load_one(const char *prefix, const char *lib_name);

#endif /* QEMU_MODULE_H_ */
