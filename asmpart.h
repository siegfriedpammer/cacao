/****************************** asmpart.h **************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Headerfile for asmpart.S. asmpart.S contains the machine dependent
	Java - C interface functions.

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at
	         Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1997/10/15

*******************************************************************************/

#include "global.h"

/* determines if the byte support instruction set (21164a and higher)
   is available.
*/
int has_no_x_instr_set();

void synchronize_caches();


/* invokes the compiler for untranslated JavaVM methods.
   Register R0 contains a pointer to the method info structure
   (prepared by createcompilerstub).
*/
void asm_call_jit_compiler();


/* This function calls a Java-method (which possibly needs compilation)
   with up to 4 parameters. This function calls a Java-method (which
   possibly needs compilation) with up to 4 parameters.
*/
java_objectheader *asm_calljavamethod(methodinfo *m, void *arg1, void *arg2,
                                      void *arg3, void *arg4);

/* This function calls a Java-method (which possibly needs compilation)
   with up to 4 parameters. This function calls a Java-method (which
   possibly needs compilation) with up to 4 parameters. 
   also supports a return value
*/
java_objectheader *asm_calljavafunction(methodinfo *m, void *arg1, void *arg2,
                                        void *arg3, void *arg4);

/* gets the class of the caller from the stack frame
*/
methodinfo *asm_getcallingmethod();


/* This funtion saves all callee saved registers and calls the function
   which is passed as parameter.
   This function is needed by the garbage collector, which needs to access
   all registers which are stored on the stack. Unused registers are
   cleared to avoid interferances with the GC.
*/
void asm_dumpregistersandcall(functionptr f);


void *asm_switchstackandcall(void *stack, void *func, void **stacktopsave, void * p);

void asm_builtin_trace();
void asm_builtin_exittrace();
