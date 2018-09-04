/*
 * hotplug.h
 *
 *  Created on: Sep 4, 2018
 *      Student (coder): Tung Dang
 */

#ifndef HW_HOTPLUG_H_
#define HW_HOTPLUG_H_

#include "qom/object.h"

#define TYPE_HOTPLUG_HANDLER "hotplug-handler"

#define HOTPLUG_HANDLER_CLASS(klass) \
	OBJECT_CLASS_CHECK(HotplugHandlerClass, (klass), TYPE_HOTPLUG_HANDLER)
#define HOTPLUG_HANDLER_GET_CLASS(obj) \
	OBJECT_GET_CLASS(HotplugHandlerClass, (obj), TYPE_HOTPLUG_HANDLER)
#define HOTPLUG_HANDLER(obj) \
	INTERFACE_CHECK(HotplugHandler, (obj), TYPE_HOTPLUG_HANDLER)

typedef struct HotplugHandler HotplugHandler;
typedef struct HotplugHandler {
	Object Parent;
} HotplugHandler;

typedef void (*hotplug_fn)(HotplugHandler *plug_handler, DeviceState *plugged_dev, Error **errp);

// Interface to be implemented by a device performing hardware unplug functions

typedef struct HotplugHandlerClass HotplugHandlerClass;
typedef struct HotplugHandlerClass {
	InterfaceClass parent;
	hotplug_fn pre_plug;
	hotplug_fn plug;
	hotplug_fn unplug_request;
	hotplug_fn unplug;
	void (*post_plug)(HotplugHandler *plug_handler, DeviceState *plugged_dev);
} HotplugHandlerClass;

void hotplug_handler_plug(HotplugHandler *plug_handler,
                          DeviceState *plugged_dev, Error **errp);
void hotplug_handler_pre_plug(HotplugHandler *plug_handler,
                              DeviceState *plugged_dev, Error **errp);
void hotplug_handler_post_plug(HotplugHandler *plug_handler, DeviceState *plugged_dev);
void hotplug_handler_unplug_request(HotplugHandler *plug_handler,
                                    DeviceState *plugged_dev, Error **errp);
void hotplug_handler_unplug(HotplugHandler *plug_handler,
                            DeviceState *plugged_dev,Error **errp);

#endif /* HW_HOTPLUG_H_ */
