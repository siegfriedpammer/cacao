/* jit/reg.inc - register allocator

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

   Authors: Andreas Krall

   Changes: Stefan Ring
            Christian Thalinger

   $Id: reg.inc 1274 2004-07-05 17:24:40Z twisti $

*/


#include "reg.h"
#include "toolbox/memory.h"


/* function prototypes for this file */

static void interface_regalloc(methodinfo *m);
static void local_regalloc(methodinfo *m);
static void allocate_scratch_registers(methodinfo *m);


/* function reg_init ***********************************************************

	initialises the register-allocator
	
*******************************************************************************/

void reg_init(methodinfo *m)
{
	s4 n;
	registerdata *r;

	if (m->registerdata)
		return;

	/* allocate registers structure */
	m->registerdata = NEW(registerdata);

	/* keep code size smaller */
	r = m->registerdata;

	/* setup the integer register table */
	r->intreg_argnum = 0;
	r->tmpintregcnt = 0;
	r->savintregcnt = 0;

	for (r->intregsnum = 0; nregdescint[r->intregsnum] != REG_END; r->intregsnum++) {
		switch (nregdescint[r->intregsnum]) {
		case REG_SAV: r->savintregcnt++;
			break;
		case REG_TMP: r->tmpintregcnt++;
			break;
		case REG_ARG: r->intreg_argnum++;
		}
	}

	r->argintregs = MNEW(s4, r->intreg_argnum);
	r->tmpintregs = MNEW(s4, r->tmpintregcnt);
	r->savintregs = MNEW(s4, r->savintregcnt);
	r->freeargintregs = MNEW(s4, r->intreg_argnum);
	r->freetmpintregs = MNEW(s4, r->tmpintregcnt);
	r->freesavintregs = MNEW(s4, r->savintregcnt);
#ifdef USETWOREGS
	r->secondregs = MNEW(s4, r->intregsnum);
#endif

	r->intreg_argnum = 0;
	r->argintreguse = 0;
	r->tmpintreguse = 0;
	r->savintreguse = 0;

	for (n = 0; n < r->intregsnum; n++) {
		switch (nregdescint[n]) {
		case REG_RET: r->intreg_ret = n; 
			break;
		case REG_SAV: r->savintregs[r->savintreguse++] = n;
			break;
		case REG_TMP: r->tmpintregs[r->tmpintreguse++] = n;
			break;
		case REG_ARG: r->argintregs[r->intreg_argnum++] = n;
			r->argintreguse++;
			break;
		}
	}

#if defined(__I386__)
	/* 
	   this assumes that we have 3 tmp regs (%ecx, %edx, %ebx) 
	   sort to [ %ebx, %edx, %ecx ]
	 */
	n = r->tmpintregs[0];
	r->tmpintregs[0] = r->tmpintregs[2];
	r->tmpintregs[2] = n;
#endif

#if defined(__X86_64__)
	/* 
	 * on x86_64 the argument registers are not in ascending order 
	 * a00 (%rdi) <-> a03 (%rcx) and a01 (%rsi) <-> a02 (%rdx)
	 */
	n = r->argintregs[3];
	r->argintregs[3] = r->argintregs[0];
	r->argintregs[0] = n;

	n = r->argintregs[2];
	r->argintregs[2] = r->argintregs[1];
	r->argintregs[1] = n;
#endif
		
#ifdef USETWOREGS
	for (n = 1; n < r->intreg_argnum; n++)
		r->secondregs[r->argintregs[n - 1]] = r->argintregs[n];
	for (n = 1; n < r->tmpintregcnt; n++)
		r->secondregs[r->tmpintregs[n - 1]] = r->tmpintregs[n];
	for (n = 1; n < r->savintregcnt; n++)
		r->secondregs[r->savintregs[n - 1]] = r->savintregs[n];

	r->secondregs[REG_ITMP1] = REG_ITMP2;
	r->secondregs[REG_ITMP3] = REG_ITMP2;
	r->secondregs[REG_RESULT] = REG_RESULT + 1;
	r->secondregs[r->argintregs[r->intreg_argnum - 1]] = REG_ITMP3;
#endif

	/* setup the float register table */
	r->fltreg_argnum = 0;
	r->tmpfltregcnt = 0;
	r->savfltregcnt = 0;

	for (r->floatregsnum = 0; nregdescfloat[r->floatregsnum] != REG_END; r->floatregsnum++) {
		switch (nregdescfloat[r->floatregsnum]) {
		case REG_SAV: r->savfltregcnt++;
			break;
		case REG_TMP: r->tmpfltregcnt++;
			break;
		case REG_ARG: r->fltreg_argnum++;
			break;
		}
	}

	r->argfltregs = MNEW(s4, r->fltreg_argnum);
	r->tmpfltregs = MNEW(s4, r->tmpfltregcnt);
	r->savfltregs = MNEW(s4, r->savfltregcnt);
	r->freeargfltregs = MNEW(s4, r->fltreg_argnum);
	r->freetmpfltregs = MNEW(s4, r->tmpfltregcnt);
	r->freesavfltregs = MNEW(s4, r->savfltregcnt);

	r->fltreg_argnum = 0;
	r->argfltreguse = 0;
	r->tmpfltreguse = 0;
	r->savfltreguse = 0;

	for (n = 0; n < r->floatregsnum; n++) {
		switch (nregdescfloat[n]) {
		case REG_RET:
			r->floatreg_ret = n; 
			break;
		case REG_SAV:
			r->savfltregs[r->savfltreguse++] = n;
			break;
		case REG_TMP:
			r->tmpfltregs[r->tmpfltreguse++] = n;
			break;
		case REG_ARG:
			r->argfltregs[r->fltreg_argnum++] = n;
			r->argfltreguse++;
			break;
		}
	}
}


