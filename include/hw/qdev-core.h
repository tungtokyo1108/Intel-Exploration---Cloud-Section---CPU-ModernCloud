/*
 * qdev-core.h
 *
 *  Created on: Sep 4, 2018
 *      Student (coder): Tung Dang
 */

#ifndef HW_QDEV_CORE_H_
#define HW_QDEV_CORE_H_

#include "qemu/queue.h"
#include "qemu/bitmap.h"
#include "qom/object.h"
#include "hw/irq.h"
#include "hw/hotplug.h"

enum {
    DEV_NVECTORS_UNSPECIFIED = -1,
};

#define TYPE_DEVICE "device"
#define DEVICE(obj) OBJECT_CHECK(DeviceState, (obj), TYPE_DEVICE)
#define DEVICE_CLASS(klass) OBJECT_CLASS_CHECK(DeviceClass, (klass), TYPE_DEVICE)
#define DEVICE_GET_CLASS(obj) OBJECT_GET_CLASS(DeviceClass, (obj), TYPE_DEVICE)

typedef enum DeviceCategory {
    DEVICE_CATEGORY_BRIDGE,
    DEVICE_CATEGORY_USB,
    DEVICE_CATEGORY_STORAGE,
    DEVICE_CATEGORY_NETWORK,
    DEVICE_CATEGORY_INPUT,
    DEVICE_CATEGORY_DISPLAY,
    DEVICE_CATEGORY_SOUND,
    DEVICE_CATEGORY_MISC,
    DEVICE_CATEGORY_CPU,
    DEVICE_CATEGORY_MAX
} DeviceCategory;

typedef void (*DeviceRealize)(DeviceState *dev, Error **errp);
typedef void (*DeviceUnrealize)(DeviceState *dev, Error **errp);
typedef void (*DeviceReset)(DeviceState *dev);
typedef void (*BusRealize)(BusState *bus, Error **errp);
typedef void (*BusUnrealize)(BusState *bus, Error **errp);

struct VMStateDescription;

typedef struct DeviceClass DeviceClass;
typedef struct DeviceClass {
	ObjectClass parent_class;
	DECLARE_BITMAP(categories,DEVICE_CATEGORY_MAX);
	const char *fw_name;
	const char *desc;
	Property *props;
	bool user_creatable;
	bool hotpluggable;

	DeviceReset reset;
	DeviceRealize realize;
	DeviceUnrealize unrealize;

	const struct VMStateDescription *vmsd;
	const char *bus_type;
} DeviceClass;

typedef struct NamedGPIOList NamedGPIOList;
typedef struct NamedGPIOList {
	char *name;
	qemu_irq *in;
	int num_in;
	int num_out;
	QLIST_ENTRY(NamedGPIOList) node;
} NamedGPIOList;

typedef struct DeviceState DeviceState;
typedef struct DeviceState {
	Object parent_obj;

	const char *id;
	char *canonical_path;
	bool realized;
	bool pending_deleted_event;
	QemuOpts *opts;
	int hotplugged;
	BusState *parent_bus;
	QLIST_HEAD(, NamedGPIOList) gpios;
	QLIST_HEAD(, BusState) child_bus;
	int num_child_bus;
	int instance_id_alias;
	int alias_required_for_version;
} DeviceState;

typedef struct DeviceListener DeviceListener;
typedef struct DeviceListener {
	void (*realize)(DeviceListener *listener, DeviceState *dev);
	void (*unrealize)(DeviceListener *listener, DeviceState *dev);
	QTAILQ_ENTRY(DeviceListener) link;
} DeviceListener;

#define TYPE_BUS "bus"
#define BUS(obj) OBJECT_CHECK(BusState, (obj), TYPE_BUS)
#define BUS_CLASS(klass) OBJECT_CLASS_CHECK(BusClass, (klass), TYPE_BUS)
#define BUS_GET_CLASS(obj) OBJECT_GET_CLASS(BusClass, (obj), TYPE_BUS)

typedef struct BusClass BusClass;
typedef struct BusClass {
	ObjectClass parent_class;
	void (*print_dev)(Monitor *mon, DeviceState *dev, int indent);
	char *(*get_dev_path)(DeviceState *dev);
	char *(*get_fw_dev_path)(DeviceState *dev);
	void (*reset)(BusState *bus);
	BusRealize realize;
	BusUnrealize unrealize;
	int max_dev;
	int automatic_ids;
} BusClass;

typedef struct BusChild BusChild;
typedef struct BusChild {
	DeviceState *child;
	int index;
	QTAILQ_ENTRY(BusChild) sibling;
} BusChild;

typedef struct BusState BusState;
typedef struct BusState {
	Object obj;
	DeviceState *parent;
	char *name;
	HotplugHandler *hotplug_handler;
	int max_index;
	bool realized;
	QTAILQ_HEAD(ChildrenHead, BusChild) children;
	QLIST_ENTRY(BusState) sibling;
} BusState;

typedef struct Property Property;
typedef struct Property {
	const char *name;
	const PropertyInfo *info;
	ptrdiff_t offset;
	uint8_t bitnr;
	bool set_default;
	union {
		int64_t i;
		uint64_t u;
	} defval;
	int arrayoffset;
	const PropertyInfo *arrayinfo;
	int arrayfieldsize;
	const char *link_type;
} Property;

