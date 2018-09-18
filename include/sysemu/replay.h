/*
 * replay.h
 *
 *  Created on: Sep 19, 2018
 *      Student (coder): Tung Dang
 */

#ifndef SYSEMU_REPLAY_H_
#define SYSEMU_REPLAY_H_

#include "sysemu/sysemu.h"

enum ReplayClockKind {
	REPLAY_CLOCK_HOST,
	REPLAY_CLOCK_VIRTUAL_RT,
	REPLAY_CLOCK_COUNT
};

typedef enum ReplayClockKind ReplayClockKind;

enum ReplayCheckpoint {
	    CHECKPOINT_CLOCK_WARP_START,
	    CHECKPOINT_CLOCK_WARP_ACCOUNT,
	    CHECKPOINT_RESET_REQUESTED,
	    CHECKPOINT_SUSPEND_REQUESTED,
	    CHECKPOINT_CLOCK_VIRTUAL,
	    CHECKPOINT_CLOCK_HOST,
	    CHECKPOINT_CLOCK_VIRTUAL_RT,
	    CHECKPOINT_INIT,
	    CHECKPOINT_RESET,
	    CHECKPOINT_COUNT
};

typedef enum ReplayCheckpoint ReplayCheckpoint;
typedef struct ReplayNetState ReplayNetState;

extern ReplayMode replay_mode;
extern char *replay_snapshot;

void replay_mutex_lock(void);
void replay_mutex_unlock(void);

// Replay process control functions

void replay_configure(struct QemuOpts *opts);
void replay_start(void);
void replay_finish(void);
void replay_add_blocker(Error *reason);

// Processing the instructions

uint64_t replay_get_current_step(void);
int replay_get_instructions(void);
void replay_account_executed_instructions(void);

// Interrupts and exceptions

bool replay_exception(void);
bool replay_has_exception(void);
bool replay_interrupt(void);
bool replay_has_interrupt(void);

// Processing clocks and other time sources

int64_t replay_save_clock(ReplayClockKind kind, int64_t clock);
int64_t replay_read_clock(ReplayClockKind kind);
#define REPLAY_CLOCK(clock, value)                                      \
    (replay_mode == REPLAY_MODE_PLAY ? replay_read_clock((clock))       \
        : replay_mode == REPLAY_MODE_RECORD                             \
            ? replay_save_clock((clock), (value))                       \
        : (value))

// Event

void replay_shutdown_request(ShutdownCause cause);
bool replay_checkpoint(ReplayCheckpoint checkpoint);

// Asynchronous events queue

void replay_disable_events(void);
void replay_enable_events(void);
bool replay_events_enabled(void);
void replay_bh_schedule_event(QEMUBH *bh);
void replay_input_event(QemuConsole *src, InputEvent *evt);
void replay_input_sync_event(void);
void replay_block_event(QEMUBH *bh, uint64_t id);
uint64_t blkreplay_next_id(void);

// Charter device

void replay_register_char_driver(struct Chardev *chr);
void replay_chr_be_write(struct Chardev *s, uint8_t *buf, int len);
void replay_char_write_event_save(int res, int offset);
void replay_char_write_event_load(int *res, int *offset);
int replay_char_read_all_load(uint8_t *buf);
void replay_char_read_all_save_error(int res);
void replay_char_read_all_save_buf(uint8_t *buf, int offset);

// Network

ReplayNetState *replay_register_net(NetFilterState *nfs);
void replay_unregister_net(ReplayNetState *rns);
void replay_net_packet_event(ReplayNetState *rns, unsigned flags, const struct iovec *iov, int iovcnt);

// Auto

void replay_audio_out(int *played);
void replay_audio_in(int *recorded, void *samples, int *wpos, int size);

// VM state operations

void replay_vmsstate_init(void);
bool replay_can_snapshot(void);

#endif /* SYSEMU_REPLAY_H_ */
