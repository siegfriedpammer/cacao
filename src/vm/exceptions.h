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

   Changes:

   $Id: exceptions.h 2458 2005-05-12 23:02:07Z twisti $

*/


#ifndef _EXCEPTIONS_H
#define _EXCEPTIONS_H

#include "vm/global.h"
#include "native/include/java_lang_String.h"
#include "native/include/java_lang_Throwable.h"
#include "vm/builtin.h"
#include "vm/references.h"
#include "vm/method.h"


#if defined(USE_THREADS) && defined(NATIVE_THREADS)

#define exceptionptr                     builtin_get_exceptionptrptr()
#define dontfillinexceptionstacktrace    builtin_get_dontfillinexceptionstacktrace()
#define threadrootmethod                 builtin_get_threadrootmethod()

#else /* defined(USE_THREADS) && defined(NATIVE_THREADS) */

#define exceptionptr        (&_exceptionptr)
#define dontfillinexceptionstacktrace (&_dontfillinexceptionstacktrace)
#define threadrootmethod    (&_threadrootmethod)

#endif /* defined(USE_THREADS) && defined(NATIVE_THREADS) */

#if !defined(USE_THREADS) || !defined(NATIVE_THREADS)
extern java_objectheader *_exceptionptr;
extern u1 _dontfillinexceptionstacktrace;
extern methodinfo* _threadrootmethod;
#endif /* !defined(USE_THREADS) || !defined(NATIVE_THREADS) */


/* global variables ***********************************************************/


/* function prototypes ********************************************************/

/* load and link exceptions used in the system */

bool exceptions_init(void);


/* exception throwing functions */

void throw_exception();
void throw_exception_exit();

void throw_main_exception();
void throw_main_exception_exit();

void throw_cacao_exception_exit(const char *exception, const char *message, ...);


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
java_objectheader *new_internalerror(const char *message, ...);
java_objectheader *new_verifyerror(methodinfo *m, const char *message);
java_objectheader *new_unsupportedclassversionerror(classinfo *c,
													const char *message, ...);

java_objectheader *new_arithmeticexception();
java_objectheader *new_arrayindexoutofboundsexception(s4 index);
java_objectheader *new_arraystoreexception();
java_objectheader *new_classcastexception();
java_objectheader *new_negativearraysizeexception();
java_objectheader *new_nullpointerexception();

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
