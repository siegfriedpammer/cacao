/* src/vm/jit/dseg.c - data segment handling stuff

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
            Andreas  Krall

   Changes: Christian Thalinger
            Joseph Wenninger
			Edwin Steiner

   $Id: dseg.c 5186 2006-07-28 13:24:43Z twisti $

*/


#include "config.h"

#include <assert.h>

#include "vm/types.h"

#include "mm/memory.h"
#include "vm/jit/codegen-common.h"


/* dseg_finish *****************************************************************

   Fills the data segment with the values stored.

*******************************************************************************/

void dseg_finish(jitdata *jd)
{
	codeinfo    *code;
	codegendata *cd;
	dsegentry   *de;

	/* get required compiler data */

	code = jd->code;
	cd   = jd->cd;

	/* process all data segment entries */

	for (de = cd->dseg; de != NULL; de = de->next) {
		switch (de->type) {
		case TYPE_INT:
			*((s4 *)     (code->entrypoint + de->disp)) = de->val.i;
			break;

		case TYPE_LNG:
			*((s8 *)     (code->entrypoint + de->disp)) = de->val.l;
			break;

		case TYPE_FLT:
			*((float *)  (code->entrypoint + de->disp)) = de->val.f;
			break;

		case TYPE_DBL:
			*((double *) (code->entrypoint + de->disp)) = de->val.d;
			break;

		case TYPE_ADR:
			*((void **)  (code->entrypoint + de->disp)) = de->val.a;
			break;
		}
	}
}


static s4 dseg_find_s4(codegendata *cd, s4 value)
{
	dsegentry *de;

	/* search all data segment entries for a matching entry */

	for (de = cd->dseg; de != NULL; de = de->next) {
		if (IS_INT_TYPE(de->type))
			if (de->flags & DSEG_FLAG_READONLY)
				if (de->val.i == value)
					return de->disp;
	}

	/* no matching entry was found */

	return 0;
}


static s4 dseg_find_s8(codegendata *cd, s8 value)
{
	dsegentry *de;

	/* search all data segment entries for a matching entry */

	for (de = cd->dseg; de != NULL; de = de->next) {
		if (IS_LNG_TYPE(de->type))
			if (de->flags & DSEG_FLAG_READONLY)
				if (de->val.l == value)
					return de->disp;
	}

	/* no matching entry was found */

	return 0;
}


static s4 dseg_find_float(codegendata *cd, float value)
{
	dsegentry *de;

	/* search all data segment entries for a matching entry */

	for (de = cd->dseg; de != NULL; de = de->next) {
		if (IS_FLT_TYPE(de->type))
			if (de->flags & DSEG_FLAG_READONLY)
				if (de->val.f == value)
					return de->disp;
	}

	/* no matching entry was found */

	return 0;
}


static s4 dseg_find_double(codegendata *cd, double value)
{
	dsegentry *de;

	/* search all data segment entries for a matching entry */

	for (de = cd->dseg; de != NULL; de = de->next) {
		if (IS_DBL_TYPE(de->type))
			if (de->flags & DSEG_FLAG_READONLY)
				if (de->val.d == value)
					return de->disp;
	}

	/* no matching entry was found */

	return 0;
}


static s4 dseg_find_address(codegendata *cd, void *value)
{
	dsegentry *de;

	/* search all data segment entries for a matching entry */

	for (de = cd->dseg; de != NULL; de = de->next) {
		if (IS_ADR_TYPE(de->type))
			if (de->flags & DSEG_FLAG_READONLY)
				if (de->val.a == value)
					return de->disp;
	}

	/* no matching entry was found */

	return 0;
}


/* dseg_add_s4_intern **********************************************************

   Internal function to add an s4 value to the data segment.

*******************************************************************************/

static s4 dseg_add_s4_intern(codegendata *cd, s4 value, u4 flags)
{
	dsegentry *de;

	/* Increase data segment size, which is also the displacement into
	   the data segment. */

	cd->dseglen += 4;

	/* allocate new entry */

	de = DNEW(dsegentry);

	de->type  = TYPE_INT;
	de->flags = flags;
	de->disp  = -(cd->dseglen);
	de->val.i = value;
	de->next  = cd->dseg;

	/* insert into the chain */

	cd->dseg = de;

	return de->disp;
}


/* dseg_add_unique_s4 **********************************************************

   Adds uniquely an s4 value to the data segment.

*******************************************************************************/

s4 dseg_add_unique_s4(codegendata *cd, s4 value)
{
	s4 disp;

	disp = dseg_add_s4_intern(cd, value, DSEG_FLAG_UNIQUE);

	return disp;
}


