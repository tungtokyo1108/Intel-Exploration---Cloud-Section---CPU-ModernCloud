/*
 * memattrs.h
 *
 *  Created on: Sep 4, 2018
 *      Student (coder): Tung Dang
 */

#ifndef EXEC_MEMATTRS_H_
#define EXEC_MEMATTRS_H_
#include <stdint.h>

/**
 * Every memory transaction has associated with it a set of attributes.
 * generic type: the ID of the bus master
 * a particular kind of bus: ARM Secure/NonSecure bit
 */

typedef struct MemTxAttrs MemTxAttrs;
typedef struct MemTxAttrs {
	unsigned int unspecified:1;
	/**
	 * ARM/AMBA: TrustZone Secure access
	 * x86: System Management Mode access
	 */
	unsigned int secure:1;
	// Memory access is usermode
	unsigned int user:1;
	// Request ID
	unsigned int requester_id:16;
} MemTxAttrs;

#define MEMTXATTRS_UNSPECIFIED ((MemTxAttrs) { .unspecified = 1 })
#define MEMTX_OK 0
#define MEMTX_ERROR             (1U << 0) /* device returned an error */
#define MEMTX_DECODE_ERROR      (1U << 1) /* nothing at that address */
typedef uint32_t MemTxResult;

#endif /* EXEC_MEMATTRS_H_ */
