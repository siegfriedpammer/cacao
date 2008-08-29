/* src/vm/jit/i386/patcher.c - i386 code patching functions

   Copyright (C) 1996-2005, 2006, 2007, 2008
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

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

*/


#include "config.h"

#include <stdint.h>

#include "vm/types.h"

#include "vm/jit/i386/codegen.h"
#include "vm/jit/i386/md.h"

#include "mm/memory.h"

#include "native/native.hpp"

#include "vm/jit/builtin.hpp"
#include "vm/class.h"
#include "vm/field.hpp"
#include "vm/initialize.h"
#include "vm/options.h"
#include "vm/references.h"
#include "vm/resolve.h"

#include "vm/jit/patcher-common.hpp"


#define PATCH_BACK_ORIGINAL_MCODE							\
	do {													\
		*((uint16_t*) pr->mpc) = (uint16_t) pr->mcode;		\
		md_icacheflush((void*) pr->mpc, PATCHER_CALL_SIZE);	\
	} while (0)


/* patcher_patch_code **********************************************************

   Just patches back the original machine code.

*******************************************************************************/

void patcher_patch_code(patchref_t *pr)
{
	PATCH_BACK_ORIGINAL_MCODE;
}


/* patcher_get_putstatic *******************************************************

   Machine code:

   <patched call position>
   b8 00 00 00 00             mov    $0x00000000,%eax

*******************************************************************************/

bool patcher_get_putstatic(patchref_t *pr)
{
	u1               *ra;
	unresolved_field *uf;
	fieldinfo        *fi;

	/* get stuff from the stack */

	ra    = (u1 *)               pr->mpc;
	uf    = (unresolved_field *) pr->ref;

	/* get the fieldinfo */

	if (!(fi = resolve_field_eager(uf)))
		return false;

	/* check if the field's class is initialized */

	if (!(fi->clazz->state & CLASS_INITIALIZED))
		if (!initialize_class(fi->clazz))
			return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch the field value's address */

	*((intptr_t *) (ra + 1)) = (intptr_t) fi->value;

	return true;
}


/* patcher_getfield ************************************************************

   Machine code:

   <patched call position>
   8b 88 00 00 00 00          mov    0x00000000(%eax),%ecx

*******************************************************************************/

