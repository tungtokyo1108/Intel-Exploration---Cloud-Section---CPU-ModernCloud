/*
 * accounting.h
 *
 *  Created on: Aug 26, 2018
 *      Student (coder): Tung Dang
 */

#ifndef BLOCK_ACCOUNTING_H_
#define BLOCK_ACCOUNTING_H_

#include "qemu/timed-average.h"
#include "qemu/thread.h"

typedef struct BlockAcctTimedStats BlockAcctTimedStats;
typedef struct BlockAcctStats BlockAcctStats;

enum BlockAcctType {
	BLOCK_ACCT_READ,
	BLOCK_ACCT_WRITE,
	BLOCK_ACCT_FLUSH,
	BLOCK_MAX_IOTYPE,
};

typedef struct BlockAcctTimedStats {
	BlockAcctStats *stats;
	TimedAverage latency[BLOCK_MAX_IOTYPE];
	unsigned interval_length;
	QSLIST_ENTRY(BlockAcctTimedStats) entries;
} BlockAcctTimedStats;

typedef struct BlockLatencyHistogram {
	int nbins;
	uint64_t *boundaries;
	uint64_t *bins;
} BlockLatencyHistogram;

typedef struct BlockAcctStats {
	QemuMutex lock;
	uint64_t nr_bytes[BLOCK_MAX_IOTYPE];
	uint64_t nr_ops[BLOCK_MAX_IOTYPE];
	uint64_t invalid_ops[BLOCK_MAX_IOTYPE];
	uint64_t failed_ops[BLOCK_MAX_IOTYPE];
	uint64_t total_time_ns[BLOCK_MAX_IOTYPE];
	uint64_t merged[BLOCK_MAX_IOTYPE];
	int64_t last_access_time_ns;
	QSLIST_HEAD(, BlockAcctTimedStats) interval;
} BlockAcctStats;

typedef struct BlockAcctCookie {
	int64_t bytes;
	// int64_t start_time ns;
	enum BlockAcctType type;
} BlockAcctCookie;



#endif /* BLOCK_ACCOUNTING_H_ */
