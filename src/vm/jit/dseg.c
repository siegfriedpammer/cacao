/* src/vm/jit/dseg.c - data segment handling stuff

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

   Authors: Reinhard Grafl
            Andreas  Krall

   Changes: Christian Thalinger
            Joseph Wenninger

   $Id: dseg.c 4011 2005-12-30 14:16:49Z twisti $

*/


#include "config.h"

#include "vm/types.h"

#include "mm/memory.h"
#include "vm/jit/codegen-common.h"


/* desg_increase ***************************************************************

   Doubles data area.

*******************************************************************************/

void dseg_increase(codegendata *cd)
{
	u1 *newstorage;

	newstorage = DMNEW(u1, cd->dsegsize * 2);

	MCOPY(newstorage + cd->dsegsize, cd->dsegtop - cd->dsegsize, u1,
		  cd->dsegsize);

	cd->dsegtop   = newstorage;
	cd->dsegsize *= 2;
	cd->dsegtop  += cd->dsegsize;
}


s4 dseg_adds4_increase(codegendata *cd, s4 value)
{
	dseg_increase(cd);

	*((s4 *) (cd->dsegtop - cd->dseglen)) = value;

	return -(cd->dseglen);
}


s4 dseg_adds4(codegendata *cd, s4 value)
{
	s4 *dataptr;

	cd->dseglen += 4;
	dataptr = (s4 *) (cd->dsegtop - cd->dseglen);

	if (cd->dseglen > cd->dsegsize)
		return dseg_adds4_increase(cd, value);

	*dataptr = value;

	return -(cd->dseglen);
}


s4 dseg_adds8_increase(codegendata *cd, s8 value)
{
	dseg_increase(cd);

	*((s8 *) (cd->dsegtop - cd->dseglen)) = value;

	return -(cd->dseglen);
}


s4 dseg_adds8(codegendata *cd, s8 value)
{
	s8 *dataptr;

	cd->dseglen = ALIGN(cd->dseglen + 8, 8);
	dataptr = (s8 *) (cd->dsegtop - cd->dseglen);

	if (cd->dseglen > cd->dsegsize)
		return dseg_adds8_increase(cd, value);

	*dataptr = value;

	return -(cd->dseglen);
}


s4 dseg_addfloat_increase(codegendata *cd, float value)
{
	dseg_increase(cd);

	*((float *) (cd->dsegtop - cd->dseglen)) = value;

	return -(cd->dseglen);
}


s4 dseg_addfloat(codegendata *cd, float value)
{
	float *dataptr;

	cd->dseglen += 4;
	dataptr = (float *) (cd->dsegtop - cd->dseglen);

	if (cd->dseglen > cd->dsegsize)
		return dseg_addfloat_increase(cd, value);

	*dataptr = value;

	return -(cd->dseglen);
}


s4 dseg_adddouble_increase(codegendata *cd, double value)
{
	dseg_increase(cd);

	*((double *) (cd->dsegtop - cd->dseglen)) = value;

	return -(cd->dseglen);
}


s4 dseg_adddouble(codegendata *cd, double value)
{
	double *dataptr;

	cd->dseglen = ALIGN(cd->dseglen + 8, 8);
	dataptr = (double *) (cd->dsegtop - cd->dseglen);

	if (cd->dseglen > cd->dsegsize)
		return dseg_adddouble_increase(cd, value);

	*dataptr = value;

	return -(cd->dseglen);
}


void dseg_addtarget(codegendata *cd, basicblock *target)
{
	jumpref *jr;

	jr = DNEW(jumpref);

	jr->tablepos = dseg_addaddress(cd, NULL);
	jr->target   = target;
	jr->next     = cd->jumpreferences;

	cd->jumpreferences = jr;
}


void dseg_adddata(codegendata *cd, u1 *mcodeptr)
{
	dataref *dr;

	dr = DNEW(dataref);

	dr->datapos = mcodeptr - cd->mcodebase;
	dr->next    = cd->datareferences;

	cd->datareferences = dr;
}


/* dseg_addlinenumbertablesize *************************************************

   XXX

*******************************************************************************/

void dseg_addlinenumbertablesize(codegendata *cd)
{
#if SIZEOF_VOID_P == 8
	/* 4-byte ALIGNMENT PADDING */

	dseg_adds4(cd, 0);
#endif

	cd->linenumbertablesizepos  = dseg_addaddress(cd, NULL);
	cd->linenumbertablestartpos = dseg_addaddress(cd, NULL);

#if SIZEOF_VOID_P == 8
	/* 4-byte ALIGNMENT PADDING */

	dseg_adds4(cd, 0);
#endif
}


void dseg_addlinenumber(codegendata *cd, u2 linenumber, u1 *mcodeptr)
{
	linenumberref *lr;

	lr = DNEW(linenumberref);

	lr->linenumber = linenumber;
	lr->tablepos   = 0;
	lr->targetmpc  = mcodeptr - cd->mcodebase;
	lr->next       = cd->linenumberreferences;

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
		lr->tablepos = dseg_addaddress(cd, NULL);

		if (cd->linenumbertab == 0)
			cd->linenumbertab = lr->tablepos;

		dseg_addaddress(cd, lr->linenumber);
	}
}


/* dseg_display ****************************************************************

   Displays the content of the methods' data segment.

*******************************************************************************/

#if !defined(NDEBUG)
void dseg_display(methodinfo *m, codegendata *cd)
{
	s4 *s4ptr;
	s4 i;
	
	s4ptr = (s4 *) (ptrint) m->mcode;

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
 */