bool patcher_getfield(patchref_t *pr)
{
	u1               *ra;
	unresolved_field *uf;
	fieldinfo        *fi;

	/* get stuff from the stack */

	ra    = (u1 *)               pr->mpc;
	uf    = (unresolved_field *) pr->ref;

	/* get the fieldinfo */

	if (!(fi = resolve_field_eager(uf)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch the field's offset */

	*((u4 *) (ra + 2)) = (u4) (fi->offset);

	/* if the field has type long, we need to patch the second move too */

	if (fi->type == TYPE_LNG)
		*((u4 *) (ra + 6 + 2)) = (u4) (fi->offset + 4);

	return true;
}


/* patcher_putfield ************************************************************

   Machine code:

   <patched call position>
   8b 88 00 00 00 00          mov    0x00000000(%eax),%ecx

*******************************************************************************/

bool patcher_putfield(patchref_t *pr)
{
	u1               *ra;
	unresolved_field *uf;
	fieldinfo        *fi;

	/* get stuff from the stack */

	ra    = (u1 *)               pr->mpc;
	uf    = (unresolved_field *) pr->ref;

	/* get the fieldinfo */

	if (!(fi = resolve_field_eager(uf)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch the field's offset */

	if (fi->type != TYPE_LNG) {
		*((u4 *) (ra + 2)) = (u4) (fi->offset);
	}
	else {
		/* The long code is special:
		 *
		 * 89 8d 00 00 00 00          mov    %ecx,0x00000000(%ebp)
		 * 89 95 00 00 00 00          mov    %edx,0x00000000(%ebp)
		 */

		*((u4 *) (ra + 2))     = (u4) (fi->offset);
		*((u4 *) (ra + 6 + 2)) = (u4) (fi->offset + 4);
	}

	return true;
}


/* patcher_putfieldconst *******************************************************

   Machine code:

   <patched call position>
   c7 85 00 00 00 00 7b 00 00 00    movl   $0x7b,0x0(%ebp)

*******************************************************************************/

bool patcher_putfieldconst(patchref_t *pr)
{
	u1               *ra;
	unresolved_field *uf;
	fieldinfo        *fi;

	/* get stuff from the stack */

	ra    = (u1 *)               pr->mpc;
	uf    = (unresolved_field *) pr->ref;

	/* get the fieldinfo */

	if (!(fi = resolve_field_eager(uf)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch the field's offset */

	if (!IS_2_WORD_TYPE(fi->type)) {
		*((u4 *) (ra + 2)) = (u4) (fi->offset);
	}
	else {
		/* long/double code is different:
		 *
		 * c7 80 00 00 00 00 c8 01 00 00    movl   $0x1c8,0x0(%eax)
		 * c7 80 04 00 00 00 00 00 00 00    movl   $0x0,0x4(%eax)
		 */

		*((u4 *) (ra + 2))      = (u4) (fi->offset);
		*((u4 *) (ra + 10 + 2)) = (u4) (fi->offset + 4);
	}

	return true;
}


/* patcher_aconst **************************************************************

   Machine code:

   <patched call postition>
   c7 04 24 00 00 00 00       movl   $0x0000000,(%esp)
   b8 00 00 00 00             mov    $0x0000000,%eax

*******************************************************************************/

bool patcher_aconst(patchref_t *pr)
{
	u1                *ra;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                pr->mpc;
	cr    = (constant_classref *) pr->ref;

	/* get the classinfo */

	if (!(c = resolve_classref_eager(cr)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch the classinfo pointer */

	*((ptrint *) (ra + 1)) = (ptrint) c;

	return true;
}


/* patcher_builtin_multianewarray **********************************************

   Machine code:

   <patched call position>
   c7 04 24 02 00 00 00       movl   $0x2,(%esp)
   c7 44 24 04 00 00 00 00    movl   $0x00000000,0x4(%esp)
   89 e0                      mov    %esp,%eax
   83 c0 0c                   add    $0xc,%eax
   89 44 24 08                mov    %eax,0x8(%esp)
   b8 00 00 00 00             mov    $0x00000000,%eax
   ff d0                      call   *%eax

*******************************************************************************/

bool patcher_builtin_multianewarray(patchref_t *pr)
{
	u1                *ra;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                pr->mpc;
	cr    = (constant_classref *) pr->ref;

	/* get the classinfo */

	if (!(c = resolve_classref_eager(cr)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch the classinfo pointer */

	*((ptrint *) (ra + 7 + 4)) = (ptrint) c;

	return true;
}


/* patcher_builtin_arraycheckcast **********************************************

   Machine code:

   <patched call position>
   c7 44 24 04 00 00 00 00    movl   $0x00000000,0x4(%esp)
   ba 00 00 00 00             mov    $0x00000000,%edx
   ff d2                      call   *%edx

*******************************************************************************/

bool patcher_builtin_arraycheckcast(patchref_t *pr)
{
	u1                *ra;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                pr->mpc;
	cr    = (constant_classref *) pr->ref;

	/* get the classinfo */

	if (!(c = resolve_classref_eager(cr)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch the classinfo pointer */

	*((ptrint *) (ra + 4)) = (ptrint) c;

	/* patch new function address */

	*((ptrint *) (ra + 8 + 1)) = (ptrint) BUILTIN_arraycheckcast;

	return true;
}


/* patcher_invokestatic_special ************************************************

   Machine code:

   <patched call position>
   b9 00 00 00 00             mov    $0x00000000,%ecx
   ff d1                      call   *%ecx

*******************************************************************************/

bool patcher_invokestatic_special(patchref_t *pr)
{
	u1                *ra;
	unresolved_method *um;
	methodinfo        *m;

	/* get stuff from the stack */

	ra    = (u1 *)                pr->mpc;
	um    = (unresolved_method *) pr->ref;

	/* get the fieldinfo */

	if (!(m = resolve_method_eager(um)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch stubroutine */

	*((ptrint *) (ra + 1)) = (ptrint) m->stubroutine;

	return true;
}


/* patcher_invokevirtual *******************************************************

   Machine code:

   <patched call position>
   8b 08                      mov    (%eax),%ecx
   8b 81 00 00 00 00          mov    0x00000000(%ecx),%eax
   ff d0                      call   *%eax

*******************************************************************************/

bool patcher_invokevirtual(patchref_t *pr)
{
	u1                *ra;
	unresolved_method *um;
	methodinfo        *m;

	/* get stuff from the stack */

	ra    = (u1 *)                pr->mpc;
	um    = (unresolved_method *) pr->ref;

	/* get the fieldinfo */

	if (!(m = resolve_method_eager(um)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch vftbl index */

	*((s4 *) (ra + 2 + 2)) = (s4) (OFFSET(vftbl_t, table[0]) +
								   sizeof(methodptr) * m->vftblindex);

	return true;
}


/* patcher_invokeinterface *****************************************************

   Machine code:

   <patched call position>
   8b 00                      mov    (%eax),%eax
   8b 88 00 00 00 00          mov    0x00000000(%eax),%ecx
   8b 81 00 00 00 00          mov    0x00000000(%ecx),%eax
   ff d0                      call   *%eax

*******************************************************************************/

bool patcher_invokeinterface(patchref_t *pr)
{
	u1                *ra;
	unresolved_method *um;
	methodinfo        *m;

	/* get stuff from the stack */

	ra    = (u1 *)                pr->mpc;
	um    = (unresolved_method *) pr->ref;

	/* get the fieldinfo */

	if (!(m = resolve_method_eager(um)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch interfacetable index */

	*((s4 *) (ra + 2 + 2)) = (s4) (OFFSET(vftbl_t, interfacetable[0]) -
								   sizeof(methodptr) * m->clazz->index);

	/* patch method offset */

	*((s4 *) (ra + 2 + 6 + 2)) =
		(s4) (sizeof(methodptr) * (m - m->clazz->methods));

	return true;
}


/* patcher_checkcast_instanceof_flags ******************************************

   Machine code:

   <patched call position>
   b9 00 00 00 00             mov    $0x00000000,%ecx

*******************************************************************************/

bool patcher_checkcast_instanceof_flags(patchref_t *pr)
{
	u1                *ra;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                pr->mpc;
	cr    = (constant_classref *) pr->ref;

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch class flags */

	*((s4 *) (ra + 1)) = (s4) c->flags;

	return true;
}


/* patcher_checkcast_interface *************************************************

   Machine code:

   <patched call position>
   8b 91 00 00 00 00          mov    0x00000000(%ecx),%edx
   81 ea 00 00 00 00          sub    $0x00000000,%edx
   85 d2                      test   %edx,%edx
   0f 8f 06 00 00 00          jg     0x00000000
   8b 35 03 00 00 00          mov    0x3,%esi
   8b 91 00 00 00 00          mov    0x00000000(%ecx),%edx

*******************************************************************************/

bool patcher_checkcast_interface(patchref_t *pr)
{
	u1                *ra;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                pr->mpc;
	cr    = (constant_classref *) pr->ref;

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch super class index */

	*((s4 *) (ra + 6 + 2)) = (s4) c->index;

	*((s4 *) (ra + 6 + 6 + 2 + 6 + 6 + 2)) =
		(s4) (OFFSET(vftbl_t, interfacetable[0]) -
			  c->index * sizeof(methodptr*));

	return true;
}


/* patcher_instanceof_interface ************************************************

   Machine code:

   <patched call position>
   8b 91 00 00 00 00          mov    0x00000000(%ecx),%edx
   81 ea 00 00 00 00          sub    $0x00000000,%edx
   85 d2                      test   %edx,%edx
   0f 8e 13 00 00 00          jle    0x00000000
   8b 91 00 00 00 00          mov    0x00000000(%ecx),%edx

*******************************************************************************/

bool patcher_instanceof_interface(patchref_t *pr)
{
	u1                *ra;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                pr->mpc;
	cr    = (constant_classref *) pr->ref;

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch super class index */

	*((s4 *) (ra + 6 + 2)) = (s4) c->index;

	*((s4 *) (ra + 6 + 6 + 2 + 6 + 2)) =
		(s4) (OFFSET(vftbl_t, interfacetable[0]) -
			  c->index * sizeof(methodptr*));

	return true;
}


/* patcher_checkcast_class *****************************************************

   Machine code:

   <patched call position>
   ba 00 00 00 00             mov    $0x00000000,%edx
   8b 89 00 00 00 00          mov    0x00000000(%ecx),%ecx
   8b 92 00 00 00 00          mov    0x00000000(%edx),%edx
   29 d1                      sub    %edx,%ecx
   ba 00 00 00 00             mov    $0x00000000,%edx

*******************************************************************************/

bool patcher_checkcast_class(patchref_t *pr)
{
	u1                *ra;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                pr->mpc;
	cr    = (constant_classref *) pr->ref;

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch super class' vftbl */

	*((ptrint *) (ra + 1)) = (ptrint) c->vftbl;
	*((ptrint *) (ra + 5 + 6 + 6 + 2 + 1)) = (ptrint) c->vftbl;

	return true;
}


/* patcher_instanceof_class ****************************************************

   Machine code:

   <patched call position>
   b9 00 00 00 00             mov    $0x0,%ecx
   8b 40 14                   mov    0x14(%eax),%eax
   8b 51 18                   mov    0x18(%ecx),%edx
   8b 49 14                   mov    0x14(%ecx),%ecx

*******************************************************************************/

bool patcher_instanceof_class(patchref_t *pr)
{
	u1                *ra;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                pr->mpc;
	cr    = (constant_classref *) pr->ref;

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch super class' vftbl */

	*((ptrint *) (ra + 1)) = (ptrint) c->vftbl;

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
 * vim:noexpandtab:sw=4:ts=4:
 */