void reg_setup(methodinfo *m)
{
	s4 i;
	varinfo5 *v;

	m->registerdata->freemem    = MNEW(s4, m->maxstack);
	m->registerdata->locals     = MNEW(varinfo5, m->maxlocals);
	m->registerdata->interfaces = MNEW(varinfo5, m->maxstack);

	for (v = m->registerdata->locals, i = m->maxlocals; i > 0; v++, i--) {
		v[0][TYPE_INT].type = -1;
		v[0][TYPE_LNG].type = -1;
		v[0][TYPE_FLT].type = -1;
		v[0][TYPE_DBL].type = -1;
		v[0][TYPE_ADR].type = -1;
	}

	for (v = m->registerdata->interfaces, i = m->maxstack; i > 0; v++, i--) {
		v[0][TYPE_INT].type = -1;
		v[0][TYPE_INT].flags = 0;
		v[0][TYPE_LNG].type = -1;
		v[0][TYPE_LNG].flags = 0;
		v[0][TYPE_FLT].type = -1;
		v[0][TYPE_FLT].flags = 0;
		v[0][TYPE_DBL].type = -1;
		v[0][TYPE_DBL].flags = 0;
		v[0][TYPE_ADR].type = -1;
		v[0][TYPE_ADR].flags = 0;
	}
}


/* function reg_close **********************************************************

	releases all allocated space for registers

*******************************************************************************/

void reg_close(methodinfo *m)
{
	registerdata *r;

	/* keep code size smaller */
	r = m->registerdata;

	if (r->argintregs) MFREE(r->argintregs, int, r->intreg_argnum);
	if (r->argfltregs) MFREE(r->argfltregs, int, r->fltreg_argnum);
	if (r->tmpintregs) MFREE(r->tmpintregs, int, r->tmpintregcnt);
	if (r->savintregs) MFREE(r->savintregs, int, r->savintregcnt);
	if (r->tmpfltregs) MFREE(r->tmpfltregs, int, r->tmpfltregcnt);
	if (r->savfltregs) MFREE(r->savfltregs, int, r->savfltregcnt);

	if (r->freeargintregs) MFREE(r->freeargintregs, int, r->intreg_argnum);
	if (r->freeargfltregs) MFREE(r->freeargfltregs, int, r->fltreg_argnum);
	if (r->freetmpintregs) MFREE(r->freetmpintregs, int, r->tmpintregcnt);
	if (r->freesavintregs) MFREE(r->freesavintregs, int, r->savintregcnt);
	if (r->freetmpfltregs) MFREE(r->freetmpfltregs, int, r->tmpfltregcnt);
	if (r->freesavfltregs) MFREE(r->freesavfltregs, int, r->savfltregcnt);

#ifdef USETWOREGS
	if (secondregs) MFREE(r->secondregs, int, r->intregsnum);
#endif

	if (r->freemem) MFREE(r->freemem, s4, m->maxstack);
	if (r->locals)  MFREE(r->locals, varinfo5, m->maxlocals);
	if (r->interfaces) MFREE(r->interfaces, varinfo5, m->maxstack);

	FREE(m->registerdata, registerdata);
}


/* function interface_regalloc *************************************************

	allocates registers for all interface variables
	
*******************************************************************************/
	
void regalloc(methodinfo *m)
{
#if defined(__I386__)
	/* remove %ecx and/or %edx from tmpintregs */
	int origtmpintregcnt = m->registerdata->tmpintregcnt;
	if (method_uses_ecx) m->registerdata->tmpintregcnt--;
	if (method_uses_edx) m->registerdata->tmpintregcnt--;
#endif

	interface_regalloc(m);
	allocate_scratch_registers(m);
	local_regalloc(m);

#if defined(__I386__)
	m->registerdata->tmpintregcnt = origtmpintregcnt;
#endif
}


/* function interface_regalloc *************************************************

	allocates registers for all interface variables
	
*******************************************************************************/
	
