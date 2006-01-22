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

   $Id: asmpart.h 4344 2006-01-22 20:00:59Z twisti $

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


/* some macros ****************************************************************/

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)

#  define ASM_CALLJAVAFUNCTION(m,a0,a1,a2,a3) \
    do { \
        if (opt_intrp) \
            (void) intrp_asm_calljavafunction((m), (a0), (a1), (a2), (a3)); \
        else \
            (void) asm_calljavafunction((m), (a0), (a1), (a2), (a3)); \
    } while (0)

#  define ASM_CALLJAVAFUNCTION_ADR(o,m,a0,a1,a2,a3) \
    do { \
        if (opt_intrp) \
            (o) = intrp_asm_calljavafunction((m), (a0), (a1), (a2), (a3)); \
        else \
            (o) = asm_calljavafunction((m), (a0), (a1), (a2), (a3)); \
    } while (0)

#  define ASM_CALLJAVAFUNCTION_INT(i,m,a0,a1,a2,a3) \
    do { \
        if (opt_intrp) \
            (i) = intrp_asm_calljavafunction_int((m), (a0), (a1), (a2), (a3)); \
        else \
            (i) = asm_calljavafunction_int((m), (a0), (a1), (a2), (a3)); \
    } while (0)


#  define ASM_CALLJAVAFUNCTION2(m,count,x,callblock) \
    do { \
        if (opt_intrp) \
            (void) intrp_asm_calljavafunction2((m), (count), (x), (callblock)); \
        else \
            (void) asm_calljavafunction2((m), (count), (x), (callblock)); \
    } while (0)

#  define ASM_CALLJAVAFUNCTION2_ADR(o,m,count,x,callblock) \
    do { \
        if (opt_intrp) \
            (o) = intrp_asm_calljavafunction2((m), (count), (x), (callblock)); \
        else \
            (o) = asm_calljavafunction2((m), (count), (x), (callblock)); \
    } while (0)

#  define ASM_CALLJAVAFUNCTION2_INT(i,m,count,x,callblock) \
    do { \
        if (opt_intrp) \
            (i) = intrp_asm_calljavafunction2int((m), (count), (x), (callblock)); \
        else \
            (i) = asm_calljavafunction2int((m), (count), (x), (callblock)); \
    } while (0)

#  define ASM_CALLJAVAFUNCTION2_LONG(l,m,count,x,callblock) \
    do { \
        if (opt_intrp) \
            (l) = intrp_asm_calljavafunction2long((m), (count), (x), (callblock)); \
        else \
            (l) = asm_calljavafunction2long((m), (count), (x), (callblock)); \
    } while (0)

#  define ASM_CALLJAVAFUNCTION2_FLOAT(f,m,count,x,callblock) \
    do { \
        if (opt_intrp) \
            (f) = intrp_asm_calljavafunction2float((m), (count), (x), (callblock)); \
        else \
            (f) = asm_calljavafunction2float((m), (count), (x), (callblock)); \
    } while (0)

#  define ASM_CALLJAVAFUNCTION2_DOUBLE(d,m,count,x,callblock) \
    do { \
        if (opt_intrp) \
            (d) = intrp_asm_calljavafunction2double((m), (count), (x), (callblock)); \
        else \
            (d) = asm_calljavafunction2double((m), (count), (x), (callblock)); \
    } while (0)


#  define ASM_GETCLASSVALUES_ATOMIC(super,sub,out) \
    do { \
        if (opt_intrp) \
            intrp_asm_getclassvalues_atomic((super), (sub), (out)); \
        else \
            asm_getclassvalues_atomic((super), (sub), (out)); \
    } while (0)

# else /* defined(ENABLE_INTRP) */

#  define ASM_CALLJAVAFUNCTION(m,a0,a1,a2,a3) \
    (void) asm_calljavafunction((m), (a0), (a1), (a2), (a3))

#  define ASM_CALLJAVAFUNCTION_ADR(o,m,a0,a1,a2,a3) \
    (o) = asm_calljavafunction((m), (a0), (a1), (a2), (a3))

#  define ASM_CALLJAVAFUNCTION_INT(i,m,a0,a1,a2,a3) \
    (i) = asm_calljavafunction_int((m), (a0), (a1), (a2), (a3))


#  define ASM_CALLJAVAFUNCTION2(m,count,x,callblock) \
    (void) asm_calljavafunction2((m), (count), (x), (callblock))

#  define ASM_CALLJAVAFUNCTION2_ADR(o,m,count,x,callblock) \
    (o) = asm_calljavafunction2((m), (count), (x), (callblock))

#  define ASM_CALLJAVAFUNCTION2_INT(i,m,count,x,callblock) \
    (i) = asm_calljavafunction2int((m), (count), (x), (callblock))

#  define ASM_CALLJAVAFUNCTION2_LONG(l,m,count,x,callblock) \
    (l) = asm_calljavafunction2long((m), (count), (x), (callblock))

