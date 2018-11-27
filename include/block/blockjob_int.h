/*
 * blockjob_int.h
 *
 *  Created on: Nov 27, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#ifndef BLOCK_BLOCKJOB_INT_H_
#define BLOCK_BLOCKJOB_INT_H_

#include "block/blockjob.h"
#include "block/block.h"

/*
 * A class type for block job driver
 */
struct BlockJobDriver {
	JobDriver job_river;

	/*
	 * Return whether the job has pending requests for the child or
	 * will submit new requests before the next pause point.
	 * This callback is polled in the context of draining a job node
	 * after requesting that the job be paused,
	 * until all activity on the child has stopped.
	 */
	bool (*drainer_poll)(BlockJob *job);

	/*
	 * If the callback is not NULL, it will be invoked before the job
	 * is resumed in a new AioContext.
	 * This is the place to move any resources besides job->bik to
	 * the new AioContext.
	 */
	void (*attached_aio_context)(BlockJob *job, AioContext *new_context);

	/*
	 * If the callback is not NULL, it will be invoked when the job
	 * has to be sychronously cancelled or completed; it should
	 * drain BlockDriverStates as required to ensure progress
	 */
	void (*drain)(BlockJob *job);
};

/*
 * Create a new long-running block device job and return it.
 * The job will call @cb asynchronousky when job completes
 */
void *block_job_create(const char *job_id, const BlockJobDriver *driver,
		               JobTxn *txn, BlockDriverState *bs, uint64_t perm,
					   uint64_t shared_perm, int64_t speed, int flags,
					   BlockCompletionFunc *cb, void* opaque, Error **errp);

/*
 * Frees block job specific resources in @job
 */
void block_job_free(Job *job);

/*
 * Resets the iostatus when user resumes @job.
 */
void block_job_user_resume(Job *job);

/*
 * Drains the main block node associated with block jobs and calls BlockJobDriver.
 * drain for job-specific actions.
 */
void block_job_drain(Job *job);

/*
 * Calculate and return delay the next request in ns.
 */
int64_t block_job_ratelimit_get_delay(BlockJob *job, uint64_t n);

BlockErrorAction block_job_error_action(BlockJob *job, BlockdevOnError on_err,
		                                int is_read, int error);

#endif /* BLOCK_BLOCKJOB_INT_H_ */
