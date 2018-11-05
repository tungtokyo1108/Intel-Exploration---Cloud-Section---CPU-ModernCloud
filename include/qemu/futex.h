/*
 * futex.h
 *
 *  Created on: Nov 5, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 *
 *  Compared with Facebook:
 *  https://github.com/facebook/folly/blob/master/folly/detail/Futex.h
 */

#ifndef QEMU_FUTEX_H_
#define QEMU_FUTEX_H_

#include <sys/syscall.h>
#include <linux/futex.h>

#define qemu_futex(...) syscall(__NR_futex, __VA_ARGS__)

static inline void qemu_futex_wake(void *f, int n)
{
	qemu_futex(f, FUTEX_WAKE, n, NULL, NULL, 0);
}

static inline void qemu_futex_wait(void *f, unsigned val)
{
    while (qemu_futex(f, FUTEX_WAIT, (int) val, NULL, NULL, 0)) {
        switch (errno) {
        case EWOULDBLOCK:
            return;
        case EINTR:
            break; /* get out of switch and retry */
        default:
            abort();
        }
    }
}

#endif /* QEMU_FUTEX_H_ */
