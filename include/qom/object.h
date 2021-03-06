/*
 * object.h
 * https://github.com/intel/nemu/blob/topic/virt-x86/include/qom/object.h
 *
 *  Created on: Sep 2, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#ifndef QOM_OBJECT_H_
#define QOM_OBJECT_H_

#include "qemu/queue.h"
#include "qemu/osdep.h"
#include "qemu/typedefs.h"

typedef struct TypeImpl TypeImpl;
typedef struct TypeImpl *Type;
typedef struct ObjectClass ObjectClass;
typedef struct Object Object;
typedef struct TypeInfo TypeInfo;
typedef struct InterfaceClass InterfaceClass;
typedef struct InterfaceInfo InterfaceInfo;
typedef struct ObjectProperty ObjectProperty;

typedef struct gchar gchar;
typedef struct GSList GSList;
typedef struct GHashTable GHashTable;
typedef struct GHashTableIter GHashTableIter;
typedef struct QEnumLookup QEnumLookup;

#define TYPE_OBJECT "object"

/**
* Interfaces for creating new types and objects.
* 
* Goals: Provides a framework for registering user creable types and instantiating objects from those types.
* Main features:
* - System for dynamically registering types.
* - Support for single-inheritance of types
* - Multiple inheritance of stateless interfaces. 
* 
* A new #Object derivative will be instantiated. 
* An #Object is casted to a subclass type using object_dynamic_cast(). 
* Macro wrappers should be defined around OBJECT_CHECK() and OBJECT_CLASS_CHECK()
* to make it easier to convert to specific type. 
*/

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
	ObjectPropertyAccessor *resolve;
	ObjectPropertyAccessor *release;
	void *opaque;
} OjectProperty;

typedef void (ObjectUnparent)(Object *obj); // when object is being removed from the composition tree
typedef void (ObjectFree)(void *obj); // when an object's last reference is removed
#define OBJECT_CLASS_CAST_CACHE 4

/*
* The ObjectClass derivatives 
* - are instantiated dynamically but there is only ever one instance for any given type.
* - typically holds a table of function pointers for the virtual methods implemented by this type.  
* - Before an object is initialized, the class for the object must be initialized. 
*   There is only one class object for all instance objects that is created. 
* - Classses are initialized by first initializing any parent classes. 
*   After the parent class object has initialized, it will be copied into the current class object 
*   and any additional storage in the class object is zero filled. 
* - The effect of this is that classes automatically inherit any virtual function pointers 
* - Introducing new virtual methods requires a class to define its own struct 
*   and to add a .class_size member to the #TypeInfo. 
*/

typedef struct ObjectClass {
	Type type;
	GSList *interfaces;
	const char *object_cast_cache[OBJECT_CLASS_CAST_CACHE];
	const char *class_cast_cache[OBJECT_CLASS_CAST_CACHE];
	ObjectUnparent *unparent;
	GHashTable *properties;
} ObjectClass;

struct Object {
	ObjectClass *class;
	ObjectFree *free;
	GHashTable *properties;
	uint32_t ref;
	Object *parent;
};

/**
 * TypeInfo describes information about the type including what it inherits from,
   the instance and class size, and constructor/destructor hooks.
 */

typedef struct TypeInfo {
	const char *name;
	const char *parent;
	/**
	 * The size of the object. 
	 * If instance_size is 0, the size of object will be the size of parent object.
	*/
	size_t instance_size;
	/**
	 * This function is called to initialize an object.
	 * The parent class will have already been initialized so the type is only responsible
	   for initializing its own members. 
	 */
	void (*instance_init)(Object *obj);
	/**
	 * This function is called to finish initialization of an object.
	*/
	void (*instance_post_init)(Object *obj);
	/**
	 * This function is called during object destruction. 
	 * An object should only free the members that are unique to its type in this function. 
	*/
	void (*instance_finalize)(Object *obj);

	bool abstract;
	/**
	 * The size of the class object for this object. 
	 * If @class_size is 0, then the size of the class will be assumed to be size of parent class. 
	 * This allows a type to avoid implementing an explicit class type 
	   if they are not adding additional virtual functions.  
	*/
	size_t class_size;
	/**
	 * This function is called after all parent class initialization has ocurred to allow a class 
	   and to set its default virtual method pointers 
	   and to override virtual methods from a parent clss.  
	*/
	void (*class_init)(ObjectClass *kclass, void *data);
	/**
	 * This function is called for all base classes after all parent class initialization has occurred. 
	 * This is the function to use to undo the effects of memcpy from the parent class to the descendants. 
	*/
	void (*class_base_init)(ObjectClass *kclass, void *data);
	/**
	 * This function is called during class destruction 
	   and is meant to realese and dynamic parameters allocated by @class_init.  
	*/
	void (*class_finalize)(ObjectClass *kclass, void *data);
	/**
	 * Data to pass to the @class_init, @class_base_init and @class_finilize_functions.
	 * Can be useful when building dynamic classes. 
	*/
	void *class_data;

	InterfaceInfo *interfaces;
} TypeInfo;

