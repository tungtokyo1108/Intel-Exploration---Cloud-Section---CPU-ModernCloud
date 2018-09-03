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

Object *object_dynamic_cast(Object *obj, const char *type_name);
Object *object_dynamic_cast_assert(Object *obj, const char *type_name, const char *file, int line, const char *func);
ObjectClass *object_get_class(Object *obj);
const char *object_get_typename(const Object *obj);

// info and all of strings it points to should exist for the left time
Type type_register_static(const TypeInfo *info);
void type_register_static_array(const TypeInfo *info, int nr_infos);
// does not require @info or its string members to continue to exit after the call return
Type type_register(const TypeInfo *info);

#define DEFINE_TYPES(type_array) \
	static void do_qemu_init_## type_array(void) \
	{                                            \
	type_register_static_array(type_array,ARRAY_SIZE(type_array)); \
	}\
	type_init(do_qemu_init_## type_array)

ObjectClass *object_class_dynamic_cast_assert(ObjectClass *klass,
                                              const char *type_name,
                                              const char *file, int line, const char *func);
ObjectClass *object_class_dynamic_cast(ObjectClass *klass, const char *type_name);
ObjectClass *object_class_get_parent(ObjectClass *klass);
const char *object_class_get_name(ObjectClass *klass);
bool object_class_is_abstract(ObjectClass *kclass);
ObjectClass *object_class_by_name(const char *type_name);
void object_class_foreach(void (*fn)(ObjectClass *klass, void *opaque),
                          const char *implements_type, bool include_abstract, void *opaque);
GSList *object_class_get_list(const char *implements_type, bool include_abstract); // A single_linked List of the classes in reverse hash-table order
GSList *object_class_get_list_sorted(const char *implements_type, bool include_abstract); // A single-linked List of the classes in alphabetatical case-insentive order
void object_ref(Object *obj); // Increase the reference count of a object
void object_unref(Object *obj); // Decreae the reference count  of a object
ObjectProperty *object_property_add(Object *obj, const char *name,
                                    const char *type,
                                    ObjectPropertyAccessor *get,
                                    ObjectPropertyAccessor *set,
                                    ObjectPropertyRelease *release, void *opaque, Error **errp);
void object_property_del(Object *obj, const char *name, Error **errp);
ObjectProperty *object_class_property_add(ObjectClass *klass, const char *name,
                                          const char *type,
                                          ObjectPropertyAccessor *get,
                                          ObjectPropertyAccessor *set,
                                          ObjectPropertyRelease *release, void *opaque, Error **errp);
ObjectProperty *object_property_find(Object *obj, const char *name, Error **errp);
ObjectProperty *object_class_property_find(ObjectClass *klass, const char *name, Error **errp);

typedef struct ObjectPropertyIterator ObjectPropertyIterator;
typedef struct ObjectPropertyIterator {
	ObjectClass *nextclass;
	GHashTableIter iter;
} ObjectPropertyIterator;

void object_property_iter_init(ObjectPropertyIterator *iter, Object *obj);

/**
 * Initialize an iterator for traversing all properties registered against an object class and all parent classes
 * To modify the property list while iterating whether removing  or adding properties
 */
void object_class_property_iter_init(ObjectPropertyIterator *iter, ObjectClass *kclass);
ObjectProperty *object_property_iter_next(ObjectPropertyIterator *iter);
void object_unparent(Object *obj);

void object_property_get(Object *obj, Visitor *v, const char *name, Error **errp); // read property from a object
void object_property_set_str(Object *obj, const char *value, const char *name, Error **errp); // Write a string value to a property
char *object_property_get_str(Object *obj, const char *name, Error **errp); // the value of the property, convert to a C string

// Write an object's canonical path to a property
void object_property_set_link(Object *obj, Object *value, const char *name, Error **errp);
Object *object_property_get_link(Object *obj, const char *name, Error **errp); // the value of the property, resolve from a path to an object
void object_property_set_bool(Object *obj, bool value, const char *name, Error **errp);
bool object_property_get_bool(Object *obj, const char *name, Error **errp);

void object_property_set_int(Object *obj, int64_t value, const char *name, Error **errp);
int64_t object_property_get_int(Object *obj, const char *name, Error **errp);
void object_property_set_uint(Object *obj, uint64_t value, const char *name, Error **errp);
uint64_t object_property_get_uint(Object *obj, const char *name, Error **errp); // the value of the property, convert to an unsigned integer
int object_property_get_enum(Object *obj, const char *name, const char *type_name, Error **errp); // the value of the property, convert to an integer
void object_property_get_uint16List(Object *obj, const char *name, uint16List **list, Error **errp);
void object_property_set(Object *obj, Visitor *v, const char *name, Error **errp);

// Parse a string and write the result into a property of an object
void object_property_parse(Object *obj, const char *string, const char *name, Error **errp);
// A string representation of the value of the property
char *object_property_print(Object *obj, const char *name, bool human, Error **errp);
const char *object_property_get_type(Object *obj, const char *name, Error **errp);

Object *object_get_objects_root(void); // the user object container
Object *object_get_internal_root(void); // the internal object container
gchar *object_get_canonical_path_component(Object *obj); // The final component in the object's canonical path
gchar *object_get_canonical_path(Object *obj); // The canonical path for a object

/**
 * Absolute paths are derived from the root object and can follow child or link properties
 * Partial paths look like relative filenames, The matching rules for partial paths are subtle but designed to make specifying objects easy
 */
Object *object_resolve_path(const char *path, bool *ambigous);
Object *object_resolve_path_type(const char *path, const char *type_name, bool *ambigous);
Object *object_resolve_path_component(Object *parent, const gchar *part);

// Child property form the composition tree. All object need to be a child of another object.
// Objects can only be a child of one object

void object_property_add_child(Object *obj, const char *name, Object *child, Error **errp);

typedef enum {
    OBJ_PROP_LINK_STRONG = 0x1,
} ObjectPropertyLinkFlags;

void object_property_allow_set_link(const Object *, const char *, Object *, Error **);

/**
 * Links establish relationships between objects. Links are undirectional
 * Links form the graph in the object model
 * Ownership of pointer that @child points is to transferred to the link property
 */

void object_property_add_link(Object *obj, const char *name,
                              const char *type, Object **child,
                              void (*check)(const Object *obj, const char *name,
                                            Object *val, Error **errp),
                              ObjectPropertyLinkFlags flags, Error **errp);

// Add a string property using getter/setter. This function will add a property of type 'string'
void object_property_add_str(Object *obj, const char *name,
                             char *(*get)(Object *, Error **),
                             void (*set)(Object *, const char *, Error **), Error **errp);
void object_class_property_add_str(ObjectClass *klass, const char *name,
                                   char *(*get)(Object *, Error **),
                                   void (*set)(Object *, const char *,
                                               Error **), Error **errp);

void object_property_add_bool(Object *obj, const char *name,
                              bool (*get)(Object *, Error **),
                              void (*set)(Object *, bool, Error **), Error **errp);
void object_class_property_add_bool(ObjectClass *klass, const char *name,
                                    bool (*get)(Object *, Error **),
                                    void (*set)(Object *, bool, Error **), Error **errp);

// Add an enum property using getter/setters
void object_property_add_enum(Object *obj, const char *name,
                              const char *type_name,
                              const QEnumLookup *lookup,
                              int (*get)(Object *, Error **),
                              void (*set)(Object *, int, Error **), Error **errp);
void object_class_property_add_enum(ObjectClass *klass, const char *name,
                                    const char *type_name,
                                    const QEnumLookup *lookup,
                                    int (*get)(Object *, Error **),
                                    void (*set)(Object *, int, Error **), Error **errp);

// Add a read-only struct tm valued property using a getter function
void object_property_add_tm(Object *obj, const char *name,
                            void (*get)(Object *, struct tm *, Error **), Error **errp);
void object_class_property_add_tm(ObjectClass *klass, const char *name,
                                  void (*get)(Object *, struct tm *, Error **), Error **errp);

// Add an integer property in memory
void object_property_add_uint8_ptr(Object *obj, const char *name, const uint8_t *v, Error **errp);
void object_class_property_add_uint8_ptr(ObjectClass *klass, const char *name, const uint8_t *v, Error **errp);
void object_property_add_uint16_ptr(Object *obj, const char *name, const uint16_t *v, Error **errp);
void object_class_property_add_uint16_ptr(ObjectClass *klass, const char *name, const uint16_t *v, Error **errp);
void object_property_add_uint32_ptr(Object *obj, const char *name, const uint32_t *v, Error **errp);
void object_class_property_add_uint32_ptr(ObjectClass *klass, const char *name, const uint32_t *v, Error **errp);
void object_property_add_uint64_ptr(Object *obj, const char *name, const uint64_t *v, Error **errp);
void object_class_property_add_uint64_ptr(ObjectClass *klass, const char *name, const uint64_t *v, Error **errp);


void object_property_add_alias(Object *obj, const char *name,
                               Object *target_obj, const char *target_name, Error **errp);
void object_property_add_const_link(Object *obj, const char *name,Object *target, Error **errp);

void object_property_set_description(Object *obj, const char *name, const char *description, Error **errp);
void object_class_property_set_description(ObjectClass *klass, const char *name,
                                           const char *description, Error **errp);

int object_child_foreach(Object *obj, int (*fn)(Object *child, void *opaque), void *opaque);
int object_child_foreach_recursive(Object *obj,
                                   int (*fn)(Object *child, void *opaque), void *opaque);

Object *container_get(Object *root, const char *path);
size_t object_type_get_instance_size(const char *type_name);

#endif /* QOM_OBJECT_H_ */
