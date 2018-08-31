/*
 * throttle-groups.h
 *
 *  Created on: Aug 31, 2018
 *      Student (coder): Tung Dang
 */

#ifndef BLOCK_THROTTLE_GROUPS_H_
#define BLOCK_THROTTLE_GROUPS_H_

#include "qemu/throttle.h"
#include "block/block_int.h"

typedef struct ThrottleGroupMember ThrottleGroupMember;
typedef struct ThrottleGroupMember {
	AioContext *aio_context;
	CoMutex throttled_reqs_lock;
	CoQueue throttled_reqs[2];
	unsigned int io_limits_disabled;
	ThrottleState *throttle_state;
	ThrottleTimers throttle_timers;
	unsigned pending_reqs[2];
	QLIST_ENTRY(ThrottleGroupMember) round_robin;
} ThrottleGroupMember;

#define TYPE_THROTTLE_GROUP "throttle-group"
#define THROTTLE_GROUP(obj) OBJECT_CHECK(ThrottleGroup, (obj), TYPE_THROTTLE_GROUP)

const char *throttle_group_get_name(ThrottleGroupMember *tgm);
ThrottleState *throttle_group_incref(const char *name);
void throttle_group_unref(ThrottleState *ts);
void throttle_group_config(ThrottleGroupMember *tgm, ThrottleConfig *cfg);
void throttle_group_get_config(ThrottleGroupMember *tgm, ThrottleConfig *cfg);
void throttle_group_register_tgm(ThrottleGroupMember *tgm,
                                const char *groupname, AioContext *ctx);
void throttle_group_unregister_tgm(ThrottleGroupMember *tgm);
void throttle_group_restart_tgm(ThrottleGroupMember *tgm);
void coroutine_fn throttle_group_co_io_limits_intercept(ThrottleGroupMember *tgm,
                                                        unsigned int bytes, bool is_write);
void throttle_group_attach_aio_context(ThrottleGroupMember *tgm, AioContext *new_context);
void throttle_group_detach_aio_context(ThrottleGroupMember *tgm);
bool throttle_group_exists(const char *name);

#endif /* BLOCK_THROTTLE_GROUPS_H_ */
