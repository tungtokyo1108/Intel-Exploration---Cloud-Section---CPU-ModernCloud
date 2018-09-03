/*
 * object.h
 *
 *  Created on: Sep 2, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QOM_OBJECT_H_
#define QOM_OBJECT_H_

#include "qemu/queue.h"
#include "qemu/osdep.h"

typedef struct TypeImpl TypeImpl;
typedef struct TypeImpl *Type;
typedef struct ObjectClass ObjectClass;
typedef struct Object Object;
typedef struct TypeInfo TypeInfo;
typedef struct InterfaceClass InterfaceClass;
typedef struct InterfaceInfo InterfaceInfo;
typedef struct OjectProperty OjectProperty;


#define TYPE_OBJECT "object"

// Called when trying to get/set a property
typedef void (ObjectPropertyAccessor)(Object *obj, Visitor *v, const char *name, void *opaque, Error **errp);

// Resolve the @Object corresponding to property @part
typedef Object *(ObjectPropertyResolve)(Object *obj, void *opaque, const char *part);

// Called when a property is removerd from a object
typedef void (ObjectPropertyRelease)(Object *obj, const char *name, void *opaque);

typedef struct OjectProperty {
	gchar *name;
	gchar *type;
	gchar *desciption;
	ObjectPropertyAccessor *get;
	ObjectPropertyAccessor *set;
	ObjectPropertyAccessor *resole;
	ObjectPropertyAccessor *release;
	void *opaque;
} OjectProperty;

typedef void (ObjectUnparent)(Object *obj); // when object is being removed from the composition tree
typedef void (ObjectFree)(void *obj); // when an object's last reference is removed
#define OBJECT_CLASS_CAST_CACHE 4

typedef struct ObjectClass {
	Type type;
	GSList *interfaces;
	const char *object_cast_cache[OBJECT_CLASS_CAST_CACHE];
	const char *class_cast_cache[OBJECT_CLASS_CAST_CACHE];
	ObjectUnparent *unparent;
	GHashTable *properties;
} ObjectClass;

struct Oject {
	ObjectClass *clas;
	ObjectFree *free;
	GHashTable *properties;
	uint32_t ref;
	Object *parent;
};

typedef struct TypeInfo {
	const char *name;
	const char *parent;
	size_t instance_size;
	void (*instance_init)(Oject *obj);
	void (*instance_post_init)(Object *obj);
	void (*instance_finalize)(Object *obj);

	bool abstract;
	size_t class_size;
	void (*class_init)(ObjectClass *kclass, void *data);
	void (*class_base_init)(ObjectClass *kclass, void *data);
	void (*class_finalize)(ObjectClass *kclass, void *data);
	void *class_data;

	InterfaceInfo *interfaces;
} TypeInfo;

// Convert an object to a @Object
#define OBJECT(obj) \
	((Object *)(obj))

// Convert a class to an @ObjectClass
#define OBJECT_CLASS(class)\
	((ObjectClass *)(class))

// Each class will define a macro based on this type to perform type safe dynamic_casts to this object type
#define OBJECT_CHECK(type,obj,name) \
	((type *)object_dynamic_cast_assert(OBJECT(obj),(name), __FILE__, __LINE__, __func__))

// This macro is wrapped by each type to perform type safe cast of a class to a specific class type
#define OBJECT_CLASS_CHECK(class_type,class,name) \
	((class_type *)object_class_dynamic_cast_assert(OBJECT_CLASS(class),(name),__FILE__, __LINE__, __func__))

// To provide a type safe macro to get a specific class type from an object
#define OBJECT_GET_CLASS(class, obj, name) \
	OBJECT_CLASS_CHECK(class, object_get_class(OBJECT(obj)),name)

typedef struct InterfaceInfo {
	const char *type;
} InterfaceInfo;

typedef struct InterfaceClass {
	ObjectClass parent_class;
	ObjectClass *concrete_class;
	Type interface_type;
} InterfaceClass;

#define TYPE_INTERFACE "interface"

#define INTERFACE_CLASS(kclass) \
	OBJECT_CLASS_CHECK(InterfaceClass, kclass, TYPE_INTERFACE)

#define INTERFACE_CHECK(interface,obj,name) \
	((interface *)object_dynamic_cast_assert(OBJECT((obj)), (name), __FILE__, __LINE__, __func__))

// this function will initialize a new object using heap allocated memory
Object *object_new(const char *type_name);
Object *object_new_with_props(const char *type_name, Object *parent, const char *id, Error **errp, ...) QEMU_SENTINEL;
Object *object_new_with_propv(const char *type_name, Object *parent, const char *id, Error **errp, va_list vargs);

int object_set_props(Object *obj, Error **errp, ...) QEMU_SENTINEL;
int object_set_propv(Object *obj, Error **errp, va_list vargs);

void object_initialize(void *obj, size_t size, const char *type_name);
void object_initialize_child(Object *parentobj, const char *propname,
                             void *childobj, size_t size, const char *type, Error **errp, ...) QEMU_SENTINEL;
void object_initialize_childv(Object *parentobj, const char *propname,
                              void *childobj, size_t size, const char *type, Error **errp, va_list vargs);

#endif /* QOM_OBJECT_H_ */
