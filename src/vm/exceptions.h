/* src/vm/exceptions.h - exception related functions prototypes

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
            Edwin Steiner

   $Id: exceptions.h 6244 2006-12-27 15:15:31Z twisti $

*/


#ifndef _EXCEPTIONS_H
#define _EXCEPTIONS_H

/* forward typedefs ***********************************************************/

#include "config.h"
#include "vm/types.h"

#include "vm/global.h"
#include "native/jni.h"
#include "native/include/java_lang_String.h"
#include "native/include/java_lang_Throwable.h"
#include "vm/builtin.h"
#include "vm/references.h"
#include "vm/method.h"


/* hardware-exception defines **************************************************

   These defines define the load-offset which indicates the given
   exception.

   ATTENTION: These offsets need NOT to be aligned to 4 or 8-byte
   boundaries, since normal loads could have such offsets with a base
   of NULL which should result in a NullPointerException.

*******************************************************************************/

#define EXCEPTION_LOAD_DISP_NULLPOINTER              0
#define EXCEPTION_LOAD_DISP_ARITHMETIC               1
#define EXCEPTION_LOAD_DISP_ARRAYINDEXOUTOFBOUNDS    2
#define EXCEPTION_LOAD_DISP_CLASSCAST                3

#define EXCEPTION_LOAD_DISP_PATCHER                  5


/* exception pointer **********************************************************/

#if defined(ENABLE_THREADS)
#define exceptionptr    &(THREADOBJECT->_exceptionptr)
#else
#define exceptionptr    &_no_threads_exceptionptr
#endif

#if !defined(ENABLE_THREADS)
extern java_objectheader *_no_threads_exceptionptr;
#endif


/* function prototypes ********************************************************/

/* load and link exceptions used in the system */
bool exceptions_init(void);


/* exception throwing functions */

void throw_exception(void);
void throw_exception_exit(void);

void throw_main_exception(void);
void throw_main_exception_exit(void);

void throw_cacao_exception_exit(const char *exception,
								const char *message, ...);

void exceptions_throw_outofmemory_exit(void);


/* initialize new exceptions */

java_objectheader *new_exception(const char *classname);

java_objectheader *new_exception_message(const char *classname,
										 const char *message);

java_objectheader *new_exception_throwable(const char *classname,
										   java_lang_Throwable *cause);

java_objectheader *new_exception_utfmessage(const char *classname,
											utf *message);

java_objectheader *new_exception_javastring(const char *classname,
											java_lang_String *message);

java_objectheader *new_exception_int(const char *classname, s4 i);


/* functions to generate compiler exceptions */

#if defined(ENABLE_JAVASE)
java_objectheader *exceptions_new_abstractmethoderror(void);
java_objectheader *exceptions_asm_new_abstractmethoderror(u1 *sp, u1 *ra);
void exceptions_throw_abstractmethoderror(void);
#endif

java_objectheader *new_classformaterror(classinfo *c, const char *message, ...);
void exceptions_throw_classformaterror(classinfo *c, const char *message, ...);

java_objectheader *new_classnotfoundexception(utf *name);
java_objectheader *new_noclassdeffounderror(utf *name);
java_objectheader *exceptions_new_linkageerror(const char *message,classinfo *c);

#if defined(ENABLE_JAVASE)
java_objectheader *exceptions_new_nosuchmethoderror(classinfo *c,
													utf *name, utf *desc);
void exceptions_throw_nosuchmethoderror(classinfo *c, utf *name, utf *desc);
#endif

java_objectheader *new_internalerror(const char *message, ...);
void exceptions_throw_internalerror(const char *message, ...);

java_objectheader *exceptions_new_verifyerror(methodinfo *m,
											  const char *message, ...);
void exceptions_throw_verifyerror(methodinfo *m, const char *message, ...);
void exceptions_throw_verifyerror_for_stack(methodinfo *m, int type);

java_objectheader *exceptions_new_virtualmachineerror(void);
void               exceptions_throw_virtualmachineerror(void);

java_objectheader *new_unsupportedclassversionerror(classinfo *c,
													const char *message, ...);

java_objectheader *new_arithmeticexception(void);

java_objectheader *new_arrayindexoutofboundsexception(s4 index);
void exceptions_throw_arrayindexoutofboundsexception(void);

java_objectheader *exceptions_new_arraystoreexception(void);
void exceptions_throw_arraystoreexception(void);

java_objectheader *exceptions_new_classcastexception(java_objectheader *o);

java_objectheader *new_illegalargumentexception(void);
void exceptions_throw_illegalargumentexception(void);

java_objectheader *exceptions_new_illegalmonitorstateexception(void);
void               exceptions_throw_illegalmonitorstateexception(void);

java_objectheader *new_negativearraysizeexception(void);
void exceptions_throw_negativearraysizeexception(void);

java_objectheader *exceptions_new_nullpointerexception(void);
void exceptions_throw_nullpointerexception(void);

java_objectheader *exceptions_new_stringindexoutofboundsexception(void);
void exceptions_throw_stringindexoutofboundsexception(void);

void classnotfoundexception_to_noclassdeffounderror(void);

java_objectheader *exceptions_get_and_clear_exception(void);

void exceptions_print_exception(java_objectheader *xptr);

#endif /* _EXCEPTIONS_H */


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
