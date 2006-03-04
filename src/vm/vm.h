/* src/vm/vm.h - basic JVM functions

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

   Authors: Christian Thalinger

   Changes:

   $Id: finalizer.c 4357 2006-01-22 23:33:38Z twisti $

*/


#ifndef _VM_H
#define _VM_H

#include "config.h"
#include "vm/types.h"

#include "native/jni.h"
#include "vm/global.h"


/* export global variables ****************************************************/

extern _Jv_JNIEnv *_Jv_env;

extern bool vm_initializing;
extern bool vm_exiting;

extern u1 *intrp_main_stack;

extern void **stackbottom;

extern char *mainstring;
extern classinfo *mainclass;


/* vm_arg **********************************************************************

   Datastructure for arguments to call Java methods via vm_call_method
   functions.

*******************************************************************************/

typedef struct vm_arg vm_arg;

struct vm_arg {
	u8 type;
	u8 data;
};


/* function prototypes ********************************************************/

void usage(void);

bool vm_create(JavaVMInitArgs *vm_args);
s4   vm_destroy(JavaVM *vm);
void vm_exit(s4 status);
void vm_shutdown(s4 status);

void vm_exit_handler(void);

/* Java method calling functions */
java_objectheader *vm_call_method_intern(methodinfo *m, void *a0, void *a1,
										 void *a2, void *a3);

s4 vm_call_method_intern_int(methodinfo *m, void *a0, void *a1, void *a2,
							 void *a3);

java_objectheader *vm_call_method(methodinfo *m, u4 count, vm_arg *vmargs);
s4     vm_call_method_int(methodinfo *m, u4 count, vm_arg *vmargs);
s8     vm_call_method_long(methodinfo *m, u4 count, vm_arg *vmargs);
float  vm_call_method_float(methodinfo *m, u4 count, vm_arg *vmargs);
double vm_call_method_double(methodinfo *m, u4 count, vm_arg *vmargs);

#endif /* _VM_H */

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