#  define ASM_CALLJAVAFUNCTION2_FLOAT(f,m,count,x,callblock) \
    (f) = asm_calljavafunction2float((m), (count), (x), (callblock))

#  define ASM_CALLJAVAFUNCTION2_DOUBLE(d,m,count,x,callblock) \
    (d) = asm_calljavafunction2double((m), (count), (x), (callblock))


#  define ASM_GETCLASSVALUES_ATOMIC(super,sub,out) \
    asm_getclassvalues_atomic((super), (sub), (out))

# endif /* defined(ENABLE_INTRP) */

#else /* defined(ENABLE_JIT) */

# define ASM_CALLJAVAFUNCTION(m,a0,a1,a2,a3) \
    (void) intrp_asm_calljavafunction((m), (a0), (a1), (a2), (a3))

# define ASM_CALLJAVAFUNCTION_ADR(o,m,a0,a1,a2,a3) \
    (o) = intrp_asm_calljavafunction((m), (a0), (a1), (a2), (a3))

# define ASM_CALLJAVAFUNCTION_INT(i,m,a0,a1,a2,a3) \
    (i) = intrp_asm_calljavafunction_int((m), (a0), (a1), (a2), (a3))


# define ASM_CALLJAVAFUNCTION2(m,count,x,callblock) \
    (void) intrp_asm_calljavafunction2((m), (count), (x), (callblock))

# define ASM_CALLJAVAFUNCTION2_ADR(o,m,count,x,callblock) \
    (o) = intrp_asm_calljavafunction2((m), (count), (x), (callblock))

# define ASM_CALLJAVAFUNCTION2_INT(i,m,count,x,callblock) \
    (i) = intrp_asm_calljavafunction2int((m), (count), (x), (callblock))

# define ASM_CALLJAVAFUNCTION2_LONG(l,m,count,x,callblock) \
    (l) = intrp_asm_calljavafunction2long((m), (count), (x), (callblock))

# define ASM_CALLJAVAFUNCTION2_FLOAT(f,m,count,x,callblock) \
    (f) = intrp_asm_calljavafunction2float((m), (count), (x), (callblock))

# define ASM_CALLJAVAFUNCTION2_DOUBLE(d,m,count,x,callblock) \
    (d) = intrp_asm_calljavafunction2double((m), (count), (x), (callblock))


#  define ASM_GETCLASSVALUES_ATOMIC(super,sub,out) \
    intrp_asm_getclassvalues_atomic((super), (sub), (out))

#endif /* defined(ENABLE_JIT) */


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

#if defined(ENABLE_JIT)
java_objectheader *asm_calljavafunction(methodinfo *m,
										void *arg1, void *arg2,
										void *arg3, void *arg4);

s4 asm_calljavafunction_int(methodinfo *m,
							void *arg1, void *arg2,
							void *arg3, void *arg4);
#endif

#if defined(ENABLE_INTRP)
java_objectheader *intrp_asm_calljavafunction(methodinfo *m,
											  void *arg1, void *arg2,
											  void *arg3, void *arg4);

s4 intrp_asm_calljavafunction_int(methodinfo *m,
								  void *arg1, void *arg2,
								  void *arg3, void *arg4);
#endif


/* 
   This function calls a Java-method (which possibly needs compilation)
   with up to 4 parameters. This function calls a Java-method (which
   possibly needs compilation) with up to 4 parameters. 
   also supports a return value
*/

#if defined(ENABLE_JIT)
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
#endif

#if defined(ENABLE_INTRP)
java_objectheader *intrp_asm_calljavafunction2(methodinfo *m, u4 count, u4 size,
											   jni_callblock *callblock);

s4 intrp_asm_calljavafunction2int(methodinfo *m, u4 count, u4 size,
								  jni_callblock *callblock);

s8 intrp_asm_calljavafunction2long(methodinfo *m, u4 count, u4 size,
								   jni_callblock *callblock);

float intrp_asm_calljavafunction2float(methodinfo *m, u4 count, u4 size,
									   jni_callblock *callblock);

double intrp_asm_calljavafunction2double(methodinfo *m, u4 count, u4 size,
										 jni_callblock *callblock);
#endif


/* We need these two labels in codegen.inc to add the asm_calljavafunction*'s
   into the methodtable */
#if defined(__I386__) || defined(__X86_64__)
void calljava_xhandler(void);
void calljava_xhandler2(void);
#endif

/* exception handling functions */

#if defined(ENABLE_JIT)
void asm_handle_exception(void);
void asm_handle_nat_exception(void);
#endif

/* wrapper for code patching functions */
void asm_wrapper_patcher(void);

void *asm_switchstackandcall(void *stack, void *func, void **stacktopsave, void * p);

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
extern threadcritnode asm_criticalsections;
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

/* may be required on some architectures (at least for PowerPC and ARM) */
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
