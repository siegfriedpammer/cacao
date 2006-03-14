#ifndef _CODE_H
#define _CODE_H

#include "config.h"
#include "vm/types.h"

#include "vm/method.h"
#include "vm/jit/replace.h"

/* A `codeinfo` represents a particular realization of a method in     */
/* machine code.                                                       */

struct codeinfo {
	methodinfo   *m;                /* method this is a realization of */
	codeinfo     *prev;             /* previous codeinfo of this method*/
	
	/* machine code */
	s4            mcodelength;      /* length of generated machine code*/
	u1           *mcode;            /* pointer to machine code         */
	u1           *entrypoint;       /* machine code entry point        */
	bool          isleafmethod;     /* does method call subroutines    */

	/* replacement */
	rplpoint     *rplpoints;        /* replacement points              */
	int           rplpointcount;    /* number of replacement points    */

	/* register allocation */
	u1           *regalloc;         /* register index for each local   */
	                                /* variable                        */

	/* profiling */
	u4            frequency;        /* number of method invocations    */
	u4           *bbfrequency;
	s8            cycles;           /* number of cpu cycles            */
};

codeinfo *code_codeinfo_new(methodinfo *m);
void code_codeinfo_free(codeinfo *code);

void code_free_code_of_method(methodinfo *m);

#endif

/* vim: noet ts=4 sw=4
 */

