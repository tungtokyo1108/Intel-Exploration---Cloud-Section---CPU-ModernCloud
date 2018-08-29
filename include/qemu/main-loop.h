/*
 * main-loop.h
 *
 *  Created on: Aug 28, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_MAIN_LOOP_H_
#define QEMU_MAIN_LOOP_H_

#include "block/aio.h"

#define SIG_IPI SIGUSR1

int qemu_init_main_loop(Error **errp);
void main_loop_wait(int nonblocking);
AioContext *qemu_get_aio_context(void);
void qemu_notify_event(void); // force processing of pending event
#ifdef _WIN32
typedef int PollingFunc(void *opaque);
int qemu_add_polling_cb(PollingFunc *func, void *opaque);
void qemu_del_polling_cb(PollingFunc *func, void *opaque);
typedef void WaitObjectFunc(void *opaque);
int qemu_add_wait_object(HANDLE handle, WaitObjectFunc *func, void *opaque);
void qemu_del_wait_object(HANDLE handle, WaitObjectFunc *func, void *opaque);
#endif

typedef void IOReadHandler(void *opaque, const uint8_t *buf, int size);
typedef int IOCanReadHandler(void *opaque);
void qemu_set_fd_handler(int fd, IOHandler *fd_read, IOHandler *fd_write, void *opaque);
void event_notifier_set_handler(EventNotifier *e, EventNotifierHandler *handler);
GSource *iohandler_get_g_source(void);
AioContext *iohandler_get_aio_context(void);
int qemu_add_child_watch(pid_t pid);
bool qemu_mutex_iothread_locked(void);
void qemu_mutex_lock_iothread(void);
void qemu_mutex_unlock_iothread(void);
void qemu_fd_register(int fd);
QEMUBH *qemu_bh_new(QEMUBHFunc *cb, void *opaque);
void qemu_bh_schedule_idle(QEMUBH *bh);

#endif /* QEMU_MAIN_LOOP_H_ */