static void interface_regalloc(methodinfo *m)
{
	int     s, t, saved;
	int     intalloc, fltalloc;
	varinfo *v;
	int		regsneeded = 0;
	registerdata *r;

	/* allocate stack space for passing arguments to called methods */

	/* keep code size smaller */
	r = m->registerdata;

#ifndef SPECIALMEMUSE
#if defined(__X86_64__)
	/*
	 * XXX: we have a problem here, but allocating a little more stack space
	 *      is better than having a bug
	 */
	/*  	if (arguments_num > (intreg_argnum + fltreg_argnum)) */
	/*  		ifmemuse = arguments_num - (intreg_argnum + fltreg_argnum); */
	if (r->arguments_num > r->fltreg_argnum)
		r->ifmemuse = r->arguments_num - r->fltreg_argnum;
#else
	if (r->arguments_num > r->intreg_argnum)
		r->ifmemuse = r->arguments_num - r->intreg_argnum;
#endif
	else
		r->ifmemuse = 0;
#endif

	r->iftmpintregcnt = r->tmpintregcnt;
	r->ifsavintregcnt = r->savintregcnt;
	r->iftmpfltregcnt = r->tmpfltregcnt;
	r->ifsavfltregcnt = r->savfltregcnt;

	for (s = 0; s < m->maxstack; s++) {
		intalloc = -1; fltalloc = -1;
		saved = (r->interfaces[s][TYPE_INT].flags |
				 r->interfaces[s][TYPE_LNG].flags |
		         r->interfaces[s][TYPE_FLT].flags |
				 r->interfaces[s][TYPE_DBL].flags |
		         r->interfaces[s][TYPE_ADR].flags) & SAVEDVAR;
 
		for (t = TYPE_INT; t <= TYPE_ADR; t++) {
			v = &r->interfaces[s][t];
			if (v->type >= 0) {
#ifdef USETWOREGS
				regsneeded = (IS_2_WORD_TYPE(t)) ? 1 : 0;
#endif
				if (!saved) {
					if (IS_FLT_DBL_TYPE(t)) {
						if (fltalloc >= 0) {
							v->flags |= r->interfaces[s][fltalloc].flags & INMEMORY;
							v->regoff = r->interfaces[s][fltalloc].regoff;
						}
						else if (r->iftmpfltregcnt > 0) {
							r->iftmpfltregcnt--;
							v->regoff = r->tmpfltregs[r->iftmpfltregcnt];
						}
						else if (r->ifsavfltregcnt > 0) {
							r->ifsavfltregcnt--;
							v->regoff = r->savfltregs[r->ifsavfltregcnt];
						}
						else {
							v->flags |= INMEMORY;
							v->regoff = r->ifmemuse;
							r->ifmemuse += regsneeded + 1;
						}
						fltalloc = t;
					}
					else {
#if defined(__I386__)
						/*
						 * for i386 put all longs in memory
						 */
						if (IS_2_WORD_TYPE(t)) {
							v->flags |= INMEMORY;
							v->regoff = r->ifmemuse++;
						} else {
#endif
							if (intalloc >= 0) {
								v->flags |= r->interfaces[s][intalloc].flags & INMEMORY;
								v->regoff = r->interfaces[s][intalloc].regoff;
							}
							else if (r->iftmpintregcnt > regsneeded) {
								r->iftmpintregcnt -= regsneeded + 1;
								v->regoff = r->tmpintregs[r->iftmpintregcnt];
							}
							else if (r->ifsavintregcnt > regsneeded) {
								r->ifsavintregcnt -= regsneeded + 1;
								v->regoff = r->savintregs[r->ifsavintregcnt];
							}
							else {
								v->flags |= INMEMORY;
								v->regoff = r->ifmemuse;
								r->ifmemuse += regsneeded + 1;
							}
#if defined(__I386__)
						}
#endif
						intalloc = t;
					}
				}
				else {
					if (IS_FLT_DBL_TYPE(t)) {
						if (fltalloc >= 0) {
							v->flags |= r->interfaces[s][fltalloc].flags & INMEMORY;
							v->regoff = r->interfaces[s][fltalloc].regoff;
						}
						else if (r->ifsavfltregcnt > 0) {
							r->ifsavfltregcnt--;
							v->regoff = r->savfltregs[r->ifsavfltregcnt];
						}
						else {
							v->flags |= INMEMORY;
							v->regoff = r->ifmemuse;
							r->ifmemuse += regsneeded + 1;
						}
						fltalloc = t;
					}
					else {
#if defined(__I386__)
						/*
						 * for i386 put all longs in memory
						 */
						if (IS_2_WORD_TYPE(t)) {
							v->flags |= INMEMORY;
							v->regoff = r->ifmemuse++;
						} else {
#endif
							if (intalloc >= 0) {
								v->flags |= r->interfaces[s][intalloc].flags & INMEMORY;
								v->regoff = r->interfaces[s][intalloc].regoff;
							}
							else if (r->ifsavintregcnt > regsneeded) {
								r->ifsavintregcnt -= regsneeded + 1;
								v->regoff = r->savintregs[r->ifsavintregcnt];
							}
							else {
								v->flags |= INMEMORY;
								v->regoff = r->ifmemuse;
								r->ifmemuse += regsneeded + 1;
							}
#if defined(__I386__)
						}
#endif
						intalloc = t;
					}
				}
			} /* if (type >= 0) */
		} /* for t */
	} /* for s */

	r->maxmemuse = r->ifmemuse;
	r->maxargintreguse = -1;
	r->maxtmpintreguse = r->iftmpintregcnt;
	r->maxsavintreguse = r->ifsavintregcnt;
	r->maxargfltreguse = -1;
	r->maxtmpfltreguse = r->iftmpfltregcnt;
	r->maxsavfltreguse = r->ifsavfltregcnt;
}



