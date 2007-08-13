/* src/vm/vm.h - basic JVM functions

   Copyright (C) 1996-2005, 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
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

   $Id: vm.h 8295 2007-08-11 17:57:24Z michi $

*/


#ifndef _VM_H
#define _VM_H

#include "config.h"

#include <stdarg.h>

#include "vm/types.h"

#include "native/jni.h"

#include "vm/global.h"

#include "vmcore/class.h"
#include "vmcore/method.h"


/* export global variables ****************************************************/

extern _Jv_JavaVM *_Jv_jvm;
extern _Jv_JNIEnv *_Jv_env;

extern bool vm_initializing;
extern bool vm_exiting;

extern char      *cacao_prefix;
extern char      *cacao_libjvm;
extern char      *classpath_libdir;

extern char      *_Jv_bootclasspath;
extern char      *_Jv_classpath;
extern char      *_Jv_java_library_path;

extern char      *mainstring;
extern classinfo *mainclass;

#if defined(ENABLE_INTRP)
extern u1 *intrp_main_stack;
#endif


/* function prototypes ********************************************************/

void usage(void);
void vm_printconfig(void);

bool vm_createjvm(JavaVM **p_vm, void **p_env, void *vm_args);
bool vm_create(JavaVMInitArgs *vm_args);
void vm_run(JavaVM *vm, JavaVMInitArgs *vm_args);
s4   vm_destroy(JavaVM *vm);
void vm_exit(s4 status);
void vm_shutdown(s4 status);

void vm_exit_handler(void);

void vm_abort(const char *text, ...);

/* Java method calling functions */

uint64_t *vm_array_from_objectarray(methodinfo *m, java_handle_t *o,
									java_objectarray *params);

java_handle_t *vm_call_method(methodinfo *m, java_handle_t *o, ...);
java_handle_t *vm_call_method_valist(methodinfo *m, java_handle_t *o,
										 va_list ap);
java_handle_t *vm_call_method_jvalue(methodinfo *m, java_handle_t *o,
										 const jvalue *args);

java_handle_t     *vm_call_array(methodinfo *m, uint64_t *array);
int32_t            vm_call_int_array(methodinfo *m, uint64_t *array);
int64_t            vm_call_long_array(methodinfo *m, uint64_t *array);
float              vm_call_float_array(methodinfo *m, uint64_t *array);
double             vm_call_double_array(methodinfo *m, uint64_t *array);

int32_t vm_call_method_int(methodinfo *m, java_handle_t *o, ...);
int32_t vm_call_method_int_valist(methodinfo *m, java_handle_t *o, va_list ap);
int32_t vm_call_method_int_jvalue(methodinfo *m, java_handle_t *o, const jvalue *args);

int64_t vm_call_method_long(methodinfo *m, java_handle_t *o, ...);
int64_t vm_call_method_long_valist(methodinfo *m, java_handle_t *o, va_list ap);
int64_t vm_call_method_long_jvalue(methodinfo *m, java_handle_t *o, const jvalue *args);

float   vm_call_method_float(methodinfo *m, java_handle_t *o, ...);
float   vm_call_method_float_valist(methodinfo *m, java_handle_t *o, va_list ap);
float   vm_call_method_float_jvalue(methodinfo *m, java_handle_t *o, const jvalue *args);

double  vm_call_method_double(methodinfo *m, java_handle_t *o, ...);
double  vm_call_method_double_valist(methodinfo *m, java_handle_t *o, va_list ap);
double  vm_call_method_double_jvalue(methodinfo *m, java_handle_t *o, const jvalue *args);

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
