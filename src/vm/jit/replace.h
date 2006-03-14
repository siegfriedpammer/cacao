/* An `rplpoint` represents a replacement point in a compiled method  */

#ifndef _REPLACE_H
#define _REPLACE_H

#include "config.h"
#include "vm/types.h"
#include "vm/method.h"

struct rplpoint {
	void     *pc;           /* machine code PC of this point  */
	rplpoint *hashlink;     /* chain to next rplpoint in hash */
	codeinfo *code;         /* codeinfo this point belongs to */
	rplpoint *target;       /* target of the replacement      */

	u1        regalloc[1];  /* VARIABLE LENGTH!               */
};

/* An `executionsstate` represents the state of a thread as it reached */
/* an replacement point or is about to enter one.                      */

#define MD_EXCSTATE_NREGS  32
#define MD_EXCSTATE_NCALLEESAVED  8

struct executionstate {
	u1           *pc;
	u8            regs[MD_EXCSTATE_NREGS];
	u8            savedregs[MD_EXCSTATE_NCALLEESAVED]; /* or read from frame */

	u1           *frame;
	
    java_objectheader *locked; /* XXX maybe just leave it in frame? */
};

/* `sourcestate` will probably only be used for debugging              */

struct sourcestate {
	u8           *javastack;
	s4            javastackdepth;

	u8            javalocals;
	s4            javalocalscount;
};

#endif

/* vim: noet ts=4 sw=4
 */
