/* src/vm/exceptions.h - exception related functions prototypes

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

   Authors: Christian Thalinger

   Changes: Edwin Steiner

   $Id: exceptions.h 3861 2005-12-03 13:31:29Z twisti $

*/


#ifndef _EXCEPTIONS_H
#define _EXCEPTIONS_H

/* forward typedefs ***********************************************************/

typedef struct exceptionentry exceptionentry;

#include "config.h"
#include "vm/types.h"

#include "vm/global.h"
#include "native/include/java_lang_String.h"
#include "native/include/java_lang_Throwable.h"
#include "vm/builtin.h"
#include "vm/references.h"
#include "vm/method.h"


#if defined(USE_THREADS) && defined(NATIVE_THREADS)
#define exceptionptr    builtin_get_exceptionptrptr()
#else
#define exceptionptr    &_no_threads_exceptionptr
#endif

#if !defined(USE_THREADS) || !defined(NATIVE_THREADS)
extern java_objectheader *_no_threads_exceptionptr;
#endif


/* exceptionentry **************************************************************

   Datastructure which represents an exception entry in the exception
   table residing in the data segment.

*******************************************************************************/

struct exceptionentry {
	classref_or_classinfo  catchtype;
	u1                    *handlerpc;
	u1                    *endpc;
	u1                    *startpc;
};


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

java_objectheader *new_classformaterror(classinfo *c, const char *message, ...);
java_objectheader *new_classnotfoundexception(utf *name);
java_objectheader *new_noclassdeffounderror(utf *name);
java_objectheader *exceptions_new_linkageerror(const char *message,classinfo *c);
java_objectheader *exceptions_new_nosuchmethoderror(classinfo *c,
													utf *name, utf *desc);
java_objectheader *new_internalerror(const char *message, ...);
java_objectheader *new_verifyerror(methodinfo *m, const char *message, ...);
java_objectheader *new_unsupportedclassversionerror(classinfo *c,
													const char *message, ...);

java_objectheader *new_arithmeticexception(void);
java_objectheader *new_arrayindexoutofboundsexception(s4 index);
java_objectheader *new_arraystoreexception(void);
java_objectheader *new_classcastexception(void);
java_objectheader *new_illegalargumentexception(void);
java_objectheader *new_illegalmonitorstateexception(void);
java_objectheader *new_negativearraysizeexception(void);
java_objectheader *new_nullpointerexception(void);

void classnotfoundexception_to_noclassdeffounderror(void);

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
 */