typedef struct PropertyInfo PropertyInfo;
typedef struct PropertyInfo {
	const char *name;
	const char *description;
	const QEnumLookup *enum_table;
	int (*print)(DeviceState *dev, Property *prop, char *dest, size_t len);
	void (*set_default_value)(Object *obj, const Property *prop);
	void (*create)(Object *obj, Property *prop, Error **errp);
	ObjectPropertyAccessor *get;
	ObjectPropertyAccessor *set;
	ObjectPropertyAccessor *release;
} PropertyInfo;

typedef struct GlobalProperty GlobalProperty;
typedef struct GlobalProperty {
	const char *driver;
	const char *property;
	const char *value;
	bool user_provided;
	bool used;
	Error **errp;
} GlobalProperty;

DeviceState *qdev_create(BusState *bus, const char *name);
DeviceState *qdev_try_create(BusState *bus, const char *name);
void qdev_init_nofail(DeviceState *dev);
void qdev_set_legacy_instance_id(DeviceState *dev, int alias_id, int required_for_version);

HotplugHandler *qdev_get_machine_hotplug_handler(DeviceState *dev);
HotplugHandler *qdev_get_hotplug_handler(DeviceState *dev);

void qdev_unplug(DeviceState *dev, Error **errp);
void qdev_simple_device_unplug_cb(HotplugHandler *hotplug_dev, DeviceState *dev, Error **errp);

void qdev_machine_creation_done(void);
bool qdev_machine_modified(void);

qemu_irq qdev_get_gpio_in(DeviceState *dev, int n);
qemu_irq qdev_get_gpio_in_named(DeviceState *dev, const char *name, int n);

void qdev_connect_gpio_out(DeviceState *dev, int n, qemu_irq pin);
void qdev_connect_gpio_out_named(DeviceState *dev, const char *name, int n, qemu_irq pin);

qemu_irq qdev_get_gpio_out_connector(DeviceState *dev, const char *name, int n);
qemu_irq qdev_intercept_gpio_out(DeviceState *dev, qemu_irq icpt, const char *name, int n);

BusState *qdev_get_child_bus(DeviceState *dev, const char *name);

// Register device properties
void qdev_init_gpio_in(DeviceState *dev, qemu_irq_handler handler, int n);
void qdev_init_gpio_out(DeviceState *dev, qemu_irq *pins, int n);
void qdev_init_gpio_out_named(DeviceState *dev, qemu_irq *pins, const char *name, int n);

// Create an array of input GPON lines for the specific device
void qdev_init_gpio_in_named_with_opaque(DeviceState *dev,
                                         qemu_irq_handler handler,
                                         void *opaque, const char *name, int n);
static inline void qdev_init_gpio_in_named(DeviceState *dev,
                                           qemu_irq_handler handler,
                                           const char *name, int n)
{
    qdev_init_gpio_in_named_with_opaque(dev, handler, dev, name, n);
}

void qdev_pass_gpios(DeviceState *dev, DeviceState *container, const char *name);
BusState *qdev_get_parent_bus(DeviceState *dev);

DeviceState *qdev_find_recursive(BusState *bus, const char *id);
typedef int (qbus_walkerfn)(BusState *bus, void *opaque);
typedef int (qdev_walkerfn)(DeviceState *dev, void *opaque);

void qbus_create_inplace(void *bus, size_t size, const char *type_name, DeviceState *parent, const char *name);
BusState *qbus_create(const char *type_name, DeviceState *parent, const char *name);
int qbus_walk_children(BusState *bus,
                       qdev_walkerfn *pre_devfn, qbus_walkerfn *pre_busfn,
                       qdev_walkerfn *post_devfn, qbus_walkerfn *post_busfn, void *opaque);
int qdev_walk_children(DeviceState *dev,
                       qdev_walkerfn *pre_devfn, qbus_walkerfn *pre_busfn,
                       qdev_walkerfn *post_devfn, qbus_walkerfn *post_busfn, void *opaque);

void qdev_reset_all(DeviceState *dev);
void qdev_reset_all_fn(void *opaque);

void qbus_reset_all(BusState *bus);
void qbus_reset_all_fn(void *opaque);

BusState *sysbus_get_default(void);

char *qdev_get_fw_dev_path(DeviceState *dev);
char *qdev_get_own_fw_dev_path_from_handler(BusState *bus, DeviceState *dev);

void qdev_machine_init(void);
void device_reset(DeviceState *dev);

void device_class_set_parent_reset(DeviceClass *dc,
                                   DeviceReset dev_reset,DeviceReset *parent_reset);
void device_class_set_parent_realize(DeviceClass *dc,
                                     DeviceRealize dev_realize, DeviceRealize *parent_realize);
void device_class_set_parent_unrealize(DeviceClass *dc,
                                       DeviceUnrealize dev_unrealize, DeviceUnrealize *parent_unrealize);

const struct VMStateDescription *qdev_get_vmsd(DeviceState *dev);

const char *qdev_fw_name(DeviceState *dev);

Object *qdev_get_machine(void);

void qdev_set_parent_bus(DeviceState *dev, BusState *bus);

extern bool qdev_hotplug;
extern bool qdev_hot_removed;

char *qdev_get_dev_path(DeviceState *dev);

GSList *qdev_build_hotpluggable_device_list(Object *peripheral);

void qbus_set_hotplug_handler(BusState *bus, DeviceState *handler, Error **errp);

void qbus_set_bus_hotplug_handler(BusState *bus, Error **errp);

static inline bool qbus_is_hotpluggable(BusState *bus)
{
   return bus->hotplug_handler;
}

void device_listener_register(DeviceListener *listener);
void device_listener_unregister(DeviceListener *listener);

#endif /* HW_QDEV_CORE_H_ */