/* dseg_add_s4 *****************************************************************

   Adds an s4 value to the data segment. It tries to reuse previously
   added values.

*******************************************************************************/

s4 dseg_add_s4(codegendata *cd, s4 value)
{
	s4 disp;

	/* search the data segment if the value is already stored */

	disp = dseg_find_s4(cd, value);

	if (disp != 0)
		return disp;
		
	disp = dseg_add_s4_intern(cd, value, DSEG_FLAG_READONLY);

	return disp;
}


/* dseg_add_s8_intern **********************************************************

   Internal function to add an s8 value to the data segment.

*******************************************************************************/

static s4 dseg_add_s8_intern(codegendata *cd, s8 value, u4 flags)
{
	dsegentry *de;

	/* Increase data segment size, which is also the displacement into
	   the data segment. */

	cd->dseglen = ALIGN(cd->dseglen + 8, 8);

	/* allocate new entry */

	de = DNEW(dsegentry);

	de->type  = TYPE_LNG;
	de->flags = flags;
	de->disp  = -(cd->dseglen);
	de->val.l = value;
	de->next  = cd->dseg;

	/* insert into the chain */

	cd->dseg = de;

	return de->disp;
}


/* dseg_add_unique_s8 **********************************************************

   Adds uniquely an s8 value to the data segment.

*******************************************************************************/

s4 dseg_add_unique_s8(codegendata *cd, s8 value)
{
	s4 disp;

	disp = dseg_add_s8_intern(cd, value, DSEG_FLAG_UNIQUE);

	return disp;
}


/* dseg_add_s8 *****************************************************************

   Adds an s8 value to the data segment. It tries to reuse previously
   added values.

*******************************************************************************/

s4 dseg_add_s8(codegendata *cd, s8 value)
{
	s4 disp;

	/* search the data segment if the value is already stored */

	disp = dseg_find_s8(cd, value);

	if (disp != 0)
		return disp;
		
	disp = dseg_add_s8_intern(cd, value, DSEG_FLAG_READONLY);

	return disp;
}


/* dseg_add_float_intern *******************************************************

   Internal function to add a float value to the data segment.

*******************************************************************************/

static s4 dseg_add_float_intern(codegendata *cd, float value, u4 flags)
{
	dsegentry *de;
		
	/* Increase data segment size, which is also the displacement into
	   the data segment. */

	cd->dseglen += 4;

	/* allocate new entry */

	de = DNEW(dsegentry);

	de->type  = TYPE_FLT;
	de->flags = flags;
	de->disp  = -(cd->dseglen);
	de->val.f = value;
	de->next  = cd->dseg;

	/* insert into the chain */

	cd->dseg = de;

	return de->disp;
}


/* dseg_add_unique_float *******************************************************

   Adds uniquely an float value to the data segment.

*******************************************************************************/

s4 dseg_add_unique_float(codegendata *cd, float value)
{
	s4 disp;

	disp = dseg_add_float_intern(cd, value, DSEG_FLAG_UNIQUE);

	return disp;
}


/* dseg_add_float **************************************************************

   Adds an float value to the data segment. It tries to reuse
   previously added values.

*******************************************************************************/

s4 dseg_add_float(codegendata *cd, float value)
{
	s4 disp;

	/* search the data segment if the value is already stored */

	disp = dseg_find_float(cd, value);

	if (disp != 0)
		return disp;
		
	disp = dseg_add_float_intern(cd, value, DSEG_FLAG_READONLY);

	return disp;
}


/* dseg_add_double_intern ******************************************************

   Internal function to add a double value to the data segment.

*******************************************************************************/

static s4 dseg_add_double_intern(codegendata *cd, double value, u4 flags)
{
	dsegentry *de;
		
	/* Increase data segment size, which is also the displacement into
	   the data segment. */

	cd->dseglen = ALIGN(cd->dseglen + 8, 8);

	/* allocate new entry */

	de = DNEW(dsegentry);

	de->type  = TYPE_DBL;
	de->flags = flags;
	de->disp  = -(cd->dseglen);
	de->val.d = value;
	de->next  = cd->dseg;

	/* insert into the chain */

	cd->dseg = de;

	return de->disp;
}


/* dseg_add_unique_double ******************************************************

   Adds uniquely a double value to the data segment.

*******************************************************************************/

s4 dseg_add_unique_double(codegendata *cd, double value)
{
	s4 disp;

	disp = dseg_add_double_intern(cd, value, DSEG_FLAG_UNIQUE);

	return disp;
}


/* dseg_add_double *************************************************************

   Adds a double value to the data segment. It tries to reuse
   previously added values.

*******************************************************************************/

s4 dseg_add_double(codegendata *cd, double value)
{
	s4 disp;

	/* search the data segment if the value is already stored */

	disp = dseg_find_double(cd, value);

	if (disp != 0)
		return disp;
		
	disp = dseg_add_double_intern(cd, value, DSEG_FLAG_READONLY);

	return disp;
}


/* dseg_add_address_intern *****************************************************

   Internal function to add an address pointer to the data segment.

*******************************************************************************/

static s4 dseg_add_address_intern(codegendata *cd, void *value, u4 flags)
{
	dsegentry *de;

	/* Increase data segment size, which is also the displacement into
	   the data segment. */

#if SIZEOF_VOID_P == 8
	cd->dseglen = ALIGN(cd->dseglen + 8, 8);
#else
	cd->dseglen += 4;
#endif

	/* allocate new entry */

	de = DNEW(dsegentry);

	de->type  = TYPE_ADR;
	de->flags = flags;
	de->disp  = -(cd->dseglen);
	de->val.a = value;
	de->next  = cd->dseg;

	/* insert into the chain */

	cd->dseg = de;

	return de->disp;
}


/* dseg_add_unique_address *****************************************************

   Adds uniquely an address value to the data segment.

*******************************************************************************/

s4 dseg_add_unique_address(codegendata *cd, void *value)
{
	s4 disp;

	disp = dseg_add_address_intern(cd, value, DSEG_FLAG_UNIQUE);

	return disp;
}


/* dseg_add_address ************************************************************

   Adds an address value to the data segment. It tries to reuse
   previously added values.

*******************************************************************************/

s4 dseg_add_address(codegendata *cd, void *value)
{
	s4 disp;

	/* search the data segment if the value is already stored */

	disp = dseg_find_address(cd, value);

	if (disp != 0)
		return disp;
		
	disp = dseg_add_address_intern(cd, value, DSEG_FLAG_READONLY);

	return disp;
}


/* dseg_add_target *************************************************************

   XXX

*******************************************************************************/

void dseg_add_target(codegendata *cd, basicblock *target)
{
	jumpref *jr;

	jr = DNEW(jumpref);

	jr->tablepos = dseg_add_unique_address(cd, NULL);
	jr->target   = target;
	jr->next     = cd->jumpreferences;

	cd->jumpreferences = jr;
}


/* dseg_addlinenumbertablesize *************************************************

   XXX

*******************************************************************************/

void dseg_addlinenumbertablesize(codegendata *cd)
{
#if SIZEOF_VOID_P == 8
	/* 4-byte ALIGNMENT PADDING */

	dseg_add_unique_s4(cd, 0);
#endif

	cd->linenumbertablesizepos  = dseg_add_unique_address(cd, NULL);
	cd->linenumbertablestartpos = dseg_add_unique_address(cd, NULL);

#if SIZEOF_VOID_P == 8
	/* 4-byte ALIGNMENT PADDING */

	dseg_add_unique_s4(cd, 0);
#endif
}


/* dseg_addlinenumber **********************************************************

   Add a line number reference.

   IN:
      cd.............current codegen data
      linenumber.....number of line that starts with the given mcodeptr
      mcodeptr.......start mcodeptr of line

*******************************************************************************/

void dseg_addlinenumber(codegendata *cd, u2 linenumber)
{
	linenumberref *lr;

	lr = DNEW(linenumberref);

	lr->linenumber = linenumber;
	lr->tablepos   = 0;
	lr->targetmpc  = cd->mcodeptr - cd->mcodebase;
	lr->next       = cd->linenumberreferences;

	cd->linenumberreferences = lr;
}


/* dseg_addlinenumber_inline_start *********************************************

   Add a marker to the line number table indicating the start of an inlined
   method body. (see doc/inlining_stacktrace.txt)

   IN:
      cd.............current codegen data
      iptr...........the ICMD_INLINE_START instruction
      mcodeptr.......start mcodeptr of inlined body

*******************************************************************************/

void dseg_addlinenumber_inline_start(codegendata *cd, instruction *iptr)
{
	linenumberref *lr;
	insinfo_inline *insinfo;
	ptrint mpc;

	lr = DNEW(linenumberref);

	lr->linenumber = (-2); /* marks start of inlined method */
	lr->tablepos   = 0;
	lr->targetmpc  = (mpc = (u1 *) cd->mcodeptr - cd->mcodebase);
	lr->next       = cd->linenumberreferences;

	cd->linenumberreferences = lr;

	insinfo = (insinfo_inline *) iptr->target;

	insinfo->startmpc = mpc; /* store for corresponding INLINE_END */
}


