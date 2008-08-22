/* src/vm/jit/m68k/patcher.c - m68k patcher functions

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

#include <assert.h>
#include <stdint.h>

#include "vm/types.h"

#include "vm/jit/m68k/md.h"

#include "mm/memory.h"
#include "native/native.h"

#include "vm/jit/builtin.hpp"
#include "vm/class.h"
#include "vm/field.hpp"
#include "vm/initialize.h"
#include "vm/options.h"
#include "vm/references.h"
#include "vm/resolve.h"

#include "vm/jit/asmpart.h"
#include "vm/jit/patcher-common.h"

#include "codegen.h"

#define PATCH_BACK_ORIGINAL_MCODE  *((u4*)(pr->mpc)) = pr->mcode

/* patcher_initialize_class ****************************************************

	just patch back original code

*******************************************************************************/


void patcher_patch_code(patchref_t *pr)	
{
#if 0
	u1* xpc    = (u1 *)      *((ptrint *) (sp + 6 * 4));
	u4 mcode  = *((u4*)      (sp + 3 * 4));
	u4 xmcode = *((u4*)      (sp + 2 * 4));

	*((u4*)(xpc)) 	= mcode;
	*((u4*)(xpc+4)) = xmcode;
#endif

	PATCH_BACK_ORIGINAL_MCODE;
	md_icacheflush((void*)pr->mpc, 2);

}


#if 0
/* patcher_initialize_class ****************************************************

   Initalizes a given classinfo pointer.  This function does not patch
   any data.

*******************************************************************************/

bool patcher_initialize_class(patchref_t *pr)
{
	classinfo *c;
	u4		   xpc, mcode, xmcode;

	/* get stuff from the stack */
	c = (classinfo *) *((ptrint *) (sp + 1 * 4));

	/* check if the class is initialized */
	if (!(c->state & CLASS_INITIALIZED))
		if (!initialize_class(c))
			return false;

	/* patch back original code */
	patcher_patch_back(sp);

	return true;
}
#endif

/* patcher_invokevirtual *******************************************************

   Machine code:
0x4029bc46:   61ff 0000 00ba    bsrl 0x4029bd02
0x4029bc4c:   246f 0000         moveal %sp@(0),%a2
0x4029bc50:   266a 0000         moveal %a2@(0),%a3
0x4029bc54:   246b 0000         moveal %a3@(0),%a2 	<-- patch this (0) offset
0x4029bc58:   4e92              jsr %a2@

*******************************************************************************/

bool patcher_invokevirtual(patchref_t *pr)
{
	u1                *ra;
	unresolved_method *um;
	methodinfo        *m;
	s2                 disp;

	/* get stuff from the stack */
	ra = (u1 *)                pr->mpc;
	um = (unresolved_method *) pr->ref;

	/* get the fieldinfo */
	if (!(m = resolve_method_eager(um)))
		return false;

	/* patch back original code */
	PATCH_BACK_ORIGINAL_MCODE;

	assert( *((u2*)(ra+8)) == 0x286b);

	/* patch vftbl index */
	disp = (OFFSET(vftbl_t, table[0]) + sizeof(methodptr) * m->vftblindex);
	*((s2 *) (ra + 10)) = disp;

	/* synchronize instruction cache */
	md_icacheflush(pr->mpc, 10 + 2 + PATCHER_CALL_SIZE);

	return true;
}

/* patcher_invokestatic_special ************************************************

   Machine code:

   INVOKESPECIAL
0x402902bc:   61ff 0000 0076    bsrl 0x40290334
0x402902c2:   247c 0000 0000    moveal #0,%a2		<-- this #0
0x402902c8:   4e92              jsr %a2@

******************************************************************************/

bool patcher_invokestatic_special(patchref_t *pr)
{
	unresolved_method *um;
	s4                 disp;
	methodinfo        *m;

	/* get stuff from the stack */
	disp =                       pr->mpc;
	um   = (unresolved_method *) pr->ref;

	/* get the fieldinfo */
	if (!(m = resolve_method_eager(um)))
		return false;

	/* patch back original code */
	PATCH_BACK_ORIGINAL_MCODE;

	*((ptrint *) (disp+2)) = (ptrint) m->stubroutine;

	/* synchronize inst cache */

	md_icacheflush(pr->mpc, PATCHER_CALL_SIZE+2+SIZEOF_VOID_P);

	return true;
}


/* patcher_resolve_classref_to_classinfo ***************************************
  ACONST:
  	0x4028f2ca:   2479 0000 0000    moveal 0x00000000,%a2
*******************************************************************************/
bool patcher_resolve_classref_to_classinfo(patchref_t *pr)
{
	constant_classref *cr;
	s4                 disp;
	u1                *pv;
	classinfo         *c;

	/* get stuff from the stack */
	cr   = (constant_classref *) pr->ref;
	disp =                       pr->mpc;

	/* get the classinfo */
	if (!(c = resolve_classref_eager(cr)))
		return false;

	/* patch back original code */
	PATCH_BACK_ORIGINAL_MCODE;

	/* patch the classinfo pointer */
	*((ptrint *) (disp+2)) = (ptrint) c;

	/* synchronize inst cache */
	md_icacheflush(pr->mpc, PATCHER_CALL_SIZE + 2 + SIZEOF_VOID_P);

	return true;
}

/* patcher_get_putstatic *******************************************************

   Machine code:

*******************************************************************************/

bool patcher_get_putstatic(patchref_t *pr)
{
	unresolved_field *uf;
	s4                disp;
	fieldinfo        *fi;

	/* get stuff from the stack */
	uf    = (unresolved_field *)  pr->ref;
	disp  =                       pr->mpc;

	/* get the fieldinfo */
	if (!(fi = resolve_field_eager(uf)))
		return false;

	/* check if the field's class is initialized */
	if (!(fi->clazz->state & CLASS_INITIALIZED))
		if (!initialize_class(fi->clazz))
			return false;

	/* patch back original code */
	PATCH_BACK_ORIGINAL_MCODE;
	md_icacheflush(pr->mpc, 2);

	/* patch the field value's address */
	assert(*((uint16_t*)(disp)) == 0x247c);
	*((intptr_t *) (disp+2)) = (intptr_t) fi->value;

	/* synchronize inst cache */
	md_icacheflush((void*)(disp+2), SIZEOF_VOID_P);

	return true;
}

/* patcher_get_putfield ********************************************************

   Machine code:

   <patched call position>

*******************************************************************************/

bool patcher_get_putfield(patchref_t *pr)
{
	u1               *ra;
	unresolved_field *uf;
	fieldinfo        *fi;

	ra = (u1 *)               pr->mpc;
	uf = (unresolved_field *) pr->ref;

	/* get the fieldinfo */
	if (!(fi = resolve_field_eager(uf)))
		return false;

	/* patch back original code */
	PATCH_BACK_ORIGINAL_MCODE;
	md_icacheflush(pr->mpc, 2);

	/* patch the field's offset */
	if (IS_LNG_TYPE(fi->type)) {
		/*
		 *	0x40d05bb2:     0x25440000	movel %d4,%a2@(0)
		 *	0x40d05bb6:     0x25430004	movel %d3,%a2@(4)
		 *					      ^^^^
		 *					      both 0000 and 0004 have to be patched
		 */

		assert( (fi->offset & 0x0000ffff) == fi->offset );

		assert( (*((uint32_t*)ra) & 0xffff0000) == *((uint32_t*)ra) );
		assert( (*((uint32_t*)(ra+4)) & 0xffff0004) == *((uint32_t*)(ra+4)) );

		*((int16_t *) (ra + 2)) = (int16_t) ((fi->offset) & 0x0000ffff);
		*((int16_t *) (ra + 6)) = (int16_t) ((fi->offset + 4) & 0x0000ffff);

		md_icacheflush(ra, 2 * 4);
	} else	{
		/*	Multiple cases here, int, adr, flt and dbl. */
		if ( (*((uint32_t*)ra) & 0xfff00000) == 0xf2200000 )	{
			/* flt/dbl case 
			 * 0x40d3ddc2:     0xf22944c0 0x0000xxxx 	fsmoves %a1@(0),%fp1
			 * 	                            ^^^^
			 * 	                            patch here 
			 */
			assert( (fi->offset & 0x0000ffff) == fi->offset );
			assert( (*((uint32_t*)(ra+4)) & 0x0000ffff) == *((uint32_t*)(ra+4)) );
			*((int16_t*)(ra+4)) = (int16_t)fi->offset;

			md_icacheflush(ra+4, 1 * 4);
		} else	{
			/*	int/adr case
			 *	0x40adb3f6:     0x254d0000 	movel %a5,%a2@(0)
			 *	                      ^^^^                     ^
			 *	                      to be patched
			 */
			assert( (*((uint32_t*)ra) & 0xffff0000) == *((uint32_t*)ra) );
			assert( (fi->offset & 0x0000ffff) == fi->offset );
			*((int16_t*)(ra+2)) = (int16_t)fi->offset;

			/* synchronize instruction cache */
			md_icacheflush(ra, 1 * 4);
		}
	}

	return true;
}
/* patcher_resolve_classref_to_flags *******************************************

   CHECKCAST/INSTANCEOF:


CHECKCAST:
0x4029b056:   61ff 0000 013e    bsrl 0x4029b196
0x4029b05c:   263c 0000 0000    movel #0,%d3		<-- patch this #0
0x4029b062:   0283 0000 0200    andil #512,%d3

INSTANCEOF:
0x402a4aa8:   61ff 0000 05c4    bsrl 0x402a506e
0x402a4aae:   283c 0000 0000    movel #0,%d4		<-- same here
0x402a4ab4:   0284 0000 0200    andil #512,%d4


*******************************************************************************/

bool patcher_resolve_classref_to_flags(patchref_t *pr)
{
	constant_classref *cr;
	s4                 disp;
	classinfo         *c;

	/* get stuff from the stack */
	cr   = (constant_classref *) pr->ref;
	disp =                       pr->mpc;

	/* get the fieldinfo */
	if (!(c = resolve_classref_eager(cr)))
		return false;

	/* patch back original code */
	PATCH_BACK_ORIGINAL_MCODE;
	md_icacheflush(pr->mpc, 2);

	/* patch class flags */
	assert( (*((u2*)(disp)) == 0x263c) || (*((u2*)(disp)) == 0x283c) );
	*((s4 *) (disp + 2)) = (s4) c->flags;

	/* synchronize insn cache */
	md_icacheflush((void*)(disp + 2), SIZEOF_VOID_P);

	return true;
}

/* patcher_resolve_classref_to_vftbl *******************************************

   CHECKCAST (class):
0x4029b094:   61ff 0000 00b4    bsrl 0x4029b14a
0x4029b09a:   287c 0000 0000    moveal #0,%a4		<-- patch this #0
0x4029b0a0:   2668 0000         moveal %a0@(0),%a3

   INSTANCEOF (class):
0x402a9300:   61ff 0000 0574    bsrl 0x402a9876
0x402a9306:   267c 0000 0000    moveal #0,%a3
0x402a930c:   246a 0000         moveal %a2@(0),%a2


*******************************************************************************/

bool patcher_resolve_classref_to_vftbl(patchref_t *pr)
{
	constant_classref *cr;
	s4                 disp;
	classinfo         *c;

	/* get stuff from the stack */
	cr   = (constant_classref *) pr->ref;
	disp =                       pr->mpc;

	/* get the fieldinfo */
	if (!(c = resolve_classref_eager(cr)))
		return false;

	/* patch back original code */
	PATCH_BACK_ORIGINAL_MCODE;
	md_icacheflush(pr->mpc, 2);

	/* patch super class' vftbl */
	assert( (*((u2*)disp) == 0x287c) || (*((u2*)disp)== 0x267c) );

	*((s4 *) (disp+2)) = (s4) c->vftbl;

	/* synchronize insin cache */
	md_icacheflush((void*)(disp+2), SIZEOF_VOID_P);

	return true;
}

/* patcher_instanceof_interface ************************************************

   Machine code:

0x402a92da:   61ff 0000 05c0    bsrl 0x402a989c
0x402a92e0:   246a 0000         moveal %a2@(0),%a2
0x402a92e4:   282a 0010         movel %a2@(16),%d4
0x402a92e8:   d8bc 0000 0000    addl #0,%d4		<-- this const
0x402a92ee:   4a84              tstl %d4
0x402a92f0:   6e0a              bles 0x402a92fc
0x402a92f2:   246a 0000         moveal %a2@(0),%a2	<-- this offset

*******************************************************************************/

bool patcher_instanceof_interface(patchref_t *pr)
{
	u1                *ra;
	constant_classref *cr;
	classinfo         *c;
	s4                 disp;

	/* get stuff from the stack */

	ra = (u1 *)                pr->mpc;
	cr = (constant_classref *) pr->ref;

	/* get the fieldinfo */
	if (!(c = resolve_classref_eager(cr)))
		return false;

	/* patch back original code */
	PATCH_BACK_ORIGINAL_MCODE;
	md_icacheflush(pr->mpc, 2);

	/* patch super class index */
	disp = -(c->index);
	assert( *((u2*)(ra + 8)) == 0xd8bc );
	*((s4 *) (ra + 10 )) = disp;

	disp = OFFSET(vftbl_t, interfacetable[0]) - c->index * sizeof(methodptr*);

	assert( (s2)disp  == disp);
	assert ( *((s2*)(ra+18)) == 0x246a );

	*((s2 *) (ra + 20)) = disp;

	/* synchronize instruction cache */
	md_icacheflush((void*)(ra + 10), 12);

	return true;
}

