/*
 * tb-context.h
 *
 *  Created on: Sep 16, 2018
 *      Student (coder): Tung Dang
 */

#ifndef EXEC_TB_CONTEXT_H_
#define EXEC_TB_CONTEXT_H_

#include "qemu/thread.h"
#include "qemu/qht.h"

#define CODE_GEN_HTABLE_BITS 15
#define CODE_GEN_HTABLE_SIZE (1 << CODE_GEN_HTABLE_BITS)

typedef struct TranslationBlock TranslationBlock;
typedef struct TBContext TBContext;

struct TBContext {
	struct qht htable;
	unsigned tb_flush_count;
};

extern TBContext tb_ctx;

#endif /* EXEC_TB_CONTEXT_H_ */
