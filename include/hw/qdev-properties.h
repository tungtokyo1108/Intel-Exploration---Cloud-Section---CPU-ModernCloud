/*
 * qdev-properties.h
 *
 *  Created on: Sep 17, 2018
 *      Student (coder): Tung Dang
 */

#ifndef HW_QDEV_PROPERTIES_H_
#define HW_QDEV_PROPERTIES_H_

#include "hw/qdev-core.h"

#include "hw/qdev-core.h"


extern const PropertyInfo qdev_prop_bit;
extern const PropertyInfo qdev_prop_bit64;
extern const PropertyInfo qdev_prop_bool;
extern const PropertyInfo qdev_prop_uint8;
extern const PropertyInfo qdev_prop_uint16;
extern const PropertyInfo qdev_prop_uint32;
extern const PropertyInfo qdev_prop_int32;
extern const PropertyInfo qdev_prop_uint64;
extern const PropertyInfo qdev_prop_int64;
extern const PropertyInfo qdev_prop_size;
extern const PropertyInfo qdev_prop_string;
extern const PropertyInfo qdev_prop_chr;
extern const PropertyInfo qdev_prop_tpm;
extern const PropertyInfo qdev_prop_ptr;
extern const PropertyInfo qdev_prop_macaddr;
extern const PropertyInfo qdev_prop_on_off_auto;
extern const PropertyInfo qdev_prop_losttickpolicy;
extern const PropertyInfo qdev_prop_blockdev_on_error;
extern const PropertyInfo qdev_prop_bios_chs_trans;
extern const PropertyInfo qdev_prop_fdc_drive_type;
extern const PropertyInfo qdev_prop_drive;
extern const PropertyInfo qdev_prop_netdev;
extern const PropertyInfo qdev_prop_pci_devfn;
extern const PropertyInfo qdev_prop_blocksize;
extern const PropertyInfo qdev_prop_pci_host_devaddr;
extern const PropertyInfo qdev_prop_uuid;
extern const PropertyInfo qdev_prop_arraylen;
extern const PropertyInfo qdev_prop_link;
extern const PropertyInfo qdev_prop_off_auto_pcibar;

#define DEFINE_PROP(_name, _state, _field, _prop, _type) {  \
	.name = (_name),                                        \
	.info = &(_prop),                                       \
	.offset = offsetof(_state,_field)                       \
	+type_check(_type, typeof_field(_state,_field)),        \
	}

#define DEFINE_PROP_SIGNED(_name, _state, _field, _defval, _prop, _type)  { \
	.name = (_name),                                                        \
	.info = &(_prop),                                                       \
	.offset = offsetof(_state, _field)                                      \
	+ type_check(_type,typeof_field(_state,_field)),                        \
	.set_default = true,                                                    \
	.defval.i = (_type)_defval,                                             \
	}

#define DEFINE_PROP_SIGNED_NODEFAULT(_name, _state, _field, _prop, _type) { \
		.name = (_name),                                                    \
		.info = &(_prop),                                                   \
		.offset = offsetof(_state,_field)                                   \
		+type_check(_type, typeof_field(_state,_field)),                    \
	}

#define DEFINE_PROP_BIT(_name, _state, _field, _bit, _defval) { \
	.name = (_name),                                            \
	.info = &(_prop),                                           \
	.offset = offsetof(_state,_field)                           \
	+type_check(_type,typeof_field(_state,_field)),             \
	.set_default = true,                                        \
	.defval.u = (bool)_defval,                                  \
	}

#define DEFINE_PROP_UNSIGNED(_name, _state, _field, _defval, _prop, _type) { \
	.name = (_name),                                                         \
	.info = &(_prop),                                                        \
	.offset = offsetof(_state,_field)                                        \
	+type_check(_type,typeof_field(_state,_field)),                          \
	.set_default = true,                                                     \
	.defval.u = (_type)_defval,                                              \
	}

#define DEFINE_PROP_UNSIGNED_NODEFAULT(_name, _state, _field, _prop, _type) { \
		.name = (_name),                                                      \
		.info = &(_prop),                                                     \
		.offset = offsetof(_state,_field)                                     \
		+type_check(_type, typeof_field(_state,_field)),                      \
	}