// Convert an object to a @Object
#define OBJECT(obj) \
	((Object *)(obj))

// Convert a class to an @ObjectClass
#define OBJECT_CLASS(class)\
	((ObjectClass *)(class))

/**
 * Each class will define a macro based on this type to perform type safe dynamic_casts to this object type
 * @type: The class type to use for the return value
 * @obj: A derivative of @type to cast
 * @name: The QOM typename of @type
*/
#define OBJECT_CHECK(type,obj,name) \
	((type *)object_dynamic_cast_assert(OBJECT(obj),(name), __FILE__, __LINE__, __func__))

// This macro is wrapped by each type to perform type safe cast of a class to a specific class type
#define OBJECT_CLASS_CHECK(class_type,class,name) \
	((class_type *)object_class_dynamic_cast_assert(OBJECT_CLASS(class),(name),__FILE__, __LINE__, __func__))

// To provide a type safe macro to get a specific class type from an object
#define OBJECT_GET_CLASS(class, obj, name) \
	OBJECT_CLASS_CHECK(class, object_get_class(OBJECT(obj)),name)

/* Interface 
* - Allow a limited form of multiple inheritance. 
* - Instance are similar to normal types except for the fact that are only defined bt their classes
*   and never carry any state. 
* - can dynamically cast an object to one of its #Interface types and vice versa. 
*/

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

/**
 * @interface: the type to return
 * @obj: the object to convert to an interface 
 * @name: the interface type name 
*/
#define INTERFACE_CHECK(interface,obj,name) \
	((interface *)object_dynamic_cast_assert(OBJECT((obj)), (name), __FILE__, __LINE__, __func__))

/**
 * This function will initialize a new object using heap allocated memory
 * The returned object has a reference count of 1, and will be freed when 
   the last reference is dropped.
*/
Object *object_new(const char *type_name);
/**
 * @id: will be used when registering the object as a child of @parent in the composition tree.
*/
Object *object_new_with_props(const char *type_name, Object *parent, const char *id, Error **errp, ...) QEMU_SENTINEL;
Object *object_new_with_propv(const char *type_name, Object *parent, const char *id, Error **errp, va_list vargs);

/**
 * This function will set a list of properties on an existing object instance.
*/
int object_set_props(Object *obj, Error **errp, ...) QEMU_SENTINEL;
int object_set_propv(Object *obj, Error **errp, va_list vargs);

/**
 * @obj: A pointer to the memory to be used for the object. 
 * @size: The maximum size available at @obj for the object.
 * @typename: The name of the type of the object to instantiate. 
 * 
 * This function will initialize an object. 
 * The memory for object should be have already been allocated.
 * The returned object has a reference count of 1, and will be finalized when last reference is dropped. 
 */
void object_initialize(void *obj, size_t size, const char *type_name);
/**
 * The object will then be added as child property to a parent will object_property_add_child function.
*/
void object_initialize_child(Object *parentobj, const char *propname,
                             void *childobj, size_t size, const char *type, Error **errp, ...) QEMU_SENTINEL;
void object_initialize_childv(Object *parentobj, const char *propname,
                              void *childobj, size_t size, const char *type, Error **errp, va_list vargs);

/**
 * This function will determine if @obj is a @typename.
 * @obj can refer to an object or an interface associated with an object.
*/
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
// void object_property_get_uint16List(Object *obj, const char *name, uint16List **list, Error **errp);
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
