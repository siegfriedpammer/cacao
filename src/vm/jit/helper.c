/* src/vm/jit/asmpart.c - code patching helper functions

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

   $Id: helper.c 2243 2005-04-06 15:49:22Z twisti $

*/


#include "vm/class.h"
#include "vm/exceptions.h"
#include "vm/method.h"
#include "vm/references.h"
#include "vm/resolve.h"

/* XXX class_resolveclassmethod */
#include "vm/loader.h"


/* asm_builtin_new_helper ******************************************************

   This function is called from asm_builtin_new code patching function.

*******************************************************************************/

classinfo *asm_builtin_new_helper(constant_classref *cr)
{
	classinfo  *c;

	/* resolve and load the class */

	if (!resolve_classref(NULL, cr, resolveEager, true, &c))
		return NULL;

	/* return the classinfo pointer */

	return c;
}


/* asm_invokespecial_helper ****************************************************

   This function is called from asm_invokespecial code patching function.

*******************************************************************************/

u1 *asm_invokespecial_helper(unresolved_method *um)
{
	methodinfo *m;

	/* resolve the method */

	if (!resolve_method(um, resolveEager, &m))
		return NULL;

	/* return the method pointer */

	return m->stubroutine;
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
