/* src/vm/jit/asmpart.h - prototypes for machine specfic functions

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

   This file is part of CACAO.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Reinhard Grafl
            Andreas Krall

   Changes: Christian Thalinger

   $Id: asmpart.h 3911 2005-12-08 14:33:46Z twisti $

*/


#ifndef _ASMPART_H
#define _ASMPART_H

#include "config.h"
#include "vm/types.h"


#if defined(USE_THREADS)
# if defined(NATIVE_THREADS)
#  include "threads/native/threads.h"
# else
#  include "threads/green/threads.h"
# endif
#endif

#include "vm/global.h"
#include "vm/linker.h"
#include "vm/resolve.h"
#include "vm/jit/stacktrace.h"


typedef struct castinfo castinfo;

struct castinfo {
	s4 super_baseval;
	s4 super_diffval;
	s4 sub_baseval;
};


#if defined(__ALPHA__)
/* 
   determines if the byte support instruction set (21164a and higher)
   is available.
*/
int has_no_x_instr_set(void);
void asm_sync_instruction_cache(void);
#endif


/* 
   invokes the compiler for untranslated JavaVM methods.
   Register R0 contains a pointer to the method info structure
   (prepared by createcompilerstub).
*/
void asm_call_jit_compiler(void);


/* 
   This function calls a Java-method (which possibly needs compilation)
   with up to 4 parameters. This function calls a Java-method (which
   possibly needs compilation) with up to 4 parameters.
*/
java_objectheader *asm_calljavafunction(methodinfo *m, void *arg1, void *arg2,
                                        void *arg3, void *arg4);

s4 asm_calljavafunction_int(methodinfo *m, void *arg1, void *arg2,
							void *arg3, void *arg4);


/* 
   This function calls a Java-method (which possibly needs compilation)
   with up to 4 parameters. This function calls a Java-method (which
   possibly needs compilation) with up to 4 parameters. 
   also supports a return value
*/
java_objectheader *asm_calljavafunction2(methodinfo *m, u4 count, u4 size,
										 jni_callblock *callblock);

s4 asm_calljavafunction2int(methodinfo *m, u4 count, u4 size,
							jni_callblock *callblock);

s8 asm_calljavafunction2long(methodinfo *m, u4 count, u4 size,
							 jni_callblock *callblock);

float asm_calljavafunction2float(methodinfo *m, u4 count, u4 size,
								 jni_callblock *callblock);

double asm_calljavafunction2double(methodinfo *m, u4 count, u4 size,
								   jni_callblock *callblock);


/* We need these two labels in codegen.inc to add the asm_calljavafunction*'s
   into the methodtable */
#if defined(__I386__) || defined(__X86_64__)
void calljava_xhandler(void);
void calljava_xhandler2(void);
#endif

/* exception handling functions */
void asm_handle_exception(void);
void asm_handle_nat_exception(void);

/* wrapper for code patching functions */
void asm_wrapper_patcher(void);

void *asm_switchstackandcall(void *stack, void *func, void **stacktopsave, void * p);

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
extern threadcritnode asm_criticalsections;
#endif

void asm_getclassvalues_atomic(vftbl_t *super, vftbl_t *sub, castinfo *out);

#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
void asm_perform_threadswitch(u1 **from, u1 **to, u1 **stackTop);
u1*  asm_initialize_thread_stack(void *func, u1 *stack);
#endif

/* may be required on some architectures (at least for PowerPC) */
void asm_cacheflush(void *p, s4 size);

#endif /* _ASMPART_H */


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */
