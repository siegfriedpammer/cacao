/* src/vm/jit/x86_64/patcher.c - x86_64 code patching functions

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

   Changes:

   $Id: patcher.c 8096 2007-06-17 13:45:58Z pm $

     GENERATED      PATCHER BRANCH           AFTER PATCH

   Short patcher call:

     foo            bras %r14, OFFSET        foo
     bar     ===>   bar                ===>  bar
     baz            baz                      baz

   Short patcher call with nops:
                                             PATCHER_NOPS_SKIP <-+
                                                                 |
     nop            bras %r14, OFFSET        nop               --+
     nop     ===>   nop                ===>  nop                 |
     nop            nop                      nop               --+
     foo            foo                      foo

   Long pacher call:
                                PATCHER_LONGBRANCHES_NOPS_SKIP <-+
                                                                 |
     br exit:       ild itmp3, disp(pv)      br exit            -+
	 nop            aadd pv, itmp3           aadd pv, itmp3      | 
	 nop     ===>   nop                ===>  nop                 |
	 .....          ....                     ....                |
	 nop            basr itmp3, itmp3        basr itmp3, itmp3  -+
   exit:                                   exit:

*/


#include "config.h"
#include "vm/types.h"

#include "vm/jit/s390/codegen.h"
#include "vm/jit/s390/md-abi.h"

#include "mm/memory.h"
#include "native/native.h"
#include "vm/builtin.h"
#include "vmcore/class.h"
#include "vm/exceptions.h"
#include "vmcore/field.h"
#include "vm/initialize.h"
#include "vmcore/options.h"
#include "vmcore/references.h"
#include "vm/resolve.h"
#include "vm/jit/patcher.h"
#include "vm/jit/stacktrace.h"

#include <assert.h>
#define OOPS() assert(0);
#define __PORTED__

/* A normal patcher branch done using BRAS */
#define PATCHER_IS_SHORTBRANCH(brcode) ((brcode & 0xFF0F0000) == 0xA7050000)

/* patcher_wrapper *************************************************************

   Wrapper for all patchers.  It also creates the stackframe info
   structure.

   If the return value of the patcher function is false, it gets the
   exception object, clears the exception pointer and returns the
   exception.

*******************************************************************************/

java_objectheader *patcher_wrapper(u1 *sp, u1 *pv, u1 *ra)
{
	stackframeinfo     sfi;
	u1                *xpc;
	java_objectheader *o;
	functionptr        f;
	bool               result;
	java_objectheader *e;

	/* define the patcher function */

	bool (*patcher_function)(u1 *);

	/* get stuff from the stack */

	xpc = (u1 *)                *((ptrint *) (sp + 5 * 4));
	o   = (java_objectheader *) *((ptrint *) (sp + 4 * 4));
	f   = (functionptr)         *((ptrint *) (sp + 0 * 4));
	
	/* For a normal branch, the patch position is SZ_BRAS bytes before the RA.
	 * For long branches it is PATCHER_LONGBRANCHES_NOPS_SKIP before the RA.
	 */

	if (PATCHER_IS_SHORTBRANCH(*(u4 *)(xpc - SZ_BRAS))) {
		xpc = xpc - SZ_BRAS;
	} else {
		xpc = xpc - PATCHER_LONGBRANCHES_NOPS_SKIP;
	}

	*((ptrint *) (sp + 5 * 4)) = (ptrint) xpc;

	/* store PV into the patcher function position */

	*((ptrint *) (sp + 0 * 4)) = (ptrint) pv;

	/* cast the passed function to a patcher function */

	patcher_function = (bool (*)(u1 *)) (ptrint) f;

	/* enter a monitor on the patching position */

	PATCHER_MONITORENTER;

	/* create the stackframeinfo */

	/* RA is passed as NULL, but the XPC is correct and can be used in
	   stacktrace_create_extern_stackframeinfo for
	   md_codegen_get_pv_from_pc. */

	stacktrace_create_extern_stackframeinfo(&sfi, pv, sp + (6 * 4), ra, xpc);

	/* call the proper patcher function */

	result = (patcher_function)(sp);

	/* remove the stackframeinfo */

	stacktrace_remove_stackframeinfo(&sfi);

	/* check for return value and exit accordingly */

	if (result == false) {
		e = exceptions_get_and_clear_exception();

		PATCHER_MONITOREXIT;

		return e;
	}

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return NULL;
}


/* patcher_get_putstatic *******************************************************

   Machine code:

*******************************************************************************/

bool patcher_get_putstatic(u1 *sp)
{
	u1               *ra;
	u4                mcode;
	unresolved_field *uf;
	s4                disp;
	fieldinfo        *fi;
	u1               *pv;

	/* get stuff from the stack */

	ra    = (u1 *)               *((ptrint *) (sp + 5 * 4));
	mcode =                      *((u4 *)     (sp + 3 * 4));
	uf    = (unresolved_field *) *((ptrint *) (sp + 2 * 4));
	disp  =                      *((s4 *)     (sp + 1 * 4));
	pv    = (u1 *)               *((ptrint *) (sp + 0 * 4));

	/* get the fieldinfo */

	if (!(fi = resolve_field_eager(uf)))
		return false;

	/* check if the field's class is initialized */

	if (!(fi->class->state & CLASS_INITIALIZED))
		if (!initialize_class(fi->class))
			return false;

	*((ptrint *) (pv + disp)) = (ptrint) &(fi->value);

	/* patch back original code */

	*((u4 *) ra) = mcode;

	return true;
}


/* patcher_get_putfield ********************************************************

   Machine code:

*******************************************************************************/

bool patcher_get_putfield(u1 *sp)
{
	u1               *ra;
	u4                mcode, brcode;
	unresolved_field *uf;
	fieldinfo        *fi;
	u1                byte;
	s4                disp;

	/* get stuff from the stack */

	ra    = (u1 *)               *((ptrint *) (sp + 5 * 4));
	mcode =                      *((u4 *)     (sp + 3 * 4));
	uf    = (unresolved_field *) *((ptrint *) (sp + 2 * 4));
	disp  =                      *((s4 *)     (sp + 1 * 4));

	/* get the fieldinfo */

	if (!(fi = resolve_field_eager(uf)))
		return false;

	/* patch back original code */

	brcode = *((u4 *) ra);
	*((u4 *) ra) = mcode;

	/* If NOPs are generated, skip them */

	if (! PATCHER_IS_SHORTBRANCH(brcode))
		ra += PATCHER_LONGBRANCHES_NOPS_SKIP;
	else if (opt_shownops)
		ra += PATCHER_NOPS_SKIP;

	/* If there is an operand load before, skip the load size passed in disp (see ICMD_PUTFIELD) */

	ra += disp;

	/* patch correct offset */

	if (fi->type == TYPE_LNG) {
		assert(N_VALID_DISP(fi->offset + 4));
		/* 2 RX operations, for 2 words; each already contains a 0 or 4 offset. */
		*((u4 *) ra ) |= (fi->offset + (*((u4 *) ra) & 0xF));
		ra += 4;
		*((u4 *) ra ) |= (fi->offset + (*((u4 *) ra) & 0xF));
	} else {
		assert(N_VALID_DISP(fi->offset));
		/* 1 RX operation */
		*((u4 *) ra) |= fi->offset;
	}

	return true;
}

/* patcher_invokestatic_special ************************************************

   Machine code:

*******************************************************************************/

__PORTED__ bool patcher_invokestatic_special(u1 *sp)
{
	u1                *ra;
	u4                 mcode;
	unresolved_method *um;
	s4                 disp;
	u1                *pv;
	methodinfo        *m;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 5 * 4));
	mcode =                       *((u4 *)     (sp + 3 * 4));
	um    = (unresolved_method *) *((ptrint *) (sp + 2 * 4));
	disp  =                       *((s4 *)     (sp + 1 * 4));
	pv    = (u1 *)                *((ptrint *) (sp + 0 * 4));

	/* get the fieldinfo */

	if (!(m = resolve_method_eager(um)))
		return false;

	*((ptrint *) (pv + disp)) = (ptrint) m->stubroutine;

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* patch stubroutine */

	return true;
}


/* patcher_invokevirtual *******************************************************

   Machine code:

*******************************************************************************/

bool patcher_invokevirtual(u1 *sp)
{
	u1                *ra;
	u4                 mcode, brcode;
	unresolved_method *um;
	methodinfo        *m;
	s4                off;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 5 * 4));
	mcode =                       *((u4 *)     (sp + 3 * 4));
	um    = (unresolved_method *) *((ptrint *) (sp + 2 * 4));

	/* get the fieldinfo */

	if (!(m = resolve_method_eager(um)))
		return false;

	/* patch back original code */

	brcode = *((u4 *) ra);
	*((u4 *) ra) = mcode;

	/* If NOPs are generated, skip them */

	if (! PATCHER_IS_SHORTBRANCH(brcode))
		ra += PATCHER_LONGBRANCHES_NOPS_SKIP;
	else if (opt_shownops)
		ra += PATCHER_NOPS_SKIP;

	/* patch vftbl index */


	off = (s4) (OFFSET(vftbl_t, table[0]) +
								   sizeof(methodptr) * m->vftblindex);

	assert(N_VALID_DISP(off));

	*((s4 *)(ra + 4)) |= off;

	return true;
}


/* patcher_invokeinterface *****************************************************

   Machine code:

*******************************************************************************/

bool patcher_invokeinterface(u1 *sp)
{
	u1                *ra;
	u4                 mcode, brcode;
	unresolved_method *um;
	methodinfo        *m;
	s4                 idx, off;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 5 * 4));
	mcode =                       *((u4 *)     (sp + 3 * 4));
	um    = (unresolved_method *) *((ptrint *) (sp + 2 * 4));

	/* get the fieldinfo */

	if (!(m = resolve_method_eager(um)))
		return false;

	/* patch back original code */

	brcode = *((u4 *) ra);
	*((u4 *) ra) = mcode;

	/* If NOPs are generated, skip them */

	if (! PATCHER_IS_SHORTBRANCH(brcode))
		ra += PATCHER_LONGBRANCHES_NOPS_SKIP;
	else if (opt_shownops)
		ra += PATCHER_NOPS_SKIP;

	/* get interfacetable index */

	idx = (s4) (OFFSET(vftbl_t, interfacetable[0]) -
		sizeof(methodptr) * m->class->index) + 
		N_DISP_MAX;

	ASSERT_VALID_DISP(idx);

	/* get method offset */

	off =
		(s4) (sizeof(methodptr) * (m - m->class->methods));
	ASSERT_VALID_DISP(off);

	/* patch them */

	*((s4 *)(ra + 4)) |= idx;
	*((s4 *)(ra + 4 + 4)) |= off;

	return true;
}


/* patcher_resolve_classref_to_flags *******************************************

   CHECKCAST/INSTANCEOF:

   <patched call position>

*******************************************************************************/

__PORTED__ bool patcher_resolve_classref_to_flags(u1 *sp)
{
	constant_classref *cr;
	s4                 disp;
	u1                *pv;
	classinfo         *c;
	u4                 mcode;
	u1                *ra;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 5 * 4));
	mcode =                       *((u4 *)     (sp + 3 * 4));
	cr    = (constant_classref *) *((ptrint *) (sp + 2 * 4));
	disp  =                       *((s4 *)     (sp + 1 * 4));
	pv    = (u1 *)                *((ptrint *) (sp + 0 * 4));

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr)))
		return false;

	/* patch class flags */

	*((s4 *) (pv + disp)) = (s4) c->flags;

	/* patch back original code */

	*((u4 *) ra) = mcode;

	return true;
}

/* patcher_resolve_classref_to_classinfo ***************************************

   ACONST:
   MULTIANEWARRAY:
   ARRAYCHECKCAST:

*******************************************************************************/

__PORTED__ bool patcher_resolve_classref_to_classinfo(u1 *sp)
{
	constant_classref *cr;
	s4                 disp;
	u1                *pv;
	classinfo         *c;
	u4                 mcode;
	u1                *ra;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 5 * 4));
	mcode =                       *((u4 *)     (sp + 3 * 4));
	cr    = (constant_classref *) *((ptrint *) (sp + 2 * 4));
	disp  =                       *((s4 *)     (sp + 1 * 4));
	pv    = (u1 *)                *((ptrint *) (sp + 0 * 4));

	/* get the classinfo */

	if (!(c = resolve_classref_eager(cr)))
		return false;

	/* patch the classinfo pointer */

	*((ptrint *) (pv + disp)) = (ptrint) c;

	/* patch back original code */

	*((u4 *) ra) = mcode;

	return true;
}

/* patcher_resolve_classref_to_vftbl *******************************************

   CHECKCAST (class):
   INSTANCEOF (class):

*******************************************************************************/

bool patcher_resolve_classref_to_vftbl(u1 *sp)
{
	constant_classref *cr;
	s4                 disp;
	u1                *pv;
	classinfo         *c;
	u4                 mcode;
	u1                *ra;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 5 * 4));
	mcode =                       *((u4 *)     (sp + 3 * 4));
	cr    = (constant_classref *) *((ptrint *) (sp + 2 * 4));
	disp  =                       *((s4 *)     (sp + 1 * 4));
	pv    = (u1 *)                *((ptrint *) (sp + 0 * 4));

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr)))
		return false;

	/* patch super class' vftbl */

	*((ptrint *) (pv + disp)) = (ptrint) c->vftbl;

	/* patch back original code */

	*((u4 *) ra) = mcode;

	return true;
}

/* patcher_checkcast_instanceof_interface **************************************

   Machine code:

*******************************************************************************/

bool patcher_checkcast_instanceof_interface(u1 *sp)
{
	u1                *ra;
	u4                 mcode, brcode;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 5 * 4));
	mcode =                       *((u4 *)     (sp + 3 * 4));
	cr    = (constant_classref *) *((ptrint *) (sp + 2 * 4));

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr)))
		return false;

	/* patch back original code */

	brcode = *((u4 *) ra);
	*((u4 *) ra) = mcode;

	/* If NOPs are generated, skip them */

	if (! PATCHER_IS_SHORTBRANCH(brcode))
		ra += PATCHER_LONGBRANCHES_NOPS_SKIP;
	else if (opt_shownops)
		ra += PATCHER_NOPS_SKIP;

	/* patch super class index */

	/* From here, split your editor and open codegen.c */

	switch (*(ra + 1) >> 4) {
		case REG_ITMP1: 
			/* First M_ALD is into ITMP1 */
			/* INSTANCEOF code */

			*(u4 *)(ra + SZ_L + SZ_L) |= (u2)(s2)(- c->index);
			*(u4 *)(ra + SZ_L + SZ_L + SZ_AHI + SZ_BRC) |=
				(u2)(s2)(OFFSET(vftbl_t, interfacetable[0]) -
					c->index * sizeof(methodptr*));

			break;

		case REG_ITMP2:
			/* First M_ALD is into ITMP2 */
			/* CHECKCAST code */

			*(u4 *)(ra + SZ_L + SZ_L) |= (u2)(s2)(- c->index);
			*(u4 *)(ra + SZ_L + SZ_L + SZ_AHI + SZ_BRC + SZ_ILL) |=
				(u2)(s2)(OFFSET(vftbl_t, interfacetable[0]) -
					c->index * sizeof(methodptr*));

			break;

		default:
			assert(0);
			break;
	}

	return true;
}

/* patcher_clinit **************************************************************

   May be used for GET/PUTSTATIC and in native stub.

   Machine code:

*******************************************************************************/

__PORTED__ bool patcher_clinit(u1 *sp)
{
	u1        *ra;
	u4         mcode;
	classinfo *c;

	/* get stuff from the stack */

	ra    = (u1 *)        *((ptrint *) (sp + 5 * 4));
	mcode =               *((u4 *)     (sp + 3 * 4));
	c     = (classinfo *) *((ptrint *) (sp + 2 * 4));

	/* check if the class is initialized */

	if (!(c->state & CLASS_INITIALIZED))
		if (!initialize_class(c))
			return false;

	/* patch back original code */

	*((u4 *) ra) = mcode;

	return true;
}


/* patcher_athrow_areturn ******************************************************

   Machine code:

   <patched call position>

*******************************************************************************/

#ifdef ENABLE_VERIFIER
__PORTED__ bool patcher_athrow_areturn(u1 *sp)
{
	u1               *ra;
	u4                mcode;
	unresolved_class *uc;

	/* get stuff from the stack */

	ra    = (u1 *)               *((ptrint *) (sp + 5 * 4));
	mcode =                      *((u4 *)     (sp + 3 * 4));
	uc    = (unresolved_class *) *((ptrint *) (sp + 2 * 4));

	/* resolve the class and check subtype constraints */

	if (!resolve_class_eager_no_access_check(uc))
		return false;

	/* patch back original code */

	*((u4 *) ra) = mcode;

	return true;
}
#endif /* ENABLE_VERIFIER */


/* patcher_resolve_native ******************************************************

*******************************************************************************/

#if !defined(WITH_STATIC_CLASSPATH)
__PORTED__ bool patcher_resolve_native(u1 *sp)
{
	u1          *ra;
	u4           mcode;
	methodinfo  *m;
	functionptr  f;
	s4           disp;
	u1          *pv;

	/* get stuff from the stack */

	ra    = (u1 *)         *((ptrint *) (sp + 5 * 4));
	mcode =                *((u4 *)     (sp + 3 * 4));
	disp  =                *((s4 *)     (sp + 1 * 4));
	m     = (methodinfo *) *((ptrint *) (sp + 2 * 4));
	pv    = (u1 *)         *((ptrint *) (sp + 0 * 4));

	/* resolve native function */

	if (!(f = native_resolve_function(m)))
		return false;

	/* patch native function pointer */

	*((ptrint *) (pv + disp)) = (ptrint) f;

	/* patch back original code */

	*((u4 *) ra) = mcode;

	return true;
}
#endif /* !defined(WITH_STATIC_CLASSPATH) */


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
