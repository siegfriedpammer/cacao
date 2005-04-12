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

   $Id: asmpart.h 2283 2005-04-12 19:53:05Z twisti $

*/


#ifndef _ASMPART_H
#define _ASMPART_H

#if defined(USE_THREADS)
# if defined(NATIVE_THREADS)
#  include "threads/native/threads.h"
# else
#  include "threads/green/threads.h"
# endif
#endif

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
int has_no_x_instr_set();
#endif


/* 
   invokes the compiler for untranslated JavaVM methods.
   Register R0 contains a pointer to the method info structure
   (prepared by createcompilerstub).
*/
void asm_call_jit_compiler();


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
java_objectheader *asm_calljavafunction2(methodinfo *m, u4 count, u4 size, void *callblock);
s4 asm_calljavafunction2int(methodinfo *m, u4 count, u4 size, void *callblock);
s8 asm_calljavafunction2long(methodinfo *m, u4 count, u4 size, void *callblock);
float asm_calljavafunction2float(methodinfo *m, u4 count, u4 size, void *callblock);
double asm_calljavafunction2double(methodinfo *m, u4 count, u4 size, void *callblock);

/* We need these two labels in codegen.inc to add the asm_calljavafunction*'s
   into the methodtable */
#if defined(__I386__) || defined(__X86_64__)
void calljava_xhandler();
void calljava_xhandler2();
#endif

void asm_handle_exception();
void asm_handle_nat_exception();
void asm_handle_nullptr_exception();

void asm_handle_builtin_exception(classinfo *);
void asm_throw_and_handle_exception();
#ifdef __ALPHA__
void asm_throw_and_handle_nat_exception();
void asm_refillin_and_handle_exception();
void asm_throw_and_handle_arrayindexoutofbounds_exception();
#endif
void asm_throw_and_handle_hardware_arithmetic_exception();

/* code patching functions */
void asm_patcher_get_putstatic(void);
void asm_patcher_get_putfield(void);

void asm_patcher_builtin_new(unresolved_class *uc);
#define asm_patcher_BUILTIN_new (functionptr) asm_patcher_builtin_new

void asm_patcher_builtin_newarray(unresolved_class *uc);
#define asm_patcher_BUILTIN_newarray (functionptr) asm_patcher_builtin_newarray

void asm_patcher_builtin_multianewarray(unresolved_class *uc);
#define asm_patcher_BUILTIN_multianewarray (functionptr) asm_patcher_builtin_multianewarray

void asm_patcher_builtin_checkarraycast(unresolved_class *uc);
#define asm_patcher_BUILTIN_checkarraycast (functionptr) asm_patcher_builtin_checkarraycast

void asm_patcher_builtin_arrayinstanceof(unresolved_class *uc);
#define asm_patcher_BUILTIN_arrayinstanceof (functionptr) asm_patcher_builtin_arrayinstanceof

void asm_patcher_invokestatic_special(void);
void asm_patcher_invokevirtual(void);
void asm_patcher_invokeinterface(void);
void asm_patcher_checkcast_instanceof_flags(void);
void asm_patcher_checkcast_instanceof_interface(void);
void asm_patcher_checkcast_class(void);
void asm_patcher_instanceof_class(void);

void asm_check_clinit(void);


stacktraceelement *asm_get_stackTrace();

void *asm_switchstackandcall(void *stack, void *func, void **stacktopsave, void * p);

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
extern threadcritnode asm_criticalsections;
#endif

void asm_getclassvalues_atomic(vftbl_t *super, vftbl_t *sub, castinfo *out);

void asm_prepare_native_stackinfo(void*ret,void*stackbegin);
void asm_remove_native_stackinfo();

#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
void asm_perform_threadswitch(u1 **from, u1 **to, u1 **stackTop);
u1*  asm_initialize_thread_stack(void *func, u1 *stack);
#endif

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