#define DEFINE_PROP_BIT64(_name, _state, _field, _bit, _defval) {         \
	.name = (_name),                                                      \
	.info = &(qdev_prop_bit64),                                           \
	.bitnr = (_bit),                                                      \
	.offset = offsetof(_state,_field)                                     \
	+type_check(_uint64_t,typeof_field(_state,_field)),                       \
	.set_default = true,                                                  \
	.defval.u = (bool)_defval,                                            \
	}

#define DEFINE_PROP_BIT64(_name, _state, _field, _defval) {               \
	.name = (_name),                                                      \
	.info = &(qdev_prop_bool),                                            \
	.offset = offsetof(_state,_field)                                     \
	+type_check(bool,typeof_field(_state,_field)),                        \
	.set_default = true,                                                  \
	.defval.u = (bool)_defval,                                            \
	}

#define PROP_ARRAY_LEN_PREFIX "len-"
#define DEFINE_PROP_ARRAY(_name, _state, _field, _arrayfield, _arrayprop, _arraytype)  { \
	.name = (PROP_ARRAY_LEN_PREFIX _name),                                               \
	.info = &(qdev_prop_arraylen),                                                       \
	.set_default = true,                                                                 \
	.defval.u = 0,                                                                       \
	.offset = offsetof(_state, _field)                                                   \
	+type_check(uint32_t, typeof_field(_state,_field)),                                  \
	.arrayinfo = &(_arrayprop),                                                          \
	.arrayfieldsize = sizeof(_arraytype),                                                \
	.arrayoffset = offsetof(_state,_arrayfield),                                         \
	}

#define DEFINE_PROP_LINK(_name, _state, _field, _type, _ptr_type) {               \
	.name = (_name),                                                              \
	.info = &(qdev_prop_link),                                                    \
	.offset = offsetof(_state,_field)                                             \
	+type_check(bool,typeof_field(_state,_field)),                                \
	.link_type = _type,                                                           \
	}

#define DEFINE_PROP_UINT8(_n, _s, _f, _d)                       \
    DEFINE_PROP_UNSIGNED(_n, _s, _f, _d, qdev_prop_uint8, uint8_t)
#define DEFINE_PROP_UINT16(_n, _s, _f, _d)                      \
    DEFINE_PROP_UNSIGNED(_n, _s, _f, _d, qdev_prop_uint16, uint16_t)
#define DEFINE_PROP_UINT32(_n, _s, _f, _d)                      \
    DEFINE_PROP_UNSIGNED(_n, _s, _f, _d, qdev_prop_uint32, uint32_t)
#define DEFINE_PROP_INT32(_n, _s, _f, _d)                      \
    DEFINE_PROP_SIGNED(_n, _s, _f, _d, qdev_prop_int32, int32_t)
#define DEFINE_PROP_UINT64(_n, _s, _f, _d)                      \
    DEFINE_PROP_UNSIGNED(_n, _s, _f, _d, qdev_prop_uint64, uint64_t)
#define DEFINE_PROP_INT64(_n, _s, _f, _d)                      \
    DEFINE_PROP_SIGNED(_n, _s, _f, _d, qdev_prop_int64, int64_t)
#define DEFINE_PROP_SIZE(_n, _s, _f, _d)                       \
    DEFINE_PROP_UNSIGNED(_n, _s, _f, _d, qdev_prop_size, uint64_t)
#define DEFINE_PROP_PCI_DEVFN(_n, _s, _f, _d)                   \
    DEFINE_PROP_SIGNED(_n, _s, _f, _d, qdev_prop_pci_devfn, int32_t)

#define DEFINE_PROP_PTR(_n, _s, _f)             \
    DEFINE_PROP(_n, _s, _f, qdev_prop_ptr, void*)

#define DEFINE_PROP_CHR(_n, _s, _f)             \
    DEFINE_PROP(_n, _s, _f, qdev_prop_chr, CharBackend)
#define DEFINE_PROP_STRING(_n, _s, _f)             \
    DEFINE_PROP(_n, _s, _f, qdev_prop_string, char*)
#define DEFINE_PROP_NETDEV(_n, _s, _f)             \
    DEFINE_PROP(_n, _s, _f, qdev_prop_netdev, NICPeers)
#define DEFINE_PROP_DRIVE(_n, _s, _f) \
    DEFINE_PROP(_n, _s, _f, qdev_prop_drive, BlockBackend *)
#define DEFINE_PROP_MACADDR(_n, _s, _f)         \
    DEFINE_PROP(_n, _s, _f, qdev_prop_macaddr, MACAddr)
