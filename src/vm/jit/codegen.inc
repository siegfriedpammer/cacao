/* jit/codegen.inc - architecture independent code generator

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   Institut f. Computersprachen, TU Wien
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser, M. Probst,
   S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich,
   J. Wenninger

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

   Changes: Michael Gschwind
            Christian Thalinger

   All functions assume the following code area / data area layout:

   +-----------+
   |           |
   | code area | code area grows to higher addresses
   |           |
   +-----------+ <-- start of procedure
   |           |
   | data area | data area grows to lower addresses
   |           |
   +-----------+

   The functions first write into a temporary code/data area allocated by
   "codegen_init". "codegen_finish" copies the code and data area into permanent
   memory. All functions writing values into the data area return the offset
   relative the begin of the code area (start of procedure).	

   $Id: codegen.inc 592 2003-11-09 19:46:15Z twisti $

*/


#include <string.h>
#include "toolbox/memory.h"
#include "toolbox/loging.h"


#define MCODEINITSIZE (1<<15)       /* 32 Kbyte code area initialization size */
#define DSEGINITSIZE  (1<<12)       /*  4 Kbyte data area initialization size */

static u1* mcodebase = NULL;        /* base pointer of code area              */
static s4* mcodeend  = NULL;        /* pointer to end of code area            */
static int mcodesize;               /* complete size of code area (bytes)     */

static u1* dsegtop = NULL;          /* pointer to top (end) of data area      */
static int dsegsize;                /* complete size of data area (bytes)     */
int dseglen;                        /* used size of data area (bytes)         */
                                    /* data area grows from top to bottom     */

static jumpref *jumpreferences;     /* list of jumptable target addresses     */
static dataref *datareferences;     /* list of data segment references        */
static branchref *xboundrefs;       /* list of bound check branches           */
static branchref *xcheckarefs;      /* list of array size check branches      */
static branchref *xnullrefs;        /* list of null check branches            */
static branchref *xcastrefs;        /* list of cast check branches            */
static branchref *xdivrefs;         /* list of divide by zero branches        */

int parentargs_base; /* offset in stackframe for the parameter from the caller*/

void codegen_init();                /* allocates code and data area           */
void codegen_close();               /* releases temporary storage             */
static void codegen_finish();       /* makes code and data area permanent and */
                                    /* updates branch references to code/data */

static s4 dseg_adds4(s4 value);         /* adds an int to data area           */
static s4 dseg_adds8(s8 value);         /* adds an long to data area          */
static s4 dseg_addfloat (float value);  /* adds an float to data area         */
static s4 dseg_adddouble(double value); /* adds an double to data area        */

#if POINTERSIZE == 8
#define dseg_addaddress(value)      dseg_adds8((s8)(value))
#else
#define dseg_addaddress(value)      dseg_adds4((s4)(value))
#endif

static void dseg_addtarget(basicblock *target);
static void dseg_adddata(u1 *ptr);
static void codegen_addreference(basicblock *target, void *branchptr);
static void codegen_addxboundrefs(void *branchptr);
static void codegen_addxnullrefs(void *branchptr);
static void codegen_addxcastrefs(void *branchptr);
static void codegen_addxdivrefs(void *branchptr);

void dseg_display(s4 *s4ptr);



/* codegen_init allocates and initialises code area, data area and references   */

void codegen_init()
{
	if (!mcodebase) {
		mcodebase = MNEW(u1, MCODEINITSIZE);
		mcodesize = MCODEINITSIZE;
	}

	if (!dsegtop) {
		dsegtop = MNEW(u1, DSEGINITSIZE);
		dsegsize = DSEGINITSIZE;
		dsegtop += dsegsize;
	}

	dseglen = 0;

	jumpreferences = NULL;
	datareferences = NULL;
	xboundrefs = NULL;
	xnullrefs = NULL;
	xcastrefs = NULL;
	xdivrefs = NULL;
}



/* codegen_close releases temporary code and data area                          */

void codegen_close()
{
	if (mcodebase) {
		MFREE(mcodebase, u1, mcodesize);
		mcodebase = NULL;
	}

	if (dsegtop) {
		MFREE(dsegtop - dsegsize, u1, dsegsize);
		dsegtop = NULL;
	}
}



/* codegen_increase doubles code area                                           */

static s4 *codegen_increase(u1 *codeptr)
{
	long len;

	len = codeptr - mcodebase;
	mcodebase = MREALLOC(mcodebase, u1, mcodesize, mcodesize * 2);
	mcodesize *= 2;
	mcodeend = (s4*) (mcodebase + mcodesize);
	return (s4*) (mcodebase + len);
}



/* desg_increase doubles data area                                            */

static void dseg_increase()
{
	u1 *newstorage = MNEW(u1, dsegsize * 2);
	memcpy(newstorage + dsegsize, dsegtop - dsegsize, dsegsize);
	MFREE(dsegtop - dsegsize, u1, dsegsize);
	dsegtop = newstorage;
	dsegsize *= 2;
	dsegtop += dsegsize;
}



static s4 dseg_adds4_increase(s4 value)
{
	dseg_increase();
	*((s4 *) (dsegtop - dseglen)) = value;
	return -dseglen;
}



static s4 dseg_adds4(s4 value)
{
	s4 *dataptr;

	dseglen += 4;
	dataptr = (s4 *) (dsegtop - dseglen);
	if (dseglen > dsegsize)
		return dseg_adds4_increase(value);
	*dataptr = value;
	return -dseglen;
}



