/*
 * blockjob.h
 *
 *  Created on: Aug 27, 2018
 *      Student (coder): Tung Dang
 */

#ifndef BLOCK_BLOCKJOB_H_
#define BLOCK_BLOCKJOB_H_

#include "block/block.h"
#include "qemu/job.h"
#include "qemu/ratelimit.h"

typedef struct BlockJobDriver BlockJobDriver;
typedef struct BlockJobTxn BlockJobTxn;
#define BLOCK_JOB_SLICE_TIME 100000000ULL

typedef struct BlockJob {
	Job job;
	BlockBackend *blk; // the block device on which the job is operating
	BlockDeviceIoStatus iostatus;
	int64_t speed;
	RateLimit limit; // rate limiting data structure for implementing speed
	Error *blocker; // block other operatins when block job is running
	Notifier finalize_cancelled_notifier;
	Notifier finalize_completed_notifier;
	Notifier pending_notifier;
	Notifier ready_notifier;
	GSList *nodes;
} BlockJob;

BlockJob *block_job_next(BlockJob *job); // get the next element from the list of block jobs
BlockJob *block_job_get(const char *id); // get the block of identified by id
int block_job_add_bdrv(BlockJob *job, const char *name, BlockDriverState *bs, uint64_t perm, uint64_t shared_perm, Error **errp);
void block_job_remove_all_bdrv(BlockJob *job);
void block_job_set_speed(BlockJob *job, int64_t speed, Error **errp); // set a rate-limiting parameter for the job, the actual meaning may vary depending on the job
BlockJobInfo *block_job_query(BlockJob *job, Error **errp); // return inforamtion about job
void block_job_iostatus_reset(BlockJob *job); // Reset I/O status on job
bool block_job_is_internal(BlockJob *job);
const BlockJobDriver *block_job_driver(BlockJob *job);

#endif /* BLOCK_BLOCKJOB_H_ */