/* function local_regalloc *****************************************************

	allocates registers for all local variables
	
*******************************************************************************/
	
static void local_regalloc(methodinfo *m)
{
	int      s, t, tt;
	int      intalloc, fltalloc;
	varinfo *v;
	int      regsneeded = 0;
	int typeloop[] = { TYPE_LNG, TYPE_DBL, TYPE_INT, TYPE_FLT, TYPE_ADR };
	registerdata *r;

	/* keep code size smaller */
	r = m->registerdata;

	if (m->isleafmethod) {
		int arg, doublewordarg, iargcnt, fargcnt;

		arg = 0, iargcnt = 0, fargcnt = 0;
		doublewordarg = 0;
		for (s = 0; s < m->maxlocals; s++) {
			intalloc = -1; fltalloc = -1;
			for (tt = 0; tt <= 4; tt++) {
				t = typeloop[tt];
				v = &r->locals[s][t];

				if (v->type >= 0) {
#ifdef USETWOREGS
					regsneeded = (IS_2_WORD_TYPE(t)) ? 1 : 0;
#endif
					if (IS_FLT_DBL_TYPE(t)) {
#if !defined(CONSECUTIVE_FLOATARGS)
						fargcnt = arg;
#endif
						if (fltalloc >= 0) {
							v->flags = r->locals[s][fltalloc].flags;
							v->regoff = r->locals[s][fltalloc].regoff;
						}
  						else if (!doublewordarg && (arg < m->paramcount) &&
								 (fargcnt < r->fltreg_argnum)) {
							v->flags = 0;
							v->regoff = r->argfltregs[fargcnt];
						}
						else if (r->maxtmpfltreguse > 0) {
							r->maxtmpfltreguse--;
							v->flags = 0;
							v->regoff = r->tmpfltregs[r->maxtmpfltreguse];
						}
						else if (r->maxsavfltreguse > 0) {
							r->maxsavfltreguse--;
							v->flags = 0;
							v->regoff = r->savfltregs[r->maxsavfltreguse];
						}
						else {
							v->flags = INMEMORY;
							v->regoff = r->maxmemuse;
							r->maxmemuse += regsneeded + 1;
						}
						fltalloc = t;

					} else {
						int regtouse;
#if defined(__I386__)
						/*
						 * for i386 put all longs in memory
						 */
						if (IS_2_WORD_TYPE(t)) {
							v->flags = INMEMORY;
							v->regoff = r->maxmemuse++;
						} else {
#endif
#if !defined(CONSECUTIVE_INTARGS)
							iargcnt = arg;
#endif
							if (intalloc >= 0) {
								v->flags = r->locals[s][intalloc].flags;
								v->regoff = r->locals[s][intalloc].regoff;
							}
							else if (!doublewordarg && (arg < m->paramcount)
#ifndef USETWOREGS
									 && ((regtouse = iargcnt) < r->intreg_argnum)
#else
									 && ((regtouse = s) < r->intreg_argnum - regsneeded)
#endif
									 ) {
								v->flags = 0;
								v->regoff = r->argintregs[regtouse];
							}
							else if (r->maxtmpintreguse > regsneeded) {
								r->maxtmpintreguse -= regsneeded + 1;
								v->flags = 0;
								v->regoff = r->tmpintregs[r->maxtmpintreguse];
							}
							else if (r->maxsavintreguse > regsneeded) {
								r->maxsavintreguse -= regsneeded + 1;
								v->flags = 0;
								v->regoff = r->savintregs[r->maxsavintreguse];
							}
							/*
							 * use unused argument registers as local registers
							 */
							else if (!doublewordarg && (arg >= m->paramcount) &&
									 (iargcnt < r->intreg_argnum)) {
								v->flags = 0;
								v->regoff = r->argintregs[iargcnt];
  								iargcnt++;
								arg++;
							}
							else {
								v->flags = INMEMORY;
								v->regoff = r->maxmemuse;
								r->maxmemuse += regsneeded + 1;
							}
#if defined(__I386__)
						}
#endif
						intalloc = t;
					}
				}
			}
			if (arg < m->paramcount) {
				if (doublewordarg) {
					doublewordarg = 0;
					/* what type was the double arg? */
					if (IS_FLT_DBL_TYPE(m->paramtypes[arg])) {
						fargcnt++;

					} else {
						iargcnt++;
					}
					arg++;

				} else if (IS_2_WORD_TYPE(m->paramtypes[arg])) {
					doublewordarg = 1;

				} else {
					if (IS_FLT_DBL_TYPE(m->paramtypes[arg])) {
						fargcnt++;

					} else {
						iargcnt++;
					}
					arg++;
				}
			}
		}
		return;
	}

	for (s = 0; s < m->maxlocals; s++) {
		intalloc = -1; fltalloc = -1;
		for (tt=0; tt<=4; tt++) {
			t = typeloop[tt];
			v = &r->locals[s][t];
			if (v->type >= 0) {
#ifdef USETWOREGS
				regsneeded = (IS_2_WORD_TYPE(t)) ? 1 : 0;
#endif
				if (IS_FLT_DBL_TYPE(t)) {
					if (fltalloc >= 0) {
						v->flags = r->locals[s][fltalloc].flags;
						v->regoff = r->locals[s][fltalloc].regoff;
					}
					else if (r->maxsavfltreguse > 0) {
						r->maxsavfltreguse--;
						v->flags = 0;
						v->regoff = r->savfltregs[r->maxsavfltreguse];
					}
					else {
						v->flags = INMEMORY;
						v->regoff = r->maxmemuse;
						r->maxmemuse += regsneeded + 1;
					}
					fltalloc = t;
				}
				else {
#if defined(__I386__)
					/*
					 * for i386 put all longs in memory
					 */
					if (IS_2_WORD_TYPE(t)) {
						v->flags = INMEMORY;
						v->regoff = r->maxmemuse++;
					} else {
#endif
						if (intalloc >= 0) {
							v->flags = r->locals[s][intalloc].flags;
							v->regoff = r->locals[s][intalloc].regoff;
						}
						else if (r->maxsavintreguse > regsneeded) {
							r->maxsavintreguse -= regsneeded+1;
							v->flags = 0;
							v->regoff = r->savintregs[r->maxsavintreguse];
						}
						else {
							v->flags = INMEMORY;
							v->regoff = r->maxmemuse;
							r->maxmemuse += regsneeded + 1;
						}
#if defined(__I386__)
					}
#endif
					intalloc = t;
				}
			}
		}
	}
}



