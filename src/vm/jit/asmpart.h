/* asmpart.h - prototypes for machine specfic functions

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser,
   M. Probst, S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck,
   P. Tomsich, J. Wenninger

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

   $Id: asmpart.h 963 2004-03-15 07:37:49Z jowenn $

*/


#ifndef _ASMPART_H
#define _ASMPART_H

#include "global.h"
#include "jni.h"

/* 
   determines if the byte support instruction set (21164a and higher)
   is available.
*/
int has_no_x_instr_set();

void synchronize_caches();


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

/* 
   This function calls a Java-method (which possibly needs compilation)
   with up to 4 parameters. This function calls a Java-method (which
   possibly needs compilation) with up to 4 parameters. 
   also supports a return value
*/
java_objectheader *asm_calljavafunction(methodinfo *m, void *arg1, void *arg2,
                                        void *arg3, void *arg4);
java_objectheader *asm_calljavafunction2(methodinfo *m, u4 count, u4 size , void *callblock);
jdouble asm_calljavafunction2double(methodinfo *m, u4 count, u4 size , void *callblock);
jlong asm_calljavafunction2long(methodinfo *m, u4 count, u4 size , void *callblock);



void asm_handle_exception();
void asm_handle_nat_exception();

void asm_check_clinit();

void asm_handle_builtin_exception(classinfo *);

/* 
   gets the class of the caller from the stack frame
*/
methodinfo *asm_getcallingmethod();

java_objectarray* Java_java_lang_VMSecurityManager_getClassContext(JNIEnv *env, jclass clazz);
stacktraceelement *asm_get_stackTrace();

/*java_lang_ClassLoader* Java_java_lang_VMSecurityManager_currentClassLoader(JNIEnv *env, jclass clazz);*/
/* 
   This funtion saves all callee saved registers and calls the function
   which is passed as parameter.
   This function is needed by the garbage collector, which needs to access
   all registers which are stored on the stack. Unused registers are
   cleared to avoid interferances with the GC.
*/
void asm_dumpregistersandcall(functionptr f);


void *asm_switchstackandcall(void *stack, void *func, void **stacktopsave, void * p);

void asm_builtin_trace();
void asm_builtin_exittrace();

int asm_xadd(int *, int);

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
