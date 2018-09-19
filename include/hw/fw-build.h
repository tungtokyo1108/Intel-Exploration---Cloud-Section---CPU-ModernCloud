/*
 * fw-build.h
 *
 *  Created on: Sep 19, 2018
 *      Student (coder): Tung Dang
 */

#ifndef HW_FW_BUILD_H_
#define HW_FW_BUILD_H_
#include "qemu/typedefs.h"
#include "qemu/osdep.h"

typedef struct BIOSLinker {
	GArray *cmd_blob;
	GArray *file_list;
} BIOSLinker;

bool bios_linker_loader_can_write_pointer(void);
BIOSLinker *bios_linker_loader_init(void);
void bios_linker_loader_alloc(BIOSLinker *linker, const char *file_name, GArray *file_blob, uint32_t alloc_align, bool alloc_fseg);
void bios_linker_loader_add_checksum(BIOSLinker *linker, const char *file, unsigned start_offset, unsigned size, 
				     unsigned checksum_offset);
void bios_linker_loader_add_pointer(BIOSLinker *linker, const char *dest_file, uint32_t dst_patched_offset, 
				    uint8_t dst_patched_size, const char *src_file, uint32_t src_offset);
void bios_linker_loader_write_pointer(BIOSLinker *linker, const char *dest_file, uint32_t dst_patched_offset,
		                              uint8_t dst_patched_size, const char *src_file, uint32_t src_offset);
void bios_linker_loader_cleanup(BIOSLinker *linker);

typedef struct AcpiConfiguration AcpiConfiguration;
typedef struct AcpiBuildState AcpiBuildState;
typedef struct AcpiMcfgInfo AcpiMcfgInfo;

typedef struct FirmwareBuildMethods {
	union {
		struct {
			GArray *(*rsdp)(GArray *table_data, BIOSLinker *linker, unsigned rsdt_tbl_offset);
			GArray *(*madt)(GArray *table_data, BIOSLinker *linker, MachineState *ms, AcpiConfiguration *conf);
			void (*setup)(MachineState *ms, AcpiConfiguration *conf);
			void (*mcfg)(GArray *table_data, BIOSLinker *linker, AcpiMcfgInfo *info);
			void (*srat)(GArray *table_data, BIOSLinker *linker, MachineState *machine, AcpiConfiguration *conf);
			void (*slit)(GArray *table_data, BIOSLinker *linker);
		} acpi;
	};
} FirmwareBuildMethods;

typedef struct FirmwareBuildState {
	union {
		struct {
			AcpiConfiguration *conf;
			AcpiConfiguration *state;
		} acpi;
	};
} FirmwareBuildState;

#endif /* HW_FW_BUILD_H_ */