/* patcher_checkcast_interface *************************************************

0x402a9400:   61ff 0000 03b6    bsrl 0x402a97b8
0x402a9406:   266a 0000         moveal %a2@(0),%a3
0x402a940a:   282b 0010         movel %a3@(16),%d4
0x402a940e:   d8bc 0000 0000    addl #0,%d4		<-- this 0
0x402a9414:   4a84              tstl %d4
0x402a9416:   6e02              bgts 0x402a941a
	      1234		tstb %d0
0x402a9418:   4afc              illegal
0x402a941a:   286b 0000         moveal %a3@(0),%a4	<-- and this 0 offset

*******************************************************************************/

bool patcher_checkcast_interface(patchref_t *pr)
{
	u1                *ra;
	constant_classref *cr;
	classinfo         *c;
	s4                 disp;

	/* get stuff from the stack */
	ra = (u1 *)                pr->mpc;
	cr = (constant_classref *) pr->ref;

	/* get the fieldinfo */
	if (!(c = resolve_classref_eager(cr)))
		return false;

	/* patch back original code */
	PATCH_BACK_ORIGINAL_MCODE;
	md_icacheflush(pr->mpc, 2);

	/* patch super class index */
	disp = -(c->index);
	assert ( *((u2 *)(ra + 8)) == 0xd8bc );
	*((s4 *) (ra + 10)) = disp;

	disp = OFFSET(vftbl_t, interfacetable[0]) - c->index * sizeof(methodptr*);
	assert( *((u2 *)(ra + 22)) == 0x286b );
	assert( (s2)disp == disp);
	*((s2 *) (ra + 24)) = disp;
	
	/* synchronize instruction cache */
	md_icacheflush((void*)(ra + 10), 16);

	return true;
}

#if 0
/* patcher_resolve_native_function *********************************************

   XXX

*******************************************************************************/

bool patcher_resolve_native_function(patchref_t *pr)
{
	methodinfo  *m;
	s4           disp;
	functionptr  f;

	/* get stuff from the stack */
	m    = (methodinfo *) pr->ref;
	disp =                pr->mpc;

	/* resolve native function */
	if (!(f = native_resolve_function(m)))
		return false;

	/* patch back original code */
	PATCH_BACK_ORIGINAL_MCODE;
	md_icacheflush(pr->mpc, 2);

	/* patch native function pointer */
	*((ptrint *) (disp + 2)) = (ptrint) f;

	/* synchronize data cache */
	md_icacheflush((void*)(disp + 2), SIZEOF_VOID_P);

	return true;
}
#endif

/* patcher_invokeinterface *****************************************************

   Machine code:
0x40adb03e:     moveal %a2@(0),%a3		0x266a0000		<-- no patching
0x40adb042:     moveal %a3@(0),%a3		0x266b0000		<-- patch this 0000
0x40adb046:     moveal %a3@(0),%a4		0xxxxx0000		<-- patch this 0000
0x40adb04a:     jsr %a4@				0xxxxx			


*******************************************************************************/

bool patcher_invokeinterface(patchref_t *pr)
{
	u1                *ra;
	unresolved_method *um;
	methodinfo        *m;
	s4                 disp;

	/* get stuff from the stack */
	ra = (u1 *)                pr->mpc;
	um = (unresolved_method *) pr->ref;


	/* get the fieldinfo */
	if (!(m = resolve_method_eager(um)))
		return false;

	/* patch back original code */
	PATCH_BACK_ORIGINAL_MCODE;
	md_icacheflush(pr->mpc, 2);

	/* if we show NOPs, we have to skip them */
	assert( *((uint32_t*)ra) == 0x246f0000 );

	/* patch interfacetable index (first #0) */
	disp = OFFSET(vftbl_t, interfacetable[0]) - sizeof(methodptr*) * m->clazz->index;
	/* XXX this disp is negative, check! 
	 * assert( (disp & 0x0000ffff) == disp);*/
	*((uint16_t *) (ra + 5 * 2)) = disp;

	/* patch method offset (second #0) */
	disp = sizeof(methodptr) * (m - m->clazz->methods);
	assert( (disp & 0x0000ffff) == disp);
	*((uint16_t *) (ra + 7 * 2)) = disp;

	/* synchronize instruction cache */
	md_icacheflush((void*)(ra + 5 * 2), 2 * 2);

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