#define DEFINE_PROP_ON_OFF_AUTO(_n, _s, _f, _d) \
    DEFINE_PROP_SIGNED(_n, _s, _f, _d, qdev_prop_on_off_auto, OnOffAuto)
#define DEFINE_PROP_LOSTTICKPOLICY(_n, _s, _f, _d) \
    DEFINE_PROP_SIGNED(_n, _s, _f, _d, qdev_prop_losttickpolicy, \
                        LostTickPolicy)
#define DEFINE_PROP_BLOCKDEV_ON_ERROR(_n, _s, _f, _d) \
    DEFINE_PROP_SIGNED(_n, _s, _f, _d, qdev_prop_blockdev_on_error, \
                        BlockdevOnError)
#define DEFINE_PROP_BIOS_CHS_TRANS(_n, _s, _f, _d) \
    DEFINE_PROP_SIGNED(_n, _s, _f, _d, qdev_prop_bios_chs_trans, int)
#define DEFINE_PROP_BLOCKSIZE(_n, _s, _f) \
    DEFINE_PROP_UNSIGNED(_n, _s, _f, 0, qdev_prop_blocksize, uint16_t)
#define DEFINE_PROP_PCI_HOST_DEVADDR(_n, _s, _f) \
    DEFINE_PROP(_n, _s, _f, qdev_prop_pci_host_devaddr, PCIHostDeviceAddress)
#define DEFINE_PROP_MEMORY_REGION(_n, _s, _f)             \
    DEFINE_PROP(_n, _s, _f, qdev_prop_ptr, MemoryRegion *)
#define DEFINE_PROP_OFF_AUTO_PCIBAR(_n, _s, _f, _d) \
    DEFINE_PROP_SIGNED(_n, _s, _f, _d, qdev_prop_off_auto_pcibar, \
OffAutoPCIBAR)

#define DEFINE_PROP_UUID(_name, _state, _field) {                             \
	.name = (_name),                                                          \
	.info = &(qdev_prop_uuid),                                                \
	.offset = offsetof(_state,_field)                                         \
	+type_check(QemuUUID,typeof_field(_state,_field)),                        \
	.set_default = true,                                                      \
	}

#define DEFINE_PROP_END_OF_LIST()               \
    {}

void *qdev_get_prop_ptr(DeviceState *dev, Property *prop);
void qdev_prop_set_bit(DeviceState *dev, const char *name, bool value);
void qdev_prop_set_uint8(DeviceState *dev, const char *name, uint8_t value);
void qdev_prop_set_uint16(DeviceState *dev, const char *name, uint16_t value);
void qdev_prop_set_uint32(DeviceState *dev, const char *name, uint32_t value);
void qdev_prop_set_int32(DeviceState *dev, const char *name, int32_t value);
void qdev_prop_set_uint64(DeviceState *dev, const char *name, uint64_t value);
void qdev_prop_set_string(DeviceState *dev, const char *name, const char *value);
void qdev_prop_set_chr(DeviceState *dev, const char *name, Chardev *value);
void qdev_prop_set_netdev(DeviceState *dev, const char *name, NetClientState *value);
void qdev_prop_set_drive(DeviceState *dev, const char *name, BlockBackend *value, Error **errp);
void qdev_prop_set_macaddr(DeviceState *dev, const char *name, const uint8_t *value);
void qdev_prop_set_enum(DeviceState *dev, const char *name, int value);
void qdev_prop_set_ptr(DeviceState *dev, const char *name, void *value);
void qdev_prop_register_global(GlobalProperty *prop);
void qdev_prop_register_global_list(GlobalProperty *props);
int qdev_prop_check_globals(void);
void qdev_prop_set_globals(DeviceState *dev);
void error_set_from_qdev_prop_error(Error **errp, int ret, DeviceState *dev, Property *prop, const char *value);

void register_compat_prop(const char *driver, const char *property, const char *value);
void register_compat_props_array(GlobalProperty *prop);
void qdev_property_add_static(DeviceState *dev, Property *prop, Error **errp);
void qdev_alias_all_properties(DeviceState *target, Object *source);
void qdev_prop_set_after_realize(DeviceState *dev, const char *name, Error **errp);
void qdev_prop_allow_set_link_before_realize(const Object *obj, const char *name, Object *val, Error **errp);

#endif /* HW_QDEV_PROPERTIES_H_ */