static void reg_init_temp(methodinfo *m)
{
	registerdata *r;

	/* keep code size smaller */
	r = m->registerdata;

	r->freememtop = 0;
	r->memuse = r->ifmemuse;

	r->freetmpinttop = 0;
	r->freesavinttop = 0;
	r->freetmpflttop = 0;
	r->freesavflttop = 0;

	r->tmpintreguse = r->iftmpintregcnt;
	r->savintreguse = r->ifsavintregcnt;
	r->tmpfltreguse = r->iftmpfltregcnt;
	r->savfltreguse = r->ifsavfltregcnt;

	/* all argument registers are available */
	r->argintreguse = r->intreg_argnum;
	r->argfltreguse = r->fltreg_argnum;
}


#define reg_new_temp(m,s) if (s->varkind == TEMPVAR) reg_new_temp_func(m, s)


static void reg_new_temp_func(methodinfo *m, stackptr s)
{
	s4 regsneeded = 0;
	s4 tryagain;
	registerdata *r;

	/* keep code size smaller */
	r = m->registerdata;

	/* Try to allocate a saved register if there is no temporary one          */
	/* available. This is what happens during the second run.                 */
	tryagain = (s->flags & SAVEDVAR) ? 1 : 2;

#ifdef USETWOREGS
	regsneeded = (IS_2_WORD_TYPE(s->type)) ? 1 : 0;
#endif

	for(; tryagain; --tryagain) {
		if (tryagain == 1) {
			if (!(s->flags & SAVEDVAR))
				s->flags |= SAVEDTMP;
			if (IS_FLT_DBL_TYPE(s->type)) {
				if (r->freesavflttop > 0) {
					r->freesavflttop--;
					s->regoff = r->freesavfltregs[r->freesavflttop];
					return;
				}
				else if (r->savfltreguse > 0) {
					r->savfltreguse--;
					if (r->savfltreguse < r->maxsavfltreguse)
						r->maxsavfltreguse = r->savfltreguse;
					s->regoff = r->savfltregs[r->savfltreguse];
					return;
				}
			}
			else {
#if defined(__I386__)
				/*
				 * for i386 put all longs in memory
				 */
				if (!IS_2_WORD_TYPE(s->type)) {
#endif
					if (r->freesavinttop > regsneeded) {
						r->freesavinttop -= regsneeded + 1;
						s->regoff = r->freesavintregs[r->freesavinttop];
						return;
					}
					else if (r->savintreguse > regsneeded) {
						r->savintreguse -= regsneeded + 1;
						if (r->savintreguse < r->maxsavintreguse)
							r->maxsavintreguse = r->savintreguse;
						s->regoff = r->savintregs[r->savintreguse];
						return;
					}
#if defined(__I386__)
				}
#endif
			}
		}
		else {
			if (IS_FLT_DBL_TYPE(s->type)) {
				if (r->freetmpflttop > 0) {
					r->freetmpflttop--;
					s->regoff = r->freetmpfltregs[r->freetmpflttop];
					return;
				}
				else if (r->tmpfltreguse > 0) {
					r->tmpfltreguse--;
					if (r->tmpfltreguse < r->maxtmpfltreguse)
						r->maxtmpfltreguse = r->tmpfltreguse;
					s->regoff = r->tmpfltregs[r->tmpfltreguse];
					return;
				}
			}
			else {
#if defined(__I386__)
				/*
				 * for i386 put all longs in memory
				 */
				if (!IS_2_WORD_TYPE(s->type)) {
#endif
					if (r->freetmpinttop > regsneeded) {
						r->freetmpinttop -= regsneeded + 1;
						s->regoff = r->freetmpintregs[r->freetmpinttop];
						return;
					}
					else if (r->tmpintreguse > regsneeded) {
						r->tmpintreguse -= regsneeded + 1;
						if (r->tmpintreguse < r->maxtmpintreguse)
							r->maxtmpintreguse = r->tmpintreguse;
						s->regoff = r->tmpintregs[r->tmpintreguse];
						return;
					}
#if defined(__I386__)
				}
#endif
			}
		}
	}

	if (r->freememtop > regsneeded) {
		r->freememtop -= regsneeded + 1;
		s->regoff = r->freemem[r->freememtop];
	}
	else {
		s->regoff = r->memuse;
		r->memuse += regsneeded + 1;
		if (r->memuse > r->maxmemuse)
			r->maxmemuse = r->memuse;
	}
	s->flags |= INMEMORY;
}


