/* src/vm/jit/asmpart.h - prototypes for machine specfic functions

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Contact: cacao@cacaojvm.org

   Authors: Reinhard Grafl
            Andreas Krall

   Changes: Christian Thalinger
            Edwin Steiner

   $Id: asmpart.h 4920 2006-05-15 13:13:22Z twisti $

*/


#ifndef _ASMPART_H
#define _ASMPART_H

#include "config.h"
#include "vm/types.h"


#if defined(USE_THREADS)
# if defined(NATIVE_THREADS)
#  include "threads/native/threads.h"
#  include "threads/native/critical.h"
# else
#  include "threads/green/threads.h"
# endif
#endif

#include "vm/global.h"
#include "vm/linker.h"
#include "vm/resolve.h"
#include "vm/vm.h"
#include "vm/jit/replace.h"
#include "vm/jit/stacktrace.h"


/* some macros ****************************************************************/

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)

#  define ASM_GETCLASSVALUES_ATOMIC(super,sub,out) \
    do { \
        if (opt_intrp) \
            intrp_asm_getclassvalues_atomic((super), (sub), (out)); \
        else \
            asm_getclassvalues_atomic((super), (sub), (out)); \
    } while (0)

# else /* defined(ENABLE_INTRP) */

#  define ASM_GETCLASSVALUES_ATOMIC(super,sub,out) \
    asm_getclassvalues_atomic((super), (sub), (out))

# endif /* defined(ENABLE_INTRP) */

#else /* defined(ENABLE_JIT) */

#  define ASM_GETCLASSVALUES_ATOMIC(super,sub,out) \
    intrp_asm_getclassvalues_atomic((super), (sub), (out))

#endif /* defined(ENABLE_JIT) */


typedef struct castinfo castinfo;

struct castinfo {
	s4 super_baseval;
	s4 super_diffval;
	s4 sub_baseval;
};


/* function prototypes ********************************************************/

/* machine dependent initialization */
s4   asm_md_init(void);

/* 
   invokes the compiler for untranslated JavaVM methods.
   Register R0 contains a pointer to the method info structure
   (prepared by createcompilerstub).
*/
void asm_call_jit_compiler(void);

#if defined(ENABLE_JIT)
java_objectheader *asm_vm_call_method(methodinfo *m, s4 vmargscount,
									  vm_arg *vmargs);

s4     asm_vm_call_method_int(methodinfo *m, s4 vmargscount, vm_arg *vmargs);
s8     asm_vm_call_method_long(methodinfo *m, s4 vmargscount, vm_arg *vmargs);
float  asm_vm_call_method_float(methodinfo *m, s4 vmargscount, vm_arg *vmargs);
double asm_vm_call_method_double(methodinfo *m, s4 vmargscount, vm_arg *vmargs);

void   asm_vm_call_method_exception_handler(void);
#endif

#if defined(ENABLE_INTRP)
java_objectheader *intrp_asm_vm_call_method(methodinfo *m, s4 vmargscount,
											vm_arg *vmargs);

s4     intrp_asm_vm_call_method_int(methodinfo *m, s4 vmargscount,
									vm_arg *vmargs);
s8     intrp_asm_vm_call_method_long(methodinfo *m, s4 vmargscount,
									 vm_arg *vmargs);
float  intrp_asm_vm_call_method_float(methodinfo *m, s4 vmargscount,
									  vm_arg *vmargs);
double intrp_asm_vm_call_method_double(methodinfo *m, s4 vmargscount,
									   vm_arg *vmargs);
#endif

/* exception handling functions */

#if defined(ENABLE_JIT)
void asm_handle_exception(void);
void asm_handle_nat_exception(void);
#endif

/* wrapper for code patching functions */
void asm_wrapper_patcher(void);

/* functions for on-stack replacement */
void asm_replacement_out(void);
void asm_replacement_in(executionstate *es);

void *asm_switchstackandcall(void *stack, void *func, void **stacktopsave, void * p);

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
extern critical_section_node_t asm_criticalsections;
#endif


#if defined(ENABLE_JIT)
void asm_getclassvalues_atomic(vftbl_t *super, vftbl_t *sub, castinfo *out);
#endif

#if defined(ENABLE_INTRP)
void intrp_asm_getclassvalues_atomic(vftbl_t *super, vftbl_t *sub, castinfo *out);
#endif


#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
void asm_perform_threadswitch(u1 **from, u1 **to, u1 **stackTop);
u1*  asm_initialize_thread_stack(void *func, u1 *stack);
#endif

/* cache flush function */
void asm_cacheflush(u1 *addr, s4 nbytes);

u8 asm_get_cycle_count(void);

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
 * vim:noexpandtab:sw=4:ts=4:
 */
