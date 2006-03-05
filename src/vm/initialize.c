/* src/vm/initialize.c - static class initializer functions

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

   Changes: Mark Probst
            Andreas Krall
            Christian Thalinger

   $Id: initialize.c 4559 2006-03-05 23:24:50Z twisti $

*/


#include "config.h"

#include <string.h>

#include "vm/types.h"

#include "vm/global.h"
#include "vm/initialize.h"
#include "vm/builtin.h"
#include "vm/class.h"
#include "vm/loader.h"
#include "vm/exceptions.h"
#include "vm/options.h"
#include "vm/statistics.h"
#include "vm/stringlocal.h"
#include "vm/vm.h"
#include "vm/jit/asmpart.h"


/* private functions **********************************************************/

static bool initialize_class_intern(classinfo *c);


/* initialize_class ************************************************************

   In Java, every class can have a static initialization
   function. This function has to be called BEFORE calling other
   methods or accessing static variables.

*******************************************************************************/

bool initialize_class(classinfo *c)
{
	bool r;

	if (!makeinitializations)
		return true;

#if defined(USE_THREADS)
	/* enter a monitor on the class */

	builtin_monitorenter((java_objectheader *) c);
#endif

	/* maybe the class is already initalized or the current thread, which can
	   pass the monitor, is currently initalizing this class */

	if (CLASS_IS_OR_ALMOST_INITIALIZED(c)) {
#if defined(USE_THREADS)
		builtin_monitorexit((java_objectheader *) c);
#endif

		return true;
	}

	/* if <clinit> throw an Error before, the class was marked with an
       error and we have to throw a NoClassDefFoundError */

	if (c->state & CLASS_ERROR) {
		*exceptionptr = new_noclassdeffounderror(c->name);

#if defined(USE_THREADS)
		builtin_monitorexit((java_objectheader *) c);
#endif

		/* ...but return true, this is ok (mauve test) */

		return true;
	}

	/* this initalizing run begins NOW */

	c->state |= CLASS_INITIALIZING;

	/* call the internal function */

	r = initialize_class_intern(c);

	/* if return value is not NULL everything was ok and the class is
	   initialized */

	if (r)
		c->state |= CLASS_INITIALIZED;

	/* this initalizing run is done */

	c->state &= ~CLASS_INITIALIZING;

#if defined(USE_THREADS)
	/* leave the monitor */

	builtin_monitorexit((java_objectheader *) c);
#endif

	return r;
}


/* initialize_class_intern *****************************************************

   This function MUST NOT be called directly, because of thread
   <clinit> race conditions.

*******************************************************************************/

static bool initialize_class_intern(classinfo *c)
{
	methodinfo        *m;
	java_objectheader *xptr;
#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
	int b;
#endif

	/* maybe the class is not already linked */

	if (!(c->state & CLASS_LINKED))
		if (!link_class(c))
			return false;

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		count_class_inits++;
#endif

	/* initialize super class */

	if (c->super.cls) {
		if (!(c->super.cls->state & CLASS_INITIALIZED)) {
#if !defined(NDEBUG)
			if (initverbose)
				log_message_class_message_class("Initialize super class ",
												c->super.cls,
												" from ",
												c);
#endif

			if (!initialize_class(c->super.cls))
				return false;
		}
	}

	/* interfaces implemented need not to be initialized (VM Spec 2.17.4) */

	m = class_findmethod(c, utf_clinit, utf_void__void);

	if (!m) {
#if !defined(NDEBUG)
		if (initverbose)
			log_message_class("Class has no static class initializer: ", c);
#endif

		return true;
	}

	/* Sun's and IBM's JVM don't care about the static flag */
/*  	if (!(m->flags & ACC_STATIC)) { */
/*  		log_text("Class initializer is not static!"); */

#if !defined(NDEBUG)
	if (initverbose)
		log_message_class("Starting static class initializer for class: ", c);
#endif

#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
	b = blockInts;
	blockInts = 0;
#endif

	/* now call the initializer */

	(void) vm_call_method(m, NULL);

#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
	assert(blockInts == 0);
	blockInts = b;
#endif

	/* we have an exception or error */

	xptr = *exceptionptr;

	if (xptr) {
		/* class is NOT initialized and is marked with error */

		c->state |= CLASS_ERROR;

		/* is this an exception, than wrap it */

		if (builtin_instanceof(xptr, class_java_lang_Exception)) {
			/* clear exception, because we are calling jit code again */

			*exceptionptr = NULL;

			/* wrap the exception */

			*exceptionptr =
				new_exception_throwable(string_java_lang_ExceptionInInitializerError,
										(java_lang_Throwable *) xptr);
		}

		return false;
	}

#if !defined(NDEBUG)
	if (initverbose)
		log_message_class("Finished static class initializer for class: ", c);
#endif

	return true;
}


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