#define reg_free_temp(m,s) if (s->varkind == TEMPVAR) reg_free_temp_func(m, s)


static void reg_free_temp_func(methodinfo *m, stackptr s)
{
	s4 regsneeded = 0;
	registerdata *r;

	/* keep code size smaller */
	r = m->registerdata;

#ifdef USETWOREGS
	regsneeded = (IS_2_WORD_TYPE(s->type)) ? 1 : 0;
#endif

	if (s->flags & INMEMORY) {
		r->freemem[r->freememtop] = s->regoff;
		if (regsneeded)
			r->freemem[r->freememtop + 1] = s->regoff + 1;
		r->freememtop += regsneeded + 1;
	}
	else if (IS_FLT_DBL_TYPE(s->type)) {
		if (s->flags & (SAVEDVAR | SAVEDTMP)) {
			s->flags &= ~SAVEDTMP;
			r->freesavfltregs[r->freesavflttop++] = s->regoff;
		} else
			r->freetmpfltregs[r->freetmpflttop++] = s->regoff;
	}
	else {
		if (s->flags & (SAVEDVAR | SAVEDTMP)) {
			s->flags &= ~SAVEDTMP;
			r->freesavintregs[r->freesavinttop] = s->regoff;
#ifdef USETWOREGS
			if (regsneeded)
				r->freesavintregs[r->freesavinttop + 1] = r->secondregs[s->regoff];
#endif
			r->freesavinttop += regsneeded + 1;
		} else {
			r->freetmpintregs[r->freetmpinttop] = s->regoff;
#ifdef USETWOREGS
			if (regsneeded)
				r->freetmpintregs[r->freetmpinttop + 1] = r->secondregs[s->regoff];
#endif
			r->freetmpinttop += regsneeded + 1;
		}
	}
}



