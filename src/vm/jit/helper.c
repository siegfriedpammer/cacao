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

   $Id: helper.c 2284 2005-04-12 20:31:38Z twisti $

*/


#include "vm/class.h"
#include "vm/exceptions.h"
#include "vm/initialize.h"
#include "vm/method.h"
#include "vm/references.h"
#include "vm/resolve.h"

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


/* helper_resolve_classinfo_flags **********************************************

   This function returns the flags of the passed class.

*******************************************************************************/

s4 helper_resolve_classinfo_flags(constant_classref *cr)
{
	classinfo *c;

	/* resolve and load the class */

	if (!(c = helper_resolve_classinfo(cr)))
		return -1;

	/* return the flags */

	return c->flags;
}


/* helper_resolve_classinfo_vftbl **********************************************

   This function return the vftbl pointer of the passed class.

*******************************************************************************/

vftbl_t *helper_resolve_classinfo_vftbl(constant_classref *cr)
{
	classinfo *c;

	/* resolve and load the class */

	if (!(c = helper_resolve_classinfo(cr)))
		return NULL;

	/* return the virtual function table pointer */

	return c->vftbl;
}


/* helper_resolve_classinfo_index **********************************************

   This function returns the index of the passed class.

*******************************************************************************/

s4 helper_resolve_classinfo_index(constant_classref *cr)
{
	classinfo *c;

	/* resolve and load the class */

	if (!(c = helper_resolve_classinfo(cr)))
		return -1;

	/* return the class' index */

	return c->index;
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


/* helper_resolve_methodinfo_vftblindex ****************************************

   This function returns the virtual function table index (vftblindex)
   of the passed method.

*******************************************************************************/

s4 helper_resolve_methodinfo_vftblindex(unresolved_method *um)
{
	methodinfo *m;

	/* resolve the method */

	if (!(m = helper_resolve_methodinfo(um)))
		return -1;

	/* return the virtual function table index */

	return m->vftblindex;
}


/* helper_resolve_methodinfo_stubroutine ***************************************

   This function returns the stubroutine of the passed method.

*******************************************************************************/

u1 *helper_resolve_methodinfo_stubroutine(unresolved_method *um)
{
	methodinfo *m;

	/* resolve the method */

	if (!(m = helper_resolve_methodinfo(um)))
		return NULL;

	/* return the method pointer */

	return m->stubroutine;
}


/* helper_resolve_fieldinfo ****************************************************

   This function returns the fieldinfo pointer of the passed field.

*******************************************************************************/

static void *helper_resolve_fieldinfo(unresolved_field *uf)
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


/* helper_resolve_fieldinfo_value_address **************************************

   This function returns the value address of the passed field.

*******************************************************************************/

void *helper_resolve_fieldinfo_value_address(unresolved_field *uf)
{
	fieldinfo *fi;

	/* resolve the field */

	if (!(fi = helper_resolve_fieldinfo(uf)))
		return NULL;

	/* check if class is initialized */

	if (!fi->class->initialized)
		if (!initialize_class(fi->class))
			return NULL;

	/* return the field value's address */

	return &(fi->value);
}


/* helper_resolve_fieldinfo_offset *********************************************

   This function returns the offset value of the passed field.

*******************************************************************************/

s4 helper_resolve_fieldinfo_offset(unresolved_field *uf)
{
	fieldinfo *fi;

	/* resolve the field */

	if (!(fi = helper_resolve_fieldinfo(uf)))
		return -1;

	/* return the field value's offset */

	return fi->offset;
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
