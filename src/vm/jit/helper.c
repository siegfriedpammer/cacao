/* src/vm/jit/asmhelper.c - code patching helper functions

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

   $Id: helper.c 2360 2005-04-24 13:07:57Z jowenn $

*/


#include "vm/class.h"
#include "vm/exceptions.h"
#include "vm/initialize.h"
#include "vm/method.h"
#include "vm/references.h"
#include "vm/resolve.h"
#include "vm/linker.h"

/* XXX class_resolveclassmethod */
#include "vm/loader.h"


/* helper_resolve_classinfo ****************************************************

   This function returns the loaded and resolved class.

*******************************************************************************/

classinfo *helper_resolve_classinfo(constant_classref *cr)
{
	classinfo *c;

	/* resolve and load the class */

	if (!resolve_classref(NULL, cr, resolveEager, true, &c)) {
		java_objectheader *xptr;
		java_objectheader *cause;

		/* get the cause */

		cause = *exceptionptr;

		/* convert ClassNotFoundException's to NoClassDefFoundError's */

		if (builtin_instanceof(cause, class_java_lang_ClassNotFoundException)) {
			/* clear exception, because we are calling jit code again */

			*exceptionptr = NULL;

			/* create new error */

			xptr =
				new_exception_javastring(string_java_lang_NoClassDefFoundError,
										 ((java_lang_Throwable *) cause)->detailMessage);

			/* we had an exception while creating the error */

			if (*exceptionptr)
				return NULL;

			/* set new exception */

			*exceptionptr = xptr;
		}

		return NULL;
	}

	/* return the classinfo pointer */

	return c;
}


/* helper_resolve_methodinfo ***************************************************

   This function returns the loaded and resolved methodinfo of the
   passed method.

*******************************************************************************/

methodinfo *helper_resolve_methodinfo(unresolved_method *um)
{
	methodinfo *m;

	/* resolve the method */

	if (!resolve_method(um, resolveEager, &m)) {
		java_objectheader *xptr;
		java_objectheader *cause;

		/* get the cause */

		cause = *exceptionptr;

		/* convert ClassNotFoundException's to NoClassDefFoundError's */

		if (builtin_instanceof(cause, class_java_lang_ClassNotFoundException)) {
			/* clear exception, because we are calling jit code again */

			*exceptionptr = NULL;

			/* create new error */

			xptr =
				new_exception_javastring(string_java_lang_NoClassDefFoundError,
										 ((java_lang_Throwable *) cause)->detailMessage);

			/* we had an exception while creating the error */

			if (*exceptionptr)
				return NULL;

			/* set new exception */

			*exceptionptr = xptr;
		}

		return NULL;
	}

	/* return the methodinfo pointer */

	return m;
}


/* helper_resolve_fieldinfo ****************************************************

   This function returns the fieldinfo pointer of the passed field.

*******************************************************************************/

void *helper_resolve_fieldinfo(unresolved_field *uf)
{
	fieldinfo *fi;

	/* resolve the field */

	if (!resolve_field(uf, resolveEager, &fi)) {
		java_objectheader *xptr;
		java_objectheader *cause;

		/* get the cause */

		cause = *exceptionptr;

		/* convert ClassNotFoundException's to NoClassDefFoundError's */

		if (builtin_instanceof(cause, class_java_lang_ClassNotFoundException)) {
			/* clear exception, because we are calling jit code again */

			*exceptionptr = NULL;

			/* create new error */

			xptr =
				new_exception_javastring(string_java_lang_NoClassDefFoundError,
										 ((java_lang_Throwable *) cause)->detailMessage);

			/* we had an exception while creating the error */

			if (*exceptionptr)
				return NULL;

			/* set new exception */

			*exceptionptr = xptr;
		}

		return NULL;
	}

	/* return the fieldinfo pointer */

	return fi;
}


/* helper_fillin_stacktrace  ****************************************************

   This function returns the exception given as parameter with a filled in stacktrace

*******************************************************************************/

java_objectheader *helper_fillin_stacktrace(java_objectheader* exc)
{
	classinfo *c;
	methodinfo *m;
        /*log_text("helper_fillin_stacktrace has beenentered");*/
	/* these are panics, since this are sever problems, which must never happen*/
	if (exc==0) panic("Exception must not be null in helper_fillin_stacktrace");
	if ( ((java_lang_Throwable *) exc)->vmState!=0) return exc;
	if (exc->vftbl==0) panic ("Exception vftbl must not be null in helper_fillin_stacktrace");
	/*get classinfo from object instance*/
	c=exc->vftbl->class;
	if (c==0) panic("Exception class must not be null in helper_fillin_stacktrace");
	/*find the fillInStackTrace method*/
	m=class_resolvemethod(c,utf_fillInStackTrace,utf_void__java_lang_Throwable);
	if (m==0) panic ("Exception does not have a fillInStackTrace method");

	/*log_text("helper_fillin_stacktrace doing it's work now");*/
	asm_calljavafunction(m,exc,0,0,0);

	/*return exception back to asmpart*/
	return exc;
}

java_objectheader *helper_fillin_stacktrace_always(java_objectheader* exc) {
	if (exc==0) panic("Exception must not be null in helper_fillin_stacktrace");
	((java_lang_Throwable *) exc)->vmState=0;
	return helper_fillin_stacktrace(exc);
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
 * vim:noexpandtab:sw=4:ts=4:
 */