static void allocate_scratch_registers(methodinfo *m)
{
	int opcode;
	int i;
	int len;
	stackptr    src;
	stackptr    dst;
	instruction *iptr;
	basicblock  *bptr;

	bptr = m->basicblocks;

	while (bptr != NULL) {
		if (bptr->flags >= BBREACHED) {
			dst = bptr->instack;

			/* initialize temp registers */
			reg_init_temp(m);

			iptr = bptr->iinstr;
			len = bptr->icount;
  
			while (--len >= 0)  {
				src = dst;
				dst = iptr->dst;
				opcode = iptr->opc;

				switch (opcode) {

					/* pop 0 push 0 */

				case ICMD_NOP:
				case ICMD_ELSE_ICONST:
				case ICMD_CHECKASIZE:
				case ICMD_CHECKEXCEPTION:
				case ICMD_IINC:
				case ICMD_JSR:
				case ICMD_RET:
				case ICMD_RETURN:
				case ICMD_GOTO:
					break;

					/* pop 0 push 1 const */
					
				case ICMD_ICONST:
				case ICMD_LCONST:
				case ICMD_FCONST:
				case ICMD_DCONST:
				case ICMD_ACONST:

					/* pop 0 push 1 load */
					
				case ICMD_ILOAD:
				case ICMD_LLOAD:
				case ICMD_FLOAD:
				case ICMD_DLOAD:
				case ICMD_ALOAD:
					reg_new_temp(m, dst);
					break;

					/* pop 2 push 1 */

				case ICMD_IALOAD:
				case ICMD_LALOAD:
				case ICMD_FALOAD:
				case ICMD_DALOAD:
				case ICMD_AALOAD:

				case ICMD_BALOAD:
				case ICMD_CALOAD:
				case ICMD_SALOAD:

					reg_free_temp(m, src);
					reg_free_temp(m, src->prev);
					reg_new_temp(m, dst);
					break;

					/* pop 3 push 0 */

				case ICMD_IASTORE:
				case ICMD_LASTORE:
				case ICMD_FASTORE:
				case ICMD_DASTORE:
				case ICMD_AASTORE:

				case ICMD_BASTORE:
				case ICMD_CASTORE:
				case ICMD_SASTORE:

					reg_free_temp(m, src);
					reg_free_temp(m, src->prev);
					reg_free_temp(m, src->prev->prev);
					break;

					/* pop 1 push 0 store */

				case ICMD_ISTORE:
				case ICMD_LSTORE:
				case ICMD_FSTORE:
				case ICMD_DSTORE:
				case ICMD_ASTORE:

					/* pop 1 push 0 */

				case ICMD_POP:

				case ICMD_IRETURN:
				case ICMD_LRETURN:
				case ICMD_FRETURN:
				case ICMD_DRETURN:
				case ICMD_ARETURN:

				case ICMD_ATHROW:

				case ICMD_PUTSTATIC:

					/* pop 1 push 0 branch */

				case ICMD_IFNULL:
				case ICMD_IFNONNULL:

				case ICMD_IFEQ:
				case ICMD_IFNE:
				case ICMD_IFLT:
				case ICMD_IFGE:
				case ICMD_IFGT:
				case ICMD_IFLE:

				case ICMD_IF_LEQ:
				case ICMD_IF_LNE:
				case ICMD_IF_LLT:
				case ICMD_IF_LGE:
				case ICMD_IF_LGT:
				case ICMD_IF_LLE:

					/* pop 1 push 0 table branch */

				case ICMD_TABLESWITCH:
				case ICMD_LOOKUPSWITCH:

				case ICMD_NULLCHECKPOP:
				case ICMD_MONITORENTER:
				case ICMD_MONITOREXIT:
					reg_free_temp(m, src);
					break;

					/* pop 2 push 0 branch */

				case ICMD_IF_ICMPEQ:
				case ICMD_IF_ICMPNE:
				case ICMD_IF_ICMPLT:
				case ICMD_IF_ICMPGE:
				case ICMD_IF_ICMPGT:
				case ICMD_IF_ICMPLE:

				case ICMD_IF_LCMPEQ:
				case ICMD_IF_LCMPNE:
				case ICMD_IF_LCMPLT:
				case ICMD_IF_LCMPGE:
				case ICMD_IF_LCMPGT:
				case ICMD_IF_LCMPLE:

				case ICMD_IF_ACMPEQ:
				case ICMD_IF_ACMPNE:

					/* pop 2 push 0 */

				case ICMD_POP2:

				case ICMD_PUTFIELD:

				case ICMD_IASTORECONST:
				case ICMD_LASTORECONST:
				case ICMD_AASTORECONST:
				case ICMD_BASTORECONST:
				case ICMD_CASTORECONST:
				case ICMD_SASTORECONST:
					reg_free_temp(m, src);
					reg_free_temp(m, src->prev);
					break;

					/* pop 0 push 1 dup */
					
				case ICMD_DUP:
					reg_new_temp(m, dst);
					break;

					/* pop 0 push 2 dup */
					
				case ICMD_DUP2:
					reg_new_temp(m, dst->prev);
					reg_new_temp(m, dst);
					break;

					/* pop 2 push 3 dup */
					
				case ICMD_DUP_X1:
					reg_new_temp(m, dst->prev->prev);
					reg_new_temp(m, dst->prev);
					reg_new_temp(m, dst);
					reg_free_temp(m, src);
					reg_free_temp(m, src->prev);
					break;

					/* pop 3 push 4 dup */
					
				case ICMD_DUP_X2:
					reg_new_temp(m, dst->prev->prev->prev);
					reg_new_temp(m, dst->prev->prev);
					reg_new_temp(m, dst->prev);
					reg_new_temp(m, dst);
					reg_free_temp(m, src);
					reg_free_temp(m, src->prev);
					reg_free_temp(m, src->prev->prev);
					break;

					/* pop 3 push 5 dup */
					
				case ICMD_DUP2_X1:
					reg_new_temp(m, dst->prev->prev->prev->prev);
					reg_new_temp(m, dst->prev->prev->prev);
					reg_new_temp(m, dst->prev->prev);
					reg_new_temp(m, dst->prev);
					reg_new_temp(m, dst);
					reg_free_temp(m, src);
					reg_free_temp(m, src->prev);
					reg_free_temp(m, src->prev->prev);
					break;

					/* pop 4 push 6 dup */
					
				case ICMD_DUP2_X2:
					reg_new_temp(m, dst->prev->prev->prev->prev->prev);
					reg_new_temp(m, dst->prev->prev->prev->prev);
					reg_new_temp(m, dst->prev->prev->prev);
					reg_new_temp(m, dst->prev->prev);
					reg_new_temp(m, dst->prev);
					reg_new_temp(m, dst);
					reg_free_temp(m, src);
					reg_free_temp(m, src->prev);
					reg_free_temp(m, src->prev->prev);
					reg_free_temp(m, src->prev->prev->prev);
					break;

					/* pop 2 push 2 swap */
					
				case ICMD_SWAP:
					reg_new_temp(m, dst->prev);
					reg_new_temp(m, dst);
					reg_free_temp(m, src);
					reg_free_temp(m, src->prev);
					break;

					/* pop 2 push 1 */
					
				case ICMD_IADD:
				case ICMD_ISUB:
				case ICMD_IMUL:
				case ICMD_IDIV:
				case ICMD_IREM:

				case ICMD_ISHL:
				case ICMD_ISHR:
				case ICMD_IUSHR:
				case ICMD_IAND:
				case ICMD_IOR:
				case ICMD_IXOR:

				case ICMD_LADD:
				case ICMD_LSUB:
				case ICMD_LMUL:
				case ICMD_LDIV:
				case ICMD_LREM:

				case ICMD_LOR:
				case ICMD_LAND:
				case ICMD_LXOR:

				case ICMD_LSHL:
				case ICMD_LSHR:
				case ICMD_LUSHR:

				case ICMD_FADD:
				case ICMD_FSUB:
				case ICMD_FMUL:
				case ICMD_FDIV:
				case ICMD_FREM:

				case ICMD_DADD:
				case ICMD_DSUB:
				case ICMD_DMUL:
				case ICMD_DDIV:
				case ICMD_DREM:

				case ICMD_LCMP:
				case ICMD_FCMPL:
				case ICMD_FCMPG:
				case ICMD_DCMPL:
				case ICMD_DCMPG:
					reg_free_temp(m, src);
					reg_free_temp(m, src->prev);
					reg_new_temp(m, dst);
					break;

					/* pop 1 push 1 */
					
				case ICMD_IADDCONST:
				case ICMD_ISUBCONST:
				case ICMD_IMULCONST:
				case ICMD_IDIVPOW2:
				case ICMD_IREMPOW2:
				case ICMD_IANDCONST:
				case ICMD_IORCONST:
				case ICMD_IXORCONST:
				case ICMD_ISHLCONST:
				case ICMD_ISHRCONST:
				case ICMD_IUSHRCONST:

				case ICMD_LADDCONST:
				case ICMD_LSUBCONST:
				case ICMD_LMULCONST:
				case ICMD_LDIVPOW2:
				case ICMD_LREMPOW2:
				case ICMD_LANDCONST:
				case ICMD_LORCONST:
				case ICMD_LXORCONST:
				case ICMD_LSHLCONST:
				case ICMD_LSHRCONST:
				case ICMD_LUSHRCONST:

				case ICMD_IFEQ_ICONST:
				case ICMD_IFNE_ICONST:
				case ICMD_IFLT_ICONST:
				case ICMD_IFGE_ICONST:
				case ICMD_IFGT_ICONST:
				case ICMD_IFLE_ICONST:

				case ICMD_INEG:
				case ICMD_INT2BYTE:
				case ICMD_INT2CHAR:
				case ICMD_INT2SHORT:
				case ICMD_LNEG:
				case ICMD_FNEG:
				case ICMD_DNEG:

				case ICMD_I2L:
				case ICMD_I2F:
				case ICMD_I2D:
				case ICMD_L2I:
				case ICMD_L2F:
				case ICMD_L2D:
				case ICMD_F2I:
				case ICMD_F2L:
				case ICMD_F2D:
				case ICMD_D2I:
				case ICMD_D2L:
				case ICMD_D2F:

				case ICMD_CHECKCAST:

				case ICMD_ARRAYLENGTH:
				case ICMD_INSTANCEOF:

				case ICMD_NEWARRAY:
				case ICMD_ANEWARRAY:

				case ICMD_GETFIELD:
					reg_free_temp(m, src);
					reg_new_temp(m, dst);
					break;

					/* pop 0 push 1 */
					
				case ICMD_GETSTATIC:

				case ICMD_NEW:

					reg_new_temp(m, dst);
					break;

					/* pop many push any */
					
				case ICMD_INVOKEVIRTUAL:
				case ICMD_INVOKESPECIAL:
				case ICMD_INVOKESTATIC:
				case ICMD_INVOKEINTERFACE:
					{
						i = iptr->op1;
						while (--i >= 0) {
							reg_free_temp(m, src);
							src = src->prev;
						}
						if (((methodinfo*)iptr->val.a)->returntype != TYPE_VOID)
							reg_new_temp(m, dst);
						break;
					}

				case ICMD_BUILTIN3:
					reg_free_temp(m, src);
					src = src->prev;
				case ICMD_BUILTIN2:
					reg_free_temp(m, src);
					src = src->prev;
				case ICMD_BUILTIN1:
					reg_free_temp(m, src);
					src = src->prev;
					if (iptr->op1 != TYPE_VOID)
						reg_new_temp(m, dst);
					break;

				case ICMD_MULTIANEWARRAY:
					i = iptr->op1;
					while (--i >= 0) {
						reg_free_temp(m, src);
						src = src->prev;
					}
					reg_new_temp(m, dst);
					break;

				default:
					printf("ICMD %d at %d\n", iptr->opc, (s4) (iptr - m->instructions));
					panic("Missing ICMD code during register allocation");
				} /* switch */
				iptr++;
			} /* while instructions */
		} /* if */
		bptr = bptr->next;
	} /* while blocks */
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
