/* src/vm/jit/intrp/patcher.c - Interpreter code patching functions

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

   $Id: patcher.c 3138 2005-09-02 15:15:18Z twisti $

*/


#include "vm/jit/intrp/types.h"

#include "mm/memory.h"
#include "native/native.h"
#include "vm/builtin.h"
#include "vm/field.h"
#include "vm/initialize.h"
#include "vm/options.h"
#include "vm/references.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/helper.h"
#include "vm/jit/patcher.h"


/* patcher_get_putstatic *******************************************************

   Machine code:

*******************************************************************************/

bool patcher_get_putstatic(u1 *sp)
{
	ptrint            *ip;
	unresolved_field  *uf;
	fieldinfo         *fi;

	ip = (ptrint *) sp;
	uf = (unresolved_field *) ip[2];

	/* get the fieldinfo */

	if (!(fi = helper_resolve_fieldinfo(uf))) {
		return false;
	}

	/* check if the field's class is initialized */

	if (!fi->class->initialized) {
		if (!initialize_class(fi->class)) {
			return false;
		}
	}

	/* patch the field's address */

	ip[1] = (ptrint) &(fi->value);

	return true;
}


/* patcher_get_putfield ********************************************************

   Machine code:

*******************************************************************************/

bool patcher_get_putfield(u1 *sp)
{
	ptrint            *ip;
	unresolved_field  *uf;
	fieldinfo         *fi;

	ip = (ptrint *) sp;
	uf = (unresolved_field *) ip[2];

	/* get the fieldinfo */

	if (!(fi = helper_resolve_fieldinfo(uf))) {
		return false;
	}

	/* patch the field's offset */

	ip[1] = fi->offset;

	return true;
}


/* patcher_builtin_new *********************************************************

   Machine code:

*******************************************************************************/

bool patcher_builtin_new(u1 *sp)
{
	ptrint            *ip;
	constant_classref *cr;
	classinfo         *c;

	ip = (ptrint *) sp;
	cr = (constant_classref *) ip[-1];
	
	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		return false;
	}

	/* patch the classinfo pointer */

	ip[1] = (ptrint) c;

	return true;
}


/* patcher_builtin_newarray ****************************************************

   Machine code:

   INFO: This one is also used for arrayinstanceof!

*******************************************************************************/

bool patcher_builtin_newarray(u1 *sp)
{
	ptrint            *ip;
	constant_classref *cr;
	classinfo         *c;

	ip = (ptrint *) sp;
	cr = (constant_classref *) ip[-1];

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		return false;
	}

	/* patch the class' vftbl pointer */

	ip[1] = (ptrint) c->vftbl;

	return true;
}


bool patcher_builtin_arrayinstanceof(u1 *sp)
{
	return patcher_builtin_newarray(sp);
}


/* patcher_builtin_multianewarray **********************************************

   Machine code:

*******************************************************************************/

bool patcher_builtin_multianewarray(u1 *sp)
{
	ptrint            *ip;
	constant_classref *cr;
	classinfo         *c;

	ip = (ptrint *) sp;
	cr = (constant_classref *) ip[3];

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		return false;
	}

	/* patch the class' vftbl pointer */

	ip[1] = (ptrint) c->vftbl;

	return true;
}


/* patcher_builtin_arraycheckcast **********************************************

   Machine code:

*******************************************************************************/

bool patcher_builtin_arraycheckcast(u1 *sp)
{
	ptrint            *ip;
	constant_classref *cr;
	classinfo         *c;

	ip = (ptrint *) sp;
	cr = (constant_classref *) ip[2];

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		return false;
	}

	/* patch the class' vftbl pointer */

	ip[1] = (ptrint) c->vftbl;

	return true;
}


/* patcher_invokestatic_special ************************************************

   Machine code:

******************************************************************************/

bool patcher_invokestatic_special(u1 *sp)
{
	ptrint            *ip;
	unresolved_method *um;
	methodinfo        *m;

	ip = (ptrint *) sp;
	um = (unresolved_method *) ip[3];

	/* get the fieldinfo */

	if (!(m = helper_resolve_methodinfo(um))) {
		return false;
	}

	/* patch stubroutine */

	ip[1] = (ptrint) m->stubroutine;

	return true;
}


/* patcher_invokevirtual *******************************************************

   Machine code:

*******************************************************************************/

bool patcher_invokevirtual(u1 *sp)
{
	ptrint            *ip;
	unresolved_method *um;
	methodinfo        *m;

	ip = (ptrint *) sp;
	um = (unresolved_method *) ip[3];

	/* get the fieldinfo */

	if (!(m = helper_resolve_methodinfo(um))) {
		return false;
	}

	/* patch vftbl index */

	ip[1] = (OFFSET(vftbl_t, table[0]) + sizeof(methodptr) * m->vftblindex);

	return true;
}


/* patcher_invokeinterface *****************************************************

   Machine code:

*******************************************************************************/

bool patcher_invokeinterface(u1 *sp)
{
	ptrint            *ip;
	unresolved_method *um;
	methodinfo        *m;

	ip = (ptrint *) sp;
	um = (unresolved_method *) ip[4];

	/* get the methodinfo */

	if (!(m = helper_resolve_methodinfo(um))) {
		return false;
	}

	/* patch interfacetable index */

	ip[1] = (OFFSET(vftbl_t, interfacetable[0]) -
			 sizeof(methodptr*) * m->class->index);

	/* patch method offset */

	ip[2] = (sizeof(methodptr) * (m - m->class->methods));

	return true;
}


/* patcher_checkcast_instanceof ************************************************

   Machine code:

*******************************************************************************/

bool patcher_checkcast_instanceof(u1 *sp)
{
	ptrint            *ip;
	constant_classref *cr;
	classinfo         *c;

	ip = (ptrint *) sp;
	cr = (constant_classref *) ip[2];

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		return false;
	}

	/* patch super class pointer */

	ip[1] = (ptrint) c;

	return true;
}


/* patcher_resolve_native ******************************************************

   XXX

*******************************************************************************/

#if !defined(ENABLE_STATICVM)
bool patcher_resolve_native(u1 *sp)
{
	ptrint      *ip;
	methodinfo  *m;
	functionptr  f;

	ip = (ptrint *) sp;
	m = (methodinfo *) ip[1];

	/* resolve native function */

	if (!(f = native_resolve_function(m))) {
		return false;
	}

	/* patch native function pointer */

	ip[2] = (ptrint) f;

	return true;
}
#endif /* !defined(ENABLE_STATICVM) */


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
