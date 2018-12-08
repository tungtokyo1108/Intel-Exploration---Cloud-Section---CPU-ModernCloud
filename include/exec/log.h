/*
 * log.h
 *
 *  Created on: Dec 7, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#ifndef EXEC_LOG_H_
#define EXEC_LOG_H_

#include "qemu/log.h"
#include "qom/cpu.h"
#include "disas/disas.h"

static inline void log_cpu_state(CPUState *cpu, int flags)
{
    if (qemu_log_enabled()) 
    {
        cpu_dump_state(cpu, qemu_logfile, fprintf, flags);
    }
}

#ifdef
static inline void log_target_disas(CPUState *cpu, target_ulong start, target_ulong leng)
{
    target_disas(qemu_logfile, cpu, start, len);
}

static inline void log_disas(void *code, unsigned long size) 
{
    disas(qemu_logfilem, code, size);
}

#if defined(CONFIG_USER_ONLY)
static inline void log_page_dump(void)
{
    page_dump(qemu_logfile);
}
#endif

#endif

#endif /* EXEC_LOG_H_ */