/* dseg_addlinenumber_inline_end ***********************************************

   Add a marker to the line number table indicating the end of an inlined
   method body. (see doc/inlining_stacktrace.txt)

   IN:
      cd.............current codegen data
      iptr...........the ICMD_INLINE_END instruction

   Note:
      iptr->method must point to the inlined callee.

*******************************************************************************/

void dseg_addlinenumber_inline_end(codegendata *cd, instruction *iptr)
{
	linenumberref *lr;
	linenumberref *prev;
	insinfo_inline *insinfo;

	insinfo = (insinfo_inline *) iptr->target;

	assert(insinfo);

	lr = DNEW(linenumberref);

	/* special entry containing the methodinfo * */
	lr->linenumber = (-3) - iptr->line;
	lr->tablepos   = 0;
	lr->targetmpc  = (ptrint) insinfo->method;
	lr->next       = cd->linenumberreferences;

	prev = lr;
	lr = DNEW(linenumberref);

	/* end marker with PC of start of body */
	lr->linenumber = (-1);
	lr->tablepos   = 0;
	lr->targetmpc  = insinfo->startmpc;
	lr->next       = prev;

	cd->linenumberreferences = lr;
}


/* dseg_createlinenumbertable **************************************************

   Creates a line number table in the data segment from the created
   entries in linenumberreferences.

*******************************************************************************/

void dseg_createlinenumbertable(codegendata *cd)
{
	linenumberref *lr;

	for (lr = cd->linenumberreferences; lr != NULL; lr = lr->next) {
		lr->tablepos = dseg_add_unique_address(cd, NULL);

		if (cd->linenumbertab == 0)
			cd->linenumbertab = lr->tablepos;

#if SIZEOF_VOID_P == 8
		/* This is for alignment and easier usage. */
		(void) dseg_add_unique_s8(cd, lr->linenumber);
#else
		(void) dseg_add_unique_s4(cd, lr->linenumber);
#endif
	}
}


/* dseg_adddata ****************************************************************

   Adds a data segment reference to the codegendata.

*******************************************************************************/

#if defined(__I386__) || defined(__X86_64__) || defined(__XDSPCORE__) || defined(ENABLE_INTRP)
void dseg_adddata(codegendata *cd)
{
	dataref *dr;

	dr = DNEW(dataref);

	dr->datapos = cd->mcodeptr - cd->mcodebase;
	dr->next    = cd->datareferences;

	cd->datareferences = dr;
}
#endif


/* dseg_resolve_datareferences *************************************************

   Resolve data segment references.

*******************************************************************************/

#if defined(__I386__) || defined(__X86_64__) || defined(__XDSPCORE__) || defined(ENABLE_INTRP)
void dseg_resolve_datareferences(jitdata *jd)
{
	codeinfo    *code;
	codegendata *cd;
	dataref     *dr;

	/* get required compiler data */

	code = jd->code;
	cd   = jd->cd;

	/* data segment references resolving */

	for (dr = cd->datareferences; dr != NULL; dr = dr->next)
		*((u1 **) (code->entrypoint + dr->datapos - SIZEOF_VOID_P)) = code->entrypoint;
}
#endif


/* dseg_display ****************************************************************

   Displays the content of the methods' data segment.

*******************************************************************************/

#if !defined(NDEBUG)
void dseg_display(jitdata *jd)
{
	codeinfo    *code;
	codegendata *cd;
	s4          *s4ptr;
	s4           i;
	
	/* get required compiler data */

	code = jd->code;
	cd   = jd->cd;

	s4ptr = (s4 *) (ptrint) code->mcode;

	printf("  --- dump of datasegment\n");

	for (i = cd->dseglen; i > 0 ; i -= 4) {
#if SIZEOF_VOID_P == 8
		printf("0x%016lx:   -%6x (%6d): %8x\n",
			   (ptrint) s4ptr, i, i, (s4) *s4ptr);
#else
		printf("0x%08x:   -%6x (%6d): %8x\n",
			   (ptrint) s4ptr, i, i, (s4) *s4ptr);
#endif
		s4ptr++;
	}

	printf("  --- begin of data segment: %p\n", (void *) s4ptr);
}
#endif /* !defined(NDEBUG) */


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