static s4 dseg_adds8_increase(s8 value)
{
	dseg_increase();
	*((s8 *) (dsegtop - dseglen)) = value;
	return -dseglen;
}



static s4 dseg_adds8(s8 value)
{
	s8 *dataptr;

	dseglen = ALIGN (dseglen + 8, 8);
	dataptr = (s8 *) (dsegtop - dseglen);
	if (dseglen > dsegsize)
		return dseg_adds8_increase(value);
	*dataptr = value;
	return -dseglen;
}



static s4 dseg_addfloat_increase(float value)
{
	dseg_increase();
	*((float *) (dsegtop - dseglen)) = value;
	return -dseglen;
}



static s4 dseg_addfloat(float value)
{
	float *dataptr;

	dseglen += 4;
	dataptr = (float *) (dsegtop - dseglen);
	if (dseglen > dsegsize)
		return dseg_addfloat_increase(value);
	*dataptr = value;
	return -dseglen;
}



static s4 dseg_adddouble_increase(double value)
{
	dseg_increase();
	*((double *) (dsegtop - dseglen)) = value;
	return -dseglen;
}



static s4 dseg_adddouble(double value)
{
	double *dataptr;

	dseglen = ALIGN (dseglen + 8, 8);
	dataptr = (double *) (dsegtop - dseglen);
	if (dseglen > dsegsize)
		return dseg_adddouble_increase(value);
	*dataptr = value;
	return -dseglen;
}



static void dseg_addtarget(basicblock *target)
{
	jumpref *jr = DNEW(jumpref);

	jr->tablepos = dseg_addaddress(NULL);
	jr->target = target;
	jr->next = jumpreferences;
	jumpreferences = jr;
}



static void dseg_adddata(u1 *ptr)
{
	dataref *dr = DNEW(dataref);

	dr->pos = (u1 *) (ptr - mcodebase);
	dr->next = datareferences;
	datareferences = dr;
}



static void codegen_addreference(basicblock *target, void *branchptr)
{
	s4 branchpos = (u1*) branchptr - mcodebase;

	if (target->mpc >= 0) {
		gen_resolvebranch((u1*) mcodebase + branchpos, branchpos, target->mpc);
	}
	else {
		branchref *br = DNEW(branchref);

		br->branchpos = branchpos;
		br->next = target->branchrefs;
		target->branchrefs= br;
	}
}



static void codegen_addxboundrefs(void *branchptr)
{
	s4 branchpos = (u1*) branchptr - mcodebase;

	branchref *br = DNEW(branchref);

	br->branchpos = branchpos;
	br->next = xboundrefs;
	xboundrefs = br;
}



static void codegen_addxcheckarefs(void *branchptr)
{
	s4 branchpos = (u1*) branchptr - mcodebase;

	branchref *br = DNEW(branchref);

	br->branchpos = branchpos;
	br->next = xcheckarefs;
	xcheckarefs = br;
}



static void codegen_addxnullrefs(void *branchptr)
{
	s4 branchpos = (u1*) branchptr - mcodebase;

	branchref *br = DNEW(branchref);

	br->branchpos = branchpos;
	br->next = xnullrefs;
	xnullrefs = br;
}



static void codegen_addxcastrefs(void *branchptr)
{
	s4 branchpos = (u1*) branchptr - mcodebase;

	branchref *br = DNEW(branchref);

	br->branchpos = branchpos;
	br->next = xcastrefs;
	xcastrefs = br;
}



static void codegen_addxdivrefs(void *branchptr)
{
	s4 branchpos = (u1*) branchptr - mcodebase;

	branchref *br = DNEW(branchref);

	br->branchpos = branchpos;
	br->next = xdivrefs;
	xdivrefs = br;
}



static void codegen_finish(int mcodelen)
{
	jumpref *jr;
	u1 *epoint;

	count_code_len += mcodelen;
	count_data_len += dseglen;

	dseglen = ALIGN(dseglen, MAX_ALIGN);

	method -> mcodelength = mcodelen + dseglen;
	method -> mcode = CNEW(u1, mcodelen + dseglen);

	memcpy ( method->mcode, dsegtop - dseglen, dseglen);
	memcpy ( method->mcode + dseglen, mcodebase, mcodelen);

	method -> entrypoint = epoint = (u1*) (method->mcode + dseglen);

	/* jump table resolving */
	jr = jumpreferences;
	while (jr != NULL) {
	    *((void**) (epoint + jr->tablepos)) = epoint + jr->target->mpc;
	    jr = jr->next;
	}

#if defined(__I386__) || defined(__X86_64__)
	{
		dataref *dr;
		/* add method into datastructure to find the entrypoint */
		addmethod(method->entrypoint, method->entrypoint + mcodelen);
        
		/* data segment references resolving */
		dr = datareferences;
		while (dr != NULL) {
			*((void**) ((long) epoint + (long) dr->pos - POINTERSIZE)) = epoint;
			dr = dr->next;
		}
	}
#endif
}



void dseg_display(s4 *s4ptr)
{
	int i;
	
	printf("  --- dump of datasegment\n");
	for (i = dseglen; i > 0 ; i -= 4) {
		printf("-%6x: %8x\n", i, (int)(*s4ptr++));
	}
	printf("  --- begin of data segment: %p\n", s4ptr);
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
