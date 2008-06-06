#ifndef _VM_JIT_OPTIMIZING_ESCAPE_H
#define _VM_JIT_OPTIMIZING_ESCAPE_H

#include "vm/jit/jit.h"
#include "vmcore/method.h"

typedef enum {
	ESCAPE_UNKNOWN,
	ESCAPE_NONE,
	ESCAPE_METHOD,
	ESCAPE_GLOBAL_THROUGH_METHOD,
	ESCAPE_GLOBAL
} escape_state_t;

void escape_analysis_perform(jitdata *jd);

void escape_analysis_escape_check(void *vp);

void bc_escape_analysis_perform(methodinfo *m);

#endif
