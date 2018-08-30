/*
 * job.h
 *
 *  Created on: Aug 28, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_JOB_H_
#define QEMU_JOB_H_

#include "qemu/queue.h"
#include "qemu/coroutine.h"
#include "block/aio.h"

typedef struct JobDriver JobDriver;
typedef struct JobTxn JobTxn;
typedef struct JobStatus JobStatus;
typedef struct Coroutine Coroutine;
typedef struct JobType JobType;
typedef struct JobVerb JobVerb;

typedef struct Job {
	char *id;
	const JobDriver *driver;
	int refcnt; // Reference count of the block job
	JobStatus status;
	AioContext *aio_context; // Aiocurrent to run the job corountine in
	Coroutine *co;
	QEMUTimer sleep_timer; // timer that is job sleep
	int pause_count;
	bool busy;
	bool paused;
	bool user_paused;
	bool cancelled;
	bool force_cancel; // the job should abort immediately without waiting for data to be in sync
	bool deferred_to_main_loop; // the job has deferred word in main-loop
	bool auto_finalize;
	bool auto_dismiss;
	int64_t progress_current;
	int64_t progress_total;
	char *error;
	int ret; // ret code to job_completed
	BlockCompletionFunc *cb; // complete function will call when the job complete
	void *opaque;
	NotifierList on_finalize_cancelled; // Notifiers called when a cancelled job is finished
	NotifierList on_finalize_completed;
	NotifierList on_pending; // Notifier called when the job transitions to Pending
	NotifierList on_ready; // Notifier called when job transitions to ready
	QLIST_ENTRY(Job) job_list;
	JobTxn *txn;
	QLIST_ENTRY(Job) txn_list;
} Job;

struct JobDriver {
	size_t instance_size;
	JobType job_type;
	CoroutineEntry *start;
	void coroutine_fn (*pause)(Job *job);
	void coroutine_fn (*resume)(Job *job);
	void (*user_resume)(Job *job); // the job is resumed by the user
	void (*complete)(Job *job, Error **errp); // completion must be triggered
	void (*drain)(Job *job); // drain any activities as required to ensure progress
	int (*prepare)(Job *job); // all the jobs belonging to the same transaction complete
	void (*commit)(Job *job);
	void (*abort)(Job *job); // all the jobs belonging to the same transaction fails
	void (*clean)(Job *job); // after a call to either .commit() or .abort()
	void (*free)(Job *job);
};

typedef enum JobCreateFlags {
	JOB_DEFAULT = 0x00,
	JOB_INTERNAL = 0x01,
	JOB_MANUAL_FINALIZE = 0x02,
	JOB_MANUAL_DISMISS = 0x04,
} JobCreateFlags;

JobTxn *job_txn_new(void); // add jobs to the transaction. All jobs in transaction either complete successfully or fail/cancel as a group
void job_txn_unref(JobTxn *txn); // release a reference that was acquired
void job_txn_add_job(JobTxn *txn, Job *job); // add job to the transaction
void *job_create(const char *job_id, const JobDriver *driver, JobTxn *txn,
                 AioContext *ctx, int flags, BlockCompletionFunc *cb,
                 void *opaque, Error **errp); // Create a new long-running job
void job_ref(Job *job); // add a reference to Job
void job_unref(Job *job); // release a reference
void job_progress_update(Job *job, uint64_t done); // Update the progress of the job
void job_progress_set_remaining(Job *job, uint64_t remaining); // set the expected end value of the progess counter of a job so that a completion percentage can calculate when progress is updated

/**
 * If a job has to conditionally perform a high-priority operation, this function is called with expected operation's length before
 */
void job_progress_increase_remaining(Job *job, uint64_t delta);
void job_event_cancelled(Job *job);
void job_event_completed(Job *job);
void job_enter_cond (Job *job, bool(*fn)(Job *job)); // Conditionally enter the job coroutine if the job is ready to run
void job_start(Job *job);
void job_enter(Job *job);
void coroutine_fn job_pause_point(Job *job); // jobs that perform lots of I/0 must call this between requests so that the job can be paused
void job_yield(Job *job);
void coroutine_fn job_sleep_ns(Job *job, int64_t ns);
JobType job_type(const Job *job);
const char *job_type_str(const Job *job);
bool job_is_internal(Job *job); // the job should not be visible to the management layer
bool job_is_cancelled(Job *job);
bool job_is_completed(Job *job);
bool job_is_ready(Job *job);
void job_pause(Job *job);
void job_resume(Job *job);
void job_user_pause(Job *job, Error **errp);
void job_user_resume(Job *job, Error **errp);
void job_drain(Job *job);
Job *job_next(Job *job); // get the next element from the list of block jobs
Job *job_get(Job *job);
int job_apply_verb(Job *job, JobVerb *verb, Error **errp); // check whether the verb can be applied to job in current state
void job_early_fail(Job *job);
void job_transition_to_ready(Job *job);
void job_completed(Job *job, int ret, Error *error);
void job_complete(Job *job, Error **errp);
void job_cancel(Job *job, bool force);
void job_user_cancel(Job *job, bool force, Error **errp);
int job_cancel_sync(Job *job);
void job_cancel_sync_all(void);
int job_complete_sync(Job *job, Error **errp);
void job_finalize(Job *job, Error **errp);
void job_dismiss(Job *job, Error **errp); // remove the concluded job from the query list and resets the passed pointer
typedef void JobDeferToMainLoopFn(Job *job, void *opaque);
void job_defer_to_main_loop(Job *job, JobDeferToMainLoopFn *fn, void *opaque);
int job_finish_sync(Job *job, void(*finish)(Job *, Error **errp), Error **errp);

#endif /* QEMU_JOB_H_ */
